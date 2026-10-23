#include "S_Lib.h"

char *thisFile;
int PrintUsage();

int quiet = FALSE;
int staticFile = FALSE;

#ifdef OS2
#  define MAX_OBJS_PERLINE 24
#else
#  define MAX_OBJS_PERLINE 96
#endif

/* ********************************** */
/* *** Environment Variables      *** */
/* ********************************** */
char *emptyString = "";

char *S_COMPILER          = emptyString;
char *S_PLATFORM          = emptyString;

char *S_CC                = emptyString;
char *S_CPP               = emptyString;
char *S_CC_OPT            = emptyString;
char *S_CC_OUTPUTFILE     = emptyString;
char *S_LINK              = emptyString;
char *S_LINK_OPT          = emptyString;
char *S_LINK_LIBS         = emptyString;
char *S_LINK_CONSOLEAPP   = emptyString;
char *S_LINK_GUIAPP       = emptyString;
char *S_LINK_CRTENTRY     = emptyString;
char *S_LINK_OUTPUTFILE   = emptyString;
char *S_LINK_BINDRES      = emptyString;
char *S_LINK_EXEEXT       = emptyString;
char *S_LIB               = emptyString;
char *S_LIB_OUTPUTFILE    = emptyString;
char *S_LIB_OPT           = emptyString;
char *S_LIB_EXT           = emptyString;
char *S_LIB_SEPARATOR     = emptyString;
char *S_LIB_OBJEXT        = emptyString;
char *S_LIB_EOL           = emptyString;
char *S_LIB_SOCKET        = emptyString;
char *S_MAKE              = emptyString;
char *S_MAKE_OPT          = emptyString;
char *S_RC                = emptyString;
char *S_RC_OPT            = emptyString;
char *S_RCBIND            = emptyString;
char *S_RCBIND_ALL        = emptyString;
char *S_RCBIND_OPT        = emptyString;
char *S_RCBIND_EXEEXT     = emptyString;
char *S_RCBIND_CONSOLEAPP = emptyString;
char *S_RCBIND_GUIAPP     = emptyString;
char *S_RCBIND_RESFILE    = emptyString;
char *S_OBJ_EXT           = emptyString;
char *S_EXE_EXT           = emptyString;
/* ********************************** */


char *CreateOutputBaseFilename(char *);
char numInputFiles;
char **inputFiles;
char *targetType;
char *outputFile;
char *exe;

char *S_GetEnv(char *);
int ParseCommandLine(int,char *[]);
int ParseEnvironment();

int GenerateStaticLibMakefile(FILE *);
int GenerateDynamicLibMakefile(FILE *);
int GenerateExecutableMakefile(FILE *,char *);


int
main(int argc, char *argv[]) {

   if ( argc == 1 ) {
      PrintUsage();
      exit(0);
      }

   ParseCommandLine(argc,argv);
   ParseEnvironment();

   if ( !quiet ) {
      printf("\n");
      printf("Input Files:");
      for ( int i=0; i<numInputFiles; i++ )
         printf(" %s",inputFiles[i]);
      printf("\n");
      printf("Platform Name: %s\n",S_PLATFORM);
      printf("Compiler Name: %s\n",S_COMPILER);
      printf("Target Type: %s\n",targetType);
      printf("Output Filename: %s\n",outputFile);
      }

   char *makefile = (char *)S_malloc(strlen("makefile")+
                                   strlen(S_PLATFORM)+
                                   strlen(S_COMPILER)+
                                   3);
   sprintf(makefile,"makefile_%s_%s",S_PLATFORM,S_COMPILER);
   FILE *fptr = fopen(makefile,"w");
   S_free(makefile);

   if ( strcmp(targetType,"StaticLib") == 0 )
      GenerateStaticLibMakefile(fptr);
   /*
   else if ( strcmp(targetType,"DynamicLib") == 0 )
     GenerateDynamicLibMakefile(fptr);
     */
   else if ( strcmp(targetType,"Executable") == 0 )
      GenerateExecutableMakefile(fptr,S_LINK_CRTENTRY);

   fclose(fptr);

   return 0;

   }


int
PrintUsage() {
   printf("Usage:\n");
   printf("\n");
   printf("makemake -q                   (Quiet)              \n");
   printf("         -i [input files]     (*.cpp *.c)          \n");
   printf("         -t [target type]     (StaticLib,DynamicLib,Executable)\n");
   printf("         -o [output filename]\n");
   return 0;
   }


int
FileHas(char *filename,char *word)
{
  FILE *fptr = fopen(filename,"r");
  if ( fptr )
    {
      char line[256];
      while ( fgets(line,255,fptr) != NULL )
	{
	  S_TokenArray tokens(line,' ','(',S_TOKENARRAY_KEEPTRAILINGDELIMITER);
	  for ( int i=0; i<tokens.numLines; i++ )
	    {
	      if ( strstr(tokens.lines[i],word) == tokens.lines[i] )
		{
		  fclose(fptr);
		  return TRUE;
		}
	    }
	}
      fclose(fptr);
    }
   return FALSE;
   }


char *S_GetEnv(char *varname) {
   char *str = getenv(varname);
   if ( str ) {
      if ( !quiet )
         printf("%s = %s\n",varname,str);
      return str;
      }
      if ( !quiet )
         printf("%s = [Undefined]\n",varname);
   return emptyString;
   }


int
ParseEnvironment() {
   if ( !quiet )
      printf("Parsing Environment...\n");
   S_LINK_BINDRES    = S_GetEnv("S_LINK_BINDRES");
   S_LIB_EXT         = S_GetEnv("S_LIB_EXT");
   S_LIB_OBJEXT      = S_GetEnv("S_LIB_OBJEXT"); 
   S_LIB_SEPARATOR   = S_GetEnv("S_LIB_SEPARATOR");
   S_LIB_EOL         = S_GetEnv("S_LIB_EOL");
   S_OBJ_EXT         = S_GetEnv("S_OBJ_EXT");
   S_RCBIND          = S_GetEnv("S_RCBIND");
   S_RCBIND_OPT      = S_GetEnv("S_RCBIND_OPT");
   S_RCBIND_RESFILE  = S_GetEnv("S_RCBIND_RESFILE");
   S_RCBIND_EXEEXT   = S_GetEnv("S_RCBIND_EXEEXT");
   S_RCBIND_ALL      = S_GetEnv("S_RCBIND_ALL");
   S_PLATFORM        = S_GetEnv("S_PLATFORM");
   S_COMPILER        = S_GetEnv("S_COMPILER");
   S_LIB_OUTPUTFILE  = S_GetEnv("S_LIB_OUTPUTFILE");
   S_LIB             = S_GetEnv("S_LIB");
   S_LIB_OPT         = S_GetEnv("S_LIB_OPT");
   S_RC              = S_GetEnv("S_RC");
   S_RC_OPT          = S_GetEnv("S_RC_OPT");
   S_CC              = S_GetEnv("S_CC");
   S_CPP             = S_GetEnv("S_CPP");
   S_CC_OPT          = S_GetEnv("S_CC_OPT");
   S_CC_OUTPUTFILE   = S_GetEnv("S_CC_OUTPUTFILE");
   S_LINK            = S_GetEnv("S_LINK");
   S_LINK_OPT        = S_GetEnv("S_LINK_OPT");
   S_LINK_LIBS       = S_GetEnv("S_LINK_LIBS");
   S_LINK_EXEEXT     = S_GetEnv("S_LINK_EXEEXT");
   S_LINK_OUTPUTFILE = S_GetEnv("S_LINK_OUTPUTFILE");
   S_LINK_CONSOLEAPP = S_GetEnv("S_LINK_CONSOLEAPP");
   S_LINK_GUIAPP     = S_GetEnv("S_LINK_GUIAPP");
   S_LINK_CRTENTRY   = S_GetEnv("S_LINK_CRTENTRY");
   return 0;
   }


int
ParseCommandLine(int argc,char *argv[] ) {

   thisFile = S_strdup(argv[0]);

   int i = 0;
   do {
      if ( strcmp(argv[i],"-i") == 0 ) {
         numInputFiles = 0;
         while ( argv[++i][0] != '-' ) {
            if ( numInputFiles == 0 )
               inputFiles = (char **)S_malloc(sizeof(char *));
            else
               inputFiles = (char **)S_realloc(inputFiles,
                                             sizeof(char *)*(numInputFiles+1));
            inputFiles[numInputFiles] = S_strdup(argv[i]);
            numInputFiles++;
            }
         i--;
         }
      else if ( strcmp(argv[i],"-t") == 0 )
         targetType = S_strdup(argv[++i]);
      else if ( strcmp(argv[i],"-o") == 0 )
         outputFile = S_strdup(argv[++i]);
      else if ( strcmp(argv[i],"-q") == 0 )
         quiet = TRUE;
      else if ( strcmp(argv[i],"-s") == 0 )
         staticFile = TRUE;
      } while ( ++i < argc );

   switch(_OsType()) {
      case S_OS_WIN95 :
      case S_OS_WINNT :
         S_PLATFORM = S_strdup("WIN32");
         break;
      case S_OS_OS2 :
         S_PLATFORM = S_strdup("OS2");
         break;
      case S_OS_LINUX :
         S_PLATFORM = S_strdup("LINUX");
         break;
      }

   exe = S_strdup("");
   if ( strcmp(S_PLATFORM,"WIN32") == 0 || strcmp(S_PLATFORM,"OS2") == 0 )
      exe = S_strdup(".exe");

   return 0;
   }


char *CreateBaseFilename(char *filename) {
   char *baseFilename = S_strdup(filename);
   int i = strlen(baseFilename);
   while ( baseFilename[i] != '.' && i >= 0 ) i--;
   if ( i > 0 )
      baseFilename[i] = 0;
   return baseFilename;
   }

char *GetExtension(char *filename) {
   int i = strlen(filename);
   while ( filename[i] != '.' && i >= 0 ) i--;
   return &filename[i];
   }

#if defined(WIN32) || defined(OS2)
   char separator = '\\';
   char *exclude = "/";
#endif
#ifdef UNIX
   char separator = '/';
   char *exclude = "\\";
#endif


int
FindDependencies(char *filename,FILE *ifptr,char *idirname =NULL,
		 char *pfilename ="") {
   char line[256];
   FILE *fptr = fopen(filename,"r");
   if ( !fptr )  {
      printf("Could not open %s in %s %s\n",filename,idirname,pfilename);
      return 0;
      }
   while ( fgets(line,255,fptr) ) {
      if ( line[0] == '#' && strstr(line,"include") && 
           !strstr(line,exclude)  ) {
         int i=0,j=0,len = strlen(line);
         while ( line[i] != '"' && i < len ) i++;
         j = ++i;
         while ( line[j] != '"' && j < len ) j++;
         if ( i < j && i < len && j < len ) {
            char *ifilename = (char *)S_malloc(j - 1 + 1);
            strncpy(ifilename,&line[i],j-i);
            ifilename[j-i] = 0;
            if ( !idirname || (idirname && idirname[0] == 0) )
	      {
		fprintf(ifptr," %s",ifilename);
	      }
            else
	      {
		fprintf(ifptr," %s%c%s",idirname,separator,ifilename);
	      }

            S_Directory currentDir;

            int k;
            char *dirname = S_strdup(ifilename);
            for ( k=strlen(dirname); dirname[k]!=separator&&k>=0; k-- );
            if ( k < 0 ) {
               k = 0;
               dirname[k] = 0;
	       }
            else
               dirname[k++] = 0;
            if ( k > 0 ) {
               S_Directory newDir(dirname);
               newDir.ChangeTo();
	       }
	    if ( dirname[0] == 0 )
	      {
		dirname = idirname;
	      }
            FindDependencies(&ifilename[k],ifptr,dirname,filename);

            currentDir.ChangeTo();
	    }
         }
      }
   fclose(fptr);
   return 0;
   }


char *CreateOutputBaseFilename(char *outputFile) {
   char *str = (char *)S_malloc(strlen(outputFile) +
                              strlen(S_PLATFORM) +
                              strlen(S_COMPILER) +
                              3);
   sprintf(str,"%s_%s_%s",outputFile,S_PLATFORM,S_COMPILER);
   return str;
   }


int
GenerateObjectCodeMakefile(FILE *fptr) {

   if ( !quiet )
      printf("Generating Object Code Section\n");

   int i,j;

   for ( j=0; j<numInputFiles; j++ ) {
      S_DirList dirList(inputFiles[j]);
      for ( i=0; i<dirList.numFiles; i++ ) {
         char *filename = CreateBaseFilename(dirList.file[i]);
         char *ext = GetExtension(dirList.file[i]);
         char *outputBaseFilename = CreateOutputBaseFilename(filename);
	 char *compiler = S_CPP;
	 if ( !strcmp(ext,".c") )
	   {
	     compiler = S_CC;
	   }
         if ( !strstr(filename,thisFile) ) {
            fprintf(fptr,"%s%s : %s%s",outputBaseFilename,S_OBJ_EXT,
                                       filename,ext);
            FindDependencies(dirList.file[i],fptr);
	    if ( staticFile )
	      {
		fprintf(fptr,
			"\n\t%s -c %s %s%s%s %s%s\n\n",
			compiler,S_CC_OPT,S_CC_OUTPUTFILE,
			outputBaseFilename,S_OBJ_EXT,filename,ext);
	      }
	    else
	      {
		fprintf(fptr,
			"\n\t%s -c $(S_CC_OPT) %s%s%s %s%s\n\n",
			compiler,S_CC_OUTPUTFILE,
			outputBaseFilename,S_OBJ_EXT,filename,ext);
	      }
	   }
         S_free(outputBaseFilename);
         }
      }

   return 0;
   }


int
GenerateStaticLibMakefile(FILE *fptr) {

   if ( !quiet )
      printf("Generating Static Lib Section\n");

   int i,j;

   char *outputBaseFilename = CreateOutputBaseFilename(outputFile);

   fprintf(fptr,"all : %s%s\n",outputBaseFilename,S_LIB_EXT);
   fprintf(fptr,"\n");
   fprintf(fptr,"%s%s : ",outputBaseFilename,S_LIB_EXT);

   for ( j=0; j<numInputFiles; j++ ) {

      S_DirList dirList(inputFiles[j]);
      for ( i=0; i<dirList.numFiles; i++ ) {
         char *filename = CreateBaseFilename(dirList.file[i]);
         char *outputBaseFilename = CreateOutputBaseFilename(filename);
         if ( !strstr(filename,thisFile) )
            fprintf(fptr,"%s%s ",outputBaseFilename,S_OBJ_EXT);
         S_free(outputBaseFilename);
         }
      }

   fprintf(fptr,"\n");

   if ( strcmp(targetType,"StaticLib") == 0 ) {
#     if defined(WIN32) || defined(OS2)
         fprintf(fptr,"\tif exist %s%s del %s%s\n",
                 outputBaseFilename,S_LIB_EXT,outputBaseFilename,S_LIB_EXT);
#     endif
#     ifdef UNIX
         fprintf(fptr,"\trm -f %s%s\n",outputBaseFilename,S_LIB_EXT);
#     endif
      int firstTime = TRUE;
      int numObjsOnLine = 0;
      int newLine = TRUE;
      for ( j=0; j<numInputFiles; j++ ) {
	S_DirList dirList(inputFiles[j]);
         for ( i=0; i<dirList.numFiles; i++ ) {

	    if ( numObjsOnLine > MAX_OBJS_PERLINE ) {
               newLine = TRUE;
               numObjsOnLine = 0;
               fprintf(fptr,"%s\n",S_LIB_EOL);
	       }

            char existingLib[1024];
            if ( !firstTime )
               sprintf(existingLib,"%s%s",outputBaseFilename,S_LIB_EXT);
            else
               sprintf(existingLib,"%s","");

            if ( newLine ) {
               if ( strcmp(S_LIB_OUTPUTFILE,"no") )
                  fprintf(fptr,"\t%s %s %s %s%s%s ",
                          S_LIB,S_LIB_OPT,existingLib,S_LIB_OUTPUTFILE,
                          outputBaseFilename,S_LIB_EXT);
               else
                  fprintf(fptr,"\t%s %s %s%s ",
                          S_LIB,S_LIB_OPT,outputBaseFilename,S_LIB_EXT);
	       }

            char *filename = CreateBaseFilename(dirList.file[i]);
            char *outputBaseFilename = CreateOutputBaseFilename(filename);
            if ( !strstr(filename,thisFile) )
               fprintf(fptr,"%s%s%s ",S_LIB_SEPARATOR,
                       outputBaseFilename,S_LIB_OBJEXT);
            S_free(outputBaseFilename);

            firstTime = FALSE;
            numObjsOnLine++;
            newLine = FALSE;

            }
        }
      }
   fprintf(fptr,"%s\n",S_LIB_EOL);

   fprintf(fptr,"\n");

   S_free(outputBaseFilename);

   GenerateObjectCodeMakefile(fptr);

   return 0;
   }


int
FileExists(char *name)
{
  FILE *fptr = fopen(name,"r");
  if ( fptr )
    {
      fclose(fptr);
      return TRUE;
    }
  return FALSE;
}


int
GenerateExecutableMakefile(FILE *fptr,char *icrtEntry)
{
  
  int i,j,k;
  
  if ( !quiet )
    {
      printf("Generating Executable Section\n");
    }
  
  if ( !quiet )
    {
      printf("Generating target exe filenames\n");
    }

  char *mainStr[2]  = {   "main(", "WinMain(" };
  char *crtEntry[2] = { icrtEntry,         "" };
  
  /* ********************************************************************** */
  /* ****** Create an array of output files that have a main in them ****** */
  /* ********************************************************************** */
  char **outputFiles = NULL;
  char **mainFiles = NULL;
  char **mainEntries = NULL;
  int numOutputFiles = 0;
  int numObjectFiles = 0;
  for ( k=0; k<numInputFiles; k++ )
    {
      S_DirList dirList(inputFiles[k]);
      numObjectFiles += dirList.numFiles;
      for ( i=0; i<dirList.numFiles; i++ )
	{
	  for ( j=0; j<2; j++ )
	    { 
	      if ( FileHas(dirList.file[i],mainStr[j]) )
		{
		  if ( outputFiles == NULL ) {
		    outputFiles = (char **)S_malloc(sizeof(char *));
		    mainFiles = (char **)S_malloc(sizeof(char *));
		    mainEntries = (char **)S_malloc(sizeof(char *));
		  }
		  else {
		    outputFiles = (char **)S_realloc(outputFiles,
						     sizeof(char *)*(numOutputFiles+1));
		    mainFiles = (char **)S_realloc(mainFiles,
						   sizeof(char *)*(numOutputFiles+1));
		    mainEntries = (char **)S_realloc(mainEntries,
						   sizeof(char *)*(numOutputFiles+1));
		  }
		  char *filename = CreateBaseFilename(dirList.file[i]);
		  outputFiles[numOutputFiles] = (char *)S_malloc( strlen(filename) +
								  strlen(exe) + 
								  strlen(S_PLATFORM) +
								  strlen(S_COMPILER) +
								  + 3 );
		  sprintf(outputFiles[numOutputFiles],"%s_%s_%s%s",
			  filename,S_PLATFORM,S_COMPILER,exe);
		  mainFiles[numOutputFiles] = (char *)S_malloc( strlen(filename) + 1 );
		  sprintf(mainFiles[numOutputFiles],"%s",filename);
		  mainEntries[numOutputFiles] = crtEntry[j];
		  numOutputFiles++;
		}
	    }
	}
    }
  /* ********************************************************************** */
  
  
  if ( !quiet )
    {
      printf("Creating dependency for all :\n");
    }

  /* ********************************************* */
  /* *** Create dependency for all exe targets *** */
  /* ********************************************* */
  fprintf(fptr,"all : ");
  for ( i=0; i<numOutputFiles; i++ )
    fprintf(fptr,"%s ",outputFiles[i]);
  fprintf(fptr,"\n\n");
  /* ********************************************* */
  
  if ( !quiet )
    printf("Creating dependency for shared lib\n");
  /* ************************************************* */
  /* *** Create dependency line for shared library *** */
  /* ************************************************* */
  char sharedLibFilename[1024];
  
  if ( outputFile ) {
    char *libBaseFilename = CreateOutputBaseFilename(outputFile);
    sprintf(sharedLibFilename,"%s%s",libBaseFilename,S_LIB_EXT);
    S_free(libBaseFilename);
  }
  
  if ( !outputFile )
    sharedLibFilename[0] = 0;

  int libNeeded = numObjectFiles - numOutputFiles;

  if ( libNeeded && outputFile ) {
    fprintf(fptr,"%s : ",sharedLibFilename);
    for ( k=0; k<numInputFiles; k++ ) {
      S_DirList dirList(inputFiles[k]);
      for ( j=0; j<dirList.numFiles; j++ ) {
	char *filename = CreateBaseFilename(dirList.file[j]);
	char *outputBaseFilename = CreateOutputBaseFilename(filename);
	if ( !strstr(filename,thisFile) && 
	     !FileHas(dirList.file[j],mainStr[0]) &&
	     !FileHas(dirList.file[j],mainStr[1]) )
	  fprintf(fptr,"%s%s ",outputBaseFilename,S_OBJ_EXT);
	S_free(outputBaseFilename);
      }
    }
    fprintf(fptr,"\n");
  }
  /* ************************************************* */

   if ( !quiet )
      printf("Generation Library code\n");
   /* ******************************************************** */
   /* *** Create a library containing all the object files *** */
   /* ******************************************************** */
   if ( libNeeded && outputFile ) {
#     if defined(WIN32) || defined(OS2)
         fprintf(fptr,"\tif exist %s del %s\n",
                 sharedLibFilename,sharedLibFilename);
#     endif
#     ifdef UNIX
         fprintf(fptr,"\trm -f %s\n",sharedLibFilename);
#     endif
      int firstTime = TRUE;
      int numObjsOnLine = 0;
      int newLine = TRUE;
      for ( j=0; j<numInputFiles; j++ ) {
         S_DirList dirList(inputFiles[j]);
         for ( k=0; k<dirList.numFiles; k++ ) {

	    if ( numObjsOnLine > MAX_OBJS_PERLINE ) {
               newLine = TRUE;
               numObjsOnLine = 0;
               fprintf(fptr,"%s\n",S_LIB_EOL);
	       }

            char existingLib[1024];
            if ( !firstTime )
               sprintf(existingLib,"%s",sharedLibFilename);
            else
               sprintf(existingLib,"%s","");

            char *filename = CreateBaseFilename(dirList.file[k]);
            char *objBaseFilename = CreateOutputBaseFilename(filename);
            if ( !strstr(filename,thisFile) &&
                 !FileHas(dirList.file[k],mainStr[0]) &&
		 !FileHas(dirList.file[k],mainStr[1]) ) {

               if ( newLine ) {
                  if ( strcmp(S_LIB_OUTPUTFILE,"no") )
                     fprintf(fptr,"\t%s %s %s %s%s ",
                             S_LIB,S_LIB_OPT,
			     existingLib,S_LIB_OUTPUTFILE,sharedLibFilename);
                  else
                     fprintf(fptr,"\t%s %s %s ",
                             S_LIB,S_LIB_OPT,sharedLibFilename);
	          }

               firstTime = FALSE;
               newLine = FALSE;
               numObjsOnLine++;
               fprintf(fptr,"%s%s%s ",S_LIB_SEPARATOR,
                       objBaseFilename,S_LIB_OBJEXT);
	       }
            S_free(objBaseFilename);
            }
         }
      fprintf(fptr,"%s\n",S_LIB_EOL);
      fprintf(fptr,"\n");
      }
   /* ******************************************************** */

   if ( !quiet )
      printf("Creating dependency line for exe file\n");

   /* ******************************************************* */
   /* *** Create dependency line for the executable files *** */
   /* ******************************************************* */
   for ( i=0; i<numOutputFiles; i++ ) {

      char *outputBaseFilename;

      /* *** Create a dependency line for each individual executable *** */
      outputBaseFilename = CreateOutputBaseFilename(mainFiles[i]);
      fprintf(fptr,"%s : %s%s ",outputFiles[i],outputBaseFilename,
                                    S_OBJ_EXT);
      if ( libNeeded )
	{
	  fprintf(fptr,"%s\n",sharedLibFilename);
	}
      else
	{
	  fprintf(fptr,"\n");
	}

      S_free(outputBaseFilename);

      outputBaseFilename = CreateBaseFilename(outputFiles[i]);

      char appType[128];
      char rcBindAppType[128];

      char *cppFilename = (char *)S_malloc(strlen(mainFiles[i])+5);
      sprintf(cppFilename,"%s.cpp",mainFiles[i]);
      sprintf(appType,"%s %s",S_LINK_GUIAPP,mainEntries[i]);
      strcpy(rcBindAppType,S_RCBIND_GUIAPP);
      if ( FileHas(cppFilename,"S_LIB_CONSOLE") ) {
         sprintf(appType,"%s %s",S_LINK_CONSOLEAPP,mainEntries[i]);
         strcpy(rcBindAppType,S_RCBIND_CONSOLEAPP);
         }
      S_free(cppFilename);

      /* ************************************************* */
      /* *** Compile resource file if one is specified *** */
      /* ************************************************* */
      char *rcFilename = (char *)S_malloc(strlen(outputBaseFilename)+4);
      sprintf(rcFilename,"%s.rc",outputBaseFilename);
      if ( FileExists(rcFilename) )
         fprintf(fptr,"\t%s %s %s\n",S_RC,S_RC_OPT,rcFilename);
      /* ************************************************* */

      /* ************ */
      /* *** Link *** */
      /* ************ */
      char *objBaseFilename = CreateOutputBaseFilename(mainFiles[i]);
      if ( staticFile )
	{
	  fprintf(fptr,
		  "\t%s %s %s "
		  "%s%s%s "
		  "%s%s ",S_LINK,S_LINK_OPT,
		  appType,S_LINK_OUTPUTFILE,
		  objBaseFilename,S_LINK_EXEEXT,objBaseFilename,S_OBJ_EXT);
	}
      else
	{
	  fprintf(fptr,
		  "\t%s $(S_LINK_OPT) %s "
		  "%s%s%s "
		  "%s%s ",S_LINK,
		  appType,S_LINK_OUTPUTFILE,
		  objBaseFilename,S_LINK_EXEEXT,objBaseFilename,S_OBJ_EXT);
	}
      S_free(objBaseFilename);
      if ( libNeeded && outputFile )
         fprintf(fptr,"%s",sharedLibFilename);
      else {
         for ( k=0; k<numInputFiles; k++ ) {
            S_DirList dirList(inputFiles[k]);
            for ( j=0; j<dirList.numFiles; j++ ) {
               char *filename = CreateBaseFilename(dirList.file[j]);
               char *objBaseFilename = CreateOutputBaseFilename(filename);
               if ( !strstr(filename,thisFile) &&
                    !FileHas(dirList.file[j],mainStr[0]) &&
		    !FileHas(dirList.file[j],mainStr[1]) ) {
                  fprintf(fptr," %s%s",objBaseFilename,S_OBJ_EXT);
	          }
               S_free(objBaseFilename);
               }
            }
	 }
      if ( !strcmp(S_LINK_BINDRES,"yes") && FileExists(rcFilename) )
         fprintf(fptr," %s.res",outputBaseFilename);
      fprintf(fptr," %s",S_LINK_LIBS);
      fprintf(fptr,"\n");

      /* ************************************************* */
      /* *** Bind in resources using resource compiler *** */
      /* ************************************************* */
      if ( strcmp(S_LINK_BINDRES,"yes") &&
          (FileExists(rcFilename) || !strcmp(S_RCBIND_ALL,"yes")) ){
         if ( FileExists(rcFilename) )
            fprintf(fptr,
                    "\t%s %s "
                    "%s %s%s.res "
                    "%s%s\n",
		    S_RCBIND,S_RCBIND_OPT,
                    rcBindAppType,S_RCBIND_RESFILE,
                    outputBaseFilename,
                    outputBaseFilename,S_RCBIND_EXEEXT);
         else
            fprintf(fptr,
                    "\t%s %s "
                    "%s %s%s\n",
		    S_RCBIND,S_RCBIND_OPT,
                    rcBindAppType,
                    outputBaseFilename,S_RCBIND_EXEEXT);
         }
      /* ************************************************* */

      S_free(rcFilename);
      fprintf(fptr,"\n");

      S_free(outputBaseFilename);
      }

   GenerateObjectCodeMakefile(fptr);

   return 0;
   }
