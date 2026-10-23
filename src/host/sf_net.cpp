/* ************************************************************************* */
/* sf_net.cpp -- implementation of the portable socket shim (see sf_net.h).  */
/* Added 2026-10-12 for the Stellar Frontier Godot port.                     */
/* This is the only translation unit that includes OS socket headers.        */
/* ************************************************************************* */
#define SF_NET_IMPL
#include <string.h>
#include <stdint.h>
#include <mutex>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
   typedef int sf_socklen;
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <sys/ioctl.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <errno.h>
   typedef socklen_t sf_socklen;
#endif

#include "sf_net.h"

/* Port-side mirror of the struct layout declared in sf_net.h. */
struct sf_port_sockaddr_in {
  unsigned short sin_family;
  uint16_t       sin_port;
  uint32_t       addr_net;
  char           sin_zero[8];
};
struct sf_port_hostent {
  char  *h_name;
  char **h_aliases;
  short  h_addrtype;
  short  h_length;
  char **h_addr_list;
};

static int map_family(int f)   { return f == SF_AF_INET ? AF_INET : f; }
static int map_type(int t) {
  switch (t) {
    case SF_SOCK_STREAM: return SOCK_STREAM;
    case SF_SOCK_DGRAM:  return SOCK_DGRAM;
    case SF_SOCK_RAW:    return SOCK_RAW;
  }
  return t;
}
static int map_proto(int p) {
  switch (p) {
    case SF_IPPROTO_ICMP: return IPPROTO_ICMP;
    case SF_IPPROTO_TCP:  return IPPROTO_TCP;
    case SF_IPPROTO_UDP:  return IPPROTO_UDP;
  }
  return p;
}
static int map_level(int l) {
  if (l == SF_SOL_SOCKET) return SOL_SOCKET;
  if (l == SF_IPPROTO_TCP) return IPPROTO_TCP;
  return l;
}
static int map_opt(int level, int o) {
  if (level == SF_SOL_SOCKET) {
    switch (o) {
      case SF_SO_DEBUG:     return SO_DEBUG;
      case SF_SO_REUSEADDR: return SO_REUSEADDR;
      case SF_SO_DONTROUTE: return SO_DONTROUTE;
      case SF_SO_SNDBUF:    return SO_SNDBUF;
      case SF_SO_RCVBUF:    return SO_RCVBUF;
    }
  }
  if (level == SF_IPPROTO_TCP && o == SF_TCP_NODELAY) return TCP_NODELAY;
  return o;
}
static int map_flags(int f) {
  int r = 0;
  if (f & SF_MSG_PEEK) r |= MSG_PEEK;
#ifdef MSG_NOSIGNAL
  r |= MSG_NOSIGNAL;            /* never let a dropped peer kill the process */
#endif
  return r;
}

static void to_os(const void *p, struct sockaddr_in *o) {
  const sf_port_sockaddr_in *a = (const sf_port_sockaddr_in *)p;
  memset(o, 0, sizeof(*o));
  o->sin_family = (unsigned short)map_family(a->sin_family);
  o->sin_port = a->sin_port;
  o->sin_addr.s_addr = a->addr_net;
}
static void from_os(const struct sockaddr_in *o, void *p) {
  sf_port_sockaddr_in *a = (sf_port_sockaddr_in *)p;
  memset(a, 0, sizeof(*a));
  a->sin_family = SF_AF_INET;
  a->sin_port = o->sin_port;
  a->addr_net = o->sin_addr.s_addr;
}

extern "C" {

int sf_net_init(void) {
  static std::once_flag once;
  static int rc = 0;
  std::call_once(once, [] {
#ifdef _WIN32
    WSADATA wsa;
    rc = WSAStartup(MAKEWORD(2, 2), &wsa) == 0 ? 0 : -1;
#else
    rc = 0;
#endif
  });
  return rc;
}

int sf_socket(int family, int type, int protocol) {
  sf_net_init();
#ifdef _WIN32
  SOCKET s = ::socket(map_family(family), map_type(type), map_proto(protocol));
  if (s == INVALID_SOCKET) return -1;
  return (int)s;      /* Winsock handles are small integers in practice */
#else
  int s = ::socket(map_family(family), map_type(type), map_proto(protocol));
#  ifdef SO_NOSIGPIPE
  if (s >= 0) { int one = 1; ::setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one)); }
#  endif
  return s;
#endif
}

int sf_closesocket(int s) {
#ifdef _WIN32
  return ::closesocket((SOCKET)s);
#else
  return ::close(s);
#endif
}

int sf_connect(int s, const void *addr, int) {
  struct sockaddr_in o; to_os(addr, &o);
  return ::connect(s, (struct sockaddr *)&o, sizeof(o));
}

int sf_bind(int s, const void *addr, int) {
  struct sockaddr_in o; to_os(addr, &o);
  return ::bind(s, (struct sockaddr *)&o, sizeof(o));
}

int sf_listen(int s, int backlog) { return ::listen(s, backlog); }

int sf_accept(int s, void *addr, void *addrlen) {
  struct sockaddr_in o; sf_socklen l = sizeof(o);
  memset(&o, 0, sizeof(o));
#ifdef _WIN32
  SOCKET r = ::accept((SOCKET)s, (struct sockaddr *)&o, &l);
  int rs = (r == INVALID_SOCKET) ? -1 : (int)r;
#else
  int rs = ::accept(s, (struct sockaddr *)&o, &l);
#endif
  if (addr) from_os(&o, addr);
  if (addrlen) *(int *)addrlen = (int)sizeof(sf_port_sockaddr_in);
  return rs;
}

int sf_send(int s, const void *buf, int len, int flags) {
  return (int)::send(s, (const char *)buf, len, map_flags(flags));
}

int sf_recv(int s, void *buf, int len, int flags) {
  return (int)::recv(s, (char *)buf, len, map_flags(flags) & ~0
#ifdef MSG_NOSIGNAL
                     & ~MSG_NOSIGNAL
#endif
                     );
}

int sf_sendto(int s, const void *buf, int len, int flags, const void *to, int) {
  struct sockaddr_in o; to_os(to, &o);
  return (int)::sendto(s, (const char *)buf, len, map_flags(flags),
                       (struct sockaddr *)&o, sizeof(o));
}

int sf_recvfrom(int s, void *buf, int len, int flags, void *from, void *fromlen) {
  struct sockaddr_in o; sf_socklen l = sizeof(o);
  memset(&o, 0, sizeof(o));
  int f = map_flags(flags);
#ifdef MSG_NOSIGNAL
  f &= ~MSG_NOSIGNAL;
#endif
  int r = (int)::recvfrom(s, (char *)buf, len, f, (struct sockaddr *)&o, &l);
  if (from) from_os(&o, from);
  if (fromlen) *(int *)fromlen = (int)sizeof(sf_port_sockaddr_in);
  return r;
}

int sf_setsockopt(int s, int level, int opt, const void *val, int len) {
  return ::setsockopt(s, map_level(level), map_opt(level, opt),
                      (const char *)val, len);
}

int sf_getsockname(int s, void *addr, void *addrlen) {
  struct sockaddr_in o; sf_socklen l = sizeof(o);
  memset(&o, 0, sizeof(o));
  int r = ::getsockname(s, (struct sockaddr *)&o, &l);
  if (addr) from_os(&o, addr);
  if (addrlen) *(int *)addrlen = (int)sizeof(sf_port_sockaddr_in);
  return r;
}

int sf_ioctl(int s, unsigned long cmd, void *arg) {
  /* The original passes an int* for both FIONBIO and FIONREAD. */
  if (cmd == SF_FIONBIO) {
#ifdef _WIN32
    u_long v = *(int *)arg ? 1 : 0;
    return ::ioctlsocket((SOCKET)s, FIONBIO, &v);
#else
    int v = *(int *)arg ? 1 : 0;
    return ::ioctl(s, FIONBIO, &v);
#endif
  }
  if (cmd == SF_FIONREAD) {
#ifdef _WIN32
    u_long v = 0;
    int r = ::ioctlsocket((SOCKET)s, FIONREAD, &v);
    *(int *)arg = (int)v;
    return r;
#else
    int v = 0;
    int r = ::ioctl(s, FIONREAD, &v);
    *(int *)arg = v;
    return r;
#endif
  }
  return -1;
}

uint32_t sf_inet_addr(const char *cp) {
  if (!cp) return SF_INADDR_NONE;
  struct in_addr a;
  if (inet_pton(AF_INET, cp, &a) == 1) return a.s_addr;
  return SF_INADDR_NONE;
}

char *sf_inet_ntoa(uint32_t s_addr_net) {
  static thread_local char buf[INET_ADDRSTRLEN + 1];
  struct in_addr a; a.s_addr = s_addr_net;
  if (!inet_ntop(AF_INET, &a, buf, sizeof(buf))) buf[0] = 0;
  return buf;
}

/* gethostbyname()/gethostbyaddr() equivalents using getaddrinfo/getnameinfo.
   Results live in per-thread storage, like the classic API's static buffers. */
struct sf_hostbuf {
  sf_port_hostent h;
  char name[256];
  char *aliases[1];
  uint32_t addrs[8];
  char *addrlist[9];
};

struct hostent *sf_gethostbyname(const char *name) {
  sf_net_init();
  static thread_local sf_hostbuf hb;
  if (!name) return NULL;
  struct addrinfo hints; memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  struct addrinfo *res = NULL;
  if (getaddrinfo(name, NULL, &hints, &res) != 0 || !res) return NULL;
  int n = 0;
  for (struct addrinfo *p = res; p && n < 8; p = p->ai_next) {
    if (p->ai_family != AF_INET) continue;
    hb.addrs[n] = ((struct sockaddr_in *)p->ai_addr)->sin_addr.s_addr;
    hb.addrlist[n] = (char *)&hb.addrs[n];
    n++;
  }
  hb.addrlist[n] = NULL;
  strncpy(hb.name, res->ai_canonname ? res->ai_canonname : name, sizeof(hb.name) - 1);
  hb.name[sizeof(hb.name) - 1] = 0;
  freeaddrinfo(res);
  if (n == 0) return NULL;
  hb.aliases[0] = NULL;
  hb.h.h_name = hb.name;
  hb.h.h_aliases = hb.aliases;
  hb.h.h_addrtype = SF_AF_INET;
  hb.h.h_length = 4;
  hb.h.h_addr_list = hb.addrlist;
  return (struct hostent *)&hb.h;
}

struct hostent *sf_gethostbyaddr(const void *addr, int len, int) {
  sf_net_init();
  static thread_local sf_hostbuf hb;
  if (!addr || len < 4) return NULL;
  struct sockaddr_in o; memset(&o, 0, sizeof(o));
  o.sin_family = AF_INET;
  memcpy(&o.sin_addr.s_addr, addr, 4);
  if (getnameinfo((struct sockaddr *)&o, sizeof(o), hb.name, sizeof(hb.name),
                  NULL, 0, NI_NAMEREQD) != 0)
    return NULL;
  memcpy(&hb.addrs[0], addr, 4);
  hb.addrlist[0] = (char *)&hb.addrs[0];
  hb.addrlist[1] = NULL;
  hb.aliases[0] = NULL;
  hb.h.h_name = hb.name;
  hb.h.h_aliases = hb.aliases;
  hb.h.h_addrtype = SF_AF_INET;
  hb.h.h_length = 4;
  hb.h.h_addr_list = hb.addrlist;
  return (struct hostent *)&hb.h;
}

int sf_errno(void) {
#ifdef _WIN32
  return WSAGetLastError();
#else
  return errno;
#endif
}

int sf_would_block(void) {
#ifdef _WIN32
  int e = WSAGetLastError();
  return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS;
#else
  return errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS;
#endif
}

uint32_t sf_htonl(uint32_t v) { return htonl(v); }
uint32_t sf_ntohl(uint32_t v) { return ntohl(v); }
uint16_t sf_htons(uint16_t v) { return htons(v); }
uint16_t sf_ntohs(uint16_t v) { return ntohs(v); }

} /* extern "C" */
