#ifndef S_IRCCLIENT_H_
#define S_IRCCLIENT_H_

#include "S_Client.h"
#include "S_StringArray.h"

#define S_IRCCLIENT_MAXTOKEN 512
#define S_IRCCLIENT_MAXTEXT  8192

#define S_IRCCLIENT_DARK   0
#define S_IRCCLIENT_LIGHT  1

typedef struct
{
  int  code;
  char cmd  [S_IRCCLIENT_MAXTOKEN];
  char from [S_IRCCLIENT_MAXTOKEN];
  char to   [S_IRCCLIENT_MAXTOKEN];
  char text [S_IRCCLIENT_MAXTEXT];
  ushort color[S_IRCCLIENT_MAXTEXT];
} S_IRCMsgStruct;


class S_IRCClient : public S_Client 
{
 public:

  S_String server;
  S_String nick;
  S_StringArray nickList;
  int bkMode;
  int Connect(char *,int);

  int Parse(char *,S_IRCMsgStruct *);
  int ParseMessage(char *,S_IRCMsgStruct *);
  int ParseCommand(char *,S_IRCMsgStruct *);
  int ParseFormats(S_IRCMsgStruct *);
  int ParseColorCode(char *,short &,short &);
  ushort ParsePrivMsg(char *,char *,char *);

  int SendPass(char *);
  int SendNick(char *);
  int SendUser(char *);
  int SendQuit(char *);
  int SendJoin(char *);
  int SendPart(char *);
  int SendNames(char *);
  int SendPing();
  int SendPong();
  int SendMsg(char *,char *);

  int RecvNextLine(S_IRCMsgStruct *,int *);
  int SetMsgColor(S_IRCMsgStruct *,ushort);

  int Init();
  int Create();
  int Destroy();

  S_IRCClient();
  virtual ~S_IRCClient();
};

#endif // S_IRCCLIENT_H_
