/*
 * This should replace the level I set up in Energia
 *
 *
 */


#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


#include "config.h"

#include "dataWriter.h"

extern "C"
{
#include <pfatfsEdited.h>
#include <pffconfEdited.h>
}

#include "pfatfsEdited.h"



//Begin definitions


void dataWriter::setupWriter()
{
  currentWritePosition = 0;
  fileWritePosition = 0;
  fileSize = 0;
  isFileOpen = false;
  numGeneratedFiles = 0;
  generateFileName();

  //This initializes the FatFs library and prepares it for
  //writing to the SD card
  FatFs.begin((char)cs_pin); //may need to add ,8 arguement to change spi clock divider

  //Occasional issues arrise when the SD card is midwrite and the MSP432 is reset
  //This line is here to hopefully prevent those issues
  FatFs.close();

  //Note, this will call openWithSize, which automatically changes fileSize
  openFile(fullFilePath);

  //This creates the generic header for the file
  writeHeader();



}
void dataWriter::writeData(short incData[],short length)
{
  /*
    NOTE:
    MSP432 stores shorts as big endian, so adjacent bytes are flipped
    you can either flip them on the MSP, or on the external software
    currently set to be interpreted by external software
  */
  //this just sends short array to
  //sendData((char *)incData,length*2);
  
  //note: changing endianness is destructive to incData
  #if CHANGE_ENDIANNESS
      //char tBuffer[length*2];
      #if TIMING_DEBUG
        long time = micros();
      #endif
      swapBytes16(incData,length);
      #if TIMING_DEBUG
         time = micros() - time;
        Serial.print("swap ");Serial.print(length * 2); Serial.print(" bytes took us");Serial.println(time);
      #endif
      delay(100);
      writeData((char *)incData,length*2);
    #else
      writeData((char *)incData,length*2);
    #endif
  
}
void dataWriter::writeData(char buffer[], short length)
{
  long bufferPosition = 0;
  //periodically closes file to ensure data saved properly
#if DEBUG
  Serial.println("Entering writeData ");
  Serial.print("isFileOpen: ");Serial.println(isFileOpen);
  Serial.print("currentWritePosition = ");Serial.println(currentWritePosition);
  Serial.print("fileWritePosition = "); Serial.println(fileWritePosition);
  Serial.print("overallWritePosition = "); Serial.println(currentWritePosition + fileWritePosition);
  //Serial.print("writeString: "); Serial.println(buffer); //note: may not be accurate if you are not passing full string to writeData
  Serial.print("writeAmount = ");Serial.println(length);
#endif


    //if the next write would put it outside EOF, it closes the file and opens the next one

  /*
    This check ensures the next write would not be too large for the file
   if it is, selects next valid file
   */
   while(fileWritePosition + currentWritePosition + length >= fileSize || (currentWritePosition + length >= MAX_UNSAVED_SEGMENT_SIZE))
   {
     long tWriteLength = 0;
     if(((long)MAX_UNSAVED_SEGMENT_SIZE - ((long)currentWritePosition)) >= (fileSize - (currentWritePosition + fileWritePosition)))
     {
        tWriteLength = fileSize-(currentWritePosition + fileWritePosition);
        SDWriter(&buffer[bufferPosition],tWriteLength);
        currentWritePosition+=tWriteLength;
        bufferPosition+=tWriteLength;
        length -=tWriteLength;
        
        #if DEBUG
         Serial.println("Entering selectNextFile");
         Serial.print("\tfileWritePosition = "); Serial.println(fileWritePosition);
         Serial.print("\tcurrentWritePosition = "); Serial.println(currentWritePosition);
         Serial.print("\tlength = ");Serial.println(length);
         Serial.print("\tSum = ");Serial.println(length + fileWritePosition + currentWritePosition);
         Serial.print("\tfileSize = ");Serial.println(fileSize);
       #endif
     
     
      selectNextFile();
      writeHeader();
     }
     else
     {
           /*
        This check ensures the file is closed periodically
       I have in the past experienced errors when writting more than about 30
       segments at the same time, so I have set the number of segments to
       write to 30. (I believe exact value should be less that 32 * 512
       
       
       Additionally, periodically closing the file should prevent data loss
       in the case of power loss while the file is open
       */
       tWriteLength = MAX_UNSAVED_SEGMENT_SIZE - currentWritePosition;
       
       #if DEBUG
        Serial.println("closing file for short save");
      #endif
    
      //This segment fills the write segment block before closing
      SDWriter(&buffer[bufferPosition],tWriteLength);
      currentWritePosition+=tWriteLength;
      bufferPosition+=tWriteLength;
      length -=tWriteLength;
    
    
      fileWritePosition += currentWritePosition;
      currentWritePosition = 0;
      closeFile();
      generateFileName();
      openFile(fullFilePath,fileWritePosition);
     }
    
    
     //This segment fills the file before switching files
     
     
     
   }
   //a final check to ensure a file is open
   if(!isFileOpen)
  {
    generateFileName();
    #if DEBUG
      Serial.print("writeData is opening a file at position : "); 
      Serial.println(currentWritePosition + fileWritePosition);
      Serial.print("The file name is : "); 
      Serial.println(fullFilePath);
    #endif
    openFile(fullFilePath,currentWritePosition + fileWritePosition);
  }

  SDWriter(&buffer[bufferPosition],length);
  currentWritePosition+=length;
} // close void dataWriter::writeData(char buffer[], short length)
void dataWriter::selectNextFile()
{
  char testVal[15] = "FILE_USED  =TT";
  char readVal[15];
  boolean validFile = false;
  while(!validFile)
  {
    validFile = true;
    closeFile();
    numGeneratedFiles++;
    generateFileName();
    fileWritePosition = 0;
    currentWritePosition = 0;

    openFile(fullFilePath);
  
    if(HEADER_SIZE >= fileSize)
    {
      #if DEBUG
        Serial.print("invalid file, too little");
      #endif
      validFile = false;
      continue;
    }

    int tVal = 0;
    char rc = FatFs.read(readVal,(WORD)14,(WORD *)&tVal);
    if(rc) die(rc);
    if(readVal == testVal)
    {
      #if DEBUG
        Serial.println("invalid file, already used");
      #endif
      validFile = false;
      continue;
    }
  }
}
void dataWriter::openFile(char fileName[],long seekPos)
{
#if DEBUG
  Serial.print("Opening file: ");
  Serial.println(fileName);
#endif
#if DEBUG
  long prevTime = micros();
#endif
  char rc = FatFs.openWithSize(fileName,&fileSize);
#if DEBUG
  prevTime = micros()-prevTime;
  Serial.print("openWithSize took: "); 
  Serial.println(prevTime);
#endif

#if DEBUG
  Serial.print("File size: "); 
  Serial.println(fileSize);
#endif

  if (rc) die(rc);
  rc = FatFs.lseek(seekPos);
  if(rc) die(rc);
  isFileOpen = true;
  //fileSize = FatFs.getFileSize(fileName);
}
void dataWriter::openFile(char fileName[])
{
  openFile(fileName,0);
}
void dataWriter::shutdown()
{

  closeFile();
  //close all files and disconnect from SD card
}
void dataWriter::closeFile()
{
  char rc = FatFs.close();
  if(rc) die(rc);

  isFileOpen = false;
}
void dataWriter::generateFileName()
{
  long tNumFiles = numGeneratedFiles;
  char tChar[FILE_ID_CHAR_SIZE+1] = {
    'A'  };
  char counter = 0;

  while(counter < FILE_ID_CHAR_SIZE)
  {
    //This loop converts the number of generated files into a string of chara
    short tVal = tNumFiles % FILE_ID_CHAR_RANGE;
    if(tVal > 9)
    {
      tVal += 65 - 10; //capital letters
    }
    else
    {
      tVal+= 48;      //numbers
    }
    tChar[FILE_ID_CHAR_SIZE-1-counter] = tVal;
    tNumFiles /= FILE_ID_CHAR_RANGE;
    counter++;
  }
  /*TODO: ASK ERIC why I need this
   I have absolutely no idea why, but it doesn't work without this
   code snippet
   
   char tChar2[FILE_ID_CHAR_SIZE];
   for(int i = 0; i < FILE_ID_CHAR_SIZE;i++)
   tChar2[i] = tChar[i];
   tChar[0] = tChar2[0];*/


  strcpy(fullFilePath,LOG_FILES_LOCATION);
  strcat(fullFilePath,"/LOG");
  strcat(fullFilePath,tChar);
  strcat(fullFilePath,".TXT");

}
void dataWriter::SDWriter(char buffer[],short length)
{
  char rc = FatFs.write(buffer,length,&bytesWritten);
  if(rc) die(rc);
}
void dataWriter::writeHeader()
{
  char outputBuffer[HEADER_SIZE];
  char dataBuffer[2];
  memset(dataBuffer,'\0',2);
  memset(outputBuffer,'\0',HEADER_SIZE);
  //Consider 16 byte segments
  /* FORMAT:
   	   FILE_USED  =TT
   	   HEADER_SIZE= 2 bytes
   	   BUFFER_SIZE= 2 bytes
   	   SAMPLE_RATE= 2 bytes
           INPUT_SIZE = 2 bytes
           DATA_ENDIAN= 0/1        let 1 be big endian, 0 be little endian (big endian on MSP432)
   	   */


  /*
  strcat(outputBuffer,"FILE_USED  =TT\r\n");
  strcat(outputBuffer,"HEADER_SIZE=");
  memset(dataBuffer,HEADER_SIZE,2);
  strcat(outputBuffer,dataBuffer);
  strcat(outputBuffer,"\r\nBUFFER_SIZE=");
  memset(dataBuffer,BUFFER_SIZE,2);
  strcat(outputBuffer,dataBuffer);
  strcat(outputBuffer,"\r\nSAMPLE_RATE=");
  memset(dataBuffer,SAVE_SAMPLE_FREQUENCY,2);
  strcat(outputBuffer,dataBuffer);
  strcat(outputBuffer,"\r\nINPUT_SIZE =");
  memset(dataBuffer,DATA_SIZE,2);
  strcat(outputBuffer,dataBuffer);
  strcat(outputBuffer,"\r\nDATA_ENDIAN=");
  memset(dataBuffer,DATA_ENDIAN,2);
  strcat(outputBuffer,dataBuffer);
  */
  int i = 0;
  if(i +16>= HEADER_SIZE)
  {
    writeData(outputBuffer,i); //prints what it can
    return; //note that this should not happen if headerSize is large enough
  }

  //there is almost certainly a better way to do this, probably with strings
  outputBuffer[i+0] = 'F';
  outputBuffer[i+1] = 'I';
  outputBuffer[i+2] = 'L';
  outputBuffer[i+3] = 'E';
  outputBuffer[i+4] = '_';
  outputBuffer[i+5] = 'U';
  outputBuffer[i+6] = 'S';
  outputBuffer[i+7] = 'E';
  outputBuffer[i+8] = 'D';
  outputBuffer[i+9] = ' ';
  outputBuffer[i+10] = ' ';
  outputBuffer[i+11] = '=';
  outputBuffer[i+12] = 'T';  //next 2 bytes are buffer Size
  outputBuffer[i+13] = 'T';
  outputBuffer[i+14] = '\r';
  outputBuffer[i+15] = '\n';

  i+=16;
  if(i +16> HEADER_SIZE)
  {
    writeData(outputBuffer,i); //prints what it can
    return; //note that this should not happen if headerSize is large enough
  }
  outputBuffer[i+0]  = 'H';
  outputBuffer[i+1]  = 'E';
  outputBuffer[i+2]  = 'A';
  outputBuffer[i+3]  = 'D';
  outputBuffer[i+4]  = 'E';
  outputBuffer[i+5]  = 'R';
  outputBuffer[i+6]  = '_';
  outputBuffer[i+7]  = 'S';
  outputBuffer[i+8]  = 'I';
  outputBuffer[i+9]  = 'Z';
  outputBuffer[i+10] = 'E';
  outputBuffer[i+11] = '=';
  outputBuffer[i+12] = (HEADER_SIZE>>8) & 0xff;  //next 2 bytes are buffer Size
  outputBuffer[i+13] = (HEADER_SIZE>>0) & 0xff;
  outputBuffer[i+14] = '\r';
  outputBuffer[i+15] = '\n';




  i+=16;
  if(i +16> HEADER_SIZE)
  {
    writeData(outputBuffer,i); //prints what it can
    return; //note that this should not happen if headerSize is large enough
  }
  outputBuffer[i+0] = 'B';
  outputBuffer[i+1] = 'U';
  outputBuffer[i+2] = 'F';
  outputBuffer[i+3] = 'F';
  outputBuffer[i+4] = 'E';
  outputBuffer[i+5] = 'R';
  outputBuffer[i+6] = '_';
  outputBuffer[i+7] = 'S';
  outputBuffer[i+8] = 'I';
  outputBuffer[i+9] = 'Z';
  outputBuffer[i+10] = 'E';
  outputBuffer[i+11] = '=';
  outputBuffer[i+12] = (BUFFER_SIZE>>8) & 0xff;  //next 2 bytes are buffer Size
  outputBuffer[i+13] = (BUFFER_SIZE>>0) & 0xff;
  outputBuffer[i+14] = '\r';
  outputBuffer[i+15] = '\n';




  i+=16;
  if(i +16> HEADER_SIZE)
  {
    writeData(outputBuffer,i); //prints what it can
    return; //note that this should not happen if headerSize is large enough
  }
  outputBuffer[i+0] = 'S';
  outputBuffer[i+1] = 'A';
  outputBuffer[i+2] = 'M';
  outputBuffer[i+3] = 'P';
  outputBuffer[i+4] = 'L';
  outputBuffer[i+5] = 'E';
  outputBuffer[i+6] = '_';
  outputBuffer[i+7] = 'R';
  outputBuffer[i+8] = 'A';
  outputBuffer[i+9] = 'T';
  outputBuffer[i+10] = 'E';
  outputBuffer[i+11] = '=';
  outputBuffer[i+12] = (SAVE_SAMPLE_FREQUENCY>>8) & 0xff;//next 2 bytes are buffer Size
  outputBuffer[i+13] = (SAVE_SAMPLE_FREQUENCY) & 0xff;
  outputBuffer[i+14] = '\r';
  outputBuffer[i+15] = '\n';

  i+=16;
  if(i +16> HEADER_SIZE)
  {
    writeData(outputBuffer,i); //prints what it can
    return; //note that this should not happen if headerSize is large enough
  }
  outputBuffer[i+0] = 'I';
  outputBuffer[i+1] = 'N';
  outputBuffer[i+2] = 'P';
  outputBuffer[i+3] = 'U';
  outputBuffer[i+4] = 'T';
  outputBuffer[i+5] = '_';
  outputBuffer[i+6] = 'S';
  outputBuffer[i+7] = 'I';
  outputBuffer[i+8] = 'Z';
  outputBuffer[i+9] = 'E';
  outputBuffer[i+10] = ' ';
  outputBuffer[i+11] = '=';
  outputBuffer[i+12] = (DATA_SIZE>>8) & 0xff;//next 2 bytes are buffer Size
  outputBuffer[i+13] = (DATA_SIZE) & 0xff;
  outputBuffer[i+14] = '\r';
  outputBuffer[i+15] = '\n';
  
  
  
  i+=16;
  if(i +16> HEADER_SIZE)
  {
    writeData(outputBuffer,i); //prints what it can
    return; //note that this should not happen if headerSize is large enough
  }
  outputBuffer[i+0] = 'D';
  outputBuffer[i+1] = 'A';
  outputBuffer[i+2] = 'T';
  outputBuffer[i+3] = 'A';
  outputBuffer[i+4] = '_';
  outputBuffer[i+5] = 'E';
  outputBuffer[i+6] = 'N';
  outputBuffer[i+7] = 'D';
  outputBuffer[i+8] = 'I';
  outputBuffer[i+9] = 'A';
  outputBuffer[i+10] = 'N';
  outputBuffer[i+11] = '=';
  outputBuffer[i+12] = ((DATA_ENDIAN^CHANGE_ENDIANNESS)>>8) & 0xff;//next 2 bytes are buffer Size
  outputBuffer[i+13] = (DATA_ENDIAN^CHANGE_ENDIANNESS) & 0xff;
  outputBuffer[i+14] = '\r';
  outputBuffer[i+15] = '\n';
  
  i+=16;
  if(i +16> HEADER_SIZE)
  {
    writeData(outputBuffer,i); //prints what it can
    return; //note that this should not happen if headerSize is large enough
  }
  

  outputBuffer[HEADER_SIZE-2] = '\r';
  outputBuffer[HEADER_SIZE-1] = '\n';
  writeData(outputBuffer,HEADER_SIZE); //prints what it can

}
void dataWriter::die(int errorCode)
{
  FatFs.close();
  //closeFile();
  #if DEBUG
    Serial.println();
    Serial.print("Failed with rc=");
    Serial.print(errorCode,DEC);
  #endif

  //alternate implementation
  //digitalWrite(RED_LED,HIGH)
  RIGHT_RLED_PORT_OUT |= RIGHT_RLED; 
  for(;;)
  { 
  }
}
void dataWriter::swapBytes16(short inBuffer[],short length)
{
  
  int i = 0;
  //splitting it up like this makes it faster
  //looping is kinda time expensive, so this should
  //speed up program, my tests appeared to be the case
  //it takes less than half the time when it is processes like this
  
  while(i+16 < length)
  {
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
  }
  while(i < length)
  {
    inBuffer[i] = ((inBuffer[i] & 0x00ff) << 8) | ((inBuffer[i++] & 0xff00) >> 8);
  }
}

