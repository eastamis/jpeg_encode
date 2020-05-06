#include <stdio.h>
#include <stdlib.h>
#include "jpginfo.h"

/* read data stream bits, count must equal or less then 16 */
void WriteBits (JPEG_INFO *ji, int code, int size)
{
  unsigned short dbuf, dmask;
  int leftbits;
  
  if ((ji->DataStream.used_bits+size) >= 16) {
    leftbits = 16 - ji->DataStream.used_bits;
    dmask = ((1<<leftbits) - 1); /* mask off any extra bits in code */
    dbuf = (unsigned short)(code>>(size-leftbits)) & dmask;
    
    ji->DataStream.data |= dbuf;
    ji->jpg_dest[ji->jpg_size] = (unsigned char)(ji->DataStream.data >> 8);
    if (ji->jpg_dest[ji->jpg_size] == 0xFF) // 0xFF must follow 0x00
      ji->jpg_dest[(ji->jpg_size++)+1] = 0;
    ji->jpg_size++;
    ji->jpg_dest[ji->jpg_size] = (unsigned char)ji->DataStream.data;
    if (ji->jpg_dest[ji->jpg_size] == 0xFF) // 0xFF must follow 0x00
      ji->jpg_dest[(ji->jpg_size++)+1] = 0;
    ji->jpg_size++;
    
    ji->DataStream.data = 0;
    ji->DataStream.used_bits = 0;    
    size -= leftbits;    
  }
  
  if (size <= 0)
    return;
  
  dmask = ((1<<size) - 1); /* mask off any extra bits in code */
  dbuf = (unsigned short)code & dmask;
  dbuf <<= (16-(size+ji->DataStream.used_bits));
  
  ji->DataStream.data |= dbuf;
  ji->DataStream.used_bits += size;
}
