#ifndef S_MSGBOX
#define S_MSGBOX

#ifdef OS2
#  define S_MSGBOX_OK    MB_OK
#  define S_MSGBOX_YESNO MB_YESNO
#  define S_MSGBOX_YES   MBID_YES
#  define S_MSGBOX_NO   MBID_NO
#endif
#ifdef WIN32
#  define S_MSGBOX_OK    MB_OK
#  define S_MSGBOX_YESNO MB_YESNO
#  define S_MSGBOX_YES   IDYES
#  define S_MSGBOX_NO    IDNO
#endif
#ifdef UNIX
#  define S_MSGBOX_OK    1
#  define S_MSGBOX_YESNO 2
#  define S_MSGBOX_YES   1
#  define S_MSGBOX_NO    0
#endif

class S_Object;

class S_MsgBox {
public:
   int rc;
   S_MsgBox(S_Object *,char *,char *,int);
   ~S_MsgBox();
   };

#endif
