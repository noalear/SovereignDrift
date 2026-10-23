/* ************************************************************************* */
/* sfwin.h -- the subset of the Win32 API used by Stellar Frontier 1.10a     */
/* and Stardock's slib, emulated on top of the Godot host.                   */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* The original program was built for Win32 with Microsoft Visual C++.  The  */
/* port keeps that code path: SFHOST builds define WIN32 and include this    */
/* header instead of <windows.h>, <dsound.h>, <ddraw.h> etc.  Everything     */
/* declared here is implemented in src/w32emu/*.cpp: a small window manager, */
/* GDI, the standard controls, menus, MessageBox, DirectSound and MCI.       */
/*                                                                           */
/* Differences from real Win32 that matter for 64-bit hosts:                 */
/*   - WPARAM/LPARAM/LRESULT are pointer sized (as in Win64).                */
/*   - Get/SetWindowLong and Get/SetClassLong take and return LONG_PTR.      */
/*   - Every handle (HWND, HDC, HMENU, HGDIOBJ...) is a small integer, so   */
/*     the 1997 code's habit of passing handles through 32-bit integers     */
/*     (e.g. AppendMenu(...,(UINT)hMenu,...)) keeps working.                 */
/* ************************************************************************* */
#ifndef SFWIN_H
#define SFWIN_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------ basic types ------------------------------ */
#define WINAPI
#define CALLBACK
#define APIENTRY
#define PASCAL
#define FAR
#define NEAR
#define CONST const
#define STDMETHODCALLTYPE
#define VOID void

typedef int             BOOL;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef uint32_t        DWORD;
typedef int32_t         LONG;
typedef uint32_t        ULONG;
typedef unsigned int    UINT;
typedef int             INT;
typedef short           SHORT;
typedef unsigned short  USHORT;
typedef char            CHAR;
typedef unsigned char   UCHAR;
typedef float           FLOAT;
typedef intptr_t        LONG_PTR;
typedef uintptr_t       ULONG_PTR;
typedef uintptr_t       UINT_PTR;
typedef intptr_t        INT_PTR;
typedef uintptr_t       DWORD_PTR;
typedef uintptr_t       WPARAM;
typedef intptr_t        LPARAM;
typedef intptr_t        LRESULT;
typedef int32_t         HRESULT;
typedef DWORD           COLORREF;
typedef WORD            ATOM;
typedef void           *LPVOID;
typedef const void     *LPCVOID;
typedef char           *LPSTR;
typedef const char     *LPCSTR;
typedef char           *LPTSTR;
typedef const char     *LPCTSTR;
typedef BYTE           *LPBYTE;
typedef DWORD          *LPDWORD;
typedef LONG           *LPLONG;
typedef WORD           *LPWORD;
typedef BOOL           *LPBOOL;
typedef int            *LPINT;
typedef DWORD           MCIERROR;
typedef UINT            MCIDEVICEID;

#ifndef TRUE
#  define TRUE  1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#define DECLARE_HANDLE(n) struct n##__ { int unused; }; typedef struct n##__ *n
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HKEY);
DECLARE_HANDLE(HGLRC);
typedef HICON     HCURSOR;
typedef HINSTANCE HMODULE;
typedef void     *HANDLE;
typedef void     *HGDIOBJ;
typedef HANDLE    HGLOBAL;
typedef HANDLE    HLOCAL;
typedef HANDLE    HRSRC;
typedef HANDLE   *PHANDLE;
typedef int (*FARPROC)();

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#define MAKEWORD(a,b)   ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a,b)   ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)       ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)       ((WORD)((DWORD_PTR)(l) >> 16))
#define LOBYTE(w)       ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)       ((BYTE)((DWORD_PTR)(w) >> 8))
#define MAKELPARAM(l,h) ((LPARAM)(DWORD)MAKELONG(l,h))
#define MAKEWPARAM(l,h) ((WPARAM)(DWORD)MAKELONG(l,h))
#define MAKELRESULT(l,h) ((LRESULT)(DWORD)MAKELONG(l,h))
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#define MAKEINTRESOURCE(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define IS_INTRESOURCE(r)  ((((ULONG_PTR)(r)) >> 16) == 0)

#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define GetRValue(rgb) (LOBYTE(rgb))
#define GetGValue(rgb) (LOBYTE(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) (LOBYTE((rgb)>>16))
#define CLR_INVALID 0xFFFFFFFF
#define PALETTERGB(r,g,b) (0x02000000 | RGB(r,g,b))
#define PALETTEINDEX(i) ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))

#ifndef min
#  define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#  define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define ZeroMemory(d,l)    memset((d),0,(l))
#define FillMemory(d,l,f)  memset((d),(f),(l))
#define CopyMemory(d,s,l)  memcpy((d),(s),(l))
#define MoveMemory(d,s,l)  memmove((d),(s),(l))

#include "sfwin_consts.h"

#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX)
#define WS_TILED            WS_OVERLAPPED
#define WS_ICONIC           WS_MINIMIZE
#define WS_CHILDWINDOW      WS_CHILD
#define WS_TILEDWINDOW      WS_OVERLAPPEDWINDOW
#define WS_EX_OVERLAPPEDWINDOW (WS_EX_WINDOWEDGE|WS_EX_CLIENTEDGE)
#define WS_EX_PALETTEWINDOW    (WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW|WS_EX_TOPMOST)

/* ------------------------------ structures ------------------------------- */
typedef struct tagPOINT { LONG x; LONG y; } POINT, *PPOINT, *LPPOINT;
typedef struct tagSIZE  { LONG cx; LONG cy; } SIZE, *PSIZE, *LPSIZE;
typedef struct tagRECT  { LONG left; LONG top; LONG right; LONG bottom; } RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT;

typedef struct tagMSG {
  HWND   hwnd;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD  time;
  POINT  pt;
  /* port: private, carries the host's translated character for a key
     message so TranslateMessage() honours the user's keyboard layout */
  DWORD  lPrivate;
} MSG, *PMSG, *LPMSG;

typedef LRESULT (CALLBACK *WNDPROC)(HWND,UINT,WPARAM,LPARAM);

typedef struct tagWNDCLASS {
  UINT      style;
  WNDPROC   lpfnWndProc;
  int       cbClsExtra;
  int       cbWndExtra;
  HINSTANCE hInstance;
  HICON     hIcon;
  HCURSOR   hCursor;
  HBRUSH    hbrBackground;
  LPCSTR    lpszMenuName;
  LPCSTR    lpszClassName;
} WNDCLASS, WNDCLASSA, *LPWNDCLASS;

typedef struct tagCREATESTRUCT {
  LPVOID    lpCreateParams;
  HINSTANCE hInstance;
  HMENU     hMenu;
  HWND      hwndParent;
  int       cy;
  int       cx;
  int       y;
  int       x;
  LONG      style;
  LPCSTR    lpszName;
  LPCSTR    lpszClass;
  DWORD     dwExStyle;
} CREATESTRUCT, *LPCREATESTRUCT;

typedef struct tagPAINTSTRUCT {
  HDC  hdc;
  BOOL fErase;
  RECT rcPaint;
  BOOL fRestore;
  BOOL fIncUpdate;
  BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagWINDOWPLACEMENT {
  UINT  length;
  UINT  flags;
  UINT  showCmd;
  POINT ptMinPosition;
  POINT ptMaxPosition;
  RECT  rcNormalPosition;
} WINDOWPLACEMENT, *LPWINDOWPLACEMENT;

typedef struct tagWINDOWPOS {
  HWND hwnd;
  HWND hwndInsertAfter;
  int  x;
  int  y;
  int  cx;
  int  cy;
  UINT flags;
} WINDOWPOS, *LPWINDOWPOS;

typedef struct tagNMHDR {
  HWND     hwndFrom;
  UINT_PTR idFrom;
  UINT     code;
} NMHDR, *LPNMHDR;

typedef struct tagSCROLLINFO {
  UINT cbSize;
  UINT fMask;
  int  nMin;
  int  nMax;
  UINT nPage;
  int  nPos;
  int  nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;
typedef const SCROLLINFO *LPCSCROLLINFO;

typedef struct tagMENUITEMINFO {
  UINT      cbSize;
  UINT      fMask;
  UINT      fType;
  UINT      fState;
  UINT      wID;
  HMENU     hSubMenu;
  HBITMAP   hbmpChecked;
  HBITMAP   hbmpUnchecked;
  ULONG_PTR dwItemData;
  LPSTR     dwTypeData;
  UINT      cch;
} MENUITEMINFO, MENUITEMINFOA, *LPMENUITEMINFO;
typedef const MENUITEMINFO *LPCMENUITEMINFO;

typedef struct tagTEXTMETRIC {
  LONG tmHeight;
  LONG tmAscent;
  LONG tmDescent;
  LONG tmInternalLeading;
  LONG tmExternalLeading;
  LONG tmAveCharWidth;
  LONG tmMaxCharWidth;
  LONG tmWeight;
  LONG tmOverhang;
  LONG tmDigitizedAspectX;
  LONG tmDigitizedAspectY;
  BYTE tmFirstChar;
  BYTE tmLastChar;
  BYTE tmDefaultChar;
  BYTE tmBreakChar;
  BYTE tmItalic;
  BYTE tmUnderlined;
  BYTE tmStruckOut;
  BYTE tmPitchAndFamily;
  BYTE tmCharSet;
} TEXTMETRIC, *LPTEXTMETRIC;

typedef struct tagLOGFONT {
  LONG lfHeight;
  LONG lfWidth;
  LONG lfEscapement;
  LONG lfOrientation;
  LONG lfWeight;
  BYTE lfItalic;
  BYTE lfUnderline;
  BYTE lfStrikeOut;
  BYTE lfCharSet;
  BYTE lfOutPrecision;
  BYTE lfClipPrecision;
  BYTE lfQuality;
  BYTE lfPitchAndFamily;
  CHAR lfFaceName[32];
} LOGFONT, *LPLOGFONT;

#pragma pack(push,2)
typedef struct tagBITMAPFILEHEADER {
  WORD  bfType;
  DWORD bfSize;
  WORD  bfReserved1;
  WORD  bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
  BYTE rgbBlue;
  BYTE rgbGreen;
  BYTE rgbRed;
  BYTE rgbReserved;
} RGBQUAD, *LPRGBQUAD;

typedef struct tagBITMAPINFO {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

typedef struct tagBITMAP {
  LONG   bmType;
  LONG   bmWidth;
  LONG   bmHeight;
  LONG   bmWidthBytes;
  WORD   bmPlanes;
  WORD   bmBitsPixel;
  LPVOID bmBits;
} BITMAP, *LPBITMAP;

typedef struct tagPALETTEENTRY {
  BYTE peRed;
  BYTE peGreen;
  BYTE peBlue;
  BYTE peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagLOGPALETTE {
  WORD         palVersion;
  WORD         palNumEntries;
  PALETTEENTRY palPalEntry[1];
} LOGPALETTE, *LPLOGPALETTE;

typedef struct _ICONINFO {
  BOOL    fIcon;
  DWORD   xHotspot;
  DWORD   yHotspot;
  HBITMAP hbmMask;
  HBITMAP hbmColor;
} ICONINFO, *PICONINFO;

typedef struct _devicemode {
  BYTE  dmDeviceName[32];
  WORD  dmSpecVersion;
  WORD  dmDriverVersion;
  WORD  dmSize;
  WORD  dmDriverExtra;
  DWORD dmFields;
  BYTE  dmUnion[16];
  short dmColor;
  short dmDuplex;
  short dmYResolution;
  short dmTTOption;
  short dmCollate;
  BYTE  dmFormName[32];
  WORD  dmLogPixels;
  DWORD dmBitsPerPel;
  DWORD dmPelsWidth;
  DWORD dmPelsHeight;
  DWORD dmDisplayFlags;
  DWORD dmDisplayFrequency;
} DEVMODE, *LPDEVMODE;

typedef struct _NOTIFYICONDATA {
  DWORD cbSize;
  HWND  hWnd;
  UINT  uID;
  UINT  uFlags;
  UINT  uCallbackMessage;
  HICON hIcon;
  CHAR  szTip[64];
} NOTIFYICONDATA, *PNOTIFYICONDATA;

typedef struct _LUID { DWORD LowPart; LONG HighPart; } LUID, *PLUID;
typedef struct _LUID_AND_ATTRIBUTES { LUID Luid; DWORD Attributes; } LUID_AND_ATTRIBUTES;
typedef struct _TOKEN_PRIVILEGES {
  DWORD PrivilegeCount;
  LUID_AND_ATTRIBUTES Privileges[1];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;
#define SE_SHUTDOWN_NAME "SeShutdownPrivilege"

typedef struct _FILETIME { DWORD dwLowDateTime; DWORD dwHighDateTime; } FILETIME;
typedef struct _WIN32_FIND_DATA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  CHAR     cFileName[260];
  CHAR     cAlternateFileName[14];
} WIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct _GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
} GUID, IID, CLSID;
typedef const GUID &REFIID;
typedef const GUID *LPCGUID;
typedef GUID *LPGUID;

/* ---------------------------- common controls ---------------------------- */
DECLARE_HANDLE(HTREEITEM_);
typedef struct _TREEITEM *HTREEITEM;
typedef struct _IMAGELIST *HIMAGELIST;
typedef struct tagTVITEM {
  UINT      mask;
  HTREEITEM hItem;
  UINT      state;
  UINT      stateMask;
  LPSTR     pszText;
  int       cchTextMax;
  int       iImage;
  int       iSelectedImage;
  int       cChildren;
  LPARAM    lParam;
} TVITEM, TV_ITEM, *LPTVITEM;
typedef struct tagNMTREEVIEW {
  NMHDR  hdr;
  UINT   action;
  TVITEM itemOld;
  TVITEM itemNew;
  POINT  ptDrag;
} NMTREEVIEW, NM_TREEVIEW, *LPNMTREEVIEW;

#define LVS_ICON      0x0000
#define LVS_REPORT    0x0001
#define LVS_SMALLICON 0x0002
#define LVS_LIST      0x0003
#define SERVICE_STOPPED 0x00000001
#define SERVICE_RUNNING 0x00000004

/* -------------------------------- MCI ------------------------------------ */
typedef struct tagMCI_OPEN_PARMS {
  DWORD_PTR   dwCallback;
  MCIDEVICEID wDeviceID;
  LPCSTR      lpstrDeviceType;
  LPCSTR      lpstrElementName;
  LPCSTR      lpstrAlias;
} MCI_OPEN_PARMS, *LPMCI_OPEN_PARMS;
typedef struct tagMCI_WAVE_SET_PARMS {
  DWORD_PTR dwCallback;
  DWORD     dwTimeFormat;
  DWORD     dwAudio;
  UINT      wInput;
  UINT      wOutput;
  WORD      wFormatTag;
  WORD      wReserved2;
  WORD      nChannels;
  WORD      wReserved3;
  DWORD     nSamplesPerSec;
  DWORD     nAvgBytesPerSec;
  WORD      nBlockAlign;
  WORD      wReserved4;
  WORD      wBitsPerSample;
  WORD      wReserved5;
} MCI_WAVE_SET_PARMS;

/* ------------------------------ wave format ------------------------------ */
/* mmsystem.h declares these with 1-byte packing (pshpack1.h): S_WaveData.cpp
   fills a PCMWAVEFORMAT and hands it to DirectSound as a WAVEFORMATEX, so
   wBitsPerSample must sit at offset 14 in both. */
#pragma pack(push, 1)
typedef struct waveformat_tag {
  WORD  wFormatTag;
  WORD  nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD  nBlockAlign;
} WAVEFORMAT;
typedef struct pcmwaveformat_tag {
  WAVEFORMAT wf;
  WORD       wBitsPerSample;
} PCMWAVEFORMAT;
typedef struct tWAVEFORMATEX {
  WORD  wFormatTag;
  WORD  nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD  nBlockAlign;
  WORD  wBitsPerSample;
  WORD  cbSize;
} WAVEFORMATEX, *LPWAVEFORMATEX;
#pragma pack(pop)

/* ----------------------------- COM basics -------------------------------- */
struct IUnknown {
  virtual HRESULT QueryInterface(REFIID, LPVOID *) = 0;
  virtual ULONG   AddRef() = 0;
  virtual ULONG   Release() = 0;
  virtual ~IUnknown() {}
};
typedef IUnknown *LPUNKNOWN;
#define S_OK     ((HRESULT)0)
#define S_FALSE  ((HRESULT)1)
#define E_FAIL   ((HRESULT)0x80004005)
#define E_NOINTERFACE ((HRESULT)0x80004002)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr)    (((HRESULT)(hr)) < 0)

/* ----------------------------- DirectSound ------------------------------- */
#define _FACDS 0x878
#define MAKE_DSHRESULT(code) ((HRESULT)(0x88780000 | (code)))
#define DS_OK                 S_OK
#define DSERR_ALLOCATED       MAKE_DSHRESULT(10)
#define DSERR_INVALIDPARAM    ((HRESULT)0x80070057)
#define DSERR_INVALIDCALL     MAKE_DSHRESULT(50)
#define DSERR_GENERIC         E_FAIL
#define DSERR_BUFFERLOST      MAKE_DSHRESULT(150)
#define DSERR_NODRIVER        MAKE_DSHRESULT(120)

typedef struct _DSBUFFERDESC {
  DWORD          dwSize;
  DWORD          dwFlags;
  DWORD          dwBufferBytes;
  DWORD          dwReserved;
  LPWAVEFORMATEX lpwfxFormat;
} DSBUFFERDESC, *LPDSBUFFERDESC;
typedef const DSBUFFERDESC *LPCDSBUFFERDESC;

typedef struct _DSCAPS { DWORD dwSize; DWORD dwFlags; DWORD reserved[22]; } DSCAPS, *LPDSCAPS;

struct IDirectSoundBuffer : public IUnknown {
  virtual HRESULT GetCaps(void *) = 0;
  virtual HRESULT GetCurrentPosition(LPDWORD, LPDWORD) = 0;
  virtual HRESULT GetFormat(LPWAVEFORMATEX, DWORD, LPDWORD) = 0;
  virtual HRESULT GetVolume(LPLONG) = 0;
  virtual HRESULT GetPan(LPLONG) = 0;
  virtual HRESULT GetFrequency(LPDWORD) = 0;
  virtual HRESULT GetStatus(LPDWORD) = 0;
  virtual HRESULT Initialize(void *, LPCDSBUFFERDESC) = 0;
  virtual HRESULT Lock(DWORD, DWORD, LPVOID *, LPDWORD, LPVOID *, LPDWORD, DWORD) = 0;
  virtual HRESULT Play(DWORD, DWORD, DWORD) = 0;
  virtual HRESULT SetCurrentPosition(DWORD) = 0;
  virtual HRESULT SetFormat(const WAVEFORMATEX *) = 0;
  virtual HRESULT SetVolume(LONG) = 0;
  virtual HRESULT SetPan(LONG) = 0;
  virtual HRESULT SetFrequency(DWORD) = 0;
  virtual HRESULT Stop() = 0;
  virtual HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD) = 0;
  virtual HRESULT Restore() = 0;
};
typedef IDirectSoundBuffer *LPDIRECTSOUNDBUFFER;

struct IDirectSound : public IUnknown {
  virtual HRESULT CreateSoundBuffer(LPCDSBUFFERDESC, LPDIRECTSOUNDBUFFER *, LPUNKNOWN) = 0;
  virtual HRESULT GetCaps(LPDSCAPS) = 0;
  virtual HRESULT DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER, LPDIRECTSOUNDBUFFER *) = 0;
  virtual HRESULT SetCooperativeLevel(HWND, DWORD) = 0;
  virtual HRESULT Compact() = 0;
  virtual HRESULT GetSpeakerConfig(LPDWORD) = 0;
  virtual HRESULT SetSpeakerConfig(DWORD) = 0;
  virtual HRESULT Initialize(LPCGUID) = 0;
};
typedef IDirectSound *LPDIRECTSOUND;

HRESULT WINAPI DirectSoundCreate(LPCGUID, LPDIRECTSOUND *, LPUNKNOWN);

/* ------------------------------ DirectDraw ------------------------------- */
/* Stellar Frontier creates its S_Root without S_ROOT_USEDIRECTDRAW, so      */
/* DirectDraw is never obtained (ddraw.dll "is not present").  These        */
/* declarations only let the original code paths compile.                    */
#define DD_OK S_OK
#define MAKE_DDHRESULT(code) ((HRESULT)(0x88760000 | (code)))
#define DDERR_GENERIC              E_FAIL
#define DDERR_INVALIDPARAMS        ((HRESULT)0x80070057)
#define DDERR_OUTOFMEMORY          ((HRESULT)0x8007000E)
#define DDERR_UNSUPPORTED          ((HRESULT)0x80004001)
#define DDERR_INVALIDCLIPLIST      MAKE_DDHRESULT(110)
#define DDERR_INVALIDMODE          MAKE_DDHRESULT(120)
#define DDERR_INVALIDOBJECT        MAKE_DDHRESULT(130)
#define DDERR_INVALIDRECT          MAKE_DDHRESULT(150)
#define DDERR_LOCKEDSURFACES       MAKE_DDHRESULT(160)
#define DDERR_NOALPHAHW            MAKE_DDHRESULT(180)
#define DDERR_NOBLTHW              MAKE_DDHRESULT(575)
#define DDERR_NOCLIPLIST           MAKE_DDHRESULT(205)
#define DDERR_NODDROPSHW           MAKE_DDHRESULT(576)
#define DDERR_NOEXCLUSIVEMODE      MAKE_DDHRESULT(225)
#define DDERR_NOMIRRORHW           MAKE_DDHRESULT(250)
#define DDERR_NORASTEROPHW         MAKE_DDHRESULT(280)
#define DDERR_NOROTATIONHW         MAKE_DDHRESULT(290)
#define DDERR_NOSTRETCHHW          MAKE_DDHRESULT(310)
#define DDERR_NOZBUFFERHW          MAKE_DDHRESULT(340)
#define DDERR_SURFACEBUSY          MAKE_DDHRESULT(430)
#define DDERR_SURFACELOST          MAKE_DDHRESULT(450)
#define DDERR_UNSUPPORTEDMODE      MAKE_DDHRESULT(590)
#define DDERR_WASSTILLDRAWING      MAKE_DDHRESULT(540)
#define DDERR_WRONGMODE            MAKE_DDHRESULT(587)
#define DDERR_IMPLICITLYCREATED    MAKE_DDHRESULT(588)
#define DDERR_INCOMPATIBLEPRIMARY  MAKE_DDHRESULT(95)

typedef struct _DDSCAPS { DWORD dwCaps; } DDSCAPS;
typedef struct _DDCOLORKEY { DWORD dwColorSpaceLowValue; DWORD dwColorSpaceHighValue; } DDCOLORKEY, *LPDDCOLORKEY;
typedef struct _DDPIXELFORMAT { DWORD dwSize; DWORD dwFlags; DWORD dwFourCC; DWORD dwRGBBitCount; DWORD dwRBitMask; DWORD dwGBitMask; DWORD dwBBitMask; DWORD dwRGBAlphaBitMask; } DDPIXELFORMAT;
typedef struct _DDSURFACEDESC {
  DWORD         dwSize;
  DWORD         dwFlags;
  DWORD         dwHeight;
  DWORD         dwWidth;
  LONG          lPitch;
  DWORD         dwBackBufferCount;
  DWORD         dwRefreshRate;
  DWORD         dwAlphaBitDepth;
  DWORD         dwReserved;
  LPVOID        lpSurface;
  DDCOLORKEY    ddckCKDestOverlay;
  DDCOLORKEY    ddckCKDestBlt;
  DDCOLORKEY    ddckCKSrcOverlay;
  DDCOLORKEY    ddckCKSrcBlt;
  DDPIXELFORMAT ddpfPixelFormat;
  DDSCAPS       ddsCaps;
} DDSURFACEDESC, *LPDDSURFACEDESC;
typedef struct _DDBLTFX { DWORD dwSize; DWORD dwDDFX; DWORD dwROP; DWORD dwFillColor; DWORD reserved[20]; } DDBLTFX, *LPDDBLTFX;
typedef struct _DDCAPS { DWORD dwSize; DWORD dwCaps; DWORD reserved[80]; } DDCAPS, *LPDDCAPS;

struct IDirectDrawClipper : public IUnknown {
  virtual HRESULT SetHWnd(DWORD, HWND) = 0;
};
typedef IDirectDrawClipper *LPDIRECTDRAWCLIPPER;
struct IDirectDrawPalette : public IUnknown {
  virtual HRESULT GetEntries(DWORD, DWORD, DWORD, LPPALETTEENTRY) = 0;
  virtual HRESULT SetEntries(DWORD, DWORD, DWORD, LPPALETTEENTRY) = 0;
};
typedef IDirectDrawPalette *LPDIRECTDRAWPALETTE;
struct IDirectDrawSurface;
typedef IDirectDrawSurface *LPDIRECTDRAWSURFACE;
struct IDirectDrawSurface : public IUnknown {
  virtual HRESULT Blt(LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX) = 0;
  virtual HRESULT Flip(LPDIRECTDRAWSURFACE, DWORD) = 0;
  virtual HRESULT GetDC(HDC *) = 0;
  virtual HRESULT ReleaseDC(HDC) = 0;
  virtual HRESULT Lock(LPRECT, LPDDSURFACEDESC, DWORD, HANDLE) = 0;
  virtual HRESULT Unlock(LPVOID) = 0;
  virtual HRESULT Restore() = 0;
  virtual HRESULT SetClipper(LPDIRECTDRAWCLIPPER) = 0;
  virtual HRESULT SetColorKey(DWORD, LPDDCOLORKEY) = 0;
  virtual HRESULT SetPalette(LPDIRECTDRAWPALETTE) = 0;
  virtual HRESULT GetSurfaceDesc(LPDDSURFACEDESC) = 0;
};
struct IDirectDraw : public IUnknown {
  virtual HRESULT CreateClipper(DWORD, LPDIRECTDRAWCLIPPER *, IUnknown *) = 0;
  virtual HRESULT CreatePalette(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE *, IUnknown *) = 0;
  virtual HRESULT CreateSurface(LPDDSURFACEDESC, LPDIRECTDRAWSURFACE *, IUnknown *) = 0;
  virtual HRESULT GetCaps(LPDDCAPS, LPDDCAPS) = 0;
  virtual HRESULT RestoreDisplayMode() = 0;
  virtual HRESULT SetCooperativeLevel(HWND, DWORD) = 0;
  virtual HRESULT SetDisplayMode(DWORD, DWORD, DWORD) = 0;
};
typedef IDirectDraw *LPDIRECTDRAW;

/* ------------------------------ kernel types ----------------------------- */
typedef union _LARGE_INTEGER {
  struct { DWORD LowPart; LONG HighPart; };
  struct { DWORD LowPart; LONG HighPart; } u;
  int64_t QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;
typedef union _ULARGE_INTEGER {
  struct { DWORD LowPart; DWORD HighPart; };
  struct { DWORD LowPart; DWORD HighPart; } u;
  uint64_t QuadPart;
} ULARGE_INTEGER;

typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);

typedef struct _STARTUPINFO {
  DWORD  cb;
  LPSTR  lpReserved;
  LPSTR  lpDesktop;
  LPSTR  lpTitle;
  DWORD  dwX;
  DWORD  dwY;
  DWORD  dwXSize;
  DWORD  dwYSize;
  DWORD  dwXCountChars;
  DWORD  dwYCountChars;
  DWORD  dwFillAttribute;
  DWORD  dwFlags;
  WORD   wShowWindow;
  WORD   cbReserved2;
  LPBYTE lpReserved2;
  HANDLE hStdInput;
  HANDLE hStdOutput;
  HANDLE hStdError;
} STARTUPINFO, *LPSTARTUPINFO;

typedef struct _PROCESS_INFORMATION {
  HANDLE hProcess;
  HANDLE hThread;
  DWORD  dwProcessId;
  DWORD  dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef struct _MEMORYSTATUS {
  DWORD  dwLength;
  DWORD  dwMemoryLoad;
  size_t dwTotalPhys;
  size_t dwAvailPhys;
  size_t dwTotalPageFile;
  size_t dwAvailPageFile;
  size_t dwTotalVirtual;
  size_t dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

typedef struct _SYSTEM_INFO {
  WORD      wProcessorArchitecture;
  WORD      wReserved;
  DWORD     dwPageSize;
  LPVOID    lpMinimumApplicationAddress;
  LPVOID    lpMaximumApplicationAddress;
  DWORD_PTR dwActiveProcessorMask;
  DWORD     dwNumberOfProcessors;
  DWORD     dwProcessorType;
  DWORD     dwAllocationGranularity;
  WORD      wProcessorLevel;
  WORD      wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

typedef struct _OSVERSIONINFO {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  CHAR  szCSDVersion[128];
} OSVERSIONINFO, *LPOSVERSIONINFO;
#define VER_PLATFORM_WIN32s        0
#define VER_PLATFORM_WIN32_WINDOWS 1
#define VER_PLATFORM_WIN32_NT      2

#define THREAD_PRIORITY_LOWEST        (-2)
#define THREAD_PRIORITY_BELOW_NORMAL  (-1)
#define THREAD_PRIORITY_NORMAL        0
#define THREAD_PRIORITY_HIGHEST       2
#define THREAD_PRIORITY_ABOVE_NORMAL  1
#define THREAD_PRIORITY_TIME_CRITICAL 15
#define THREAD_PRIORITY_IDLE          (-15)
#define DETACHED_PROCESS              0x00000008
#define CREATE_NEW_PROCESS_GROUP      0x00000200
#define CREATE_NO_WINDOW              0x08000000
#define IDLE_PRIORITY_CLASS           0x00000040
#define NORMAL_PRIORITY_CLASS         0x00000020
#define HIGH_PRIORITY_CLASS           0x00000080
#define REALTIME_PRIORITY_CLASS       0x00000100
#define STARTF_USESHOWWINDOW          0x00000001
#define GENERIC_READ                  0x80000000
#define GENERIC_WRITE                 0x40000000
#define FILE_SHARE_READ               0x00000001
#define FILE_SHARE_WRITE              0x00000002
#define CREATE_NEW                    1
#define CREATE_ALWAYS                 2
#define OPEN_EXISTING                 3
#define OPEN_ALWAYS                   4
#define TRUNCATE_EXISTING             5
#define WAIT_ABANDONED                0x00000080
#define STILL_ACTIVE                  0x00000103
#define ERROR_FILE_EXISTS             80
#define ERROR_TIMEOUT                 1460

/* ------------------------------- winsock --------------------------------- */
#define WSADESCRIPTION_LEN 256
#define WSASYS_STATUS_LEN  128
typedef struct WSAData {
  WORD   wVersion;
  WORD   wHighVersion;
  char   szDescription[WSADESCRIPTION_LEN+1];
  char   szSystemStatus[WSASYS_STATUS_LEN+1];
  unsigned short iMaxSockets;
  unsigned short iMaxUdpDg;
  char  *lpVendorInfo;
} WSADATA, *LPWSADATA;
typedef uintptr_t SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR    (-1)
#define WSABASEERR      10000
#define WSAEWOULDBLOCK  (WSABASEERR+35)
#define WSAEINPROGRESS  (WSABASEERR+36)
#define WSAECONNRESET   (WSABASEERR+54)
#define WSAENOTCONN     (WSABASEERR+57)
int  WSAStartup(WORD, LPWSADATA);
int  WSACleanup(void);
int  WSAGetLastError(void);

/* =============================== functions =============================== */

/* ---- threads, synchronisation, time ---- */
/* The C runtime's _beginthread/_endthread are emulated too (threads must be
   known to the emulation).  On Windows hosts the real CRT declarations are
   seen first and the program's calls are renamed to the emulation's. */
#ifdef _WIN32
#  include <process.h>
#endif
#define _beginthread sf_beginthread
#define _endthread   sf_endthread
uintptr_t _beginthread(void (*)(void *), unsigned, void *);
void      _endthread(void);
HANDLE  CreateThread(void *, size_t, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
void    ExitThread(DWORD);
BOOL    TerminateThread(HANDLE, DWORD);
DWORD   SuspendThread(HANDLE);
DWORD   ResumeThread(HANDLE);
BOOL    SetThreadPriority(HANDLE, int);
int     GetThreadPriority(HANDLE);
DWORD   GetCurrentThreadId(void);
DWORD   GetCurrentProcessId(void);
HANDLE  GetCurrentThread(void);
HANDLE  CreateMutex(void *, BOOL, LPCSTR);
BOOL    ReleaseMutex(HANDLE);
HANDLE  CreateEvent(void *, BOOL, BOOL, LPCSTR);
BOOL    SetEvent(HANDLE);
BOOL    ResetEvent(HANDLE);
BOOL    PulseEvent(HANDLE);
DWORD   WaitForSingleObject(HANDLE, DWORD);
DWORD   WaitForMultipleObjects(DWORD, const HANDLE *, BOOL, DWORD);
BOOL    QueryPerformanceCounter(LARGE_INTEGER *);
BOOL    QueryPerformanceFrequency(LARGE_INTEGER *);
BOOL    CreateProcess(LPCSTR, LPSTR, void *, void *, BOOL, DWORD, LPVOID,
                      LPCSTR, LPSTARTUPINFO, LPPROCESS_INFORMATION);
BOOL    TerminateProcess(HANDLE, UINT);
BOOL    GetExitCodeProcess(HANDLE, LPDWORD);
BOOL    SetPriorityClass(HANDLE, DWORD);
void    GlobalMemoryStatus(LPMEMORYSTATUS);
void    GetSystemInfo(LPSYSTEM_INFO);
UINT    GetSystemDirectory(LPSTR, UINT);
UINT    GetWindowsDirectory(LPSTR, UINT);
BOOL    GetVersionEx(LPOSVERSIONINFO);
HANDLE  CreateFile(LPCSTR, DWORD, DWORD, void *, DWORD, DWORD, HANDLE);
BOOL    GetFileTime(HANDLE, FILETIME *, FILETIME *, FILETIME *);
BOOL    SetFileTime(HANDLE, const FILETIME *, const FILETIME *, const FILETIME *);
BOOL    SetFileAttributes(LPCSTR, DWORD);
DWORD   GetFileAttributes(LPCSTR);
BOOL    CopyFile(LPCSTR, LPCSTR, BOOL);
BOOL    DeleteFile(LPCSTR);
BOOL    MoveFile(LPCSTR, LPCSTR);
DWORD   GetCurrentDirectory(DWORD, LPSTR);
BOOL    SetCurrentDirectory(LPCSTR);

/* ---- kernel ---- */
DWORD   GetLastError(void);
void    SetLastError(DWORD);
DWORD   GetVersion(void);
DWORD   GetTickCount(void);
void    Sleep(DWORD);
HMODULE LoadLibrary(LPCSTR);
BOOL    FreeLibrary(HMODULE);
FARPROC GetProcAddress(HMODULE, LPCSTR);
DWORD   GetModuleFileName(HMODULE, LPSTR, DWORD);
HANDLE  GetCurrentProcess(void);
BOOL    SetEnvironmentVariable(LPCSTR, LPCSTR);
HGLOBAL GlobalAlloc(UINT, size_t);
HGLOBAL GlobalFree(HGLOBAL);
LPVOID  GlobalLock(HGLOBAL);
BOOL    GlobalUnlock(HGLOBAL);
BOOL    FreeResource(HGLOBAL);
BOOL    CloseHandle(HANDLE);
HANDLE  FindFirstFile(LPCSTR, LPWIN32_FIND_DATA);
BOOL    FindNextFile(HANDLE, LPWIN32_FIND_DATA);
BOOL    FindClose(HANDLE);
BOOL    CreateDirectory(LPCSTR, void *);
BOOL    RemoveDirectory(LPCSTR);
BOOL    OpenProcessToken(HANDLE, DWORD, PHANDLE);
BOOL    LookupPrivilegeValue(LPCSTR, LPCSTR, PLUID);
BOOL    AdjustTokenPrivileges(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, LPDWORD);
BOOL    ExitWindowsEx(UINT, DWORD);
void    OutputDebugString(LPCSTR);

/* ---- shell ---- */
BOOL      Shell_NotifyIcon(DWORD, PNOTIFYICONDATA);
void      SHChangeNotify(LONG, UINT, LPCVOID, LPCVOID);
HINSTANCE ShellExecute(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);

/* ---- multimedia ---- */
MCIERROR mciSendCommand(MCIDEVICEID, UINT, DWORD_PTR, DWORD_PTR);
MCIERROR mciSendString(LPCSTR, LPSTR, UINT, HWND);
DWORD    timeGetTime(void);

/* ---- windows ---- */
ATOM    RegisterClass(const WNDCLASS *);
BOOL    UnregisterClass(LPCSTR, HINSTANCE);
HWND    CreateWindowEx(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int,
                       HWND, HMENU, HINSTANCE, LPVOID);
#define CreateWindow(c,n,s,x,y,w,h,p,m,i,l) CreateWindowEx(0,c,n,s,x,y,w,h,p,m,i,l)
BOOL    DestroyWindow(HWND);
BOOL    IsWindow(HWND);
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CallWindowProc(WNDPROC, HWND, UINT, WPARAM, LPARAM);
LRESULT SendMessage(HWND, UINT, WPARAM, LPARAM);
BOOL    PostMessage(HWND, UINT, WPARAM, LPARAM);
BOOL    PostThreadMessage(DWORD, UINT, WPARAM, LPARAM);
void    PostQuitMessage(int);
BOOL    GetMessage(LPMSG, HWND, UINT, UINT);
BOOL    PeekMessage(LPMSG, HWND, UINT, UINT, UINT);
BOOL    WaitMessage(void);
BOOL    TranslateMessage(const MSG *);
LRESULT DispatchMessage(const MSG *);
BOOL    IsDialogMessage(HWND, LPMSG);
DWORD   GetMessageTime(void);
UINT_PTR SetTimer(HWND, UINT_PTR, UINT, void *);
BOOL    KillTimer(HWND, UINT_PTR);
DWORD   GetMessagePos(void);
LONG_PTR GetWindowLong(HWND, int);
LONG_PTR SetWindowLong(HWND, int, LONG_PTR);
#define GetWindowLongPtr GetWindowLong
#define SetWindowLongPtr SetWindowLong
#define GWLP_USERDATA GWL_USERDATA
ULONG_PTR GetClassLong(HWND, int);
ULONG_PTR SetClassLong(HWND, int, LONG_PTR);
int     GetClassName(HWND, LPSTR, int);
BOOL    GetClientRect(HWND, LPRECT);
BOOL    GetWindowRect(HWND, LPRECT);
BOOL    ClientToScreen(HWND, LPPOINT);
BOOL    ScreenToClient(HWND, LPPOINT);
int     MapWindowPoints(HWND, HWND, LPPOINT, UINT);
BOOL    AdjustWindowRect(LPRECT, DWORD, BOOL);
BOOL    AdjustWindowRectEx(LPRECT, DWORD, BOOL, DWORD);
BOOL    SetWindowPos(HWND, HWND, int, int, int, int, UINT);
BOOL    MoveWindow(HWND, int, int, int, int, BOOL);
BOOL    SetWindowPlacement(HWND, const WINDOWPLACEMENT *);
BOOL    GetWindowPlacement(HWND, WINDOWPLACEMENT *);
BOOL    ShowWindow(HWND, int);
BOOL    IsWindowVisible(HWND);
BOOL    IsIconic(HWND);
BOOL    IsZoomed(HWND);
BOOL    EnableWindow(HWND, BOOL);
BOOL    IsWindowEnabled(HWND);
HWND    SetFocus(HWND);
HWND    GetFocus(void);
HWND    SetActiveWindow(HWND);
HWND    GetActiveWindow(void);
BOOL    SetForegroundWindow(HWND);
HWND    GetForegroundWindow(void);
HWND    SetCapture(HWND);
BOOL    ReleaseCapture(void);
HWND    GetCapture(void);
HWND    GetParent(HWND);
HWND    SetParent(HWND, HWND);
HWND    GetWindow(HWND, UINT);
HWND    GetTopWindow(HWND);
#define GetNextWindow(h,c) GetWindow(h,c)
HWND    GetDesktopWindow(void);
HWND    GetDlgItem(HWND, int);
int     GetDlgCtrlID(HWND);
HWND    WindowFromPoint(POINT);
HWND    ChildWindowFromPoint(HWND, POINT);
BOOL    IsChild(HWND, HWND);
BOOL    BringWindowToTop(HWND);
BOOL    SetWindowText(HWND, LPCSTR);
int     GetWindowText(HWND, LPSTR, int);
int     GetWindowTextLength(HWND);
BOOL    InvalidateRect(HWND, const RECT *, BOOL);
BOOL    ValidateRect(HWND, const RECT *);
BOOL    UpdateWindow(HWND);
BOOL    RedrawWindow(HWND, const RECT *, HRGN, UINT);
HDC     BeginPaint(HWND, LPPAINTSTRUCT);
BOOL    EndPaint(HWND, const PAINTSTRUCT *);
HDC     GetDC(HWND);
HDC     GetWindowDC(HWND);
int     ReleaseDC(HWND, HDC);
BOOL    GetCursorPos(LPPOINT);
BOOL    SetCursorPos(int, int);
HCURSOR SetCursor(HCURSOR);
HCURSOR LoadCursor(HINSTANCE, LPCSTR);
int     ShowCursor(BOOL);
HICON   LoadIcon(HINSTANCE, LPCSTR);
HICON   CreateIcon(HINSTANCE, int, int, BYTE, BYTE, const BYTE *, const BYTE *);
HICON   CreateIconIndirect(PICONINFO);
BOOL    DestroyIcon(HICON);
BOOL    DrawIcon(HDC, int, int, HICON);
BOOL    DrawIconEx(HDC, int, int, HICON, int, int, UINT, HBRUSH, UINT);
int     GetSystemMetrics(int);
BOOL    SystemParametersInfo(UINT, UINT, LPVOID, UINT);
DWORD   GetSysColor(int);
HBRUSH  GetSysColorBrush(int);
SHORT   GetKeyState(int);
SHORT   GetAsyncKeyState(int);
BOOL    MessageBeep(UINT);
int     MessageBox(HWND, LPCSTR, LPCSTR, UINT);
LONG    ChangeDisplaySettings(LPDEVMODE, DWORD);
BOOL    SetRect(LPRECT, int, int, int, int);
BOOL    SetRectEmpty(LPRECT);
BOOL    CopyRect(LPRECT, const RECT *);
BOOL    InflateRect(LPRECT, int, int);
BOOL    OffsetRect(LPRECT, int, int);
BOOL    IntersectRect(LPRECT, const RECT *, const RECT *);
BOOL    UnionRect(LPRECT, const RECT *, const RECT *);
BOOL    IsRectEmpty(const RECT *);
BOOL    PtInRect(const RECT *, POINT);
BOOL    EqualRect(const RECT *, const RECT *);

/* ---- scroll bars ---- */
int     SetScrollPos(HWND, int, int, BOOL);
int     GetScrollPos(HWND, int);
BOOL    SetScrollRange(HWND, int, int, int, BOOL);
BOOL    GetScrollRange(HWND, int, LPINT, LPINT);
int     SetScrollInfo(HWND, int, LPCSCROLLINFO, BOOL);
BOOL    GetScrollInfo(HWND, int, LPSCROLLINFO);
BOOL    ShowScrollBar(HWND, int, BOOL);
BOOL    EnableScrollBar(HWND, UINT, UINT);

/* ---- menus ---- */
HMENU   CreateMenu(void);
HMENU   CreatePopupMenu(void);
BOOL    DestroyMenu(HMENU);
BOOL    IsMenu(HMENU);
BOOL    AppendMenu(HMENU, UINT, UINT_PTR, LPCSTR);
BOOL    InsertMenu(HMENU, UINT, UINT, UINT_PTR, LPCSTR);
BOOL    InsertMenuItem(HMENU, UINT, BOOL, LPCMENUITEMINFO);
BOOL    SetMenuItemInfo(HMENU, UINT, BOOL, LPCMENUITEMINFO);
BOOL    GetMenuItemInfo(HMENU, UINT, BOOL, LPMENUITEMINFO);
BOOL    RemoveMenu(HMENU, UINT, UINT);
BOOL    DeleteMenu(HMENU, UINT, UINT);
DWORD   CheckMenuItem(HMENU, UINT, UINT);
BOOL    EnableMenuItem(HMENU, UINT, UINT);
UINT    GetMenuState(HMENU, UINT, UINT);
int     GetMenuString(HMENU, UINT, LPSTR, int, UINT);
int     GetMenuItemCount(HMENU);
UINT    GetMenuItemID(HMENU, int);
HMENU   GetSubMenu(HMENU, int);
BOOL    SetMenu(HWND, HMENU);
HMENU   GetMenu(HWND);
BOOL    DrawMenuBar(HWND);
HMENU   GetSystemMenu(HWND, BOOL);
BOOL    TrackPopupMenu(HMENU, UINT, int, int, int, HWND, const RECT *);

/* ---- GDI ---- */
HDC     CreateDC(LPCSTR, LPCSTR, LPCSTR, const void *);
HDC     CreateCompatibleDC(HDC);
BOOL    DeleteDC(HDC);
int     SaveDC(HDC);
BOOL    RestoreDC(HDC, int);
int     GetDeviceCaps(HDC, int);
HGDIOBJ SelectObject(HDC, HGDIOBJ);
BOOL    DeleteObject(HGDIOBJ);
HGDIOBJ GetStockObject(int);
int     GetObject(HGDIOBJ, int, LPVOID);
HPEN    CreatePen(int, int, COLORREF);
HBRUSH  CreateSolidBrush(COLORREF);
HFONT   CreateFont(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD,
                   DWORD, DWORD, DWORD, DWORD, LPCSTR);
HFONT   CreateFontIndirect(const LOGFONT *);
HBITMAP CreateCompatibleBitmap(HDC, int, int);
HBITMAP CreateBitmap(int, int, UINT, UINT, const void *);
HBITMAP CreateDIBitmap(HDC, const BITMAPINFOHEADER *, DWORD, const void *,
                       const BITMAPINFO *, UINT);
HBITMAP CreateDIBSection(HDC, const BITMAPINFO *, UINT, void **, HANDLE, DWORD);
int     GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, LPBITMAPINFO, UINT);
int     SetDIBits(HDC, HBITMAP, UINT, UINT, const void *, const BITMAPINFO *, UINT);
int     StretchDIBits(HDC, int, int, int, int, int, int, int, int,
                      const void *, const BITMAPINFO *, UINT, DWORD);
int     SetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int, int, UINT, UINT,
                          const void *, const BITMAPINFO *, UINT);
BOOL    BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);
BOOL    StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
BOOL    PatBlt(HDC, int, int, int, int, DWORD);
int     SetStretchBltMode(HDC, int);
int     SetBkMode(HDC, int);
int     GetBkMode(HDC);
COLORREF SetBkColor(HDC, COLORREF);
COLORREF GetBkColor(HDC);
COLORREF SetTextColor(HDC, COLORREF);
COLORREF GetTextColor(HDC);
UINT    SetTextAlign(HDC, UINT);
UINT    GetTextAlign(HDC);
int     SetROP2(HDC, int);
BOOL    TextOut(HDC, int, int, LPCSTR, int);
BOOL    ExtTextOut(HDC, int, int, UINT, const RECT *, LPCSTR, UINT, const INT *);
int     DrawText(HDC, LPCSTR, int, LPRECT, UINT);
BOOL    GetTextExtentPoint32(HDC, LPCSTR, int, LPSIZE);
#define GetTextExtentPoint GetTextExtentPoint32
BOOL    GetTextMetrics(HDC, LPTEXTMETRIC);
BOOL    MoveToEx(HDC, int, int, LPPOINT);
BOOL    LineTo(HDC, int, int);
BOOL    Polyline(HDC, const POINT *, int);
BOOL    Polygon(HDC, const POINT *, int);
BOOL    Rectangle(HDC, int, int, int, int);
BOOL    RoundRect(HDC, int, int, int, int, int, int);
BOOL    Ellipse(HDC, int, int, int, int);
BOOL    Arc(HDC, int, int, int, int, int, int, int, int);
BOOL    Pie(HDC, int, int, int, int, int, int, int, int);
BOOL    Chord(HDC, int, int, int, int, int, int, int, int);
int     FillRect(HDC, const RECT *, HBRUSH);
int     FrameRect(HDC, const RECT *, HBRUSH);
BOOL    InvertRect(HDC, const RECT *);
BOOL    DrawFocusRect(HDC, const RECT *);
BOOL    DrawEdge(HDC, LPRECT, UINT, UINT);
COLORREF SetPixel(HDC, int, int, COLORREF);
COLORREF GetPixel(HDC, int, int);
int     SelectClipRgn(HDC, HRGN);
int     IntersectClipRect(HDC, int, int, int, int);
int     ExcludeClipRect(HDC, int, int, int, int);
int     GetClipBox(HDC, LPRECT);
HRGN    CreateRectRgn(int, int, int, int);
HPALETTE CreatePalette(const LOGPALETTE *);
HPALETTE SelectPalette(HDC, HPALETTE, BOOL);
UINT    RealizePalette(HDC);
UINT    GetPaletteEntries(HPALETTE, UINT, UINT, LPPALETTEENTRY);
UINT    SetPaletteEntries(HPALETTE, UINT, UINT, const PALETTEENTRY *);
UINT    GetSystemPaletteEntries(HDC, UINT, UINT, LPPALETTEENTRY);
BOOL    GdiFlush(void);

#define EDGE_RAISED  (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#define EDGE_SUNKEN  (BDR_SUNKENOUTER | BDR_SUNKENINNER)
#define EDGE_ETCHED  (BDR_SUNKENOUTER | BDR_RAISEDINNER)
#define EDGE_BUMP    (BDR_RAISEDOUTER | BDR_SUNKENINNER)
#define BDR_RAISEDOUTER 0x0001
#define BDR_SUNKENOUTER 0x0002
#define BDR_RAISEDINNER 0x0004
#define BDR_SUNKENINNER 0x0008
#define BF_LEFT    0x0001
#define BF_TOP     0x0002
#define BF_RIGHT   0x0004
#define BF_BOTTOM  0x0008
#define BF_RECT    (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)
#define BF_MIDDLE  0x0800
#define BF_SOFT    0x1000
#define BF_ADJUST  0x2000
#define BF_FLAT    0x4000
#define BF_MONO    0x8000

#define RDW_INVALIDATE 0x0001
#define RDW_INTERNALPAINT 0x0002
#define RDW_ERASE      0x0004
#define RDW_VALIDATE   0x0008
#define RDW_NOINTERNALPAINT 0x0010
#define RDW_NOERASE    0x0020
#define RDW_NOCHILDREN 0x0040
#define RDW_ALLCHILDREN 0x0080
#define RDW_UPDATENOW  0x0100
#define RDW_ERASENOW   0x0200
#define RDW_FRAME      0x0400
#define RDW_NOFRAME    0x0800

#define ESB_ENABLE_BOTH   0x0000
#define ESB_DISABLE_BOTH  0x0003

#include "sf_msvcrt.h"

#endif /* SFWIN_H */
