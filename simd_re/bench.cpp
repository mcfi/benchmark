#include <benchmark/benchmark.h>
#include <cstdint>

///////////////////////////////////////////////////////////////
// Architecture-specific function declarations
///////////////////////////////////////////////////////////////

#if defined(__x86_64__) || defined(_M_X64)

// SSE2
extern "C" {
void sse2_unroll_1(std::uint64_t);
void sse2_unroll_2(std::uint64_t);
void sse2_unroll_3(std::uint64_t);
void sse2_unroll_4(std::uint64_t);
void sse2_unroll_5(std::uint64_t);
void sse2_unroll_6(std::uint64_t);
void sse2_unroll_7(std::uint64_t);
void sse2_unroll_8(std::uint64_t);
}

// AVX / AVX2
#ifdef __AVX__
extern "C" {
void avx_unroll_1(std::uint64_t);
void avx_unroll_2(std::uint64_t);
void avx_unroll_3(std::uint64_t);
void avx_unroll_4(std::uint64_t);
void avx_unroll_5(std::uint64_t);
void avx_unroll_6(std::uint64_t);
void avx_unroll_7(std::uint64_t);
void avx_unroll_8(std::uint64_t);
}
#endif

// AVX-512
#ifdef __AVX512F__
extern "C" {
void avx512_unroll_1(std::uint64_t);
void avx512_unroll_2(std::uint64_t);
void avx512_unroll_3(std::uint64_t);
void avx512_unroll_4(std::uint64_t);
void avx512_unroll_5(std::uint64_t);
void avx512_unroll_6(std::uint64_t);
void avx512_unroll_7(std::uint64_t);
void avx512_unroll_8(std::uint64_t);
}
#endif

#endif  // x86-64


#if defined(__aarch64__)

// NEON
extern "C" {
void neon_unroll_1(std::uint64_t);
void neon_unroll_2(std::uint64_t);
void neon_unroll_3(std::uint64_t);
void neon_unroll_4(std::uint64_t);
void neon_unroll_5(std::uint64_t);
void neon_unroll_6(std::uint64_t);
void neon_unroll_7(std::uint64_t);
void neon_unroll_8(std::uint64_t);
}

// SVE
#ifdef __ARM_FEATURE_SVE
#include <arm_sve.h>

extern "C" {
void sve_unroll_1(std::uint64_t);
void sve_unroll_2(std::uint64_t);
void sve_unroll_3(std::uint64_t);
void sve_unroll_4(std::uint64_t);
void sve_unroll_5(std::uint64_t);
void sve_unroll_6(std::uint64_t);
void sve_unroll_7(std::uint64_t);
void sve_unroll_8(std::uint64_t);
}

static inline int sve_lane_count_32()
{
    // Number of active 32-bit lanes in current SVE VL
    return svcntw();
}
#endif  // __ARM_FEATURE_SVE

#endif  // __aarch64__


///////////////////////////////////////////////////////////////
// Generic SIMD benchmark macro
//
// - Func:          assembly function to call
// - UNROLL_COUNT:  how many independent vector accumulators (1..8)
// - LANES_EXPR:    number of lanes per vector (e.g., 4, 8, 16, or sve_lane_count_32())
//
// The assembly functions are assumed to do:
//
//   inner_unroll = 8 repeats of the UNROLL_COUNT ops per outer loop
//   outer loop count = N (passed as argument)
//
///////////////////////////////////////////////////////////////

#define DEFINE_SIMD_BENCH(Func, UNROLL_COUNT, LANES_EXPR)                            \
    static void bench_##Func(benchmark::State& state) {                              \
        constexpr std::uint64_t N = 1'000'000; /* outer loop count passed to asm */  \
        constexpr std::uint64_t inner_unroll = 8;                                    \
        constexpr std::uint64_t unroll_count = (UNROLL_COUNT);                       \
                                                                                     \
        const double lanes = static_cast<double>(LANES_EXPR);                        \
        const double ops_per_call =                                                 \
            static_cast<double>(N) * inner_unroll * unroll_count; /* vector ops */   \
                                                                                     \
        for (auto _ : state) {                                                       \
            Func(N);                                                                 \
        }                                                                            \
                                                                                     \
        const double iters = static_cast<double>(state.iterations());                \
        const double total_vec_ops  = ops_per_call * iters;                          \
        const double total_lane_ops = total_vec_ops * lanes;                         \
                                                                                     \
        using benchmark::Counter;                                                    \
        /* vec_ops and lane_ops will show up as X/s (rate) */                        \
        state.counters["vec_ops"]  = Counter(total_vec_ops, Counter::kIsRate);       \
        state.counters["lane_ops"] = Counter(total_lane_ops, Counter::kIsRate);      \
        state.counters["unroll"]   = Counter(unroll_count);                          \
        state.counters["lanes"]    = Counter(lanes);                                 \
    }                                                                                \
    BENCHMARK(bench_##Func);


///////////////////////////////////////////////////////////////
// Register benchmarks per-architecture
///////////////////////////////////////////////////////////////

#if defined(__x86_64__) || defined(_M_X64)

// SSE2: 128-bit, 4 lanes of int32
DEFINE_SIMD_BENCH(sse2_unroll_1, 1, 4)
DEFINE_SIMD_BENCH(sse2_unroll_2, 2, 4)
DEFINE_SIMD_BENCH(sse2_unroll_3, 3, 4)
DEFINE_SIMD_BENCH(sse2_unroll_4, 4, 4)
DEFINE_SIMD_BENCH(sse2_unroll_5, 5, 4)
DEFINE_SIMD_BENCH(sse2_unroll_6, 6, 4)
DEFINE_SIMD_BENCH(sse2_unroll_7, 7, 4)
DEFINE_SIMD_BENCH(sse2_unroll_8, 8, 4)

#ifdef __AVX__
// AVX / AVX2: 256-bit, 8 lanes of int32
DEFINE_SIMD_BENCH(avx_unroll_1, 1, 8)
DEFINE_SIMD_BENCH(avx_unroll_2, 2, 8)
DEFINE_SIMD_BENCH(avx_unroll_3, 3, 8)
DEFINE_SIMD_BENCH(avx_unroll_4, 4, 8)
DEFINE_SIMD_BENCH(avx_unroll_5, 5, 8)
DEFINE_SIMD_BENCH(avx_unroll_6, 6, 8)
DEFINE_SIMD_BENCH(avx_unroll_7, 7, 8)
DEFINE_SIMD_BENCH(avx_unroll_8, 8, 8)
#endif

#ifdef __AVX512F__
// AVX-512: 512-bit, 16 lanes of int32
DEFINE_SIMD_BENCH(avx512_unroll_1, 1, 16)
DEFINE_SIMD_BENCH(avx512_unroll_2, 2, 16)
DEFINE_SIMD_BENCH(avx512_unroll_3, 3, 16)
DEFINE_SIMD_BENCH(avx512_unroll_4, 4, 16)
DEFINE_SIMD_BENCH(avx512_unroll_5, 5, 16)
DEFINE_SIMD_BENCH(avx512_unroll_6, 6, 16)
DEFINE_SIMD_BENCH(avx512_unroll_7, 7, 16)
DEFINE_SIMD_BENCH(avx512_unroll_8, 8, 16)
#endif

#endif  // x86-64


#if defined(__aarch64__)

// NEON: 128-bit, 4 lanes of int32 (vN.4s)
DEFINE_SIMD_BENCH(neon_unroll_1, 1, 4)
DEFINE_SIMD_BENCH(neon_unroll_2, 2, 4)
DEFINE_SIMD_BENCH(neon_unroll_3, 3, 4)
DEFINE_SIMD_BENCH(neon_unroll_4, 4, 4)
DEFINE_SIMD_BENCH(neon_unroll_5, 5, 4)
DEFINE_SIMD_BENCH(neon_unroll_6, 6, 4)
DEFINE_SIMD_BENCH(neon_unroll_7, 7, 4)
DEFINE_SIMD_BENCH(neon_unroll_8, 8, 4)

#ifdef __ARM_FEATURE_SVE
// SVE: variable-length, lanes = svcntw()
DEFINE_SIMD_BENCH(sve_unroll_1, 1, sve_lane_count_32())
DEFINE_SIMD_BENCH(sve_unroll_2, 2, sve_lane_count_32())
DEFINE_SIMD_BENCH(sve_unroll_3, 3, sve_lane_count_32())
DEFINE_SIMD_BENCH(sve_unroll_4, 4, sve_lane_count_32())
DEFINE_SIMD_BENCH(sve_unroll_5, 5, sve_lane_count_32())
DEFINE_SIMD_BENCH(sve_unroll_6, 6, sve_lane_count_32())
DEFINE_SIMD_BENCH(sve_unroll_7, 7, sve_lane_count_32())
DEFINE_SIMD_BENCH(sve_unroll_8, 8, sve_lane_count_32())
#endif  // __ARM_FEATURE_SVE

#endif  // __aarch64__


///////////////////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////////////////

BENCHMARK_MAIN();
