#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define CLASS_NAME        0
#define CLASS_SUBCLASS    1

#define MAX_STRING_SIZE 1024
#define MAX_SUBCLASSES 1024

int numSubClasses = 0;
char name[MAX_STRING_SIZE];
char subClasses[MAX_SUBCLASSES][MAX_STRING_SIZE];

int CreateHeaderFile();
int CreateClassFile();


int
main(int argc,char *argv[])
{

  int i;

  name[0] = 0;
  for ( i=0; i<MAX_SUBCLASSES; i++ )
    subClasses[i][0] = 0;

  i = 1;
  int mode = CLASS_NAME;

  while ( i < argc ) {
    if ( !strcmp(argv[i],"-name") )
      mode = CLASS_NAME;
    else if ( !strcmp(argv[i],"-subclass") )
      mode = CLASS_SUBCLASS;
    else
      {
	switch(mode)
	  {
	  case CLASS_NAME :
	    strcpy(name,argv[i]);
	    break;
	  case CLASS_SUBCLASS :
	    strcpy(subClasses[numSubClasses++],argv[i]);
	    break;
	  }
      }
    i++;
  }

  if ( !name[0] )
    {
      printf("Usage: makeclass -name [name] -subclass [subclass list ...]\n");
      return 0;
    }

  CreateHeaderFile();
  CreateClassFile();

  return 0;
}


int
CreateHeaderFile()
{

  int i;

  char NAME[MAX_STRING_SIZE];
  strcpy(NAME,name);
  for ( i=0; i<(int)strlen(NAME); i++ )
    NAME[i] = toupper(NAME[i]);

  char headerFilename[MAX_STRING_SIZE];
  sprintf(headerFilename,"%s.h",name);

  FILE *fptr = fopen(headerFilename,"w");
  if ( fptr )
    {

      fprintf(fptr,"#ifndef %s_H_\n",NAME);
      fprintf(fptr,"#define %s_H_\n",NAME);
      fprintf(fptr,"\n");

      for ( i=0; i<numSubClasses; i++ )
	fprintf(fptr,"#include \"%s.h\"\n",subClasses[i]);
      fprintf(fptr,"\n");

      fprintf(fptr,"class %s ",name);

      if ( numSubClasses > 0 )
	{
	  fprintf(fptr,": ");
	  for ( i=0; i<numSubClasses; i++ )
	    {
	      if ( i > 0 )
		fprintf(fptr,", ");
	      fprintf(fptr,"public %s ",subClasses[i]);
            }
	}

      fprintf(fptr,"\n{\n");
      fprintf(fptr," public:\n");
      fprintf(fptr,"\n");

      fprintf(fptr,"  int Init();\n");
      fprintf(fptr,"  int Create();\n");
      fprintf(fptr,"  int Destroy();\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"  %s();\n",name);
      fprintf(fptr,"  virtual ~%s();\n",name);

      fprintf(fptr,"};\n");
      fprintf(fptr,"\n");

      fprintf(fptr,"#endif // %s_H_\n",NAME);

      fclose(fptr);
    }

  return 0;
}


int
CreateClassFile()
{

  int i;

  char classFilename[MAX_STRING_SIZE];
  sprintf(classFilename,"%s.cpp",name);

  FILE *fptr = fopen(classFilename,"w");
  if ( fptr )
    {

      fprintf(fptr,"#include \"%s.h\"\n",name);
      fprintf(fptr,"\n");
      fprintf(fptr,"\n");

      fprintf(fptr,"%s::%s()\n",name,name);
      fprintf(fptr,"{\n");
      fprintf(fptr,"  Init(); // Call at top of all constructors\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"  Create(); // Call Create() method matching params\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"}\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"\n");

      fprintf(fptr,"%s::~%s()\n",name,name);
      fprintf(fptr,"{\n");
      fprintf(fptr,"  Destroy();  "
                   "// All destructors consist only of call to Destroy()\n");
      fprintf(fptr,"}\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"\n");

      fprintf(fptr,"int\n");
      fprintf(fptr,"%s::Init()\n",name);
      fprintf(fptr,"{\n");
      fprintf(fptr,"  // Init member variables\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"  return 0;\n");
      fprintf(fptr,"}\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"\n");

      fprintf(fptr,"int\n");
      fprintf(fptr,"%s::Create()\n",name);
      fprintf(fptr,"{\n");
      fprintf(fptr,"  Destroy(); // Call at top of all Create() methods\n");
      fprintf(fptr,"\n");

      fprintf(fptr,"  // Call subclass and member Create() methods\n");
      for ( i=0; i<numSubClasses; i++ )
	fprintf(fptr,"  %s::Create();\n",subClasses[i]);

      fprintf(fptr,"\n");
      fprintf(fptr,"  // Allocate member variable data\n");

      fprintf(fptr,"\n");
      fprintf(fptr,"  return 0;\n");
      fprintf(fptr,"}\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"\n");

      fprintf(fptr,"int\n");
      fprintf(fptr,"%s::Destroy()\n",name);
      fprintf(fptr,"{\n");
      fprintf(fptr,"  // Free member variable data\n");

      fprintf(fptr,"\n");
      fprintf(fptr,"  Init(); // Call at bottom of all Destroy() methods\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"  return 0;\n");
      fprintf(fptr,"}\n");
      fprintf(fptr,"\n");
      fprintf(fptr,"\n");

      fclose(fptr);
    }

  return 0;
}
