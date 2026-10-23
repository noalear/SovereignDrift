/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
#ifndef S_MD5_H_
#define S_MD5_H_

#include "S_String.h"

//
// From global.h
//
typedef unsigned char *POINTER;
typedef unsigned short int UINT2;
typedef ulong UINT4;
#define PROTO_LIST(list) list

//
// MD5 context.
//
typedef struct {
  UINT4 state[4];                    // state (ABCD)
  UINT4 count[2];                    // number of bits, modulo 2^64 (lsb first)
  unsigned char buffer[64];          // input buffer
} MD5_CTX;

void MD5Init PROTO_LIST ((MD5_CTX *));
void MD5Update PROTO_LIST((MD5_CTX *, unsigned char *, unsigned int));
void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *));

class S_MD5 
{
 public:

  unsigned char digest[16];

  int Init();
  int Create(unsigned char * =NULL,int=0);
  int Create(char * =NULL);
  int Destroy();

  int CreateFromFile(char *,int=0,int=0,ulong * =NULL);

  int operator=(S_MD5 &);
  int operator==(S_MD5 &);
  int operator!=(S_MD5 &);
  int Equals(S_MD5 &);

  char *Str();

  S_MD5(char * =NULL);
  S_MD5(unsigned char *,int);
  virtual ~S_MD5();
};

#endif // S_MD5_H_

