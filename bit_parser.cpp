#include <stdexcept>
#include "bit_parser.h"

BitParser::BitParser(unsigned char const * buf, int size):m_buf(buf),m_size(size)
{
  
};

int BitParser::parse(int bits)
{
  int res = 0;
  int exp = 0;
  while( bits > 0)
  {
    if( byteOffset >= m_size)
    {
      std::string errMsg("Try accessing outside bit Parser buffer !");
      errMsg += " (" + std::to_string(byteOffset) + ")";
      throw std::runtime_error(errMsg);
    }
    
    int writingBits = (bits >= (8-bitOffset))?(8-bitOffset):bits;
    res += ((m_buf[byteOffset] >> bitOffset) & ((1<<writingBits)-1)) << exp;
    
    bitOffset+=writingBits;
    bits -= writingBits;
    
    if( bitOffset >= 8 )
    {
      byteOffset++;
      exp += writingBits;
      bitOffset = 0;
    }
  }
  return res;
}

int BitParser::bytePos()
{
  return byteOffset;
}

int BitParser::bitPos()
{
  return bitOffset;
}