#include "InputOutput/HtmlFormatter.h"
#include "InputOutput/HttpFileRequestHandler.h"
#include "InputOutput/HttpResponse.h"
#include "InputOutput/HttpServer.h"
#include "InputOutput/ListenTask.h"
#include "InputOutput/Scheduler.h"
#include "InputOutput/SignalTask.h"
#include "InputOutput/Statistics.h"
#include "InputOutput/StatisticsDocumentation.h"
#include "InputOutput/Task.h"

#include <iostream>

using namespace palo;
using namespace std;



void wait1 () {
  Statistics::Timer timer("wait");

  sleep(1);
}



double burn1 () {
  Statistics::Timer timer("burn");

  double x = 1.0;

  for (int i = 0;  i < 10000;  i++) {
    for (int j = 0;  j < 10000;  j++) {
      x = x * i + j / 10;
    }
  }

  return x;
}



void func1 () {
  Statistics::Timer timer("func1");

  wait1();
  burn1();
}



void func3 () {
  Statistics::Timer timer("func3");

  wait1();
  burn1();
}



void func2 () {
  Statistics::Timer timer("func2");

  wait1();
  wait1();

  for (int i = 0;  i < 3;  i++) {
    func3();
  }

  burn1();
}



void func4 () {
  Statistics::Timer timer("func4");
}



void func5 () {
  char * a = new char [1000];
  delete a;

  a = new char[2000];
}



void func7 () {
  string * a = new string("hallo");

  (*a) = *a + *a + *a + *a + *a + *a + *a + *a + *a + *a + *a + *a + *a + *a + *a + *a + *a;
}



void func6 () {
  char * a = new char [100];
  delete a;

  func5();
  func7();

  a = new char[200];
}





void generateTimings () {
  cout << "starting test1" << endl;

  for (int i = 0;  i < 3;  i++) {
    func1();
  }

  cout << "starting test2" << endl;

  for (int i = 0;  i < 3;  i++) {
    func2();
  }

  cout << "starting test3" << endl;

  for (int i = 0;  i < 10000;  i++) {
    func4();
  }
}



void dumpTimings () {
  map<string, Statistics::Timing> timings = Statistics::statistics.getFullTimings();

  for (map<string, Statistics::Timing>::const_iterator i = timings.begin();  i != timings.end();  i++) {
    cout << i->first << " = "
         << i->second.calls << ", "
         << i->second.wallTime << ", "
         << i->second.userTime << ", "
         << i->second.sysTime << ", "
         << (i->second.wallTime / i->second.calls) << ", "
         << (i->second.userTime / i->second.calls) << ", "
         << (i->second.sysTime / i->second.calls) << endl;
  }
}



class ControlCHandler : public SignalTask {
public:
  ControlCHandler(Scheduler* scheduler) {
    this->scheduler = scheduler;
  }
  
  

  void handleSignal (signal_t signal) {
    cerr << "control-c pressed" << endl;

    scheduler->beginShutdown();
  }
  
  private:
    Scheduler* scheduler;
};



class StatisticsHandler : public HttpRequestHandler {
public:
  HttpResponse * handleHttpRequest (const HttpRequest * request) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);

    StatisticsDocumentation fd(Statistics::statistics);
    HtmlFormatter hd("TestStatistics.tmpl");

    response->getBody().copy(hd.getDocumentation(&fd));
    response->setContentType("text/html;charset=utf-8");
    
    return response;
  }
};



class NotFoundHandler : public HttpRequestHandler {
public:
  HttpResponse * handleHttpRequest (const HttpRequest * request) {
    HttpResponse * response = new HttpResponse(HttpResponse::NOT_FOUND);

    response->getBody().appendText("File not found!");

    return response;
  }
};



int main (int argc, char * argv []) {
  int port = 7777;

  if (argc == 2) {
    port = ::atoi(argv[1]);
  }

#ifndef ENABLE_TIME_PROFILER  
  cout << "this tests requires --enable-time-profiler in configure" << endl;
  exit(1);
#endif

  generateTimings();
  dumpTimings();

  Scheduler* scheduler = Scheduler::getScheduler();
  HttpServer httpServer(scheduler);

  StatisticsHandler sHandler;
  HttpFileRequestHandler fHandler("Api/favicon.ico", "image/x-icon");
  NotFoundHandler notFound;

  httpServer.addHandler("/", &sHandler);
  httpServer.addHandler("/favicon.ico", &fHandler);
  httpServer.addNotFoundHandler(&notFound);

  scheduler->addListenTask(&httpServer, port);

  ControlCHandler controlCHandler(scheduler);
  scheduler->addSignalTask(&controlCHandler, SIGINT);

  cout << "enough time burned\n"
       << "please check statistics on port " << port << endl;

  scheduler->run();

  return 0;
}
