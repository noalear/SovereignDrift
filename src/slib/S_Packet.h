/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#define S_PACKET_ERROR 0
#define S_PACKET_OK    1

#define S_PACKET_MEMALLOCFAILED   -1
#define S_PACKET_COMPRESSFAILED   -2
#define S_PACKET_UNCOMPRESSFAILED -3
#define S_PACKET_FILEREADFAILED   -4

#define S_PACKET_FLUSHPOPPED      -1
#define S_PACKET_FLUSHALL         -2

class S_Packet
{
 public:
  ulong index;
  ulong headerSize;
  ulong packetSize;
  ulong uncompPacketSize;
  
  int SetBuffer(int,uchar);
  uchar Buffer(int);
  
  int SetPacket(int,uchar);
  uchar Packet(int);
  
  ulong bufferSize;
  unsigned char *buffer;
  
  int readComplete;
  ulong totalBytesRcvd;
  
  int CreateSpace(int);
  int GrowBuffer(ulong);   
  int ShrinkBuffer(void);   
  int SetPacketSize(void);
  
  S_Packet();
  ~S_Packet();
  
  int Rewind(void);
  int Flush(int=S_PACKET_FLUSHALL);
  
  int Merge(S_Packet *);
  
  int PushULong(ulong);
  int PushLong(slong);
  int PushUShort(unsigned short);
  int PushShort(short);
  int PushUChar(unsigned char);
  int PushChar(char);
  int PushString(char *);
  int PushBuffer(ulong,char *,ulong=0);
  
  uchar PeekUChar();
  char PeekChar();
  ulong PeekULong();
  
  int Write(FILE *);
  int Read(FILE *);
  
  ulong PopULong(void);
  slong PopLong(void);
  unsigned short PopUShort(void);
  short PopShort(void);
  unsigned char PopUChar(void);
  char PopChar(void);
  char *PopString(void);
  char *PopBuffer();
  
  int Compress(void);
  int UnCompress(void);
};
