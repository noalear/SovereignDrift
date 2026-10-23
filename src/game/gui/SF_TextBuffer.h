#ifndef SF_TEXTBUFFER_INCLUDED
#define SF_TEXTBUFFER_INCLUDED

class SF_TextBuffer {
public:
   char **text;
   ulong *textColor;
   ulong *lastUpdate;
   int numLines;
   int x0,y0;

   SF_TextBuffer(int=0,int=0,int=1);
   ~SF_TextBuffer();

   int Flush();
   int AddLine(char *,ulong=0);
   int SetLine(int,char *,ulong,ulong);
   int Draw(S_CanvasBuffer *,ulong);
   };

#endif
