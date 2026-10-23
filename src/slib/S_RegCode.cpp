#include "S_OS.h"
#include "S_RegCode.h"


int
S_RegCode::Init()
{
  options = S_REGCODE_ALPHANUMERIC;
  numPiratedSerials = 0;
  piratedSerials = NULL;
  return 0;
}


S_RegCode::S_RegCode()
{
  Init();
}


int
S_RegCode::Create(char *ikey,int ioptions,char *iexclude)
{
  options = ioptions;
  exclude.Create(iexclude);
  if ( ikey )
    {
      key.Create(ikey);
    }
  return 0;
}


int
S_RegCode::IsPrintable(char ch)
{
  if ( options & S_REGCODE_NUMERIC && ch >= 48 && ch <= 57 &&
       !exclude.HasCharacter(ch) )
    return TRUE;
  if ( options & S_REGCODE_UPPERCASEALPHA && ch >= 65 && ch <= 90 &&
       !exclude.HasCharacter(ch) )
    return TRUE;
  if ( options & S_REGCODE_LOWERCASEALPHA && ch >= 97 && ch <= 122 &&
       !exclude.HasCharacter(ch) )
    return TRUE;
  return FALSE;
}


int
S_RegCode::GenerateCode(ulong serialNumber,ulong duration)
{
  
  int i;
  
  if ( IsPiratedSerial(serialNumber) )
    {
      code.Create("");
      return -1;
    }
  
  code.Create(key.str);
  
  int keylen = strlen(key.str);
  for ( i=0; i<keylen; i++ )
    {
      ulong base = keylen-i;
      int count = 0;
      do
	{
	  code.str[i] += (char)(1 + (base*(serialNumber+i)) + duration);
	  if ( i > 0 )
            code.str[i] += code.str[i-1];
	  if ( i < keylen-1 )
            code.str[i] += code.str[i+1];
	  base+=i;
	  count++;
	  if ( count > 1024 )
            code.str[i] += (char)base;
	} while ( !IsPrintable(code.str[i]) );
    }
  
  return 0;
}


int
S_RegCode::GetDuration(ulong nSerial,char *sCode)
{
  
  if ( IsPiratedSerial(nSerial) )
    {
      return -1;
    }
  
  S_RegCode rc;
   rc.Create(key.str,options,exclude.str);
   rc.GenerateCode(nSerial);
   if ( !strcmp(sCode,rc.code.str) )
     return 0;
   
   for ( int duration=0; duration<60; duration++ ) {
     rc.GenerateCode(nSerial,duration);
     if ( !strcmp(sCode,rc.code.str) )
       return duration;
   }
   
   return -1;
}


int
S_RegCode::Destroy()
{
  if ( piratedSerials )
    {
      S_free(piratedSerials);
      piratedSerials = NULL;
    }
  return 0;
}


S_RegCode::~S_RegCode()
{
  Destroy();
}


int
S_RegCode::IsPiratedSerial(ulong iserial)
{
  for ( int i=0; i<numPiratedSerials; i++ )
    {
      if ( piratedSerials[i] == iserial )
	{
	  return TRUE;
	}
    }
  
  return FALSE;
}


int
S_RegCode::AddPiratedSerial(ulong iserial)
{
  if ( !piratedSerials )
    {
      piratedSerials = (ulong *)S_malloc(sizeof(ulong));
    }
  else
    {
      piratedSerials = (ulong *)
	S_realloc(piratedSerials,(numPiratedSerials+1)*sizeof(ulong));
    }
  
  if ( piratedSerials )
    {
      piratedSerials[numPiratedSerials] = iserial;
      numPiratedSerials++;
    }
  
  return 0;
}
