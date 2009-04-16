#include "InputOutput/Condition.h"

#include <iostream>

using namespace std;
using namespace palo;

static int Errors = 0;



static void CHECK (const string& test, bool result, bool expect) {
  bool ok = result == expect;

  cout << test << ": " << (ok ? "OK" : "FAILED") << endl;

  if (! ok) {
    cout << "expecting " << (expect ? "TRUE" : "FALSE") << ", got " << (result ? "TRUE" : "FALSE") << endl;
    Errors++;
  }
}



int main () {

  // simple condition
  Condition * c = Condition::parseCondition("> 10.1");

  CHECK("condition1", c->check(0), false);
  CHECK("condition1", c->check(10.1), false);
  CHECK("condition1", c->check(20), true);

  c = Condition::parseCondition(">= 10.1");

  CHECK("condition2", c->check(10.1), true);


  // combination
  c = Condition::parseCondition("> 10.1 and < 20");

  CHECK("condition3", c->check(0), false);
  CHECK("condition3", c->check(10.1), false);
  CHECK("condition3", c->check(20), false);

  // complex
  c = Condition::parseCondition("(> 10 xor < -10) or = 9");

  CHECK("condition4", c->check(-11), true);
  CHECK("condition4", c->check(0), false);
  CHECK("condition4", c->check(20), true);
  CHECK("condition4", c->check(9), true);

  // print results
  if (Errors > 0) {
    cout << Errors << " tests failed!" << endl;
  }
  else {
    cout << "all tests passed" << endl;
  }

  return 0;
}
