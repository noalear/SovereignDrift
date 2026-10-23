/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_PLATFORM_INCLUDED
#define S_PLATFORM_INCLUDED

/* ************************************************************************ */
/* *** SFHOST: Godot-hosted platform for the Stellar Frontier port.     *** */
/* *** Added 2026-10-12, revised 2026-10-12.                            *** */
/* *** The reference build of Stellar Frontier 1.10a is Win32 + Visual  *** */
/* *** C++, so SFHOST presents exactly that configuration to the code:  *** */
/* *** WIN32 and MSVCPP are defined and the Win32 API is provided by    *** */
/* *** the port's emulation layer (src/w32emu/sfwin.h) on every OS.     *** */
/* ************************************************************************ */
#ifdef SFHOST
#  ifndef WIN32
#    define WIN32
#  endif
#  define MSVCPP
#  define PLATFORM "Microsoft Win32 API Windows95/NT"
#  define COMPILER "Microsoft Visual C++"
#  ifdef _WIN32
#     define SFHOST_WINDOWS   /* host OS is Windows */
#  else
#     define SFHOST_POSIX     /* host OS is Linux/macOS */
#  endif
#else  /* !SFHOST : original platform detection follows unchanged */

#if defined (__WINDOWS__) && defined(__IBMCPP__)
#define WIN32
#define IBMCPP
#define PLATFORM "Microsoft Win32 API Windows95/NT"
#define COMPILER "IBM VisualAge C++ for Windows"
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#  ifndef WIN32
#    define WIN32
#  endif
#  define MSVCPP
#  define PLATFORM "Microsoft Win32 API Windows95/NT"
#  define COMPILER "Microsoft Visual C++"
#endif

#if defined(__OS2__) && defined(__IBMCPP__)
#  define OS2
#  define IBMCPP
#  define IBMTCPIP
#  define IBMMMOS2
#  define PLATFORM "IBM OS/2 Presentation Manager API"
#  define COMPILER "IBM VisualAge C++ for OS/2"
#endif

#if defined(OS2_MODE) && defined(__GNUC__)
#  define OS2
#  define GNUCPP
#  define PLATFORM "IBM OS/2 Presentation Manager API"
#  define COMPILER "GNU C/C++ Compiler for OS/2"
#  ifdef GNUCPPOMF
#     define IBMTCPIP
/*#     define IBMMMOS2*/
#  endif
#endif

#if defined(__unix__) || defined(__unix) || defined(unix)
#  ifdef __GNUC__
#     define GNUCPP  /* *** GNU C/C++ compiler *** */
#     define COMPILER "GNU C/C++ Compiler for UNIX"
#  endif
#  define UNIX
#  define XWIN
#  define GTK
#  define PTHREAD
#endif

#ifdef __sgi
#  define SGI
#  define PLATFORM "SGI IRIX"
#  define COMPILER "SGI IRIX CC"
#  define ULONG_MAX (1<<31)
#endif

#ifdef __linux__
#  define LINUX                  /* *** LINUX OS       *** */
#  define PLATFORM "Linux"
#endif

#ifdef __FreeBSD__
#  define BSD                  /* *** FreeBSD OS       *** */
#  define PLATFORM "FreeBSD"
#endif

#endif /* !SFHOST */

#endif
