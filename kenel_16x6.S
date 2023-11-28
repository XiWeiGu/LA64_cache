#include "loongarch64_asm.S"

/**
  void kernel_16x6_*(double* a, double* b, int Loops)
  */
#define A      $r4   // param 1: a
#define B      $r5   // param 2: b
#define L      $r6   // param 3: Loops

#define ZERO    $r0

/* LASX vectors */
#define U0     $xr0
#define U1     $xr1
#define U2     $xr2
#define U3     $xr3
#define U4     $xr4
#define U5     $xr5
#define U6     $xr6
#define D0     $xr7
#define D1     $xr8
#define D2     $xr9
#define D3     $xr10
#define D4     $xr11
#define D5     $xr12
#define D6     $xr13
#define D7     $xr14
#define D8     $xr15
#define D9     $xr16
#define D10    $xr17
#define D11    $xr18
#define D12    $xr19
#define D13    $xr20
#define D14    $xr21
#define D15    $xr22
#define D16    $xr23
#define D17    $xr24
#define D18    $xr25
#define D19    $xr26
#define D20    $xr27
#define D21    $xr28
#define D22    $xr29
#define D23    $xr30
#define VALPHA $xr31

/* Prefetch interval */
#define A_PRE  0x200
#define B_PRE  0x100
.macro Kernel_16x6
    /* Load 16 * 64 from A0 */
    xvld     U0,   A,    0x00
    xvld     U1,   A,    0x20
    xvld     U2,   A,    0x40
    xvld     U3,   A,    0x60

    /* Cumulative D0~D23 */
    xvldrepl.d U4,  B, 0x00
    xvldrepl.d U5,  B, 0x08
    xvldrepl.d U6,  B, 0x10

    xvfmadd.d  D0,  U0, U4, D0
    xvfmadd.d  D1,  U1, U4, D1
    xvfmadd.d  D2,  U2, U4, D2
    xvfmadd.d  D3,  U3, U4, D3
    preld      0,   B,  B_PRE

    xvfmadd.d  D4,  U0, U5, D4
    xvfmadd.d  D5,  U1, U5, D5
    xvfmadd.d  D6,  U2, U5, D6
    xvfmadd.d  D7,  U3, U5, D7

    xvfmadd.d  D8,  U0, U6, D8
    xvfmadd.d  D9,  U1, U6, D9
    xvfmadd.d  D10, U2, U6, D10
    xvfmadd.d  D11, U3, U6, D11
    preld      0,   A,  A_PRE

    xvldrepl.d U4,  B, 0x18
    xvldrepl.d U5,  B, 0x20
    xvldrepl.d U6,  B, 0x28

    xvfmadd.d  D12, U0, U4, D12
    xvfmadd.d  D13, U1, U4, D13
    xvfmadd.d  D14, U2, U4, D14
    xvfmadd.d  D15, U3, U4, D15

    xvfmadd.d  D16, U0, U5, D16
    xvfmadd.d  D17, U1, U5, D17
    xvfmadd.d  D18, U2, U5, D18
    xvfmadd.d  D19, U3, U5, D19
    preld      0,   A,  A_PRE + 0x40

    xvfmadd.d  D20, U0, U6, D20
    xvfmadd.d  D21, U1, U6, D21
    xvfmadd.d  D22, U2, U6, D22
    xvfmadd.d  D23, U3, U6, D23
.endm

.macro Kernel_16x6_L1
    Kernel_16x6

    addi.d     A,  A, 0x00
    addi.d     B,  B, 0x00
.endm

.macro Kernel_16x6_NULL
    Kernel_16x6

    addi.d     A,  A, 0x80
    addi.d     B,  B, 0x30
.endm

function kernel_16x6_L1_lasx
    push_if_used 17, 32
    blt     L,  ZERO,   .L1_end
.L1:
    Kernel_16x6_L1
    addi.d  L,  L,  -1
    blt     ZERO,   L,  .L1
.L1_end:
    pop_if_used 17, 32
endfunc

function kernel_16x6_NULL_lasx
    push_if_used 17, 32
    blt     L,  ZERO,   .LNULL_end
.LNULL:
    Kernel_16x6_NULL
    addi.d  L,  L,  -1
    blt     ZERO,   L,  .LNULL
.LNULL_end:
    pop_if_used 17, 32
endfunc

#define A0  $r7
#define B0  $r8
#define TL  $r9

function kernel_16x6_L2_lasx
    push_if_used 17, 32
    blt     L,  ZERO,   .L2_end
    // BackUp A, B
    move    A0, A
    move    B0, B
.L2:
    move   A,   A0
    move   B,   B0
    addi.d  TL,  ZERO,  640
.L2_sub: // Loops: 640
    Kernel_16x6_NULL
    addi.d  TL, TL, -1
    blt     ZERO,   TL, .L2_sub
    addi.d  L,  L,  -1
    blt     ZERO,   L,  .L2
.L2_end:
    pop_if_used 17, 32
endfunc

function kernel_16x6_L3_lasx
    push_if_used 17, 32
    blt     L,  ZERO,   .L3_end
    // BackUp A, B
    move    A0, A
    move    B0, B
.L3:
    move    A,   A0
    move    B,   B0
    addi.d  TL,  ZERO,  1
    slli.d  TL,  TL,    12
.L3_sub: // Loops: 4096, A=512KB, B=192KB
    Kernel_16x6_NULL
    addi.d  TL, TL, -1
    blt     ZERO,   TL, .L3_sub
    addi.d  L,  L,  -1
    blt     ZERO,   L,  .L3
.L3_end:
    pop_if_used 17, 32
endfunc
