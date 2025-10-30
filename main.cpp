#include <iostream>

#include "nbt_parser.h"

int main(int argc, char** argv)
{
  try
  {
    int arg_idx = 1;
    if(argc <= 1)
    {
      std::cout << "Usage : nbt_parser <mode> " << std::endl;
      std::cout << "  Modes : chunk_blocks|map|search" << std::endl;
      return 1;
    }
    
    std::string mode(argv[arg_idx++]);
    if( mode == "chunk_blocks" )
    {
      if(argc <= 5)
      {
        std::cout << "Usage : nbt_parser chunk_blocks <filename> <min_height> <max_height> [(chunk_x chunk_z) ...]" << std::endl;
        return 1;
      }
      std::string file(argv[arg_idx++]);
      std::vector<Chunk> chunks;
      int minHeight = std::stoi(argv[arg_idx++]);
      int maxHeight = std::stoi(argv[arg_idx++]);
      
      while(argc >= arg_idx + 2)
      {
        int x = std::stoi(argv[arg_idx++]);
        int z = std::stoi(argv[arg_idx++]);
        if( x < 0 || x >=32 || z < 0 || z >= 32 )
          throw std::runtime_error("Coordinates must be in [0:31]");
        
        chunks.push_back(Chunk(x,z));
      }
      
      parse_chunks(file,chunks);
      
      std::vector<std::string> ret = block_layers(chunks,minHeight,maxHeight);
      for(auto & s : ret)
      {
        std::cout << s << std::endl;
      }
    }
    else if( mode == "search" )
    {
      if(argc <= 3)
      {
        std::cout << "Usage : nbt_parser search <filename> <block_type> [(chunk_x chunk_z) ...] [<limit>]" << std::endl;
        return 1;
      }
      
      std::string file(argv[arg_idx++]);
      
      std::string blockType(argv[arg_idx++]);
      
      std::vector<Chunk> chunks;
      while(argc >= arg_idx + 2)
      {
        int x = std::stoi(argv[arg_idx++]);
        int z = std::stoi(argv[arg_idx++]);
        if( x < 0 || x >=32 || z < 0 || z >= 32 )
          throw std::runtime_error("Coordinates must be in [0:31]");
        
        chunks.push_back(Chunk(x,z));
      }

      int limit = -1;
      if( arg_idx < argc )
        limit = std::stoi(argv[arg_idx++]);

      parse_chunks(file,chunks);

      std::vector<std::string> ret = block_search(chunks,blockType,limit);
       for(auto & s : ret)
      {
        std::cout << s << std::endl;
      }
    }
    else if(mode == "map")
    {
      if(argc != 3)
      {
        std::cout << "Usage : nbt_parser map <filename>"<< std::endl;
        return 1;
      }
      std::string file(argv[arg_idx++]);
      std::unique_ptr<Tag> mapTag;
      parse_gzip_nbt(file,mapTag);
      std::vector<unsigned char> pixels = getElement(mapTag.get(),"data/colors")->getByteArray();;
      for(auto e: pixels)
        std::cout << int(e) << " ";
      std::cout << std::endl;
    }
    else
    {
      std::cout << "Not implemented " << argv[1] << std::endl;
      return 1;
    }
    return 0;
  }
  catch(std::invalid_argument const& e)
  {
    std::cout << "Integer Expected : " << e.what() << std::endl;
    return 1;
  }
  catch(std::exception const& e)
  {
    std::cout << "Error : " << e.what() << std::endl;
    return 1;
  }
  std::vector<Chunk> chunks;
  chunks.push_back(Chunk(21,25));
  parse_chunks(std::string(argv[1]),chunks);
  
  std::cout << chunks[0].tag->str(0);
  
  //Gzip
  if(argc <3 )
    return 1;
  
  std::unique_ptr<Tag> levelTag;
  parse_gzip_nbt(std::string(argv[2]),levelTag);
  
  std::cout << levelTag->str(0);

  return 0;
}