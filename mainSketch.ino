#include "msp430g2553.h"
#include <pfatfsEdited.h>
#include <pffconfEdited.h>

#include "SPI.h" 
#include "pfatfsEdited.h"

/* Low frequency audio collection and storage
 program, makes extensive use of the SD Card Library by XXXXXXX 
 
 currently ADC is on pin 1, tweak in prepareADC()
 
 7-7-15 light codes
 
 green ->             writting to SD
 solid red ->         program succesfully shut down
 1Hz blinking red ->  unrecovered error
 */
//debug values
#define DEBUG          true    //if it is false, can increase BUFFER_RAM to 256 and BUFFER_COUNT to 3, if true lower to (AT most) 192
#define WRITE_DEBUG    false
#define TIMING_DEBUG   false
#define INCLUDE_HEADER true
#define READ_FROM_DIRECTORY true  //seems to cause RAM issues when true, keep false for most testing
//lower BUFFER_RAM to 96 while testing READ_FROM_DIRECTORY, raise to 196 when not testing 

#define ACCESS_SD_CARD true //Just in case want to test things without SD card

//Assigned Pins
#define CS_PIN       8      //P1.6?      //MISO     also used by green LED
#define BUTTON_PIN   5      //P1.3 
#define RED_LED_PIN  2      //P1.0       //currently unused by code
// Slave transit enable 6   //P1.4
// SCLK              7      //P1.5
// Master out slave in MOSI //P1.7


//currently assumes sample size is 2 bytes
#define BUFFER_RAM 96 //MUST be divisible by (Size of each sample) * (BUFFER_COUNT+1) * BUFFER_SIZE
#define BUFFER_COUNT 2
#define BUFFER_SIZE BUFFER_RAM/(BUFFER_COUNT+1)


#define ADC_SAMPLE_FREQUENCY 500 //Measured in Hz

#define NUM_SECTOR_WRITES 30 //This will have an affect on how frequentyly writes are finalized
//may be a data loss concern

#define LOG_FILES_LOCATION "LOGFILES"


//BEGIN ADC read interrupt values  
volatile char writeBuffer[BUFFER_COUNT][BUFFER_SIZE];
volatile char bufferPosition = 0;
volatile char bufferNumber = 0;
volatile boolean writeDataAvailable = false;
volatile char dataReadyBuffer = 0;

//begin SD card writing values
long SDwritePosition = 0; //increment by 512 to find desired sector
char SDwriteBuffer[BUFFER_SIZE];
char readyBuffers[BUFFER_COUNT] = {0};

//may eventually change this to a String and adapt code
//or reading contents of SD card
char currentFile = 0;
boolean isFileOpen = false;
String fileName;
long currentWriteLength = 0;
unsigned short int bw, br;//, i;
long AccStringLength = 0;
long fileSize = 0;

#if READ_FROM_DIRECTORY
  #define FILE_PATH_SIZE 22
  char currentFilePath[FILE_PATH_SIZE];
  DIR directory;
#else
  char currentFilePath[13] = "LOG00000.TXT";
#endif
boolean collectData = false;

#if TIMING_DEBUG
long TTIME = 0;
#endif


#if LIMITED_WRITES
short tWriteCount = 0;
#define SECONDS_TO_SAVE 1
#endif


//----BEGIN SETUP FUNCTIONSS-------------------------------
void setup()
{
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



  setupPins();
  prepareADC();
  prepareADCTimer();
  
  
#if ACCESS_SD_CARD
  FatFs.begin(CS_PIN);

  #if READ_FROM_DIRECTORY
    getNextFile();
  #endif
  
  #if INCLUDE_HEADER
    generateHeader();
  #endif
#endif
  
  waitForStart();

  __enable_interrupt();






  collectData = true; //will likely move this
#if DEBUG
  Serial.println("END SETUP");
#endif



}
#if READ_FROM_DIRECTORY

void getNextFile()
{
  //selects directory
  //directory = 0;
#if ACCESS_SD_CARD
  
  Serial.println("Open Directory");
  char rc = FatFs.opendir(&directory,LOG_FILES_LOCATION);
  if(rc) recover(rc+80);
  FILINFO file;

  rc = FatFs.readdir(&directory,&file);
  if(rc) recover(rc+90);
  
  sprintf(currentFilePath,"%s/%s",LOG_FILES_LOCATION,file.fname);
  Serial.println(currentFilePath);
  
  for(int i = 0; i < 7; i++)
  {
    rc = FatFs.readdir(&directory,&file);
    if(rc) recover(rc+90);
    
    sprintf(currentFilePath,"%s/%s",LOG_FILES_LOCATION,file.fname);
    Serial.println(currentFilePath);
  }
#endif
  

  //TODO: actually use the file.fname to rename currentFilePath
  //copies file name to current File Name

  //will later use this to change file name
  /* JACOB TODO:
   I am having trouble with the next bit of code
   It should essentially be the same outside of the sketch, so you may
   want to create your own sketch to test stuff
   
   what I need: 
   1. I need to take a character arrayA of length 9
   2. append a '/' character
   3. append a character arrayB of length 13 to it
   
   ex:
   arrayA = LOGFILES
   arrayB = LOG00000.TXT
   output LOGFILES/LOG00000.TXT
   
   //strcpy(currentFilePath, LOG_FILES_LOCATION);
   //char forwardSlash[1] = {'/'};
   //strcat(currentFilePath,(const char*)'////');
   //strcat(currentFilePath,file.fname);
   
   
   //memcpy(currentFilePath,tString,strlen(tString));
   
   
   
   
   
   
   */

  SDwritePosition = 0;
  currentWriteLength = 0;



#if DEBUG
  Serial.println("Opening new file: ");
  Serial.println(currentFilePath);
#endif
}
#endif  //READ_FROM_DIRECTORY
void generateHeader()
{
  //Consider 16 byte segments
  /* FORMAT:
   HEADER_SIZE= 2 bytes
   BUFFER_SIZE= 2 bytes
   
   
   
   
   
   
   */

#if DEBUG
  Serial.println("Entering generate header");
#endif



  int HEADER_SIZE = 512;

#if ACCESS_SD_CARD
  char FatFsReturnChar = FatFs.open(currentFilePath);
  if (FatFsReturnChar) recover(FatFsReturnChar+100);
  isFileOpen = true;
  bw = 0;
  FatFsReturnChar = FatFs.lseek(SDwritePosition);
  if (FatFsReturnChar) recover(FatFsReturnChar+10);
#endif



  int i = 0;

  if(i+16 > BUFFER_SIZE)
  {
    for(i = 0; i < BUFFER_SIZE;i++)
    {
      SDwriteBuffer[i] = '0';
    }
  }

  //there is almost certainly a better way to do this, probably with strings
  SDwriteBuffer[i+0]  = 'B';
  SDwriteBuffer[i+1]  = 'U';
  SDwriteBuffer[i+2]  = 'Y';
  SDwriteBuffer[i+3]  = 'T';
  SDwriteBuffer[i+4]  = 'S';
  SDwriteBuffer[i+5]  = 'R';
  SDwriteBuffer[i+6]  = '_';
  SDwriteBuffer[i+7]  = 'S';
  SDwriteBuffer[i+8]  = 'I';
  SDwriteBuffer[i+9]  = 'Z';
  SDwriteBuffer[i+10] = 'E';
  SDwriteBuffer[i+11] = '=';
  SDwriteBuffer[i+12] = (HEADER_SIZE>>8) & 0xff;  //next 2 bytes are buffer Size
  SDwriteBuffer[i+13] = (HEADER_SIZE>>0) & 0xff;
  SDwriteBuffer[i+14] = '\r';
  SDwriteBuffer[i+15] = '\n';

  i+=16;
  if(i+16 > BUFFER_SIZE)
  {
#if ACCESS_SD_CARD
    writeSingleBuffer(SDwriteBuffer, bw);
    currentWriteLength+= BUFFER_SIZE;
#endif
    for(i = 0; i < BUFFER_SIZE;i++)
    {
      SDwriteBuffer[i] = '0';
    }
    i = 0;
  }

  SDwriteBuffer[i+0] = 'B';
  SDwriteBuffer[i+1] = 'U';
  SDwriteBuffer[i+2] = 'F';
  SDwriteBuffer[i+3] = 'F';
  SDwriteBuffer[i+4] = 'E';
  SDwriteBuffer[i+5] = 'R';
  SDwriteBuffer[i+6] = '_';
  SDwriteBuffer[i+7] = 'S';
  SDwriteBuffer[i+8] = 'I';
  SDwriteBuffer[i+9] = 'Z';
  SDwriteBuffer[i+10] = 'E';
  SDwriteBuffer[i+11] = '=';
  SDwriteBuffer[i+12] = (BUFFER_SIZE>>8) & 0xff;  //next 2 bytes are buffer Size
  SDwriteBuffer[i+13] = (BUFFER_SIZE>>0) & 0xff;
  SDwriteBuffer[i+14] = '\r';
  SDwriteBuffer[i+15] = '\n';




  i+=16;
  if(i+16 > BUFFER_SIZE)
  {
#if ACCESS_SD_CARD
    writeSingleBuffer(SDwriteBuffer, bw);
    currentWriteLength+= BUFFER_SIZE;
#endif
    for(i = 0; i < BUFFER_SIZE;i++)
    {
      SDwriteBuffer[i] = '0';
    }
    i = 0;
  }

  SDwriteBuffer[i+0] = 'F';
  SDwriteBuffer[i+1] = 'I';
  SDwriteBuffer[i+2] = 'L';
  SDwriteBuffer[i+3] = 'E';
  SDwriteBuffer[i+4] = '_';
  SDwriteBuffer[i+5] = 'U';
  SDwriteBuffer[i+6] = 'S';
  SDwriteBuffer[i+7] = 'E';
  SDwriteBuffer[i+8] = 'D';
  SDwriteBuffer[i+9] = ' ';
  SDwriteBuffer[i+10] = ' ';
  SDwriteBuffer[i+11] = '=';
  SDwriteBuffer[i+12] = 'T';  //next 2 bytes are buffer Size
  SDwriteBuffer[i+13] = 'T';
  SDwriteBuffer[i+14] = '\r';
  SDwriteBuffer[i+15] = '\n';


  i+=16;
  if(i+16 > BUFFER_SIZE)
  {
#if ACCESS_SD_CARD
    writeSingleBuffer(SDwriteBuffer, bw);
    currentWriteLength+= BUFFER_SIZE;
#endif
    for(i = 0; i < BUFFER_SIZE;i++)
    {
      SDwriteBuffer[i] = '0';
    }
    i = 0;
  }

  SDwriteBuffer[i+0] = 'A';
  SDwriteBuffer[i+1] = 'D';
  SDwriteBuffer[i+2] = 'C';
  SDwriteBuffer[i+3] = '_';
  SDwriteBuffer[i+4] = 'R';
  SDwriteBuffer[i+5] = 'A';
  SDwriteBuffer[i+6] = 'T';
  SDwriteBuffer[i+7] = 'E';
  SDwriteBuffer[i+8] = ' ';
  SDwriteBuffer[i+9] = ' ';
  SDwriteBuffer[i+10] = ' ';
  SDwriteBuffer[i+11] = '=';
  SDwriteBuffer[i+12] = (ADC_SAMPLE_FREQUENCY>>8) & 0xff;
  ;  //next 2 bytes are buffer Size
  SDwriteBuffer[i+13] = (ADC_SAMPLE_FREQUENCY) & 0xff;
  ;
  SDwriteBuffer[i+14] = '\r';
  SDwriteBuffer[i+15] = '\n';




#if ACCESS_SD_CARD
  writeSingleBuffer(SDwriteBuffer, bw);
#endif

  //writes

  //this should place
  SDwritePosition = HEADER_SIZE;
  currentWriteLength = 0;

#if ACCESS_SD_CARD
  closeFile(bw);
#endif


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
  TACCR0 =  2000000./ADC_SAMPLE_FREQUENCY;                          // 16MHz / 8 / X = YKHz 

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




  }

#if DEBUG
  Serial.println("Exit Loop");
#endif

}
#if ACCESS_SD_CARD
void writeSingleBuffer(char *buffer, short unsigned int bw)
{
  char FatFsReturnChar = FatFs.write(buffer, BUFFER_SIZE,&bw);
  if (FatFsReturnChar) recover(FatFsReturnChar+20);
  currentWriteLength += BUFFER_SIZE;
}
#endif //ACCESS_SD_CARD

#if ACCESS_SD_CARD
void closeFile(short unsigned int bw)
{
  char FatFsReturnChar = FatFs.write(0, 0, &bw);  //Finalize write
  if (FatFsReturnChar) recover(FatFsReturnChar);
  FatFsReturnChar = FatFs.close();  //Close file
  if (FatFsReturnChar) recover(FatFsReturnChar);
  isFileOpen = false;
}
#endif //ACCESS_SD_CARD

#if !WRITE_DEBUG  //THis is the usual write program
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

#if ACCESS_SD_CARD
    FatFsReturnChar = FatFs.open(currentFilePath);
    if (FatFsReturnChar) recover(FatFsReturnChar);
    isFileOpen = true;
    // bw = 0;
    FatFsReturnChar = FatFs.lseek(SDwritePosition);
    if (FatFsReturnChar) recover(FatFsReturnChar+10);
  }
#endif


  memcpy(&SDwriteBuffer, (const char*)&writeBuffer[bufferNumber], BUFFER_SIZE);
  for(int i = 0; i < BUFFER_SIZE; i++)
  {
    SDwriteBuffer[i] = 'R';
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

#if ACCESS_SD_CARD   //consider changing to singleBufferWrite

  writeSingleBuffer(SDwriteBuffer,bw);
  /*
  FatFsReturnChar = FatFs.write(SDwriteBuffer, BUFFER_SIZE,&bw);
   if (FatFsReturnChar) recover(FatFsReturnChar+20);
   writeDataAvailable = false;
   currentWriteLength += BUFFER_SIZE;*/
#endif


  if(currentWriteLength+BUFFER_SIZE > 512 * NUM_SECTOR_WRITES)
  {
#if DEBUG
    Serial.println("Close File");
    delay(5);
#endif
    SDwritePosition =  SDwritePosition + 512*NUM_SECTOR_WRITES;
    currentWriteLength = 0;



#if ACCESS_SD_CARD
    closeFile(bw);
#endif
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
#if ACCESS_SD_CARD
  FatFs.close();
  collectData = false;
  if(errorVal == 6)
  {
    //TODO: Recover from error code 6
  }
#endif
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
      P1OUT ^= BIT0;                      // Toggle P1.0 RED LED
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

#if ACCESS_SD_CARD
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
#endif
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

    if(bufferPosition  > BUFFER_SIZE-2)      //bufferPosition + 1 > BUFFER_SIZE - 1, this would put a value out of range
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


