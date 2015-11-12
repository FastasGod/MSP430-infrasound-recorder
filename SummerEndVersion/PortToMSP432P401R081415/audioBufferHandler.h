#ifndef audioBufferHandler_h
#define audioBufferHandler_h

#include "config.h"
class audioBufferHandler
{
  public:
    void addSample(short);
    char isBufferReady();
    void setupBuffers();
    
  private:
    short audioBuffers[BUFFER_COUNT][BUFFER_SIZE];
    char readyBuffers[BUFFER_COUNT];
    char numReadyBuffers;
    char currentBuffer;
    short bufferPosition;
    
    char bufferReady;
}; extern audioBufferHandler buffers;






#endif
