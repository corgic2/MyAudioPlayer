#include "ST_Buffer.h"


ST_Buffer::ST_Buffer(int bufferSize)
{
    buffer = static_cast<uint8_t*>(av_malloc(bufferSize));
}

ST_Buffer::~ST_Buffer()
{
    av_free(buffer);
}
