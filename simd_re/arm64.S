    .text

    // NEON integer add (32-bit lanes) benchmark
    // Functions: neon_unroll_1 .. neon_unroll_8
    // Arg: x0 = iteration count
    // Clobbers: v0..v7, x0

    .macro NEON_ADD_N n
        .rept 8
            .if \n > 0
                add     v0.4s, v0.4s, v0.4s
            .endif
            .if \n > 1
                add     v1.4s, v1.4s, v1.4s
            .endif
            .if \n > 2
                add     v2.4s, v2.4s, v2.4s
            .endif
            .if \n > 3
                add     v3.4s, v3.4s, v3.4s
            .endif
            .if \n > 4
                add     v4.4s, v4.4s, v4.4s
            .endif
            .if \n > 5
                add     v5.4s, v5.4s, v5.4s
            .endif
            .if \n > 6
                add     v6.4s, v6.4s, v6.4s
            .endif
            .if \n > 7
                add     v7.4s, v7.4s, v7.4s
            .endif
        .endr
    .endm

    .macro DEFINE_NEON_FUNC n
        .globl  neon_unroll_\n
        .p2align 4
        .type   neon_unroll_\n,@function
neon_unroll_\n:
1:
        NEON_ADD_N \n
        subs    x0, x0, #1
        b.ne    1b
        ret
    .endm

    // SVE integer add (32-bit lanes) benchmark
    // Functions: sve_unroll_1 .. sve_unroll_8
    // Arg: x0 = iteration count
    // Clobbers: z0..z7, p0, x0

    .macro SVE_ADD_N n
        .rept 8
            .if \n > 0
                add     z0.s, p0/m, z0.s, z0.s
            .endif
            .if \n > 1
                add     z1.s, p0/m, z1.s, z1.s
            .endif
            .if \n > 2
                add     z2.s, p0/m, z2.s, z2.s
            .endif
            .if \n > 3
                add     z3.s, p0/m, z3.s, z3.s
            .endif
            .if \n > 4
                add     z4.s, p0/m, z4.s, z4.s
            .endif
            .if \n > 5
                add     z5.s, p0/m, z5.s, z5.s
            .endif
            .if \n > 6
                add     z6.s, p0/m, z6.s, z6.s
            .endif
            .if \n > 7
                add     z7.s, p0/m, z7.s, z7.s
            .endif
        .endr
    .endm

    .macro DEFINE_SVE_FUNC n
        .globl  sve_unroll_\n
        .p2align 4
        .type   sve_unroll_\n,@function
sve_unroll_\n:
        // Enable all lanes in p0
        ptrue   p0.s
1:
        SVE_ADD_N \n
        subs    x0, x0, #1
        b.ne    1b
        ret
    .endm

// Instantiate unroll levels 1..8
DEFINE_NEON_FUNC 1
DEFINE_NEON_FUNC 2
DEFINE_NEON_FUNC 3
DEFINE_NEON_FUNC 4
DEFINE_NEON_FUNC 5
DEFINE_NEON_FUNC 6
DEFINE_NEON_FUNC 7
DEFINE_NEON_FUNC 8

// Instantiate unroll levels 1..8
DEFINE_SVE_FUNC 1
DEFINE_SVE_FUNC 2
DEFINE_SVE_FUNC 3
DEFINE_SVE_FUNC 4
DEFINE_SVE_FUNC 5
DEFINE_SVE_FUNC 6
DEFINE_SVE_FUNC 7
DEFINE_SVE_FUNC 8
