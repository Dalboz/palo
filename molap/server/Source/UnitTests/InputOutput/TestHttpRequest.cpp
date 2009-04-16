#include "InputOutput/HttpRequest.h"

#include <iostream>

using namespace std;
using namespace palo;

static int Errors = 0;



static void CHECK (const string& test, string result, string expect) {
  bool ok = result == expect;

  cout << test << ": " << (ok ? "OK" : "FAILED") << endl;

  if (! ok) {
    cout << "expecting '" << expect << "', got '" << result << "'" << endl;
    Errors++;
  }
}



static void CHECK (const string& test, size_t result, size_t expect) {
  bool ok = result == expect;

  cout << test << ": " << (ok ? "OK" : "FAILED") << endl;

  if (! ok) {
    cout << "expecting " << expect << ", got " << result << endl;
    Errors++;
  }
}



HttpRequest * constructRequest () {
  HttpRequest * request = new HttpRequest("GET /path1/path2?KEY1=VALUE1&KEY2=VALUE2 HTTP/1.1\r\n"
                                          "Host: localhost\r\n");

  request->setBody("KEY3=VALUE3&KEY3=VALUEA&KEY4=VALUE4\r\n");

  return request;
}



int main () {
  HttpRequest * request = constructRequest ();

  CHECK("request path", request->getRequestPath(), "/path1/path2");

  if (request->hasHeaderField("Host:")) {
    const string& host = request->getHeaderField("Host:");

    CHECK("host", host, "localhost");
  }
  else {
    CHECK("host", "", "localhost");
  }

  const vector<string>& keys = request->getKeys();

  CHECK("number keys", keys.size(), 4);

  CHECK("KEY1", request->getValue("KEY1"), "VALUE1");
  CHECK("KEY2", request->getValue("KEY2"), "VALUE2");
  CHECK("KEY3", request->getValue("KEY3"), "VALUEA");
  CHECK("KEY4", request->getValue("KEY4"), "VALUE4\r\n");

  // print results
  if (Errors > 0) {
    cout << Errors << " tests failed!" << endl;
  }
  else {
    cout << "all tests passed" << endl;
  }

  return 0;
}
