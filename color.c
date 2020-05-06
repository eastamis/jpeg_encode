#include <stdlib.h>
#include <stdio.h>

extern long round_up (long a, long b);

#define MAXJSAMPLE    255
#define CENTERJSAMPLE 128

#define SCALEBITS   16          /* speediest right-shift on some machines */
#define CBCR_OFFSET ((long) CENTERJSAMPLE << SCALEBITS)
#define ONE_HALF    ((long) 1 << (SCALEBITS-1))
#define FIX(x)      ((long) ((x) * (1L<<SCALEBITS) + 0.5))

/* We allocate one big table and divide it up into eight parts, instead of
 * doing eight alloc_small requests.  This lets us use a single table base
 * address, which can be held in a register in the inner loops on many
 * machines (more than can hold all eight addresses, anyway).
 */

#define R_Y_OFF     0			/* offset to R => Y section */
#define G_Y_OFF     (1*(MAXJSAMPLE+1))	/* offset to G => Y section */
#define B_Y_OFF     (2*(MAXJSAMPLE+1))	/* etc. */
#define R_CB_OFF    (3*(MAXJSAMPLE+1))
#define G_CB_OFF    (4*(MAXJSAMPLE+1))
#define B_CB_OFF    (5*(MAXJSAMPLE+1))
#define R_CR_OFF    B_CB_OFF		/* B=>Cb, R=>Cr are the same */
#define G_CR_OFF    (6*(MAXJSAMPLE+1))
#define B_CR_OFF    (7*(MAXJSAMPLE+1))
#define TABLE_SIZE  (8*(MAXJSAMPLE+1))


//
static long rgb_ycc_tab[TABLE_SIZE];

/*
 * Initialize for RGB->YCC colorspace conversion.
 */

void
create_rgb2ycc_table (void)
{
  long i;

  for (i = 0; i <= MAXJSAMPLE; i++) {
    rgb_ycc_tab[i+R_Y_OFF] = FIX(0.29900) * i;
    rgb_ycc_tab[i+G_Y_OFF] = FIX(0.58700) * i;
    rgb_ycc_tab[i+B_Y_OFF] = FIX(0.11400) * i     + ONE_HALF;
    rgb_ycc_tab[i+R_CB_OFF] = (-FIX(0.16874)) * i;
    rgb_ycc_tab[i+G_CB_OFF] = (-FIX(0.33126)) * i;
    /* We use a rounding fudge-factor of 0.5-epsilon for Cb and Cr.
     * This ensures that the maximum output will round to MAXJSAMPLE
     * not MAXJSAMPLE+1, and thus that we don't have to range-limit.
     */
    rgb_ycc_tab[i+B_CB_OFF] = FIX(0.50000) * i    + CBCR_OFFSET + ONE_HALF-1;
/*  B=>Cb and R=>Cr tables are the same
    rgb_ycc_tab[i+R_CR_OFF] = FIX(0.50000) * i    + CBCR_OFFSET + ONE_HALF-1;
*/
    rgb_ycc_tab[i+G_CR_OFF] = (-FIX(0.41869)) * i;
    rgb_ycc_tab[i+B_CR_OFF] = (-FIX(0.08131)) * i;
  }
}

void rgbtoyuv420p (unsigned char *pRGB, unsigned char *pYUV, int width, int height)
{
  unsigned char *pY, *pCb, *pCr;
  unsigned char *pInvRGB;
  int stride = round_up(width*3, 4);
  unsigned r, g, b;
  int x, y;
  
  pInvRGB = pRGB + ((height-2)*stride);
  pY = pYUV;
  pCb = pY + (width*height);
  pCr = pCb + ((width*height)>>2);
  
  // debug, gray color
  //memset(pCb, 128, ((width*height)>>2));
  //memset(pCr, 128, ((width*height)>>2));
  
  for (y=0; y<height; y+=2) {
    pRGB = pInvRGB;
    for (x = 0; x < width; x+=2) {
      // Y(0,0)
      r = pInvRGB[stride+2];
      g = pInvRGB[stride+1];
      b = pInvRGB[stride+0];
      /* Y */
      pY[x] = (unsigned char)
                  ((rgb_ycc_tab[r+R_Y_OFF]+rgb_ycc_tab[g+G_Y_OFF]+
                    rgb_ycc_tab[b+B_Y_OFF]) >> SCALEBITS);
		  
      // Y(0,1)
		  r = pInvRGB[stride+5];
      g = pInvRGB[stride+4];
      b = pInvRGB[stride+3];
      /* Y */
      pY[x+1] = (unsigned char)
                    ((rgb_ycc_tab[r+R_Y_OFF]+rgb_ycc_tab[g+G_Y_OFF]+
                      rgb_ycc_tab[b+B_Y_OFF]) >> SCALEBITS);
      
      // Y(1,0)
      r = pInvRGB[2];
      g = pInvRGB[1];
      b = pInvRGB[0];
      /* Y */
      pY[width+x] = (unsigned char)
                         ((rgb_ycc_tab[r+R_Y_OFF]+rgb_ycc_tab[g+G_Y_OFF]+
                           rgb_ycc_tab[b+B_Y_OFF]) >> SCALEBITS);
		     
		  // Y(1,1)
      r = pInvRGB[5];
      g = pInvRGB[4];
      b = pInvRGB[3];
      /* Y */
      pY[width+x+1] = (unsigned char)
                           ((rgb_ycc_tab[r+R_Y_OFF]+rgb_ycc_tab[g+G_Y_OFF]+
                             rgb_ycc_tab[b+B_Y_OFF]) >> SCALEBITS);
      /* Cb */
      pCb[(x>>1)] = (unsigned char)
                        ((rgb_ycc_tab[r+R_CB_OFF]+rgb_ycc_tab[g+G_CB_OFF]+
                        rgb_ycc_tab[b+B_CB_OFF]) >> SCALEBITS);
      /* Cr */
      pCr[(x>>1)] = (unsigned char)
                        ((rgb_ycc_tab[r+R_CR_OFF]+rgb_ycc_tab[g+G_CR_OFF]+
                        rgb_ycc_tab[b+B_CR_OFF]) >> SCALEBITS);
		  
		  // next 2 pixel
		  pInvRGB += 6;		  
    }
    
    // next 2 line
    pInvRGB = pRGB - (stride<<1);
    pY += (width<<1);
    pCb += (width>>1);
    pCr += (width>>1);
  }    
}
