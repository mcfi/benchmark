// umemcpy_bench.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define DoNotOptimize(value) asm volatile("" : "=r"(value) : "0"(value))

static void
die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/*
 * Parse sizes like:
 *   4096
 *   64K, 1M, 2G
 */
static size_t
parse_size(const char *s)
{
    char *end;
    errno = 0;
    unsigned long long val = strtoull(s, &end, 10);
    if (errno != 0 || end == s) {
        fprintf(stderr, "Invalid size: %s\n", s);
        exit(EXIT_FAILURE);
    }

    switch (*end) {
    case 'k': case 'K':
        val *= 1024ULL;
        break;
    case 'm': case 'M':
        val *= 1024ULL * 1024ULL;
        break;
    case 'g': case 'G':
        val *= 1024ULL * 1024ULL * 1024ULL;
        break;
    case '\0':
        break;
    default:
        fprintf(stderr, "Unknown size suffix '%c' in \"%s\"\n", *end, s);
        exit(EXIT_FAILURE);
    }

    return (size_t)val;
}

static double
timespec_diff_sec(const struct timespec *start, const struct timespec *end)
{
    long sec  = end->tv_sec  - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    if (nsec < 0) {
        sec -= 1;
        nsec += 1000000000L;
    }
    return (double)sec + (double)nsec / 1e9;
}

static void
usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s <size> [iterations]\n"
        "\n"
        "  <size>       bytes per memcpy, allow K/M/G suffix, e.g. 64K, 1M, 256M\n"
        "  [iterations] number of memcpy calls (default: 100000)\n"
        "\n"
        "Example:\n"
        "  %s 1M 200000\n",
        prog, prog);
}

/*
 * Simple memcpy microbenchmark.
 *
 * - Allocates two aligned buffers (64-byte aligned).
 * - Touches them to fault in pages.
 * - Does a few warmup copies.
 * - Times a tight loop of memcpy(dst, src, size).
 * - Prints total bytes, elapsed seconds, and GB/s.
 */
int
main(int argc, char **argv)
{
    if (argc < 2 || argc > 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    size_t size = parse_size(argv[1]);
    unsigned long long iters = 100000ULL;
    if (argc == 3) {
        char *end;
        errno = 0;
        iters = strtoull(argv[2], &end, 10);
        if (errno != 0 || end == argv[2] || *end != '\0' || iters == 0) {
            fprintf(stderr, "Invalid iteration count: %s\n", argv[2]);
            return EXIT_FAILURE;
        }
    }

    if (size == 0) {
        fprintf(stderr, "Size must be > 0\n");
        return EXIT_FAILURE;
    }

    printf("memcpy benchmark:\n");
    printf("  size       = %zu bytes\n", size);
    printf("  iterations = %llu\n", iters);

    void *src = NULL;
    void *dst = NULL;

    // 64-byte alignment to be nice to caches / SIMD.
    if (posix_memalign(&src, 64, size) != 0)
        die("posix_memalign(src)");
    if (posix_memalign(&dst, 64, size) != 0)
        die("posix_memalign(dst)");

    // Touch pages and seed src with non-zero data.
    memset(src, 0xA5, size);
    memset(dst, 0x00, size);

    // Warmup (avoid cold start artifacts).
    for (int i = 0; i < 10; ++i) {
        memcpy(dst, src, size);
    }

    struct timespec t0, t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0)
        die("clock_gettime start");

    // Main timed loop.
    for (unsigned long long i = 0; i < iters; ++i) {
        char* p = memcpy(dst, src, size);
	DoNotOptimize(p);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0)
        die("clock_gettime end");

    double elapsed = timespec_diff_sec(&t0, &t1);
    double bytes   = (double)size * (double)iters;
    double gb      = bytes / (1024.0 * 1024.0 * 1024.0);
    double gbps    = gb / elapsed;

    // Prevent compiler from optimizing copies away.
    volatile unsigned char *vdst = (volatile unsigned char *)dst;
    unsigned char sink = vdst[0] ^ vdst[size / 2] ^ vdst[size - 1];

    printf("\nResults:\n");
    printf("  elapsed     = %.6f s\n", elapsed);
    printf("  total_bytes = %.3f GiB\n", gb);
    printf("  bandwidth   = %.3f GiB/s\n", gbps);
    printf("  sink byte   = %u (ignore, prevents optimization)\n", sink);

    free(src);
    free(dst);
    return 0;
}

