#ifndef S_FILEDIALOG
#define S_FILEDIALOG

#define S_FILEDIALOG_OPEN   1
#define S_FILEDIALOG_SAVEAS 2

class S_Object;

class S_FileDialog {
public:
   int rc;
   char filename[1024];
   S_FileDialog(S_Object *,char *,int=S_FILEDIALOG_OPEN);
   ~S_FileDialog();
   };

#endif
