/*
 * audioProcessor.cpp
 *
 *  Created on: Sep 2, 2015
 *      Author: e4e
 */


#include "audioProcessor.h"
#include "config.h"
//There is currently nothing enabled/ written here


//LOWPASSFilter LPF;

void audioProcessor::setupProcessor()
{
	for(int i = 0; i < FILTER_ORDER+1;i++)
	{
		//LPF.previousInputs[i] = 0;
		//LPF.previousOutputs[i] = 0;
	}
}

void audioProcessor::processAudio(short dataBuffer[BUFFER_SIZE/2])
{
	//currently unimplemented
	performLPF(dataBuffer);

	//currently unimplemented
	searchForSound(dataBuffer);
        
        //currently disabled
        downsampleAudio(dataBuffer);

  

}
void audioProcessor::downsampleAudio(short dataBuffer[BUFFER_SIZE/2])
{
  //TODO: CHECK IMPLEMENTATION
    //NOTE: I start at 1 and OVERSAMPLE_MULTIPLIER because element 0 already equals element 0
  	short shortCount = 1;
	short longCount = OVERSAMPLE_MULTIPLIER;

	//this should downsample the buffer
	//though the rest of the samples will remain
	while(longCount < BUFFER_SIZE/2)
	{
		dataBuffer[shortCount] = dataBuffer[longCount];

		shortCount+=1;
		longCount+= OVERSAMPLE_MULTIPLIER;
	}
}



void audioProcessor::performLPF(short dataBuffer[BUFFER_SIZE/2])
{
	//TODO: IMPLEMENT
	//It appears IIR filters will be better for embedded systems
		//look into the exponential moving average filter
		//http://coactionos.com/embedded%20design%20tips/2013/10/04/Tips-An-Easy-to-Use-Digital-Filter/

	//also this one http://www.mikroe.com/chapters/view/73/chapter-3-iir-filters/
}
void audioProcessor::searchForSound(short dataBuffer[BUFFER_SIZE/2])
{
	//TODO:IMPLEMENT
}
