/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#include "S_TextEdit.h"
#include "S_Root.h"


int
S_TextEdit::Init()
{
  return 0;
}


S_TextEdit::S_TextEdit()
{
  Init();
}


S_TextEdit::S_TextEdit(S_Object *iparent,int (*ipMsgHandler)(S_TextEdit *),
                       char *text,int ix,int iy,int icx,int icy,
                       ulong options)
{
  Init();
  Create(iparent,ipMsgHandler,text,ix,iy,icx,icy,options);
}


int
S_TextEdit::Create(S_Object *iparent,int (*ipMsgHandler)(S_TextEdit *),
                   char *text,int ix,int iy,int icx,int icy,
                   ulong options)
{
  S_Object::Create(iparent,(int(*)(S_Object *))ipMsgHandler,
		   S_OBJECT_TEXTEDIT,ix,iy,icx,icy);
# ifdef OS2
  hwnd = WinCreateWindow(parent->hwnd,WC_MLE,text,
			 options|WS_VISIBLE|MLS_BORDER|WS_TABSTOP,
			 x,parent->cyClient-cy-y,cx,cy,
			 parent->hwnd,HWND_TOP,id,NULL,
			 &defaultPres2);
  WinSetWindowULong(hwnd,QWL_USER,(ULONG)this);
# endif
# ifdef WIN32
  hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT",text,
			WS_VISIBLE | WS_CHILD | options | WS_TABSTOP |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
			ES_WANTRETURN | ES_NOHIDESEL,
			x,y,cx,cy,
			parent->hwnd,(HMENU)id,root->hInst,NULL);
  SetWindowLong(hwnd,GWL_USERDATA,(slong)this);
  SetFont(defaultFont);
# endif
# ifdef GTK
# endif
  hwndFrame = hwndClient = hwnd;
  
  SetText(text);
  parent->AddChild(this);
  return 0;
}


S_TextEdit::~S_TextEdit()
{
  if ( parent )
    parent->RemoveChild(this);
}


int
S_TextEdit::SetReadOnly(int flag)
{
# ifdef OS2
  WinSendMsg(hwnd,MLM_SETREADONLY,(MPARAM)flag,(MPARAM)0);
# endif
# ifdef WIN32
  SendMessage(hwnd,EM_SETREADONLY,(WPARAM)flag,(LPARAM)0);
# endif
  return 0;
}


int
S_TextEdit::AppendLine(char *text)
{
  if ( text )
    Append(text);
  
  char ret[3];
  sprintf(ret,"%c%c",13,10);
# ifdef OS2
  WinSendMsg(hwnd,MLM_INSERT,(MPARAM)ret,0);
# endif
# ifdef WIN32
  SendMessage(hwnd,EM_REPLACESEL,0,(LPARAM)ret);
# endif
  return 0;
}


int
S_TextEdit::Append(char *text)
{
  if ( text )
    {
#     ifdef OS2
      int len = (int) WinSendMsg(hwnd,MLM_QUERYTEXTLENGTH,0,0);
      WinSendMsg(hwnd,MLM_SETSEL,(MPARAM)len,(MPARAM)len);
      WinSendMsg(hwnd,MLM_INSERT,(MPARAM)text,0);
#     endif
#     ifdef WIN32
      int len = SendMessage(hwnd,WM_GETTEXTLENGTH,0,0);
      SendMessage(hwnd,EM_SETSEL,(WPARAM)len,(LPARAM)len);
      SendMessage(hwnd,EM_REPLACESEL,0,(LPARAM)text);
#     endif
    }
  return 0;
}


int
S_TextEdit::GetNumLines()
{
# ifdef OS2
  return (int)WinSendMsg(hwnd,MLM_QUERYLINECOUNT,0,0);
# endif
# ifdef WIN32
  return SendMessage(hwnd,EM_GETLINECOUNT,0,0);
# endif
# ifdef GTK
  return 0;
# endif
}


int
S_TextEdit::Flush()
{
# ifdef OS2
  SetText("");
# endif
# ifdef WIN32
  SetText("");
# endif
  return 0;
}


int
S_TextEdit::ScrollUp(int num)
{
  for ( int i=0; i<num; i++ ) {
#   ifdef OS2
    HWND hwndScroll = WinWindowFromID(hwnd,FID_VERTSCROLL);
    int pos = (int)WinSendMsg(hwndScroll,SBM_QUERYPOS,0,0);
    WinSendMsg(hwndScroll,SBM_SETPOS,(MPARAM)(pos-1),0);
#   endif
#   ifdef WIN32
    SendMessage(hwnd,EM_SCROLL,(WPARAM)SB_LINEUP,0);
#   endif
  }
  return 0;
}


int
S_TextEdit::ScrollDown(int num)
{
  for ( int i=0; i<num; i++ )
    {
#     ifdef OS2
      HWND hwndScroll = WinWindowFromID(hwnd,FID_VERTSCROLL);
      int pos = (int)WinSendMsg(hwndScroll,SBM_QUERYPOS,0,0);
      WinSendMsg(hwndScroll,SBM_SETPOS,(MPARAM)(pos+1),0);
#     endif
#     ifdef WIN32
      SendMessage(hwnd,EM_SCROLL,(WPARAM)SB_LINEDOWN,0);
#     endif
    }
  return 0;
}


int
S_TextEdit::ScrollToTop()
{
  ScrollUp(GetNumLines());
  return 0;
}


char *
S_TextEdit::GetString()
{
  int len = 0;
# ifdef OS2
  len = (int)WinSendMsg(hwnd,MLM_QUERYTEXTLENGTH,0,0);
# endif
# ifdef WIN32
  len = SendMessage(hwnd,WM_GETTEXTLENGTH,0,0);
# endif
  if ( str == NULL )
    str = (char *) S_malloc(len+1);
  else
    str = (char *) S_realloc(str,len+1);
# ifdef OS2
  WinSendMsg(hwnd,MLM_SETSEL,(MPARAM)0,(MPARAM)len);
  WinSendMsg(hwnd,MLM_QUERYSELTEXT,(MPARAM)str,0);
# endif
# ifdef WIN32
  SendMessage(hwnd,WM_GETTEXT,len+1,(LPARAM)str);
# endif
  return str;
}


int
S_TextEdit::ImportFile(char *filename)
{
# ifdef OS2
  FILE *fptr;
  if ( (fptr = fopen(filename,"rb")) != NULL )
    {
      fseek(fptr,0L,SEEK_END);
      ulong bytes = ftell(fptr);
      fseek(fptr,0L,SEEK_SET);
      char *szMleBuf = (char *)S_malloc((bytes+1)*sizeof(char));
      if ( szMleBuf ) {
	ulong i = 0;
	while ( (szMleBuf[i++]=fgetc(fptr)) != EOF && i < bytes );
	fclose(fptr);
	szMleBuf[i] = '\0';
	ulong lOffset = 0;
	WinSendMsg(hwnd,MLM_SETIMPORTEXPORT,
		   (MPARAM)szMleBuf,(MPARAM)bytes);
	WinSendMsg(hwnd,MLM_IMPORT,(MPARAM)&lOffset,(MPARAM)&bytes);
	S_free(szMleBuf);
      }
    }
# endif
# ifdef WIN32
  FILE *fptr;
  fptr = fopen(filename,"r");
  if ( fptr )
    {
      char line[1024];
      while ( fgets(line,1023,fptr) != NULL )
	{
	  line[strlen(line)-1] = 0;
	  AppendLine(line);
	}
      fclose(fptr);
      ScrollToTop();
    }
# endif
  return 0;
}
