# pragma once

# include <iostream>
# include <sys/types.h>
# include <sys/socket.h>
# include <cstdlib>
# include <cstring>
# include <unistd.h>
# include <netinet/in.h>
# include <arpa/inet.h>

# include <pthread.h>
# include "Log.hpp"


# define BACKLOG 5

class TcpServer
{
private:
  int _port;
  int _listen_sock;
  static TcpServer* svr;
private:
  TcpServer(int port):_port(port),_listen_sock(-1)
  {}
  TcpServer(const TcpServer& s)
  {}
public:
  static TcpServer* getinstance(int port)
  {
    
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    if (nullptr == svr)
    {
      pthread_mutex_lock(&lock);  
      if (nullptr == svr)
      {
        svr = new TcpServer(port);
        svr->InitServer();
      }
      pthread_mutex_unlock(&lock);  
    }
    return svr;
  }
  void InitServer()
  {
    Socket();
    Bind();
    Listen();
    LOG(INFO, "TCP server init successfully");
  }
  void Socket()
  {
    _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_sock < 0)
    {
      LOG(FATAL, "socket error");
      exit(1);
    }
   int opt = 1;
   setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
   LOG(INFO, "create socket successfully");  
  }
  void Bind()
  {
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(_port);
    local.sin_addr.s_addr = INADDR_ANY;
    if (bind(_listen_sock, (struct sockaddr*)&local, sizeof(local)) < 0)
    {
      
      LOG(FATAL, "bind socket error");
      exit(2);
    }
    LOG(INFO, "bind socket successfully");
  }
  void Listen()
  {
    if (listen(_listen_sock, BACKLOG) < 0)
    {
      LOG(FATAL, "listen socket error");
      exit(3);
    }
    LOG(INFO, "listen socket successfully");
  }
  int Sock()
  {
    return _listen_sock;
  }
  ~TcpServer()
  {
    if (_listen_sock > 0) close(_listen_sock);
  }
  
};

TcpServer* TcpServer::svr = nullptr;




