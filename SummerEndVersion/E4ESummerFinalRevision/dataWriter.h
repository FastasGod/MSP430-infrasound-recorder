//long SDwritePosition = 0; //increment by 512 to find desired sector
//char SDwriteBuffer[BUFFER_SIZE];
//char readyBuffers[BUFFER_COUNT] = {0};
#ifndef dataWriter_h
#define dataWriter_h

#include <stdbool.h>
#include <pfatfsEdited.h>
#include <pffconfEdited.h>

#include "config.h"


//Begin dataWriterConfigs
#ifndef dataWriterConf
  #define dataWriterConf  1
  
  #define LOG_FILE_PATH_SIZE				22
  #define LOG_FILES_LOCATION             "LOGFILES" //Path to folder to contain output files
  #define FILE_ID_CHAR_SIZE				5
  #define FILE_ID_CHAR_RANGE				16			//probably use either 10 or 16 (LOG99999 vs. LOGfffff
//#define LOG_FILES_LOCATION_SLASH		LOG_FILES_LOCATION/
  #define MAX_UNSAVED_SEGMENT_SIZE                      512 *  30 
  #define DATA_SIZE                                     2  
#endif




class dataWriter
{
  public:
    void setupWriter();
    void writeData(short[],short);
    void writeData(char[],short);
    //void writeData(short[],short);
    //void writeData(char[],short);
    void shutdown();
  
  private:
    void selectNextFile();
    unsigned long writeBufferPosition;
    unsigned long fileWritePosition;
    unsigned long currentWritePosition;
    unsigned long fileSize;
    bool isFileOpen;
    long numGeneratedFiles;
    char fullFilePath[]; //Example name "LOGFILES/LOG00000.TXT
    
    void SDWriter(char[],short);

    void closeFile();
    void openFile(char[],long);
    void openFile(char[]);
    //void openFile(char[]);
    //void openFile(char[],long);
    void writeHeader();
    void generateHeader(char[]);
    void generateFileName();
    void die(int);
    
    void swapBytes16(short[],short);
    
    unsigned short int bytesWritten,br;
    int rc;
    DIR directory;
    FILINFO file;

};


#endif
