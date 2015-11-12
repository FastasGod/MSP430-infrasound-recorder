#include "audioBufferHandler.h"
#include "config.h"

void audioBufferHandler::setupBuffers()
{
  currentBuffer = 0;
  bufferPosition = 0;
  numReadyBuffers = 0;  //Used for FIFO operation of readyBuffers
  bufferReady = false;
}


char audioBufferHandler::isBufferReady()
{
  return bufferReady;
  //if(3);
}
void audioBufferHandler::addSample(short incomingVal)
{
  //saves incoming value to buffer, then increments buffer position
  //selects new buffer if current buffer is full
  audioBuffers[currentBuffer][bufferPosition] = incomingVal;
  
  
  
  if(++bufferPosition >= BUFFER_SIZE)
  {
    //used to send appropriate buffer to writtingHandler
    readyBuffers[numReadyBuffers] = currentBuffer;
    bufferReady = true;
    
    
    bufferPosition = 0;
    if(++currentBuffer >= BUFFER_COUNT)
      currentBuffer = 0;
  }
  
}
