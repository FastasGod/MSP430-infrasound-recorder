/*
The setup code herein is meant for the MSP432
much of the setup may need adjustment



*/

#include <stdbool.h>
#include "msp432p401r.h"
#include "driverlib/interrupt.h"
#include "driverlib/driverlib.h"
#include "msp432_startup_ccs.c"


#include "config.h"
#include "dataWriter.h"
#include "audioBufferHandler.h"
#include "audioProcessor.h"

#ifndef DEBUG
  #define DEBUG                true   //if it is false, can increase BUFFER_RAM to 256 and BUFFER_COUNT to 3, if true lower to (AT most) 192
  #define TIMING_DEBUG         true
  #define SAMPLE_TIMING_DEBUG        true
  #define USE_SERIAL (DEBUG || TIMING_DEBUG || SAMPLE_TIMING_DEBUG)
#endif
//lower BUFFER_RAM to 96 while testing READ_FROM_DIRECTORY, raise to 196 when not testing 


audioBufferHandler buffer;
audioProcessor processor;
dataWriter writer;

bool collectData = false;

#if SAMPLE_TIMING_DEBUG
  long NTimer = 0;
  long NTimerPrevMicros = 0;
#endif

void Timer_A0_N(void);

short workingBuffer[BUFFER_SIZE/2];
short tSendVal = 16705;

void setup()
{
  //WDTCTL = WDTPW + WDTHOLD;
  
  
  prepareAll();
  delay(100);
  collectData = true;
  
  #if USE_SERIAL
    Serial.print("Size of double: ");Serial.println(sizeof(double));
    Serial.print("size of float: "); Serial.println(sizeof(float));
  #endif
  
  #if !USE_SERIAL & USE_LEDS
    blinkLED();
  #endif
  /*
  {
    char tempString[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    short tempStringLength = sizeof(tempString);
    long tShort;
    for(int i = 0; i < BUFFER_SIZE * BUFFER_COUNT/2; i++)
    {
      tShort =  (long)tempString[(i%(tempStringLength/2))*2] * 256;
      tShort += (long)tempString[(i%(tempStringLength/2))*2+1];
      buffer.addSample(tShort);
    }
    
  }
  */
  
  //This is a short bit to indicate
  //program has begun
}
void loop()
{
  #if DEBUG
    Serial.print("Collectdata = "); Serial.println(collectData);
    Serial.print("Num Ready Buffers: "); Serial.println(buffer.getNumReadyBuffers());
    Serial.println("Doing LOOP");
    delay(500);
  #endif
  //if a buffer is full, it is processed and sent to the data writer
  if(buffer.getNumReadyBuffers()) //if true, there should be a buffer ready to be processed / sent to SD card
  {
    #if DEBUG
      Serial.println("Handling buffer data");
    #endif
    
    //retrieve afull buffer from the buffers
    buffer.getFullBuffer(workingBuffer);
    
    
    //process the buffer, return the end size of the buffer (used when downsampling
    //you may need something more complex since processing may change type from short to float
    
    //note, process audio currently does nothing more than
    //downsample audio
    processor.processAudio(workingBuffer);
    
    
    writer.writeData(workingBuffer,BUFFER_SIZE/(2 * OVERSAMPLE_MULTIPLIER));
    //writer.writeData("Flippity FLoppity FLoop, my hand is covered in goop six times seven less nine times eleven if four score and not a bit more\r\nFlippity FLoppity FLoop, my hand is covered in goop six times seven less nine times eleven if four score and not a bit more\r\nFlippity FLoppity FLoop, my hand is covered in goop six times seven less nine times eleven if four score and not a bit more\r\n",375);
  }
  for(int i = 0; i < BUFFER_SIZE/4;i++)
  {
    //buffer.addSample(tSendVal);
  }
  tSendVal+= 256 + 1;
  
  //reading the pin like this should be quicker than calling digitalRead
  //alternate line (if (digitalRead(PUSH1) == LOW)
  if((LEFT_BUTTON_PORT_IN & LEFT_BUTTON) == 0)
  {
    #if DEBUG
      Serial.println("Shutting down");
    #endif
    writer.shutdown();
    collectData = false;
    
    //alternate implementation
    //digitalWrite(RED_LED,HIGH)
    RIGHT_RLED_PORT_OUT |= RIGHT_RLED;
    for(;;);
  }
  
  
  
  
  
  #if SAMPLE_TIMING_DEBUG
  //Use this to test timing stuff
    sampleRateTest();
  #endif
}
void prepareAll()
{
  #if USE_SERIAL
    Serial.begin(9600);
    delay(500);
    Serial.println("Begin Setup");
    delay(5);
    Serial.print("\n");
  #endif
  
  #if DEBUG
    //Serial.println("\tBegin Pins Setup");
  #endif
  preparePins();
    
  #if DEBUG
    Serial.println("\tBegin PowerModes Setup");
  #endif
  preparePowerModes();
  
  #if DEBUG
    Serial.println("\tBegin Buffers setup");
  #endif
  buffer.setupBuffers();
  
  #if DEBUG
    Serial.println("\tBegin processor setup");
  #endif
  processor.setupProcessor();
  
  #if DEBUG
    Serial.println("\tBegin writer setup");
  #endif
  writer.setupWriter();
  
  #if DEBUG
    Serial.println("\tBegin ADC Setup");
  #endif
  prepareADC();

  #if DEBUG
    Serial.println("Setup complete\n-------------------------\n");
  #endif
}
void preparePins()
{
  //Sets the red LED as an output pin
  #if USE_LEDS
    P1OUT &= 0x00;     //Shuts everything off
    P1DIR &= 0x00;     //Sets all pins to input
    P2OUT &= 0x00;     //Shuts everything off
    P2DIR &= 0x00;     //Sets all pins to input
    
    //LEFT_BUTTON_PORT_DIR &= !LEFT_BUTTON; //LEFT_BUTTON is already an input
    LEFT_BUTTON_PORT_REN |= LEFT_BUTTON;
    pinMode(PUSH1, INPUT_PULLUP); //for some reason I need to do this using energia, manually not working
  
    LEFT_RLED_PORT_DIR |= LEFT_RLED;                      // Configure P1.0 as output (LEFT RED_LED
    RIGHT_RLED_PORT_DIR |= RIGHT_RLED;
    
    //blinkLED();
  #endif
  
  

    
  
  
  //pinMode(RED_LED,OUTPUT);
}
void preparePowerModes()
{
  //Sets to low power mode and may lower SMCLK
  
  
  //sets to power mode AM_LDO_VCORE0
  
  PCMCTL0 = CPM_0;
  #if 0
  
  //NOTE: CHANGING THE SMCLK MESSES UP SERIAL.PRINT, unless this can be fixed, SMCLK should not be changed
  
  	  //This is the setup for the SMCLK, which is used by TIMER_A0_N which is used by ADC
  	  //Eventually should figure out clocks a bit better to determine frequency
  	  //TIMER_A0_N which is used by the ADC
    CSKEY = 0x695A; // unlock CS registers
  	  //Messing with clock periods
    CSCTL0 = DCORSEL_3;
  
  	  //sets SMCLK and HSMCLK to DCOCLK reference
    CSCTL1 |= SELS_3;
  	  //sets SMCLK divider to 1/2
  	  //CSCTL1 |= DIVS_4;
  
    CSKEY = 0; // lock CS registers
    
  #endif
}
void prepareADC()
{
  
  //THIS SEGMENT SETS UP THE TIMER
  //------------------------------------------------------------------------------
  //ID a clock divider ID_x -> 2^x division
  //TASSEL_2 has to do with the timer source
  TA0CTL = TASSEL_2 + ID_2 + MC_1;
  //need to have a way to programmatically solve this
  
  //the timer counts up to the value set in this register,
  
  #if USE_SERIAL
    TA0CCR0 = (short)((long)3064000/ADC_SAMPLE_FREQUENCY);
  #else
    TA0CCR0 = (short)(3064000/ADC_SAMPLE_FREQUENCY);
  #endif
  TA0CCTL0 = CM_1 + SCS ;
  
  TA0CTL |= TAIE;
  
  Interrupt_registerInterrupt(INT_TA0_N,Timer_A0_N);
  Interrupt_enableInterrupt(INT_TA0_N);
  Interrupt_enableMaster();
  //-------------------------------------------------------------------------------
  
  
  
}
void blinkLED()
{
    RIGHT_RLED_PORT_OUT |= RIGHT_RLED;
    delay(1000);
    RIGHT_RLED_PORT_OUT &= !RIGHT_RLED;
    delay(1000);
    /*
    alternate implementation
    digitalWrite(RED_LED,HIGH);
    delay(1000);
    digitalWrite(RED_LED,LOW);
    delay(1000);
    */
}


/* 
END SETUP FUNCTIONS

-------------------------------------------------------------------------------------------

BEGIN LOOP FUNCTIONS
*/




#if SAMPLE_TIMING_DEBUG
void sampleRateTest()
{
    long curTime = micros();
    if(curTime - NTimerPrevMicros > 1 * 1000000)
    {
        Serial.print("Expected: ");
        Serial.println((curTime - NTimerPrevMicros) * (float)ADC_SAMPLE_FREQUENCY / 1000000.);
        Serial.print("Got: ");
        Serial.println(NTimer);

      //writer.writeData("Expected: ",10);
      //writer.writeData((short)((curTime - NTimerPrevMicros) * (float)ADC_SAMPLE_FREQUENCY / 1000000.),1);
      //writer.writeData("Got: ",5);
      //writer.writeData((short)Ntimer,1);
     // Serial.println("enterLoop");
       NTimer = 0;
       NTimerPrevMicros = micros();
    }
}


#endif
/* 
END LOOP FUNCTIONS

-------------------------------------------------------------------------------------------

BEGIN INTERRUPT FUNCTIONS
*/
void Timer_A0_N(void)
{
  if(collectData)
  {
    short tVal = getADCValue();
    buffer.addSample(tVal);  
  }
  TA0CTL &= ~TAIFG;                //resets flag
  
  
  
  #if SAMPLE_TIMING_DEBUG
    NTimer++;
  #endif
  #if USE_LEDS
    //I use this method of swtching the LED because it should
    //be faster than using digitalWrite
    //definitly want the interrupt to be as quick as possible
    //alternate iplementation (digitalWrite(LED1,!digitalRead(LED1)))
    //LEFT_RLED_PORT_OUT ^= LEFT_RLED;                  // Toggle P1.0 (Left RED_LED
  #endif
}
short getADCValue()
{
  //TODO: Implement ADCcapture, would rather not use ENERGIA CODE
  return 16706; //should convert to AB
}
