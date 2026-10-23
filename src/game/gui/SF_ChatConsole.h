/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef SF_CHATCONSOLE_INCLUDED
#define SF_CHATCONSOLE_INCLUDED

class SF_Sprite;
class SF_ChatConsole;

class SF_ChatChannel 
{
 public:
  int scrollBegin;
  int maxScrollBegin;
  int maxDisplayLines;
  int numLines;
  
  ulong *rgbColor;
  ushort **color;
  char **text;
  int AppendLine(char *,ulong = 0xFFFFFF,ushort * =NULL);
  int AddLine(char *,ulong = 0xFFFFFF,ushort * =NULL);
  
  SF_ChatConsole *console;
  
  int Init();
  int Destroy();
  SF_ChatChannel();
  ~SF_ChatChannel();
  
};

#define SF_CHATCONSOLE_NUMCHANNELS 3

#define SF_CHANNEL_BROADCAST 0
#define SF_CHANNEL_TEAM      1
#define SF_CHANNEL_IRC       2


int SF_ChatConsole_IRCThread(void *);

class SF_ChatConsole {
public:

  /* Port 2026-10-12: member used by SF_ChatConsole.cpp and SF_NetEngine.cpp
     but missing from the released header. */
  int devmode;

  S_IRCClient ircClient;
  S_Thread ircThread;

  int insertText;
  S_IRCMsgStruct ircInsertMsg;

  int runIRC;
  int ConnectIRC();  
  int DisconnectIRC();  

   int x0;
   int y0;
   int visible;
   int redrawAll;

   int zoom;

   int textStart;

   int xUpButton;
   int yUpButton;
   int xDownButton;
   int yDownButton;
   int xThumbButton;
   int yThumbButton;

   int UpButtonContains(int,int);
   int DownButtonContains(int,int);
   int ThumbButtonContains(int,int);

   int HandleMouseClick(int,int,int,int);

   int dragging;
   int beginDragY;
   int beginDragYScroll;

   SF_ChatChannel channels[SF_CHATCONSOLE_NUMCHANNELS];
   SF_ChatChannel *channel;

   char textEntry[1024];

   SF_Sprite *bmpBack;
   SF_Sprite *bmpScroll;
   S_CanvasBuffer *parentBuffer;
   S_CanvasBuffer *buffer;
   S_Font *font;

   int ProcessColorCode(char *,int &,int &,slong &);
   char *DrawLine(char *,int,int,ulong,ushort * =NULL);

   int SendChatMsg(char *);
   int KeyPressed(uchar,uchar,int,uchar);
   int HandleDevCommands(char *);
   int HandleIRCCommands(char *);

   int SetBmpBack(SF_Sprite *);
   int SetBmpScroll(SF_Sprite *);

   SF_ChatConsole(S_CanvasBuffer *,int,int);
   ~SF_ChatConsole();

   int Draw();
   int DrawIndicatorRect();
   int SetVisible(int);
   int Contains(int,int);

   };

#define SF_KEY_ESC 1

#define SF_KEY_NUM_1 79
#define SF_KEY_NUM_2 80
#define SF_KEY_NUM_3 81
#define SF_KEY_NUM_4 75
#define SF_KEY_NUM_5 76
#define SF_KEY_NUM_6 77
#define SF_KEY_NUM_7 71
#define SF_KEY_NUM_8 72
#define SF_KEY_NUM_9 73

#define SF_KEY_1 2
#define SF_KEY_2 3
#define SF_KEY_3 4
#define SF_KEY_4 5
#define SF_KEY_5 6
#define SF_KEY_6 7
#define SF_KEY_7 8
#define SF_KEY_8 9
#define SF_KEY_9 10
#define SF_KEY_0 11

#define SF_KEY_TAB 15

#define SF_KEY_Q 16
#define SF_KEY_W 17
#define SF_KEY_E 18
#define SF_KEY_R 19
#define SF_KEY_T 20
#define SF_KEY_Y 21
#define SF_KEY_U 22
#define SF_KEY_I 23
#define SF_KEY_O 24
#define SF_KEY_P 25

#define SF_KEY_ENTER     28
#define SF_KEY_NUM_ENTER 90 
#define SF_KEY_A 30
#define SF_KEY_S 31
#define SF_KEY_D 32
#define SF_KEY_F 33
#define SF_KEY_G 34
#define SF_KEY_H 35
#define SF_KEY_J 36
#define SF_KEY_K 37
#define SF_KEY_L 38

#define SF_KEY_Z 44
#define SF_KEY_X 45
#define SF_KEY_C 46
#define SF_KEY_V 47
#define SF_KEY_B 48
#define SF_KEY_N 49
#define SF_KEY_M 50
#define SF_KEY_LT 51
#define SF_KEY_GT 52
#define SF_KEY_QUESTION 53

#define SF_KEY_SPACE 57

#define SF_KEY_F1  59
#define SF_KEY_F2  60
#define SF_KEY_F3  61
#define SF_KEY_F4  62
#define SF_KEY_F5  63
#define SF_KEY_F6  64
#define SF_KEY_F7  65
#define SF_KEY_F8  66
#define SF_KEY_F9  67
#define SF_KEY_F10 68
#define SF_KEY_F11 87
#define SF_KEY_F12 88

#define SF_KEY_MINUS     12
#define SF_KEY_PLUS      13
#define SF_KEY_BACKSPACE 14
#define SF_KEY_NUM_MINUS 74
#define SF_KEY_HOME      71
#define SF_KEY_NUM_PLUS  78

#define SF_KEY_BACKSLASH 43
#define SF_KEY_LSHIFT    42 
#define SF_KEY_RSHIFT    54

#define SF_KEY_LCTRL     29

#ifdef UNIX
#  define SF_KEY_RCTRL     91
#endif
#ifdef OS2
#  define SF_KEY_RCTRL     91
#endif
#ifdef WIN32
#  define SF_KEY_RCTRL     29
#endif

#endif
