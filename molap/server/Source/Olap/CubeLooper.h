////////////////////////////////////////////////////////////////////////////////
/// @brief palo cube
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

#ifndef OLAP_CUBE_LOOPER_H
#define OLAP_CUBE_LOOPER_H 1

#include "palo.h"

#include "Olap/CubePage.h"
#include "Olap/CubeStorage.h"
#include "Olap/Cube.h"

#if defined(_MSC_VER)
#pragma warning( disable : 4311 4312 )
#endif

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP cube looper
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CubeLooper {
  public:
    CubeLooper (Cube * cube) : cube(cube) {
      length = cube->getDimensions()->size();
      dimensions = &cube->dimensions;
      identifiers.resize(length);
    }

  private:
    static const uint32_t BLOCKSIZE = 100;

  private:
    static bool isStringElement (CubePage::element_t ptr) {
      return (((long) ptr) & 0x03) == 0x01;
    }

    static bool isDoubleElement (CubePage::element_t ptr) {
      return (((long) ptr) & 0x03) == 0x02;
    }

  public:
    template <typename L>
    double loop (L * looper,
                 uint32_t size, 
                 IdentifiersType * start,
                 bool all,
                 bool skipEmpty,
                 double& maxSize) {

      // setup element mapping
      maxSize = 1.0;
      IdentifiersType first(length, 0);

      for (size_t i = 0;  i < length;  i++) {
        vector<Element*> elements = dimensions->at(i)->getElements(0);

        for (vector<Element*>::iterator j = elements.begin();  j != elements.end();  j++) {
          identifiers[i].push_back((*j)->getIdentifier());
        }

        if (identifiers[i].empty()) {
          return 0.0;
        }

        sort(identifiers[i].begin(), identifiers[i].end());
        first[i] = *(identifiers[i].begin());
        maxSize *= (double) identifiers[i].size();
      }

      IdentifiersType position;

      if (start == 0) {
        position = first;
      }
      else {
        position = *start;
        incrementPath(&position, size);
      }

      // get the sorted list of pointers to the storage
      IdentifiersType last(length, 0);
      vector<CubePage::element_t> elements = getElementList(&position, &last, true);

      if (elements.empty() && skipEmpty) {
        return maxSize;
      }

      while (0 < size && ! elements.empty()) {
        for (vector<CubePage::element_t>::iterator iter = elements.begin();  0 < size && iter != elements.end();  iter++) {
          CubePage::element_t value = *iter;
          CubePage::element_t current = (CubePage::element_t) (((long) value) & ~0x03);
          IdentifiersType path(length, 0);
          
          if (isStringElement(value)) {
            cube->storageString->fillPath(current, &path);
          }
          else if (isDoubleElement(value)) {
            cube->storageDouble->fillPath(current, &path);
          }
          else {
            continue;
          }

          // export empty elements
          if (all) {
            while (0 < size && isLess(&position, &path)) {
              loopElement(looper, &position, size);
              incrementPath(&position, size);
            }

            if (0 == size) {
              continue;
            }
          }
          else {
            position = path;
          }

          // export current element
          if (isStringElement(value)) {
            loopElement(looper, &position, size, * (const char * *) current);
          }
          else if (isDoubleElement(value)) {
            loopElement(looper, &position, size, * (double *) current);
          }

          incrementPath(&position, size);
        }

        if (size != 0) {
          elements = getElementList(&position, &last, false);
        }
      }

      if (all && elements.empty()) {
        while (0 < size) {
          loopElement(looper, &position, size);
          incrementPath(&position, size);
        }

        return maxSize;
      }
      else if (0 < size && elements.empty()) {
        return maxSize;
      }
      else {
        return pathPosition(&position) + 1;
      }
    }

  private:
    struct CompareResult 
      : public std::binary_function<const CubePage::element_t&, const CubePage::element_t&, bool> {
      CompareResult (size_t valueSizeString, size_t valueSizeDouble, size_t length) 
        : valueSizeString(valueSizeString), valueSizeDouble(valueSizeDouble), length(length) {
      }

      bool operator () (const CubePage::element_t& a, const CubePage::element_t& b) {
        IdentifierType * aa = 0;
        IdentifierType * bb = 0;

        if (isStringElement(a)) {
          aa = (IdentifierType*) (((CubePage::key_t) (((long) a) & ~0x03)) + valueSizeString);
        }
        else if (isDoubleElement(a)) {
          aa = (IdentifierType*) (((CubePage::key_t) (((long) a) & ~0x03)) + valueSizeDouble);
        }

        if (isStringElement(b)) {
          bb = (IdentifierType*) (((CubePage::key_t) (((long) b) & ~0x03)) + valueSizeString);
        }
        else if (isDoubleElement(b)) {
          bb = (IdentifierType*) (((CubePage::key_t) (((long) b) & ~0x03)) + valueSizeDouble);
        }

        for (size_t pos = 0;  pos < length;  aa++, bb++, pos++) {
          if (*aa < *bb) {
            return true;
          }
          else if (*aa > *bb) {
            return false;
          }
        }

        return false;
      }

      size_t valueSizeString;
      size_t valueSizeDouble;
      size_t length;
    };



    vector<CubePage::element_t> getElementList (IdentifiersType * position,
                                                IdentifiersType * last,
                                                bool first) {
      vector<CubePage::element_t> result;
      CubePage * pageString = 0;
      CubePage * pageDouble = 0;

      CubeStorage * storageString = cube->storageString;
      CubeStorage * storageDouble = cube->storageDouble;

      if (length == 0) {
        return result;
      }

      IdentifiersType next(length, 0);

      if (last != 0) {
        next = *last;
      }

      bool done = false;

      while (! done) {

        // one-dimensional cube has only one cube page
        if (length == 1) {
          if (first) {
            pageString = storageString->lookupCubePage(0, 0);
            pageDouble = storageDouble->lookupCubePage(0, 0);
            first = false;
          }
          else {
            return result;
          }
        }

        // two-dimensional cube has a page for first dimension
        else if (length == 2) {
          IdentifierType id1 = position->at(0);
          IdentifierType id2 = 0;

          if (first) {
            first = false;
            next = *position;
          }
          else {
            if (next.at(0) == id1) {
              if (! incrementPath(&id1)) {
                last->at(0) = id1;
                return result;
              }
            }
          }

          pageString = storageString->lookupCubePage(id1, id2);
          pageDouble = storageDouble->lookupCubePage(id1, id2);

          while (pageString == 0 && pageDouble == 0) {
            if (! incrementPath(&id1)) {
              last->at(0) = id1;
              return result;
            }

            pageString = storageString->lookupCubePage(id1, id2);
            pageDouble = storageDouble->lookupCubePage(id1, id2);
          }

          next.at(0) = last->at(0) = id1;
        }

        // general case, cube depends on first and second dimension
        else {
          IdentifierType id1 = position->at(0);
          IdentifierType id2 = position->at(1);

          if (first) {
            first = false;
            next = *position;
          }
          else {
            if (next.at(0) == id1) {
              if (! incrementPath(&id1, &id2)) {
                last->at(0) = id1;
                last->at(1) = id2;
                return result;
              }
            }
          }

          pageString = storageString->lookupCubePage(id1, id2);
          pageDouble = storageDouble->lookupCubePage(id1, id2);

          while (pageString == 0 && pageDouble == 0) {
            if (! incrementPath(&id1, &id2)) {
              last->at(0) = id1;
              last->at(1) = id2;
              return result;
            }

            pageString = storageString->lookupCubePage(id1, id2);
            pageDouble = storageDouble->lookupCubePage(id1, id2);
          }

          next.at(0) = last->at(0) = id1;
          next.at(1) = last->at(1) = id2;
        }

        // check if the page contains any suitable string element
        CubePage::buffer_t begin;
        CubePage::buffer_t end;

        if (pageString != 0) {
          begin = pageString->begin();
          end   = pageString->end();

          while (begin < end) {
            if (! isLess(pageString, begin, position)) {
              result.push_back((CubePage::buffer_t) (((long) begin) | 0x01));
              done = true;
            }

            begin += pageString->rowSize;
          }
        }

        // check if the page contains any suitable double element
        if (pageDouble != 0) {
          begin = pageDouble->begin();
          end   = pageDouble->end();

          while (begin < end) {
            if (! isLess(pageDouble, begin, position)) {
              result.push_back((CubePage::buffer_t) (((long) begin) | 0x02));
              done = true;
            }

            begin += pageDouble->rowSize;
          }
        }
      }

      // sort list of elements
      CompareResult compare(storageString->valueSize, storageDouble->valueSize, length);
      sort(result.begin(), result.end(), compare);

      return result;
    }



    bool isLess (CubePage * page, CubePage::buffer_t left, IdentifiersType * right) {
      CubePage::key_t key = left + page->valueSize;

      for (IdentifiersType::iterator i = right->begin(); i != right->end(); i++, key += sizeof(IdentifierType)) {
        IdentifierType l = *(IdentifierType*) key;
        IdentifierType r = *i;

        if (l < r) {
          return true;
        }
        else if (l > r) {
          return false;
        }
      }

      return false;
    }



    template <typename L>
    void loopElement (L * looper,
                      IdentifiersType * position,
                      uint32_t& size,
                      double value) {
      if (looper->loopDouble(position, value)) {
        size--;
      }
    }



    template <typename L>
    void loopElement (L * looper,
                      IdentifiersType * position,
                      uint32_t& size,
                      const char * value) {
      if (looper->loopString(position, value)) {
        size--;
      }
    }



    template <typename L>
    void loopElement (L * looper,
                      IdentifiersType * position,
                      uint32_t& size) {
      CellPath cp(cube, position);
      Element::ElementType type = cp.getPathType();

      if (type == Element::NUMERIC) {
        if (looper->loopDouble(position)) {
          size--;
        }
      }
      else if (type == Element::STRING) {
        if (looper->loopString(position)) {
          size--;
        }
      }
      else if (type == Element::CONSOLIDATED) {
        if (looper->loopConsolidated(position, &cp)) {
          size--;
        }
      }
    }



    void incrementPath (IdentifiersType * a, uint32_t& size) {
      int32_t pos = (int32_t) length - 1;

      while (0 <= pos) {
        vector<IdentifierType>& list = identifiers.at(pos);
        vector<IdentifierType>::iterator i = find(list.begin(), list.end(), a->at(pos));
      
        // should never happen
        if (i == list.end()) {
          Logger::error << "element not found in dimension hierarchy" << endl;
          size = 0;
          return;
        }

        // get next dimension element
        i++;

        if (i == list.end()) {
          for (size_t j = pos;  j < length;  j++) {
            a->at(j) = *identifiers.at(j).begin();
          }

          pos -= 1;
        }
        else {
          a->at(pos) = *i;
          return;
        }
      }

      size = 0;
    }



    bool incrementPath (IdentifierType * id1) {
      vector<IdentifierType>& list = identifiers.at(0);
      vector<IdentifierType>::iterator i = find(list.begin(), list.end(), *id1);

      i++;

      if (i == list.end()) {
        return false;
      }
      

      *id1 = *i;

      return true;
    }



    bool incrementPath (IdentifierType * id1, IdentifierType * id2) {
      vector<IdentifierType>& list2 = identifiers.at(1);
      vector<IdentifierType>::iterator j = find(list2.begin(), list2.end(), *id2);
        
      j++;

      if (j != list2.end()) {
        *id2 = *j;
        return true;
      }

      *id2 = *list2.begin();

      vector<IdentifierType>& list1 = identifiers.at(0);
      vector<IdentifierType>::iterator i = find(list1.begin(), list1.end(), *id1);

      i++;

      if (i != list1.end()) {
        *id1 = *i;
        return true;
      }

      return false;
    }



    bool isLess (const IdentifiersType * a, const IdentifiersType * b) {
      IdentifiersType::const_iterator ia = a->begin();
      IdentifiersType::const_iterator ib = b->begin();

      for (;  ia != a->end();  ia++, ib++) {
        if (*ia < *ib) {
          return true;
        }
        else if (*ia > *ib) {
          return false;
        }
      }
      
      return false;
    }



    double pathPosition (IdentifiersType * path) {
      double size = 0.0;

      for (uint32_t i = 0;  i < length;  i++) {
        vector<IdentifierType>& list = identifiers[i];
        vector<IdentifierType>::iterator f = find(list.begin(), list.end(), (*path)[i]);
        int32_t pos = (int32_t) (f - list.begin());
        
        size = (size * list.size()) + (double) pos;
      }

      return size;
    }

  private:
    Cube * cube;
    size_t length;
    vector<Dimension*> * dimensions;
    vector<IdentifiersType> identifiers;
  };
}

#endif
