/* ************************************************************************* */
/* sf_msvcrt.h -- Microsoft C runtime behaviour the 1997 code depends on.    */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* The reference build used Visual C++ with the multithreaded runtime.       */
/* Where that runtime differs from the host's C library in ways the game can */
/* observe, the MSVC behaviour is reproduced here:                           */
/*   - rand()/srand(): MSVC's LCG, RAND_MAX 0x7FFF, one state per thread    */
/*   - clock(): wall-clock milliseconds since start-up, CLOCKS_PER_SEC 1000 */
/*   - printf/scanf family on LP64 hosts: "%ld"-style conversions refer to  */
/*     32-bit integers (long is 32 bits on Win32), see sf_fmt below.        */
/*   - strcpy/strcat/strncpy/memcpy with overlapping arguments: the program */
/*     does e.g. strcpy(s, s+1) (trek.cpp, design module lists).  MSVC's    */
/*     routines copy front to back (memcpy is memmove), so that works on    */
/*     Windows; glibc's vectorised versions corrupt the string.             */
/* ************************************************************************* */
#ifndef SF_MSVCRT_H
#define SF_MSVCRT_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
int     sf_msvc_rand(void);
void    sf_msvc_srand(unsigned int);
clock_t sf_msvc_clock(void);
#ifdef __cplusplus
}
#endif

#undef  RAND_MAX
#define RAND_MAX 0x7fff
#define rand()   sf_msvc_rand()
#define srand(s) sf_msvc_srand(s)
#define clock()  sf_msvc_clock()
#undef  CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000
#ifndef CLK_TCK
#  define CLK_TCK CLOCKS_PER_SEC
#endif

/* exit() ends the program (its main thread), not the host application */
#ifdef __cplusplus
extern "C" void sf_exit(int);
#else
void sf_exit(int);
#endif
#define exit(c) sf_exit(c)

#define _snprintf  snprintf
#define _vsnprintf vsnprintf

#if !defined(W32_INTERNAL_H)
#include <string.h>
#ifdef __cplusplus
#include <cstring>
extern "C" {
#endif
char *sf_msvc_strcpy(char *, const char *);
char *sf_msvc_strcat(char *, const char *);
char *sf_msvc_strncpy(char *, const char *, size_t);
char *sf_msvc_strncat(char *, const char *, size_t);
void *sf_msvc_memcpy(void *, const void *, size_t);
#ifdef __cplusplus
}
namespace std {
using ::sf_msvc_strcpy; using ::sf_msvc_strcat; using ::sf_msvc_strncpy;
using ::sf_msvc_strncat; using ::sf_msvc_memcpy;
}
#endif
#define strcpy(d, s)     sf_msvc_strcpy(d, s)
#define strcat(d, s)     sf_msvc_strcat(d, s)
#define strncpy(d, s, n) sf_msvc_strncpy(d, s, n)
#define strncat(d, s, n) sf_msvc_strncat(d, s, n)
#define memcpy(d, s, n)  sf_msvc_memcpy(d, s, n)
#endif

#if defined(__cplusplus) && (defined(__LP64__) || defined(_LP64))
/* On LP64 hosts 'long' is 64 bits but every long in this program is 32 bits
   (see tools/fix_long32.py), so the "l" size prefix in printf/scanf format
   strings must be dropped for integer conversions.  "ll" and floating point
   conversions are left alone. */
extern "C" {
int sf_printf(const char *, ...);
int sf_fprintf(FILE *, const char *, ...);
int sf_sprintf(char *, const char *, ...);
int sf_snprintf(char *, size_t, const char *, ...);
int sf_vsprintf(char *, const char *, va_list);
int sf_vsnprintf(char *, size_t, const char *, va_list);
int sf_vfprintf(FILE *, const char *, va_list);
int sf_sscanf(const char *, const char *, ...);
int sf_fscanf(FILE *, const char *, ...);
}
#  define printf    sf_printf
#  define fprintf   sf_fprintf
#  define sprintf   sf_sprintf
#  undef  _snprintf
#  define _snprintf sf_snprintf
#  define snprintf  sf_snprintf
#  define vsprintf  sf_vsprintf
#  undef  _vsnprintf
#  define _vsnprintf sf_vsnprintf
#  define vsnprintf sf_vsnprintf
#  define vfprintf  sf_vfprintf
#  define sscanf    sf_sscanf
#  define fscanf    sf_fscanf
#endif

#endif /* SF_MSVCRT_H */
