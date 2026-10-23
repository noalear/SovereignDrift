#include "S_OS.h"
#include "S_String.h"
#include "S_DebugMem.h"


int
S_String::Init()
{
  str = NULL;
  len = 0;
  maxlen = 0;
  return 0;
}


S_String::S_String()
{
  Init();
}


S_String::S_String(char *istr)
{
  Init();
  Create(istr);
}


S_String::S_String(int ilen)
{
  Init();
  Create(ilen);
}


S_String::S_String(const char *fmt,...)
{
  Init();

  va_list v;
  va_start(v,fmt);
  VCreate(fmt,v);
  va_end(v);
}


int
S_String::Create(char *istr)
{
  Destroy();
  if ( istr )
    {
      str = (char *)malloc(strlen(istr)+1);
      if ( str )
	{
	  strcpy(str,istr);
	  len = maxlen = strlen(str);
	}
    }
  return 0;
}


int
S_String::Create(int imaxlen)
{
  Destroy();
  if ( imaxlen )
    {
      maxlen = imaxlen;
      str = (char *)S_malloc(maxlen+1);
      if ( str )
	{
	  for ( int i=0; i<maxlen; i++ )
	    {
	      str[i] = ' ';
	    }
	  SetLen(maxlen);
	}
    }
  return 0;
}


int
S_String::Create(const char *fmt,...)
{
  Destroy();

  va_list v;
  va_start(v,fmt);
  VCreate(fmt,v);
  va_end(v);

  return 0;
}


int
S_String::VCreate(const char *fmt,va_list v)
{
  Destroy();

  maxlen = -1;

  //
  // Find out how large of a string we are going to need.
  //
  // BoundsChecker is unhappy with this
  //
  FILE *nul = fopen(NULL_DEV,"w");
  if ( nul )
    {
      maxlen = vfprintf(nul,fmt,v);
      fclose(nul);
    }

  //
  // This is unsafe, but I'll use it anyway.
  //
  //char tmp[4096];
  //maxlen = vsprintf(tmp,fmt,v);

  if ( maxlen >= 0 )
    {
      str = (char *)S_malloc(maxlen+1);
      if ( str )
	{
          vsprintf(str,fmt,v);
	}
    }

  len = maxlen;

  return 0;
}


int
S_String::Destroy()
{
  if ( str )
    {
      S_free(str);
    }
  Init();
  return 0;
}


S_String::~S_String()
{
  Destroy();
}


int
S_String::Append(const char *fmt,...)
{

  S_String old(str);

  va_list v;
  va_start(v,fmt);
  VCreate(fmt,v);
  va_end(v);

  S_String append(str);

  Create("%s%s",old.str,append.str);

  return 0;
}

int
S_String::Prepend(const char *fmt,...)
{

  S_String old(str);

  va_list v;
  va_start(v,fmt);
  VCreate(fmt,v);
  va_end(v);

  S_String prepend(str);

  Create("%s%s",prepend.str,old.str);

  return 0;
}


int
S_String::HasCharacter(char ch) {
  if ( str )
    {
      for ( int i=0; i<len; i++ )
	{
	  if ( str[i] == ch )
	    {
	      return TRUE;
	    }
	}
    }
  return FALSE;
}


int
S_String::ReplaceCharacter(char ch,char *sch) {
  S_String old(str);
  str[0] = 0;
  for ( int j=0; j<old.len; j++ )
    {
      S_String tmp(str);
      if ( old.str[j] == ch )
	{
	  Create("%s%s",tmp.str,sch);
	}
      else
	{
	  Create("%s%c",tmp.str,old.str[j]);
	}
    }
  return 0;
}


int
S_String::RemoveCharacter(char ch)
{
  if ( str )
    {
      for ( int j=0; j<len; j++ )
	{
	  if ( str[j] == ch )
	    {
	      for ( int k=j; k<len; k++ )
		{
		  str[k] = str[k+1];
		}
	    }
	}
      len = strlen(str);
    }
  return 0;
}


int
S_String::ReplaceCharacter(char ch,char sch) {
  for ( int j=0; j<len; j++ )
    {
      if ( str[j] == ch )
	{
	  str[j] = sch;
	}
    }
  return 0;
}


int
S_String::ReplaceLastOccurence(char ch,char sch) {
  for ( int j=len; j>=0; j-- )
    {
      if ( str[j] == ch )
	{
	  str[j] = sch;
	  return 0;
	}
    }
  return 0;
}


int
S_String::SetLen(int ilen)
{
  if ( str && len >=0 && len <= maxlen )
    {
      len = ilen;
      str[len] = 0;
    }
  return 0;
}


int
S_String::AllUpperCase() {
  for ( int j=0; j<len; j++ )
    {
      str[j] = toupper(str[j]);
    }
  return 0;
}
  

int
S_String::AllLowerCase() {
  for ( int j=0; j<len; j++ )
    {
      str[j] = tolower(str[j]);
    }
  return 0;
}
  


