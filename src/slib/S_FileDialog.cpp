#include "S_OS.h"
#include "S_FileDialog.h"
#include "S_Object.h"

S_FileDialog::S_FileDialog(S_Object *parent,char *mask,int type) {
   if ( !parent )
      return;
   strcpy(filename,mask);
#  ifdef OS2
      FILEDLG fd;
      memset(&fd,0,sizeof(fd));
      fd.cbSize = sizeof(fd);

      HWND hwndDialog = NULL;
      if ( type == S_FILEDIALOG_OPEN ) {
         fd.fl = FDS_CENTER | FDS_OPEN_DIALOG;
         fd.pszTitle = "Open...";
         hwndDialog = WinFileDlg(HWND_DESKTOP,parent->hwnd,&fd);
         }
      if ( type == S_FILEDIALOG_SAVEAS ) {
         fd.fl = FDS_CENTER | FDS_SAVEAS_DIALOG;
         fd.pszTitle = "Save As...";
         hwndDialog = WinFileDlg(HWND_DESKTOP,parent->hwnd,&fd);
         }
      rc = hwndDialog != NULL && fd.lReturn == DID_OK;
#  endif
#  ifdef WIN32
      OPENFILENAME ofn;
      memset(&ofn,0,sizeof(ofn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = parent->hwnd;
      ofn.lpstrFile = filename;
      ofn.nMaxFile = 1024;
      if ( type == S_FILEDIALOG_OPEN ) {
         ofn.lpstrTitle = "Open...";
         rc = GetOpenFileName(&ofn);
         }
      if ( type == S_FILEDIALOG_SAVEAS ) {
         ofn.lpstrTitle = "Save As...";
         rc = GetSaveFileName(&ofn);
         }
#  endif
   }

S_FileDialog::~S_FileDialog() {
   }
