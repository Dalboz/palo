#include <iostream>

#include "palo.h"

#include "InputOutput/HttpRequest.h"
#include "InputOutput/HttpRequestHandler.h"
#include "InputOutput/HttpResponse.h"
#include "InputOutput/HttpServer.h"
#include "InputOutput/ListenTask.h"
#include "InputOutput/Scheduler.h"
#include "InputOutput/SignalTask.h"
#include "InputOutput/Task.h"

using namespace palo;
using namespace std;

Scheduler* scheduler = Scheduler::getScheduler();



class TestHandler : public HttpRequestHandler {
public:
  HttpResponse * handleHttpRequest (const HttpRequest * request) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);

    response->getBody().appendText("Hallo World");

    return response;
  }
};



class ExitHandler : public HttpRequestHandler {
public:
  HttpResponse * handleHttpRequest (const HttpRequest * request) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);

    scheduler->beginShutdown();

    return response;
  }
};



class ControlCHandler : public SignalTask {
public:
  void handleSignal (signal_t signal) {
    scheduler->beginShutdown();
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

  HttpServer httpServer(scheduler);

  TestHandler testHandler;
  ExitHandler exitHandler;
  ControlCHandler controlCHandler;
  NotFoundHandler notFound;

  httpServer.addHandler("/hello/world", &testHandler);
  httpServer.addHandler("/exit",        &exitHandler);
  httpServer.addNotFoundHandler(&notFound);

  scheduler->addListenTask(&httpServer, port);
  scheduler->addSignalTask(&controlCHandler, SIGINT);

  cout << "please try the server on port " << port << "\n"
       << "using /hello/world or /exit" << endl;

  scheduler->run();
}
