#include <string>

struct BitParser
{
  BitParser(unsigned char const * buf, int size);
  
  int parse(int bits);
  int bytePos();
  int bitPos();
// private:
  unsigned char const * m_buf;
  int const m_size;
  
  int byteOffset = 0;
  int bitOffset = 0;
};

