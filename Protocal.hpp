# pragma once

# include <iostream>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/stat.h>
# include <sys/sendfile.h>
# include <algorithm>
# include <fcntl.h>
# include "Util.hpp"
# include "Log.hpp"
# include <string>
# include <vector>
# include <sstream>
# include <unordered_map>
# include <sys/wait.h>

# define SEP ": "
# define LINE_END "\r\n"
# define OK 200
# define NOT_FOUND 404
# define HTTP_VERSION "HTTP/1.1"

# define WEB_ROOT "wwwroot"
# define HOME_PAGE "index.html"
# define PAGE_404 "404.html"

static std::string Code2Desc(int code)
{
  std::string desc;
  switch(code)
  {
    case 200:
      desc = "OK";
      break;
    case 404:
      desc = "Not Found";
      break;
    default:
      break;
  }
  return desc;
}
static std::string Suffix2Desc(const std::string& suffix)
{
  static std::unordered_map<std::string, std::string> suffix2desc = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".jpg", "application/x-jpg"},
    {".xml", "application"},
    {".png", "image/png"}
  };
  auto iter = suffix2desc.find(suffix);
  if (iter != suffix2desc.end())
  {
    return iter->second;
  }
  return "text/html";
}
class HttpRequest
{
public:
  std::string request_line;
  std::vector<std::string> request_header;
  std::string blank;
  std::string request_body;

  // 解析之后的数据
  std::string method;
  std::string uri;
  std::string version;

  std::unordered_map<std::string, std::string> header_kv;
  int content_length;
  std::string path;
  std::string query_string;
  std::string suffix;
  
  bool cgi;

  HttpRequest():content_length(0),cgi(false)
  {}
  ~HttpRequest()
  {}
};

class HttpResponse
{
public:
  std::string status_line;
  std::vector<std::string> response_header;
  std::string blank;
  std::string response_body;

  int status_code;
  int fd;
  int size;

  HttpResponse():blank("\n"), status_code(OK), fd(-1)
  {}
  ~HttpResponse()
  {}
};
// 读取请求，分析请求，构建响应
// IO 通信
class EndPoint
{
private:
  int _sock;
  bool _stop;
  HttpRequest http_request;
  HttpResponse http_response;
public:
  bool IsStop()
  {
    return _stop;
  }
private:
  bool RecvHttpRequestLine()
  {
    auto& line = http_request.request_line;
    if(Util::ReadLine(_sock, line) > 0)
    {
      line.resize(line.size() - 1);
      LOG(INFO, http_request.request_line);
   }
   else
   {
     _stop = true;
   }
   return _stop;
    
 }
  bool RecvHttpRequestHeader()
  {
    std::string line;
    while (true)
    {
      line.clear();
      if(Util::ReadLine(_sock, line) <= 0)
      {
        _stop = true;
        break;
      }
      if (line == "\n")
      {
        http_request.blank = line;
        break;
      }
      line.resize(line.size() - 1);
      http_request.request_header.push_back(line);
      LOG(INFO, line);
    }
    return _stop;
  }
  void ParseHttpRequestLine()
  {
    auto& line = http_request.request_line;
    std::stringstream ss(line);
    ss >> http_request.method >> http_request.uri >> http_request.version;
    auto& method = http_request.method;
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    LOG(INFO, http_request.method);
    LOG(INFO, http_request.uri);
    LOG(INFO, http_request.version);
  }
  void ParseHttpRequsetHeader()
  {
    std::string key;
    std::string value;
    for (auto& iter : http_request.request_header)
    {
      if (Util::CutString(iter, key, value, SEP))
      {
        http_request.header_kv.insert({key, value});
      }
    }
  }
  bool IsNeedRecvHttpRequestBody()
  {
    auto& method = http_request.method;
    if (method == "POST")
    {
      auto& header_kv = http_request.header_kv;
      auto iter = header_kv.find("Content-Length");
      if (iter != header_kv.end())
      {
        http_request.content_length = atoi(iter->second.c_str());
        return true;
      }
    }
    return false;
  }
  bool RecvHttpRequestBody()
  {
    if (IsNeedRecvHttpRequestBody())
    {
      int content_length = http_request.content_length;
      auto& body = http_request.request_body;

      char ch = 0;
      while (content_length)
      {
        ssize_t s = recv(_sock, &ch, 1, 0);
        if (s > 0)
        {
          body.push_back(ch);
          content_length--;
        }
        else
        {
          _stop = true;
          break;
        }
      }
    }
    return _stop;
  }
public:
  EndPoint(int sock):_sock(sock),_stop(false)
  {}
  void RecvHttpRequest()
  {
    if(!RecvHttpRequestLine() && !RecvHttpRequestHeader())
    {    
      ParseHttpRequestLine();
      ParseHttpRequsetHeader();
      RecvHttpRequestBody();
    }
    
  }
private:
  void ParseHttpRequest()
  {
    ParseHttpRequestLine();
    ParseHttpRequsetHeader();
    RecvHttpRequestBody();
  }
  int ProcessNonCgi()
  {
    http_response.fd = open(http_request.path.c_str(), O_RDONLY);
    
    if (http_response.fd >= 0)
    {
    /*
     http_response.status_line = HTTP_VERSION;
     http_response.status_line += " ";
     http_response.status_line += std::to_string(http_response.status_code);
     http_response.status_line += " ";
     http_response.status_line += Code2Desc(http_response.status_code);
     http_response.status_line += LINE_END;

     std::string header_line = "Content-Type: ";
     header_line += Suffix2Desc(http_request.suffix);
     header_line += LINE_END;
     http_response.response_header.push_back(header_line);

     header_line = "Content-Length: ";
     header_line += std::to_string(size);
     header_line += LINE_END;
     http_response.response_header.push_back(header_line);
     */
     return OK;
    }
    return NOT_FOUND;
  }
  int ProcessCgi()
  {
    // 父进程数据
    auto& method = http_request.method;
    auto& query_string = http_request.query_string;
    auto& body_text = http_request.request_body;
    int content_length = http_request.content_length;
    auto& bin = http_request.path;
    auto& response_body = http_response.response_body;;
    std::string query_string_env;
    std::string method_env;
    std::string content_length_env;
    int code = OK;
    // 站在父进程角度的输入输出
    int input[2];
    int output[2];
    
    if (pipe(input) < 0)
    {
      LOG(ERROR, "pipe input error");
      return NOT_FOUND;
    }
    if (pipe(output) < 0)
    {
      LOG(ERROR, "pipe output error");
      return NOT_FOUND;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
      // exec
      close(input[0]);
      close(output[1]);
      
      
      method_env = "METHOD=";
      method_env += method;
      putenv((char*)method_env.c_str());
        
      if (method == "GET")
      {
        query_string_env = "QUERY_STRING=";
        query_string_env += query_string;
        putenv((char*)query_string_env.c_str());
      }
      else if (method == "POST")
      {
        content_length_env = "CONTENT_LENGTH=";
        content_length_env += std::to_string(content_length);
        putenv((char*)content_length_env.c_str());
      }

      // 站在子进程角度
      // input[1] 写出
      // output[0] 读入
      dup2(input[1], 1);
      dup2(output[0], 0);
      execl(bin.c_str(), bin.c_str(), nullptr);
      exit(1);
    }
    else if (pid < 0)
    {
      LOG(ERROR, "fork error");
      return NOT_FOUND;
    }
    else 
    {
      close(input[1]);
      close(output[0]);

      if (method == "POST")
      {
        const char* start = body_text.c_str();
        int total = 0;
        int size = 0;
        while (total < content_length && (size = write(output[1], start + total, body_text.size() - total)) > 0)
        {
          total += size;
        }
      }
      char ch = 0;
      while (read(input[0], &ch, 1) > 0)
      {
        response_body.push_back(ch);
      }
      int status = 0;
      pid_t ret = waitpid(pid, &status, 0);
      if (ret == pid)
      {
        if (WIFEXITED(status))
        {
          if (WEXITSTATUS(status) == 0)
          {
            code = OK;
          }
          else 
          {
            code = NOT_FOUND;
          }
        }
        else 
        {
          code = NOT_FOUND;
        }
      }

      close(input[0]);
      close(output[1]);
    }
    return code;
  }
  void HandlerError(std::string page)
  {
    http_request.cgi = false;
    http_response.fd = open(page.c_str(), O_RDONLY);
    if (http_response.fd > 0)
    {
      struct stat st;
      stat(page.c_str(), &st);
      std::string line = "Content-Type: text/html";
      line += LINE_END;
      http_response.response_header.push_back(line);
      line = "Content-Length: ";
      line += std::to_string(st.st_size);
      line += LINE_END;
      http_response.response_header.push_back(line);
    }

  }
  void BuildOKResponse()
  {
    std::string line = "Content-Type: ";
    line += Suffix2Desc(http_request.suffix);
    line += LINE_END;
    http_response.response_header.push_back(line);

    line = "Content-Length: ";
    if (http_request.cgi)
    {
      line += std::to_string(http_response.response_body.size());
    }
    else 
    {   
      line += std::to_string(http_response.size);
    }
    line += LINE_END;
    http_response.response_header.push_back(line);
  }
  void BuildHttpResponseHelper()
  {
    auto& code = http_response.status_code;
    auto& status_line = http_response.status_line;
    status_line += HTTP_VERSION;
    status_line += " ";
    status_line += std::to_string(code);
    status_line += " ";
    status_line += Code2Desc(code);
    status_line += LINE_END;

    std::string path = WEB_ROOT;
    path += "/";
    switch(code)
    {
      case OK:
        BuildOKResponse();
        break;
      case NOT_FOUND:
        path += PAGE_404;
        HandlerError(path);
        break;
      default:
        break;
    }
  }
public:
  void BuildHttpResponse()
  {
    std::string _path;
    int size = 0; 
    struct stat st;
    auto& code = http_response.status_code;
    std::size_t found = 0;

    if (http_request.method != "GET" && http_request.method != "POST")
    {
      // 非法操作
      LOG(WARNING, "method is not right");
      code = NOT_FOUND;
      goto END;  
    }
    if (http_request.method == "GET")
    {
      size_t pos = http_request.uri.find('?');
      if (pos != std::string::npos)
      {
        Util::CutString(http_request.uri, http_request.path, http_request.query_string, "?");
        http_request.cgi = true;
      }
      else
      {
        http_request.path = http_request.uri;
      }
    }
    else if (http_request.method == "POST")
    {
      http_request.cgi = true;
      http_request.path = http_request.uri;
    }
    else
    {
      // TODO
    }
    _path = http_request.path;
    http_request.path = WEB_ROOT;
    http_request.path += _path;
    if (http_request.path[http_request.path.size() - 1] == '/')
    {
      http_request.path += HOME_PAGE;
    }
    LOG(INFO, http_request.path);
    if (stat(http_request.path.c_str(), &st) == 0)
    {
      if (S_ISDIR(st.st_mode))
      {
        // 请求的资源是目录
        http_request.path += "/";
        http_request.path += HOME_PAGE;
        stat(http_request.path.c_str(), &st);
      }
      if ((st.st_mode&S_IXUSR) || (st.st_mode&S_IXGRP) || (st.st_mode&S_IXOTH))
      {
        // 请求的资源是可执行程序
        http_request.cgi = true;
      }
      size = st.st_size;
      http_response.size = size;
    }
    else
    {
      std::string info = _path;
      info += " not found";
      LOG(WARNING, info);
      code = NOT_FOUND;
      goto END;
   }
   found = _path.rfind(".");
   if (found == std::string::npos)
   {
     http_request.suffix = ".html";
   }
   else 
   {
     http_request.suffix = http_request.path.substr(found);
   }
   if (http_request.cgi)
   {
    code = ProcessCgi(); 
   }
   else
   {
     // 返回静态网页
     code = ProcessNonCgi();
     
   }
END:
  
  BuildHttpResponseHelper();
  return;
  }
  void SendHttpResponse()
  {
    send(_sock, http_response.status_line.c_str(), http_response.status_line.size(), 0);
    for (auto iter : http_response.response_header)
    {
      send(_sock, iter.c_str(), iter.size(), 0);
    }
    send(_sock, http_response.blank.c_str(), http_response.blank.size(), 0);
    if (http_request.cgi)
    {
      auto& response_body = http_response.response_body;
      size_t size = 0;
      size_t total = 0;
      const char* start = response_body.c_str();
      while (total < response_body.size() && send(_sock, start + total, response_body.size() - total, 0) > 0)
      {
        total += size;
      }

    }
    else 
    {
     sendfile(_sock, http_response.fd, nullptr, http_response.size);
     close(http_response.fd);
    }
  }
  ~EndPoint()
  {
    close(_sock);
  }
};

class CallBack
{
public:
  CallBack()
  {

  }
  void operator()(int sock)
  {
    HandlerRequest(sock);
  }
  void HandlerRequest(int sock)
  {
    LOG(INFO, "Handler Request Begin");

#ifdef DEBUG
    char buffer[4096];
    recv(sock, buffer, sizeof(buffer), 0);

    std::cout << "-------------------------------------begin-----------------------------------------" << std::endl;
    std::cout << buffer << std::endl;
    std::cout << "------------------------------------ end ------------------------------------------" << std::endl;
#else
    EndPoint* ep = new EndPoint(sock);
    ep->RecvHttpRequest();
    if (!ep->IsStop())
    {
      ep->BuildHttpResponse();
      ep->SendHttpResponse();
    }
    else 
    {
      LOG(WARNING, "Recv Error, Stop, Build And Send");
    }
    delete ep;

#endif
    LOG(INFO, "Handler Request End");
  }
  ~CallBack()
  {

  }
};


