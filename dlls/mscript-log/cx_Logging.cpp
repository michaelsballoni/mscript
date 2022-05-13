//-----------------------------------------------------------------------------
// cx_Logging.c
//   Shared library for logging used by Python and C code.
//-----------------------------------------------------------------------------
#include "cx_Logging.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <io.h>

#define THREAD_FORMAT           "%.5ld"
#define DATE_FORMAT             "%.4d/%.2d/%.2d"
#define TIME_FORMAT             "%.2d:%.2d:%.2d.%.3d"
#define TICKS_FORMAT            "%.10d"

// define platform specific methods for manipulating locks
#define INITIALIZE_LOCK(lock)   InitializeCriticalSection(&lock)
#define ACQUIRE_LOCK(lock)      EnterCriticalSection(&lock)
#define RELEASE_LOCK(lock)      LeaveCriticalSection(&lock)

// define macro to get the build version as a string
#define xstr(s)                 str(s)
#define str(s)                  #s
#define BUILD_VERSION_STRING    xstr(BUILD_VERSION)


// define global logging state
LoggingState *gLoggingState;
LOCK_TYPE gLoggingStateLock;


// define keywords for common Python methods
static const char *gStartLoggingWithFileKeywordList[] = {"fileName", "level",
        "maxFiles", "maxFileSize", "prefix", "encoding", "reuse", "rotate",
        NULL};
static const char *gStartLoggingNoFileKeywordList[] = {"level", "prefix", "encoding",
        NULL};


//-----------------------------------------------------------------------------
// LoggingState_OpenFileForWriting()
//   Open the file for writing, if possible. In order to workaround the nasty
// fact that on Windows file handles are inherited by child processes by
// default and open files cannot be renamed, the handle opened by fopen() is
// first cloned to a non-inheritable file handle and the original closed. In
// this manner, any subprocess created does not prevent log file rotation from
// occurring.
//-----------------------------------------------------------------------------
static int LoggingState_OpenFileForWriting(
    LoggingState *state)                // state to use for writing
{
    struct stat statBuffer;

    HANDLE sourceHandle, targetHandle;
    int fd, dupfd;

    // verify that a file is not reused if such is not supposed to take place
    if (!state->reuseExistingFiles &&
            stat(state->fileName, &statBuffer) == 0) {
        sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                "File %s exists and reuse not specified.", state->fileName);
        return -1;
    }

    // open file for writing
    if (state->reuseExistingFiles)
#pragma warning(suppress : 4996)
        state->fp = fopen(state->fileName, "w");
    else
        state->fp = _fsopen(state->fileName, "w", _SH_DENYWR);

    if (!state->fp) {
        sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                "Failed to open file %s: OS error %d", state->fileName, errno);
        return -1;
    }

    // duplicate file handle as described above
    fd = _fileno(state->fp);
    sourceHandle = (HANDLE) _get_osfhandle(fd);
    if (!DuplicateHandle(GetCurrentProcess(), sourceHandle,
            GetCurrentProcess(), &targetHandle, 0, FALSE,
            DUPLICATE_SAME_ACCESS)) {
        fclose(state->fp);
        state->fp = NULL;
        sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                "Failed to duplicate handle on file %s: Windows error %ld",
                state->fileName, GetLastError());
        return -1;
    }
    fclose(state->fp);
    dupfd = _open_osfhandle((intptr_t) targetHandle, O_TEXT);
    state->fp = _fdopen(dupfd, "w");
    if (!state->fp) {
        sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
            "Failed to open file from handle for %s", state->fileName);
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// WriteString()
//   Write string to the file.
//-----------------------------------------------------------------------------
static int WriteString(
    LoggingState *state,                // state to use for writing
    const char *string)                 // string to write to the file
{
    if (fputs(string, state->fp) == EOF) {
        sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                "Failed to write to file %s: OS error %d.", state->fileName,
                errno);
        return -1;
    }
    return 0;
}


//-----------------------------------------------------------------------------
// WriteLevel()
//   Write the level to the file.
//-----------------------------------------------------------------------------
static int WriteLevel(
    LoggingState *state,                // state to use for writing
    unsigned long level)                // level to write to the file
{
    switch(level) {
        case LOG_LEVEL_DEBUG:
            return WriteString(state, "DEBUG");
        case LOG_LEVEL_INFO:
            return WriteString(state, "INFO");
        case LOG_LEVEL_WARNING:
            return WriteString(state, "WARN");
        case LOG_LEVEL_ERROR:
            return WriteString(state, "ERROR");
        case LOG_LEVEL_CRITICAL:
            return WriteString(state, "CRIT");
        case LOG_LEVEL_NONE:
            return WriteString(state, "TRACE");
    }

    char temp[20];
    sprintf_s<sizeof(temp)>(temp, "%ld", level);
    return WriteString(state, temp);
}


//-----------------------------------------------------------------------------
// WritePrefix()
//   Write the prefix to the file.
//-----------------------------------------------------------------------------
static int WritePrefix(
    LoggingState *state,                // state to use for writing
    unsigned long level)                // level at which to write
{
    SYSTEMTIME time{};
    char temp[40], *ptr;
    int gotTime;

    gotTime = 0;
    ptr = state->prefix;
    while (*ptr) {
        if (*ptr != '%') {
            if (fputc(*ptr++, state->fp) == EOF) {
                sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                        "Failed to write character to file %s.",
                        state->fileName);
                return -1;
            }
            continue;
        }
        ptr++;
        switch(*ptr) {
            case 'i':
                sprintf_s<sizeof(temp)>(temp, THREAD_FORMAT, (long) GetCurrentThreadId());
                if (WriteString(state, temp) < 0)
                    return -1;
                break;
            case 'd':
            case 't':
                if (!gotTime) {
                    gotTime = 1;
                    GetLocalTime(&time);
                }
                if (*ptr == 'd')
                    sprintf_s<sizeof(temp)>(temp, DATE_FORMAT, time.wYear, time.wMonth,
                            time.wDay);
                else
                    sprintf_s<sizeof(temp)>(temp, TIME_FORMAT, time.wHour, time.wMinute,
                            time.wSecond, time.wMilliseconds);
                if (WriteString(state, temp) < 0)
                    return -1;
                break;
            case 'l':
                if (WriteLevel(state, level) < 0)
                    return -1;
                break;
            case '\0':
                break;
            default:
                if (fputc('%', state->fp) == EOF ||
                        fputc(*ptr, state->fp) == EOF) {
                    sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                            "Failed to write %% directive to file %s.",
                            state->fileName);
                    return -1;
                }
        }
        if (*ptr)
            ptr++;
    }
    if (*state->prefix) {
        if (fputc(' ', state->fp) == EOF) {
            sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                    "Failed to write separator to file %s.", state->fileName);
            return -1;
        }
    }

    return 0;
}


//-----------------------------------------------------------------------------
// WriteTrailer()
//   Write the trailing line feed to the file and flush it.
//-----------------------------------------------------------------------------
static int WriteTrailer(
    LoggingState *state)                // state to use for writing
{
    if (WriteString(state, "\n") < 0)
        return -1;
    if (fflush(state->fp) == EOF) {
        sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                "Cannot flush file %s", state->fileName);
        return -1;
    }

    return 0;
}


//-----------------------------------------------------------------------------
// SwitchLogFiles()
//   Switch log files by moving to the next file in sequence. An exception is
// raised if the file exists and reuse has not been specified.
//-----------------------------------------------------------------------------
static int SwitchLogFiles(
    LoggingState *state)                // state to use
{
    state->seqNum++;
    if (state->seqNum > state->maxFiles)
        state->seqNum = 1;
#pragma warning(suppress : 4996)
    sprintf(state->fileName, state->fileNameMask, state->seqNum);
    if (LoggingState_OpenFileForWriting(state) < 0)
        return -1;

    return 0;
}


//-----------------------------------------------------------------------------
// CheckForLogFileFull()
//   Checks to determine if the current file has reached its maximum size and
// if so, starts a new one.
//-----------------------------------------------------------------------------
static int CheckForLogFileFull(
    LoggingState *state)                // state to use for writing
{
    unsigned long position = 0;

    if (state->rotateFiles && state->maxFiles > 1) {
        if (state->fp) {
            position = ftell(state->fp);
            if (position < 0) {
                sprintf_s<sizeof(state->exceptionInfo.message)>(state->exceptionInfo.message,
                        "Cannot get file position for %s: OS error %d.",
                        state->fileName, errno);
                return -1;
            }
        }
        if (!state->fp || position >= state->maxFileSize) {
            if (state->fp) {
                if (WritePrefix(state, LOG_LEVEL_NONE) < 0)
                    return -1;
                if (WriteString(state, "switching to a new log file\n") < 0)
                    return -1;
                fclose(state->fp);
                state->fp = NULL;
            }
            if (SwitchLogFiles(state) < 0)
                return -1;
            if (WritePrefix(state, LOG_LEVEL_NONE) < 0)
                return -1;
            if (WriteString(state,
                    "starting logging (after switch) at level ") < 0)
                return -1;
            if (WriteLevel(state, state->level) < 0)
                return -1;
            if (WriteTrailer(state) < 0)
                return -1;
        }
    }
    return 0;
}


//-----------------------------------------------------------------------------
// WriteMessage()
//   Write the message to the file.
//-----------------------------------------------------------------------------
static int WriteMessage(
    LoggingState *state,                // state to use for writing
    unsigned long level,                // level at which to write
    const char *message)                // message to write
{
    if (CheckForLogFileFull(state) < 0)
        return -1;
    if (state->fp) {
        if (WritePrefix(state, level) < 0)
            return -1;
        if (!message)
            message = "(null)";
        if (WriteString(state, message) < 0)
            return -1;
        if (WriteTrailer(state) < 0)
            return -1;
    }
    return 0;
}


//-----------------------------------------------------------------------------
// LoggingState_Free()
//   Free the logging state.
//-----------------------------------------------------------------------------
static void LoggingState_Free(
    LoggingState *state)                // state to stop logging for
{
    if (state->fp) {
        if (state->fileOwned) {
            WriteMessage(state, LOG_LEVEL_NONE, "ending logging");
            fclose(state->fp);
        }
    }
    if (state->fileName)
        free(state->fileName);
    if (state->fileNameMask)
        free(state->fileNameMask);
    if (state->prefix)
        free(state->prefix);
    free(state);
}


//-----------------------------------------------------------------------------
// LoggingState_InitializeSeqNum()
//   Initialize the sequence number to start logging at when rotating files.
// The sequence number to use is the one following the most recent log file
// if all possible log file names are already used. If there is an available
// log file name, the lowest sequence number is used.
//-----------------------------------------------------------------------------
static void LoggingState_InitializeSeqNum(
    LoggingState *state)                // logging state just created
{
    struct stat statBuffer;
    unsigned long seqNum;
    time_t mtime = 0;

    for (seqNum = 1; seqNum <= state->maxFiles; seqNum++) {
#pragma warning(suppress : 4996)
        sprintf(state->fileName, state->fileNameMask, seqNum);
        if (stat(state->fileName, &statBuffer) < 0) {
            state->seqNum = seqNum;
            break;
        }
        if (statBuffer.st_mtime > mtime) {
            state->seqNum = seqNum + 1;
            if (state->seqNum > state->maxFiles)
                state->seqNum = 1;
            mtime = statBuffer.st_mtime;
        }
    }
#pragma warning(suppress : 4996)
    sprintf(state->fileName, state->fileNameMask, state->seqNum);
}


//-----------------------------------------------------------------------------
// LoggingState_OnCreate()
//   Called when the logging state is created and a file needs to be opened. It
// opens the file and writes the initial log messages to it.
//-----------------------------------------------------------------------------
static int LoggingState_OnCreate(
    LoggingState *state)                // logging state just created
{
    // open the file
    if (state->rotateFiles && state->maxFiles > 1)
        LoggingState_InitializeSeqNum(state);
    state->fileOwned = 1;
    if (LoggingState_OpenFileForWriting(state) < 0)
        return -1;

    // put out an initial message regardless of level
    if (WritePrefix(state, LOG_LEVEL_NONE) < 0)
        return -1;
    if (WriteString(state, "starting logging at level ") < 0)
        return -1;
    if (WriteLevel(state, state->level) < 0)
        return -1;
    if (WriteTrailer(state) < 0)
        return -1;

    // open the file
    return 0;
}


//-----------------------------------------------------------------------------
// LoggingState_New()
//   Create a new logging state.
//-----------------------------------------------------------------------------
static LoggingState* LoggingState_New(
    FILE *fp,                           // file to associate
    const char *fileName,               // name of file to open
    unsigned long level,                // level to use
    unsigned long maxFiles,             // maximum number of files
    unsigned long maxFileSize,          // maximum size of each file
    const char *prefix,                 // prefix to use
    int reuseExistingFiles,             // reuse existing files?
    int rotateFiles,                    // rotate files?
    ExceptionInfo *exceptionInfo)       // exception info
{
    char seqNumTemp[100];
    LoggingState *state;
    char *tmp;

    // initialize the logging state
    state = (LoggingState*) malloc(sizeof(LoggingState));
    if (!state) {
        strcpy_s<sizeof(exceptionInfo->message)>(exceptionInfo->message,
                "Failed to allocate memory for logging state.");
        return NULL;
    }
    state->fp = fp;
    state->fileOwned = 0;
    state->level = level;
    state->fileName = NULL;
    state->fileNameMask = NULL;
    state->prefix = NULL;
    state->reuseExistingFiles = reuseExistingFiles;
    state->rotateFiles = rotateFiles;
    if (maxFiles == 0)
        state->maxFiles = 1;
    else state->maxFiles = maxFiles;
    if (maxFileSize == 0)
        state->maxFileSize = DEFAULT_MAX_FILE_SIZE;
    else state->maxFileSize = maxFileSize;

    // allocate space for a file name mask
    state->fileNameMask = (char*)malloc(strlen(fileName) + 23);
    if (!state->fileNameMask) {
        strcpy_s<sizeof(exceptionInfo->message)>(exceptionInfo->message,
            "Failed to allocate memory for file name mask.");
        LoggingState_Free(state);
        return NULL;
    }

    // build the file name mask
    // if max files = 1 then use the file name exactly as is
#pragma warning(suppress : 4996)
    strcpy(state->fileNameMask, fileName);
    if (state->maxFiles > 1) {
        sprintf_s<sizeof(seqNumTemp)>(seqNumTemp, "%ld", state->maxFiles);
        tmp = (char*)strrchr(fileName, '.');
        if (tmp) {
#pragma warning(suppress : 4996)
            sprintf(state->fileNameMask + (tmp - fileName), ".%%.%ldld",
                    (unsigned long) strlen(seqNumTemp));
#pragma warning(suppress : 4996)
            strcat(state->fileNameMask, tmp);
        } else {
#pragma warning(suppress : 4996)
            sprintf(state->fileNameMask + strlen(fileName), ".%%.%ldld",
                    (unsigned long) strlen(seqNumTemp));
        }
    }

    // allocate space for the file name
    state->fileName = (char*)malloc(strlen(fileName) + 23);
    if (!state->fileName) {
        strcpy_s<sizeof(exceptionInfo->message)>(exceptionInfo->message,
            "Failed to allocate memory for file name.");
        LoggingState_Free(state);
        return NULL;
    }
#pragma warning(suppress : 4996)
    strcpy(state->fileName, fileName);

    // copy the prefix
    state->prefix = (char*)malloc(strlen(prefix) + 1);
    if (!state->prefix) {
        strcpy_s<sizeof(exceptionInfo->message)>(exceptionInfo->message,
            "Failed to allocate memory for prefix.");
        LoggingState_Free(state);
        return NULL;
    }
#pragma warning(suppress : 4996)
    strcpy(state->prefix, prefix);

    // open the file, if necessary and write any initial messages
    if (!state->fp && LoggingState_OnCreate(state) < 0) {
        strcpy_s<sizeof(exceptionInfo->message)>(exceptionInfo->message,
            state->exceptionInfo.message);
        LoggingState_Free(state);
        return NULL;
    }

    return state;
}


//-----------------------------------------------------------------------------
// LoggingState_SetLevel()
//   Set the level for the logging state.
//-----------------------------------------------------------------------------
static int LoggingState_SetLevel(
    LoggingState *state,                // state on which to change level
    unsigned long newLevel)             // new level to set
{
    if (WritePrefix(state, LOG_LEVEL_NONE) < 0)
        return -1;
    if (WriteString(state, "switched logging level from ") < 0)
        return -1;
    if (WriteLevel(state, state->level) < 0)
        return -1;
    if (WriteString(state, " to ") < 0)
        return -1;
    if (WriteLevel(state, newLevel) < 0)
        return -1;
    if (WriteTrailer(state) < 0)
        return -1;
    state->level = newLevel;
    return 0;
}


//-----------------------------------------------------------------------------
// StartLogging()
//   Start logging to the specified file.
//-----------------------------------------------------------------------------
CX_LOGGING_API(int) StartLogging(
    const char *fileName,               // name of file to write to
    unsigned long level,                // level to use for logging
    unsigned long maxFiles,             // maximum number of files to have
    unsigned long maxFileSize,          // maximum size of each file
    const char *prefix)                 // prefix to use in logging
{
    ExceptionInfo exceptionInfo;

    return StartLoggingEx(fileName, level, maxFiles, maxFileSize, prefix, 1, 1,
            &exceptionInfo);
}


//-----------------------------------------------------------------------------
// StartLoggingEx()
//   Start logging to the specified file.
//-----------------------------------------------------------------------------
CX_LOGGING_API(int) StartLoggingEx(
    const char *fileName,               // name of file to write to
    unsigned long level,                // level to use for logging
    unsigned long maxFiles,             // maximum number of files to have
    unsigned long maxFileSize,          // maximum size of each file
    const char *prefix,                 // prefix to use in logging
    int reuseExistingFiles,             // reuse existing files?
    int rotateFiles,                    // rotate files?
    ExceptionInfo* exceptionInfo)       // exception information (OUT)
{
    LoggingState *loggingState, *origLoggingState;

    loggingState = LoggingState_New(NULL, fileName, level, maxFiles,
            maxFileSize, prefix, reuseExistingFiles, rotateFiles,
            exceptionInfo);
    if (!loggingState)
        return -1;
    ACQUIRE_LOCK(gLoggingStateLock);
    origLoggingState = gLoggingState;
    gLoggingState = loggingState;
    RELEASE_LOCK(gLoggingStateLock);
    if (origLoggingState)
        LoggingState_Free(origLoggingState);
    return 0;
}


//-----------------------------------------------------------------------------
// StopLogging()
//   Stop logging to the specified file.
//-----------------------------------------------------------------------------
CX_LOGGING_API(void) StopLogging(void)
{
    LoggingState *loggingState;

    ACQUIRE_LOCK(gLoggingStateLock);
    loggingState = gLoggingState;
    gLoggingState = NULL;
    RELEASE_LOCK(gLoggingStateLock);
    if (loggingState)
        LoggingState_Free(loggingState);
}


//-----------------------------------------------------------------------------
// LogMessage()
//   Log a message to the log file.
//-----------------------------------------------------------------------------
CX_LOGGING_API(int) LogMessage(
    unsigned long level,                // level at which to log
    const char *message)                // message to log
{
    int result = 0;

    if (gLoggingState) {
        ACQUIRE_LOCK(gLoggingStateLock);
        if (gLoggingState && level >= gLoggingState->level)
            result = WriteMessage(gLoggingState, level, message);
        RELEASE_LOCK(gLoggingStateLock);
    }

    return result;
}


//-----------------------------------------------------------------------------
// GetLoggingLevel()
//   Return the current logging level.
//-----------------------------------------------------------------------------
CX_LOGGING_API(unsigned long) GetLoggingLevel(void)
{
    unsigned long level = LOG_LEVEL_NONE;

    ACQUIRE_LOCK(gLoggingStateLock);
    if (gLoggingState)
        level = gLoggingState->level;
    RELEASE_LOCK(gLoggingStateLock);
    return level;
}


//-----------------------------------------------------------------------------
// SetLoggingLevel()
//   Set the current logging level.
//-----------------------------------------------------------------------------
CX_LOGGING_API(int) SetLoggingLevel(
    unsigned long newLevel)             // new level to use
{
    int result = 0;

    ACQUIRE_LOCK(gLoggingStateLock);
    if (gLoggingState)
        result = LoggingState_SetLevel(gLoggingState, newLevel);
    RELEASE_LOCK(gLoggingStateLock);

    return result;
}


//-----------------------------------------------------------------------------
// IsLoggingStarted()
//   Return a boolean indicating if logging is currently started.
//-----------------------------------------------------------------------------
CX_LOGGING_API(int) IsLoggingStarted(void)
{
    return (gLoggingState != NULL);
}
