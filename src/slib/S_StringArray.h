#ifndef S_STRINGARRAY
#define S_STRINGARRAY

#define S_STRINGARRAY_CASEINSENSITIVE   0
#define S_STRINGARRAY_CASESENSITIVE     1

class S_StringArray
{
 public:
  
  int maxLines;
  int numLines;
  char **lines;
  int *pos;

  int Init();
  S_StringArray();
  int Create();
  int Destroy();
  ~S_StringArray();

  int HasString(char *,int=S_STRINGARRAY_CASEINSENSITIVE);
  int HasSubString(char *,int=S_STRINGARRAY_CASEINSENSITIVE);

  char *GetLine(int);
  int GetIndex(char *,int=S_STRINGARRAY_CASEINSENSITIVE);
  int ReplaceCharacter(char,char);
  int RemoveCharacter(char);
  int RemoveCharacter(int,char);
  int AppendLine(const char *,...);
  int AppendLine(char *);
  int InsertLine(char *,int);
  int RemoveLine(int);
  int SetLine(char *,int);
};

#endif
