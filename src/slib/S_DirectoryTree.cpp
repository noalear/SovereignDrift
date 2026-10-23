#include "S_OS.h"
#include "S_DirList.h"
#include "S_DirectoryTree.h"

S_DirectoryTree::S_DirectoryTree(char *irootDir) {
   rootDir = S_strdup(irootDir);
   S_DirList dirList(rootDir,S_DIRLIST_DIRECTORIES);
   printf("%s numFiles: %ld\n",rootDir,dirList.numFiles);
   for ( int i=0; i<dirList.numFiles; i++ ) {
      int j;
      char *justTheFile;
      for ( j=strlen(dirList.file[i]); j>=0 && dirList.file[i][j]!='\\'; j--);
      justTheFile = &dirList.file[i][j+1];
      /*printf("%d: %s --> %s\n",i,dirList.file[i],justTheFile);*/
      if ( strcmp(justTheFile,".") && strcmp(justTheFile,"..") ) {
         char *newDirName=(char *)S_malloc(strlen(rootDir)+strlen("*.*\\")+1);
         sprintf(newDirName,"%s\\*.*",dirList.file[i]);
         S_DirectoryTree *newDir = new S_DirectoryTree(newDirName);
         S_free(newDirName);
         AddChild(newDir);
         /*printf("%s\n",dirList.file[i]);*/
         }
      }
   }

S_DirectoryTree::~S_DirectoryTree() {
   if ( rootDir ) {
      S_free(rootDir);
      rootDir = NULL;
      }
   }
