#include <vector>
#include <iostream>

#include "deflate.h"
#include "bit_parser.h"

std::string to_bin(int v,int size = 0)
{
  std::string s;
  while(v>0 || size > 0)
  {
    if( v & 1)
      s = "1" + s;
    else
      s = "0" + s;
    v >>= 1;
    size--;
  }
  return s;
}

class Dictionary
{
public:
  Dictionary(std::vector<int> const & alphabetSizes);
  int getCode(int length, int value) const;
private:
  std::vector<std::vector<int>> words;
  std::vector<int> startOffsets;
  std::vector<int> uncompleteCode;
};

class DictionaryParser
{
public:
  DictionaryParser(BitParser & bp, Dictionary const & dict);
  int parse();
private:
  Dictionary const m_dict; 
  BitParser & m_bp;
};


Dictionary::Dictionary(std::vector<int> const & alphabetSizes)
{
  int maxLength = 0;
  for(int const & length: alphabetSizes)
    maxLength = length > maxLength ? length : maxLength;
  
  words.resize(maxLength+1);
  startOffsets.resize(maxLength+1);
  uncompleteCode.resize(maxLength+1);
  
  int code = 0;
  for(int length = 1; length<= maxLength; length++)
  {
    int startOffset = code;
    startOffsets[length] = startOffset;
    int maxCode = (1<<length) - code;
    std::vector<int> & codes = words[length];
    
    for(int word = 0; word <alphabetSizes.size(); word++)
    {
      if(alphabetSizes[word] == length)
      {
        if( code - startOffset >= maxCode)
          throw std::runtime_error("Invalid dictionary");
        codes.push_back(word);
        code++;
      }
    }
    uncompleteCode[length] = code;
    code <<= 1; 
  }
  
  // std::cout << "--- dictionary debug ---" << std::endl;
  // for(int length = 1; length<= maxLength; length++)
  // {
    // std::cout << "length : " << length << std::endl;
    // auto & v = words[length];
    // int offset = startOffsets[length];
    // for(int i=0; i < v.size(); i++)
    // {
      // std::cout << "  " << to_bin(offset+i,length) << " -> " << v[i] << std::endl;
    // }
  // }
}

int Dictionary::getCode(int length, int value) const
{
  if( length >= uncompleteCode.size() )
  {
    throw std::runtime_error("Too large code asked");
  }
  if( uncompleteCode[length] <= value )
    return -1;
  int offset = startOffsets[length];
  return words[length][value - offset];
}

DictionaryParser::DictionaryParser(BitParser & bp, Dictionary const & dict): m_bp(bp),m_dict(dict)
{
}

int DictionaryParser::parse()
{
  int value = 0;
  int length = 0;
  
  while(1)
  {
    int bit = m_bp.parse(1);
    value = (value << 1) + bit;
    length++;
    int decoded = m_dict.getCode(length,value);
    if( decoded != -1)
      return decoded;
  }
}

std::vector<int> read_dictionary(DictionaryParser & dp, BitParser & bp, int maxSize)
{
  std::vector<int> alphabet;
  while(alphabet.size() < maxSize)
  {
    int lengthLength = dp.parse();
    int repeat = 0;
    int repeatCode = 0;

    if( lengthLength == 16 )
    {
      if( alphabet.empty())
        throw std::runtime_error("Repeat code with empty dictionary !");

      repeat = 3 + bp.parse(2);
      repeatCode = alphabet.back();
    }
    else if( lengthLength == 17 )
      repeat = 3 + bp.parse(3);
    else if( lengthLength == 18 )
      repeat = 11 + bp.parse(7);
    else
    {
      //std::cout << alphabet.size() <<" : " << lengthLength << std::endl;
      alphabet.push_back(lengthLength);
      continue;
    }
    
    //std::cout << "[" << lengthLength << "]  Expanding (" << repeatCode << ") x " << repeat << " times" << std::endl;
    for(int i=0; i < repeat; i++)
    {
      alphabet.push_back(repeatCode);
    }
  }
  return std::move(alphabet);
}

void read_block(BitParser & bp, std::vector<unsigned char>& output)
{
  int literalCodes = bp.parse(5) + 257;
  int distanceCodes = bp.parse(5) + 1;
  int lengthCodes = bp.parse(4) + 4;
  std::vector<int> baseLengthAlphabet(19,0);

  static int const lengthLengths[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
  for(int i = 0; i < lengthCodes; i++)
  {
    baseLengthAlphabet[lengthLengths[i]]=bp.parse(3);
  }
  
  // std::cout << "LITERAL  : " << literalCodes << std::endl;
  // std::cout << "DISTANCE  : " << distanceCodes << std::endl;
  // std::cout << "LENGTH  : " << lengthCodes << std::endl;
  
  // for(int i = 0; i < lengthCodes; i++)
  // {
    // std::cout << "baseAlphabet["<<i<<"] : " << baseLengthAlphabet[i] << std::endl;
  // }

  Dictionary dict(baseLengthAlphabet);
  DictionaryParser lengthDp(bp,dict);

  std::vector<int> litralLengthAlphabet = read_dictionary(lengthDp, bp, literalCodes);
  Dictionary dictLength(litralLengthAlphabet);
  DictionaryParser litteralDp(bp,dictLength);

  std::vector<int> distanceAlphabet = read_dictionary(lengthDp, bp, distanceCodes);
  Dictionary dictDistance(distanceAlphabet);
  DictionaryParser distanceDp(bp,dictDistance);
  
  
  static int const lengthValue[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
  static int const lengthBits[29] =  {0,0,0,0,0,0,0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5,  0};

  static int const distanceValue[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
  static int const distanceBits[30] =  {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13};
  
  int startSize = output.size();

  int litteral = litteralDp.parse();
  while( litteral != 256 )
  {
    if( litteral < 256 )
    {
      output.push_back(litteral);
    }
    else if( litteral <= 285 )
    {
      int lengthCode = litteral-257;
      int length = lengthValue[lengthCode] + bp.parse(lengthBits[lengthCode]);

      int distanceCode = distanceDp.parse();
      int distance = distanceValue[distanceCode] + bp.parse(distanceBits[distanceCode]);

      int copyIdx = output.size() - distance;
      if( copyIdx < startSize )
        throw std::runtime_error("Invalid length/distance (" + std::to_string(length) + "/" + std::to_string(distance) + "), output is size " + std::to_string(output.size()) + "");
      for(int i = 0; i < length; i++)
      {
        output.push_back(output[copyIdx++]);
      }
    }
    else
    {
      throw std::runtime_error("Invalid litteral (" + std::to_string(litteral) + ")");
    }
    
    litteral = litteralDp.parse();
  }
}

int deflate_decode(unsigned char const * buf, int size, std::vector<unsigned char>& raw)
{
  BitParser bp(buf,size);

  int bfinal = 0;
  int btype = 0;
  
  do
  {
    bfinal = bp.parse(1);
    btype = bp.parse(2);
    if( btype == 3)
    {
      std::cout << "Deflate invalid compression " << btype << std::endl;
      break;
    }
    else if( btype !=2)
    {
      std::cout << "only compression type 2 is supported for now " << btype << std::endl;
      break;
    }
    else
    {
      try
      {
        read_block(bp,raw);
      }
      catch(std::exception const & e)
      {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        break;
      }
      break;
    }
    
  }
  while(bfinal == 0);
  
  int sizeRead = bp.bytePos() + ((bp.bitPos() == 0)?0:1);
  return sizeRead;
}

bool zlib_decode(unsigned char const * buf, int length, std::vector<unsigned char>& decoded)
{
  int cm = (buf[0] & 0x0f);
  int cinfo = (buf[0] & 0xf0) >> 4;
  
  int fcheck = (buf[1] & 0x1f);
  int fdict = (buf[1] & 0x20) >> 5;
  int flevel = (buf[1] & 0xc0)>>6;
  
  int dictionary = 0;
  if(fdict)
  {
    dictionary = (buf[2]) + (int(buf[3])<<8) + (int(buf[4])<<16) + (int(buf[5])<<24);
  }
  
  // std::cout << "CM         " << cm << std::endl;
  // std::cout << "CINFO      " << cinfo << std::endl;
  // std::cout << "FCHECK     " << fcheck << std::endl;
  // std::cout << "FLEVEL     " << flevel << std::endl;
  
  // std::cout << "DICTIONARY " << dictionary << std::endl;
  
  if( fdict )
  {
    std::cout << "dictionary not supported" << std::endl;
    return false;
  }

  decoded.resize(0);
  if( cm != 8 )
  {
    std::cout << "unknown compression method cm = " << cm << std::endl;
    return false;
  }

  int headerSize = fdict?6:2;
  
  int read = 0;
  try
  {
    read = deflate_decode(&buf[headerSize], length - headerSize , decoded);
  }
  catch(std::exception const & e)
  {
    std::cerr << "deflate_decode failed : " << e.what() << std::endl;
    return false;
  }
  
  if( headerSize + read + 4 != length)
  {
    std::cout << "Zlib : read size and length doesnt match !" << std::endl;
    return false;
  }
  
  unsigned char const * endBuf = &buf[headerSize+read];
  char adlerBuf[20];
  sprintf(adlerBuf,"0x%02X%02X%02X%02X", endBuf[0], endBuf[1], endBuf[2], endBuf[3]);
  //std::cout << adlerBuf << std::endl;
  
  return true;
}

bool gzip_decode(unsigned char const * buf, int length, std::vector<unsigned char>& decoded)
{
  int id1 = buf[0];
  int id2 = buf[1];
  int cm = buf[2];
  int flg = buf[3];
  
  bool ftext = (flg & 0x01);
  bool fhrc = ((flg & 0x02)>>1);
  bool fextra = ((flg & 0x04)>>2);
  bool fname = ((flg & 0x08)>>3);
  bool fcomment = ((flg & 0x10)>>4);
  
  unsigned int mtime = (buf[4]) + (int(buf[5])<<8) + (int(buf[6])<<16) + (int(buf[7])<<24);
  int xfl = buf[8];
  int os = buf[9];
  
  int idx = 10;
  int xlen = 0;
  if (fextra)
  {
    xlen = buf[idx] + (int(buf[idx+1])<<8);
    idx+=2;
  }
  
  std::vector<unsigned char> extraField(xlen);
  for(int i = 0; i < xlen;i++)
    extraField[i] = buf[idx++];
  
  std::vector<unsigned char> strBuf;
  if (fname)
  {
    unsigned char b = buf[idx++];
    while(b!=0)
    {
      strBuf.push_back(b);
      b = buf[idx++];
    }
  }
  std::string originalFilename(strBuf.data(),strBuf.data()+ strBuf.size());
  
  strBuf.clear();
  if (fcomment)
  {
    unsigned char b = buf[idx++];
    while(b!=0)
    {
      strBuf.push_back(b);
      b = buf[idx++];
    }
  }
  std::string fileComment(strBuf.data(),strBuf.data()+ strBuf.size());
  
  int crc16 = 0;
  if (fhrc)
  {
    crc16 = buf[idx] + (int(buf[idx+1])<<8);
    idx+=2;
  }
  
  for(int i = 0; i < xlen;i++)
  {
    char buff[5];
    sprintf(buff,"%02X",extraField[i]);
    std::cout << "  " << buff << std::endl;
  }

  // std::cout << "ID1 : " << id1 << std::endl;
  // std::cout << "ID2 : " << id2 << std::endl;
  // std::cout << "CM : " << cm << std::endl;
  // std::cout << "FLG : " << flg << std::endl;
  // std::cout << "MTIME : " << mtime << std::endl;
  // std::cout << "XFL : " << xfl << std::endl;
  // std::cout << "OS : " << os << std::endl;
  // std::cout << "Original Filename : " << originalFilename << std::endl;
  // std::cout << "File Comment : " << fileComment << std::endl;

  if( cm != 8 )
  {
    std::cout << "unknown compression method cm = " << cm << std::endl;
    return false;
  }

  int read = 0;
  decoded.resize(0);
  try
  {
    read = deflate_decode(&buf[idx], length - idx , decoded);
  }
  catch(std::exception const & e)
  {
    std::cerr << "deflate_decode failed : " << e.what() << std::endl;
    return false;
  }
  
  if( idx + read + 8 != length)
  {
    std::cout << "Gzip : read size and length doesnt match !" << std::endl;
    return false;
  }
  idx+=read;

  int crc32 = buf[idx] + (int(buf[idx+1])<<8) + (int(buf[idx+2])<<16) + (int(buf[idx+3])<<24);
  idx+=4;

  int isize = buf[idx] + (int(buf[idx+1])<<8) + (int(buf[idx+2])<<16) + (int(buf[idx+3])<<24);
  idx+=4;
  //std::cout << "ISIZE : " << isize << std::endl;

  return true;
}
