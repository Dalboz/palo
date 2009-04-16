////////////////////////////////////////////////////////////////////////////////
/// @brief palo cache consolidations storage
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

#include "Olap/CacheConsolidationsStorage.h"

namespace palo {
  
  set<CacheStorage*> CacheConsolidationsStorage::cacheSet;  
  
  CacheConsolidationsStorage::CacheConsolidationsStorage (const vector<size_t>* sizeDimensions,
                            size_t valueSize)
    : CacheStorage(sizeDimensions, valueSize)  {

    page = new CacheConsolidationsPage(index, keySize, valueSize, counterSize);
    registerCache(this);
  }



  CacheConsolidationsStorage::~CacheConsolidationsStorage () {
    removeCache(this);
  }


    
  void CacheConsolidationsStorage::clear () {
    CacheStorage::clear();
    // generate page
    page = new CacheConsolidationsPage(index, keySize, valueSize, counterSize);
  }
  

  set<CacheStorage*>* CacheConsolidationsStorage::getStorages () {
    return &cacheSet;
  }  


}
