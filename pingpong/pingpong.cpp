#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

std::mutex mtx;
std::condition_variable cv;
bool server_ready = false;
std::mutex cout_mutex;

void server_thread(int port, int buffer_size, int num_iter, bool use_ipv6) {
  int sock, conn;
  struct sockaddr_in serv_addr4, cli_addr4;
  struct sockaddr_in6 serv_addr6, cli_addr6;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

  int domain = use_ipv6 ? AF_INET6 : AF_INET;
  sock = socket(domain, SOCK_STREAM, 0);
  if (sock < 0) {
    std::cerr << "Server: Socket creation failed: " << strerror(errno)
              << std::endl;
    exit(1);
  }

  int opt = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    std::cerr << "Server: setsockopt(SO_REUSEADDR) failed: " << strerror(errno)
              << std::endl;
    close(sock);
    exit(1);
  }

  if (use_ipv6) {
    memset(&serv_addr6, 0, sizeof(serv_addr6));
    serv_addr6.sin6_family = AF_INET6;
    serv_addr6.sin6_port = htons(port);
    serv_addr6.sin6_addr = in6addr_any;

    if (bind(sock, (struct sockaddr *)&serv_addr6, sizeof(serv_addr6)) < 0) {
      std::cerr << "Server: Socket binding failed: " << strerror(errno)
                << std::endl;
      close(sock);
      exit(1);
    }
  } else {
    memset(&serv_addr4, 0, sizeof(serv_addr4));
    serv_addr4.sin_family = AF_INET;
    serv_addr4.sin_port = htons(port);
    serv_addr4.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&serv_addr4, sizeof(serv_addr4)) < 0) {
      std::cerr << "Server: Socket binding failed: " << strerror(errno)
                << std::endl;
      close(sock);
      exit(1);
    }
  }

  if (listen(sock, 1) < 0) {
    std::cerr << "Server: Socket listening failed: " << strerror(errno)
              << std::endl;
    close(sock);
    exit(1);
  }

  {
    std::unique_lock<std::mutex> lk(mtx);
    server_ready = true;
    cv.notify_all();
  }

  socklen_t clilen;
  if (use_ipv6) {
    clilen = sizeof(cli_addr6);
    conn = accept(sock, (struct sockaddr *)&cli_addr6, &clilen);
  } else {
    clilen = sizeof(cli_addr4);
    conn = accept(sock, (struct sockaddr *)&cli_addr4, &clilen);
  }
  if (conn < 0) {
    std::cerr << "Server: Socket accept failed: " << strerror(errno)
              << std::endl;
    close(sock);
    exit(1);
  }

  void *ptr = mmap(nullptr, buffer_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    std::cerr << "mmap failed!" << std::endl;
    exit(1);
  }
  char *buf = static_cast<char *>(ptr);
  memset(buf, 'x', buffer_size);

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < num_iter; ++i) {
    ssize_t sent = send(conn, buf, buffer_size, 0);
    if (sent < 0) {
      std::cerr << "Server: Send failed at iteration " << i << ": "
                << strerror(errno) << std::endl;
      break;
    }
  }
  auto end = std::chrono::high_resolution_clock::now();

  {
    std::unique_lock<std::mutex> lk(cout_mutex);
    std::cout << "Server: Sent " << num_iter << " buffers in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       start)
                     .count()
              << " ms" << std::endl;
  }
  close(conn);
  close(sock);
  if (munmap(ptr, buffer_size) != 0) {
    std::cerr << "munmap failed!" << std::endl;
    exit(1);
  }
}

void client_thread(int port, int buffer_size, int num_iter, bool use_ipv6) {
  int sock;
  struct sockaddr_in serv_addr4;
  struct sockaddr_in6 serv_addr6;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(2, &cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

  {
    std::unique_lock<std::mutex> lk(mtx);
    cv.wait(lk, [] { return server_ready; });
  }

  int domain = use_ipv6 ? AF_INET6 : AF_INET;
  sock = socket(domain, SOCK_STREAM, 0);

  if (sock < 0) {
    std::cerr << "Client: Socket creation failed: " << strerror(errno)
              << std::endl;
    exit(1);
  }

  if (use_ipv6) {
    memset(&serv_addr6, 0, sizeof(serv_addr6));
    serv_addr6.sin6_family = AF_INET6;
    serv_addr6.sin6_port = htons(port);
    serv_addr6.sin6_addr = in6addr_loopback;

    if (connect(sock, (struct sockaddr *)&serv_addr6, sizeof(serv_addr6)) < 0) {
      std::cerr << "Client: Socket connection failed: " << strerror(errno)
                << std::endl;
      close(sock);
      exit(1);
    }
  } else {
    memset(&serv_addr4, 0, sizeof(serv_addr4));
    serv_addr4.sin_family = AF_INET;
    serv_addr4.sin_port = htons(port);
    serv_addr4.sin_addr.s_addr = INADDR_ANY;

    if (connect(sock, (struct sockaddr *)&serv_addr4, sizeof(serv_addr4)) < 0) {
      std::cerr << "Client: Socket connection failed: " << strerror(errno)
                << std::endl;
      close(sock);
      exit(1);
    }
  }

  void *ptr = mmap(nullptr, buffer_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    std::cerr << "mmap failed!" << std::endl;
    exit(1);
  }
  char *buf = static_cast<char *>(ptr);

  auto start = std::chrono::high_resolution_clock::now();
  int received = 0;
  for (int i = 0; i < num_iter; ++i) {
    while (received < buffer_size) {
      ssize_t bytes_received =
          recv(sock, buf + received, buffer_size - received, 0);
      if (bytes_received < 0) {
        std::cerr << "Client: Receive failed at iteration " << i << ": "
                  << strerror(errno) << std::endl;
        close(sock);
        delete[] buf;
        exit(1);
      }
      if (bytes_received == 0) {
        std::cerr << "Client: Connection closed by server at iteration " << i
                  << std::endl;
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
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       start)
                     .count()
              << " ms" << std::endl;
  }

  close(sock);
  if (munmap(ptr, buffer_size) != 0) {
    std::cerr << "munmap failed!" << std::endl;
    exit(1);
  }
}

void print_usage(const char *prog_name) {
  std::cerr
      << "Usage: " << prog_name << " [options]\n"
      << "Options:\n"
      << "  -b, --buffer-size <size>   Buffer size in bytes (default: 4096)\n"
      << "  -n, --num-iter <count>     Number of iterations (default: 1000)\n"
      << "  -4, --ipv4                 Use IPv4\n"
      << "  -6, --ipv6                 Use IPv6 (default)\n"
      << "  -h, --help                 Show this help message\n";
}

int main(int argc, char *argv[]) {
  int port = 50007;
  int buffer_size = 4096;
  int num_iter = 1000;
  bool use_ipv6 = true;

  static struct option long_options[] = {
      {"buffer-size", required_argument, nullptr, 'b'},
      {"num-iter", required_argument, nullptr, 'n'},
      {"ipv4", no_argument, nullptr, '4'},
      {"ipv6", no_argument, nullptr, '6'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "b:n:46h", long_options, nullptr)) !=
         -1) {
    switch (opt) {
    case 'b':
      buffer_size = std::atoi(optarg);
      break;
    case 'n':
      num_iter = std::atoi(optarg);
      break;
    case '4':
      use_ipv6 = false;
      break;
    case '6':
      use_ipv6 = true;
      break;
    case 'h':
      print_usage(argv[0]);
      return 0;
    default:
      print_usage(argv[0]);
      return 1;
    }
  }

  if (buffer_size < 1) {
    std::cerr << "Buffer size must be positive." << std::endl;
    return 1;
  }

  if (num_iter < 1) {
    std::cerr << "Number of iterations must be positive." << std::endl;
    return 1;
  }

  std::cout << "Using buffer_size=" << buffer_size
            << " bytes, num_iter=" << num_iter << ", "
            << (use_ipv6 ? "IPv6" : "IPv4") << std::endl;

  std::thread serv(server_thread, port, buffer_size, num_iter, use_ipv6);
  std::thread cli(client_thread, port, buffer_size, num_iter, use_ipv6);

  serv.join();
  cli.join();

  return 0;
}
