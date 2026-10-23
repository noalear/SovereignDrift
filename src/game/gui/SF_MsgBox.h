#ifndef SF_MSGBOX_INCLUDED
#define SF_MSGBOX_INCLUDED

class SF_MsgBox : public S_Window {
public:
   char *text;

   void *arg;
   int (*handler)(void *);

   S_StaticText *msgText;
   S_Button   *okButton;

   int Hide();
   int Show(char *,char *,int (*)(void *) =NULL,void * =NULL);

   SF_MsgBox(S_Object *);
   ~SF_MsgBox();

   int HandleCreateMsg();
   int HandleSizeMsg();
   int HandlePaintMsg();
   int HandleCloseMsg();
   };

int SF_MsgBox_OkButtonHandler(S_Button *);

#endif

