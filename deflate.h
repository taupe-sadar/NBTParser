#include <vector>

bool zlib_decode(unsigned char const * buf, int length,  std::vector<unsigned char>& decoded);
bool gzip_decode(unsigned char const * buf, int length, std::vector<unsigned char>& decoded);