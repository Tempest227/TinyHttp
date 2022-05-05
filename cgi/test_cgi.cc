# include <iostream>
# include <cstdlib>
# include <unistd.h>

using namespace std;

bool GetQueryString(string& query_string)
{
  bool result = false;
  std::string method = getenv("METHOD");
  if ("GET" == method)
  {
    query_string = getenv("QUERY_STRING");
    cerr << "Get Debug QUERY_STRING: " << query_string << endl;
    result = true;
  }
  else if ("POST" == method)
  {
    // cgi 如何得知从标准输入读取多少个字节
    int content_length = atoi(getenv("CONTEN_LENGTH"));
    char c = 0;
    while (content_length)
    {
      read(0, &c, 1);
      query_string.push_back(c);
      content_length--;
    }
    cerr << "Post Debug QUERY_STRING: " << query_string << endl;
    result = true;
  }
  else 
  {
    result = false;
  }

  return result;
  
}

void CutString(string& in, const string& sep, string& out1, string& out2)
{
  auto pos = in.find(sep);
  if (string::npos != pos)
  {
    out1 = in.substr(0, pos);
    out2 = in.substr(pos+sep.size());
  }
}

int main()
{
  string query_string;
  GetQueryString(query_string);
  string str1;
  string str2;
  CutString(query_string, "&", str1, str2);

  string name1;
  string value1;
  CutString(str1, "=", name1, value1);

  string name2;
  string value2;
  CutString(str2, "=", name2, value2);

  cout << name1 << ":" << value1 << endl;
  cout << name2 << ":" << value2 << endl;

  cerr << name1 << ":" << value1 << endl;
  cerr << name2 << ":" << value2 << endl;

  return 0;
}
