class SF_NagDialog : public S_Window {
public:

   S_StaticText *msgText;

   S_GroupBox *regGroupBox;
   S_StaticText *regStaticText;
   S_StaticText *regKeyStaticText1;
   S_TextBox *regKeyTextBox1;
   S_StaticText *regKeyStaticText2;
   S_TextBox *regKeyTextBox2;
   S_Button *regButton;
   S_Button *stardockButton;
   S_Button *orderFormButton;
   S_Button *noregButton;

   int HandleCreateMsg();
   int HandleSizeMsg();
   int HandlePaintMsg();
   int HandleDestroyMsg();

   SF_NagDialog(S_Object *);
   ~SF_NagDialog();
   };
