#include "msp430g2553.h"
#include <pfatfsEdited.h>
#include <pffconfEdited.h>

#include "SPI.h" 
#include "pfatfsEdited.h"

/* Low frequency audio collection and storage
 program, makes extensive use of the SD Card Library by XXXXXXX 
 
 currently ADC is on pin 1, tweak in prepareADC()
 
 
 */
//debug values
#define DEBUG          false    //if it is false, can increase BUFFER_RAM to 256 and BUFFER_COUNT to 3
#define WRITE_DEBUG    false
#define TIMING_DEBUG   false
#define INCLUDE_HEADER true

//Assigned Pins
#define CS_PIN       8      //P1.6?      //MISO     also used by green LED
#define BUTTON_PIN   5      //P1.3 
#define RED_LED_PIN  2      //P1.0       //currently unused by code
// Slave transit enable 6   //P1.4
// SCLK              7      //P1.5
// Master out slave in MOSI //P1.7


//currently assumes sample size is 2 bytes
#define BUFFER_RAM 256 //MUST be divisible by (Size of each sample) * (BUFFER_COUNT+1) * BUFFER_SIZE
#define BUFFER_COUNT 3
#define BUFFER_SIZE BUFFER_RAM/(BUFFER_COUNT+1)


#define ADC_SAMPLE_FREQUENCY 1 //Measured in KHz

#define MAX_FILE_NAME_SIZE 13
#define NUM_SECTOR_WRITES 20 //This will have an affect on how frequentyly writes are finalized
//may be a data loss concern


//BEGIN ADC read interrupt values  
volatile char writeBuffer[BUFFER_COUNT][BUFFER_SIZE];
volatile char bufferPosition = 0;
volatile char bufferNumber = 0;
volatile boolean writeDataAvailable = false;
volatile char dataReadyBuffer = 0;

//begin SD card writing values
long SDwritePosition = 0; //increment by 512 to find desired sector
char SDwriteBuffer[BUFFER_SIZE];

//may eventually change this to a String and adapt code
char currentFileName[MAX_FILE_NAME_SIZE] = "LOG00000.TXT"; //eventually should be set by reading a config file
//or reading contents of SD card
char currentFile = 0;
boolean isFileOpen = false;
String fileName;
long currentWriteLength = 0;
unsigned short int bw, br;//, i;
long AccStringLength = 0;
long fileSize = 0;

boolean collectData = false;

#if TIMING_DEBUG
long TTIME = 0;
#endif


#if LIMITED_WRITES
short tWriteCount = 0;
#define SECONDS_TO_SAVE
#endif


//----BEGIN SETUP FUNCTIONSS-------------------------------
void setup()
{
  //Serial.begin(9600);
#if DEBUG + WRITE_DEBUG + TIMING_DEBUG + LIMITED_WRITES
  Serial.begin(9600);
#endif

#if DEBUG
  Serial.println("\n\nBEGIN SETUP");
#endif

  for(char i = 0; i < BUFFER_COUNT; i++)
  {
    emptyBuffer(writeBuffer,i);
  }
  FatFs.begin(CS_PIN);

  setupPins();
  prepareADC();
  prepareADCTimer();
  waitForStart();
  disk_initialize();

  __enable_interrupt();


  #if INCLUDE_HEADER
    generateHeader();
  #endif




  collectData = true;
  #if DEBUG
    Serial.println("END SETUP");
  #endif


  
}
void generateHeader()
{
  //Consider 16 byte segments
  /* FORMAT:
    HEADER_SIZE= 2 bytes
    BUFFER_SIZE= 2 bytes
  
  
  
  
  
  
  */
  int HEADER_SIZE = 512;
  
  for(int i = 0; i < BUFFER_SIZE;i++)
  {
    SDwriteBuffer[i] = '0';
  }
  
  
  //there is almost certainly a better way to do this, probably with strings
  SDwriteBuffer[0]  = 'H';
  SDwriteBuffer[1]  = 'E';
  SDwriteBuffer[2]  = 'A';
  SDwriteBuffer[3]  = 'D';
  SDwriteBuffer[4]  = 'E';
  SDwriteBuffer[5]  = 'R';
  SDwriteBuffer[6]  = '_';
  SDwriteBuffer[7]  = 'S';
  SDwriteBuffer[8]  = 'I';
  SDwriteBuffer[9]  = 'Z';
  SDwriteBuffer[10] = 'E';
  SDwriteBuffer[11] = '=';
  SDwriteBuffer[12] = (HEADER_SIZE>>8) & 0xff;  //next 2 bytes are buffer Size
  SDwriteBuffer[13] = (HEADER_SIZE>>0) & 0xff;
  SDwriteBuffer[14] = '\r';
  SDwriteBuffer[15] = '\n';
  
  
  SDwriteBuffer[16] = 'B';
  SDwriteBuffer[17] = 'U';
  SDwriteBuffer[18] = 'F';
  SDwriteBuffer[19] = 'F';
  SDwriteBuffer[20] = 'E';
  SDwriteBuffer[21] = 'R';
  SDwriteBuffer[22] = '_';
  SDwriteBuffer[23] = 'S';
  SDwriteBuffer[24] = 'I';
  SDwriteBuffer[25] = 'Z';
  SDwriteBuffer[26] = 'E';
  SDwriteBuffer[27] = '=';
  SDwriteBuffer[28] = (BUFFER_SIZE>>8) & 0xff;  //next 2 bytes are buffer Size
  SDwriteBuffer[29] = (BUFFER_SIZE>>0) & 0xff;
  SDwriteBuffer[30] = '\r';
  SDwriteBuffer[31] = '\n';
  
  
  
  
  SDwriteBuffer[63] = '\n';
  
  
  
  
  
  
  
  
  
  
  //writes
  char FatFsReturnChar = FatFs.open(currentFileName);
  if (FatFsReturnChar) recover(FatFsReturnChar);
  isFileOpen = true;
  bw = 0;
  FatFsReturnChar = FatFs.lseek(SDwritePosition);
  if (FatFsReturnChar) recover(FatFsReturnChar+10);
  
  FatFsReturnChar = FatFs.write(SDwriteBuffer, BUFFER_SIZE,&bw);
  if (FatFsReturnChar) recover(FatFsReturnChar+20);
  currentWriteLength += BUFFER_SIZE;
    
    //quick ceiling calculation
    SDwritePosition = (currentWriteLength +511) / 512;
    SDwritePosition *= 512;
    currentWriteLength = SDwritePosition;
    
    
    
    //recover(0);
}
void prepareADC()
{
#if DEBUG
  Serial.println("BEGIN prepareADC");
#endif

  //Vcc,Gnd References, 8x sample and hold time, REFBURST, 
  ADC10CTL0 = SREF_0 + ADC10SHT_1 + REFBURST + ADC10ON + ADC10IE;

  //input is pin P1.1, clock divider is 4
  ADC10CTL1 = INCH_1 + ADC10DIV_3;

  //enablesP1.1
  ADC10AE0 = BIT1;
}
void prepareADCTimer()
{
#if DEBUG
  Serial.println("BEGIN prepareADCTimer");
#endif
  delay(100);

  BCSCTL1 = CALBC1_16MHZ; // Set DCO Clock to 16MHz
  DCOCTL = CALDCO_16MHZ;

  TACCTL0 = CCIE;                             // CCR0 interrupt enabled
  TACTL = TASSEL_2 + MC_1 + ID_3;           // SMCLK/8, upmode
  TACCR0 =  2000. / ADC_SAMPLE_FREQUENCY;                          // 16MHz / 8 / 1000 = 2KHz

}

void setupPins()
{
#if DEBUG
  Serial.println("BEGIN setupPins");
#endif
  P1OUT &= 0x00;     //Shuts everything off
  P1DIR &= 0x00;     //Sets all pins to input

  P1REN |= BIT3;     // Enable internal pull-up/down resistors
  P1OUT |= BIT3;     //Select pull-up mode for P1.3 

    P1DIR |= BIT0; //Sets pin 1.0 to output       // Enable internal pull-up/down resistors
}
void waitForStart()
{
#if DEBUG
  Serial.println("BEGIN waitForStart");
  Serial.println("Waiting for button = 1");
#endif


  while(digitalRead(BUTTON_PIN)==1){
  }
  delay(200);


#if DEBUG
  Serial.println("Waiting for button = 0");
#endif


  while(digitalRead(BUTTON_PIN)==0){
  }
  delay(200);
}
//-------END SETUP FUNCTIONS-------------------------------------


//-------BEGIN LOOP FUNCTIONS------------------------------------
void loop()
{
  if(digitalRead(BUTTON_PIN) == LOW)
  {
    buttonShutoffSequence();
  }
  
  #if DEBUG
    Serial.println("Enter Loop");
  #endif

  if(writeDataAvailable)
  {
    #if !WRITE_DEBUG
      dataWriteSequence();
    #endif
  
    #if WRITE_DEBUG
      testDataWriteSequence();
    #endif
  
    #if LIMITED_WRITES
      limitedWritesCheck();
    #endif
    
    checkFile();   //not implemented, will handle switching files when appropriate
    
    
    
  }

  #if DEBUG
    Serial.println("Exit Loop");
  #endif

}
#if !WRITE_DEBUG
void checkFile()
{
  //TODO: Check for EOF, switch to next file if it is
}
void dataWriteSequence()
{
#if DEBUG
  Serial.println("Enter Data Available");
#endif
  bw = 0;
  char FatFsReturnChar = 0;
  if(!isFileOpen)
  {
#if DEBUG
    Serial.println("Enter isFileOpen");
    delay(5);
#endif


    FatFsReturnChar = FatFs.open(currentFileName);
    if (FatFsReturnChar) recover(FatFsReturnChar);
    isFileOpen = true;
   // bw = 0;
    FatFsReturnChar = FatFs.lseek(SDwritePosition);
    if (FatFsReturnChar) recover(FatFsReturnChar+10);
  }


  memcpy(&SDwriteBuffer, (const char*)&writeBuffer[bufferNumber], BUFFER_SIZE);
  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    //SDwriteBuffer[i] = i%10 + 48;//prints digit characters
  }
  
  
  //should help ensure false data not added, may cause issue with volatile data
  //larger RAM / buffers should solve this problem
  
  //emptyBuffer(writeBuffer,bufferNumber);

#if TIMING_DEBUG
  long time = micros()-TTIME;
  TTIME = micros();
  Serial.println(time);
  delay(5);
#endif

#if DEBUG
  Serial.println("Executing write");
  delay(5);
#endif


  FatFsReturnChar = FatFs.write(SDwriteBuffer, BUFFER_SIZE,&bw);
  if (FatFsReturnChar) recover(FatFsReturnChar+20);
  writeDataAvailable = false;
  currentWriteLength += BUFFER_SIZE;


  if(currentWriteLength+BUFFER_SIZE > 512 * NUM_SECTOR_WRITES)
  {
  #if DEBUG
    Serial.println("Close File");
    delay(5);
  #endif
    SDwritePosition =  SDwritePosition + 512*NUM_SECTOR_WRITES;
    currentWriteLength = 0;
    FatFsReturnChar = FatFs.write(0, 0, &bw);  //Finalize write
    if (FatFsReturnChar) recover(FatFsReturnChar+30);
    FatFsReturnChar = FatFs.close();  //Close file
    if (FatFsReturnChar) recover(FatFsReturnChar+40);
    isFileOpen = false;
  }
}
#endif

void buttonShutoffSequence()
{
#if DEBUG
  Serial.println("button high, closing file");
#endif
  char FatFsReturnChar = FatFs.close();  //Close file
  if (FatFsReturnChar) recover(FatFsReturnChar);
  recover(0);
}
void recover(byte errorVal)
{
  FatFs.close();
  collectData = false;
  if(errorVal == 6)
  {
    char errorCount = 0;
    /*
    while(errorVal == 6 && errorCount < 10)
     {
     FatFs.close();
     delay(20);
     FatFs.begin(CS_PIN);
     delay(20);
     errorVal = FatFs.open(currentFileName);
     
     //escape when file set up
     if(errorVal == 0)
     {
     collectData = true;
     return;
     }
     errorCount++;
     #if DEBUG
     Serial.println("Caught in errorVAl loop");
     #endif
     delay(1000);
     }*/
  }
  //FatFs.close();  //Close file

  collectData = false;
  //TODO:
  //eventually this should attempt to restart program
  //may also handle closing files and such
  if(errorVal)
  {
#if DEBUG
    Serial.print("\n\nError Detected, code: ");
    Serial.print(errorVal);
#endif
    for(;;)
    {
      //unrecovered error loop
      P1OUT ^= BIT0;                      // Toggle P1.6
      delay(500);
    }
  }


  else //error Value == 0
  {
    P1OUT |= BIT0;                      // Toggle P1.6
#if DEBUG
    Serial.print("\n\nCode succesfully exited: ");
    Serial.print(errorVal);
#endif

    for(;;)
    {
      //sucessful exit loop
      P1OUT |= BIT0;                      // Toggle P1.6
      delay(2000);
    }
  }
}



void emptyBuffer(volatile char buffer[][BUFFER_SIZE],char column)
{
  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    buffer[column][i] = 0;
  }
}


#if TIMING_DEBUG
void testDataWriteSequence()
{
#if DEBUG
  Serial.println("Enter Test write");
  delay(5);
#endif

  char buf[] = "Best";
  char rc;
  rc = FatFs.open("TESTLOG.TXT");
  if (rc) recover(rc);
  rc = FatFs.lseek(  AccStringLength );
  if (rc) recover(rc);
  AccStringLength =  AccStringLength + 512;
  int StringLength =  strlen(buf);
  rc = FatFs.write(buf, StringLength,&bw);
  if (rc) recover(rc);
  rc = FatFs.write(0, 0, &bw);  //Finalize write
  if (rc) recover(rc);
  rc = FatFs.close();  //Close file
  if (rc) recover(rc);
}
#endif


#if LIMITED_WRITES
void limitedWritesCheck()
{
  if(++tWriteCount >= SECONDS_TO_SAVE * ADC_SAMPLE_FREQUENCY * 1000 / (BUFFER_SIZE /2)) // X seconds of sample
  {
    char FatFsReturnChar = FatFs.close();  //Close file
    if (FatFsReturnChar) recover(FatFsReturnChar);
    recover(0);
  }
}
#endif
//----------END LOOP FUNCTIONS----------------------------------


//--------BEGIN INTERRUPT FUNCTIONS-----------------------------

//when complete this will be the ADC interrupt
#pragma vector=TIMER0_A0_VECTOR 
__interrupt void Timer_A (void) 
{   
  if(collectData)
  {
    //P1OUT |= BIT6;                          // Turn on Red LED


    /* taken from http://www.embeddedrelated.com/showarticle/199.php */
    ADC10CTL0 |= ENC + ADC10SC; // Sampling and conversion start

    __bis_SR_register(CPUOFF + GIE); // LPM0 with interrupts enabled

    short input = ADC10MEM;
    /* end taken*/

    if(bufferPosition +2 > BUFFER_SIZE)      //bufferPosition + 1 > BUFFER_SIZE - 1, this would put a value out of range
    {
      writeDataAvailable = true;
      dataReadyBuffer = bufferNumber;



      bufferPosition = 0;
      bufferNumber += 1;
      if(bufferNumber >= BUFFER_COUNT)
      {
        bufferNumber = 0;
      }
    }
    writeBuffer[bufferNumber][bufferPosition+1] = input & 0xff;
    writeBuffer[bufferNumber][bufferPosition] = (input>>8) & 0xff;

    bufferPosition += 2;
  }

}


// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

