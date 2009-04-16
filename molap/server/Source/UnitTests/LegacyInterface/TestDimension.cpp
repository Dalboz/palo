////////////////////////////////////////////////////////////////////////////////
/// @brief generate sample cube using libpalo 1.0
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
////////////////////////////////////////////////////////////////////////////////

#include "PaloConnector.h"

#include <vector>
#include <sstream>
#include <iostream>

using namespace std;
using namespace palo;


static PaloConnector * Palo;

static const string DatabaseName = "TestDimension";
static const string DimensionName = "Dimension1";
static const string Dimension2Name = "Dimension2";
static const string Dimension3Name = "Dimension3";

static vector<string> Elements;



static string itoa (int i) {
  stringstream str;

  str << i;
    
  return str.str();
}



static int Errors = 0;



static void CHECK (const string& test, const string& element, int result, int expect) {
  bool ok = result == expect;

  cout << test << " for " << element << ": " << (ok ? "OK" : "FAILED") << endl;

  if (! ok) {
    cout << "expecting " << expect << ", got " << result << endl;
    Errors++;
  }
}



static void CHECK (const string& test, const string& element, double result, double expect) {
  bool ok = result == expect;

  cout << test << " for " << element << ": " << (ok ? "OK" : "FAILED") << endl;

  if (! ok) {
    cout << "expecting " << expect << ", got " << result << endl;
    Errors++;
  }
}



static void CHECK (const string& test, const string& element, string result, string expect) {
  bool ok = result == expect;

  cout << test << " for " << element << ": " << (ok ? "OK" : "FAILED") << endl;

  if (! ok) {
    cout << "expecting '" << expect << "', got '" << result << "'" << endl;
    Errors++;
  }
}



static void CreateDatabase () {

  // create element names
  for (int i = 1;  i <= 15;  i++) {
    stringstream a;

    a << "A" << i;

    Elements.push_back(a.str());
  }

  // create database
  Palo->createDatabase(DatabaseName);

  // dimension 3
  vector<string> sub;

  Palo->createDimension(DatabaseName, Dimension3Name);

  Palo->addNumericElement(DatabaseName, Dimension3Name, "AA2");
  Palo->addNumericElement(DatabaseName, Dimension3Name, "AA3");

  sub.clear();
  sub.push_back("AA2");
  Palo->appendConsolidateElement(DatabaseName, Dimension3Name, "AA1", sub, de_n);

  sub.clear();
  sub.push_back("AA3");
  Palo->appendConsolidateElement(DatabaseName, Dimension3Name, "AA1", sub, de_n);

  // dimension 2
  Palo->createDimension(DatabaseName, Dimension2Name);

  Palo->addNumericElement(DatabaseName, Dimension2Name, "A1");
  Palo->addNumericElement(DatabaseName, Dimension2Name, "A2");
  Palo->addNumericElement(DatabaseName, Dimension2Name, "A3");

  // dimension 1
  Palo->createDimension(DatabaseName, DimensionName);

  Palo->addNumericElement(DatabaseName, DimensionName, "A5");
  Palo->addNumericElement(DatabaseName, DimensionName, "A11");
  Palo->addNumericElement(DatabaseName, DimensionName, "A15");

  sub.clear();
  sub.push_back("A11");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A10", sub, de_n);
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A8", sub, de_n);

  sub.clear();
  sub.push_back("A8");
  sub.push_back("A15");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A13", sub, de_n);

  sub.clear();
  sub.push_back("A13");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A14", sub, de_n);

  sub.clear();
  sub.push_back("A8");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A12", sub, de_n);

  sub.clear();
  sub.push_back("A10");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A9", sub, de_n);

  sub.clear();
  sub.push_back("A9");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A6", sub, de_n);

  sub.clear();
  sub.push_back("A5");
  sub.push_back("A6");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A2", sub, de_n);

  sub.clear();
  sub.push_back("A5");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A3", sub, de_n);

  sub.clear();
  sub.push_back("A6");
  sub.push_back("A8");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A7", sub, de_n);

  sub.clear();
  sub.push_back("A7");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A4", sub, de_n);

  sub.clear();
  sub.push_back("A2");
  sub.push_back("A3");
  sub.push_back("A4");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A1", sub, de_n);
}



static void RestructureDimension () {
  int size;

  size = Palo->ecount(DatabaseName, DimensionName);
  CHECK("ecount", DatabaseName, size, 15);

  size = Palo->etoplevel(DatabaseName, DimensionName);
  CHECK("etoplevel", DatabaseName, size, 6);

  Palo->deleteElement(DatabaseName, DimensionName, "A12");
  Palo->deleteElement(DatabaseName, DimensionName, "A13");
  Palo->deleteElement(DatabaseName, DimensionName, "A14");
  Palo->deleteElement(DatabaseName, DimensionName, "A15");

  size = Palo->ecount(DatabaseName, DimensionName);
  CHECK("ecount", DatabaseName, size, 11);

  size = Palo->etoplevel(DatabaseName, DimensionName);
  CHECK("etoplevel", DatabaseName, size, 6);

  Palo->addNumericElement(DatabaseName, DimensionName, "A12");
  Palo->addNumericElement(DatabaseName, DimensionName, "A14");
  Palo->addNumericElement(DatabaseName, DimensionName, "A15");

  vector<string> sub;

  sub.clear();
  sub.push_back("A12");
  Palo->appendChildren(DatabaseName, DimensionName, "A7", sub, de_n);

  sub.clear();
  sub.push_back("A8");
  sub.push_back("A14");
  sub.push_back("A15");
  Palo->addConsolidateElement(DatabaseName, DimensionName, "A13", sub, de_n);

  int index;

  index = Palo->eindex(DatabaseName, DimensionName, Elements[11]);
  CHECK("eindex", Elements[11], index, 12);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[12]);
  CHECK("eindex", Elements[12], index, 15);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[13]);
  CHECK("eindex", Elements[13], index, 13);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[14]);
  CHECK("eindex", Elements[14], index, 14);
}



static void TestEIndent () {
  int indent;

  indent = Palo->eindent(DatabaseName, DimensionName, Elements[0]);
  CHECK("eindent", Elements[0], indent, 1);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[1]);
  CHECK("eindent", Elements[1], indent, 2);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[2]);
  CHECK("eindent", Elements[2], indent, 2);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[3]);
  CHECK("eindent", Elements[3], indent, 2);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[4]);
  CHECK("eindent", Elements[4], indent, 3);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[5]);
  CHECK("eindent", Elements[5], indent, 3);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[6]);
  CHECK("eindent", Elements[6], indent, 3);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[7]);
  CHECK("eindent", Elements[7], indent, 3);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[8]);
  CHECK("eindent", Elements[8], indent, 4);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[9]);
  CHECK("eindent", Elements[9], indent, 5);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[10]);
  CHECK("eindent", Elements[10], indent, 6);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[11]);
  CHECK("eindent", Elements[11], indent, 1);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[12]);
  CHECK("eindent", Elements[12], indent, 2);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[13]);
  CHECK("eindent", Elements[13], indent, 1);
  indent = Palo->eindent(DatabaseName, DimensionName, Elements[14]);
  CHECK("eindent", Elements[14], indent, 3);
}



static void TestELevel () {
  int level;

  level = Palo->elevel(DatabaseName, DimensionName, Elements[0]);
  CHECK("elevel", Elements[0], level, 6);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[1]);
  CHECK("elevel", Elements[1], level, 4);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[2]);
  CHECK("elevel", Elements[2], level, 1);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[3]);
  CHECK("elevel", Elements[3], level, 5);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[4]);
  CHECK("elevel", Elements[4], level, 0);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[5]);
  CHECK("elevel", Elements[5], level, 3);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[6]);
  CHECK("elevel", Elements[6], level, 4);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[7]);
  CHECK("elevel", Elements[7], level, 1);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[8]);
  CHECK("elevel", Elements[8], level, 2);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[9]);
  CHECK("elevel", Elements[9], level, 1);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[10]);
  CHECK("elevel", Elements[10], level, 0);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[11]);
  CHECK("elevel", Elements[11], level, 2);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[12]);
  CHECK("elevel", Elements[12], level, 2);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[13]);
  CHECK("elevel", Elements[13], level, 3);
  level = Palo->elevel(DatabaseName, DimensionName, Elements[14]);
  CHECK("elevel", Elements[14], level, 0);
}



static void TestEIndex () {
  int index;

  index = Palo->eindex(DatabaseName, DimensionName, Elements[0]);
  CHECK("eindex", Elements[0], index, 15);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[1]);
  CHECK("eindex", Elements[1], index, 11);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[2]);
  CHECK("eindex", Elements[2], index, 12);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[3]);
  CHECK("eindex", Elements[3], index, 14);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[4]);
  CHECK("eindex", Elements[4], index, 1);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[5]);
  CHECK("eindex", Elements[5], index, 10);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[6]);
  CHECK("eindex", Elements[6], index, 13);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[7]);
  CHECK("eindex", Elements[7], index, 5);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[8]);
  CHECK("eindex", Elements[8], index, 9);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[9]);
  CHECK("eindex", Elements[9], index, 4);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[10]);
  CHECK("eindex", Elements[10], index, 2);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[11]);
  CHECK("eindex", Elements[11], index, 8);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[12]);
  CHECK("eindex", Elements[12], index, 6);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[13]);
  CHECK("eindex", Elements[13], index, 7);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[14]);
  CHECK("eindex", Elements[14], index, 3);
}



static void TestEWeight () {
  double weight;

  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[0]);
  CHECK("eweight", Elements[0] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[1]);
  CHECK("eweight", Elements[0] + "/" + Elements[1], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[2]);
  CHECK("eweight", Elements[0] + "/" + Elements[2], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[3]);
  CHECK("eweight", Elements[0] + "/" + Elements[3], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[4]);
  CHECK("eweight", Elements[0] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[5]);
  CHECK("eweight", Elements[0] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[6]);
  CHECK("eweight", Elements[0] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[7]);
  CHECK("eweight", Elements[0] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[8]);
  CHECK("eweight", Elements[0] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[9]);
  CHECK("eweight", Elements[0] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[10]);
  CHECK("eweight", Elements[0] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[11]);
  CHECK("eweight", Elements[0] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[12]);
  CHECK("eweight", Elements[0] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[13]);
  CHECK("eweight", Elements[0] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[0], Elements[14]);
  CHECK("eweight", Elements[0] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[0]);
  CHECK("eweight", Elements[1] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[1]);
  CHECK("eweight", Elements[1] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[2]);
  CHECK("eweight", Elements[1] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[3]);
  CHECK("eweight", Elements[1] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[4]);
  CHECK("eweight", Elements[1] + "/" + Elements[4], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[5]);
  CHECK("eweight", Elements[1] + "/" + Elements[5], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[6]);
  CHECK("eweight", Elements[1] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[7]);
  CHECK("eweight", Elements[1] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[8]);
  CHECK("eweight", Elements[1] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[9]);
  CHECK("eweight", Elements[1] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[10]);
  CHECK("eweight", Elements[1] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[11]);
  CHECK("eweight", Elements[1] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[12]);
  CHECK("eweight", Elements[1] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[13]);
  CHECK("eweight", Elements[1] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[1], Elements[14]);
  CHECK("eweight", Elements[1] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[0]);
  CHECK("eweight", Elements[2] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[1]);
  CHECK("eweight", Elements[2] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[2]);
  CHECK("eweight", Elements[2] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[3]);
  CHECK("eweight", Elements[2] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[4]);
  CHECK("eweight", Elements[2] + "/" + Elements[4], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[5]);
  CHECK("eweight", Elements[2] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[6]);
  CHECK("eweight", Elements[2] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[7]);
  CHECK("eweight", Elements[2] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[8]);
  CHECK("eweight", Elements[2] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[9]);
  CHECK("eweight", Elements[2] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[10]);
  CHECK("eweight", Elements[2] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[11]);
  CHECK("eweight", Elements[2] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[12]);
  CHECK("eweight", Elements[2] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[13]);
  CHECK("eweight", Elements[2] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[2], Elements[14]);
  CHECK("eweight", Elements[2] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[0]);
  CHECK("eweight", Elements[3] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[1]);
  CHECK("eweight", Elements[3] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[2]);
  CHECK("eweight", Elements[3] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[3]);
  CHECK("eweight", Elements[3] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[4]);
  CHECK("eweight", Elements[3] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[5]);
  CHECK("eweight", Elements[3] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[6]);
  CHECK("eweight", Elements[3] + "/" + Elements[6], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[7]);
  CHECK("eweight", Elements[3] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[8]);
  CHECK("eweight", Elements[3] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[9]);
  CHECK("eweight", Elements[3] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[10]);
  CHECK("eweight", Elements[3] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[11]);
  CHECK("eweight", Elements[3] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[12]);
  CHECK("eweight", Elements[3] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[13]);
  CHECK("eweight", Elements[3] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[3], Elements[14]);
  CHECK("eweight", Elements[3] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[0]);
  CHECK("eweight", Elements[4] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[1]);
  CHECK("eweight", Elements[4] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[2]);
  CHECK("eweight", Elements[4] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[3]);
  CHECK("eweight", Elements[4] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[4]);
  CHECK("eweight", Elements[4] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[5]);
  CHECK("eweight", Elements[4] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[6]);
  CHECK("eweight", Elements[4] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[7]);
  CHECK("eweight", Elements[4] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[8]);
  CHECK("eweight", Elements[4] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[9]);
  CHECK("eweight", Elements[4] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[10]);
  CHECK("eweight", Elements[4] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[11]);
  CHECK("eweight", Elements[4] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[12]);
  CHECK("eweight", Elements[4] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[13]);
  CHECK("eweight", Elements[4] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[4], Elements[14]);
  CHECK("eweight", Elements[4] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[0]);
  CHECK("eweight", Elements[5] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[1]);
  CHECK("eweight", Elements[5] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[2]);
  CHECK("eweight", Elements[5] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[3]);
  CHECK("eweight", Elements[5] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[4]);
  CHECK("eweight", Elements[5] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[5]);
  CHECK("eweight", Elements[5] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[6]);
  CHECK("eweight", Elements[5] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[7]);
  CHECK("eweight", Elements[5] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[8]);
  CHECK("eweight", Elements[5] + "/" + Elements[8], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[9]);
  CHECK("eweight", Elements[5] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[10]);
  CHECK("eweight", Elements[5] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[11]);
  CHECK("eweight", Elements[5] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[12]);
  CHECK("eweight", Elements[5] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[13]);
  CHECK("eweight", Elements[5] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[5], Elements[14]);
  CHECK("eweight", Elements[5] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[0]);
  CHECK("eweight", Elements[6] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[1]);
  CHECK("eweight", Elements[6] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[2]);
  CHECK("eweight", Elements[6] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[3]);
  CHECK("eweight", Elements[6] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[4]);
  CHECK("eweight", Elements[6] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[5]);
  CHECK("eweight", Elements[6] + "/" + Elements[5], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[6]);
  CHECK("eweight", Elements[6] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[7]);
  CHECK("eweight", Elements[6] + "/" + Elements[7], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[8]);
  CHECK("eweight", Elements[6] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[9]);
  CHECK("eweight", Elements[6] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[10]);
  CHECK("eweight", Elements[6] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[11]);
  CHECK("eweight", Elements[6] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[12]);
  CHECK("eweight", Elements[6] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[13]);
  CHECK("eweight", Elements[6] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[6], Elements[14]);
  CHECK("eweight", Elements[6] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[0]);
  CHECK("eweight", Elements[7] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[1]);
  CHECK("eweight", Elements[7] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[2]);
  CHECK("eweight", Elements[7] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[3]);
  CHECK("eweight", Elements[7] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[4]);
  CHECK("eweight", Elements[7] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[5]);
  CHECK("eweight", Elements[7] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[6]);
  CHECK("eweight", Elements[7] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[7]);
  CHECK("eweight", Elements[7] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[8]);
  CHECK("eweight", Elements[7] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[9]);
  CHECK("eweight", Elements[7] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[10]);
  CHECK("eweight", Elements[7] + "/" + Elements[10], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[11]);
  CHECK("eweight", Elements[7] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[12]);
  CHECK("eweight", Elements[7] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[13]);
  CHECK("eweight", Elements[7] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[7], Elements[14]);
  CHECK("eweight", Elements[7] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[0]);
  CHECK("eweight", Elements[8] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[1]);
  CHECK("eweight", Elements[8] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[2]);
  CHECK("eweight", Elements[8] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[3]);
  CHECK("eweight", Elements[8] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[4]);
  CHECK("eweight", Elements[8] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[5]);
  CHECK("eweight", Elements[8] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[6]);
  CHECK("eweight", Elements[8] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[7]);
  CHECK("eweight", Elements[8] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[8]);
  CHECK("eweight", Elements[8] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[9]);
  CHECK("eweight", Elements[8] + "/" + Elements[9], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[10]);
  CHECK("eweight", Elements[8] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[11]);
  CHECK("eweight", Elements[8] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[12]);
  CHECK("eweight", Elements[8] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[13]);
  CHECK("eweight", Elements[8] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[8], Elements[14]);
  CHECK("eweight", Elements[8] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[0]);
  CHECK("eweight", Elements[9] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[1]);
  CHECK("eweight", Elements[9] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[2]);
  CHECK("eweight", Elements[9] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[3]);
  CHECK("eweight", Elements[9] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[4]);
  CHECK("eweight", Elements[9] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[5]);
  CHECK("eweight", Elements[9] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[6]);
  CHECK("eweight", Elements[9] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[7]);
  CHECK("eweight", Elements[9] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[8]);
  CHECK("eweight", Elements[9] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[9]);
  CHECK("eweight", Elements[9] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[10]);
  CHECK("eweight", Elements[9] + "/" + Elements[10], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[11]);
  CHECK("eweight", Elements[9] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[12]);
  CHECK("eweight", Elements[9] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[13]);
  CHECK("eweight", Elements[9] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[9], Elements[14]);
  CHECK("eweight", Elements[9] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[0]);
  CHECK("eweight", Elements[10] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[1]);
  CHECK("eweight", Elements[10] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[2]);
  CHECK("eweight", Elements[10] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[3]);
  CHECK("eweight", Elements[10] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[4]);
  CHECK("eweight", Elements[10] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[5]);
  CHECK("eweight", Elements[10] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[6]);
  CHECK("eweight", Elements[10] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[7]);
  CHECK("eweight", Elements[10] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[8]);
  CHECK("eweight", Elements[10] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[9]);
  CHECK("eweight", Elements[10] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[10]);
  CHECK("eweight", Elements[10] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[11]);
  CHECK("eweight", Elements[10] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[12]);
  CHECK("eweight", Elements[10] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[13]);
  CHECK("eweight", Elements[10] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[10], Elements[14]);
  CHECK("eweight", Elements[10] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[0]);
  CHECK("eweight", Elements[11] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[1]);
  CHECK("eweight", Elements[11] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[2]);
  CHECK("eweight", Elements[11] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[3]);
  CHECK("eweight", Elements[11] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[4]);
  CHECK("eweight", Elements[11] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[5]);
  CHECK("eweight", Elements[11] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[6]);
  CHECK("eweight", Elements[11] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[7]);
  CHECK("eweight", Elements[11] + "/" + Elements[7], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[8]);
  CHECK("eweight", Elements[11] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[9]);
  CHECK("eweight", Elements[11] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[10]);
  CHECK("eweight", Elements[11] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[11]);
  CHECK("eweight", Elements[11] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[12]);
  CHECK("eweight", Elements[11] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[13]);
  CHECK("eweight", Elements[11] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[11], Elements[14]);
  CHECK("eweight", Elements[11] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[0]);
  CHECK("eweight", Elements[12] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[1]);
  CHECK("eweight", Elements[12] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[2]);
  CHECK("eweight", Elements[12] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[3]);
  CHECK("eweight", Elements[12] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[4]);
  CHECK("eweight", Elements[12] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[5]);
  CHECK("eweight", Elements[12] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[6]);
  CHECK("eweight", Elements[12] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[7]);
  CHECK("eweight", Elements[12] + "/" + Elements[7], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[8]);
  CHECK("eweight", Elements[12] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[9]);
  CHECK("eweight", Elements[12] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[10]);
  CHECK("eweight", Elements[12] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[11]);
  CHECK("eweight", Elements[12] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[12]);
  CHECK("eweight", Elements[12] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[13]);
  CHECK("eweight", Elements[12] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[12], Elements[14]);
  CHECK("eweight", Elements[12] + "/" + Elements[14], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[0]);
  CHECK("eweight", Elements[13] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[1]);
  CHECK("eweight", Elements[13] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[2]);
  CHECK("eweight", Elements[13] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[3]);
  CHECK("eweight", Elements[13] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[4]);
  CHECK("eweight", Elements[13] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[5]);
  CHECK("eweight", Elements[13] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[6]);
  CHECK("eweight", Elements[13] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[7]);
  CHECK("eweight", Elements[13] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[8]);
  CHECK("eweight", Elements[13] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[9]);
  CHECK("eweight", Elements[13] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[10]);
  CHECK("eweight", Elements[13] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[11]);
  CHECK("eweight", Elements[13] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[12]);
  CHECK("eweight", Elements[13] + "/" + Elements[12], weight, 0.1);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[13]);
  CHECK("eweight", Elements[13] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[13], Elements[14]);
  CHECK("eweight", Elements[13] + "/" + Elements[14], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[0]);
  CHECK("eweight", Elements[14] + "/" + Elements[0], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[1]);
  CHECK("eweight", Elements[14] + "/" + Elements[1], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[2]);
  CHECK("eweight", Elements[14] + "/" + Elements[2], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[3]);
  CHECK("eweight", Elements[14] + "/" + Elements[3], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[4]);
  CHECK("eweight", Elements[14] + "/" + Elements[4], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[5]);
  CHECK("eweight", Elements[14] + "/" + Elements[5], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[6]);
  CHECK("eweight", Elements[14] + "/" + Elements[6], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[7]);
  CHECK("eweight", Elements[14] + "/" + Elements[7], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[8]);
  CHECK("eweight", Elements[14] + "/" + Elements[8], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[9]);
  CHECK("eweight", Elements[14] + "/" + Elements[9], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[10]);
  CHECK("eweight", Elements[14] + "/" + Elements[10], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[11]);
  CHECK("eweight", Elements[14] + "/" + Elements[11], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[12]);
  CHECK("eweight", Elements[14] + "/" + Elements[12], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[13]);
  CHECK("eweight", Elements[14] + "/" + Elements[13], weight, -9999.0);
  weight = Palo->eweight(DatabaseName, DimensionName, Elements[14], Elements[14]);
  CHECK("eweight", Elements[14] + "/" + Elements[14], weight, -9999.0);
}



static void TestEName () {
  string name;

  name = Palo->ename(DatabaseName, DimensionName, 1);
  CHECK("ename", Elements[0], name, "A5");
  name = Palo->ename(DatabaseName, DimensionName, 2);
  CHECK("ename", Elements[1], name, "A11");
  name = Palo->ename(DatabaseName, DimensionName, 3);
  CHECK("ename", Elements[2], name, "A15");
  name = Palo->ename(DatabaseName, DimensionName, 4);
  CHECK("ename", Elements[3], name, "A10");
  name = Palo->ename(DatabaseName, DimensionName, 5);
  CHECK("ename", Elements[4], name, "A8");
  name = Palo->ename(DatabaseName, DimensionName, 6);
  CHECK("ename", Elements[5], name, "A13");
  name = Palo->ename(DatabaseName, DimensionName, 7);
  CHECK("ename", Elements[6], name, "A14");
  name = Palo->ename(DatabaseName, DimensionName, 8);
  CHECK("ename", Elements[7], name, "A12");
  name = Palo->ename(DatabaseName, DimensionName, 9);
  CHECK("ename", Elements[8], name, "A9");
  name = Palo->ename(DatabaseName, DimensionName, 10);
  CHECK("ename", Elements[9], name, "A6");
  name = Palo->ename(DatabaseName, DimensionName, 11);
  CHECK("ename", Elements[10], name, "A2");
  name = Palo->ename(DatabaseName, DimensionName, 12);
  CHECK("ename", Elements[11], name, "A3");
  name = Palo->ename(DatabaseName, DimensionName, 13);
  CHECK("ename", Elements[12], name, "A7");
  name = Palo->ename(DatabaseName, DimensionName, 14);
  CHECK("ename", Elements[13], name, "A4");
  name = Palo->ename(DatabaseName, DimensionName, 15);
  CHECK("ename", Elements[14], name, "A1");
}

static void TestENext () {
  string name;

  name = Palo->enext(DatabaseName, DimensionName, Elements[0]);
  CHECK("enext", Elements[0], name, "ERROR");
  name = Palo->enext(DatabaseName, DimensionName, Elements[1]);
  CHECK("enext", Elements[1], name, "A3");
  name = Palo->enext(DatabaseName, DimensionName, Elements[2]);
  CHECK("enext", Elements[2], name, "A7");
  name = Palo->enext(DatabaseName, DimensionName, Elements[3]);
  CHECK("enext", Elements[3], name, "A1");
  name = Palo->enext(DatabaseName, DimensionName, Elements[4]);
  CHECK("enext", Elements[4], name, "A11");
  name = Palo->enext(DatabaseName, DimensionName, Elements[5]);
  CHECK("enext", Elements[5], name, "A2");
  name = Palo->enext(DatabaseName, DimensionName, Elements[6]);
  CHECK("enext", Elements[6], name, "A4");
  name = Palo->enext(DatabaseName, DimensionName, Elements[7]);
  CHECK("enext", Elements[7], name, "A13");
  name = Palo->enext(DatabaseName, DimensionName, Elements[8]);
  CHECK("enext", Elements[8], name, "A6");
  name = Palo->enext(DatabaseName, DimensionName, Elements[9]);
  CHECK("enext", Elements[9], name, "A8");
  name = Palo->enext(DatabaseName, DimensionName, Elements[10]);
  CHECK("enext", Elements[10], name, "A15");
  name = Palo->enext(DatabaseName, DimensionName, Elements[11]);
  CHECK("enext", Elements[11], name, "A9");
  name = Palo->enext(DatabaseName, DimensionName, Elements[12]);
  CHECK("enext", Elements[12], name, "A14");
  name = Palo->enext(DatabaseName, DimensionName, Elements[13]);
  CHECK("enext", Elements[13], name, "A12");
  name = Palo->enext(DatabaseName, DimensionName, Elements[14]);
  CHECK("enext", Elements[14], name, "A10");
}



static void TestEPrev () {
  string name;

  name = Palo->eprev(DatabaseName, DimensionName, Elements[0]);
  CHECK("eprev", Elements[0], name, "A4");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[1]);
  CHECK("eprev", Elements[1], name, "A6");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[2]);
  CHECK("eprev", Elements[2], name, "A2");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[3]);
  CHECK("eprev", Elements[3], name, "A7");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[4]);
  CHECK("eprev", Elements[4], name, "ERROR");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[5]);
  CHECK("eprev", Elements[5], name, "A9");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[6]);
  CHECK("eprev", Elements[6], name, "A3");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[7]);
  CHECK("eprev", Elements[7], name, "A10");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[8]);
  CHECK("eprev", Elements[8], name, "A12");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[9]);
  CHECK("eprev", Elements[9], name, "A15");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[10]);
  CHECK("eprev", Elements[10], name, "A5");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[11]);
  CHECK("eprev", Elements[11], name, "A14");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[12]);
  CHECK("eprev", Elements[12], name, "A8");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[13]);
  CHECK("eprev", Elements[13], name, "A13");
  name = Palo->eprev(DatabaseName, DimensionName, Elements[14]);
  CHECK("eprev", Elements[14], name, "A11");
}



static void TestEChildCount () {
  int count;

  count = Palo->echildcount(DatabaseName, DimensionName, Elements[0]);
  CHECK("echildcount", Elements[0], count, 3);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[1]);
  CHECK("echildcount", Elements[1], count, 2);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[2]);
  CHECK("echildcount", Elements[2], count, 1);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[3]);
  CHECK("echildcount", Elements[3], count, 1);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[4]);
  CHECK("echildcount", Elements[4], count, 0);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[5]);
  CHECK("echildcount", Elements[5], count, 1);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[6]);
  CHECK("echildcount", Elements[6], count, 2);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[7]);
  CHECK("echildcount", Elements[7], count, 1);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[8]);
  CHECK("echildcount", Elements[8], count, 1);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[9]);
  CHECK("echildcount", Elements[9], count, 1);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[10]);
  CHECK("echildcount", Elements[10], count, 0);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[11]);
  CHECK("echildcount", Elements[11], count, 1);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[12]);
  CHECK("echildcount", Elements[12], count, 2);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[13]);
  CHECK("echildcount", Elements[13], count, 1);
  count = Palo->echildcount(DatabaseName, DimensionName, Elements[14]);
  CHECK("echildcount", Elements[14], count, 0);
}




static void TestEParentCount () {
  int count;

  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[0]);
  CHECK("eparentcount", Elements[0], count, 0);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[1]);
  CHECK("eparentcount", Elements[1], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[2]);
  CHECK("eparentcount", Elements[2], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[3]);
  CHECK("eparentcount", Elements[3], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[4]);
  CHECK("eparentcount", Elements[4], count, 2);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[5]);
  CHECK("eparentcount", Elements[5], count, 2);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[6]);
  CHECK("eparentcount", Elements[6], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[7]);
  CHECK("eparentcount", Elements[7], count, 3);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[8]);
  CHECK("eparentcount", Elements[8], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[9]);
  CHECK("eparentcount", Elements[9], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[10]);
  CHECK("eparentcount", Elements[10], count, 2);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[11]);
  CHECK("eparentcount", Elements[11], count, 0);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[12]);
  CHECK("eparentcount", Elements[12], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[13]);
  CHECK("eparentcount", Elements[13], count, 0);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[14]);
  CHECK("eparentcount", Elements[14], count, 1);
}




static void TestESibling () {
  string name;

  name = Palo->esibling(DatabaseName, DimensionName, Elements[0], -1);
  CHECK("esibling", Elements[0] + "/" + itoa(-1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[0], 0);
  CHECK("esibling", Elements[0] + "/" + itoa(0), name, "A1");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[0], 1);
  CHECK("esibling", Elements[0] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[0], 2);
  CHECK("esibling", Elements[0] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[0], 3);
  CHECK("esibling", Elements[0] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[0], 4);
  CHECK("esibling", Elements[0] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[1], -1);
  CHECK("esibling", Elements[1] + "/" + itoa(-1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[1], 0);
  CHECK("esibling", Elements[1] + "/" + itoa(0), name, "A2");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[1], 1);
  CHECK("esibling", Elements[1] + "/" + itoa(1), name, "A3");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[1], 2);
  CHECK("esibling", Elements[1] + "/" + itoa(2), name, "A4");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[1], 3);
  CHECK("esibling", Elements[1] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[1], 4);
  CHECK("esibling", Elements[1] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[2], -1);
  CHECK("esibling", Elements[2] + "/" + itoa(-1), name, "A2");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[2], 0);
  CHECK("esibling", Elements[2] + "/" + itoa(0), name, "A3");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[2], 1);
  CHECK("esibling", Elements[2] + "/" + itoa(1), name, "A4");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[2], 2);
  CHECK("esibling", Elements[2] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[2], 3);
  CHECK("esibling", Elements[2] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[2], 4);
  CHECK("esibling", Elements[2] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[3], -1);
  CHECK("esibling", Elements[3] + "/" + itoa(-1), name, "A3");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[3], 0);
  CHECK("esibling", Elements[3] + "/" + itoa(0), name, "A4");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[3], 1);
  CHECK("esibling", Elements[3] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[3], 2);
  CHECK("esibling", Elements[3] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[3], 3);
  CHECK("esibling", Elements[3] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[3], 4);
  CHECK("esibling", Elements[3] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[4], -1);
  CHECK("esibling", Elements[4] + "/" + itoa(-1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[4], 0);
  CHECK("esibling", Elements[4] + "/" + itoa(0), name, "A5");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[4], 1);
  CHECK("esibling", Elements[4] + "/" + itoa(1), name, "A6");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[4], 2);
  CHECK("esibling", Elements[4] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[4], 3);
  CHECK("esibling", Elements[4] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[4], 4);
  CHECK("esibling", Elements[4] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[5], -1);
  CHECK("esibling", Elements[5] + "/" + itoa(-1), name, "A5");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[5], 0);
  CHECK("esibling", Elements[5] + "/" + itoa(0), name, "A6");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[5], 1);
  CHECK("esibling", Elements[5] + "/" + itoa(1), name, "A8");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[5], 2);
  CHECK("esibling", Elements[5] + "/" + itoa(2), name, "A12");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[5], 3);
  CHECK("esibling", Elements[5] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[5], 4);
  CHECK("esibling", Elements[5] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[6], -1);
  CHECK("esibling", Elements[6] + "/" + itoa(-1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[6], 0);
  CHECK("esibling", Elements[6] + "/" + itoa(0), name, "A7");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[6], 1);
  CHECK("esibling", Elements[6] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[6], 2);
  CHECK("esibling", Elements[6] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[6], 3);
  CHECK("esibling", Elements[6] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[6], 4);
  CHECK("esibling", Elements[6] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[7], -1);
  CHECK("esibling", Elements[7] + "/" + itoa(-1), name, "A6");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[7], 0);
  CHECK("esibling", Elements[7] + "/" + itoa(0), name, "A8");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[7], 1);
  CHECK("esibling", Elements[7] + "/" + itoa(1), name, "A12");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[7], 2);
  CHECK("esibling", Elements[7] + "/" + itoa(2), name, "A14");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[7], 3);
  CHECK("esibling", Elements[7] + "/" + itoa(3), name, "A15");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[7], 4);
  CHECK("esibling", Elements[7] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[8], -1);
  CHECK("esibling", Elements[8] + "/" + itoa(-1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[8], 0);
  CHECK("esibling", Elements[8] + "/" + itoa(0), name, "A9");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[8], 1);
  CHECK("esibling", Elements[8] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[8], 2);
  CHECK("esibling", Elements[8] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[8], 3);
  CHECK("esibling", Elements[8] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[8], 4);
  CHECK("esibling", Elements[8] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[9], -1);
  CHECK("esibling", Elements[9] + "/" + itoa(-1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[9], 0);
  CHECK("esibling", Elements[9] + "/" + itoa(0), name, "A10");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[9], 1);
  CHECK("esibling", Elements[9] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[9], 2);
  CHECK("esibling", Elements[9] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[9], 3);
  CHECK("esibling", Elements[9] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[9], 4);
  CHECK("esibling", Elements[9] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[10], -1);
  CHECK("esibling", Elements[10] + "/" + itoa(-1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[10], 0);
  CHECK("esibling", Elements[10] + "/" + itoa(0), name, "A11");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[10], 1);
  CHECK("esibling", Elements[10] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[10], 2);
  CHECK("esibling", Elements[10] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[10], 3);
  CHECK("esibling", Elements[10] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[10], 4);
  CHECK("esibling", Elements[10] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[11], -1);
  CHECK("esibling", Elements[11] + "/" + itoa(-1), name, "A8");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[11], 0);
  CHECK("esibling", Elements[11] + "/" + itoa(0), name, "A12");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[11], 1);
  CHECK("esibling", Elements[11] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[11], 2);
  CHECK("esibling", Elements[11] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[11], 3);
  CHECK("esibling", Elements[11] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[11], 4);
  CHECK("esibling", Elements[11] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[12], -1);
  CHECK("esibling", Elements[12] + "/" + itoa(-1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[12], 0);
  CHECK("esibling", Elements[12] + "/" + itoa(0), name, "A13");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[12], 1);
  CHECK("esibling", Elements[12] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[12], 2);
  CHECK("esibling", Elements[12] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[12], 3);
  CHECK("esibling", Elements[12] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[12], 4);
  CHECK("esibling", Elements[12] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[13], -1);
  CHECK("esibling", Elements[13] + "/" + itoa(-1), name, "A8");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[13], 0);
  CHECK("esibling", Elements[13] + "/" + itoa(0), name, "A14");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[13], 1);
  CHECK("esibling", Elements[13] + "/" + itoa(1), name, "A15");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[13], 2);
  CHECK("esibling", Elements[13] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[13], 3);
  CHECK("esibling", Elements[13] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[13], 4);
  CHECK("esibling", Elements[13] + "/" + itoa(4), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[14], -1);
  CHECK("esibling", Elements[14] + "/" + itoa(-1), name, "A14");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[14], 0);
  CHECK("esibling", Elements[14] + "/" + itoa(0), name, "A15");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[14], 1);
  CHECK("esibling", Elements[14] + "/" + itoa(1), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[14], 2);
  CHECK("esibling", Elements[14] + "/" + itoa(2), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[14], 3);
  CHECK("esibling", Elements[14] + "/" + itoa(3), name, "ERROR");
  name = Palo->esibling(DatabaseName, DimensionName, Elements[14], 4);
  CHECK("esibling", Elements[14] + "/" + itoa(4), name, "ERROR");
}




static void TestEChildName () {
  string name;

  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 0);
  CHECK("echildname", Elements[0] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 1);
  CHECK("echildname", Elements[0] + "/" + itoa(1), name, "A2");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 2);
  CHECK("echildname", Elements[0] + "/" + itoa(2), name, "A3");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 3);
  CHECK("echildname", Elements[0] + "/" + itoa(3), name, "A4");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 4);
  CHECK("echildname", Elements[0] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 5);
  CHECK("echildname", Elements[0] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 6);
  CHECK("echildname", Elements[0] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 7);
  CHECK("echildname", Elements[0] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 8);
  CHECK("echildname", Elements[0] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 9);
  CHECK("echildname", Elements[0] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 10);
  CHECK("echildname", Elements[0] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 11);
  CHECK("echildname", Elements[0] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 12);
  CHECK("echildname", Elements[0] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 13);
  CHECK("echildname", Elements[0] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[0], 14);
  CHECK("echildname", Elements[0] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 0);
  CHECK("echildname", Elements[1] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 1);
  CHECK("echildname", Elements[1] + "/" + itoa(1), name, "A5");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 2);
  CHECK("echildname", Elements[1] + "/" + itoa(2), name, "A6");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 3);
  CHECK("echildname", Elements[1] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 4);
  CHECK("echildname", Elements[1] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 5);
  CHECK("echildname", Elements[1] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 6);
  CHECK("echildname", Elements[1] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 7);
  CHECK("echildname", Elements[1] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 8);
  CHECK("echildname", Elements[1] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 9);
  CHECK("echildname", Elements[1] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 10);
  CHECK("echildname", Elements[1] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 11);
  CHECK("echildname", Elements[1] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 12);
  CHECK("echildname", Elements[1] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 13);
  CHECK("echildname", Elements[1] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[1], 14);
  CHECK("echildname", Elements[1] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 0);
  CHECK("echildname", Elements[2] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 1);
  CHECK("echildname", Elements[2] + "/" + itoa(1), name, "A5");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 2);
  CHECK("echildname", Elements[2] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 3);
  CHECK("echildname", Elements[2] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 4);
  CHECK("echildname", Elements[2] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 5);
  CHECK("echildname", Elements[2] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 6);
  CHECK("echildname", Elements[2] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 7);
  CHECK("echildname", Elements[2] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 8);
  CHECK("echildname", Elements[2] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 9);
  CHECK("echildname", Elements[2] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 10);
  CHECK("echildname", Elements[2] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 11);
  CHECK("echildname", Elements[2] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 12);
  CHECK("echildname", Elements[2] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 13);
  CHECK("echildname", Elements[2] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[2], 14);
  CHECK("echildname", Elements[2] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 0);
  CHECK("echildname", Elements[3] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 1);
  CHECK("echildname", Elements[3] + "/" + itoa(1), name, "A7");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 2);
  CHECK("echildname", Elements[3] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 3);
  CHECK("echildname", Elements[3] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 4);
  CHECK("echildname", Elements[3] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 5);
  CHECK("echildname", Elements[3] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 6);
  CHECK("echildname", Elements[3] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 7);
  CHECK("echildname", Elements[3] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 8);
  CHECK("echildname", Elements[3] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 9);
  CHECK("echildname", Elements[3] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 10);
  CHECK("echildname", Elements[3] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 11);
  CHECK("echildname", Elements[3] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 12);
  CHECK("echildname", Elements[3] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 13);
  CHECK("echildname", Elements[3] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[3], 14);
  CHECK("echildname", Elements[3] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 0);
  CHECK("echildname", Elements[4] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 1);
  CHECK("echildname", Elements[4] + "/" + itoa(1), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 2);
  CHECK("echildname", Elements[4] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 3);
  CHECK("echildname", Elements[4] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 4);
  CHECK("echildname", Elements[4] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 5);
  CHECK("echildname", Elements[4] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 6);
  CHECK("echildname", Elements[4] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 7);
  CHECK("echildname", Elements[4] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 8);
  CHECK("echildname", Elements[4] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 9);
  CHECK("echildname", Elements[4] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 10);
  CHECK("echildname", Elements[4] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 11);
  CHECK("echildname", Elements[4] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 12);
  CHECK("echildname", Elements[4] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 13);
  CHECK("echildname", Elements[4] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[4], 14);
  CHECK("echildname", Elements[4] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 0);
  CHECK("echildname", Elements[5] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 1);
  CHECK("echildname", Elements[5] + "/" + itoa(1), name, "A9");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 2);
  CHECK("echildname", Elements[5] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 3);
  CHECK("echildname", Elements[5] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 4);
  CHECK("echildname", Elements[5] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 5);
  CHECK("echildname", Elements[5] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 6);
  CHECK("echildname", Elements[5] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 7);
  CHECK("echildname", Elements[5] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 8);
  CHECK("echildname", Elements[5] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 9);
  CHECK("echildname", Elements[5] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 10);
  CHECK("echildname", Elements[5] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 11);
  CHECK("echildname", Elements[5] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 12);
  CHECK("echildname", Elements[5] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 13);
  CHECK("echildname", Elements[5] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[5], 14);
  CHECK("echildname", Elements[5] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 0);
  CHECK("echildname", Elements[6] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 1);
  CHECK("echildname", Elements[6] + "/" + itoa(1), name, "A6");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 2);
  CHECK("echildname", Elements[6] + "/" + itoa(2), name, "A8");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 3);
  CHECK("echildname", Elements[6] + "/" + itoa(3), name, "A12");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 4);
  CHECK("echildname", Elements[6] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 5);
  CHECK("echildname", Elements[6] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 6);
  CHECK("echildname", Elements[6] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 7);
  CHECK("echildname", Elements[6] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 8);
  CHECK("echildname", Elements[6] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 9);
  CHECK("echildname", Elements[6] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 10);
  CHECK("echildname", Elements[6] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 11);
  CHECK("echildname", Elements[6] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 12);
  CHECK("echildname", Elements[6] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 13);
  CHECK("echildname", Elements[6] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[6], 14);
  CHECK("echildname", Elements[6] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 0);
  CHECK("echildname", Elements[7] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 1);
  CHECK("echildname", Elements[7] + "/" + itoa(1), name, "A11");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 2);
  CHECK("echildname", Elements[7] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 3);
  CHECK("echildname", Elements[7] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 4);
  CHECK("echildname", Elements[7] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 5);
  CHECK("echildname", Elements[7] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 6);
  CHECK("echildname", Elements[7] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 7);
  CHECK("echildname", Elements[7] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 8);
  CHECK("echildname", Elements[7] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 9);
  CHECK("echildname", Elements[7] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 10);
  CHECK("echildname", Elements[7] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 11);
  CHECK("echildname", Elements[7] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 12);
  CHECK("echildname", Elements[7] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 13);
  CHECK("echildname", Elements[7] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[7], 14);
  CHECK("echildname", Elements[7] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 0);
  CHECK("echildname", Elements[8] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 1);
  CHECK("echildname", Elements[8] + "/" + itoa(1), name, "A10");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 2);
  CHECK("echildname", Elements[8] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 3);
  CHECK("echildname", Elements[8] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 4);
  CHECK("echildname", Elements[8] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 5);
  CHECK("echildname", Elements[8] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 6);
  CHECK("echildname", Elements[8] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 7);
  CHECK("echildname", Elements[8] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 8);
  CHECK("echildname", Elements[8] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 9);
  CHECK("echildname", Elements[8] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 10);
  CHECK("echildname", Elements[8] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 11);
  CHECK("echildname", Elements[8] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 12);
  CHECK("echildname", Elements[8] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 13);
  CHECK("echildname", Elements[8] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[8], 14);
  CHECK("echildname", Elements[8] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 0);
  CHECK("echildname", Elements[9] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 1);
  CHECK("echildname", Elements[9] + "/" + itoa(1), name, "A11");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 2);
  CHECK("echildname", Elements[9] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 3);
  CHECK("echildname", Elements[9] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 4);
  CHECK("echildname", Elements[9] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 5);
  CHECK("echildname", Elements[9] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 6);
  CHECK("echildname", Elements[9] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 7);
  CHECK("echildname", Elements[9] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 8);
  CHECK("echildname", Elements[9] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 9);
  CHECK("echildname", Elements[9] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 10);
  CHECK("echildname", Elements[9] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 11);
  CHECK("echildname", Elements[9] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 12);
  CHECK("echildname", Elements[9] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 13);
  CHECK("echildname", Elements[9] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[9], 14);
  CHECK("echildname", Elements[9] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 0);
  CHECK("echildname", Elements[10] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 1);
  CHECK("echildname", Elements[10] + "/" + itoa(1), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 2);
  CHECK("echildname", Elements[10] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 3);
  CHECK("echildname", Elements[10] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 4);
  CHECK("echildname", Elements[10] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 5);
  CHECK("echildname", Elements[10] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 6);
  CHECK("echildname", Elements[10] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 7);
  CHECK("echildname", Elements[10] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 8);
  CHECK("echildname", Elements[10] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 9);
  CHECK("echildname", Elements[10] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 10);
  CHECK("echildname", Elements[10] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 11);
  CHECK("echildname", Elements[10] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 12);
  CHECK("echildname", Elements[10] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 13);
  CHECK("echildname", Elements[10] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[10], 14);
  CHECK("echildname", Elements[10] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 0);
  CHECK("echildname", Elements[11] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 1);
  CHECK("echildname", Elements[11] + "/" + itoa(1), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 2);
  CHECK("echildname", Elements[11] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 3);
  CHECK("echildname", Elements[11] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 4);
  CHECK("echildname", Elements[11] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 5);
  CHECK("echildname", Elements[11] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 6);
  CHECK("echildname", Elements[11] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 7);
  CHECK("echildname", Elements[11] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 8);
  CHECK("echildname", Elements[11] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 9);
  CHECK("echildname", Elements[11] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 10);
  CHECK("echildname", Elements[11] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 11);
  CHECK("echildname", Elements[11] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 12);
  CHECK("echildname", Elements[11] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 13);
  CHECK("echildname", Elements[11] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[11], 14);
  CHECK("echildname", Elements[11] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 0);
  CHECK("echildname", Elements[12] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 1);
  CHECK("echildname", Elements[12] + "/" + itoa(1), name, "A8");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 2);
  CHECK("echildname", Elements[12] + "/" + itoa(2), name, "A14");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 3);
  CHECK("echildname", Elements[12] + "/" + itoa(3), name, "A15");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 4);
  CHECK("echildname", Elements[12] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 5);
  CHECK("echildname", Elements[12] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 6);
  CHECK("echildname", Elements[12] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 7);
  CHECK("echildname", Elements[12] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 8);
  CHECK("echildname", Elements[12] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 9);
  CHECK("echildname", Elements[12] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 10);
  CHECK("echildname", Elements[12] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 11);
  CHECK("echildname", Elements[12] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 12);
  CHECK("echildname", Elements[12] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 13);
  CHECK("echildname", Elements[12] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[12], 14);
  CHECK("echildname", Elements[12] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 0);
  CHECK("echildname", Elements[13] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 1);
  CHECK("echildname", Elements[13] + "/" + itoa(1), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 2);
  CHECK("echildname", Elements[13] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 3);
  CHECK("echildname", Elements[13] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 4);
  CHECK("echildname", Elements[13] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 5);
  CHECK("echildname", Elements[13] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 6);
  CHECK("echildname", Elements[13] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 7);
  CHECK("echildname", Elements[13] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 8);
  CHECK("echildname", Elements[13] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 9);
  CHECK("echildname", Elements[13] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 10);
  CHECK("echildname", Elements[13] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 11);
  CHECK("echildname", Elements[13] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 12);
  CHECK("echildname", Elements[13] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 13);
  CHECK("echildname", Elements[13] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[13], 14);
  CHECK("echildname", Elements[13] + "/" + itoa(14), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 0);
  CHECK("echildname", Elements[14] + "/" + itoa(0), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 1);
  CHECK("echildname", Elements[14] + "/" + itoa(1), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 2);
  CHECK("echildname", Elements[14] + "/" + itoa(2), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 3);
  CHECK("echildname", Elements[14] + "/" + itoa(3), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 4);
  CHECK("echildname", Elements[14] + "/" + itoa(4), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 5);
  CHECK("echildname", Elements[14] + "/" + itoa(5), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 6);
  CHECK("echildname", Elements[14] + "/" + itoa(6), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 7);
  CHECK("echildname", Elements[14] + "/" + itoa(7), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 8);
  CHECK("echildname", Elements[14] + "/" + itoa(8), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 9);
  CHECK("echildname", Elements[14] + "/" + itoa(9), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 10);
  CHECK("echildname", Elements[14] + "/" + itoa(10), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 11);
  CHECK("echildname", Elements[14] + "/" + itoa(11), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 12);
  CHECK("echildname", Elements[14] + "/" + itoa(12), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 13);
  CHECK("echildname", Elements[14] + "/" + itoa(13), name, "ERROR");
  name = Palo->echildname(DatabaseName, DimensionName, Elements[14], 14);
  CHECK("echildname", Elements[14] + "/" + itoa(14), name, "ERROR");
}




static void TestEParentName () {
  string name;

  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 0);
  CHECK("eparentname", Elements[0] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 1);
  CHECK("eparentname", Elements[0] + "/" + itoa(1), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 2);
  CHECK("eparentname", Elements[0] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 3);
  CHECK("eparentname", Elements[0] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 4);
  CHECK("eparentname", Elements[0] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 5);
  CHECK("eparentname", Elements[0] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 6);
  CHECK("eparentname", Elements[0] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 7);
  CHECK("eparentname", Elements[0] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 8);
  CHECK("eparentname", Elements[0] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 9);
  CHECK("eparentname", Elements[0] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 10);
  CHECK("eparentname", Elements[0] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 11);
  CHECK("eparentname", Elements[0] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 12);
  CHECK("eparentname", Elements[0] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 13);
  CHECK("eparentname", Elements[0] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[0], 14);
  CHECK("eparentname", Elements[0] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 0);
  CHECK("eparentname", Elements[1] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 1);
  CHECK("eparentname", Elements[1] + "/" + itoa(1), name, "A1");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 2);
  CHECK("eparentname", Elements[1] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 3);
  CHECK("eparentname", Elements[1] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 4);
  CHECK("eparentname", Elements[1] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 5);
  CHECK("eparentname", Elements[1] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 6);
  CHECK("eparentname", Elements[1] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 7);
  CHECK("eparentname", Elements[1] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 8);
  CHECK("eparentname", Elements[1] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 9);
  CHECK("eparentname", Elements[1] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 10);
  CHECK("eparentname", Elements[1] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 11);
  CHECK("eparentname", Elements[1] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 12);
  CHECK("eparentname", Elements[1] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 13);
  CHECK("eparentname", Elements[1] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[1], 14);
  CHECK("eparentname", Elements[1] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 0);
  CHECK("eparentname", Elements[2] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 1);
  CHECK("eparentname", Elements[2] + "/" + itoa(1), name, "A1");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 2);
  CHECK("eparentname", Elements[2] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 3);
  CHECK("eparentname", Elements[2] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 4);
  CHECK("eparentname", Elements[2] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 5);
  CHECK("eparentname", Elements[2] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 6);
  CHECK("eparentname", Elements[2] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 7);
  CHECK("eparentname", Elements[2] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 8);
  CHECK("eparentname", Elements[2] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 9);
  CHECK("eparentname", Elements[2] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 10);
  CHECK("eparentname", Elements[2] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 11);
  CHECK("eparentname", Elements[2] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 12);
  CHECK("eparentname", Elements[2] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 13);
  CHECK("eparentname", Elements[2] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[2], 14);
  CHECK("eparentname", Elements[2] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 0);
  CHECK("eparentname", Elements[3] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 1);
  CHECK("eparentname", Elements[3] + "/" + itoa(1), name, "A1");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 2);
  CHECK("eparentname", Elements[3] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 3);
  CHECK("eparentname", Elements[3] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 4);
  CHECK("eparentname", Elements[3] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 5);
  CHECK("eparentname", Elements[3] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 6);
  CHECK("eparentname", Elements[3] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 7);
  CHECK("eparentname", Elements[3] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 8);
  CHECK("eparentname", Elements[3] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 9);
  CHECK("eparentname", Elements[3] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 10);
  CHECK("eparentname", Elements[3] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 11);
  CHECK("eparentname", Elements[3] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 12);
  CHECK("eparentname", Elements[3] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 13);
  CHECK("eparentname", Elements[3] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[3], 14);
  CHECK("eparentname", Elements[3] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 0);
  CHECK("eparentname", Elements[4] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 1);
  CHECK("eparentname", Elements[4] + "/" + itoa(1), name, "A2");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 2);
  CHECK("eparentname", Elements[4] + "/" + itoa(2), name, "A3");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 3);
  CHECK("eparentname", Elements[4] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 4);
  CHECK("eparentname", Elements[4] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 5);
  CHECK("eparentname", Elements[4] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 6);
  CHECK("eparentname", Elements[4] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 7);
  CHECK("eparentname", Elements[4] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 8);
  CHECK("eparentname", Elements[4] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 9);
  CHECK("eparentname", Elements[4] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 10);
  CHECK("eparentname", Elements[4] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 11);
  CHECK("eparentname", Elements[4] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 12);
  CHECK("eparentname", Elements[4] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 13);
  CHECK("eparentname", Elements[4] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[4], 14);
  CHECK("eparentname", Elements[4] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 0);
  CHECK("eparentname", Elements[5] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 1);
  CHECK("eparentname", Elements[5] + "/" + itoa(1), name, "A2");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 2);
  CHECK("eparentname", Elements[5] + "/" + itoa(2), name, "A7");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 3);
  CHECK("eparentname", Elements[5] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 4);
  CHECK("eparentname", Elements[5] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 5);
  CHECK("eparentname", Elements[5] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 6);
  CHECK("eparentname", Elements[5] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 7);
  CHECK("eparentname", Elements[5] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 8);
  CHECK("eparentname", Elements[5] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 9);
  CHECK("eparentname", Elements[5] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 10);
  CHECK("eparentname", Elements[5] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 11);
  CHECK("eparentname", Elements[5] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 12);
  CHECK("eparentname", Elements[5] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 13);
  CHECK("eparentname", Elements[5] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[5], 14);
  CHECK("eparentname", Elements[5] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 0);
  CHECK("eparentname", Elements[6] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 1);
  CHECK("eparentname", Elements[6] + "/" + itoa(1), name, "A4");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 2);
  CHECK("eparentname", Elements[6] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 3);
  CHECK("eparentname", Elements[6] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 4);
  CHECK("eparentname", Elements[6] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 5);
  CHECK("eparentname", Elements[6] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 6);
  CHECK("eparentname", Elements[6] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 7);
  CHECK("eparentname", Elements[6] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 8);
  CHECK("eparentname", Elements[6] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 9);
  CHECK("eparentname", Elements[6] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 10);
  CHECK("eparentname", Elements[6] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 11);
  CHECK("eparentname", Elements[6] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 12);
  CHECK("eparentname", Elements[6] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 13);
  CHECK("eparentname", Elements[6] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[6], 14);
  CHECK("eparentname", Elements[6] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 0);
  CHECK("eparentname", Elements[7] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 1);
  CHECK("eparentname", Elements[7] + "/" + itoa(1), name, "A7");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 2);
  CHECK("eparentname", Elements[7] + "/" + itoa(2), name, "A13");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 3);
  CHECK("eparentname", Elements[7] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 4);
  CHECK("eparentname", Elements[7] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 5);
  CHECK("eparentname", Elements[7] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 6);
  CHECK("eparentname", Elements[7] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 7);
  CHECK("eparentname", Elements[7] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 8);
  CHECK("eparentname", Elements[7] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 9);
  CHECK("eparentname", Elements[7] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 10);
  CHECK("eparentname", Elements[7] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 11);
  CHECK("eparentname", Elements[7] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 12);
  CHECK("eparentname", Elements[7] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 13);
  CHECK("eparentname", Elements[7] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[7], 14);
  CHECK("eparentname", Elements[7] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 0);
  CHECK("eparentname", Elements[8] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 1);
  CHECK("eparentname", Elements[8] + "/" + itoa(1), name, "A6");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 2);
  CHECK("eparentname", Elements[8] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 3);
  CHECK("eparentname", Elements[8] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 4);
  CHECK("eparentname", Elements[8] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 5);
  CHECK("eparentname", Elements[8] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 6);
  CHECK("eparentname", Elements[8] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 7);
  CHECK("eparentname", Elements[8] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 8);
  CHECK("eparentname", Elements[8] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 9);
  CHECK("eparentname", Elements[8] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 10);
  CHECK("eparentname", Elements[8] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 11);
  CHECK("eparentname", Elements[8] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 12);
  CHECK("eparentname", Elements[8] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 13);
  CHECK("eparentname", Elements[8] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[8], 14);
  CHECK("eparentname", Elements[8] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 0);
  CHECK("eparentname", Elements[9] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 1);
  CHECK("eparentname", Elements[9] + "/" + itoa(1), name, "A9");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 2);
  CHECK("eparentname", Elements[9] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 3);
  CHECK("eparentname", Elements[9] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 4);
  CHECK("eparentname", Elements[9] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 5);
  CHECK("eparentname", Elements[9] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 6);
  CHECK("eparentname", Elements[9] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 7);
  CHECK("eparentname", Elements[9] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 8);
  CHECK("eparentname", Elements[9] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 9);
  CHECK("eparentname", Elements[9] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 10);
  CHECK("eparentname", Elements[9] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 11);
  CHECK("eparentname", Elements[9] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 12);
  CHECK("eparentname", Elements[9] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 13);
  CHECK("eparentname", Elements[9] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[9], 14);
  CHECK("eparentname", Elements[9] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 0);
  CHECK("eparentname", Elements[10] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 1);
  CHECK("eparentname", Elements[10] + "/" + itoa(1), name, "A10");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 2);
  CHECK("eparentname", Elements[10] + "/" + itoa(2), name, "A8");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 3);
  CHECK("eparentname", Elements[10] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 4);
  CHECK("eparentname", Elements[10] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 5);
  CHECK("eparentname", Elements[10] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 6);
  CHECK("eparentname", Elements[10] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 7);
  CHECK("eparentname", Elements[10] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 8);
  CHECK("eparentname", Elements[10] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 9);
  CHECK("eparentname", Elements[10] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 10);
  CHECK("eparentname", Elements[10] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 11);
  CHECK("eparentname", Elements[10] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 12);
  CHECK("eparentname", Elements[10] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 13);
  CHECK("eparentname", Elements[10] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[10], 14);
  CHECK("eparentname", Elements[10] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 0);
  CHECK("eparentname", Elements[11] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 1);
  CHECK("eparentname", Elements[11] + "/" + itoa(1), name, "A7");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 2);
  CHECK("eparentname", Elements[11] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 3);
  CHECK("eparentname", Elements[11] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 4);
  CHECK("eparentname", Elements[11] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 5);
  CHECK("eparentname", Elements[11] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 6);
  CHECK("eparentname", Elements[11] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 7);
  CHECK("eparentname", Elements[11] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 8);
  CHECK("eparentname", Elements[11] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 9);
  CHECK("eparentname", Elements[11] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 10);
  CHECK("eparentname", Elements[11] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 11);
  CHECK("eparentname", Elements[11] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 12);
  CHECK("eparentname", Elements[11] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 13);
  CHECK("eparentname", Elements[11] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[11], 14);
  CHECK("eparentname", Elements[11] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 0);
  CHECK("eparentname", Elements[12] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 1);
  CHECK("eparentname", Elements[12] + "/" + itoa(1), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 2);
  CHECK("eparentname", Elements[12] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 3);
  CHECK("eparentname", Elements[12] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 4);
  CHECK("eparentname", Elements[12] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 5);
  CHECK("eparentname", Elements[12] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 6);
  CHECK("eparentname", Elements[12] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 7);
  CHECK("eparentname", Elements[12] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 8);
  CHECK("eparentname", Elements[12] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 9);
  CHECK("eparentname", Elements[12] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 10);
  CHECK("eparentname", Elements[12] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 11);
  CHECK("eparentname", Elements[12] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 12);
  CHECK("eparentname", Elements[12] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 13);
  CHECK("eparentname", Elements[12] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[12], 14);
  CHECK("eparentname", Elements[12] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 0);
  CHECK("eparentname", Elements[13] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 1);
  CHECK("eparentname", Elements[13] + "/" + itoa(1), name, "A13");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 2);
  CHECK("eparentname", Elements[13] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 3);
  CHECK("eparentname", Elements[13] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 4);
  CHECK("eparentname", Elements[13] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 5);
  CHECK("eparentname", Elements[13] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 6);
  CHECK("eparentname", Elements[13] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 7);
  CHECK("eparentname", Elements[13] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 8);
  CHECK("eparentname", Elements[13] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 9);
  CHECK("eparentname", Elements[13] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 10);
  CHECK("eparentname", Elements[13] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 11);
  CHECK("eparentname", Elements[13] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 12);
  CHECK("eparentname", Elements[13] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 13);
  CHECK("eparentname", Elements[13] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[13], 14);
  CHECK("eparentname", Elements[13] + "/" + itoa(14), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 0);
  CHECK("eparentname", Elements[14] + "/" + itoa(0), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 1);
  CHECK("eparentname", Elements[14] + "/" + itoa(1), name, "A13");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 2);
  CHECK("eparentname", Elements[14] + "/" + itoa(2), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 3);
  CHECK("eparentname", Elements[14] + "/" + itoa(3), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 4);
  CHECK("eparentname", Elements[14] + "/" + itoa(4), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 5);
  CHECK("eparentname", Elements[14] + "/" + itoa(5), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 6);
  CHECK("eparentname", Elements[14] + "/" + itoa(6), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 7);
  CHECK("eparentname", Elements[14] + "/" + itoa(7), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 8);
  CHECK("eparentname", Elements[14] + "/" + itoa(8), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 9);
  CHECK("eparentname", Elements[14] + "/" + itoa(9), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 10);
  CHECK("eparentname", Elements[14] + "/" + itoa(10), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 11);
  CHECK("eparentname", Elements[14] + "/" + itoa(11), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 12);
  CHECK("eparentname", Elements[14] + "/" + itoa(12), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 13);
  CHECK("eparentname", Elements[14] + "/" + itoa(13), name, "ERROR");
  name = Palo->eparentname(DatabaseName, DimensionName, Elements[14], 14);
  CHECK("eparentname", Elements[14] + "/" + itoa(14), name, "ERROR");
}




static void TestEIsChild () {
  int flag;

  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[0]);
  CHECK("eischild", Elements[0] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[1]);
  CHECK("eischild", Elements[0] + "/" + Elements[1], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[2]);
  CHECK("eischild", Elements[0] + "/" + Elements[2], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[3]);
  CHECK("eischild", Elements[0] + "/" + Elements[3], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[4]);
  CHECK("eischild", Elements[0] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[5]);
  CHECK("eischild", Elements[0] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[6]);
  CHECK("eischild", Elements[0] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[7]);
  CHECK("eischild", Elements[0] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[8]);
  CHECK("eischild", Elements[0] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[9]);
  CHECK("eischild", Elements[0] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[10]);
  CHECK("eischild", Elements[0] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[11]);
  CHECK("eischild", Elements[0] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[12]);
  CHECK("eischild", Elements[0] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[13]);
  CHECK("eischild", Elements[0] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[0], Elements[14]);
  CHECK("eischild", Elements[0] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[0]);
  CHECK("eischild", Elements[1] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[1]);
  CHECK("eischild", Elements[1] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[2]);
  CHECK("eischild", Elements[1] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[3]);
  CHECK("eischild", Elements[1] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[4]);
  CHECK("eischild", Elements[1] + "/" + Elements[4], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[5]);
  CHECK("eischild", Elements[1] + "/" + Elements[5], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[6]);
  CHECK("eischild", Elements[1] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[7]);
  CHECK("eischild", Elements[1] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[8]);
  CHECK("eischild", Elements[1] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[9]);
  CHECK("eischild", Elements[1] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[10]);
  CHECK("eischild", Elements[1] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[11]);
  CHECK("eischild", Elements[1] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[12]);
  CHECK("eischild", Elements[1] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[13]);
  CHECK("eischild", Elements[1] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[1], Elements[14]);
  CHECK("eischild", Elements[1] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[0]);
  CHECK("eischild", Elements[2] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[1]);
  CHECK("eischild", Elements[2] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[2]);
  CHECK("eischild", Elements[2] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[3]);
  CHECK("eischild", Elements[2] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[4]);
  CHECK("eischild", Elements[2] + "/" + Elements[4], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[5]);
  CHECK("eischild", Elements[2] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[6]);
  CHECK("eischild", Elements[2] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[7]);
  CHECK("eischild", Elements[2] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[8]);
  CHECK("eischild", Elements[2] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[9]);
  CHECK("eischild", Elements[2] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[10]);
  CHECK("eischild", Elements[2] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[11]);
  CHECK("eischild", Elements[2] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[12]);
  CHECK("eischild", Elements[2] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[13]);
  CHECK("eischild", Elements[2] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[2], Elements[14]);
  CHECK("eischild", Elements[2] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[0]);
  CHECK("eischild", Elements[3] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[1]);
  CHECK("eischild", Elements[3] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[2]);
  CHECK("eischild", Elements[3] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[3]);
  CHECK("eischild", Elements[3] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[4]);
  CHECK("eischild", Elements[3] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[5]);
  CHECK("eischild", Elements[3] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[6]);
  CHECK("eischild", Elements[3] + "/" + Elements[6], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[7]);
  CHECK("eischild", Elements[3] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[8]);
  CHECK("eischild", Elements[3] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[9]);
  CHECK("eischild", Elements[3] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[10]);
  CHECK("eischild", Elements[3] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[11]);
  CHECK("eischild", Elements[3] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[12]);
  CHECK("eischild", Elements[3] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[13]);
  CHECK("eischild", Elements[3] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[3], Elements[14]);
  CHECK("eischild", Elements[3] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[0]);
  CHECK("eischild", Elements[4] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[1]);
  CHECK("eischild", Elements[4] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[2]);
  CHECK("eischild", Elements[4] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[3]);
  CHECK("eischild", Elements[4] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[4]);
  CHECK("eischild", Elements[4] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[5]);
  CHECK("eischild", Elements[4] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[6]);
  CHECK("eischild", Elements[4] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[7]);
  CHECK("eischild", Elements[4] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[8]);
  CHECK("eischild", Elements[4] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[9]);
  CHECK("eischild", Elements[4] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[10]);
  CHECK("eischild", Elements[4] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[11]);
  CHECK("eischild", Elements[4] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[12]);
  CHECK("eischild", Elements[4] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[13]);
  CHECK("eischild", Elements[4] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[4], Elements[14]);
  CHECK("eischild", Elements[4] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[0]);
  CHECK("eischild", Elements[5] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[1]);
  CHECK("eischild", Elements[5] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[2]);
  CHECK("eischild", Elements[5] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[3]);
  CHECK("eischild", Elements[5] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[4]);
  CHECK("eischild", Elements[5] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[5]);
  CHECK("eischild", Elements[5] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[6]);
  CHECK("eischild", Elements[5] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[7]);
  CHECK("eischild", Elements[5] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[8]);
  CHECK("eischild", Elements[5] + "/" + Elements[8], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[9]);
  CHECK("eischild", Elements[5] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[10]);
  CHECK("eischild", Elements[5] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[11]);
  CHECK("eischild", Elements[5] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[12]);
  CHECK("eischild", Elements[5] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[13]);
  CHECK("eischild", Elements[5] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[5], Elements[14]);
  CHECK("eischild", Elements[5] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[0]);
  CHECK("eischild", Elements[6] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[1]);
  CHECK("eischild", Elements[6] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[2]);
  CHECK("eischild", Elements[6] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[3]);
  CHECK("eischild", Elements[6] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[4]);
  CHECK("eischild", Elements[6] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[5]);
  CHECK("eischild", Elements[6] + "/" + Elements[5], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[6]);
  CHECK("eischild", Elements[6] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[7]);
  CHECK("eischild", Elements[6] + "/" + Elements[7], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[8]);
  CHECK("eischild", Elements[6] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[9]);
  CHECK("eischild", Elements[6] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[10]);
  CHECK("eischild", Elements[6] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[11]);
  CHECK("eischild", Elements[6] + "/" + Elements[11], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[12]);
  CHECK("eischild", Elements[6] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[13]);
  CHECK("eischild", Elements[6] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[6], Elements[14]);
  CHECK("eischild", Elements[6] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[0]);
  CHECK("eischild", Elements[7] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[1]);
  CHECK("eischild", Elements[7] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[2]);
  CHECK("eischild", Elements[7] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[3]);
  CHECK("eischild", Elements[7] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[4]);
  CHECK("eischild", Elements[7] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[5]);
  CHECK("eischild", Elements[7] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[6]);
  CHECK("eischild", Elements[7] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[7]);
  CHECK("eischild", Elements[7] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[8]);
  CHECK("eischild", Elements[7] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[9]);
  CHECK("eischild", Elements[7] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[10]);
  CHECK("eischild", Elements[7] + "/" + Elements[10], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[11]);
  CHECK("eischild", Elements[7] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[12]);
  CHECK("eischild", Elements[7] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[13]);
  CHECK("eischild", Elements[7] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[7], Elements[14]);
  CHECK("eischild", Elements[7] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[0]);
  CHECK("eischild", Elements[8] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[1]);
  CHECK("eischild", Elements[8] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[2]);
  CHECK("eischild", Elements[8] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[3]);
  CHECK("eischild", Elements[8] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[4]);
  CHECK("eischild", Elements[8] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[5]);
  CHECK("eischild", Elements[8] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[6]);
  CHECK("eischild", Elements[8] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[7]);
  CHECK("eischild", Elements[8] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[8]);
  CHECK("eischild", Elements[8] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[9]);
  CHECK("eischild", Elements[8] + "/" + Elements[9], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[10]);
  CHECK("eischild", Elements[8] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[11]);
  CHECK("eischild", Elements[8] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[12]);
  CHECK("eischild", Elements[8] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[13]);
  CHECK("eischild", Elements[8] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[8], Elements[14]);
  CHECK("eischild", Elements[8] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[0]);
  CHECK("eischild", Elements[9] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[1]);
  CHECK("eischild", Elements[9] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[2]);
  CHECK("eischild", Elements[9] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[3]);
  CHECK("eischild", Elements[9] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[4]);
  CHECK("eischild", Elements[9] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[5]);
  CHECK("eischild", Elements[9] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[6]);
  CHECK("eischild", Elements[9] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[7]);
  CHECK("eischild", Elements[9] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[8]);
  CHECK("eischild", Elements[9] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[9]);
  CHECK("eischild", Elements[9] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[10]);
  CHECK("eischild", Elements[9] + "/" + Elements[10], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[11]);
  CHECK("eischild", Elements[9] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[12]);
  CHECK("eischild", Elements[9] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[13]);
  CHECK("eischild", Elements[9] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[9], Elements[14]);
  CHECK("eischild", Elements[9] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[0]);
  CHECK("eischild", Elements[10] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[1]);
  CHECK("eischild", Elements[10] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[2]);
  CHECK("eischild", Elements[10] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[3]);
  CHECK("eischild", Elements[10] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[4]);
  CHECK("eischild", Elements[10] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[5]);
  CHECK("eischild", Elements[10] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[6]);
  CHECK("eischild", Elements[10] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[7]);
  CHECK("eischild", Elements[10] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[8]);
  CHECK("eischild", Elements[10] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[9]);
  CHECK("eischild", Elements[10] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[10]);
  CHECK("eischild", Elements[10] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[11]);
  CHECK("eischild", Elements[10] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[12]);
  CHECK("eischild", Elements[10] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[13]);
  CHECK("eischild", Elements[10] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[10], Elements[14]);
  CHECK("eischild", Elements[10] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[0]);
  CHECK("eischild", Elements[11] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[1]);
  CHECK("eischild", Elements[11] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[2]);
  CHECK("eischild", Elements[11] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[3]);
  CHECK("eischild", Elements[11] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[4]);
  CHECK("eischild", Elements[11] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[5]);
  CHECK("eischild", Elements[11] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[6]);
  CHECK("eischild", Elements[11] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[7]);
  CHECK("eischild", Elements[11] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[8]);
  CHECK("eischild", Elements[11] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[9]);
  CHECK("eischild", Elements[11] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[10]);
  CHECK("eischild", Elements[11] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[11]);
  CHECK("eischild", Elements[11] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[12]);
  CHECK("eischild", Elements[11] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[13]);
  CHECK("eischild", Elements[11] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[11], Elements[14]);
  CHECK("eischild", Elements[11] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[0]);
  CHECK("eischild", Elements[12] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[1]);
  CHECK("eischild", Elements[12] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[2]);
  CHECK("eischild", Elements[12] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[3]);
  CHECK("eischild", Elements[12] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[4]);
  CHECK("eischild", Elements[12] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[5]);
  CHECK("eischild", Elements[12] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[6]);
  CHECK("eischild", Elements[12] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[7]);
  CHECK("eischild", Elements[12] + "/" + Elements[7], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[8]);
  CHECK("eischild", Elements[12] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[9]);
  CHECK("eischild", Elements[12] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[10]);
  CHECK("eischild", Elements[12] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[11]);
  CHECK("eischild", Elements[12] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[12]);
  CHECK("eischild", Elements[12] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[13]);
  CHECK("eischild", Elements[12] + "/" + Elements[13], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[12], Elements[14]);
  CHECK("eischild", Elements[12] + "/" + Elements[14], flag, 1);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[0]);
  CHECK("eischild", Elements[13] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[1]);
  CHECK("eischild", Elements[13] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[2]);
  CHECK("eischild", Elements[13] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[3]);
  CHECK("eischild", Elements[13] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[4]);
  CHECK("eischild", Elements[13] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[5]);
  CHECK("eischild", Elements[13] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[6]);
  CHECK("eischild", Elements[13] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[7]);
  CHECK("eischild", Elements[13] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[8]);
  CHECK("eischild", Elements[13] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[9]);
  CHECK("eischild", Elements[13] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[10]);
  CHECK("eischild", Elements[13] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[11]);
  CHECK("eischild", Elements[13] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[12]);
  CHECK("eischild", Elements[13] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[13]);
  CHECK("eischild", Elements[13] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[13], Elements[14]);
  CHECK("eischild", Elements[13] + "/" + Elements[14], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[0]);
  CHECK("eischild", Elements[14] + "/" + Elements[0], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[1]);
  CHECK("eischild", Elements[14] + "/" + Elements[1], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[2]);
  CHECK("eischild", Elements[14] + "/" + Elements[2], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[3]);
  CHECK("eischild", Elements[14] + "/" + Elements[3], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[4]);
  CHECK("eischild", Elements[14] + "/" + Elements[4], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[5]);
  CHECK("eischild", Elements[14] + "/" + Elements[5], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[6]);
  CHECK("eischild", Elements[14] + "/" + Elements[6], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[7]);
  CHECK("eischild", Elements[14] + "/" + Elements[7], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[8]);
  CHECK("eischild", Elements[14] + "/" + Elements[8], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[9]);
  CHECK("eischild", Elements[14] + "/" + Elements[9], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[10]);
  CHECK("eischild", Elements[14] + "/" + Elements[10], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[11]);
  CHECK("eischild", Elements[14] + "/" + Elements[11], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[12]);
  CHECK("eischild", Elements[14] + "/" + Elements[12], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[13]);
  CHECK("eischild", Elements[14] + "/" + Elements[13], flag, 0);
  flag = Palo->eischild(DatabaseName, DimensionName, Elements[14], Elements[14]);
  CHECK("eischild", Elements[14] + "/" + Elements[14], flag, 0);
}



static void TestEFirst () {
  string name;

  name = Palo->efirst(DatabaseName, DimensionName);
  CHECK("efirst", DimensionName, name, "A5");

  Palo->emove(DatabaseName, DimensionName, Elements[4], 10);

  name = Palo->efirst(DatabaseName, DimensionName);
  CHECK("efirst", DimensionName, name, "A11");
}




static void TestEIndex2 () {
  int index;

  index = Palo->eindex(DatabaseName, DimensionName, Elements[0]);
  CHECK("eindex", Elements[0], index, 10);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[1]);
  CHECK("eindex", Elements[1], index, 6);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[2]);
  CHECK("eindex", Elements[2], index, 7);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[3]);
  CHECK("eindex", Elements[3], index, 9);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[4]);
  CHECK("eindex", Elements[4], index, 11);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[5]);
  CHECK("eindex", Elements[5], index, 5);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[6]);
  CHECK("eindex", Elements[6], index, 8);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[7]);
  CHECK("eindex", Elements[7], index, 3);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[8]);
  CHECK("eindex", Elements[8], index, 4);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[9]);
  CHECK("eindex", Elements[9], index, 2);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[10]);
  CHECK("eindex", Elements[10], index, 1);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[11]);
  CHECK("eindex", Elements[11], index, 12);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[12]);
  CHECK("eindex", Elements[12], index, 15);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[13]);
  CHECK("eindex", Elements[13], index, 13);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[14]);
  CHECK("eindex", Elements[14], index, 14);

  Palo->emove(DatabaseName, DimensionName, Elements[14], 3);

  index = Palo->eindex(DatabaseName, DimensionName, Elements[0]);
  CHECK("eindex", Elements[0], index, 11);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[1]);
  CHECK("eindex", Elements[1], index, 7);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[2]);
  CHECK("eindex", Elements[2], index, 8);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[3]);
  CHECK("eindex", Elements[3], index, 10);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[4]);
  CHECK("eindex", Elements[4], index, 12);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[5]);
  CHECK("eindex", Elements[5], index, 6);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[6]);
  CHECK("eindex", Elements[6], index, 9);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[7]);
  CHECK("eindex", Elements[7], index, 3);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[8]);
  CHECK("eindex", Elements[8], index, 5);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[9]);
  CHECK("eindex", Elements[9], index, 2);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[10]);
  CHECK("eindex", Elements[10], index, 1);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[11]);
  CHECK("eindex", Elements[11], index, 13);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[12]);
  CHECK("eindex", Elements[12], index, 15);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[13]);
  CHECK("eindex", Elements[13], index, 14);
  index = Palo->eindex(DatabaseName, DimensionName, Elements[14]);
  CHECK("eindex", Elements[14], index, 4);
}



static void TestEType () {
  int type;

  type = Palo->etype(DatabaseName, DimensionName, Elements[0]);
  CHECK("etype", Elements[0], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[1]);
  CHECK("etype", Elements[1], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[2]);
  CHECK("etype", Elements[2], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[3]);
  CHECK("etype", Elements[3], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[4]);
  CHECK("etype", Elements[4], type, 0);
  type = Palo->etype(DatabaseName, DimensionName, Elements[5]);
  CHECK("etype", Elements[5], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[6]);
  CHECK("etype", Elements[6], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[7]);
  CHECK("etype", Elements[7], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[8]);
  CHECK("etype", Elements[8], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[9]);
  CHECK("etype", Elements[9], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[10]);
  CHECK("etype", Elements[10], type, 0);
  type = Palo->etype(DatabaseName, DimensionName, Elements[11]);
  CHECK("etype", Elements[11], type, 0);
  type = Palo->etype(DatabaseName, DimensionName, Elements[12]);
  CHECK("etype", Elements[12], type, 2);
  type = Palo->etype(DatabaseName, DimensionName, Elements[13]);
  CHECK("etype", Elements[13], type, 0);
  type = Palo->etype(DatabaseName, DimensionName, Elements[14]);
  CHECK("etype", Elements[14], type, 0);
}



static void TestRename () {
  Palo->renameElement(DatabaseName, DimensionName, Elements[0], "äöüß");
  Elements[0] = "äöüß";

  int count;

  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[0]);
  CHECK("eparentcount", Elements[0], count, 0);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[1]);
  CHECK("eparentcount", Elements[1], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[2]);
  CHECK("eparentcount", Elements[2], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[3]);
  CHECK("eparentcount", Elements[3], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[4]);
  CHECK("eparentcount", Elements[4], count, 2);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[5]);
  CHECK("eparentcount", Elements[5], count, 2);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[6]);
  CHECK("eparentcount", Elements[6], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[7]);
  CHECK("eparentcount", Elements[7], count, 2);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[8]);
  CHECK("eparentcount", Elements[8], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[9]);
  CHECK("eparentcount", Elements[9], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[10]);
  CHECK("eparentcount", Elements[10], count, 2);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[11]);
  CHECK("eparentcount", Elements[11], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[12]);
  CHECK("eparentcount", Elements[12], count, 0);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[13]);
  CHECK("eparentcount", Elements[13], count, 1);
  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[14]);
  CHECK("eparentcount", Elements[14], count, 1);
}



static void TestEDimension () {
  string dim;
  vector<string> sub;

  sub.clear();
  sub.push_back("A1");

  dim = Palo->edimension(DatabaseName, sub, true);
  CHECK("edimension", DatabaseName, "Dimension2", dim);

  sub.clear();
  sub.push_back("A2");

  dim = Palo->edimension(DatabaseName, sub, true);
  CHECK("edimension", DatabaseName, "ERROR", dim);

  dim = Palo->edimension(DatabaseName, sub, false);
  CHECK("edimension", DatabaseName, "Dimension2", dim);

  sub.clear();
  sub.push_back("A1");
  sub.push_back("A2");

  dim = Palo->edimension(DatabaseName, sub, true);
  CHECK("edimension", DatabaseName, "Dimension2", dim);

  sub.clear();
  sub.push_back("A10");

  dim = Palo->edimension(DatabaseName, sub, true);
  CHECK("edimension", DatabaseName, "Dimension1", dim);
}



#if 0
static void TestEParentCount () {
  int count;

  for (int i = 0;  i < 15;  i++) {
    cout << "  count = Palo->eparentcount(DatabaseName, DimensionName, Elements[" << i << "]);" << endl;
    count = Palo->eparentcount(DatabaseName, DimensionName, Elements[i]);
    cout << "  CHECK(\"eparentcount\", Elements[" << i << "], count, " << count << ");" << endl;
  }
}
#endif



static void showHelp () {
  cout << " --palo <host> <port>              default: localhost 1234\n"
       << " --user <name> <password>          default: user password\n"
       << " --palo10                          do not test esibling, this crashes palo 1.0" << endl;
}



int main (int argc, char * argv []) { 
  string serverName   = "localhost";
  string serverPort   = "1234";
  string user         = "admin";
  string password     = "admin";
  bool   palo10       = false;

  // parse arguments
  for (int i = 1;  i < argc;  i++) {
    string next = argv[i];
    
    if (next == "--palo") {
      if (i + 2 >= argc) {
        showHelp();
        return 0;
      }

      serverName = argv[++i];
      serverPort = argv[++i];
    }
    else if (next == "--user") {
      if (i + 2 >= argc) {
        showHelp();
        return 0;
      }

      user = argv[++i];
      password = argv[++i];
    }
    else if (next == "--palo10") {
      palo10 = true;
    }
    else {
      showHelp();
      return 0;
    }
  }

  // generate connector
  PaloConnector p(serverName, serverPort, user, password);
  Palo = &p;

  // create database and dimension hierarchy
  CreateDatabase();

  // check eindent
  TestEIndent();

  // check elevel
  TestELevel();

  // check eindex
  TestEIndex();

  // check eweight
  TestEWeight();

  // check ename
  TestEName();

  // check enext
  TestENext();

  // check eprev
  TestEPrev();

  // check echildcount
  TestEChildCount();

  // check eparentcount
  TestEParentCount();

  // restructure dimension
  RestructureDimension();

  // check esibling
  if (! palo10) {
    TestESibling();
  }

  // check echildname
  TestEChildName();

  // check eparentname
  TestEParentName();

  // check eischild
  TestEIsChild();

  // check emove and efirst
  TestEFirst();

  // check eindex again
  TestEIndex2();

  // check etype
  TestEType();

  // rename element
  TestRename();

  // check edimension
  TestEDimension();

  // print results
  if (Errors > 0) {
    cout << Errors << " tests failed!" << endl;
  }
  else {
    cout << "all tests passed" << endl;
  }

  return 0;
}
