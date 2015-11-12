#include "msp432p401r.h"
//#include "driverlib/interrupt.h"
#include "config.h"
#include "sdWritingHandler.h"
#include "audioBufferHandler.h"
#include "msp432_startup_ccs.c"

#define DEBUG          true   //if it is false, can increase BUFFER_RAM to 256 and BUFFER_COUNT to 3, if true lower to (AT most) 192
#define WRITE_DEBUG    false
#define TIMING_DEBUG   false
#define INCLUDE_HEADER true
#define READ_FROM_DIRECTORY false  //seems to cause RAM issues when true, keep false for most testing
//lower BUFFER_RAM to 96 while testing READ_FROM_DIRECTORY, raise to 196 when not testing 

audioBufferHandler buffers;




volatile long NTimer = 0;
long prevMicros = 0;
void setup()
{
  #if DEBUG
    Serial.begin(9600);
    delay(500);
    Serial.println("Begin Setup");
    delay(5);
  #endif
  
  #if DEBUG
    Serial.println("\tBegin Pins Setup");
  #endif
  preparePins();
  
  #if DEBUG
    Serial.println("\tBegin PowerModes Setup");
  #endif
  preparePowerModes();

  #if DEBUG
    Serial.println("\tBegin ADCTimer Setup");
  #endif
  prepareADCTimer();

  #if DEBUG
    Serial.println("\tBegin Interrupts Setup");
  #endif
  prepareInterrupts();


  #if DEBUG
    Serial.println("Setup complete\n-------------------------\n");
  #endif
}
void preparePins()
{
  //Sets the red LED as an output pin
  P1DIR &= 0;

	// The following code toggles P1.0 port
  P1DIR |= BIT0;                      // Configure P1.0 as output
  P1OUT |= BIT0;                  // Toggle P1.0
  
  
  
  //pinMode(RED_LED,OUTPUT);
}
void preparePowerModes()
{
  //sets to power mode AM_LDO_VCORE0
  PCMCTL0 = CPM_0;
  #if !DEBUG
  //NOTE: CHANGING THE SMCLK MESSES UP SERIAL.PRINT
  
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
void prepareADCTimer()
{
  TA0CTL = TASSEL_2 + ID_3 + MC_1;
  TA0CCR0 = 1500000 / ADC_SAMPLE_FREQUENCY;
  TA0CCTL0 = CM_1 + SCS ;
  
  TA0CTL |= TAIE;
  
  
}
void prepareInterrupts()
{
  //prepareADCTimer();
  #if DEBUG
   Serial.print("Gonna enableThingy");
   delay(5);
  #endif
  //NVIC_EnableIRQ(TA0_N_IRQn);
  #if DEBUG
    Serial.print("Enabled Thingy");
    delay(5);
  #endif
  //Interrupt_registerInterrupt(INT_TA0_0,Timer_A0_0);
  //Interrupt_enableMaster();
  //Interrupt_enableInterrupt(INT_TA0_0);
}


/* 
END SETUP FUNCTIONS

-------------------------------------------------------------------------------------------

BEGIN LOOP FUNCTIONS
*/





void loop()
{
  //buffers.addSample(tVal);
  #if DEBUG
  if(micros() - prevMicros > 10000000)
  {
    Serial.println(NTimer);
   // Serial.println("enterLoop");
     prevMicros = micros();
  }
  #endif
}


/* 
END LOOP FUNCTIONS

-------------------------------------------------------------------------------------------

BEGIN INTERRUPT FUNCTIONS
*/

/*
void timerAInterrupt()
{
  #if DEBUG
    Serial.println("timerAInterrupt");
  #endif
  digitalWrite(RED_LED,!digitalRead(RED_LED));
  
  tVal++;
}
*/
void Timer_A0_N(void)
{
	P1OUT ^= BIT0;                  // Toggle P1.0

	TA0CTL &= ~TAIFG;
	NTimer++;
}

