////////////////////////////////////////////////////////////////////////////////
/// @brief palo cache rules storage
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

#ifndef OLAP_CACHE_RULES_STORAGE_H
#define OLAP_CACHE_RULES_STORAGE_H 1

#include "palo.h"

#include <set>

#include "Exceptions/ErrorException.h"

#include "InputOutput/Logger.h"

#include "Olap/CellPath.h"
#include "Olap/CubeIndex.h"
#include "Olap/CacheStorage.h"
#include "Olap/CacheRulesPage.h"
#include "Olap/Element.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo cache storage
  ///
  /// A cache storage uses a cache page with a fixed memory size to store the values. 
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CacheRulesStorage: public CacheStorage {

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Creates an empty cache storage
    ////////////////////////////////////////////////////////////////////////////////

    CacheRulesStorage (const vector<size_t>* sizeDimensions, size_t valueSize);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Remove a cache storage
    ////////////////////////////////////////////////////////////////////////////////

    ~CacheRulesStorage ();

  public:
    void clear ();
    
  protected:
    set<CacheStorage*>* getStorages ();
    
  private:
    static set<CacheStorage*> cacheSet;

  private:
    static void registerCache(CacheRulesStorage* cache) {
      cacheSet.insert(cache);
    }
    
    static void removeCache(CacheRulesStorage* cache) {
      cacheSet.erase(cache);
    } 
    
  };

}

#endif 
