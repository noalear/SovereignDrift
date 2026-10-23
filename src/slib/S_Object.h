/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_OBJECT
#define S_OBJECT

#define S_OBJECT_GENERIC      0
#define S_OBJECT_BUTTON       1
#define S_OBJECT_LISTBOX      2
#define S_OBJECT_COMBOBOX     3
#define S_OBJECT_TEXTBOX      4
#define S_OBJECT_TEXTEDIT     5
#define S_OBJECT_MENUBAR      6
#define S_OBJECT_MENUITEM     7
#define S_OBJECT_SUBMENU      8
#define S_OBJECT_STATICTEXT   9
#define S_OBJECT_SCROLLBAR   10
#define S_OBJECT_NOTEBOOK    11
#define S_OBJECT_CHECKBOX    12
#define S_OBJECT_RADIOBUTTON 13
#define S_OBJECT_WINDOW      14
#define S_OBJECT_MSGBOX      15
#define S_OBJECT_GROUPBOX    16
#define S_OBJECT_TREEVIEW    17
#define S_OBJECT_CONTAINER   18
#define S_OBJECT_PROGRESSBAR 19
#define S_OBJECT_TOOLBAR     20

#ifdef OS2
typedef struct
{
  ulong id;
  ulong cb;
  ulong ulColor;
} _WindowColorParamStruct;

typedef struct 
{
  ulong id;
  ulong cb;
  char szFontNameSize[20];
} _WindowFontParamStruct;

typedef struct
{
  ulong cb;
  _WindowColorParamStruct fcparam;
  _WindowColorParamStruct bcparam;
  _WindowFontParamStruct fntparam;
} _WindowPresentationParams1;

typedef struct
{
  ulong cb;
  _WindowColorParamStruct fcparam;
  _WindowFontParamStruct fntparam;
} _WindowPresentationParams2;
#endif
#ifdef GTK
typedef GtkWidget *HWND;
#endif

/* ************************************************************************* */
/* *** The base object from which all GUI window objects are derived.    *** */
/* *** This object allows me to keep track of parent child relationships *** */
/* ************************************************************************* */

class S_Root;
class S_Font;

class S_Object
{
 public:
  
  int type;
  
  S_Root *root;
  
  /* ************************* */
  /* *** Basic window data *** */
  /* ************************* */
  HWND hwnd;
  HWND hwndFrame;
  HWND hwndClient;
  
  int x;
  int y;
  int cx;
  int cy;
  int cxClient;
  int cyClient;
  void *puser;
  
  /* ******************* */
  /* *** For parents *** */
  /* ******************* */
  S_Object *firstChild;
  S_Object *lastChild;
  int AddChild(S_Object *);
  int RemoveChild(S_Object *);
  int ReleaseChildren();
  virtual int Release();
  
  /* ******************** */
  /* *** For Children *** */
  /* ******************** */
  S_Object *parent;
  S_Object *next;
  S_Object *prev;
  int id;
  
  int Init();
  S_Object();
  S_Object(S_Object *,int (*)(S_Object *) = NULL,int=S_OBJECT_GENERIC,
	   int=0,int=0,int=0,int=0);
  int Create(S_Object * =NULL,int (*)(S_Object *) = NULL,int=S_OBJECT_GENERIC,
	     int=0,int=0,int=0,int=0);
  int Destroy();
  virtual ~S_Object();
  
  /* ************************************************** */
  /* *** Font,text color, and background color info *** */
  /* ************************************************** */
  S_Font *defaultFont;
  S_Font *font;
  ulong backGroundColor;
  ulong backGroundRGB;
  ulong textColor;
# ifdef OS2
  _WindowPresentationParams1 defaultPres1;
  _WindowPresentationParams2 defaultPres2;
# endif
# ifdef WIN32
  HBRUSH hBackGroundBrush;
# endif
  
  char *str;
  
  /* ******************************* */
  /* *** Basic Window Operations *** */
  /* ******************************* */
  int SetText(char *);
  virtual char *GetText();
  int SetTextColor(ulong);
  int SetBackGroundColor(ulong);
  int SetFont(char *,int);
  int SetFont(S_Font *);
  virtual int SetEnabled(int);
  virtual int IsEnabled();

  S_Object *GetDefaultButton();
  int IsDefaultButton();
  int IsTabStop();
  S_Object *GetNextTabStop(S_Object *);
  S_Object *QueryFocus();
  int HasFocus();
  int SetSize(int,int);
  int SetPosition(int,int);
  int Focus();
  int IsVisible();
  int Show();
  int Hide();
  int Close(int=FALSE);
  int Refresh();
  
  int Profile(int=0);
  
  /* ************************* */
  /* *** A message handler *** */
  /* ************************* */
  ulong msg;
  ulong mp1;
  ulong mp2;
  int (*pMsgHandler)(void *);
  
  S_Object *label;
  int AddLabel(char *,int,int);
  
  int GetNumChildren();
  S_Object *GetObjectContaining(int,int,int,int=TRUE);
};

#endif
