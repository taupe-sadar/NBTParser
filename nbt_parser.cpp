#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <bitset>

#include "deflate.h"
#include "nbt_parser.h"

int SECTORSIZE = 4*1024;

void parse_header(unsigned char const * buf, std::vector<Chunk>& chunks)
{
  for(auto & c : chunks)
  {
    int chunkcount = c.z * 32 + c.x;
    c.offset = (buf[4*chunkcount]<<16) + (buf[4*chunkcount +1]<<8) + buf[4*chunkcount+2];
    c.sector_count = buf[4*chunkcount+3];
    unsigned char const * bufTimestamp = buf + 32*32*4;
    c.timestamp = (buf[4*chunkcount]<<24) + (buf[4*chunkcount +1]<<16) + (buf[4*chunkcount+2]<<8) + buf[4*chunkcount];
  }
}

void parse_chunks(std::string const & filename, std::vector<Chunk> & chunks)
{
  std::ifstream ifs(filename.c_str(),std::ifstream::binary);
  if( !ifs.good() )
    throw std::runtime_error("Cannot open file " + filename);

  int const headersize= SECTORSIZE * 2;
  char bufferheader[headersize];
  
  ifs.read(bufferheader, headersize);
  
  parse_header((unsigned char*)bufferheader, chunks);

  for(auto & c : chunks)
  {
    ifs.seekg(c.offset * SECTORSIZE);
    unsigned char payload_header[5];
    ifs.read((char*)payload_header,5);
    int length = ((int)payload_header[3]) + ((int)payload_header[2]<<8) + ((int)payload_header[1]<<16) + ((int)payload_header[0]<<24);
    int compression = payload_header[4];
    
    std::string serr("Chunk[" + std::to_string(c.x) + "," + std::to_string(c.z) + "]");
    if( length <= 1 )
    {
      throw std::runtime_error(serr + " : Unexpected length " + std::to_string(length));
    }
    if( compression != 2 )
    {
      throw std::runtime_error(serr + " : Unexpected compression " + std::to_string(compression));
    }
    
    std::vector<char> data(length-1);
    ifs.read(data.data(),length-1);
    std::vector<unsigned char> nbtBuffer;
    if(!zlib_decode((unsigned char * )data.data(), length-1, nbtBuffer))
    {
      throw std::runtime_error(serr + " : zlib_decode failed");
    }

    c.tag = std::move(nbt_parse(nbtBuffer));
  }
}

void parse_gzip_nbt(std::string const & filename, std::unique_ptr<Tag>& tag)
{
  std::ifstream ifs(filename.c_str(),std::ifstream::binary);
  if( !ifs.good() )
  {
    std::cout << "Cannot open file " << filename << std::endl;
    return;
  }
  ifs.seekg(0, std::ios::end);
  int filesize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  std::vector<char> data(filesize);
  ifs.read(data.data(),filesize);
  std::vector<unsigned char> nbtBuffer;
  if(!gzip_decode((unsigned char * )data.data(), filesize, nbtBuffer))
  {
    std::cout << "gzip_decode failed" << std::endl;
    return;
  }
  tag = std::move(nbt_parse(nbtBuffer));
}

int bitBlockCoding(int numPaletteElts)
{
  int bits = 0;
  int maxElement = numPaletteElts - 1;
  while(maxElement > 0)
  {
    maxElement >>= 1;
    bits++;
  }
  if( bits < 4 )
    return 4;
  else
    return bits;
}

std::vector<std::string> chunkBlocks(std::vector<long> const & allBlocks, std::map<int,int> const & sectionMapping)
{
  int bits = bitBlockCoding(sectionMapping.size()); 
  long mask = (1 << bits) - 1;
  
  std::vector<std::string> ret;
  std::string current;
  int count = 0;
  for(long lineBlocks : allBlocks)
  {
    int bitsRemaining = 64;
    while(bitsRemaining >= bits)
    {
      current += std::to_string( sectionMapping.at(lineBlocks & mask) ) + " ";
      count++;
      if( count == 256 )
      {
        ret.push_back(current);
        current.clear();
        count = 0;
      }
      lineBlocks >>= bits;
      bitsRemaining -= bits;
    }
  }
  return ret;
}

std::string emptyLayer()
{
  std::string s;
  for(int i=0;i<256;i++)
    s += "0 ";
  return s;
}

std::vector<std::string> block_layers(std::unique_ptr<Tag> const & tag, int minHeight, int maxHeight, std::map<std::string,int> & allBlockTypes)
{
  std::vector<std::string> v;
  if( minHeight > maxHeight )
    return v;

  const Tag* tagSections = getElement(tag.get(),"Level/Sections");
  int numSections = tagSections->size();
  // std::cout << tag->str(0) << std::endl;
  // std::cout << tagSections->str(0) << std::endl;

  std::map<int , std::vector<std::string>> retSections;
  int const minSection = minHeight/16;
  int const maxSection = maxHeight/16;
  
  for(int i = 0; i < numSections ; i ++)
  {
    const Tag* tagSection = getElement(tagSections,std::to_string(i));

    const Tag* tagYPos = getElement(tagSection,"Y");
    int YPos = tagYPos->getByte();

    if( minSection > YPos || maxSection < YPos)
      continue;
    
    int boundMin = (YPos == minHeight/16)? (minHeight%16):0;
    int boundMax = (YPos == maxHeight/16)? ((maxHeight%16)+1):16;

    std::set<std::string> elts = tagSection->elements();
    if(elts.count("Palette") == 0)
      continue;
    
    const Tag* tagPalette = getElement(tagSection,"Palette");
    int numPaletteElts = tagPalette->size();

    std::map<int,int> sectionMapping;
    for(int p = 0; p < numPaletteElts ; p ++)
    {
      std::string blockType = getElement(tagPalette,std::to_string(p)+"/Name")->getString();
      
      if( blockType == "minecraft:lava" )
      {
        std::string lavaLevel = getElement(tagPalette,std::to_string(p) + "/Properties/level")->getString();
        if(lavaLevel == "0")
          blockType = "minecraft:lava_source";
      }
      
      if(allBlockTypes.count(blockType) == 0)
        allBlockTypes[blockType]=allBlockTypes.size();
      sectionMapping[p] = allBlockTypes[blockType];
    }
    
    std::vector<long> allBlocks = getElement(tagSection,"BlockStates")->getLongArray();
    retSections[YPos] = chunkBlocks(allBlocks,sectionMapping);
  }
  
  for( int y = minSection; y <= maxSection; y++ )
  {
    int boundMin = (y == minHeight/16)? (minHeight%16):0;
    int boundMax = (y ==maxHeight/16)? ((maxHeight%16)+1):16;
    if(retSections.count(y) > 0 )
    {
      auto itBegin = retSections[y].begin() + boundMin;
      auto itEnd = retSections[y].end() - (16 - boundMax);
      v.insert(v.end(),itBegin,itEnd);
    }
    else
    {
      v.resize(v.size() + boundMax - boundMin,emptyLayer());
    }
  }
  
  return v;
}

std::vector<std::string> block_layers(std::vector<Chunk> const & chunks, int minHeight, int maxHeight)
{
  std::map<std::string,int> allBlockTypes{{"minecraft:air",0}};
  std::vector<std::string> v;
  for(auto const & chunk : chunks)
  {
    std::vector<std::string> chunk_v = block_layers(chunk.tag,minHeight,maxHeight,allBlockTypes);
    v.insert(v.end(),chunk_v.begin(),chunk_v.end());
  }

  std::vector<std::string> vectorTypes(allBlockTypes.size());
  for(auto & b : allBlockTypes)
    vectorTypes[b.second] = b.first + ":" + std::to_string(b.second);

  v.insert(v.begin(),vectorTypes.begin(),vectorTypes.end());
  return v;
}

