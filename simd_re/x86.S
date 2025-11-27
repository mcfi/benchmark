    .text

    ################################################################
    # Macros: SSE2 paddd on xmm0..xmm7, AVX vpaddd on ymm0..ymm7,
    #         AVX-512 vpaddd on zmm0..zmm7
    # Each *_PADD_N(n) emits 8 iterations of n registers.
    ################################################################

    .macro SSE_PADD_N n
        .rept 8
            .if \n > 0
                paddd   %xmm0, %xmm0
            .endif
            .if \n > 1
                paddd   %xmm1, %xmm1
            .endif
            .if \n > 2
                paddd   %xmm2, %xmm2
            .endif
            .if \n > 3
                paddd   %xmm3, %xmm3
            .endif
            .if \n > 4
                paddd   %xmm4, %xmm4
            .endif
            .if \n > 5
                paddd   %xmm5, %xmm5
            .endif
            .if \n > 6
                paddd   %xmm6, %xmm6
            .endif
            .if \n > 7
                paddd   %xmm7, %xmm7
            .endif
        .endr
    .endm

    .macro YMM_PADD_N n
        .rept 8
            .if \n > 0
                vpaddd  %ymm0, %ymm0, %ymm0
            .endif
            .if \n > 1
                vpaddd  %ymm1, %ymm1, %ymm1
            .endif
            .if \n > 2
                vpaddd  %ymm2, %ymm2, %ymm2
            .endif
            .if \n > 3
                vpaddd  %ymm3, %ymm3, %ymm3
            .endif
            .if \n > 4
                vpaddd  %ymm4, %ymm4, %ymm4
            .endif
            .if \n > 5
                vpaddd  %ymm5, %ymm5, %ymm5
            .endif
            .if \n > 6
                vpaddd  %ymm6, %ymm6, %ymm6
            .endif
            .if \n > 7
                vpaddd  %ymm7, %ymm7, %ymm7
            .endif
        .endr
    .endm

    .macro ZMM_PADD_N n
        .rept 8
            .if \n > 0
                vpaddd  %zmm0, %zmm0, %zmm0
            .endif
            .if \n > 1
                vpaddd  %zmm1, %zmm1, %zmm1
            .endif
            .if \n > 2
                vpaddd  %zmm2, %zmm2, %zmm2
            .endif
            .if \n > 3
                vpaddd  %zmm3, %zmm3, %zmm3
            .endif
            .if \n > 4
                vpaddd  %zmm4, %zmm4, %zmm4
            .endif
            .if \n > 5
                vpaddd  %zmm5, %zmm5, %zmm5
            .endif
            .if \n > 6
                vpaddd  %zmm6, %zmm6, %zmm6
            .endif
            .if \n > 7
                vpaddd  %zmm7, %zmm7, %zmm7
            .endif
        .endr
    .endm

    ################################################################
    # Function generators
    ################################################################

    .macro DEFINE_SSE_FUNC n
        .globl  sse2_unroll_\n
        .p2align 4
        .type   sse2_unroll_\n,@function
sse2_unroll_\n:
1:
        SSE_PADD_N \n
        decq    %rdi
        jnz     1b
        ret
    .endm

    .macro DEFINE_YMM_FUNC n
        .globl  avx_unroll_\n
        .p2align 4
        .type   avx_unroll_\n,@function
avx_unroll_\n:
1:
        YMM_PADD_N \n
        decq    %rdi
        jnz     1b
        ret
    .endm

    .macro DEFINE_ZMM_FUNC n
        .globl  avx512_unroll_\n
        .p2align 4
        .type   avx512_unroll_\n,@function
avx512_unroll_\n:
1:
        ZMM_PADD_N \n
        decq    %rdi
        jnz     1b
        ret
    .endm

################################################################
# Instantiate unroll levels 1..8
################################################################

DEFINE_SSE_FUNC 1
DEFINE_SSE_FUNC 2
DEFINE_SSE_FUNC 3
DEFINE_SSE_FUNC 4
DEFINE_SSE_FUNC 5
DEFINE_SSE_FUNC 6
DEFINE_SSE_FUNC 7
DEFINE_SSE_FUNC 8

DEFINE_YMM_FUNC 1
DEFINE_YMM_FUNC 2
DEFINE_YMM_FUNC 3
DEFINE_YMM_FUNC 4
DEFINE_YMM_FUNC 5
DEFINE_YMM_FUNC 6
DEFINE_YMM_FUNC 7
DEFINE_YMM_FUNC 8

DEFINE_ZMM_FUNC 1
DEFINE_ZMM_FUNC 2
DEFINE_ZMM_FUNC 3
DEFINE_ZMM_FUNC 4
DEFINE_ZMM_FUNC 5
DEFINE_ZMM_FUNC 6
DEFINE_ZMM_FUNC 7
DEFINE_ZMM_FUNC 8
