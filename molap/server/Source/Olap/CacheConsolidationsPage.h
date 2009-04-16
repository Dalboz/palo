////////////////////////////////////////////////////////////////////////////////
/// @brief palo cache consolidations page
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

#ifndef OLAP_CACHE_CONSOLIDATIONS_PAGE_H
#define OLAP_CACHE_CONSOLIDATIONS_PAGE_H 1

#include "palo.h"

#include "Olap/CachePage.h"

#include <set>

namespace palo {
  class CubeIndex;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo cache page
  /// 
  /// A cache page stores/cachss the consolidated cell data into a fixed memory size.
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CacheConsolidationsPage : public CachePage {

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the cache size of all cache pages
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t getTotalCacheSize () {
      return totalCacheSize; 
    }   

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the maximum cache size 
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t getMaximumCacheSize () {
      return maximumCacheSize; 
    }   

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief set the maximum cache size 
    ////////////////////////////////////////////////////////////////////////////////
    
    static void setMaximumCacheSize (size_t maximum) {
      maximumCacheSize = maximum; 
    }   

    static bool useCache () {
      return (maximumCacheSize>0) ? true : false; 
    }   

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates an empty cache page 
    ////////////////////////////////////////////////////////////////////////////////

    CacheConsolidationsPage (CubeIndex* index,
              size_t keySize,
              size_t valueSize,
              size_t counterSize) : CachePage(index, keySize, valueSize, counterSize) {
      totalCacheSize += totalSize;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief delete cube storage
    ////////////////////////////////////////////////////////////////////////////////

    ~CacheConsolidationsPage () {
      totalCacheSize -= totalSize;      
    }

    void shrink () {
      totalCacheSize -= totalSize;
      CachePage::shrink();
      totalCacheSize += totalSize;
    }
          
  protected:
    void grow () {
      totalCacheSize -= totalSize;
      CachePage::grow();
      totalCacheSize += totalSize;
    }
  
    
  private:
    static size_t totalCacheSize;  
    static size_t maximumCacheSize;
    
  };

}

#endif
