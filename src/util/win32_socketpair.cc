#include "win32_socketpair.h"
#include "util/scope_guard.h"

// based on libnix's implementation by Diego Casorran

#ifndef AF_LOCAL
# define AF_LOCAL AF_UNIX
#endif
#ifndef PF_LOCAL
# define PF_LOCAL PF_UNIX
#endif

extern "C" int socketpair(int family, int type, int protocol, SOCKET sv[2]) {
  SOCKET insock = INVALID_SOCKET;
  SOCKET outsock = INVALID_SOCKET;
  SOCKET newsock = INVALID_SOCKET;
  sockaddr_in sock_in, sock_out;
  int len;

  bool success = false;
  scope(exit) {
    if (newsock != INVALID_SOCKET) closesocket(newsock);
    if (!success) {
      if (insock != INVALID_SOCKET) closesocket(insock);
      if (outsock != INVALID_SOCKET) closesocket(outsock);
    }
  };

  /* windowz only has AF_INET (we use that for AF_LOCAL too) */
  if (family != AF_LOCAL && family != AF_INET)
    return -1;

  /* STRAM and DGRAM sockets only */
  if (type != SOCK_STREAM && type != SOCK_DGRAM)
    return -1;

  /* yes, we all love windoze */
  if ((family == AF_LOCAL && protocol != PF_UNSPEC && protocol != PF_LOCAL) ||
      (family == AF_INET && protocol != PF_UNSPEC && protocol != PF_INET))
    return -1;

  /* create the first socket */
  newsock = socket(AF_INET, type, 0);
  if (newsock == INVALID_SOCKET)
    return -1;

  /* bind the socket to any unused port */
  sock_in.sin_family = AF_INET;
  sock_in.sin_port = 0;
  sock_in.sin_addr.s_addr = INADDR_ANY;
  if (bind(newsock, (sockaddr *)&sock_in, sizeof(sock_in)) < 0)
    return -1;
  len = sizeof(sock_in);
  if (getsockname(newsock, (sockaddr *)&sock_in, &len) < 0)
    return -1;

  /* For stream sockets, create a listener */
  if (type == SOCK_STREAM)
    listen(newsock, 2);

  /* create a connecting socket */
  outsock = socket(AF_INET, type, 0);
  if (outsock == INVALID_SOCKET)
    return -1;

  /* For datagram sockets, bind the 2nd socket to an unused address, too */
  if (type == SOCK_DGRAM) {
    sock_out.sin_family = AF_INET;
    sock_out.sin_port = 0;
    sock_out.sin_addr.s_addr = INADDR_ANY;
    if (bind(outsock, (sockaddr *)&sock_out, sizeof(sock_out)) < 0)
      return -1;
    len = sizeof(sock_out);
    if (getsockname(outsock, (sockaddr *)&sock_out, &len) < 0)
      return -1;
  }

  /* Force IP address to loopback */
  sock_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (type == SOCK_DGRAM)
    sock_out.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  /* Do a connect */
  if (connect(outsock, (sockaddr *)&sock_in, sizeof(sock_in)) < 0)
    return -1;

  if (type == SOCK_STREAM) {
    /* For stream sockets, accept the connection and close the listener */
    len = sizeof(sock_in);
    insock = accept(newsock, (sockaddr *)&sock_in, &len);
    if (insock == INVALID_SOCKET)
      return -1;
    closesocket(newsock);
    newsock = INVALID_SOCKET;
  } else {
    /* For datagram sockets, connect the 2nd socket */
    if (connect(newsock, (sockaddr *)&sock_out, sizeof(sock_out)) < 0)
      return -1;
    insock = newsock;
    newsock = INVALID_SOCKET;
  }

  /* set the descriptors */
  sv[0] = insock;
  sv[1] = outsock;

  /* we've done it */
  success = true;
  return 0;
}
