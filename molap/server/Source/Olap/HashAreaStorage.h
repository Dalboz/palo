////////////////////////////////////////////////////////////////////////////////
/// @brief palo hash storage
///
/// @file
///
/// The contents of this file are subject to the Jedox AG Palo license. You
/// may not use this file except in compliance with the license. You may obtain
/// a copy of the License at
///
/// <a href="http://www.palo.com/license.txt">
///   http://www.palo.com/license.txt
/// </a>
///
/// Software distributed under the license is distributed on an "AS IS" basis,
/// WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the license
/// for the specific language governing rights and limitations under the
/// license.
///
/// Developed by triagens GmbH, Koeln on behalf of Jedox AG. Intellectual
/// property rights has triagens GmbH, Koeln. Exclusive worldwide
/// exploitation right (commercial copyright) has Jedox AG, Freiburg.
///
/// @author Frank Celler, triagens GmbH, Cologne, Germany
/// @author Achim Brandt, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef OLAP_HASH_AREA_STORAGE_H
#define OLAP_HASH_AREA_STORAGE_H 1

#include "palo.h"

#include <limits>

#include <math.h>

#if defined(_MSC_VER)
#include <float.h>
#endif

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo hash area storage
  ///
  /// A storage that is used to hold a specific sub-cube. The access to the
  /// elements is done via a precomputed hash function
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS HashAreaStorage {

  public:
    HashAreaStorage (const vector< map<Element*, uint32_t> >& hashMapping)
      : hashMapping(hashMapping) {

      // compute the number of elements
      areaSize = 1;

      for (vector< map<Element*, uint32_t> >::const_iterator i = hashMapping.begin();
           i != hashMapping.end();
           i++) {
        areaSize *= (*i).size();
      }

      Logger::debug << "hash area size " << areaSize << endl;

      storage = new double[areaSize];
      //memset(storage, 0, sizeof(double) * areaSize);
      double * s = storage;
      for (size_t i = 0;  i < areaSize;  i++, s++) {
        *s = numeric_limits<double>::quiet_NaN();
      }
      
      length = hashMapping.size();
    }

    ~HashAreaStorage () {
      delete[] storage;
    }

  public:
    double* getArea () {
      return storage;
    }

    void fillAreaStorage(AreaStorage* area,
                         vector< map< uint32_t, vector< pair<IdentifierType, double> > > >& reverseMapping,
                         vector<uint32_t>& hashSteps,
                         vector<uint32_t>& lengths) {

      vector<uint32_t> numList(length, 0);
      vector<uint32_t> hashList(length, 0);
      IdentifiersType path(length, 0);
      double * s = storage;

      for (size_t i = 0;  i < areaSize;  i++, s++) {
        if (!isnan(*s)) 
        fillAreaStorageRecursive(area,
                                 path,
                                 reverseMapping,
                                 hashList,
                                 0,
                                 *s);
        // increment path
        size_t pos = 0;

        while (pos < length) {
          numList[pos]++;

          if (numList[pos] == lengths[pos]) {
            numList[pos] = 0;
            hashList[pos] = 0;
            pos++;
          }
          else {
            hashList[pos] = hashSteps[pos] * numList[pos];
            break;
          }
        }
      }
    }

  private:
    void fillAreaStorageRecursive (AreaStorage* area,
                                   IdentifiersType& path,
                                   vector< map< uint32_t, vector< pair<IdentifierType, double> > > >& reverseMapping,
                                   vector<uint32_t>& hashList,
                                   size_t pos,
                                   double value) {
      if (pos == length - 1) {
        vector< pair<IdentifierType, double> >& l = reverseMapping[pos][hashList[pos]];

        for (vector< pair<IdentifierType, double> >::iterator i = l.begin();  i != l.end();  i++) {
          path[pos] = i->first;

          // for (size_t j = 0;  j < length;  j++) {
          //   cout << path[j] << " ";
          // }
          // 
          // cout << " = " << (value * i->second) << endl;

          area->addDoubleValue(&path, value * i->second);
        }
      }
      else {
        vector< pair<IdentifierType, double> >& l = reverseMapping[pos][hashList[pos]];

        for (vector< pair<IdentifierType, double> >::iterator i = l.begin();  i != l.end();  i++) {
          path[pos] = i->first;

          fillAreaStorageRecursive(area, path, reverseMapping, hashList, pos + 1, value * i->second);
        }
      }
    }

  private:
    size_t areaSize;
    double * storage;
    size_t length;
    vector< map<Element*, uint32_t> > hashMapping;
  };

}

#endif


