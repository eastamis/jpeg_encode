#include <stdlib.h>
#include <stdio.h>
#include "jpginfo.h"

/* writebit.c */
extern void WriteBits (JPEG_INFO *ji, int code, int size);

/* myjpgenc.c */
extern const int zigzag_order[64];

/*
 * Huffman table setup routines
 */

/* Define a Huffman table */
int add_huff_table (JPEG_INFO *ji, int tblno, int is_ac,
                     const unsigned char *bits, 
                     const unsigned char *val)
{
  unsigned char huffsize[256];
  unsigned short huffcode[256];
  unsigned char *bitsptr;
  unsigned char *hvalptr;
  unsigned short *hcwdptr;
  unsigned short *hcsiptr;
  unsigned short code, tmp;
  int nsymbols, len, bitcount;
  
  if (is_ac) {
    bitcount = sizeof(ji->AC_Huff_Table[tblno].bits);
    bitsptr = ji->AC_Huff_Table[tblno].bits;
    hvalptr = ji->AC_Huff_Table[tblno].huff_val;    
    hcwdptr = ji->AC_Huff_Table[tblno].codeword;
    hcsiptr = ji->AC_Huff_Table[tblno].codesize;
  } else {
    bitcount = sizeof(ji->DC_Huff_Table[tblno].bits);
    bitsptr = ji->DC_Huff_Table[tblno].bits;
    hvalptr = ji->DC_Huff_Table[tblno].huff_val;
    hcwdptr = ji->DC_Huff_Table[tblno].codeword;
    hcsiptr = ji->DC_Huff_Table[tblno].codesize;
  }
  
  /* Copy the number-of-symbols-of-each-code-length counts */
  memcpy(bitsptr, bits, bitcount);
  
  /* Validate the counts.  We do this here mainly so we can copy the right
   * number of symbols from the val[] array, without risking marching off
   * the end of memory.
   */
  bitcount = 0;
  nsymbols = 0;
  for (len = 1; len <= 16; len++) {
    nsymbols += bits[len];
    /* generate codesize */
    tmp = bits[len];
    while (tmp--)
      huffsize[bitcount++] = len;
  }
  //if (nsymbols < 1 || nsymbols > 256) {
  //  printf("Bad huffman table\n");
  //  return;
  //}
  
  /* generate the Huffman codeword */
  code = 0;
  tmp = huffsize[0];
  len = 0;
  while (huffsize[len]) {
    while ((huffsize[len]) == tmp) {
      huffcode[len++] = code;
      code++;
    }
    code <<= 1;
    tmp++;
  }
  
  // huffman value
  memcpy(hvalptr, val, nsymbols * sizeof(char));
  
  /* generate encoding tables */
  /* These are code and size indexed by symbol value */

  /* This is also a convenient place to check for out-of-range
   * and duplicated VAL entries.
   */
  bitcount = is_ac ? 252 : 12; // max symbols, dc table is 12, ac table is 252
  
  for (len = 0; len < nsymbols; len++) {
    tmp = hvalptr[len];
    if (tmp < 0 || tmp > bitcount || hcsiptr[tmp]) {
      printf("Bad Huffman table\n");
      return 0;
    }
    hcwdptr[tmp] = huffcode[len];
    hcsiptr[tmp] = huffsize[len];
  }
  
  return 1;
}

void set_huff_table (JPEG_INFO *ji)
/* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
/* IMPORTANT: these are only valid for 8-bit data precision! */
{
  static const unsigned char bits_dc_luminance[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const unsigned char val_dc_luminance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const unsigned char bits_dc_chrominance[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const unsigned char val_dc_chrominance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const unsigned char bits_ac_luminance[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const unsigned char val_ac_luminance[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  static const unsigned char bits_ac_chrominance[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const unsigned char val_ac_chrominance[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  add_huff_table(ji, 0, 0, bits_dc_luminance, val_dc_luminance);
  add_huff_table(ji, 0, 1, bits_ac_luminance, val_ac_luminance);
  add_huff_table(ji, 1, 0, bits_dc_chrominance, val_dc_chrominance);
  add_huff_table(ji, 1, 1, bits_ac_chrominance, val_ac_chrominance);
}


void HuffEnc_DC (JPEG_INFO *ji, int cid, int dc_coff)
{
  int temp, temp2;
  int nbits;
  HUFFMAN_TABLE_DC *htbl = &ji->DC_Huff_Table[(cid==0)?0:1];
  
  /* Encode the DC coefficient difference per section */
  
  temp = temp2 = dc_coff - ji->LastDC[cid];
  
  if (temp < 0) {
    temp = -temp;		/* temp is abs value of input */
    /* For a negative input, want temp2 = bitwise complement of abs(input) */
    /* This code assumes we are on a two's complement machine */
    temp2--;
  }
  
  /* Find the number of bits needed for the magnitude of the coefficient */
  nbits = 0;
  while (temp) {
    nbits++;
    temp >>= 1;
  }
  /* Check for out-of-range coefficient values.
   * Since we're encoding a difference, the range limit is twice as much.
   */
  if (nbits > 11) {
    printf("Bad DCT coefficient![0x%x]\n", dc_coff);
    return;
  }
  
  /* write the Huffman-coded symbol for the number of bits */
  //printf("DC: code=%d, size=%d\n", htbl->codeword[nbits], htbl->codesize[nbits]);
  WriteBits(ji, htbl->codeword[nbits], htbl->codesize[nbits]);
  
  /* write that number of bits of the value, if positive, */
  /* or the complement of its magnitude, if negative. */
  //printf("DC: code=%d, size=%d\n", temp2, nbits);
  WriteBits(ji, temp2, nbits);  
  
  ji->LastDC[cid] = dc_coff;
}

void HuffEnc_AC (JPEG_INFO *ji, int cid, int *ac_coff)
{
  int temp, temp2;
  int nbits;
  int n, r, i;
  HUFFMAN_TABLE_AC *htbl = &ji->AC_Huff_Table[(cid==0)?0:1];
  
  /* Encode the AC coefficients per section F.1.2.2 */
  
  r = 0;			/* r = run length of zeros */
  
  for (n = 1; n <= 63; n++) {
    if ((temp = ac_coff[zigzag_order[n]-1]) == 0) {
      r++;
    } else {
      /* if run length > 15, must write special run-length-16 codes (0xF0) */
      while (r > 15) {
        WriteBits(ji, htbl->codeword[0xF0], htbl->codesize[0xF0]);
        r -= 16;
      }

      temp2 = temp;
      if (temp < 0) {
         temp = -temp;		/* temp is abs value of input */
         /* This code assumes we are on a two's complement machine */
         temp2--;
      }
      
      /* Find the number of bits needed for the magnitude of the coefficient */
      nbits = 1;		/* there must be at least one 1 bit */
      while ((temp >>= 1))
        nbits++;
      /* Check for out-of-range coefficient values */
      if (nbits > 10) {
        printf("Bad DCT coefficient![0x%x]\n", ac_coff[zigzag_order[n]-1]);
        return;
      }
      
      /* write Huffman symbol for run length / number of bits */
      i = (r << 4) + nbits;
      WriteBits(ji, htbl->codeword[i], htbl->codesize[i]);
      
      /* write that number of bits of the value, if positive, */
      /* or the complement of its magnitude, if negative. */
      WriteBits(ji, temp2, nbits);
      
      r = 0;
    }
  }

  /* If the last coef(s) were zero, write an end-of-block code */
  if (r > 0)
    WriteBits(ji, htbl->codeword[0], htbl->codesize[0]);
}
