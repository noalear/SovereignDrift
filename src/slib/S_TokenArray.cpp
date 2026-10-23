#include "S_OS.h"
#include "S_TokenArray.h"


int
S_TokenArray::Init()
{
  options = 0;
  delimiter1 = ' ';
  delimiter2 = '\t';
  return 0;
}


S_TokenArray::S_TokenArray(char *line,char ich1,char ich2,int ioptions)
{
  Init();
  if ( line )
    Create(line,ich1,ich2,ioptions);
}


int
S_TokenArray::IsWhiteSpace(char ch)
{
  return ch == delimiter1 || ch == delimiter2;
}


int
S_TokenArray::Create(char *line,char idelimiter1,char idelimiter2,
                     int ioptions)
{
  Destroy();
  options = ioptions;
  delimiter1 = idelimiter1;
  delimiter2 = idelimiter2;
  if ( line )
    {
      int len = strlen(line);
      if ( len > 0 )
	{
	  int i=0;
	  int j=0;
	  int endOfLine = FALSE;
	  
	  int inQuote = FALSE;
	  do
	    {
	      if ( !(options & S_TOKENARRAY_SINGLEDELIMIT) )
		while ( IsWhiteSpace(line[i]) && i<len ) i++;
	      if ( line[i] == '"' && !(options & S_TOKENARRAY_IGNOREQUOTES) )
		{
		  inQuote = TRUE;
		  i++;
		}
	      j = i+1;
	      while ( (inQuote || !IsWhiteSpace(line[j])) && j<len )
		{
		  if ( inQuote && line[j] == '"' )
		    inQuote = FALSE;
		  j++;
		}

	      char ch = 0;
	      if ( options & S_TOKENARRAY_KEEPTRAILINGDELIMITER && j+1 < len )
		{
		  ch = line[j+1];
		  line[j+1] = 0;
		}
	      else
		{
		  ch = line[j];
		  line[j] = 0;
		}

	      
	      if ( options & S_TOKENARRAY_SINGLEDELIMIT )
		{
		  if ( !IsWhiteSpace(line[j]) )
		    {
		      if ( i == 0 )
			{
			  AppendLine(line+i);
			  if ( numLines > 0 )
			    {
			      pos[numLines-1] = i;
			    }
			}
		      else
			{
			  AppendLine(line+i+1);
			  if ( numLines > 0 )
			    {
			      pos[numLines-1] = i+1;
			    }
			}
		    }
		  else
		    {
		      AppendLine("");
		      if ( numLines > 0 )
			{
			  pos[numLines-1] = i;
			}
		    }
		}
	      else
		{
		  AppendLine(line+i);
		  if ( numLines > 0 )
		    {
		      pos[numLines-1] = i;
		    }
		}
	      
	      if ( options & S_TOKENARRAY_KEEPTRAILINGDELIMITER && j+1 < len)
		{
		  line[j+1] = ch;
		  i = j;
		}
	      else
		{
		  line[j] = ch;
		  i = j;
		}

	      endOfLine = (i >= len);
	    } while ( !endOfLine );
	  RemoveCharacter('"');
	}
    }
  return 0;
}


int
S_TokenArray::Destroy()
{
  Init();
  return 0;
}


S_TokenArray::~S_TokenArray()
{
  Destroy();
}
