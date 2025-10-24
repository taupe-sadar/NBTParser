#include "nbt.h"

struct Chunk
{
  Chunk(int _x, int _z):x(_x),z(_z){};
  int x;
  int z;
  int offset;
  int sector_count;
  long timestamp;

  std::unique_ptr<Tag> tag;
};

void parse_chunks(std::string const & filename, std::vector<Chunk> & chunks);
void parse_gzip_nbt(std::string const & filename, std::unique_ptr<Tag>& tag);

std::vector<std::string> block_layers(std::vector<Chunk> const & chunks, int minHeight, int MaxHeight);

