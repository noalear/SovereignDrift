/* ************************************************************************* */
/* sf_net_api.h -- maps the BSD socket names used by the 1997 network code   */
/* onto the portable sf_net shim.  Included ONLY by the handful of network   */
/* source files (S_Client, S_Server, S_Packet, SF_NetEngine, SF_Server, ...) */
/* so the short macro names cannot leak into the rest of the game.           */
/* Added 2026-10-12 for the Godot port.                                      */
/* ************************************************************************* */
#ifndef SF_NET_API_H
#define SF_NET_API_H
#include "sf_net.h"

#define AF_INET        SF_AF_INET
#define PF_INET        SF_AF_INET
#define SOCK_STREAM    SF_SOCK_STREAM
#define SOCK_DGRAM     SF_SOCK_DGRAM
#define SOCK_RAW       SF_SOCK_RAW
#define IPPROTO_IP     SF_IPPROTO_IP
#define IPPROTO_ICMP   SF_IPPROTO_ICMP
#define IPPROTO_TCP    SF_IPPROTO_TCP
#define IPPROTO_UDP    SF_IPPROTO_UDP
#define SOL_SOCKET     SF_SOL_SOCKET
#define SO_DEBUG       SF_SO_DEBUG
#define SO_REUSEADDR   SF_SO_REUSEADDR
#define SO_DONTROUTE   SF_SO_DONTROUTE
#define SO_SNDBUF      SF_SO_SNDBUF
#define SO_RCVBUF      SF_SO_RCVBUF
#define TCP_NODELAY    SF_TCP_NODELAY
#define MSG_PEEK       SF_MSG_PEEK
#define FIONREAD       SF_FIONREAD
#define FIONBIO        SF_FIONBIO
#define INADDR_ANY     SF_INADDR_ANY
#define INADDR_NONE    SF_INADDR_NONE

#define socket(a,b,c)            sf_socket((a),(b),(c))
#define connect(s,a,l)           sf_connect((s),(a),(l))
#define bind(s,a,l)              sf_bind((s),(a),(l))
#define listen(s,b)              sf_listen((s),(b))
#define accept(s,a,l)            sf_accept((s),(a),(l))
#define send(s,b,l,f)            sf_send((s),(b),(l),(f))
#define recv(s,b,l,f)            sf_recv((s),(b),(l),(f))
#define sendto(s,b,l,f,t,tl)     sf_sendto((s),(b),(l),(f),(t),(tl))
#define recvfrom(s,b,l,f,fr,fl)  sf_recvfrom((s),(b),(l),(f),(fr),(fl))
#define setsockopt(s,lv,o,v,l)   sf_setsockopt((s),(lv),(o),(v),(l))
#define getsockname(s,a,l)       sf_getsockname((s),(a),(l))
#define inet_addr(c)             sf_inet_addr(c)
#define inet_ntoa(a)             sf_inet_ntoa((a).s_addr)
#define gethostbyname(n)         sf_gethostbyname(n)
#define gethostbyaddr(a,l,t)     sf_gethostbyaddr((a),(l),(t))
#ifndef htonl
#define htonl(x)                 sf_htonl(x)
#endif
#ifndef ntohl
#define ntohl(x)                 sf_ntohl(x)
#endif
#ifndef htons
#define htons(x)                 sf_htons(x)
#endif
#ifndef ntohs
#define ntohs(x)                 sf_ntohs(x)
#endif
/* The 1997 code calls ioctl()/ioctlsocket() with OS/2-style 4 arguments. */
#define ioctl(s,c,a,...)         sf_ioctl((s),(c),(a))
#define ioctlsocket(s,c,a)       sf_ioctl((s),(c),(a))

#endif
