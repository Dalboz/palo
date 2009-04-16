#include <iostream>

using namespace std;

int main () {
  while (! cin.eof()) {
    string line;

    getline(cin, line);

    cout << "line 1\n"
         << "line 2\n";

    sleep(10);

    cout << "DONE\n";
  }
}
