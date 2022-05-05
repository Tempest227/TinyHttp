# pragma once

# include <iostream>
# include <string>
# include <sys/types.h>
# include <sys/socket.h>

class Util
{
public:
  static int ReadLine(int sock, std::string& out)
  {
    char ch = 'x';
    while (ch != '\n')
    {
      ssize_t s = recv(sock, &ch, 1, 0);
      if (s > 0)
      {
        if (ch == '\r')
        {
          recv(sock, &ch, 1, MSG_PEEK);
          if (ch == '\n')
          {
            recv(sock, &ch, 1, 0);
          }
          else 
          {
            ch = '\n';
          }
        }
        out.push_back(ch);
      }
      else if (s == 0)
      {
        return 0;
      }
      else
      {
        return -1;
      }
    }
    return out.size();
  }
  static bool CutString(const std::string& target, std::string& key_out, std::string& value_out,const std::string& seq)
  {
    size_t pos = target.find(seq);
    if (pos != std::string::npos)
    {
      key_out = target.substr(0, pos);
      value_out = target.substr(pos + seq.size());
      return true;
    }
    return false;
  }

};

