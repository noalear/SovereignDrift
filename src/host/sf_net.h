/* ************************************************************************* */
/* sf_net.h  --  Portable socket shim for the Stellar Frontier Godot port.   */
/*                                                                           */
/* Added 2026-10-12 (new file, not part of the original Stardock release).   */
/*                                                                           */
/* The 1997 slib code calls BSD socket functions directly.  On Windows those */
/* live in winsock2.h, which drags in <windows.h> and clashes with slib's    */
/* own HWND/MSG/HDC typedefs.  This header provides the *types* the original */
/* code uses (sockaddr_in, hostent, ...) with no system headers at all; the  */
/* real calls are made in sf_net.cpp, the only file that sees the OS socket  */
/* headers.  Layouts here are private to the port: sf_net.cpp converts.      */
/* ************************************************************************* */
#ifndef SF_NET_H
#define SF_NET_H

#include <stdint.h>
#include <stddef.h>

#ifndef SF_NET_IMPL

struct in_addr { uint32_t s_addr; };            /* network byte order */

struct sockaddr {
  unsigned short sa_family;
  char sa_data[14];
};

struct sockaddr_in {
  unsigned short sin_family;
  uint16_t       sin_port;                       /* network byte order */
  struct in_addr sin_addr;
  char           sin_zero[8];
};

struct hostent {
  char  *h_name;
  char **h_aliases;
  short  h_addrtype;
  short  h_length;
  char **h_addr_list;
};
#define h_addr h_addr_list[0]

#endif /* SF_NET_IMPL */

/* Constants: values are the port's own; sf_net.cpp maps them to the OS. */
#define SF_AF_INET        2
#define SF_SOCK_STREAM    1
#define SF_SOCK_DGRAM     2
#define SF_SOCK_RAW       3
#define SF_IPPROTO_IP     0
#define SF_IPPROTO_ICMP   1
#define SF_IPPROTO_TCP    6
#define SF_IPPROTO_UDP   17
#define SF_SOL_SOCKET     0xFFFF
#define SF_SO_DEBUG       0x0001
#define SF_SO_REUSEADDR   0x0004
#define SF_SO_DONTROUTE   0x0010
#define SF_SO_SNDBUF      0x1001
#define SF_SO_RCVBUF      0x1002
#define SF_TCP_NODELAY    0x0001
#define SF_MSG_PEEK       0x2
#define SF_FIONREAD       0x541B
#define SF_FIONBIO        0x5421
#define SF_INADDR_ANY     ((uint32_t)0x00000000)
#define SF_INADDR_NONE    ((uint32_t)0xFFFFFFFF)

#ifdef __cplusplus
extern "C" {
#endif

int       sf_net_init(void);
int       sf_socket(int family, int type, int protocol);
int       sf_closesocket(int s);
int       sf_connect(int s, const void *addr, int addrlen);
int       sf_bind(int s, const void *addr, int addrlen);
int       sf_listen(int s, int backlog);
int       sf_accept(int s, void *addr, void *addrlen);
int       sf_send(int s, const void *buf, int len, int flags);
int       sf_recv(int s, void *buf, int len, int flags);
int       sf_sendto(int s, const void *buf, int len, int flags,
                    const void *to, int tolen);
int       sf_recvfrom(int s, void *buf, int len, int flags,
                      void *from, void *fromlen);
int       sf_setsockopt(int s, int level, int opt, const void *val, int len);
int       sf_getsockname(int s, void *addr, void *addrlen);
int       sf_ioctl(int s, unsigned long cmd, void *arg);
uint32_t  sf_inet_addr(const char *cp);
char     *sf_inet_ntoa(uint32_t s_addr_net);
struct hostent *sf_gethostbyname(const char *name);
struct hostent *sf_gethostbyaddr(const void *addr, int len, int type);
int       sf_errno(void);
int       sf_would_block(void);

uint32_t  sf_htonl(uint32_t);
uint32_t  sf_ntohl(uint32_t);
uint16_t  sf_htons(uint16_t);
uint16_t  sf_ntohs(uint16_t);

#ifdef __cplusplus
}
#endif

#endif /* SF_NET_H */
