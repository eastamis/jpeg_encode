#include <stdlib.h>
#include <stdio.h>

/* Bit-Reversal Transform */
void BRT (int *src, int *dst)
{
    dst[0] = src[0];
    dst[1] = src[4];
    dst[2] = src[2];
    dst[3] = src[6];
    dst[4] = src[1];
    dst[5] = src[5];
    dst[6] = src[3];
    dst[7] = src[7];
}

/*  Walsh-Hadamard Transform */
void WHT (int *src, int *dst)
{
    int g8_1, g8_2, g8_3, g8_4, g8_5, g8_6, g8_7, g8_8;
    int g4_1, g4_2, g4_3, g4_4;
    int g4ud_1, g4ud_2, g4ud_3, g4ud_4;
    
    /* Walsh-Hadamard transform, arranged in Walsh order */
    /*      /  1  1  1  1  1  1  1  1 \
            |  1  1  1  1 -1 -1 -1 -1 |
            |  1  1 -1 -1 -1 -1  1  1 |
       Hw = |  1  1 -1 -1  1  1 -1 -1 |
            |  1 -1 -1  1  1 -1 -1  1 |
            |  1 -1 -1  1 -1  1  1 -1 |
            |  1 -1  1 -1 -1  1 -1  1 |
            \  1 -1  1 -1  1 -1  1 -1 /  
       h[0] = x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7];
       h[1] = x[0] + x[1] + x[2] + x[3] - x[4] - x[5] - x[6] - x[7];
       h[2] = x[0] + x[1] - x[2] - x[3] - x[4] - x[5] + x[6] + x[7];
       h[3] = x[0] + x[1] - x[2] - x[3] + x[4] + x[5] - x[6] - x[7];
       h[4] = x[0] - x[1] - x[2] + x[3] + x[4] - x[5] - x[6] + x[7];
       h[5] = x[0] - x[1] - x[2] + x[3] - x[4] + x[5] + x[6] - x[7];
       h[6] = x[0] - x[1] + x[2] - x[3] - x[4] + x[5] - x[6] + x[7];
       h[7] = x[0] - x[1] + x[2] - x[3] + x[4] - x[5] + x[6] - x[7];
    */

    /* Manz's algorithm, Fast structure of the WHT */
    /* G8 */
    g8_1 = src[0] + src[1];
    g8_2 = src[4] + src[5];
    g8_3 = src[2] + src[3];
    g8_4 = src[6] + src[7];
    g8_5 = src[0] - src[1];
    g8_6 = src[4] - src[5];
    g8_7 = src[2] - src[3];
    g8_8 = src[6] - src[7];
    /* G4 */
    g4_1 = g8_1 + g8_3;
    g4_2 = g8_2 + g8_4;
    g4_3 = g8_1 - g8_3;
    g4_4 = g8_2 - g8_4;
    /* G4ud */
    g4ud_1 = g8_5 - g8_7;
    g4ud_2 = g8_6 - g8_8;
    g4ud_3 = g8_5 + g8_7;
    g4ud_4 = g8_6 + g8_8;
    /* G2 and G2ud */
    dst[0] = g4_1 + g4_2;
    dst[1] = g4_1 - g4_2;
    dst[2] = g4_3 - g4_4;
    dst[3] = g4_3 + g4_4;
    dst[4] = g4ud_1 + g4ud_2;
    dst[5] = g4ud_1 - g4ud_2;
    dst[6] = g4ud_3 - g4ud_4;
    dst[7] = g4ud_3 + g4ud_4;
}

/* -pi / 8 */
void Rotate1 (int s1, int s2, int *d1, int *d2)
{
    int r;
    
    r = s1 + (s2>>2);
    *d2 = s2 - (r>>1);
    *d1 = r + (*d2>>2);
}

/* 3pi / 8 */
void Rotate2 (int s1, int s2, int *d1, int *d2)
{
    int r;
    
    r = s1 - (((s2<<2)+s2)>>3);
    *d2 = s2 + r;
    *d1 = r - (((*d2<<2)+*d2)>>3);
}

/* 7pi / 16 */
void Rotate3 (int s1, int s2, int *d1, int *d2)
{
    int r;
    
    r = s1 - (((s2<<1)+s2)>>2);
    *d2 = s2 + r;
    *d1 = r - (((*d2<<1)+*d2)>>2);
}

/* 3pi / 16 */
void Rotate4 (int s1, int s2, int *d1, int *d2)
{
    int r;
    
    r = s2 + (s1>>2);
    *d1 = s1 - (r>>1);
    *d2 = r + (*d1>>2);
}

/* Integer Lifting Transform */
void ILT (int *src, int *dst)
{
    int t5, t6, t7, t8;
    
    dst[0] = src[0];
    dst[1] = src[1];
    
    // -pi/8
    Rotate1(src[2], src[3], &dst[2], &dst[3]);
    
    // 3pi/8
    Rotate2(src[4], src[6], &t5, &t7);
    Rotate2(src[5], src[7], &t6, &t8);
    
    t5 = 0 - t5;
    t6 = 0 - t6;
    t7 = 0 - t7;
    
    // 7pi/16
    Rotate3(t6, t7, &dst[4], &dst[7]);
    
    // 3pi/16
    Rotate4(t8, t5, &dst[5], &dst[6]);
}

/* intDCT */
void intDCT (int *dctbuf)
{
    int b[8];
    int t[8];
    
    WHT(dctbuf, b);
    BRT(b, t);
    ILT(t, b);
    BRT(b, dctbuf);
}

/* intDCT_2D */
void intDCT_2D (int *src, int *dst)
{
    int *dataptr;
    int buf[8];
    int i, n;
    
    /* Pass 1: process rows. */
    dataptr = src;
    for (i = 0; i < 8; i++) {
        intDCT(dataptr);
        dataptr += 8;		/* advance pointer to next row */
    }
    
    /* Pass 2: process columns. */
    dataptr = src;
    for (i = 0; i < 8; i++) {
        // get column data
        for (n = 0; n < 8; n++)
            buf[n] = dataptr[8*n];
      
        intDCT(buf);
    
        for (n = 0; n < 8; n++)
            dst[8*n] = buf[n] / 8; /* must divide 8 */
    
        dataptr++;			/* advance pointer to next column */
        dst++;
    }
}
