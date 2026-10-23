/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_OS
#define S_OS

#include "S_Platform.h"

#define S_LIB_MAJORVERSION      00
#define S_LIB_MINORVERSION      01
#define S_LIB_MINORMINORVERSION 00

#define S_OS_UNKNOWN 0
#define S_OS_OS2     1
#define S_OS_WINNT   2
#define S_OS_WIN95   3
#define S_OS_LINUX   4
#define S_OS_WIN98   5
#define S_OS_WINME   6
#define S_OS_WIN2K   7
#define S_OS_WINXP   8

/* *********************** */
/* *** ANSI C includes *** */
/* *********************** */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <fcntl.h>

/* *********************** */
/* *** ANSI C includes *** */
/* *********************** */

#include <stdint.h>

typedef signed char schar;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
/* Port 2026-10-12: original was 'typedef unsigned long ulong;' (32 bits on all
   1997 targets).  Pinned to exactly 32 bits; 'slong' replaces bare 'long'. */
#ifdef SFHOST
   /* glibc's <sys/types.h> already has 'typedef unsigned long ulong' (64 bit
      on LP64).  Pull it in first, then route slib's ulong to its own type. */
#  include <sys/types.h>
#  define ulong sf_ulong
#endif
typedef uint32_t ulong;
typedef int32_t  slong;

#ifdef OS2
#  include <iostream.h>
#  include <fstream.h>
#else
#  include <iostream>
#  include <fstream>
   using namespace std;
#endif

#ifdef SFHOST
   /* Port 2026-10-12: pull in the C++ library headers the game uses BEFORE
      slib defines its min()/max() macros, which would otherwise break them. */
#  include <sstream>
#  include <iomanip>
#  include <string>
#  include <locale>
#  include <algorithm>
#  include <vector>
#  include <memory>
#endif

#ifdef GNUCPP
#  define FALSE 0
#  define TRUE 1

/* *** Some basic macros *** */
#  define min(a,b) (((a) < (b)) ? (a) : (b))
#  define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#if defined(WIN32) && defined(SFHOST)
/* *** Port 2026-10-12: the Win32 API comes from the port's emulation layer  *** */
/* *** (src/w32emu) instead of the system headers; sockets from sf_net.     *** */
#  include "sfwin.h"
#  include "sf_net.h"
#  include <ctype.h>
#  include <errno.h>
#  include <strings.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  define stricmp(x,y)    strcasecmp(x,y)
#  define strnicmp(x,y,n) strncasecmp(x,y,n)
#  define _stricmp(x,y)   strcasecmp(x,y)
#  define htonl(x) sf_htonl(x)
#  define ntohl(x) sf_ntohl(x)
#  define htons(x) sf_htons(x)
#  define ntohs(x) sf_ntohs(x)
#  define closesocket(s)  sf_closesocket(s)
#  ifdef SFHOST_POSIX
     /* <unistd.h> is avoided: its socklen_t clashes with the Win32 one below */
     extern "C" ssize_t write(int, const void *, size_t);
     extern "C" int close(int);
#    define O_BINARY 0
#    define _open  open
#    define _write write
#    define _close close
#  else
#    include <io.h>
#  endif
#  include "sf_files.h"
#elif defined(WIN32)
#  define STRICT
#  include <windows.h>
#  include <shlobj.h>
#  include <commctrl.h>
#  include <dsound.h>
#  include <ddraw.h>
#  include <winsock.h>
#  include <process.h>
#  include <ctype.h>
#  include <io.h>
#endif
#ifdef WIN32
#  define bcopy(s,d,n)   memcpy((d),(s),(n))
#  define bcmp(s1,s2,n)  memcmp((s1),(s2),(n))
#  define bzero(s,n)     memset(((s),0,(n)))
   typedef int socklen_t;
   typedef char *caddr_t;
#  define WSA_MAJOR_VERSION 1
#  define WSA_MINOR_VERSION 1
#  define WSA_VERSION MAKEWORD(WSA_MAJOR_VERSION, WSA_MINOR_VERSION)
#  define _CloseSocket closesocket
#  define BCA_RLE8 BI_RLE8
#  define ROP_SRCCOPY SRCCOPY
#  define ROP_SRCINVERT SRCINVERT
#  define ROP_SRCAND SRCAND
#  define BBO_IGNORE 0
#  define ICMP_ECHO        8
#  define ICMP_ECHOREPLY   0
#  define setenv(a,b,c) SetEnvironmentVariable((a),(b))
#  define NULL_DEV         "nul"
#endif

#ifdef OS2

/* *** API includes *** */
#  ifndef OS2_INCLUDED
#    define OS2_INCLUDED
#    define INCL_DOSDEVIOCTL
#    define INCL_ERRORS
#    define INCL_DEV
#    define INCL_DOS
#    define INCL_GPI
#    define INCL_WIN
#    define INCL_SW
#    define INCL_MMIOS2
#    define INCL_MACHDR
#    define INCL_OS2MM
#    include <os2.h>
#    ifdef IBMMMOS2
#       include <os2me.h>
#       include <sw.h>
#    endif
#  endif

#  include <io.h>
#  include <process.h>
#  include <ctype.h>

/* *** TCP/IP includes *** */
#  ifdef IBMCPP
#     include <types.h>
#     include <utils.h>
#  endif
#  ifdef GNUCPP
#     include <sys\types.h>
#  endif
#  ifdef IBMTCPIP
#     define _CloseSocket soclose
#  else
#     define _CloseSocket close
#  endif
#  include <netinet\in.h>
#  include <sys\socket.h>
#  include <sys\ioctl.h>
#  include <netdb.h>
   typedef int socklen_t;
#  define SOCK_STREAM      1
#  define SOCK_DGRAM       2
#  define AF_INET          2
#  define TCP_NODELAY 0x0001
#  define ICMP_ECHO        8
#  define ICMP_ECHOREPLY   0
#  define NULL_DEV         "nul"
#endif

#if defined(UNIX) && !defined(SFHOST)
#  include <stdlib.h>
#  include <sys/stat.h>
#  include <sys/time.h>
#  include <sys/param.h>
#  include <utime.h>
#  include <glob.h>
#  include <fcntl.h>
#  include <ctype.h>
#  include <unistd.h>
#  include <signal.h>
#  include <sys/ipc.h>
#  include <sys/sem.h>
#  include <sys/types.h>
#  ifdef BSD
#  else
#     include <sys/io.h>
#  endif
#  include <errno.h>

/* *** TCP/IP includes *** */
#  ifdef SGI
#     include <strings.h>
#  endif
#  include <string.h>
#  include <sys/types.h>
#  include <netinet/in_systm.h>
#  include <netinet/in.h>
#  include <netinet/ip.h>
#  include <netinet/ip_icmp.h>
#  include <sys/socket.h>
#  include <sys/ioctl.h>
#  include <netdb.h>
#  include <arpa/inet.h>
#  include <unistd.h>

#  define _CloseSocket close
#  define stricmp(x,y) strcasecmp(x,y)

#  define BCA_RLE8 0
#  define ROP_SRCCOPY 0
#  define ROP_SRCINVERT 0
#  define ROP_SRCAND 0
#  define BBO_IGNORE 0

#  define NULL_DEV         "/dev/null"
#endif

#ifdef XWIN
#  include <X11/StringDefs.h>
#  include <X11/Intrinsic.h>
#endif

#ifdef GTK
#  include <gtk/gtk.h>
#endif

#ifdef PTHREAD
#  include <pthread.h>
#endif

#if !defined(UNIX)
struct icmp
{
  uchar icmp_type;
  uchar icmp_code;
  ushort icmp_cksum;
  ushort icmp_id;
  ushort icmp_seq;
  char icmp_data[1];
};
#endif

char *stristr(char *,char *);

char *_OsROT13(char *);
ulong _OsFreeMem();
char *_OsString(void);
int   _OsOpenURL(char *);
int   _OsVersion(void);
int   _OsMinorVersion(void);
int   _OsType(void);
int   _OsTcpipInit(void);
int   _OsGetProcessID();
int   _OsGetThreadID();
int   _OsRunProcess(char *,char * ="");
int   _OsDeleteFile(char *);
int   _OsDeleteSelf(char *,char * =NULL);
int   _OsRemoveDir(char *);
int   _OsSystemDisk();
char *_OsSystemDir();
int   _OsSetFileReadOnly(char *,int=TRUE);
int   _OsCopyFile(char *,char *);
int   _OsSleep(ulong);
int   _OsFileExists(char *);
char *_OsFilename(char *);
ulong _OsFileSize(char *);
ulong _OsFileCheckSum(char *,int=0,int=0,ulong * =NULL);
int   _OsGetNumCPUs();

#define S_OS_FILEATTRFAILED    -4
#define S_OS_FILECOPYFAILED    -3
#define S_OS_DIRDELETEFAILED   -2
#define S_OS_FILEDELETEFAILED  -1
#define S_OS_OK                 0
#define S_OS_FILECREATIONDATE   0
#define S_OS_FILEMODIFIEDDATE   1
#define S_OS_FILEACCESSEDDATE   2
time_t _OsGetFileDate(char *,ulong=0);
int _OsSetFileDate(char *,time_t,ulong=0);

typedef struct
{
  slong cx;
  slong cy;
} SizeStruct;

typedef struct
{
  slong xLeft;
  slong yBottom;
  slong xRight;
  slong yTop;
} RectStruct;

typedef struct
{
  slong x;
  slong y;
} PointStruct;

typedef struct
{
  unsigned char bBlue;
  unsigned char bGreen;
  unsigned char bRed;
} RgbStruct;

typedef struct
{
  unsigned char bBlue;
  unsigned char bGreen;
  unsigned char bRed;
  unsigned char fcOptions;
} Rgb2Struct;

#define RGBL_RED(x)   (((x)&0xFF0000)>>16)
#define RGBL_GREEN(x) (((x)&0xFF00)>>8)
#define RGBL_BLUE(x)  (((x)&0xFF))

#define RGB_GAMMA(x,g) ( ((RGBL_RED(x)*(g)/100)<<16) + ((RGBL_GREEN(x)*(g)/100)<<8) + (RGBL_BLUE(x)*(g)/100) )

#  ifdef OS2
      typedef HPAL HPALETTE;
      typedef void *LPDIRECTDRAWSURFACE;
      typedef void *LPDIRECTDRAWPALETTE;
#  endif
#  ifdef WIN32
      typedef HPALETTE HPAL;
#     define RGB_INVERT(x) (RGB(RGBL_RED(x),RGBL_GREEN(x),RGBL_BLUE(x)))
#  endif

#  ifdef IBMMMOS2
#     define MciSendString  mciSendString
#     define MciSendCommand mciSendCommand
#  endif
#  ifdef WIN32
#     define MciSendString(c,r,rl,hwnd,u) mciSendString(c,r,rl,hwnd)
#     define MciSendCommand(i,m,p1,p2,u) mciSendCommand(i,m,p1,(DWORD)(p2))
#  endif

typedef struct
{
  ulong cbFix;
  ulong cx;
  ulong cy;
  unsigned short cPlanes;
  unsigned short cBitCount;
  ulong ulCompression;
  
  ulong cbImage;
  ulong cxResolution;
  ulong cyResolution;
  ulong cclrUsed;
  ulong cclrImportant;
  Rgb2Struct  argbColor[1];
} BitmapInfo3Struct;

#include "S_DebugMem.h"

#endif
