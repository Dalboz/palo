////////////////////////////////////////////////////////////////////////////////
/// @brief palo area storage
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

#ifndef OLAP_AREA_STORAGE_H
#define OLAP_AREA_STORAGE_H 1

#include "palo.h"

#include <set>

#include "Exceptions/ErrorException.h"

#include "InputOutput/Logger.h"

#include "Olap/CellPath.h"
#include "Olap/Cube.h"
#include "Olap/CubeIndex.h"
#include "Olap/AreaPage.h"
#include "Olap/Element.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo area storage
  ///
  /// A cache storage uses a cache page with a fixed memory size to store the 
  /// values. 
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS AreaStorage {

  public:
    static const size_t MAXIMUM_AREA_SIZE = 10000000;  

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Creates a filled cache storage
    ////////////////////////////////////////////////////////////////////////////////

    AreaStorage (const vector<Dimension*>* dimensions,
                 vector<IdentifiersType>* area, 
                 size_t numElements,
                 bool isPathList);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Remove a cache storage
    ////////////////////////////////////////////////////////////////////////////////

    ~AreaStorage ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a pointer to a cell value
    ///
    /// The method returns "0" for an undefined value   
    ////////////////////////////////////////////////////////////////////////////////

    template <typename PATH>
    AreaPage::element_t getCellValue (const PATH path) {
      fillKeyBuffer(tmpKeyBuffer, path);

      return index->lookupKey(tmpKeyBuffer);
    }
    
    void addDoubleValue(IdentifiersType* path, double value); 

    void addDoubleValue(uint8_t * keyBuffer, double value);

  public:
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the maximum number of each dimension  
    ////////////////////////////////////////////////////////////////////////////////
    
    const vector<size_t> * getSizeDimensions () const {
      return &maxima;
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief number of used rows  
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t size () const {
      return numberElements;
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size of a value in byte  
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t getValueSize () const {
      return valueSize;
    };
        
  public:
    void print ();
    
    Cube::CellValueType* getCell (size_t pos);     
    Cube::CellValueType* getCell (size_t pos, IdentifiersType* path);     
    
  private:
  
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief add unknown value for path
    ////////////////////////////////////////////////////////////////////////////////

    void addCell (IdentifiersType* path);
    void addErrorCell ();

    void addAreaCells (vector<IdentifiersType>* area);
    
    void loop (const vector< IdentifiersType >* paths, vector<size_t>& combinations);

    void addPathCells (vector<IdentifiersType>* paths);


    ////////////////////////////////////////////////////////////////////////////////
    /// @brief fills path
    ////////////////////////////////////////////////////////////////////////////////
    
    void fillPath (const AreaPage::element_t row, IdentifiersType* path) {
      const IdentifierType * buffer = (const IdentifierType*) (row + valueSize);
      IdentifiersType::iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++, buffer++) {
        *pathIter = *buffer;
      }
    }

    void fillKeyBuffer (AreaPage::key_t keyBuffer, const IdentifiersType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      IdentifiersType::const_iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter);
      }
    }



    void fillKeyBuffer (AreaPage::key_t keyBuffer, const PathType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      PathType::const_iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter)->getIdentifier();
      }
    }



    void fillKeyBuffer (AreaPage::key_t keyBuffer, const PathWeightType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      PathType::const_iterator pathIter = path->first.begin();

      for (;  pathIter != path->first.end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter)->getIdentifier();
      }
    }



    void fillKeyBuffer (AreaPage::key_t keyBuffer, AreaPage::key_t path) {
      memcpy(keyBuffer, path, keySize);
    }



    void fillKeyBuffer (AreaPage::key_t keyBuffer, const CellPath * path) {
      fillKeyBuffer(keyBuffer, path->getPathIdentifier());
    }

    
  private:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief total number of elements
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t numberElements;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief total number of elements
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t maximumNumberElements;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief maximum possible value for dimension identifier
    ////////////////////////////////////////////////////////////////////////////////
    
    vector<size_t> maxima;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief the used page
    ////////////////////////////////////////////////////////////////////////////////
    
    AreaPage * page;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store a path 
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t keySize;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store a value
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t valueSize;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store the path and the value
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t elementSize;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Pointer to a cube index object
    ////////////////////////////////////////////////////////////////////////////////
    
    CubeIndex * index;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief temporary buffer
    ////////////////////////////////////////////////////////////////////////////////
    
    uint8_t * tmpKeyBuffer;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief temporary buffer
    ////////////////////////////////////////////////////////////////////////////////
    
    uint8_t * tmpElementBuffer;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief list of dummy values for paths with errors 
    ////////////////////////////////////////////////////////////////////////////////
    
    vector<Cube::CellValueType*> errorValues;
  };

}

#endif 
