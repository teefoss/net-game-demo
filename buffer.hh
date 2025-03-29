//
//  buffer.hh
//  NetTest2
//
//  Created by Thomas Foster on 3/17/25.
//

#ifndef buffer_hh
#define buffer_hh

#include <stdlib.h>

struct Buffer {
    char * data;
    size_t size;
    size_t capacity;
};

void BufferInit(Buffer * buffer, size_t size);

/// Append `data` of `size` bytes to buffer.
/// - returns: Returns `false` if the write would result in an overflow.
bool BufferWrite(Buffer * buffer, void * data, size_t size);

/// Remove `size` bytes of data from the beginning of the buffer.
/// - returns: Returns `false` if the read would result in an underflow.
bool BufferRead(Buffer * buffer, void * data, size_t size);

void BufferGrowIfNeeded(Buffer * buffer, size_t needed);
void BufferClear(Buffer * buffer);

#endif /* buffer_hh */
