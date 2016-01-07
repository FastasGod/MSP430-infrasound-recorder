#include <string.h>
#include "audioBufferHandler.h"
#include "config.h"

void audioBufferHandler::setupBuffers()
{
  currentBuffer = 0;
  bufferPosition = 0;
  numReadyBuffers = 0;  //Used for FIFO operation of readyBuffers
  for(int i = 0; i < BUFFER_SIZE/2;i++)
  {
    for(int j = 0; j < BUFFER_COUNT;j++)
    {
      audioBuffers[j][i] = 0;
    }
  }
  //bufferReady = false;
}
short audioBufferHandler::getNumReadyBuffers()
{
  return numReadyBuffers;
  //if(3);
}
void audioBufferHandler::addSample(short incomingVal)
{
  //saves incoming value to buffer, then increments buffer position
  //selects new buffer if current buffer is full
  audioBuffers[currentBuffer][bufferPosition] = incomingVal;
  
  
  
  if(++bufferPosition >= BUFFER_SIZE/2) //Buffer size is in bytes, but we are using shorts (2Bytes)
  {
    //used to return appropriate buffer to writtingHandler
    numReadyBuffers++;
    if(numReadyBuffers > BUFFER_COUNT)
    {
      numReadyBuffers = BUFFER_COUNT;
    }
    
    bufferPosition = 0;
    if(++currentBuffer >= BUFFER_COUNT)
      currentBuffer = 0;
  }
  
}
void audioBufferHandler::getFullBuffer(short outgoingBuffer[BUFFER_SIZE/2])
{
  short nextBuffer = currentBuffer - numReadyBuffers;
  
  if(nextBuffer < 0)
  {
    nextBuffer = BUFFER_COUNT + nextBuffer;
  }

	//Notes that a full buffer has been grabbed
  numReadyBuffers--;
  if(numReadyBuffers < 0)
    numReadyBuffers = 0;
    
    
  memcpy(outgoingBuffer,&audioBuffers[nextBuffer],BUFFER_SIZE);
  //outgoingBuffer = audioBuffers[nextBuffer];
  
  
	//NOTE:BUFFER_SIZE/2 because a short is 2 bytes
	//memcpy(outgoingBuffer,audioBuffers[nextBuffer],BUFFER_SIZE/2);
}
