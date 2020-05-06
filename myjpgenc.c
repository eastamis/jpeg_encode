#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "jpginfo.h"

#ifdef JPG_DEBUG
/* debug.c */
extern void write_bmp (JPEG_INFO *ji);
extern void intIDCT_2D (JPEG_INFO *ji, int *dct, int *img);
extern void InvDCT_2D (JPEG_INFO *ji);
extern void prepare_range_limit_table (JPEG_INFO *ji);
#endif //JPG_DEBUG

/* huffenc.c */
extern void set_huff_table (JPEG_INFO *ji);
extern void HuffEnc_DC (JPEG_INFO *ji, int cid, int dc_coff);
extern void HuffEnc_AC (JPEG_INFO *ji, int cid, int *ac_coff);

/* quant.c */
extern void set_jpegenc_quality (JPEG_INFO *ji);
extern void Quant_dct (JPEG_INFO *ji, int qtbl_no, int *dct_coff);

/* intdct.c */
extern void intDCT_2D (int *src, int *dst);

/* color.c */
extern void create_rgb2ycc_table(void);
extern void rgbtoyuv420p(unsigned char *pRGB, 
                unsigned char *pYUV, int width, int height);

// zig-zag
const int zigzag_order[64] =
    {  0,  1,  8, 16,  9,  2,  3, 10,
      17, 24, 32, 25, 18, 11,  4,  5,
      12, 19, 26, 33, 40, 48, 41, 34,
      27, 20, 13,  6,  7, 14, 21, 28,
      35, 42, 49, 56, 57, 50, 43, 36,
      29, 22, 15, 23, 30, 37, 44, 51,
      58, 59, 52, 45, 38, 31, 39, 46,
      53, 60, 61, 54, 47, 55, 62, 63};


//
// CODE SEGMENT
//

// Compute a/b rounded up to next integer
long div_round_up (long a, long b)
{
  return (a + b - 1L) / b;
}

// Compute a rounded up to next multiple of b
long round_up (long a, long b)
{
  a += b - 1L;
  return a - (a % b);
}


int read_bmp (JPEG_INFO *ji, char *bmpfile)
{
  FILE *f;
  BITMAPFILEHEADER bf;
  BITMAPINFOHEADER bi;
  unsigned char * rgb_data;
  int result = 0;
  
  if ((f = fopen(bmpfile, "rb")) == NULL) {
    printf("The file %s was not opened!\n", bmpfile);
    return 0;
  }
  
  memset(&bf, 0, sizeof(BITMAPFILEHEADER));
  fread(&bf, sizeof(BITMAPFILEHEADER), 1, f);
  
  if (bf.bfType != 0x4d42) {
    printf("The BMP file is not valid!\n");
    goto read_bmp_done;
  }
  
  memset(&bi, 0, sizeof(BITMAPINFOHEADER));
  fread(&bi, sizeof(BITMAPINFOHEADER), 1, f);
  
  if (bi.biPlanes != 1 && bi.biBitCount != 24) {
    printf("The color format of this BMP is not support!\n");
    goto read_bmp_done;
  }
  
  ji->Width = bi.biWidth;
  ji->Height = bi.biHeight;
  ji->img_size = bi.biSizeImage;
  
  // allocate ycbcr buffer
  ji->Y_Data = malloc((ji->Width*ji->Height)+((ji->Width*ji->Height)>>1));
  if (ji->Y_Data == NULL) {
    printf("Memory Insufficient for YCbCr data!\n");
    goto read_bmp_done;
  }
  ji->Cb_Data = ji->Y_Data + (ji->Width*ji->Height);
  ji->Cr_Data = ji->Cb_Data + ((ji->Width*ji->Height)>>2);
  
#ifdef JPG_DEBUG
  ji->dctY_Data = malloc(((ji->Width*ji->Height)+((ji->Width*ji->Height)>>1))*sizeof(int));
  if (ji->dctY_Data == NULL) {
    printf("Memory Insufficient for YCbCr data!\n");
    goto read_bmp_done;
  }
  ji->dctCb_Data = ji->dctY_Data + (ji->Width*ji->Height);
  ji->dctCr_Data = ji->dctCb_Data + ((ji->Width*ji->Height)>>2);
#endif //JPG_DEBUG
  
  // move to rgb data area, it's not for RGB24 format
  //if (fseek(f, bf.bfOffBits, SEEK_SET)) {
  //  printf("file operate failed!\n");
  //  goto read_bmp_done;
  //}  
  
  rgb_data = malloc(bi.biSizeImage);
  if (rgb_data == NULL) {
    printf("Memory Insufficient for RGB data!\n");
    goto read_bmp_done;
  }
  
  fread(rgb_data, bi.biSizeImage, 1, f);
  // convert rgb to ycbcr420p
  rgbtoyuv420p(rgb_data, ji->Y_Data, ji->Width, ji->Height);
  
  free(rgb_data);  
  result = 1;
  
read_bmp_done:
  
  fclose(f);
  return result;
}


void Encode_MCU (JPEG_INFO *ji)
{
  int dct_coff[64], img_block[64];
  unsigned char *img_ptr;
  int h_offset, v_offset, nextline;
  int h, v, n, m, cid;
  int mcu_no;
  
  for (mcu_no = 0; mcu_no < ji->Total_MCU; mcu_no++) {
    //printf("--------------- Encode_MCU(), no=%d ---------------\n", mcu_no);    
    cid = 0;
    
    while (cid < 3) { // 3 components, Y, Cb, Cr
    
      v_offset = (mcu_no / ji->Row_MCUs) * ji->MCU_Height;
      if (cid != 0)
        v_offset >>= 1;
      
      for (v = 0; v < ji->CompInfo[cid].vert_sampling; v++) {
        
        h_offset = (mcu_no % ji->Row_MCUs) * ji->MCU_Width;
        if (cid != 0)
          h_offset >>= 1;
        
        for (h = 0; h < ji->CompInfo[cid].horz_sampling; h++) {
          switch (cid) {
          case 0: // Y data
            nextline = ji->Width;
            img_ptr = ji->Y_Data + (v_offset*nextline+h_offset);
            break;
          case 1: // Cb data
            nextline = ji->Width >> 1;
            img_ptr = ji->Cb_Data + (v_offset*nextline+h_offset);
            break;
          case 2: // Cr data
            nextline = ji->Width >> 1;
            img_ptr = ji->Cr_Data + (v_offset*nextline+h_offset);
            break;
          }
          
          for (n = 0; n < 8; n++) {
            for (m = 0; m < 8; m++) {
              img_block[n*8+m] = (int)img_ptr[n*nextline+m] - 
                                          128/*CENTERJSAMPLE*/;
            }
          }
          
          intDCT_2D(img_block, dct_coff);
          Quant_dct(ji, ((cid==0)?0:1), dct_coff);
          HuffEnc_DC(ji, cid, dct_coff[0]);
          HuffEnc_AC(ji, cid, &dct_coff[1]);

#ifdef JPG_DEBUG
          {
            //int i;
            int *dct_img_ptr;
            //printf("\nImage block-----\n");
            //for (i = 0; i < 64; i++) {
            //  if ((i % 8) == 0) printf("    ");
            //  printf(" %d", img_block[i]);
            //  if ((i % 8) == 7) printf("\n");
            //}
            //printf("\nDCT coefficients-----\n");
            //for (i = 0; i < 64; i++) {
            //  if ((i % 8) == 0) printf("    ");
            //  printf(" %d", dct_coff[i]);
            //  if ((i % 8) == 7) printf("\n");
            //}
            //intIDCT_2D(ji, dct_coff, img_block);
            //printf("\nIDCT, image block-----\n");
            //for (i = 0; i < 64; i++) {
            //  if ((i % 8) == 0) printf("    ");
            //  printf(" %d", img_block[i]);
            //  if ((i % 8) == 7) printf("\n");
            //}
            //printf("\n"); getchar();          
          
            switch (cid) {
            case 0: // Y data
              dct_img_ptr = ji->dctY_Data + (v_offset*nextline+h_offset);
              break;
            case 1: // Cb data
              dct_img_ptr = ji->dctCb_Data + (v_offset*nextline+h_offset);
              break;
            case 2: // Cr data
              dct_img_ptr = ji->dctCr_Data + (v_offset*nextline+h_offset);
              break;
            }
            for (n = 0; n < 8; n++) {
              for (m = 0; m < 8; m++) {
                dct_img_ptr[n*nextline+m] = dct_coff[n*8+m];
              }
            }
          }
#endif //JPG_DEBUG

          h_offset += 8;
        }
        
        v_offset += 8;
      }
      
      cid++;
    }  // end of while (cid < 3)
  }
}

void Create_StartOfImage (JPEG_INFO *ji)
{
  static const unsigned char app0_buf[] = 
      { 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 
        0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00 };
  
  // SOI marker
  ji->jpg_dest[0] = 0xFF;
  ji->jpg_dest[1] = 0xD8;
  ji->jpg_size = 2;
  
  // APP0 marker
  ji->jpg_dest[ji->jpg_size++] = 0xFF;
  ji->jpg_dest[ji->jpg_size++] = 0xE0;
  ji->jpg_dest[ji->jpg_size++] = 0x00;   // 0x0010 = 16
  ji->jpg_dest[ji->jpg_size++] = 0x10;   // table size
  // APP0 content, copy from JPEG Library (jpeg-6b)
  memcpy(&ji->jpg_dest[ji->jpg_size], app0_buf, sizeof(app0_buf));
  ji->jpg_size += sizeof(app0_buf);
}

void Create_QuantTable (JPEG_INFO *ji)
{
  const int tbl_size = 67;
  unsigned char *dqt_buf;   //dqt_buf[65]
  int no, i, offset = 0;
  
  // set JPEG Quality
  set_jpegenc_quality(ji);
  
  // just support two quantization tables
  for (no = 0; no < 2; no++) { 
    printf("DQT marker\n");
    printf("    Quantization Table size is %d bytes\n", tbl_size);
    printf("    QT No.: %x, bits: 8\n", no);
    
    // DQT marker
    ji->jpg_dest[ji->jpg_size++] = 0xFF;
    ji->jpg_dest[ji->jpg_size++] = 0xDB;
    ji->jpg_dest[ji->jpg_size++] = 0x00;   // 0x0043 = 67
    ji->jpg_dest[ji->jpg_size++] = 0x43;   // table size
    dqt_buf = &ji->jpg_dest[ji->jpg_size];
    dqt_buf[offset++] = no; // just support 8-bits JPEG now
    
    printf("    Quantization table:\n");
    for (i = 0; i < 64; i++, offset++) {
      if ((i % 8) == 0)
        printf("    ");
      
      dqt_buf[offset] = ji->Quan_Table[no].quant_val[zigzag_order[i]];
      printf(" %d", dqt_buf[offset]);
      
      if ((i % 8) == 7)
        printf("\n");
    }
    printf("\n");
    
    ji->jpg_size += offset;
    offset = 0;
  }
}

void Create_StartOfFrame (JPEG_INFO *ji)
{
  const int tbl_size = 17;
  unsigned char *sof_buf;   //sof_buf[15];
  int n, offset = 0;
  
  printf("SOF marker\n");
  printf("    Start of Frame size is %d bytes\n", tbl_size);
  
  // SOF marker
  ji->jpg_dest[ji->jpg_size++] = 0xFF;
  ji->jpg_dest[ji->jpg_size++] = 0xC0;
  ji->jpg_dest[ji->jpg_size++] = 0x00;   // 0x0011 = 17
  ji->jpg_dest[ji->jpg_size++] = 0x11;   // table size
  sof_buf = &ji->jpg_dest[ji->jpg_size];
  
  // Sample precision
  sof_buf[offset] = 8;
  printf("    Sample precision is %d bits\n", sof_buf[offset]);
  offset++;
  
  // image width and height
  sof_buf[offset++] = ji->Height >> 8;
  sof_buf[offset++] = ji->Height % 256;
  sof_buf[offset++] = ji->Width >> 8;
  sof_buf[offset++] = ji->Width % 256;
  printf("    image width is %d, height is %d\n", ji->Width, ji->Height);
  
  // Number of components
  ji->components = sof_buf[offset++] = 3;
  printf("    %d components\n", ji->components);
  
  // account how many blocks in a MCU
  ji->MCU_Blocks = 0;
  
  for (n = 0; n < ji->components; n++) {
    sof_buf[offset++] = n+1;
    switch (n) {    
    case 0:
      printf("      Y: ");
      // horizontal and vertical sampling
      ji->CompInfo[n].horz_sampling = 2;
      ji->CompInfo[n].vert_sampling = 2;
      sof_buf[offset++] = 0x22;
      // quantization table idenfifier
      ji->CompInfo[n].quan_table_id = sof_buf[offset++] = 0;
      break;
    case 1:
    case 2:
      printf("     %s ", ((n == 1) ? "Cb:" : "Cr:"));
      // horizontal and vertical sampling
      ji->CompInfo[n].horz_sampling = 1;
      ji->CompInfo[n].vert_sampling = 1;
      sof_buf[offset++] = 0x11;
      // quantization table idenfifier
      ji->CompInfo[n].quan_table_id = sof_buf[offset++] = 1;
      break;
    }
    
    // MCU blocks
    ji->MCU_Blocks += (ji->CompInfo[n].horz_sampling * ji->CompInfo[n].vert_sampling);
    
    printf("horizontal sampling is %d, vertical sampling is %d\n", 
           ji->CompInfo[n].horz_sampling, ji->CompInfo[n].vert_sampling);
    printf("         quantization table idenfifier is %d\n",
           ji->CompInfo[n].quan_table_id);    
  }
  
  // SOF marker finished
  ji->jpg_size += offset;
  
  // caculate mcu variables
  ji->MCU_Width = ji->CompInfo[0].horz_sampling / ji->CompInfo[1].horz_sampling * 8;
  ji->MCU_Height = ji->CompInfo[0].vert_sampling / ji->CompInfo[1].vert_sampling * 8;
  ji->Row_MCUs = div_round_up(ji->Width, ji->MCU_Width);
  printf("\nMCU_Width=%d, MCU_Height=%d, MCU_blocks=%d, MCUs in row=%d\n", 
         ji->MCU_Width, ji->MCU_Height, ji->MCU_Blocks, ji->Row_MCUs);
         
  ji->Total_MCU = div_round_up(ji->Width, 16) * div_round_up(ji->Height, 16);
  printf("Total MCU is %d\n\n", ji->Total_MCU);
}

void Create_HuffmanTable (JPEG_INFO *ji)
{
  const int dc_size = 31;
  const int ac_size = 181;
  //unsigned char dc_buf[29];
  //unsigned char ac_buf[179];
  unsigned char *dht_buf;
  unsigned char *bitsptr;
  unsigned char *hvalptr;
  int no, i, offset = 0, count = 0;
  
  // initialize huffman table
  set_huff_table(ji);
  
  // just support two tables for DC&AC huffman table
  for (no = 0; no < 4; no++) { 
    printf("DHT marker\n");
    printf("    Huffman Table size is %d bytes\n", ((no%2)?ac_size:dc_size));
    printf("    %s table, no is %d\n", ((no%2)?"AC":"DC"), no/2);
    
    // DHT marker
    ji->jpg_dest[ji->jpg_size++] = 0xFF;
    ji->jpg_dest[ji->jpg_size++] = 0xC4;
    
    if (no%2 == 0) {
      ji->jpg_dest[ji->jpg_size++] = 0x00;   // 0x001F = 31
      ji->jpg_dest[ji->jpg_size++] = 0x1F;   // table size
      
      bitsptr = ji->DC_Huff_Table[no/2].bits;
      hvalptr = ji->DC_Huff_Table[no/2].huff_val;
    } else {
      ji->jpg_dest[ji->jpg_size++] = 0x00;   // 0x00B5 = 181
      ji->jpg_dest[ji->jpg_size++] = 0xB5;   // table size
      
      bitsptr = ji->AC_Huff_Table[no/2].bits;
      hvalptr = ji->AC_Huff_Table[no/2].huff_val;
    }
    
    // pointer to DHT content
    dht_buf = &ji->jpg_dest[ji->jpg_size];
    
    // table no
    dht_buf[offset++] = (((no%2)<<4) & 0x10) | ((no/2)&0x0f);
    
    printf("    codesize(bits): \n");
    printf("    ");
    for (i = 1; i < 17; i++, offset++) {
      dht_buf[offset] = bitsptr[i];
      printf(" %d", dht_buf[offset]);
      count += dht_buf[offset];
    }
    
    printf("\n    total symbol is %d, codeword: \n", count);
    for (i = 0; i < count; i++, offset++) {
      if ((i % 16) == 0)  printf("    ");
      dht_buf[offset] = hvalptr[i];
      printf(" %d", dht_buf[offset]);
      if ((i % 16) == 15) printf("\n");
    }
    if (count % 16) printf("\n");
    printf("    --------------------------------------------------\n\n");
    
    // next DHT marker
    ji->jpg_size += offset;
    offset = 0;
    count = 0;
  }
}

void Create_StartOfScan (JPEG_INFO *ji)
{
  const int tbl_size = 12;
  unsigned char *sos_buf;   //sos_buf[10];
  int n, offset = 0;
  
  printf("SOS marker\n");
  printf("    Start of Scan size is %d bytes\n", tbl_size);
  
  // SOS marker
  ji->jpg_dest[ji->jpg_size++] = 0xFF;
  ji->jpg_dest[ji->jpg_size++] = 0xDA;
  ji->jpg_dest[ji->jpg_size++] = 0x00;   // 0x000C = 12
  ji->jpg_dest[ji->jpg_size++] = 0x0C;   // table size
  sos_buf = &ji->jpg_dest[ji->jpg_size];  
  
  // component count
  sos_buf[offset++] = 3;
  
  // component descriptor
  for (n = 0; n < 3; n++) {
    sos_buf[offset++] = n+1;
    switch (n) {
    case 0:
      printf("      Y: ");
      sos_buf[offset] = 0x00; // DC Huffman table
      break;
    case 1:
      printf("     Cb: ");
      sos_buf[offset] = 0x11; // AC Huffman table
      break;
    case 2:
      printf("     Cr: ");
      sos_buf[offset] = 0x11; // AC Huffman table
      break;
    }
    
    printf("Huffman table: DC is %d, AC is %d\n", 
           (sos_buf[offset]>>4)&0x0f, sos_buf[offset]&0x0f);
    offset++;
  }
  
  // spectral selection start and end
  sos_buf[offset] = 0x00;
  sos_buf[offset+1] = 0x3F;
  printf("    Spectral selection start is %d, end is %d\n", 
         sos_buf[offset], sos_buf[offset+1]);
  offset += 2;
  
  // successive approximation
  sos_buf[offset] = 0x00;
  printf("    Successive approximation is (%d, %d)\n\n",
         ((sos_buf[offset]>>4)&0x0f), (sos_buf[offset]&0x0f));
  offset++;
  
  // SOS marker finished
  ji->jpg_size += offset;
}

void Create_EndOfImage (JPEG_INFO *ji)
{
  unsigned char ones;
  
  // write last data
  if (ji->DataStream.used_bits > 0) {
    if (ji->DataStream.used_bits > 8) {
      ji->jpg_dest[ji->jpg_size++] = (unsigned char)(ji->DataStream.data >> 8);
      ones = (1<<(16-ji->DataStream.used_bits)) - 1;
      ji->jpg_dest[ji->jpg_size++] = (unsigned char)ji->DataStream.data | ones;
    } else {
      ones = (1<<(8-ji->DataStream.used_bits)) - 1;
      ji->jpg_dest[ji->jpg_size++] = (unsigned char)(ji->DataStream.data>>8) | ones;
    }
  }
  
  // EOI marker
  ji->jpg_dest[ji->jpg_size++] = 0xFF;
  ji->jpg_dest[ji->jpg_size++] = 0xD9;
}

void main (int argc, char* argv[])
{
  JPEG_INFO *ji;
  int quality = -1;
  float cr;
  FILE *f;
  
  if (argc < 2) {
    printf("Usage: need a bmp filename\n");
    exit(0);
  }
  
  // quality parameter
  if (argc == 3) {
    quality = atoi(argv[2]);
  }
  if (quality < 0 || quality > 100) {
    quality = 75;   //default quality
  }
  
  // allocate jpeg information structure
  ji = malloc(sizeof(JPEG_INFO));
  if (ji == NULL) {
    printf("Memory Insufficient!\n");
    exit(0);
  }
  memset(ji, 0, sizeof(JPEG_INFO));
  ji->Quality = quality;
  
  // prepare rgb to ycbcr convertion
  create_rgb2ycc_table();
  
#ifdef JPG_DEBUG
  prepare_range_limit_table(ji);
#endif //JPG_DEBUG

  // read bmp and convert to ycbcr color space
  if (!read_bmp(ji, argv[1])) {
    goto jpeg_encode_done;
  }
  
  // allocate jpeg destination buffer
  ji->jpg_dest = malloc((ji->Width*ji->Height)+((ji->Width*ji->Height)>>1));
  if (ji->jpg_dest == NULL) {
    printf("Memory Insufficient for JPEG destination!\n");
    goto jpeg_encode_done;
  }
  
  // create JPEG file header
  Create_StartOfImage(ji);   //SOI
  Create_QuantTable(ji);     //DQT
  Create_StartOfFrame(ji);   //SOF
  Create_HuffmanTable(ji);   //DHT
  Create_StartOfScan(ji);    //SOS
  
  // start compression, assume no any errors
  Encode_MCU(ji);
  
  Create_EndOfImage(ji);     //EOI
  
  cr = (float)ji->img_size / ji->jpg_size;
  printf("JPEG size is %d bytes\nCompression Rate = %.1f\n", 
         ji->jpg_size, cr);
  
  // write to .jpg file
  if ((f = fopen("test.jpg", "wb")) != NULL) {
    fwrite(ji->jpg_dest, ji->jpg_size, 1, f);
    fclose(f);
  }
  
#ifdef JPG_DEBUG
  InvDCT_2D(ji);
  write_bmp(ji);
#endif //JPG_DEBUG

jpeg_encode_done:  
  if (ji->jpg_dest != NULL)
    free(ji->jpg_dest);
  if (ji->Y_Data != NULL)
    free(ji->Y_Data);
#ifdef JPG_DEBUG
  if (ji->Range_Limit != NULL)
    free(ji->Range_Limit);
  if (ji->dctY_Data != NULL)
    free(ji->dctY_Data);  
#endif //JPG_DEBUG
  free(ji);
}
