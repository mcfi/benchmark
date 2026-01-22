#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

std::mutex mtx;
std::condition_variable cv;
bool server_ready = false;
std::mutex cout_mutex;

void server_thread(int port, int buffer_size, int num_iter) {
    int sock, conn;
    struct sockaddr_in serv_addr, cli_addr;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Server: Socket creation failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Server: setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    if (bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Server: Socket binding failed: " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    if (listen(sock, 1) < 0) {
        std::cerr << "Server: Socket listening failed: " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    {
        std::unique_lock<std::mutex> lk(mtx);
        server_ready = true;
        cv.notify_all();
    }

    socklen_t clilen = sizeof(cli_addr);
    conn = accept(sock, (struct sockaddr *)&cli_addr, &clilen);
    if (conn < 0) {
        std::cerr << "Server: Socket accept failed: " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    void* ptr = mmap(nullptr, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "mmap failed!" << std::endl;
        exit(1);
    }
    char* buf = static_cast<char*>(ptr);
    memset(buf, 'x', buffer_size);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_iter; ++i) {
        ssize_t sent = send(conn, buf, buffer_size, 0);
        if (sent < 0) {
            std::cerr << "Server: Send failed at iteration " << i << ": " << strerror(errno) << std::endl;
            break;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    {
        std::unique_lock<std::mutex> lk(cout_mutex);
        std::cout << "Server: Sent " << num_iter << " buffers in "
                << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                << " ms" << std::endl;
    }
    close(conn);
    close(sock);
    if (munmap(ptr, buffer_size) != 0) {
        std::cerr << "munmap failed!" << std::endl;
        exit(1);
    }
}

void client_thread(int port, int buffer_size, int num_iter) {
    int sock;
    struct sockaddr_in serv_addr;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

    {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, []{ return server_ready; });
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        std::cerr << "Client: Socket creation failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Client: Socket connection failed: " << strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    void* ptr = mmap(nullptr, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        std::cerr << "mmap failed!" << std::endl;
        exit(1);
    }
    char* buf = static_cast<char*>(ptr);

    auto start = std::chrono::high_resolution_clock::now();
    int received = 0;
    for (int i = 0; i < num_iter; ++i) {
        while (received < buffer_size) {
            ssize_t bytes_received = recv(sock, buf + received, buffer_size - received, 0);
            if (bytes_received < 0) {
                std::cerr << "Client: Receive failed at iteration " << i << ": " << strerror(errno) << std::endl;
                close(sock);
                delete[] buf;
                exit(1);
            }
            if (bytes_received == 0) {
                std::cerr << "Client: Connection closed by server at iteration " << i << std::endl;
                break;
            }
            received += bytes_received;
        }
        received = 0;
    }
    auto end = std::chrono::high_resolution_clock::now();

    {
        std::unique_lock<std::mutex> lk(cout_mutex);
        std::cout << "Client: Received " << num_iter << " buffers in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;
    }

    close(sock);
    if (munmap(ptr, buffer_size) != 0) {
        std::cerr << "munmap failed!" << std::endl;
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    int port = 50007;
    int buffer_size = 4096; // Default 4KB
    int num_iter = 1000;    // Default 1000

    // Parse command line arguments
    if (argc > 1) buffer_size = std::atoi(argv[1]);
    if (argc > 2) num_iter = std::atoi(argv[2]);

    if (num_iter < 1) {
        std::cerr << "Number of iterations must be positive." << std::endl;
        return 1;
    }

    std::cout << "Using buffer_size=" << buffer_size << " bytes, num_iter=" << num_iter << std::endl;

    std::thread serv(server_thread, port, buffer_size, num_iter);
    std::thread cli(client_thread, port, buffer_size, num_iter);

    serv.join();
    cli.join();

    return 0;
}

