//-----------------------------------------------------------------------------
// cx_Logging.h
//   Include file for managing logging.
//-----------------------------------------------------------------------------

// Adapted from https://github.com/anthony-tuininga/cx_Logging

#include <windows.h>
#include <stdio.h>
#include <share.h>

#define LOCK_TYPE CRITICAL_SECTION
#define CX_LOGGING_API(t) t

// define structure for managing exception information
typedef struct {
    char message[MAX_PATH + 1024];
} ExceptionInfo;

// define structure for managing logging state
typedef struct {
    FILE *fp;
    char *fileName;
    char *fileNameMask;
    char *prefix;
    unsigned long level;
    unsigned long maxFiles;
    unsigned long maxFileSize;
    unsigned long seqNum;
    int reuseExistingFiles;
    int rotateFiles;
    int fileOwned;
    ExceptionInfo exceptionInfo;
} LoggingState;

// define logging levels
#define LOG_LEVEL_DEBUG                 10
#define LOG_LEVEL_INFO                  20
#define LOG_LEVEL_WARNING               30
#define LOG_LEVEL_ERROR                 40
#define LOG_LEVEL_CRITICAL              50
#define LOG_LEVEL_NONE                  100

// define defaults
#define DEFAULT_MAX_FILE_SIZE           1024 * 1024
#define DEFAULT_PREFIX                  "%t"

// declarations of methods exported
CX_LOGGING_API(int) StartLogging(const char*, unsigned long, unsigned long,
        unsigned long, const char *);
CX_LOGGING_API(int) StartLoggingEx(const char*, unsigned long, unsigned long,
        unsigned long, const char *, int, int, ExceptionInfo*);
CX_LOGGING_API(void) StopLogging(void);

CX_LOGGING_API(int) LogMessage(unsigned long, const char*);

CX_LOGGING_API(unsigned long) GetLoggingLevel(void);
CX_LOGGING_API(int) SetLoggingLevel(unsigned long);

CX_LOGGING_API(int) IsLoggingStarted(void);
