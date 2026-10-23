#include "S_OS.h"
#include "S_Record.h"


S_Record::S_Record(char *ifilename,int imaxEntries)
{
  name = NULL;
  if ( ifilename )
    name = S_strdup(ifilename);
  numEntries = 0;
  maxEntries = imaxEntries;
  entries = NULL;
  if ( imaxEntries )
    {
      entries = (S_RecordEntry *)S_malloc(sizeof(S_RecordEntry)*maxEntries);
    }
}


int
S_Record::Exists()
{
  return _OsFileExists(name);
}


int
S_Record_IsWhiteSpace(char ch)
{
  return ch == ' ' || ch == '\t';
}


int
S_Record_HasWhiteSpace(char *str)
{
  for ( uint i=0; i<strlen(str); i++ )
    if ( S_Record_IsWhiteSpace(str[i]) )
      return TRUE;
  return FALSE;
}


int
S_Record_RemoveCharacter(char *str,char ch)
{
  int len = strlen(str);
  for ( int i=0; i<len; i++ )
    if ( str[i] == ch )
      for ( int j=i; j<len; j++ )
	str[j] = str[j+1];
  return 0;
}


int
S_Record::New()
{
  FILE *fptr = fopen(name,"w");
  if ( fptr )
    fclose(fptr);
  return 0;
}


int
S_Record::Open()
{
  int i,j;
  FILE *fptr = fopen(name,"r");
  if ( fptr )
    {
      numEntries = 0;
      char line[S_RECORD_STRSIZE];
      while ( fgets(line,S_RECORD_STRSIZE-1,fptr) != NULL )
	{
	  int inQuotes = FALSE;
	  S_Record_RemoveCharacter(line,'\n');
	  S_Record_RemoveCharacter(line,10);
	  S_Record_RemoveCharacter(line,13);
	  i = 0;
	  if ( line[0] != 0 )
	    {
	      while ( line[i] != '"' && S_Record_IsWhiteSpace(line[i]) ) i++;
	      if ( line[i] == '"' )
		inQuotes = TRUE;
	      j = i;
	      while ( !S_Record_IsWhiteSpace(line[++j]) || inQuotes )
		{
		  if ( line[j] == '"' )
		    inQuotes = FALSE;
		}
	      
	      if ( !entries || numEntries >= maxEntries )
		{
		  if ( !entries || maxEntries == 0 )
		    entries = (S_RecordEntry *)S_malloc(sizeof(S_RecordEntry) *
							(maxEntries+1));
		  else
		    entries = (S_RecordEntry *)S_realloc(entries,
							 sizeof(S_RecordEntry) *
							 (maxEntries+1));
		  maxEntries++;
		}
	      
	      /* *** Copy data from line into new string element *** */
	      int k;
	      for ( k=0; k<j-i; k++ )
		entries[numEntries].keyword[k] = line[k+i];
	      entries[numEntries].keyword[k] = 0;
	      
	      i = j;
	      while ( S_Record_IsWhiteSpace(line[i]) ) i++;
	      strcpy(entries[numEntries].value,&line[i]);
	      
	      S_Record_RemoveCharacter(entries[numEntries].keyword,'"');
	      S_Record_RemoveCharacter(entries[numEntries].value,'"');
	      
	      numEntries++;
            }
	}
      fclose(fptr);
    }
  return 0;
}


char *S_Record_Blank = "";

char *
S_Record::GetEntry(char *ikeyword)
{
  for ( int i=0; i<numEntries; i++ )
    if ( strcmp(ikeyword,entries[i].keyword) == 0 )
      return entries[i].value;
  return S_Record_Blank;
}


int 
S_Record::GetIndex(char *ikeyword)
{
  for ( int i=0; i<numEntries; i++ )
    if ( strcmp(ikeyword,entries[i].keyword) == 0 )
      return i;
  return -1;
}


int
S_Record::SetEntry(char *ikeyword,char *ivalue)
{
  if ( ikeyword && ivalue )
    {
      S_Record_RemoveCharacter(ikeyword,'"');   
      S_Record_RemoveCharacter(ivalue,'"');   
      for ( int i=0; i<numEntries; i++ )
	{
	  if ( strcmp(ikeyword,entries[i].keyword) == 0 )
	    {
	      strcpy(entries[i].value,ivalue);
	    }
	}
    }
  return 0;
}


int
S_Record::InsertEntry(int i,char *ikeyword,char *ivalue)
{
  if ( ikeyword && ivalue )
    {
      S_Record_RemoveCharacter(ikeyword,'"');   
      S_Record_RemoveCharacter(ivalue,'"');   
      numEntries++;
      if ( numEntries > maxEntries )
	{
	  maxEntries = numEntries;
	  entries = (S_RecordEntry *)S_realloc(entries,
					       sizeof(S_RecordEntry)*
					       maxEntries);
	}
      for ( int j=numEntries-1; j>i; j-- )
	{
	  strcpy(entries[j].keyword,entries[j-1].keyword);
	  strcpy(entries[j].value,entries[j-1].value);
	}
      strcpy(entries[i].keyword,ikeyword);
      strcpy(entries[i].value,ivalue);
    }
  return 0;
}


int
S_Record::RemoveEntry(int i)
{
  for ( int j=i; j<numEntries-1; j++ )
    {
      strcpy(entries[j].keyword,entries[j+1].keyword);
      strcpy(entries[j].value,entries[j+1].value);
    }
  numEntries--;
  return 0;
}


int
S_Record::AddEntry(char *ikeyword,char *ivalue)
{
  if ( !ikeyword || !ivalue ) return 0;
  S_Record_RemoveCharacter(ikeyword,'"');   
  S_Record_RemoveCharacter(ivalue,'"');   
  if ( numEntries+1 > maxEntries )
    maxEntries = numEntries+1;
  if ( maxEntries == 0 )
    entries = (S_RecordEntry *)S_malloc(sizeof(S_RecordEntry) *
					maxEntries);
  else
    entries = (S_RecordEntry *)S_realloc(entries,
					 sizeof(S_RecordEntry) *
					 maxEntries);
  strcpy(entries[numEntries].keyword,ikeyword);
  strcpy(entries[numEntries].value,ivalue);
  numEntries++; 
  return 0;
}


int
S_Record::Close()
{
  int i;
  FILE *fptr = fopen(name,"w");
  if ( fptr )
    {
      for ( i=0; i<numEntries; i++ )
	{
	  if ( S_Record_HasWhiteSpace(entries[i].keyword) )
            fprintf(fptr,"\"%s\" ",entries[i].keyword);
	  else
            fprintf(fptr,"%s ",entries[i].keyword);
	  if ( S_Record_HasWhiteSpace(entries[i].value) )
            fprintf(fptr,"\"%s\"",entries[i].value);
	  else
            fprintf(fptr,"%s",entries[i].value);
	  fprintf(fptr,"\n");
	}
      /*
	if ( entries ) {
	S_free(entries);
	entries = NULL;
	maxEntries = 0;
	}
      */
      fclose(fptr);
      numEntries = 0;
      fptr = NULL;
    }
  return 0;
}


S_Record::~S_Record()
{
  if ( entries )
    {
      S_free(entries);
      entries = NULL;
    }
  if ( name )
    {
      S_free(name);
      name = NULL;
    }
}
