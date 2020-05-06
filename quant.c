#include <stdlib.h>
#include <stdio.h>
#include "jpginfo.h"

/*
 * Quantization table setup routines
 */

/* Define a quantization table equal to the basic_table times
 * a scale factor (given as a percentage).
 */
void add_quant_table (JPEG_INFO *ji, int which_tbl,
		                  const unsigned int *basic_table)
{
  QUANT_TABLE *qtblptr;
  int n;
  int temp;

  // just support two quantization table
  if (which_tbl < 0 || which_tbl > 1) {
    printf("quantization table index invalidate![%d]\n", which_tbl);
    return;
  }
  
  qtblptr = &ji->Quan_Table[which_tbl];
  
  for (n = 0; n < 64; n++) {
    temp = (basic_table[n] * ji->Quality + 50) / 100;
    /* limit the values to the valid range */
    if (temp <= 0)  temp = 1;
    if (temp > 255) temp = 255;		/* limit to baseline range */
    qtblptr->quant_val[n] = (unsigned char)temp;
  }
}


void set_jpegenc_quality (JPEG_INFO *ji)
/* Set or change the 'quality' (quantization) setting, using default tables
 * and a straight percentage-scaling quality scale.  In most cases it's better
 * to use jpeg_set_quality (below); this entry point is provided for
 * applications that insist on a linear percentage scaling.
 */
{
  /* These are the sample quantization tables given in JPEG spec section K.1.
   * The spec says that the values given produce "good" quality, and
   * when divided by 2, "very good" quality.
   */
  static const unsigned int std_luminance_quant_tbl[64] = {
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68, 109, 103,  77,
    24,  35,  55,  64,  81, 104, 113,  92,
    49,  64,  78,  87, 103, 121, 120, 101,
    72,  92,  95,  98, 112, 100, 103,  99
  };
  static const unsigned int std_chrominance_quant_tbl[64] = {
    17,  18,  24,  47,  99,  99,  99,  99,
    18,  21,  26,  66,  99,  99,  99,  99,
    24,  26,  56,  99,  99,  99,  99,  99,
    47,  66,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
  };
  
  /* Convert a user-specified quality rating to a percentage scaling factor
   * for an underlying quantization table, using our recommended scaling curve.
   * The input 'quality' factor should be 0 (terrible) to 100 (very good).
   */
  /* Safety limit on quality factor.  Convert 0 to 1 to avoid zero divide. */
  if (ji->Quality <= 0) ji->Quality = 1;
  if (ji->Quality > 100) ji->Quality = 100;

  /* The basic table is used as-is (scaling 100) for a quality of 50.
   * Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
   * note that at Q=100 the scaling is 0, which will cause jpeg_add_quant_table
   * to make all the table entries 1 (hence, minimum quantization loss).
   * Qualities 1..50 are converted to scaling percentage 5000/Q.
   */
  if (ji->Quality < 50)
    ji->Quality = 5000 / ji->Quality;
  else
    ji->Quality = 200 - ji->Quality*2;

  /* Set up two quantization tables using the specified scaling */
  add_quant_table(ji, 0, std_luminance_quant_tbl);
  add_quant_table(ji, 1, std_chrominance_quant_tbl);
}

void Quant_dct (JPEG_INFO *ji, int qtbl_no, int *dct_coff)
{
  int n;
  
  for (n = 0; n < 64; n++) {
    dct_coff[n] /= ji->Quan_Table[qtbl_no].quant_val[n];
  }
}