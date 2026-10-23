#define S_LIB_CONSOLE

#include "sf.h"

int
main(int argc,char *argv[]) {
   if ( argc == 2 ) {
      S_RegCode rc2;
      rc2.Create(SF_REGCODE_KEY);
      rc2.GenerateCode(atoi(argv[1]));
      FILE *fptr = fopen("reg.out","w");
      if ( fptr ) {
         fprintf(fptr,"SF-%06d-%s\n",atoi(argv[1]),rc2.code.str);
         fclose(fptr);
         }
      printf("SF-%06d-%s\n",atoi(argv[1]),rc2.code.str);
      }
   else if ( argc == 3 ) {
      FILE *fptr = fopen("reg.out","w");
      if ( fptr ) {
         for ( int i=atoi(argv[1]); i<atoi(argv[2]); i++ ) {
            S_RegCode rc;
            rc.Create(SF_REGCODE_KEY);
            rc.GenerateCode(i);
            fprintf(fptr,"SF-%06d-%s\n",i,rc.code.str);
            printf("SF-%06d-%s\n",i,rc.code.str);
            }
         fclose(fptr);
         }
      }
   else
      printf("Usage: reg [Start Serial Number] [End Serial Number]\n");
   return 0;
   }
