//
//  buffer.cc
//  NetTest2
//
//  Created by Thomas Foster on 3/17/25.
//

#include "buffer.hh"
#include <string.h>

void BufferGrowIfNeeded(Buffer * buffer, size_t needed)
{
    size_t available = buffer->capacity - buffer->size;

    if ( available >= needed ) {
        return;
    }

    while ( available < needed ) {
        buffer->capacity *= 2;
        available = buffer->capacity - buffer->size;
    }

    buffer->data = (char *)realloc(buffer->data, buffer->capacity);
}

void BufferInit(Buffer * buffer, size_t size)
{
    if ( size < 256 ) {
        size = 256;
    }

    if ( buffer->data ) {
        free(buffer->data);
    }

    buffer->data = (char *)malloc(size);
    buffer->size = 0;
    buffer->capacity = size;
}

void BufferClear(Buffer * buffer)
{
    buffer->size = 0;
}

bool BufferWrite(Buffer * buffer, void * data, size_t size)
{
    BufferGrowIfNeeded(buffer, size);
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;

    return true;
}

bool BufferRead(Buffer * buffer, void * data, size_t size)
{
    if ( buffer->size < size ) {
        return false; // Underflow
    }

    if ( data ) {
        memcpy(data, buffer->data, size);
    }

    memmove(buffer->data, buffer->data + size, buffer->size - size);
    buffer->size -= size;

    return true;
}
