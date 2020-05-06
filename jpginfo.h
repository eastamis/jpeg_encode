/* JPEG marker codes */ 
#define TEM  0x01 
#define SOF  0xc0 
#define DHT  0xc4 
#define JPGA 0xc8 
#define DAC  0xcc 
#define RST  0xd0 
#define SOI  0xd8 
#define EOI  0xd9 
#define SOS  0xda 
#define DQT  0xdb 
#define DNL  0xdc 
#define DRI  0xdd 
#define DHP  0xde 
#define EXP  0xdf 
#define APP0 0xe0 
#define APP1 0xe1
#define APP2 0xe2
#define APP3 0xe3
#define APPC 0xec
#define APPD 0xed
#define APPE 0xee
#define JPG  0xf0 
#define COM  0xfe 


/* Global variables */
typedef struct {
  unsigned short data;
  int used_bits;
} STREAM_STATUS;

typedef struct {
  unsigned char horz_sampling;
  unsigned char vert_sampling;
  unsigned char quan_table_id;
  unsigned char dc_table_id;
  unsigned char ac_table_id;
} COMPONENT_INFO;

typedef struct {
  unsigned char bits[17];
  unsigned char huff_val[12];
  unsigned short codeword[12];
  unsigned short codesize[12];
} HUFFMAN_TABLE_DC;

typedef struct {
  unsigned char bits[17];
  unsigned char huff_val[162];
  unsigned short codeword[256];
  unsigned short codesize[256];
} HUFFMAN_TABLE_AC;

typedef struct {
  unsigned char quant_val[64];
} QUANT_TABLE;

typedef struct {  
  int           Quality;
  
  // Quantization Table
  QUANT_TABLE   Quan_Table[2];
  
  // Components in MCU
  int components;
  COMPONENT_INFO CompInfo[3];
  
  // MCU information
  int           MCU_Blocks;
  int           Row_MCUs;
  int           MCU_Width;
  int           MCU_Height;
  int           Total_MCU;
  
  // Last DC coff value
  int           LastDC[3];
  
  // Huffman table
  HUFFMAN_TABLE_DC DC_Huff_Table[2];
  HUFFMAN_TABLE_AC AC_Huff_Table[2];
  
  // DRI marker, restart interval
  //int           Restart_Interval;
  
  // picture area
  int           Width;
  int           Height;

  // ycbcr data buffer
  unsigned char *Y_Data;
  unsigned char *Cb_Data;
  unsigned char *Cr_Data;
  
  unsigned int *dctY_Data;
  unsigned int *dctCb_Data;
  unsigned int *dctCr_Data;
  
  unsigned char *jpg_dest;
  unsigned long jpg_size;
  unsigned long img_size;
  
  // data stream status
  STREAM_STATUS DataStream;
  
#ifdef JPG_DEBUG  
  // image value range limit
  unsigned char *Range_Limit;
#endif //JPG_DEBUG  
  
} JPEG_INFO;
