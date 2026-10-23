#ifndef S_CONTAINER
#define S_CONTAINER

#include "S_Object.h"

#define S_CONTAINER_MSG         0
#define S_CONTAINER_DOUBLECLICK 1

#ifdef WIN32
#  define S_CONTAINER_LISTVIEW    LVS_LIST
#  define S_CONTAINER_DETAILSVIEW LVS_REPORT
#endif
#ifdef OS2
#  define S_CONTAINER_LISTVIEW    0
#  define S_CONTAINER_DETAILSVIEW 0
#endif
#ifdef UNIX
#  define S_CONTAINER_LISTVIEW    0
#  define S_CONTAINER_DETAILSVIEW 0
#endif

class S_Container : public S_Object {
public:
   S_Container();
   S_Container(S_Object *,int (*)(S_Container *) = NULL,
             int=0,int=0,int=0,int=0,int=S_CONTAINER_DETAILSVIEW);
   int Create(S_Object *,int (*)(S_Container *) = NULL,
          int=0,int=0,int=0,int=0,int=S_CONTAINER_DETAILSVIEW);
   int Destroy();
   ~S_Container();

   int InsertColumn(int,char *,int);
   int InsertItem(int,char *,char * =NULL,char * =NULL,char * =NULL,char * =NULL);
   int Flush();

   int InsertItem(char *,int);
   int RemoveItem(int);
   int GetIndex();
   int SetIndex(int);

   char *GetItem();
   };

#endif
