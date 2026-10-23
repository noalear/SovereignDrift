#include "S_OS.h"
#include "S_Canvas.h"
#include "S_IRCClient.h"
#include "S_TokenArray.h"

S_IRCClient::S_IRCClient()
{
   Init(); // Call at top of all constructors

   Create(); // Call Create() method matching params

}


S_IRCClient::~S_IRCClient()
{
   Destroy();  // All destructors consist only of call to Destroy()
}


int
S_IRCClient::Init()
{
   // Init member variables

   bkMode = S_IRCCLIENT_DARK;

   return 0;
}


int
S_IRCClient::Create()
{
   Destroy(); // Call at top of all Create() methods

   // Call subclass and member Create() methods
   S_Client::Create();

   // Allocate member variable data

   return 0;
}


int
S_IRCClient::Destroy()
{
   // Free member variable data

   Init(); // Call at bottom of all Destroy() methods

   return 0;
}


int
S_IRCClient::Connect(char *iserver,int iport)
{
   server.Create(iserver);
   S_Client::Connect(iserver,iport,S_CLIENT_RAWDATA);

   return 0;
}


int
S_IRCClient::SendPass(char *password)
{
   if ( !password )
   {
      return 0;
   }
   S_String buffer("PASS %s%c%c",password,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}

int
S_IRCClient::SendNick(char *inick)
{
   if ( !inick )
   {
      return 0;
   }
   nick.Create(inick);
   S_String buffer("NICK %s%c%c",inick,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}

int
S_IRCClient::SendUser(char *realName)
{
   if ( !realName )
   {
      return 0;
   }
   S_String buffer("USER %s %s %s :%s%c%c",
      nick.str,"localhost",server.str,realName,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}


int
S_IRCClient::SendQuit(char *msg)
{
   S_String buffer;
   if ( msg )
   {
      buffer.Create("QUIT :%s%c%c",msg,13,10);
   }
   else
   {
      buffer.Create("QUIT%c%c",13,10);
   }
   Send(buffer.str,buffer.len);

   nickList.Destroy();

   return 0;
}

int
S_IRCClient::SendJoin(char *channel)
{
   if ( !channel )
   {
      return 0;
   }
   S_String buffer("JOIN %s%c%c",channel,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}

int
S_IRCClient::SendPart(char *channel)
{
   if ( !channel )
   {
      return 0;
   }
   S_String buffer("PART %s%c%c",channel,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}

int
S_IRCClient::SendNames(char *channel)
{
   if ( !channel )
   {
      return 0;
   }
   S_String buffer("NAMES %s%c%c",channel,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}

int
S_IRCClient::SendPing()
{
   S_String buffer("PING %s%c%c",server.str,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}

int
S_IRCClient::SendPong()
{
   S_String buffer("PONG %s%c%c",server.str,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}

int
S_IRCClient::SendMsg(char *to,char *msg)
{
   if ( !msg || !to )
   {
      return 0;
   }
   S_String buffer("PRIVMSG %s :%s%c%c",to,msg,13,10);
   Send(buffer.str,buffer.len);
   return 0;
}


int
S_IRCClient::RecvNextLine(S_IRCMsgStruct *msg,int *run)
{

   if ( !msg )
   {
      return 0;
   }

   int i = 0;
   int crRcvd = FALSE;
   int lfRcvd = FALSE;

   char line[S_IRCCLIENT_MAXTEXT];
   line[0] = 0;

   do 
   {
      int bytesRcvd = Recv(line+i,1,S_CLIENT_TCP | S_CLIENT_NONBLOCK);
      if ( bytesRcvd > 0 )
      {
         if ( line[i] == 13 )
         {
            crRcvd = TRUE;
         }
         if ( line[i] == 10 )
         {
            lfRcvd = TRUE;
         }
         if ( i < S_IRCCLIENT_MAXTEXT-1 )
         {
            i++;
         }
      }
      else
      {
         _OsSleep(10000);
      }
   } while ( !crRcvd && !lfRcvd && *run );

   line[i] = 0;

   //
   // Strip out newline type characters.
   //
   for ( i=0; i<(int)strlen(line); i++ )
   {
      if ( line[i] == 10 || line[i] == 13 )
      {
         line[i] = 0;
      }
   }

   //printf("%s\n",line);

   Parse(line,msg);

   return msg->code;
}


int
S_IRCClient::Parse(char *line,S_IRCMsgStruct *msg)
{

   //printf("%s\n",line);

   msg->cmd[0] = 0;
   msg->from[0] = 0;
   msg->to[0] = 0;
   msg->text[0] = 0;
   msg->code = 0;
   SetMsgColor(msg, bkMode ? 0xFF01 : 0xFF10);

   if ( line[0] == ':' )
   {
      ParseMessage(line,msg);
   }
   else
   {
      ParseCommand(line,msg);
   }

   return msg->code;
}


int
S_IRCClient::ParseCommand(char *line,S_IRCMsgStruct *msg)
{

   S_TokenArray tokens(line,' ',' ',S_TOKENARRAY_IGNOREQUOTES);

   if ( tokens.numLines > 0 && tokens.lines[0] )
   {
      strcpy(msg->cmd,tokens.lines[0]);
      msg->code = atol(msg->cmd);
   }

   char *params = strstr(line,":");

   int colonParams = params != NULL;

   if ( !params )
   {
      params = strstr(line," ");
   }

   if ( !params )
   {
      return msg->code;
   }

   params++;

   if ( msg->code == 0 )
   {
      if ( !stricmp(msg->cmd,"PART") )
      {
         if ( tokens.numLines > 1 )
         {
            S_String part;
            if ( colonParams )
            {
               part.Create("*** %s has left %s (%s)",
                  msg->from,tokens.lines[1],params);
            }
            else
            {
               part.Create("*** %s has left %s",msg->from,tokens.lines[1]);
            }
            strcpy(msg->text,part.str);
            SetMsgColor(msg, bkMode ? 0xFF03 : 0xFF09);

            nickList.RemoveLine(nickList.GetIndex(msg->from));
         }
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"NICK") )
      {
         if ( tokens.numLines > 1 ) 
         {
            S_String nick("*** %s is now known as %s",
               msg->from,params);
            strcpy(msg->text,nick.str);
            SetMsgColor(msg, bkMode ? 0xFF03 : 0xFF09);

            nickList.RemoveLine(nickList.GetIndex(msg->from));
            nickList.AppendLine(params);
         }
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"KICK") )
      {
         if ( tokens.numLines > 2 ) 
         {
            S_String kick;
            if ( params[0] != 0 )
            {
               kick.Create("*** %s was kicked by %s (%s)",
                  tokens.lines[2],msg->from,params);
            }
            else
            {
               kick.Create("*** %s was kicked by %s",
                  tokens.lines[2],msg->from);
            }
            strcpy(msg->text,kick.str);
            SetMsgColor(msg, bkMode ? 0xFF02 : 0xFF0B);

            nickList.RemoveLine(nickList.GetIndex(tokens.lines[2]));
         }
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"JOIN") )
      {
         if ( tokens.numLines > 1 )
         {
            S_String join("*** %s has joined %s",msg->from,tokens.lines[1]);
            strcpy(msg->text,join.str);
            SetMsgColor(msg, bkMode ? 0xFF03 : 0xFF09);

            nickList.AppendLine(msg->from);
         }
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"QUIT") )
      {
         S_String quit;
         if ( params[0] != 0 )
         {
            quit.Create("*** %s has quit IRC (%s)",msg->from,params);
         }
         else
         {
            quit.Create("*** %s has quit IRC",msg->from);
         }
         strcpy(msg->text,quit.str);
         SetMsgColor(msg, bkMode ? 0xFF02 : 0xFF0B);

         if (nickList.HasString(msg->from)) {
            nickList.RemoveLine(nickList.GetIndex(msg->from));
         }

         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"MODE") )
      {
         S_String mode("*** %s sets mode: %s",msg->from,params);
         strcpy(msg->text,mode.str);
         SetMsgColor(msg, bkMode ? 0xFF03 : 0xFF09);
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"TOPIC") )
      {
         S_String topic("*** %s changes topic to '%s'",msg->from,params);
         strcpy(msg->text,topic.str);
         SetMsgColor(msg, bkMode ? 0xFF03 : 0xFF09);
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"PROP") )
      {
         for ( int i=0; i<(int)strlen(tokens.lines[2]); i++ )
         {
            tokens.lines[2][i] = tolower(tokens.lines[2][i]);
         }
         S_String topic("*** %s changes %s to '%s'",
            msg->from,tokens.lines[2],params);
         strcpy(msg->text,topic.str);
         SetMsgColor(msg, bkMode ? 0xFF03 : 0xFF09);
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"NOTICE") )
      {
         S_String notice("-%s- %s",msg->from,params);
         strcpy(msg->text,notice.str);
         SetMsgColor(msg, bkMode ? 0xFF05 : 0xFF04);
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"PRIVMSG") )
      {
         ushort color = ParsePrivMsg(msg->text,params,msg->from);
         SetMsgColor(msg,color);
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"PING") )
      {
         SendPong();
         return msg->code;
      }
      else if ( !stricmp(msg->cmd,"ERROR") )
      {
         sprintf(msg->text,line);
         SetMsgColor(msg, bkMode ? 0xFF05 : 0xFF04);
         return msg->code;
      }
      else
      {
         sprintf(msg->text,"Unhandled COMMAND %s",line);
         SetMsgColor(msg, bkMode ? 0xFF05 : 0xFF04);
      }
   }
   else if(msg->code == 353) {
      nickList.Create();
      char *nick;
      for (int ndx = 4; ndx < tokens.numLines-1; ndx++) {
         nick = tokens.lines[ndx];
         if (ndx == 4)
            nick++;

         if (*nick == '@')
            nick++;

         nickList.AppendLine(nick);
      }
   }


   return msg->code;
}


ushort
S_IRCClient::ParsePrivMsg(char *out,char *in,char *from)
{
   if ( in && in[0]==1 && strstr(in,"ACTION") == in+1 )
   {
      S_String action("* %s %s",from,in+8);
      action.ReplaceCharacter((char)1,(char)0);
      strcpy(out,action.str);
      return bkMode ? 0xFF06 : 0xFF0D;
   }

   if ( in )
   {
      S_String text("<%s> %s",from,in);
      strcpy(out,text.str);
      return  bkMode ? 0xFF01 : 0xFF10;
   }
   return  bkMode ? 0xFF01 : 0xFF10;
}


int
S_IRCClient::ParseMessage(char *line,S_IRCMsgStruct *msg)
{

   S_TokenArray tokens(line+1,' ',' ',S_TOKENARRAY_IGNOREQUOTES);

   //
   // Strip off who the message is from before sending to Command Parser.
   //
   if ( tokens.numLines > 0 && tokens.lines[0] )
   {
      strcpy(msg->from,tokens.lines[0]);
      for ( int i=0; i<(int)strlen(msg->from); i++ )
      {
         if ( msg->from[i] == '!' )
         {
            msg->from[i] = 0;
         }
      }
   }

   char *command = strstr(line," ");
   if ( command )
   {
      ParseCommand(command+1,msg);
   }

   ParseFormats(msg);

   return msg->code;
}


int
S_IRCClient::ParseFormats(S_IRCMsgStruct *msg)
{

   if ( !msg )
   {
      return 0;
   }

   char *text = msg->text;
   ushort *color = msg->color;

   int nchars = 0;
   char *msgText = S_strdup(msg->text);
   if ( !msgText )
   {
      return 0;
   }

   //printf("%s\n",text);

   short defFgIndex = (schar)(msg->color[0] & 0xFF);
   short defBgIndex = (schar)((msg->color[0] & 0xFF00) >> 8);

   short fgIndex = defFgIndex;
   short bgIndex = defBgIndex;

   int endOfString = FALSE;

   do
   {
      int inFormatBlock = FALSE;
      do
      {

         switch ( *text )
         {
         case 2 : // Bold
            text++;
            break;
         case 3 : // Color
            text += ParseColorCode(text,fgIndex,bgIndex);
            break;
         case 9 : // Italics
            text++;
            break;
         case 15 : // Plain
            fgIndex = defFgIndex;
            bgIndex = defBgIndex;
            text++;
            break;
         case 22 : // Reverse
            fgIndex = 1;
            bgIndex = 16;
            text++;
            break;
         case 31 : // Underline
            text++;
            break;
         default :
            inFormatBlock = FALSE;
         }

      } while ( inFormatBlock );

      //
      // Skip regular msg text.
      //
      int found = FALSE;
      do
      {
         found = 
            *text == 0  || *text == 2  || *text == 3  || *text == 9 || 
            *text == 15 || *text == 22 || *text == 31;
         if ( !found )
         {
            msgText[nchars] = *text;
            color[nchars] = fgIndex | (bgIndex<<8);
            nchars++;
            text++;
         }
      } while ( !found );

      endOfString = *text == '\0';

   } while ( !endOfString );

   //
   // Replace raw text with format codes with clean text that has been rid
   // of all format codes.
   //
   msgText[nchars] = 0;
   strcpy(msg->text,msgText);
   S_free(msgText);  

   return 0;
}


int
S_IRCClient::SetMsgColor(S_IRCMsgStruct *msg,ushort color)
{
   if ( !msg )
   {
      return 0;
   }
   for ( int i=0; i<(int)strlen(msg->text); i++ )
   {
      msg->color[i] = color;
   }
   return 0;
}



int 
S_IRCClient::ParseColorCode(char *itext,
                            short &fgIndex,
                            short &bgIndex)
{

   int ret = 0;
   char *text = itext;

   if ( text[0] != 3 )
   {
      return ret;
   }

   ret++;
   text++;

   int i = 0;
   char snum[128];
   for ( i=0; i<128; i++ )
   {
      snum[i] = 0;
   }

   i = 0;
   while ( *text >= 48 && *text <= 57 )
   {
      snum[i++] = *text;
      ret++;
      text++;
   }

   if ( snum[0] == 0 )
   {
      // Blank color command
      return ret;
   }

   fgIndex = (short)atol(snum);

   if ( *text != ',' )
   {
      // Forground color only
      return ret;
   }

   text++;
   ret++;

   for ( i=0; i<128; i++ )
   {
      snum[i] = 0;
   }

   i = 0;
   while ( *text >= 48 && *text <= 57 )
   {
      snum[i++] = *text;
      ret++;
      text++;
   }

   if ( snum[0] == 0 )
   {
      // Blank bg color command
      return ret;
   }

   bgIndex = (short)atol(snum);

   return ret;
}

