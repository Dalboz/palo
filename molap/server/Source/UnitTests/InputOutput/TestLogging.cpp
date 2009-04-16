#include "InputOutput/Logger.h"

using namespace palo;

int main () {
  Logger::setLogLevel("info");
  Logger::info << "you should see the number 12345: " << 12345 << endl;
  Logger::trace << "you must not see this" << endl;
  Logger::setLogLevel("debug");
  Logger::trace << "you should see this" << endl;

  return 0;
}
