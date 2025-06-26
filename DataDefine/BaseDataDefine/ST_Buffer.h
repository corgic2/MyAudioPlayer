#pragma once
#include <cstdint>

extern "C"
{
#include "libavutil/mem.h"
}

class ST_Buffer
{
public:
    ST_Buffer(int bufferSize);
    ~ST_Buffer();

    uint8_t* GetRawBuffer()
    {
        return buffer;
    }

private:
    uint8_t* buffer = nullptr;
};
