#include <iostream>
#include <string>

using namespace std;

int main () {
  while (! cin.eof()) {
    string line;

    getline(cin, line);

    cerr << "WORKER received '" << line << "'" << endl;

    sleep(10);

    string response;

    if (line == "CUBE;2;5") {
      response += "AREA;Bereich1;2;5;*;*\n";
    }
    else if (line == "CUBE;1;13") {
      response += "AREA;Bereich2;1;13;*;*;*;*;*;*\n";
    }

    response += "DONE";

    cerr << "WORKER replied '" << response << "'" << endl;
    cout << response << "\n";
  }
}
