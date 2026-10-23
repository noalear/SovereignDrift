#ifndef S_LISTBOX
#define S_LISTBOX

#include "S_Object.h"

#define S_LISTBOX_MSG         0
#define S_LISTBOX_DOUBLECLICK 1

#ifdef WIN32
#  define S_LISTBOX_MULTICOLUMN LBS_MULTICOLUMN
#endif
#ifdef OS2
#  define S_LISTBOX_MULTICOLUMN 0
#endif
#ifdef UNIX
#  define S_LISTBOX_MULTICOLUMN 0
#endif

class S_ListBox : public S_Object {
public:
   S_ListBox();
   S_ListBox(S_Object *,int (*)(S_ListBox *) = NULL,
             int=0,int=0,int=0,int=0,int=0);
   int Create(S_Object *,int (*)(S_ListBox *) = NULL,
          int=0,int=0,int=0,int=0,int=0);
   int Destroy();
   virtual ~S_ListBox();

   int AddItem(char *);
   int InsertItem(char *,int);
   int RemoveItem(int);
   int GetIndex();
   int SetIndex(int);
   int GetNumItems();
   int Flush();

   char *GetItem();
   };

#endif
