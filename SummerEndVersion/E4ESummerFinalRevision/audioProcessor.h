/*
 * audioProcessor.h
 *
 *  Created on: Sep 2, 2015
 *      Author: e4e
 */

#ifndef AUDIOPROCESSOR_H_
#define AUDIOPROCESSOR_H_

#include "config.h"


#define FILTER_ORDER		1

class audioProcessor
{
  public:
    void setupProcessor();
    short previousInputs[FILTER_ORDER+1];
    double previousOutputs[FILTER_ORDER+1]; //Because it will have to handle decimal multiplication
    void processAudio(short[BUFFER_SIZE/2]);


  private:
    void downsampleAudio(short[BUFFER_SIZE/2]);
    void performLPF(short[BUFFER_SIZE/2]);
    void searchForSound(short[BUFFER_SIZE/2]);
};





#endif /* AUDIOPROCESSOR_H_ */
