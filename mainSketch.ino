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
#define DEBUG          true    //if it is false, can increase BUFFER_RAM to 256 and BUFFER_COUNT to 3
#define WRITE_DEBUG    false
#define TIMING_DEBUG   false
#define INCLUDE_HEADER true
#define READ_FROM_DIRECTORY true  //seems to cause RAM issues when true, keep false for most testing
//lower BUFFER_RAM to 96 while testing this, raise to 196 when not testing 

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


#define ADC_SAMPLE_FREQUENCY 1 //Measured in KHz

#define FILE_PATH_SIZE 22
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
  char currentFilePath[FILE_PATH_SIZE] = "LOGFILES/LOG00001.TXT";
#else
  char currentFilePath[13] = "LOG00000.TXT";
#endif
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
  
  #if READ_FROM_DIRECTORY
    getNextFile();
  #else
    #if INCLUDE_HEADER
    generateHeader();
    #endif
  #endif
  
  
  
  setupPins();
  prepareADC();
  prepareADCTimer();
  waitForStart();
  disk_initialize();

  __enable_interrupt();






  collectData = true;
  #if DEBUG
    Serial.println("END SETUP");
  #endif


  
}
//#if READ_FROM_DIRECTORY
void getNextFile()
{
  //selects directory
  //directory = 0;
  DIR directory;
  char rc = FatFs.opendir(&directory, LOG_FILES_LOCATION);
  if(rc) recover(rc+80);
  
  FILINFO file;

  rc = FatFs.readdir(&directory,&file);
  if(rc) recover(rc+90);
  
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
  
  
  #if INCLUDE_HEADER
    generateHeader();
  #endif
  
  #if DEBUG
    Serial.println("Opening new file: ");
    for(int i = 0; i < 22; i++)
    {
      Serial.println(currentFilePath[i]);
    }
    Serial.println(currentFilePath);
  #endif
}
//#endif  //READ_FROM_DIRECTORY
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
  
  char FatFsReturnChar = FatFs.open(currentFilePath);
  if (FatFsReturnChar) recover(FatFsReturnChar+100);
  isFileOpen = true;
  bw = 0;
  FatFsReturnChar = FatFs.lseek(SDwritePosition);
  if (FatFsReturnChar) recover(FatFsReturnChar+10);
  
  
  
  
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
    writeSingleBuffer(SDwriteBuffer, bw);
    currentWriteLength+= BUFFER_SIZE;
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
    writeSingleBuffer(SDwriteBuffer, bw);
    currentWriteLength+= BUFFER_SIZE;
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
  SDwriteBuffer[i+9] = '=';
  SDwriteBuffer[i+10] = 'T';
  SDwriteBuffer[i+11] = 'R';
  SDwriteBuffer[i+12] = 'U';  //next 2 bytes are buffer Size
  SDwriteBuffer[i+13] = 'E';
  SDwriteBuffer[i+14] = '\r';
  SDwriteBuffer[i+15] = '\n';
  
  
  
  
  
  
  writeSingleBuffer(SDwriteBuffer, bw);
  

  //writes
 
    //quick ceiling calculation
    //this should place
    SDwritePosition = HEADER_SIZE;
    currentWriteLength = HEADER_SIZE;
    
    FatFsReturnChar = FatFs.write(0, 0, &bw);  //Finalize write
    if (FatFsReturnChar) recover(FatFsReturnChar);
    FatFsReturnChar = FatFs.close();  //Close file
    if (FatFsReturnChar) recover(FatFsReturnChar);
    isFileOpen = false;
    

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
void writeSingleBuffer(char *buffer, short unsigned int bw)
{
  char FatFsReturnChar = FatFs.write(buffer, BUFFER_SIZE,&bw);
  if (FatFsReturnChar) recover(FatFsReturnChar+20);
  currentWriteLength += BUFFER_SIZE;
}
void checkFile()
{
  //TODO: Check for EOF, switch to next file if it is
}
#if !WRITE_DEBUG
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


    FatFsReturnChar = FatFs.open(currentFilePath);
    if (FatFsReturnChar) recover(FatFsReturnChar);
    isFileOpen = true;
   // bw = 0;
    FatFsReturnChar = FatFs.lseek(SDwritePosition);
    if (FatFsReturnChar) recover(FatFsReturnChar+10);
  }


  memcpy(&SDwriteBuffer, (const char*)&writeBuffer[bufferNumber], BUFFER_SIZE);
  //SDwriteBuffer[0] = 'R';
  
  
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

