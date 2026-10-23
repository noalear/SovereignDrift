#include "sf.h"
#include "SF_TextBuffer.h"
#include "SF_Externals.h"

SF_TextBuffer::SF_TextBuffer(int ix0,int iy0,int inumLines) {
   x0 = ix0;
   y0 = iy0;

   text = NULL;
   textColor = NULL;
   lastUpdate = 0;
   numLines = 0;

   for ( int i=0; i<inumLines; i++ )
      AddLine("",0);
   }

SF_TextBuffer::~SF_TextBuffer() {
   Flush();
   }


int
SF_TextBuffer::Flush() {
   int i;
   if ( text ) {
      for ( i=0; i<numLines; i++ )
         if ( text[i] ) {
            S_free(text[i]);
            text[i] = NULL;
	    }
      S_free(text);
      text = NULL;
      }
   if ( textColor ) {
      S_free(textColor);
      textColor = NULL;
      }
   if ( lastUpdate ) {
      S_free(lastUpdate);
      lastUpdate = NULL;
      }
   numLines = 0;
   return 0;
   }


int
SF_TextBuffer::AddLine(char *itext,ulong color) {
   if ( itext == NULL )
      return 0;
   if ( numLines == 0 ) {
      text = (char **)S_malloc(sizeof(char *));
      textColor = (ulong *)S_malloc(sizeof(ulong));
      lastUpdate = (ulong *)S_malloc(sizeof(ulong));
      }
   else {
      text = (char **)S_realloc(text,sizeof(char *)*(numLines+1));
      textColor = (ulong *)S_realloc(textColor,sizeof(ulong)*(numLines+1));
      lastUpdate = (ulong *)S_realloc(lastUpdate,sizeof(ulong)*(numLines+1));
      }
   text[numLines] = (char *)S_malloc(strlen(itext)+1);
   strcpy(text[numLines],itext);
   textColor[numLines] = color;
   lastUpdate[numLines] = 0;
   numLines++;
   return 0;
   }


int
SF_TextBuffer::SetLine(int i,char *itext,ulong color,ulong ilastUpdate) {
   if ( i<0 || i>=numLines )
      return 0;
   text[i] = (char *)S_realloc(text[i],strlen(itext)+1);
   strcpy(text[i],itext);
   textColor[i] = color;
   lastUpdate[i] = ilastUpdate;
   return 0;
   }


int
SF_TextBuffer::Draw(S_CanvasBuffer *canvas,ulong frame) {
   if ( canvas )
      for ( int i=0; i<numLines; i++ )
         if ( frame - lastUpdate[i] < (uint)(frameRate*20) )
            canvas->DrawString(text[i],x0,y0+canvas->lMaxAscender*i,
                               textColor[i],
                               S_CANVASBUFFER_ADDRECTANGLE);
   return 0;
   }
