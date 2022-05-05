# pragma once

# include <iostream>
# include "TcpServer.hpp"
# include <pthread.h>
# include "Protocal.hpp"
# include "Log.hpp"
# include <signal.h>
# include "Task.hpp"
# include "ThreadPool.hpp"

# define PORT 8888

class HttpServer
{
private:
  int _port;
  bool _stop;
public:
  HttpServer(int port = PORT):_port(port),_stop(false)
  {}
  void InitServer()
  {
    // 忽略信号
    signal(SIGPIPE, SIG_IGN);
   //_tcp_server = TcpServer::getinstance(_port);
  }
  void Loop()
  {
    TcpServer* tsvr = TcpServer::getinstance(_port);
    
    LOG(INFO, "Loop begin");
    while (!_stop)
    {
      struct sockaddr_in peer;
      socklen_t len = sizeof(peer);
      int sock = accept(tsvr->Sock(), (struct sockaddr*)&peer, &len);
      
      if (sock < 0)
      {
        continue;
      }
      LOG(INFO, "Get a new link");
      // int* _sock = new int(sock);
      // pthread_t tid;
     //  pthread_create(&tid, nullptr, Entrance::HandlerRequest, _sock);
      Task task(sock);
      ThreadPool::getinstance()->PushTask(task);
    }
  }
  ~HttpServer()
  {}
};

