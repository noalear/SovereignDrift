#include "S_OS.h"
#include "S_DBRecord.h"
#include "S_TokenArray.h"


int
S_DBRecord::Init()
{
  
  // Init member variables
  
  return 0;
}


S_DBRecord::S_DBRecord()
{
  Init(); // Call at top of all constructors
  
  Create(); // Call Create() method matching params
  
}


int
S_DBRecord::Create(char *name)
{
  Destroy(); // Call at top of all Create() methods
  
  // Call subclass and member Create() methods
  S_Tree::Create(name);
  S_StringArray::Create();
  
  // Allocate member variable data
  
  return 0;
}


int
S_DBRecord::Destroy()
{
  
  // Free member variable data
  
  Init(); // Call at bottom of all Destroy() methods
  return 0;
}


S_DBRecord::~S_DBRecord() {
   Destroy();  // All destructors consist only of call to Destroy()
   }


int
S_DBRecord::Write(FILE *fptr)
{
  
  int j;
  for ( j=0; j<level; j++ )
    fprintf(fptr,"   ");
  fprintf(fptr,"S_DBRecord {\n");
  for ( int i=0; i<numLines; i++ )
    {
      for ( j=0; j<level; j++ )
	fprintf(fptr,"   ");
      fprintf(fptr,"   %s\n",lines[i]);
    }
  
  for ( S_Tree *tmp=child; tmp!=NULL; tmp=tmp->next )
    ((S_DBRecord *)tmp)->Write(fptr);
  
  for ( j=0; j<level; j++ )
    fprintf(fptr,"   ");
  fprintf(fptr,"   }\n");
  
  return 0;
}


int
S_DBRecord::Read(FILE *fptr)
{
  int keepReading = TRUE;
  char line[1024];
  do
    {
      
      keepReading = (int)fgets(line,1023,fptr);
      if ( keepReading )
	{
	  S_TokenArray tokens(line);
	  tokens.RemoveCharacter(10);
	  tokens.RemoveCharacter(13);
	  
	  if ( tokens.numLines == 2 )
	    {
	      if ( !strcmp("S_DBRecord",tokens.lines[0]) &&
		   !strcmp("{",tokens.lines[1]) )
		{
		  S_DBRecord *record = new S_DBRecord();
		  record->Read(fptr);
		  AddChild(record);
		}
	      else
		{
		  AppendLine("%s %s",tokens.lines[0],tokens.lines[1]);
		}
	    }
	  
	  if ( tokens.numLines == 1 && !strcmp("}",tokens.lines[0]) )
            keepReading = FALSE;
	}
      
    } while ( keepReading );
  
  return 0;
}



S_DBRecord *
S_DBRecord::GetRecordNamed(char *iname)
{
  for ( S_Tree *tmp=child; tmp!=NULL; tmp=tmp->next )
    {
      S_DBRecord *record = (S_DBRecord *)tmp;
      for ( int i=0; i<record->numLines; i++ )
	{
	  S_TokenArray tokens(record->lines[i]);
	  if ( !strcmp("Name",tokens.lines[0]) &&
	       !strcmp(iname,tokens.lines[1]) )
            return record;
	}
    }
  return NULL;
}


char *
S_DBRecord::GetKeywordValue(char *ikeyword)
{
  if ( ikeyword )
    {
      for ( int i=0; i<numLines; i++ )
	{
	  S_TokenArray tokens(lines[i]);
	  if ( tokens.numLines == 2 )
	    {
	      if ( !strcmp(tokens.lines[0],ikeyword) )
		return tokens.lines[1];
            }
	}
    }
  return NULL;
}
