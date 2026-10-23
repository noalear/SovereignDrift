/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#include "S_OS.h"
#ifdef SFHOST
#  include "sf_net_api.h"   /* port 2026-10-12: portable sockets */
#endif
#include "S_Packet.h"


S_Packet::S_Packet(void)
{
  index = 0;
  headerSize = 2*sizeof(ulong);
  packetSize = headerSize;
  uncompPacketSize = headerSize;
  readComplete = FALSE;
  totalBytesRcvd = 0;
  bufferSize = 0;
  buffer = (unsigned char *)S_malloc(headerSize);
  if ( buffer != NULL )
    {
      index = headerSize;
      bufferSize = headerSize;
    }
}


S_Packet::~S_Packet(void)
{
  if ( buffer != NULL )
    {
      S_free(buffer);
      buffer = NULL;
    }
}


int
S_Packet::SetBuffer(int i,uchar ch)
{
  if ( buffer && i>=0 && i<(int)bufferSize )
    {
      buffer[i] = ch;
      return 0;
    }
  return -1;
}


uchar
S_Packet::Buffer(int i)
{
  if ( buffer && i>=0 && i<(int)bufferSize )
    return buffer[i];
  return 0;
}


int
S_Packet::SetPacket(int i,uchar ch)
 {
   if ( i>=(int)headerSize )
     SetBuffer(i,ch);
   else
     printf("SetPacket(%d,%d) is INVALID\n",i,ch);
   return 0;
 }


uchar 
S_Packet::Packet(int i)
{
  if ( i>=(int)headerSize && i<(int)packetSize )
    return Buffer(i);
  printf("Packet(%d) is INVALID\n",i);
  return 0;
}


int
S_Packet::ShrinkBuffer(void)
{
  ulong newBufferSize = packetSize;
  if ( !buffer )
    {
      buffer = (uchar *)S_malloc(newBufferSize);
      bufferSize = newBufferSize;
    }
  if ( buffer && newBufferSize < bufferSize )
    buffer = (unsigned char *) S_realloc(buffer,newBufferSize);
  if ( buffer != NULL )
    bufferSize = newBufferSize;
  return bufferSize == newBufferSize;
}


int
S_Packet::GrowBuffer(ulong newBufferSize)
{
  if ( !buffer )
    {
      buffer = (uchar *)S_malloc(newBufferSize);
      bufferSize = newBufferSize;
    }
  if ( buffer )
    {
      if ( newBufferSize > bufferSize )
	buffer = (unsigned char *) S_realloc(buffer,newBufferSize);
      else
	return TRUE;
    }
  if ( buffer != NULL )
    bufferSize = newBufferSize;
  return bufferSize == newBufferSize;
}


int
S_Packet::SetPacketSize()
{
  int i;
  ulong netPacketSize = htonl(packetSize);
  ulong netUncompPacketSize = htonl(uncompPacketSize);
  char *cPacketSize = (char *)&netPacketSize;
  for ( i=0; i<(int)sizeof(ulong); i++ )
    SetBuffer(i,cPacketSize[i]);
  char *cUncompPacketSize = (char *)&netUncompPacketSize;
  for ( i=sizeof(ulong); i<(int)(2*sizeof(ulong)); i++ )
    SetBuffer(i,cUncompPacketSize[i - sizeof(ulong)]);
  return S_PACKET_OK;
}


int
S_Packet::Write(FILE *fptr)
{
  SetPacketSize();
  fwrite(buffer,packetSize,1,fptr);
  return 0;
}


int
S_Packet::Read(FILE *fptr)
{
  if ( buffer )
    {
      int rc = fread(buffer,2*sizeof(ulong),1,fptr);
      if ( rc )
	{
	  ulong *ul = (ulong *)buffer;
	  packetSize = ntohl(*ul);
	  ul = (ulong *)(buffer + sizeof(ulong));
	  uncompPacketSize = ntohl(*ul);
	  if ( !GrowBuffer(uncompPacketSize) )
            return S_PACKET_ERROR;
	  rc = fread(buffer+2*sizeof(ulong),packetSize-2*sizeof(ulong),1,fptr);
	  if ( rc ) {
            if (packetSize != uncompPacketSize)
	      UnCompress();
            Rewind();
            return S_PACKET_OK;
	  }
	  return S_PACKET_FILEREADFAILED;
	}
    }
  return S_PACKET_FILEREADFAILED;
}


int
S_Packet::Rewind(void)
 {
   index = headerSize;
   return S_PACKET_OK;
 }


int
S_Packet::Flush(int flag)
{
  int numBytesToFlush = flag;
  if ( flag == S_PACKET_FLUSHPOPPED )
    numBytesToFlush = index - headerSize;
  if ( flag == S_PACKET_FLUSHALL ) {
    numBytesToFlush = packetSize - headerSize;
    index = packetSize;
  }
  uint i;
  for ( i=index-numBytesToFlush; i<(uint)(packetSize-numBytesToFlush); i++)
    SetPacket(i,Packet(i+numBytesToFlush));;
  for ( i=packetSize-numBytesToFlush; i<(uint)packetSize; i++ )
    SetPacket(i,0);
  packetSize -= numBytesToFlush;
  if ( packetSize < headerSize || packetSize >= bufferSize )
    packetSize = headerSize;
  uncompPacketSize = packetSize;
  totalBytesRcvd = 0;
  index = headerSize;
  return S_PACKET_OK;
}


int
S_Packet::Merge(S_Packet *mergePacket)
{
  ulong newPacketSize = packetSize + mergePacket->packetSize - 
    mergePacket->headerSize;
  if ( !GrowBuffer(newPacketSize) )
    return S_PACKET_ERROR;
  for ( uint i=packetSize; i<(uint)newPacketSize; i++ )
    SetPacket(i,mergePacket->Packet(i-packetSize+headerSize));
  packetSize = newPacketSize;
  index += mergePacket->packetSize - mergePacket->headerSize;
  uncompPacketSize = packetSize;
  return S_PACKET_OK;
}



inline int
S_Packet::CreateSpace(int space)
 {
   for ( int i=packetSize+space-1; i>=(int)index+space; i-- )
     SetPacket(i,Packet(i-space));
   return 0;
 }


int
S_Packet::PushULong( ulong ul )
 {
   int ulSize = sizeof(ulong);
   if ( !GrowBuffer(packetSize + ulSize) )
     return S_PACKET_ERROR;
   CreateSpace(sizeof(ulong));
   ul = htonl(ul);
   unsigned char *cl = (unsigned char *)&ul;
   for ( int i=0; i<ulSize; i++ )
     SetPacket(index++,cl[i]);
   packetSize += ulSize;
   uncompPacketSize = packetSize;
   return S_PACKET_OK;
 }


int
S_Packet::PushLong( slong l )
 {
   int lSize = sizeof(slong);
   if ( !GrowBuffer(packetSize + lSize) )
     return S_PACKET_ERROR;
   CreateSpace(sizeof(slong));
   l = htonl(l);
   unsigned char *cl = (unsigned char *)&l;
   for ( int i=0; i<lSize; i++ )
     SetPacket(index++,cl[i]);
   packetSize += lSize;
   uncompPacketSize = packetSize;
   return S_PACKET_OK;
 }


int
S_Packet::PushUShort( unsigned short us )
 {
   int usSize = sizeof(unsigned short);
   if ( !GrowBuffer(packetSize + usSize) )
     return S_PACKET_ERROR;
   CreateSpace(sizeof(unsigned short));
   us = htons(us);
   unsigned char *cs = (unsigned char *)&us;
   for ( int i=0; i<usSize; i++ )
     SetPacket(index++,cs[i]);
   packetSize += usSize;
   uncompPacketSize = packetSize;
   return S_PACKET_OK;
 }


int
S_Packet::PushShort( short s )
{
  int sSize = sizeof(short);
  if ( !GrowBuffer(packetSize + sSize) )
    return S_PACKET_ERROR;
  CreateSpace(sizeof(short));
  s = htons(s);
  unsigned char *cs = (unsigned char *)&s;
  for ( int i=0; i<sSize; i++ )
    SetPacket(index++,cs[i]);
  packetSize += sSize;
  uncompPacketSize = packetSize;
  return S_PACKET_OK;
}


int
S_Packet::PushUChar( unsigned char uc )
{
  int ucSize = sizeof(unsigned char);
  if ( !GrowBuffer(packetSize + ucSize) )
    return S_PACKET_ERROR;
  CreateSpace(sizeof(unsigned char));
  SetPacket(index++,uc);
  packetSize += ucSize;
  uncompPacketSize = packetSize;
  return S_PACKET_OK;
}


int
S_Packet::PushChar( char c )
{
  int cSize = sizeof(char);
  if ( !GrowBuffer(packetSize + cSize) )
    return S_PACKET_ERROR;
  CreateSpace(sizeof(char));
  SetPacket(index++,c);
  packetSize += cSize;
  uncompPacketSize = packetSize;
  return S_PACKET_OK;
}


int
S_Packet::PushString( char *istr )
{
  char *str = "";
  if ( istr )
    {
      str = istr;
    }
  int sSize = strlen(str) + 1;
  if ( !GrowBuffer(packetSize + sSize) )
    return S_PACKET_ERROR;
  CreateSpace(sSize);
  for ( int i=0; i<sSize; i++ )
    {
      SetPacket(index++,str[i]);
    }
  packetSize += sSize;
  uncompPacketSize = packetSize;
  return S_PACKET_OK;
}


int
S_Packet::PushBuffer( ulong bSize,char *buf,ulong vSize )
{
  if ( vSize != 0 )
    PushULong(vSize);
  else
    PushULong(bSize);
  if ( bSize == 0 )
    return S_PACKET_OK;
  if ( !buf )
    return S_PACKET_OK;
  if ( !GrowBuffer(packetSize + bSize) )
    return S_PACKET_ERROR;
  CreateSpace(bSize);
  for ( int i=0; i<(int)bSize; i++ )
    SetPacket(index++,buf[i]);
  packetSize += bSize;
  uncompPacketSize = packetSize;
  return S_PACKET_OK;
}


ulong 
S_Packet::PopULong(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  ulong *ul = (ulong *)&buffer[index];
  index += sizeof(ulong);
  return ntohl(*ul);
}


ulong 
S_Packet::PeekULong(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  ulong *ul = (ulong *)&buffer[index];
  return ntohl(*ul);
}


slong 
S_Packet::PopLong(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  slong *l = (slong *)&buffer[index];
  index += sizeof(slong);
  return ntohl(*l);
}


unsigned short 
S_Packet::PopUShort(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  unsigned short *us = (unsigned short *)&buffer[index];
  index += sizeof(unsigned short);
  return ntohs(*us);
}


short 
S_Packet::PopShort(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  short *s = (short *)&buffer[index];
  index += sizeof(short);
  return ntohs(*s);
}


unsigned char 
S_Packet::PopUChar(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  unsigned char *uc = (unsigned char *)&buffer[index];
  index += sizeof(unsigned char);
  return *uc;
}


unsigned char 
S_Packet::PeekUChar(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  unsigned char *uc;
  uc = (unsigned char *)&buffer[index];
  return *uc;
}


char 
S_Packet::PopChar(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  char *c = (char *)&buffer[index];
  index += sizeof(char);
  return *c;
}


char 
S_Packet::PeekChar(void)
{
  if ( !buffer || index >= packetSize ) return 0;
  char *c = (char *)&buffer[index];
  return *c;
}


char *
S_Packet::PopString(void)
{
  int i;
  
  if ( !buffer || index >= packetSize )
    {
      return NULL;
    }
  
  int len = 0;
  while ( index < packetSize && buffer[index + len++] != 0 );

  char *str = S_strdup("");
  if ( !str )
    {
      index += max(1,len);
      return NULL;
    }

  if ( len > 1 )
    {
      S_free(str);
      str = (char *)S_malloc(sizeof(char)*len);
      if ( !str )
	{
	  index += max(1,len);
	  return NULL;
	}
      for ( i=0; i<len; i++ )
	{
	  str[i] = buffer[index++];
	}
      return str;
    }

  index++;
  return str;
}


char *
S_Packet::PopBuffer(void)
{
  if ( !buffer || index >= packetSize ) return NULL;
  ulong bSize = PopULong();
  if ( bSize == 0 )
    return NULL;
  char *buf = (char *)S_malloc(bSize);
  if ( buf )
    for ( int i=0; i<(int)bSize&&i<(int)packetSize; i++ )
      buf[i] = buffer[index++];
  return buf;
}


#ifdef __cplusplus
extern "C" {
#endif
int compress(       unsigned char *,ulong *,
              const unsigned char *,ulong);
int uncompress(       unsigned char *,ulong *,
                const unsigned char *,ulong);
#ifdef __cplusplus
}
#endif


int
S_Packet::Compress(void)
{
  
  ulong uncompLen = packetSize - headerSize;
  unsigned char *uncomp = buffer + headerSize;
  ulong compLen = uncompLen*11/10 + 12;
  unsigned char *comp = (unsigned char *) S_malloc(compLen);
  if ( !comp )
    return S_PACKET_MEMALLOCFAILED;
  
  int rc = compress(comp,&compLen,uncomp,uncompLen);
  if ( rc != 0 )
    return S_PACKET_COMPRESSFAILED;
  
  if ( !GrowBuffer(compLen + headerSize) )
    return S_PACKET_ERROR;
  uncompPacketSize = packetSize;
  packetSize = compLen + headerSize;
  for ( int i=headerSize; i<(int)packetSize; i++ )
    SetPacket(i,comp[i-headerSize]);
  S_free(comp);
  
  return S_PACKET_OK;   
}


int
S_Packet::UnCompress(void)
{
  
  ulong compLen = packetSize - headerSize;
  unsigned char *comp = buffer + headerSize;
  ulong uncompLen = uncompPacketSize - headerSize;
  unsigned char *uncomp = (unsigned char *) S_malloc(uncompLen);
  if ( !uncomp )
    return S_PACKET_MEMALLOCFAILED;
  
  int rc = uncompress(uncomp,&uncompLen,comp,compLen);
  if ( rc != 0 )
    return S_PACKET_UNCOMPRESSFAILED;
  
  GrowBuffer(uncompLen + headerSize);
  packetSize = uncompPacketSize;
  for ( int i=headerSize; i<(int)packetSize; i++ )
    SetPacket(i,uncomp[i-headerSize]);
  S_free(uncomp);
  
  return S_PACKET_OK;
}
