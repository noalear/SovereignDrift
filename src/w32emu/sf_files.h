/* ************************************************************************* */
/* sf_files.h -- Windows file semantics for the original program.            */
/*                                                                           */
/* NEW FILE, added 2026-10-12 for the Stellar Frontier Godot port.           */
/*                                                                           */
/* 1. File names.  The C and C++ file opening entry points used by the game */
/*    are routed through sf_hostpath(), which maps the program's own .exe to */
/*    its zap archive and the fake install root to the real one, and on     */
/*    POSIX hosts turns '\' into '/', "nul" into "/dev/null" and resolves   */
/*    each path component case-insensitively (Windows paths such as         */
/*    "..\dat\server.dat" are written throughout the program).             */
/* 2. Text mode (POSIX hosts only).  Files opened for reading in text mode   */
/*    are read as the Microsoft C runtime reads them: CR LF becomes LF and   */
/*    Ctrl-Z ends the file (the shipped .prf profiles have CR LF endings).   */
/* 3. iostream behaviour (all hosts).  The reference build used Visual C++'s */
/*    C++98 library, where istream::putback() and seekg() leave eofbit set.  */
/*    C++11 libraries clear eofbit first, which changes what the data-file   */
/*    readers (trek.cpp, construct.cpp) see at the end of each file.         */
/* ************************************************************************* */
#ifndef SF_FILES_H
#define SF_FILES_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Translate a path as the Win32 program wrote it into a host path.  Always
   returns buf (which may be the translated copy of path). */
const char *sf_hostpath(const char *path, char *buf, int buflen);
FILE *sf_fopen(const char *path, const char *mode);
/* POSIX: if hostpath needs text-mode translation, make a translated copy and
   return a descriptor for it (writing its /proc path into buf); else -1. */
int  sf_text_copy(const char *hostpath, char *buf, int buflen);
void sf_text_done(int fd);
#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#  include <fstream>

/* C++98 (Visual C++ 6) semantics for the unformatted input functions whose
   meaning changed in C++11. */
#define SF_CXX98_ISTREAM(Self, Base)                                             \
  Self &putback(char c) {                                                    \
    if (this->rdstate() & std::ios_base::eofbit) {                           \
      this->setstate(std::ios_base::failbit);                                \
      return *this;                                                          \
    }                                                                        \
    Base::putback(c);                                                        \
    return *this;                                                            \
  }                                                                          \
  Self &unget() {                                                            \
    if (this->rdstate() & std::ios_base::eofbit) {                           \
      this->setstate(std::ios_base::failbit);                                \
      return *this;                                                          \
    }                                                                        \
    Base::unget();                                                           \
    return *this;                                                            \
  }                                                                          \
  Self &seekg(std::streampos p) {                                            \
    std::ios_base::iostate eof = this->rdstate() & std::ios_base::eofbit;    \
    Base::seekg(p);                                                          \
    this->setstate(eof);                                                     \
    return *this;                                                            \
  }                                                                          \
  Self &seekg(std::streamoff o, std::ios_base::seekdir d) {                  \
    std::ios_base::iostate eof = this->rdstate() & std::ios_base::eofbit;    \
    Base::seekg(o, d);                                                       \
    this->setstate(eof);                                                     \
    return *this;                                                            \
  }

#define SF_HOSTPATH(p, b) sf_hostpath((p), (b), sizeof(b))

class sf_ifstream : public std::ifstream {
public:
  sf_ifstream() {}
  explicit sf_ifstream(const char *p,
                       std::ios_base::openmode m = std::ios_base::in) {
    open(p,m);
  }
  void open(const char *p, std::ios_base::openmode m = std::ios_base::in) {
    char b[1024];
    const char *hp = SF_HOSTPATH(p, b);
#ifdef SFHOST_POSIX
    if (!(m & std::ios_base::binary)) {
      char t[64];
      int fd = sf_text_copy(hp, t, sizeof(t));
      if (fd >= 0) {
        std::ifstream::open(t, m);
        sf_text_done(fd);
        return;
      }
    }
#endif
    std::ifstream::open(hp, m);
  }
  SF_CXX98_ISTREAM(sf_ifstream, std::ifstream)
};

class sf_ofstream : public std::ofstream {
public:
  sf_ofstream() {}
  explicit sf_ofstream(const char *p,
                       std::ios_base::openmode m = std::ios_base::out) {
    open(p,m);
  }
  void open(const char *p, std::ios_base::openmode m = std::ios_base::out) {
    char b[1024];
    std::ofstream::open(SF_HOSTPATH(p, b),m);
  }
};

class sf_fstream : public std::fstream {
public:
  sf_fstream() {}
  explicit sf_fstream(const char *p,
                      std::ios_base::openmode m = std::ios_base::in|std::ios_base::out) {
    open(p,m);
  }
  void open(const char *p,
            std::ios_base::openmode m = std::ios_base::in|std::ios_base::out) {
    char b[1024];
    std::fstream::open(SF_HOSTPATH(p, b),m);
  }
  SF_CXX98_ISTREAM(sf_fstream, std::fstream)
};

#  define ifstream sf_ifstream
#  define ofstream sf_ofstream
#  define fstream  sf_fstream
#endif
#define fopen(p,m) sf_fopen((p),(m))

#endif /* SF_FILES_H */
