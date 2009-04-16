#include "Collections/AssociativeArray.h"

#include <string.h>

#include <iostream>

using namespace std;
using namespace palo;



struct Data {
  const char * name;
  int32_t      value;
};



struct DataDesc {
  bool isEmptyElement (const Data& element) {
    return element.name == 0;
  }

  size_t hashElement (const Data& element) {
    return element.name[0] & 0x7F;
  }

  size_t hashKey (const char * key) {
    return key[0] & 0x7F;
  }

  bool isEqualElementElement (const Data& left, const Data& right) {
    return strcmp(left.name, right.name) == 0;
  }

  bool isEqualKeyElement (const char * key, const Data& element) {
    return strcmp(key, element.name) == 0;
  }

  void clearElement (Data& element) {
    element.name = 0;
  }
};



AssociativeArray<const char*, Data, DataDesc> array(1);



static bool retrieveKey (const char * key) {
  const Data& element = array.findKey(key);

  if (element.name == 0) {
    cout << "retrieving key " << key << ", not found" << endl;
    return false;
  }
  else {
    cout << "retrieving key " << key << ", found value " << element.value << endl;
    return true;
  }
}



static bool removeKey (const char * key) {
  Data res = array.removeKey(key);

  if (res.name != 0) {
    cout << "delete key " << key << " done" << endl;
    return true;
  }
  else {
    cout << "delete key " << key << " failed" << endl;
    return false;
  }
}



static bool retrieveElement (const char * key) {
  Data find;
  find.name = key;

  const Data& element = array.findElement(find);

  if (element.name == 0) {
    cout << "retrieving element " << key << ", not found" << endl;
    return false;
  }
  else {
    cout << "retrieving element " << key << ", found value " << element.value << endl;
    return true;
  }
}



static bool removeElement (const char * key) {
  Data find;
  find.name = key;

  bool res = array.removeElement(find);

  if (res) {
    cout << "delete element " << key << " done" << endl;
    return true;
  }
  else {
    cout << "delete element " << key << " failed" << endl;
    return false;
  }
}



static int Errors = 0;



static void CHECK (bool got, bool expect) {
  if (got != expect) {
    cerr << "FAILED, expecting " << (expect ? "true" : "false") << ", got " << (got ? "true" : "false") << endl;
    Errors++;
  }
}




int main () {
  const char * test[] = { "Test1",
                          "Test2",
                          "Test3",
                          "Hallo1",
                          "Hallo2",
                          "Hallo3" };

  // build array
  for (size_t i = 0;  i < sizeof(test) / sizeof(test[0]);  i++) {
    cout << "adding " << test[i] << " with value " << i << endl;

    Data d;
    d.name  = test[i];
    d.value = (int32_t) i;

    CHECK(array.addElement(d), true);
  }


  CHECK(retrieveKey("Test3"),  true);
  CHECK(retrieveKey("Emil1"),  false);
  CHECK(retrieveKey("Hallo1"), true);
  CHECK(removeKey(  "Test1"),  true);
  CHECK(retrieveKey("Test3"),  true);
  CHECK(removeKey(  "Test3"),  true);
  CHECK(retrieveKey("Test3"),  false);
  CHECK(removeKey(  "Test3"),  false);
  CHECK(retrieveKey("Test3"),  false);

  CHECK(retrieveElement("Test2"),  true);
  CHECK(retrieveElement("Hallo2"), true);
  CHECK(retrieveElement("Test3"),  false);
  CHECK(removeElement(  "Test2"),  true);
  CHECK(retrieveElement("Test2"),  false);

  if (0 < Errors) {
    cerr << Errors << " tests failed" << endl;
    return 1;
  }
  else {
    cerr << "tests are OK" << endl;
    return 0;
  }
}
