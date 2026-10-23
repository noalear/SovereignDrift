#include "S_OS.h"
#include "S_Logger.h"

int
S_Logger::Init()
{
  options = S_LOGGER_TOCONSOLE;
  return 0;
}


S_Logger::S_Logger(char *ifilename,int ioptions)
{
  Init();
  if ( ifilename || ioptions )
    Create(ifilename,options);
}


int
S_Logger::Create(char *ifilename,int ioptions)
{
  Destroy();
  options = ioptions;
  filename.Create(ifilename);
  return 0;
}


int
S_Logger::Destroy()
{
  Init();
  return 0;
}

S_Logger::~S_Logger()
{
  Destroy();
}


char *
S_Logger::Log(const char *fmt,...)
{
  
  va_list v;
  va_start(v,fmt);
  VLog(fmt,v);
  va_end(v);
  
  return line.str;
}


char *
S_Logger::VLog(const char *fmt,va_list v)
{
  
  S_String tmp;
  tmp.VCreate(fmt,v);
  
  if ( options && tmp.str )
    {
      
      //
      // Create a timestamp string if selected.
      //
      char timeStamp[25] = {0};
      if ( options & S_LOGGER_TIMESTAMP )
	{
	  time_t timer;
	  timer = time(&timer);
	  strncpy(timeStamp,ctime(&timer),24);
	  timeStamp[24] = 0;
	}
      
      //
      // Create the line of text to be logged.  With timestamp if appropriate.
      //
      if ( timeStamp[0] )
	{
          line.Create("%s: %s",timeStamp,tmp.str);
	}
      else
        {
          line.Create("%s",tmp.str);
        }
      
      //
      // Write log line to stdout if selected.
      //
      if ( options & S_LOGGER_TOCONSOLE )
	{
	  printf("%s\n",line.str);
	}
      
      //
      // Write log line to file if selected.
      //
      if ( options & S_LOGGER_TOFILE && filename.str )
	{
	  mutex.Request();
	  FILE *fptr = fopen(filename.str,"a");
	  if ( fptr )
	    {
	      fprintf(fptr,"%s\n",line.str);
	      fclose(fptr);
	    }
	  mutex.Release();
	}
      
    }
  
  return line.str;
}
