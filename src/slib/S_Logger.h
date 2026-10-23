#ifndef S_LOGGER
#define S_LOGGER

#define S_LOGGER_TOFILE    1
#define S_LOGGER_TOCONSOLE 2
#define S_LOGGER_TIMESTAMP 4

#include "S_Mutex.h"
#include "S_String.h"

class S_Logger
{
 public:
  int options;
  S_String filename;
  
  S_String line;
  S_Mutex mutex;
  
  char *Log(const char *,...);
  char *VLog(const char *,va_list);
  
  int Init();
  S_Logger(char * =NULL,int=S_LOGGER_TOCONSOLE|S_LOGGER_TIMESTAMP);
  int Create(char * =NULL,int=S_LOGGER_TOCONSOLE|S_LOGGER_TIMESTAMP);
  int Destroy();
  ~S_Logger();
};

#endif
