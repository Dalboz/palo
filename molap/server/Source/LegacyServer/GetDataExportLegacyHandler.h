////////////////////////////////////////////////////////////////////////////////
/// @brief getdata_export
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

#ifndef LEGACY_SERVER_GET_DATA_EXPORT_LEGACY_HANDLER_H
#define LEGACY_SERVER_GET_DATA_EXPORT_LEGACY_HANDLER_H 1

#include "palo.h"

#include <iostream>

#include "Exceptions/ParameterException.h"

#include "Collections/StringUtils.h"

#include "InputOutput/BinaryCondition.h"
#include "InputOutput/Condition.h"
#include "InputOutput/ConstantCondition.h"
#include "InputOutput/UnaryCondition.h"

#include "Olap/CellPath.h"
#include "Olap/CubeLooper.h"
#include "Olap/Database.h"
#include "Olap/Server.h"

#include "LegacyServer/LegacyRequestHandler.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief getdata_export
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS GetDataExportLegacyHandler : public LegacyRequestHandler {
  public:
    GetDataExportLegacyHandler (Server * server) 
      : LegacyRequestHandler(server) {
    }

  public:
    LegacyArgument * handleLegacyRequest (const vector<LegacyArgument*>* arguments, User* user, PaloSession*) {
      Statistics::Timer timer("LegacyHandler::getdata_export");

      if (arguments->size() != 5) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting database, cube, area, and options as arguments for getdata_export",
                                 "number of arguments", (int) arguments->size() - 1);
      }
      
      this->user = user;

      Database * database   = findDatabase(arguments, 1, user);
      cube                  = findCube(database, arguments, 2, user);
      ArgumentMulti * multi = asMulti(arguments, 3);

      dimensions = cube->getDimensions();
      numDimensions = dimensions->size();

      // extract options
      const vector<LegacyArgument*> * options = asMulti(arguments, 4)->getValue();

      if (options->size() != 9) {
        throw ParameterException(ErrorException::ERROR_INV_ARG_COUNT,
                                 "expecting an options block for getdata_export",
                                 "number of options", (int) options->size());
      }

      baseOnly                           = asInt32(options, 0)->getValue() == 1;
      skipEmpty                          = asInt32(options, 1)->getValue() == 1;
      int32_t compare1                   = asInt32(options, 2)->getValue();
      double value1                      = asDouble(options, 3)->getValue();
      int32_t andor                      = asInt32(options, 4)->getValue();
      int32_t compare2                   = asInt32(options, 5)->getValue();
      double value2                      = asDouble(options, 6)->getValue();
      const vector<LegacyArgument*> * ba = asMulti(options, 7)->getValue();
      blocksize                          = asUInt32(options, 8)->getValue();

      // extract area
      area.clear();
      area.resize(dimensions->size());
      const vector<LegacyArgument*> * value = multi->getValue();

      if (value->empty()) {
        for (size_t i = 0;  i < numDimensions;  i++) {
          vector<Element*> elements = (*dimensions)[i]->getElements(user);

          for (vector<Element*>::iterator j = elements.begin();  j != elements.end();  j++) {
            Element * element = *j;

            if (! baseOnly || (element->getElementType() == Element::NUMERIC || element->getElementType() == Element::STRING)) {
              area[i].insert(element->getIdentifier());
            }
          }
        }
      }
      else {

        // check size of paths
        if (value->size() != dimensions->size()) {
          throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                   "wrong number of dimensions",
                                   "number of dimensions", (int)(dimensions->size()));
        }

        for (size_t i = 0;  i < value->size();  i++) {
          const vector<LegacyArgument*> * single = asMulti(value, i)->getValue();
        
          for (size_t j = 0;  j < single->size();  j++) {
            Element * element = (*dimensions)[i]->findElementByName(asString(single, j)->getValue(), user);
            
            if (! baseOnly || (element->getElementType() == Element::NUMERIC || element->getElementType() == Element::STRING)) {
              area[i].insert(element->getIdentifier());
            }
          }
        }
      }

      // convert begin after path
      IdentifiersType beginAfter;
      bool useBeginAfter = true;

      if (ba->empty()) {
        useBeginAfter = false;
      }
      else {
        if (ba->size() != dimensions->size()) {
          throw ParameterException(ErrorException::ERROR_INVALID_COORDINATES,
                                   "illegal begin after",
                                   "begin after", (int) ba->size());
        }

        for (uint32_t i = 0;  i <dimensions->size();  i++) {
          IdentifierType id = (*dimensions)[i]->findElementByName(asString(ba, i)->getValue(), user)->getIdentifier();
            
          beginAfter.push_back(id);
        }
      }

      // compute condition
      condition = computeCondition(compare1, value1, andor, compare2, value2);
      
      // generate response
      return generateResponse(useBeginAfter ? &beginAfter : 0);
    }

  public:
    bool loopString (const IdentifiersType * path, const char * value) {
      for (size_t i = 0;  i < numDimensions;  i++) {
        if (area[i].find((*path)[i]) == area[i].end()) {
          return false;
        }
      }

      ArgumentMulti * p = new ArgumentMulti();

      for (size_t i = 0;  i < path->size();  i++) {
        Element * element = (*dimensions)[i]->findElement((*path)[i], user);
        p->append(new ArgumentString(element->getName()));
      }

      ArgumentMulti * pair = new ArgumentMulti();
      pair->append(p);
      pair->append(new ArgumentString(value));

      expt->append(pair);
      size++;

      return true;
    }



    bool loopString (const IdentifiersType * path) {
      if (skipEmpty) {
        return false;
      }

      return loopString(path, "");
    }



    bool loopDouble (const IdentifiersType * path, double value) {
      if (condition != 0 && ! condition->check(value)) {
        return false;
      }

      for (size_t i = 0;  i < numDimensions;  i++) {
        if (area[i].find((*path)[i]) == area[i].end()) {
          return false;
        }
      }

      ArgumentMulti * p = new ArgumentMulti();

      for (size_t i = 0;  i < path->size();  i++) {
        Element * element = (*dimensions)[i]->findElement((*path)[i], user);
        p->append(new ArgumentString(element->getName()));
      }

      ArgumentMulti * pair = new ArgumentMulti();
      pair->append(p);
      pair->append(new ArgumentDouble(value));

      expt->append(pair);
      size++;

      return true;
    }



    bool loopDouble (const IdentifiersType * path) {
      if (skipEmpty) {
        return false;
      }

      return loopDouble(path, 0.0);
    }



    bool loopConsolidated (const IdentifiersType * path, CellPath * cp) {
      if (baseOnly) {
        return false;
      }

      for (size_t i = 0;  i < numDimensions;  i++) {
        if (area[i].find((*path)[i]) == area[i].end()) {
          return false;
        }
      }

      bool found;
      set< pair<Rule*, IdentifiersType> > ruleHistory;
      double value = cube->getCellValue(cp, &found, user, 0, &ruleHistory).doubleValue;

      if (! found && skipEmpty) {
        return false;
      }

      if (condition != 0 && ! condition->check(value)) {
        return false;
      }

      ArgumentMulti * p = new ArgumentMulti();

      for (size_t i = 0;  i < path->size();  i++) {
        Element * element = (*dimensions)[i]->findElement((*path)[i], user);
        p->append(new ArgumentString(element->getName()));
      }

      ArgumentMulti * pair = new ArgumentMulti();
      pair->append(p);
      pair->append(new ArgumentDouble(value));

      expt->append(pair);
      size++;

      return true;
    }

  private:
    Condition * computeCondition (int32_t compare1, double value1, int32_t andor, int32_t compare2, double value2) {
      Condition * left = 0;
      Condition * right = 0;

      try {
        left = computeCondition(compare1, value1);
        right = computeCondition(compare2, value2);

        switch (andor) {
          case 1:
            return new BinaryCondition< logical_and<bool> >(left, right);

          case 2:
            return new BinaryCondition< logical_or<bool> >(left, right);

          case 3:
            return new BinaryCondition< Condition::logical_xor<bool> >(left, right);

          default:
            throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                                     "malformed condition",
                                     "andor", (int) andor);
        }
      }
      catch (...) {
        if (left != 0) { delete left; }
        if (right != 0) { delete right; }
        throw;
      }
    }

    Condition * computeCondition (int32_t compare, double value) {
      switch (compare) {
        case 1:
          return new UnaryCondition< less<double>, less<string> >(value);

        case 2:
          return new UnaryCondition< greater<double>, greater<string> >(value);

        case 3:
          return new UnaryCondition< less_equal<double>, less_equal<string> >(value);

        case 4:
          return new UnaryCondition< greater_equal<double>, greater_equal<string> >(value);

        case 5:
          return new UnaryCondition< equal_to<double>, equal_to<string> >(value);

        case 6:
          return new UnaryCondition< not_equal_to<double>, not_equal_to<string> >(value);

        case 7:
          return new ConstantCondition<true>();

        default:
          throw ParameterException(ErrorException::ERROR_CONVERSION_FAILED,
                                   "malformed condition",
                                   "compare", (int) compare);
      }
    }



    LegacyArgument * generateResponse (IdentifiersType * beginAfter) {
      size = 0;
      expt = new ArgumentMulti();
      double f = 0.0;
      CubeLooper looper(cube);

      try {
        double max;
        double pos = looper.loop(this, blocksize, beginAfter, ! baseOnly || ! skipEmpty, skipEmpty, max);
        f = (double) pos / (double) max;
      }
      catch (...) {
        delete expt;
        if (condition != 0) { delete condition; }
        area.clear();
        throw;
      }

      if (condition != 0) { delete condition; }
      area.clear();

      ArgumentMulti * result = new ArgumentMulti();
      result->append(expt);
      result->append(new ArgumentDouble(f));

      return result;
    }



  private:
    User * user;
    Cube * cube;
    const vector<Dimension*> * dimensions;
    size_t numDimensions;
    uint32_t blocksize;
    vector< set<IdentifierType> > area;
    bool baseOnly;
    bool skipEmpty;
    ArgumentMulti * expt;
    Condition * condition;
    size_t size;
  };

}

#endif

