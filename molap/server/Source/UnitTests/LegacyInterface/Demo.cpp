////////////////////////////////////////////////////////////////////////////////
/// @brief get timings for various operations
///
/// @file
///
/// Copyright (C) 2006-2008 Jedox AG
///
/// This program is free software; you can redistribute it and/or modify it
/// under the terms of the GNU General Public License (Version 2) as published
/// by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
///
/// This program is distributed in the hope that it will be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
/// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
/// more details.
///
/// You should have received a copy of the GNU General Public License along with
/// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
/// Place, Suite 330, Boston, MA 02111-1307 USA
/// 
/// You may obtain a copy of the License at
///
/// <a href="http://www.jedox.com/license_palo.txt">
///   http://www.jedox.com/license_palo.txt
/// </a>
///
/// If you are developing and distributing open source applications under the
/// GPL License, then you are free to use Palo under the GPL License.  For OEMs,
/// ISVs, and VARs who distribute Palo with their products, and do not license
/// and distribute their source code under the GPL, Jedox provides a flexible
/// OEM Commercial License.
///
/// Developed by triagens GmbH, Koeln on behalf of Jedox AG. Intellectual
/// property rights has triagens GmbH, Koeln. Exclusive worldwide
/// exploitation right (commercial copyright) has Jedox AG, Freiburg.
///
/// @author Frank Celler, triagens GmbH, Cologne, Germany
/// @author Achim Brandt, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "CubeSample.h"

#include <iostream>

using namespace std;
using namespace palo;



static void showHelp () {
  cout << " --palo <host> <port>              default: localhost 1234\n"
       << "\n"
       << " --user <name> <password>          default: user password\n"
       << "\n"
       << " --getdata1                        AA,BA,CA,DA\n"
       << "                                   AB,BA,CA,DA\n"
       << "                                   AA,BB,CA,DA\n"
       << "                                   ...\n"
       << "                                   AB,BB,CB,DB\n"
       << "\n"
       << " --getdata2 X Y                    <X> , <Y> dimensions to loop\n"
       << "                                   AB0,BA0,CA0,DA0\n"
       << "                                   AB1,BA0,CA0,DA0\n"
       << "                                   AA2,BB0,CA0,DA0\n"
       << "                                   ...\n"
       << "                                   ABn,BB0,CB0,DB0\n"
       << "\n"
       << " --getdata_multi1                  like getdata1 using getdata_multi\n"
       << "\n"
       << " --getdata_multi2                  like getdata2 using getdata_multi\n" 
       << "\n"
       << " --getdata_area1 <X>               <X> dimensions not to loop\n"
       << "                                   AA0,BA0,CA0,DA0\n"
       << "                                   AA1,BA0,CA0,DA0\n"
       << "                                   ...\n"
       << "                                   AAn,BAm,CAk,DA0\n"
       << "\n"
       << " --getdata_area2 <X>               <X> dimensions not to loop\n"
       << "                                   AA0,BA0,CA0,DA\n"
       << "                                   AA1,BA0,CA0,DA\n"
       << "                                   ...\n"
       << "                                   AAn,BAm,CAk,DA" << endl;
}



static long int toInteger (const string& str) {
  char *p;    
  long int result = strtol(str.c_str(), &p, 10);    

  if (*p != '\0') {
    showHelp();    
    exit(0);
  }    

  return result;    
}




static double toDouble (const string& str) {
  char *p;    
  double result = strtod(str.c_str(), &p);    

  if (*p != '\0') {
    showHelp();    
    exit(0);
  }    

  return result;    
}



int main (int argc, char * argv []) { 
  string serverName   = "localhost";
  string serverPort   = "1234";
  string user         = "user";
  string password     = "pass";
  int consolidations  = 0; // number of consolidation per dimension
  int baseElements    = 0; // number of elements per consolidation
  int bigElements     = 0; // number of elements of big dimension
  int bigDimension    = 0; // nmber of big dimension (0=first)
  double percent      = 0.0; 
  
  string databaseName = "Demo";
  string cubeName     = "Sales";
  
  bool getData1       = false;
  bool getData2       = false;
  bool getMultiData1  = false;
  bool getMultiData2  = false;
  bool getArea1       = false;
  bool getArea2       = false;

  int num1 = 0;
  int num2 = 0;
  int num3 = 0;
  int num4 = 0;

  // parse arguments
  for (int i = 1;  i < argc;  i++) {
    string next = argv[i];
    
    if (next == "--palo") {
      if (i+2 >= argc) {
        showHelp();
        return 0;
      }

      serverName = argv[++i];
      serverPort = argv[++i];
    }
    else if (next == "--user") {
      if (i+2 >= argc) {
        showHelp();
        return 0;
      }

      user = argv[++i];
      password = argv[++i];
    }
    else if (next == "--getdata1") {
      getData1 = true;
    }
    else if (next == "--getdata2") {
      if (i + 2 >= argc) {
        showHelp();
        return 0;
      }

      num1 = toInteger(argv[++i]);

      if (num1 < 0 || num1 > 5) {
        showHelp();
        return 0;
      }

      num3 = toInteger(argv[++i]);

      if (num3 < 0 || num3 > 5 || num3 == num1) {
        showHelp();
        return 0;
      }

      getData2 = true;
    }
    else if (next == "--getdata_multi1") {
      getMultiData1 = true;
    }
    else if (next == "--getdata_multi2") {
      if (i + 2 >= argc) {
        showHelp();
        return 0;
      }

      num1 = toInteger(argv[++i]);

      if (num1 < 0 || num1 > 5) {
        showHelp();
        return 0;
      }

      num3 = toInteger(argv[++i]);

      if (num3 < 0 || num3 > 5 || num3 == num1) {
        showHelp();
        return 0;
      }

      getMultiData2 = true;
    }
    else if (next == "--getdata_area1") {
      if (i + 1 >= argc) {
        showHelp();
        return 0;
      }

      num3 = toInteger(argv[++i]);

      if (num3 < 0 || num3 > 5) {
        showHelp();
        return 0;
      }

      getArea1 = true;
    }
    else if (next == "--getdata_area2") {
      if (i + 1 >= argc) {
        showHelp();
        return 0;
      }

      num4 = toInteger(argv[++i]);

      if (num4 < 0 || num4 > 5) {
        showHelp();
        return 0;
      }

      getArea2 = true;
    }
    else {
      showHelp();
      exit(0);
    }
  }

  // execute commands
  string input;

  PaloConnector p(serverName, serverPort, user, password);
  CubeSample s(p);

  s.loadDatabase(databaseName, cubeName);

  if (getData1) {
    s.getData1(false);
  }

  if (getMultiData1) {
    s.getData1(true);
  }

  if (getData2) {
    s.getData3(num1, num3, false);
  }

  if (getMultiData2) {
    s.getData3(num1, num3, true);
  }

  if (getArea1) {
    s.getArea1(num3);
  }

  if (getArea2) {
    s.getArea2(num4);
  }
}
