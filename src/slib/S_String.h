#ifndef S_STRING
#define S_STRING

class S_String
{
 public:
  char *str;
  int len;
  int maxlen;
  
  int SetLen(int);
  int Append(const char *,...);
  int Prepend(const char *,...);

  int AllUpperCase();
  int AllLowerCase();
  
  int Init();
  S_String();
  S_String(int);
  S_String(char *);
  S_String(const char *,...);
  int Create(int);
  int Create(char *);
  int Create(const char *,...);
  int VCreate(const char *,va_list);
  int SetSize(int);
  int Destroy();
  ~S_String();

  int HasCharacter(char);
  int RemoveCharacter(char);
  int ReplaceCharacter(char,char *);
  int ReplaceCharacter(char,char);
  int ReplaceLastOccurence(char,char);
};

#endif
