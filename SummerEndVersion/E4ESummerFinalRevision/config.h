/* This is the config file for the MSP432 INFRASOUND project */
#define DEBUG                true   //if it is false, can increase BUFFER_RAM to 256 and BUFFER_COUNT to 3, if true lower to (AT most) 192
#define TIMING_DEBUG         false
#define SAMPLE_TIMING_DEBUG  true

#define USE_SERIAL (DEBUG || TIMING_DEBUG || SAMPLE_TIMING_DEBUG)
//
#define USE_LEDS            true


#define LEFT_RLED_PORT_DIR   P1DIR
#define LEFT_RLED_PORT_OUT   P1OUT
#define LEFT_RLED            BIT0

#define RIGHT_RLED_PORT_DIR  P2DIR
#define RIGHT_RLED_PORT_OUT  P2OUT
#define RIGHT_RLED           BIT0


#define LEFT_BUTTON_PORT_DIR     P1DIR
#define LEFT_BUTTON_PORT_IN      P1IN
#define LEFT_BUTTON_PORT_REN     P1REN
#define LEFT_BUTTON              BIT1

#define  SAVE_SAMPLE_FREQUENCY            (long)4000
#define  OVERSAMPLE_MULTIPLIER            (long)8
#define  ADC_SAMPLE_FREQUENCY             (OVERSAMPLE_MULTIPLIER * SAVE_SAMPLE_FREQUENCY)

//  audioBufferHandlerConf
#define audioBufferHandlerConf            1

//NOTE: there are occasional other sizable buffers in the program
#define BUFFER_RAM                        49152                  //Measured in bytes 49152 for 3 16K buffers
#define BUFFER_COUNT                      2
#define BUFFER_SIZE                       BUFFER_RAM/(BUFFER_COUNT+1)  //In current setup there will be recording buffers and
//a separate   


//  dataWriter configs
#define dataWriterConf                                  1

#define HEADER_SIZE                                     512 
#define cs_pin                                          18
#define FILE_ID_CHAR_SIZE				5                       //current file name pattern is LOGXXXXX with num X being
#define FILE_ID_CHAR_RANGE				10			//probably use either 10 or 16 (LOG99999 vs. LOGfffff
#define LOG_FILE_PATH_SIZE				17 + FILE_ID_CHAR_SIZE
#define LOG_FILES_LOCATION                              "LOGFILES" //Path to folder to contain output files
#define MAX_UNSAVED_SEGMENT_SIZE                        512 * 30  //This will periodically close the file during writes

#define WRITE_BUFFER_SIZE                               512

//Data size is the size of each data sample, the following values will likely become pointless as data is processed and
//data is passed through program differently
#define DATA_SIZE                                        2                    //size of each datatype in bytes

//This is used for converting short to char,
//you may want to programatically switch these values
//around on the processor, but I left it to be done on
//the matlab decoder
#define DATA_ENDIAN                                      1              //let 1 be big endian, 0 be little endian

//Use this to change endianness on proccessor instead of matlab
//NOTE: using this will disrupt the workingBuffer data, do all processing beforehand, or
#define CHANGE_ENDIANNESS                                1
