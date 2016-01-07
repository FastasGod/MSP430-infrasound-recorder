#ifndef audioBufferHandler_h
#define audioBufferHandler_h

#include "config.h"
class audioBufferHandler
{
  public:
    void addSample(short);
    short getNumReadyBuffers();
    void setupBuffers();
    void getFullBuffer(short[BUFFER_SIZE/2]);
    
  private:
    short audioBuffers[BUFFER_COUNT][BUFFER_SIZE/2];
    signed short numReadyBuffers;
    short currentBuffer;
    short bufferPosition;
    
};// extern audioBufferHandler buffers;






#endif
