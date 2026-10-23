#include "S_OS.h"
#include "S_String.h"
#include "S_StringArray.h"


int
S_StringArray::Init()
{
  maxLines = 0;
  numLines = 0;
  lines = NULL;
  pos = NULL;
  return 0;
}


S_StringArray::S_StringArray()
{
  Init();
}


int
S_StringArray::Create()
{
  Destroy();
  return 0;
}


int
S_StringArray::Destroy()
{
  if ( lines )
    {
      for ( int i=0; i<numLines; i++ )
	{
	  if ( lines[i] )
	    {
	      S_free(lines[i]);
	      lines[i] = NULL;
	    }
	}
      S_free(lines);
      lines = NULL;
      numLines = 0;
    }
  if ( pos )
    {
      S_free(pos);
      pos = NULL;
    }
  Init();
  return 0;
}


S_StringArray::~S_StringArray()
{
  Destroy();
}


int
S_StringArray::AppendLine(const char *fmt,...)
{

  S_String line;

  va_list v;
  va_start(v,fmt);
  line.VCreate(fmt,v);
  va_end(v);
 
  InsertLine(line.str,numLines);
  return 0;
}


int
S_StringArray::AppendLine(char *line)
{
  InsertLine(line,numLines);
  return 0;
}


int
S_StringArray::InsertLine(char *line,int index)
{

  if ( index >= 0 )
    {

      if ( index > numLines )
	{
	  index = numLines;
	}

      if ( index >= maxLines )
	{
	  if ( maxLines == 0 )
	    {
	      lines = (char **)S_malloc(sizeof(char *));
	      pos = (int *)S_malloc(sizeof(int));
	    }
	  else
	    {
	      lines = (char **)S_realloc(lines,sizeof(char *)*(index+1));
	      pos = (int *)S_realloc(pos,sizeof(int)*(index+1));
	    }
	  maxLines = index+1;
        }

      if ( pos )
	{
	  pos[index] = 0;
	}
      if ( lines )
	{
	  lines[index] = S_strdup(line);
	  numLines++;
        }

    }
   
  return 0;
}


int
S_StringArray::SetLine(char *line,int index)
{
  if ( line && index >= 0 && index < numLines )
    {
      if ( lines[index] )
	{
	  S_free(lines[index]);
	  lines[index] = S_strdup(line);
	}
    }
  return 0;
}


char *
S_StringArray::GetLine(int index)
{
  if ( lines && index >= 0 && index < numLines )
    {
      return lines[index];
    }
  return NULL;
}


int
S_StringArray::RemoveLine(int index)
{
  if ( index >= 0 && index < numLines )
    {
      S_free(lines[index]);
      for ( int i=index; i<numLines-1; i++ )
	{
	  lines[i] = lines[i+1];
	  pos[i] = pos[i+1];
	}
      numLines--;
    }
  return 0;
}


int
S_StringArray::HasString(char *istr,int caseSensitive)
{
  for ( int i=0; i<numLines; i++ )
    {
      if ( !caseSensitive && !strcmp(lines[i],istr) )
	{
	  return TRUE;
	}
      if ( caseSensitive && !stricmp(lines[i],istr) )
	{
	  return TRUE;
	}
    }
  return FALSE;
}


int
S_StringArray::HasSubString(char *istr,int caseSensitive)
{
  for ( int i=0; i<numLines; i++ )
    {
      if ( caseSensitive && strstr(lines[i],istr) )
	{
	  return TRUE;
	}
      if ( !caseSensitive && stristr(lines[i],istr) )
	{
	  return TRUE;
	}
    }
  return FALSE;
}


int
S_StringArray::RemoveCharacter(int i,char ch)
{
  if ( i >= 0 && i < numLines && lines[i] )
    {
      int len = strlen(lines[i]);
      for ( int j=0; j<len; j++ )
	{
	  if ( lines[i][j] == ch )
	    {
	      for ( int k=j; k<len; k++ )
		{
		  lines[i][k] = lines[i][k+1];
		}
	    }
	}
    }
  return 0;
}


int
S_StringArray::RemoveCharacter(char ch)
{
  for ( int i=0; i<numLines; i++ )
    {
      RemoveCharacter(i,ch);
    }
  return 0;
}


int
S_StringArray::ReplaceCharacter(char ch,char ch2)
{
  for ( int i=0; i<numLines; i++ )
    {
      if ( lines[i] )
	{
	  int len = strlen(lines[i]);
	  for ( int j=0; j<len; j++ )
	    {
	      if ( lines[i][j] == ch )
		{
		  lines[i][j] = ch2;
		}
	    }
	}
    }
  return 0;
}


int
S_StringArray::GetIndex(char *istr,int caseSensitive)
{
  if ( istr )
    {
      for ( int i=0; i<numLines; i++ )
	{
	  if ( caseSensitive && strstr(lines[i],istr) )
	    {
	      return i;
	    }
	  if ( !caseSensitive && stristr(lines[i],istr) )
	    {
	      return i;
	    }
	}
    }
  return -1;
}
