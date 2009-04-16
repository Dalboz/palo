#include <iostream>

#include "palo.h"

#include "InputOutput/HttpRequest.h"
#include "InputOutput/HttpRequestHandler.h"
#include "InputOutput/HttpResponse.h"
#include "InputOutput/HttpServer.h"
#include "InputOutput/Logger.h"
#include "InputOutput/Scheduler.h"
#include "InputOutput/SignalTask.h"
#include "InputOutput/Worker.h"

using namespace palo;
using namespace std;

Scheduler* scheduler = Scheduler::getScheduler();
Worker * worker;

class ControlCHandler : public SignalTask {
public:
  void handleSignal (signal_t signal) {
    scheduler->beginShutdown();
  }
};



class TestHandler : public HttpRequestHandler {
public:
  TestHandler () {
    semaphore = scheduler->createSemaphore();
  }

  ~TestHandler () {
    scheduler->deleteSemaphore(semaphore);
  }

public:
  HttpResponse * handleHttpRequest (const HttpRequest * request) {
    if (! worker->execute(semaphore, request->getValue("worker"))) {
      return handleSemaphoreRaised(request, 0);
    }

    return new HttpResponse(semaphore, 0);
  }

  HttpResponse * handleSemaphoreRaised (const HttpRequest * request, IdentifierType clientData) {
    HttpResponse * response = new HttpResponse(HttpResponse::OK);

    if (worker->getResultStatus() == Worker::RESULT_OK) {
      response->getBody().appendText("worker returned:");
      response->getBody().appendEol();

      const vector<string>& result = worker->getResult();

      for (vector<string>::const_iterator i = result.begin();  i != result.end();  i++) {
        response->getBody().appendText(*i);
        response->getBody().appendEol();
      }

    }
    else {
      response->getBody().appendText("worker failed");
    }

    return response;
  }

private:
  semaphore_t semaphore;
};



int main (int argc, char * argv []) {
  int port = 7777;

  Logger::setLogLevel("trace");

  if (argc != 2) {
    cout << "usage: " << argv[0] << " <worker>" << endl;
    exit(1);
  }
  
  Worker::setExecutable(argv[1]);
  Worker::setArguments(vector<string>());
  worker = new Worker(scheduler,"egal");
  bool ok = worker->start();

  if (! ok) {
    cerr << "cannot start worker" << endl;
    exit(1);
  }

  ControlCHandler controlCHandler;
  scheduler->addSignalTask(&controlCHandler, SIGINT);

  HttpServer httpServer(scheduler);
  TestHandler * testHandler = new TestHandler();
  httpServer.addHandler("/", testHandler);

  scheduler->addListenTask(&httpServer, port);

  cout << "please try the server on port " << port << " using /" << endl;

  scheduler->run();
}
