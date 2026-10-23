#include "S_OS.h"
#include "S_MsgBox.h"
#include "S_Object.h"

S_MsgBox::S_MsgBox(S_Object *parent,char *windowTitle,char *text,int type) {
   if ( !parent )
      return;
#  ifdef OS2
      rc = WinMessageBox(HWND_DESKTOP,parent->hwnd,text,windowTitle,0,type);
#  endif
#  ifdef WIN32
      rc = MessageBox(parent->hwnd,text,windowTitle,type);
#  endif
   }

S_MsgBox::~S_MsgBox() {
   }
