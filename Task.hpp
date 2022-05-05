# pragma once 

# include <iostream>
# include "Protocal.hpp"

class Task
{
  private:
    int _sock;
    CallBack _handler;
  public:
    Task()
    {}
    Task(int sock):_sock(sock)
    {}
    void ProcessOn()
    {
      _handler(_sock);
    }
    ~Task()
    {}
};
