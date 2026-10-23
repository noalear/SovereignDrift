#ifndef S_TEXTEDIT
#define S_TEXTEDIT

#include "S_Object.h"

#define S_TEXTEDIT_NOVSCROLL 0

#ifdef OS2
#  define S_TEXTEDIT_VSCROLL MLS_VSCROLL
#  define S_TEXTEDIT_WORDWRAP MLS_WORDWRAP
#  define S_TEXTEDIT_NOWORDWRAP 0
#endif
#ifdef WIN32
#  define S_TEXTEDIT_VSCROLL WS_VSCROLL
#  define S_TEXTEDIT_WORDWRAP 0
#  define S_TEXTEDIT_NOWORDWRAP ES_AUTOHSCROLL
#endif
#ifdef UNIX
#  define S_TEXTEDIT_VSCROLL 0
#  define S_TEXTEDIT_WORDWRAP 0
#  define S_TEXTEDIT_NOWORDWRAP 0
#endif

class S_TextEdit : public S_Object
{
 public:
  int Init();
  S_TextEdit();
  S_TextEdit(S_Object *,int (*)(S_TextEdit *) = NULL,char* = NULL,
	     int=0,int=0,int=0,int=0,
	     ulong=S_TEXTEDIT_VSCROLL|S_TEXTEDIT_WORDWRAP);
  int Create(S_Object *,int (*)(S_TextEdit *) = NULL,char* = NULL,
	     int=0,int=0,int=0,int=0,
	     ulong=S_TEXTEDIT_VSCROLL|S_TEXTEDIT_WORDWRAP);
  int Destroy();
  virtual ~S_TextEdit();
  
  int SetReadOnly(int);
  
  char *GetString();
  
  int Flush();
  
  int Append(char * =NULL);
  int AppendLine(char * =NULL);
  
  int ImportFile(char *);
  int GetNumLines();
  int ScrollUp(int);
  int ScrollDown(int);
  int ScrollToTop();
};

#endif
