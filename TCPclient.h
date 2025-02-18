#ifndef TCPclient_h
#define TCPclient_h 1

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <syslog.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

using namespace std;

class TCPclient {
 public:
  TCPclient(const char*, int);
  ~TCPclient() {};

  int    SendMessage(void*, int);
  int    RecieveMsg(void*, int);
  bool   isAlive();
  void   Close();

 private:
  int    ClntSock;
  bool   alive_flag;
  struct sockaddr_in  addr;
  struct hostent*     s_host;
};

#endif
