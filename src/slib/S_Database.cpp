#include "S_OS.h"
#include "S_Database.h"
#include "S_TokenArray.h"


int
S_Database::Init()
{
  
  // Init member variables
  
  return 0;
}


S_Database::S_Database()
{
  Init(); // Call at top of all constructors
  
  Create(); // Call Create() method matching params
  
}


int
S_Database::Create()
{
  Destroy(); // Call at top of all Create() methods
  
  // Call subclass and member Create() methods
  S_DBRecord::Create("S_Database");
  
  // Allocate member variable data
  
  return 0;
}


int
S_Database::Create(char *ifilename)
{
  Destroy(); // Call at top of all Create() methods
  
  // Call subclass and member Create() methods
  filename.Create(ifilename);
  
  // Allocate member variable data
  
  Load();
  
  return 0;
}


int
S_Database::Destroy()
{
  
  Save();
  
  // Free member variable data
  DeleteChildren();
  
  Init(); // Call at bottom of all Destroy() methods
  return 0;
}


S_Database::~S_Database()
{
  Destroy();  // All destructors consist only of call to Destroy()
}


int
S_Database::Load()
{
  if ( filename.str ) {
    FILE *fptr = fopen(filename.str,"r");
    if ( fptr )
      {
	char line[1024];
	while ( fgets(line,1023,fptr) )
	  {
            S_TokenArray tokens(line);
            tokens.RemoveCharacter(10);
            tokens.RemoveCharacter(13);
            if ( tokens.numLines == 2 && 
                 !strcmp("S_DBRecord",tokens.lines[0]) &&
                 !strcmp("{",tokens.lines[1]) )
	      Read(fptr);
	  }
	fclose(fptr);
      }
  }
  return 0;
}


int
S_Database::Save()
{
  if ( filename.str )
    {
      FILE *fptr = fopen(filename.str,"w");
      if ( fptr )
	{
	  Write(fptr);
	  fclose(fptr);
	}
    }
  return 0;
}
