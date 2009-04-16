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

#ifndef OLAP_CUBE_H
#define OLAP_CUBE_H 1

#include "palo.h"

#include <map>
#include <list>

#include "InputOutput/JournalFileWriter.h"
#include "InputOutput/Logger.h"

#include "Olap/CellPath.h"
#include "Olap/CubePage.h"
#include "Olap/CubeWorker.h"
#include "Olap/Dimension.h"
#include "Olap/Element.h"

namespace palo {
  class AreaStorage;
  class CacheConsolidationsStorage;
  class CacheRulesStorage;
  class CubeLooper;
  class CubeStorage;
  class Condition;
  class Database;
  class ExportStorage;
  class PaloSession;
  class Rule;
  class RuleMarker;
  class RuleNode;
  class Lock;
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP cube
  ///
  /// An OLAP cube stores the data
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS Cube {
    friend class CubeLooper;

  public:
    static double splashLimit1; // error
    static double splashLimit2; // warning
    static double splashLimit3; // info

	static int goalseekTimeoutMiliSec;
	static int goalseekCellLimit;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief status of the cube
    /// 
    /// UNLOADED: the cube was not loaded<br>
    /// LOADED:   the cube is loaded and not changed<br>
    /// CHANGED:  the cube is new or changed
    ////////////////////////////////////////////////////////////////////////////////
  
    enum CubeStatus {
      UNLOADED,
      LOADED,
      CHANGED
    };      

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief "splash mode" for setting numeric values in aggregations
    /// 
    /// DISABLED: do not set the value<br>
    /// DEFAULT:  <br>
    ///           1. value = 0.0 <br>
    ///              clears all base path cells<br>
    ///           2. value <> 0.0 and old_value = 0.0<br>
    ///              compute a "splash value" and distribute this value to all 
    ///              base path cells<br> 
    ///           3. value <> 0.0 and old_value <> 0.0<br>
    ///              compute a scale factor and recalculate all base cells<br> 
    /// SET_BASE: sets all base path elements to the same value<br> 
    /// ADD_BASE: adds value to all base path elements
    ////////////////////////////////////////////////////////////////////////////////
  
    enum SplashMode {
      DISABLED,
      DEFAULT,
      SET_BASE,
      ADD_BASE
    };      

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief a cell value
    ////////////////////////////////////////////////////////////////////////////////

    struct CellValueType {
      Element::ElementType type;

      double doubleValue;
      string charValue;
      IdentifierType rule;
    };

	typedef uint32_t CellLockInfo;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets flag for ignoring cell data (for debugging only)
    ////////////////////////////////////////////////////////////////////////////////

    static void setIgnoreCellData (bool ignore) {
      ignoreCellData = ignore;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates a new cube from type line
    ////////////////////////////////////////////////////////////////////////////////

    static Cube* loadCubeFromType (FileReader*,
                                   Database*,
                                   IdentifierType,
                                   const string& name, 
                                   vector<Dimension*>* dimensions,
                                   uint32_t type);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief set the maximum size of cache pages 
    ////////////////////////////////////////////////////////////////////////////////

    static void setMaximumCacheSize (size_t max);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief set the maximum size of rule cache pages 
    ////////////////////////////////////////////////////////////////////////////////

    static void setMaximumRuleCacheSize (size_t max);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief set the number of base elements for caching consolidated values
    ////////////////////////////////////////////////////////////////////////////////

    static void setCacheBarrier (double barrier);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief set the number cache invalidations that clears a cube cache
    ////////////////////////////////////////////////////////////////////////////////

    static void setCacheClearBarrier (uint32_t barrier);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief set the number cells that clears a cube cache
    ////////////////////////////////////////////////////////////////////////////////

    static void setCacheClearBarrierCells (double barrier);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief computes parameter for area mapping
    ////////////////////////////////////////////////////////////////////////////////

    static bool computeAreaParameters (Cube* cube,
                                       vector<IdentifiersType>* area,
                                       vector< map<Element*, uint32_t> >& hashMapping,
                                       vector<ElementsType>& numericArea,
                                       vector< vector< vector< pair<uint32_t, double> > > >& numericMapping,
                                       vector< map< uint32_t, vector< pair<IdentifierType, double> > > >& reverseNumericMapping,
                                       vector<uint32_t>& hashSteps,
                                       vector<uint32_t>& lengths,
                                       bool ignoreUnknownElements);

	////////////////////////////////////////////////////////////////////////////////
    /// @brief sets timeout for goalseek operations
    ////////////////////////////////////////////////////////////////////////////////
	static void setGoalseekTimeout(int milisecs);

	////////////////////////////////////////////////////////////////////////////////
    /// @brief sets cell limit for goalseek operations
    ////////////////////////////////////////////////////////////////////////////////
	static void setGoalseekCellLimit(int cellCount);
	
  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Creates empty cube
    ////////////////////////////////////////////////////////////////////////////////

    Cube (IdentifierType identifier,
          const string& name,
          Database* database,
          vector<Dimension*>* dimensions);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Destructor
    ////////////////////////////////////////////////////////////////////////////////

    virtual ~Cube ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name notification callbacks
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called after a cube has been added to a database
    ////////////////////////////////////////////////////////////////////////////////
      
    virtual void notifyAddCube () {
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called after a cube has been removed from a database
    ////////////////////////////////////////////////////////////////////////////////
      
    virtual void notifyRemoveCube () {
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief called after a cube has been renamed
    ////////////////////////////////////////////////////////////////////////////////
      
    virtual void notifyRenameCube (const string& oldName) {
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name functions to save and load the dimension
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns true if cube is loadable
    ////////////////////////////////////////////////////////////////////////////////

    bool isLoadable () const;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief changes cube path
    ////////////////////////////////////////////////////////////////////////////////

    void setCubeFile (const FileName&);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief reads data from file
    ////////////////////////////////////////////////////////////////////////////////

    virtual void loadCube (bool processJournal);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief reads journal data from file
    ////////////////////////////////////////////////////////////////////////////////

    void processCubeJournal (CubeStatus cubeStatus);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief replacement for JournalFileWriter::archiveJournalFiles
	/// @brief takes care about server progress
    ////////////////////////////////////////////////////////////////////////////////
	void archiveJournalFiles (const FileName& fileName);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief saves cube name and type to file  
    ////////////////////////////////////////////////////////////////////////////////

    virtual void saveCubeType (FileWriter* file) = 0;
      
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief saves data to file  
    ////////////////////////////////////////////////////////////////////////////////

    virtual void saveCube ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes cube file from disk  
    ////////////////////////////////////////////////////////////////////////////////

    void deleteCubeFiles ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief unloads saved cubes from memory  
    ////////////////////////////////////////////////////////////////////////////////

    void unloadCube ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name getter and setter
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns cube type
    ////////////////////////////////////////////////////////////////////////////////

    virtual ItemType getType() const = 0;
  
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets the token
    ////////////////////////////////////////////////////////////////////////////////

    uint32_t getToken () const;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets the client cache token
    ////////////////////////////////////////////////////////////////////////////////

    uint32_t getClientCacheToken ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets identifier of cube
    ////////////////////////////////////////////////////////////////////////////////

    IdentifierType getIdentifier () const {
      return identifier;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets cube name
    ////////////////////////////////////////////////////////////////////////////////

    void setName (const string& name) {
      this->name = name;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets cube name
    ////////////////////////////////////////////////////////////////////////////////

    const string& getName () const {
      return name;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets deletable attribute
    ////////////////////////////////////////////////////////////////////////////////

    void setDeletable (bool deletable) {
      this->deletable = deletable;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets deletable attribute
    ////////////////////////////////////////////////////////////////////////////////

    bool isDeletable () const {
      return deletable;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets renamable attribute
    ////////////////////////////////////////////////////////////////////////////////

    void setRenamable (bool renamable) {
      this->renamable = renamable;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets renamable attribute
    ////////////////////////////////////////////////////////////////////////////////

    bool isRenamable () const {
      return renamable;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets cube dimension list
    ////////////////////////////////////////////////////////////////////////////////

    const vector<Dimension*>* getDimensions () const {
      return &dimensions;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets rule list
    ////////////////////////////////////////////////////////////////////////////////

    vector<Rule*> getRules (User*);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets cube status
    ////////////////////////////////////////////////////////////////////////////////

    CubeStatus getStatus () const {
      return status;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets thedatabase
    ////////////////////////////////////////////////////////////////////////////////

    Database* getDatabase () const {
      return database;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the number of filled cells
    ////////////////////////////////////////////////////////////////////////////////

    int32_t sizeFilledCells ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name functions to update internal structures
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates a new rule
    ////////////////////////////////////////////////////////////////////////////////

    Rule* createRule (RuleNode*, const string& external, const string& comment, bool activate, User*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief modifies an existing rule
    ////////////////////////////////////////////////////////////////////////////////

    void modifyRule (Rule*, RuleNode*, const string& external, const string& comment, User*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief activeate/deactivate an existing rule
    ////////////////////////////////////////////////////////////////////////////////

    void activateRule (Rule*, bool activate, User*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes a rule
    ////////////////////////////////////////////////////////////////////////////////

    void deleteRule (IdentifierType, User*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief finds a rule
    ////////////////////////////////////////////////////////////////////////////////

    Rule* findRule (IdentifierType, User*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief evaluates a rule
    ////////////////////////////////////////////////////////////////////////////////

    double computeRule (CubePage::element_t, double dflt, User*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief increments the token
    ////////////////////////////////////////////////////////////////////////////////

    void updateToken ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief increments the client cache token
    ////////////////////////////////////////////////////////////////////////////////

    void updateClientCacheToken ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes element
    ////////////////////////////////////////////////////////////////////////////////

    virtual void deleteElement (const string& username,
                                const string& event,
                                Dimension* dimension,
                                IdentifierType element, 
                                bool processStorageDouble,
                                bool processStorageString,
                                bool deleteRules);

	virtual void deleteElements (const string& username,
		const string& event,
		Dimension* dimension,
		IdentifiersType elements,
		bool processStorageDouble,
		bool processStorageString,
		bool deleteRules);
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief opens journal
    ////////////////////////////////////////////////////////////////////////////////

    void openJournal () {
      closeJournal();
      journal = new JournalFileWriter(FileName(*fileName, "log"), false);
      journal->openFile();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief closes journal
    ////////////////////////////////////////////////////////////////////////////////

    void closeJournal () {
      if (journal != 0) {
        delete journal;
        journal = 0;
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief set the worker area of the cube
    ////////////////////////////////////////////////////////////////////////////////

    void setWorkerAreas (vector<string>* areaIdentifiers, vector< vector< IdentifiersType > >* areas);
    
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief remove unused worker
    ////////////////////////////////////////////////////////////////////////////////

    void removeWorker ();
    
    
    Lock* lockCube (vector<IdentifiersType>* area, const string& areaString, User* user);
    
    void commitCube (long int id, User* user);
    
    void rollbackCube (long int id, User* user, size_t numSteps);

	const vector<Lock*>& getCubeLocks (User* user);

	bool hasLockedArea () {
		return hasLock;
	}


	////////////////////////////////////////////////////////////////////////////////
	/// @}
	////////////////////////////////////////////////////////////////////////////////

public:

	////////////////////////////////////////////////////////////////////////////////
	/// @{
	/// @name functions dealing with cells and cell values
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	/// @brief clears all cells
	////////////////////////////////////////////////////////////////////////////////

	virtual void clearCells (User* user);

    virtual void clearCells (vector<IdentifiersType> * baseElements, User * user);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief sets NUMERIC value to a cell
	////////////////////////////////////////////////////////////////////////////////

	virtual semaphore_t setCellValue (CellPath* cellPath,
		double value,
		User* user,
		PaloSession * session,
		bool checkArea,
		bool sepRight,
		bool addValue,
		SplashMode splashMode, // = DEFAULT,
		Lock * lock,
		bool ignoreJournal = false);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief sets STRING value to a cell
	////////////////////////////////////////////////////////////////////////////////

	virtual semaphore_t setCellValue (CellPath* cellPath,
		const string& value, 
		User* user, 
		PaloSession * session,
		bool checkArea,
		bool sepRight,
		Lock * lock);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief clears value of cell in cube
	////////////////////////////////////////////////////////////////////////////////

	virtual semaphore_t clearCellValue (CellPath* cellPath,
		User* user,
		PaloSession * session,
		bool checkArea,
		bool sepRight,
		Lock* lock,
		bool ignoreJournal = false);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief gets a value from the cube
	///
	/// elementType contains the datatype of the returned value,
	/// found contains true if at least one entry exists
	////////////////////////////////////////////////////////////////////////////////

	CellValueType getCellValue (CellPath* cellPath,
		bool* found, 
		User* user,
		PaloSession * session,
		set< pair<Rule*, IdentifiersType> >* ruleHistory,
		bool useEnterpriseRules = true,
		bool* isCachable = 0);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief copies a cell value (or cell values) to an other cell 
	////////////////////////////////////////////////////////////////////////////////

	virtual void copyCellValues (CellPath* cellPathFrom,
		CellPath* cellPathTo,
		User* user,
		PaloSession * session,
		double factor = 1.0);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief copies a cell value (or cell values) to an other cell 
	////////////////////////////////////////////////////////////////////////////////

	virtual void copyLikeCellValues (CellPath* cellPathFrom,
		CellPath* cellPathTo,
		User* user,
		PaloSession * session,
		double value);


	////////////////////////////////////////////////////////////////////////////////
	/// @brief gets values from the cube
	///
	/// the storage returns the values
	/// area is a cube area given by the request
	/// cellPathes returns the cell pathes computed by the getAreaCellValues method
	/// return true if the storage is filled
	////////////////////////////////////////////////////////////////////////////////

	bool getAreaCellValues (AreaStorage* storage, 
		vector<IdentifiersType>* area,  
		vector<IdentifiersType>* resultPathes,
		User* user); 

	double getNumAreaBaseCells (vector<IdentifiersType>* area);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief gets values from the cube
	///
	/// the storage returns the values
	/// cellPathes is the list of cell pathes given by the request
	////////////////////////////////////////////////////////////////////////////////

	void getListCellValues (AreaStorage* storage, 
		vector<IdentifiersType>* cellPathes,
		User* user); 


	////////////////////////////////////////////////////////////////////////////////
	/// @brief gets values from the cube
	///
	/// the storage returns the values
	/// area is a cube area given by the request
	/// cellPathes returns the cell pathes computed by the getAreaCellValues method
	////////////////////////////////////////////////////////////////////////////////

	void getExportValues (ExportStorage* storage, 
		vector<IdentifiersType>* area,  
		vector<IdentifiersType>* resultPathes,
		IdentifiersType* startPath,
		bool hasStartPath,
		bool useRules,
		bool baseElementsOnly,
		bool skipEmpty,
		Condition * condition,
		User* user); 


	////////////////////////////////////////////////////////////////////////////////
	/// @brief gets values from the cube
	///
	/// cell goal seek
	/// 
	///
	////////////////////////////////////////////////////////////////////////////////

	void cellGoalSeek(CellPath* cellPath,
		User* user,
		PaloSession * session,
		const double& value		
	); 


	////////////////////////////////////////////////////////////////////////////////
	/// @}
	////////////////////////////////////////////////////////////////////////////////

public:
	void sendExecuteShutdown ();

	static bool isInArea (CellPath* cellPath, vector< set<IdentifierType> >* area);      

	static bool isInArea (const IdentifierType*, const vector< set<IdentifierType> >* area);      

	void clearRulesCache ();      

	void clearAllCaches ();

	CellValueType getConsolidatedRuleValue (const CellPath*, bool* found, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory);

	CubeStorage* getStorageDouble () {
		return storageDouble;
	}

	void removeFromMarker (RuleMarker*);

	void removeToMarker (RuleMarker*);

	void addFromMarker (RuleMarker*);

	void addToMarker (RuleMarker*);

	void checkFromMarkers (CubePage::key_t key);

	void clearAllMarkers ();

	void rebuildAllMarkers ();

	const set<RuleMarker*>& getFromMarkers () const {
		return fromMarkers;
	}

	void checkNewMarkerRules ();

protected:
	void setBaseCellValue (const IdentifiersType * path, const string& value);

	void loadCubeOverview (FileReader*);

	void loadCubeCells (FileReader*);

	bool loadCubeJournal ();

	void loadCubeRuleInfo (FileReader*);

	void loadCubeRule (FileReader*, int version);

	void loadCubeRules ();

	void saveCubeOverview (FileWriter*);

	void saveCubeDimensions (FileWriter*);

	void saveCubeCells (FileWriter* file);

	void saveCubeRule (FileWriter* file, Rule* rule);

	void saveCubeRules ();

	virtual void checkPathAccessRight (User* user, const CellPath* cellPath, RightsType minimumRight);

	virtual RightsType getMinimumAccessRight (User* user);

	void checkSepRight (User* user, RightsType minimumRight);

protected:

	virtual void checkCubeAccessRight(User* user, RightsType minimumRight) {
		if (user != 0 && user->getRoleCubeRight() < minimumRight) {
			throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
				"insufficient access rights for cube",
				"user", (int) user->getIdentifier());
		}
	}

	virtual void checkCubeRuleRight(User* user, RightsType minimumRight) {
		// check cube data right
		if (user != 0 && user->getCubeDataRight(database, this) < RIGHT_READ) {
			throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
				"insufficient access rights for cube",
				"user", (int) user->getIdentifier());
		}

		if (user != 0 && user->getRoleRuleRight() < minimumRight) {
			throw ParameterException(ErrorException::ERROR_NOT_AUTHORIZED,
				"insufficient access rights for cube rules",
				"user", (int) user->getIdentifier());
		}
	}

private:
	bool isInArea (CellPath* cellPath, string& areaIdentifier);      

	void setCellValueConsolidated (CellPath* cellPath, double value, SplashMode splashMode, User* user, Lock* lock);

	// does not count elements with weight 0.0
	double countBaseElements (const CellPath * path);

	double computeBaseElements (const CellPath * path,
		vector<IdentifiersWeightType> * baseElements, 
		bool ignoreNullWeight);

	double computeBaseElements (const CellPath * path, vector< set<IdentifierType> > * baseElements);

	void deleteNumericBasePath (const vector<ElementsWeightType> * baseElements);

	void deleteNumericBasePathRecursive (size_t position,
		vector<ElementsWeightType>::const_iterator baseElementsIter,
		PathType * newPath);

	void setBaseElementsRecursive (vector<IdentifiersWeightType> *baseElements,
		double value,
		SplashMode splashMode,
		CellPath* cellPath,
		User* user,
		Lock* lock);

	void setBaseElementsRecursive (size_t position,
		vector<IdentifiersWeightType> *baseElements,
		IdentifiersType *path,
		double value,
		SplashMode splashMode,
		size_t * count,
		Lock* lock);

	void setBaseCellValue (const IdentifiersType * path, double value);

	void setBaseCellValue (const PathType * path, double value);

	void setBaseCellValue (const PathWeightType * path, double value);

	void setBaseCellValue (const PathType * path, const string& value);

	void setBaseCellValue (const PathWeightType * path, const string& value);

	void reorganizeCubeStorage (CubeStorage** storage);

	void makeOrderedElements (ElementsWeightType*);

	bool isEqualBaseElements (vector<ElementsWeightType>*, vector<ElementsWeightType>*);

	struct CopyElementInfo {		
		CopyElementInfo(Element* e, bool check) : element(e), zero_check(check) {};
		Element* element;
		bool zero_check;
		std::vector<CopyElementInfo> children;		
	};

	bool computeCompatibleElements (CellPath* cellPathFrom,
		vector< vector<Element*> >* baseElementsFrom,
		CellPath* cellPathTo,
		vector< vector<Element*> >* baseElementsTo);
	

	bool buildTreeWithoutMultipleParents(Dimension* dimension, Element* el, map<Element*, ElementsWeightType>* tree, bool* multiParent);

	bool computeCompatibleElements_m2 (CellPath* cellPathFrom,
		vector< vector<Element*> >* baseElementsFrom,		
		CellPath* cellPathTo,
		vector< vector<Element*> >* baseElementsTo		
		);
	
	bool computeCompatibleElements_m3 (CellPath* cellPathFrom,
		vector< vector<CopyElementInfo> >& baseElementsFrom,
		CellPath* cellPathTo,
		vector< vector<CopyElementInfo> >& baseElementsTo);

	bool computeCompatibleElements (Dimension* dimension,
		Element* from, 
		vector<Element*>* baseElementsFrom,
		Element* to,
		vector<Element*>* baseElementsTo);

	bool computeCompatibleElements_m2(Dimension* dimension,
		Element* from, const map<Element*, ElementsWeightType>* fromElementsTree, vector<Element*>* fromElements,
		Element* to, const map<Element*, ElementsWeightType>* toElementsTree, vector<Element*>* toElements);

	bool computeCompatibleElements_m3(Dimension* dimension,
		Element* from, 
		set<Element*>& accessedFromElements,
		vector<CopyElementInfo>& baseElementsFrom,
		Element* to,
		set<Element*>& accessedToElements,
		vector<CopyElementInfo>& baseElementsTo);

	bool computeSplashElements_m3 (CellPath* cellPathFrom,
		vector< vector<CopyElementInfo> >& baseElementsFrom);

	bool computeSplashElements_m3(Dimension* dimension,
		Element* from, 
		set<Element*>& accessedFromElements,
		vector<CopyElementInfo>& baseElementsFrom);

	void copyCellValues (vector< vector<Element*> >*,
		vector< vector<Element*> >*,
		User* user,
		PaloSession* session,
		double,
		Lock* lock);

	void copyCellValues (vector< vector<CopyElementInfo> >* elementsWeigthFrom,
		vector< vector<CopyElementInfo> >* elementsWeigthTo, 
		User* user,
		PaloSession* session,
		double factor,
		Lock* lock);

	void copyCellValuesRecursive (vector< vector<Element*> >::iterator fromIterator,
		vector< vector<Element*> >::iterator endIterator,
		vector< vector<Element*> >::iterator toIterator,
		vector< Element* >* fromElements,
		vector< Element* >* toElements,
		int pos,
		User* user,
		PaloSession* session,
		double factor,
		Lock * lock);	

	void copyCellValuesRecursive_m3(vector< CopyElementInfo* >& from, vector< CopyElementInfo* >& to,
		vector< Element* >& fromElements,
		vector< Element* >& toElements,
		int lastDiggDimIndex,		
		int zeroCheckCount,
		User* user,
		PaloSession* session,
		double factor,
		size_t* count,
		bool* addCubeToChangedMarkers,
		Lock * lock);

	void scaleCellValuesRecursive_m3(vector< CopyElementInfo* >& from, 
		vector< Element* >& fromElements,		
		int lastDiggDimIndex,		
		int zeroCheckCount,
		User* user,
		PaloSession* session,
		double factor,
		size_t* count,
		bool* addCubeToChangedMarkers,
		Lock * lock);


	void getParentElements (Dimension* dimension, Element* child, set<IdentifierType>* parents);
	// check cache size and cacheClearBarrier
	bool isInvalid ();
	// check cacheClearBarrierCells (used in the functions below)
	void invalidateCache (CellPath* cellPath, vector< set<IdentifierType> >* deleteArea);    
	// update cache functions 
	void removeCachedCells (CellPath* cellPath);
	void removeCachedCells (CellPath* cellPath, vector< set<IdentifierType> >* baseElements);
	void removeCachedCells (CellPath* cellPath, vector<IdentifiersWeightType>* baseElements);
	void removeCachedCells (CellPath* cellPath, vector<ElementsWeightType>* baseElements);
	void removeCachedCells (Dimension* dimension, IdentifierType element);

	double computeAreaBaseElements (vector<IdentifiersType>* paths, 
		vector< map<IdentifierType, map<IdentifierType, double> > > *baseElements,
		bool ignoreWrongIdentifiers);
	void computeAreaBaseElementsRecursive (Dimension * dimension,
		IdentifierType elementId,
		Element * element,
		double weight,
		map<IdentifierType, map<IdentifierType, double> > * mapping);         

	bool computeBaseElementsOnly (vector<IdentifiersType>* area,
		vector<IdentifiersType>* resultArea,
		vector< map<IdentifierType, map<IdentifierType, double> > > *baseElements,
		bool ignoreWrongIdentifiers);

	////////////////////////////////////////////////////////////////////////////////
	/// @brief gets cell/area and cell/values values from the cube
	///
	/// the storage contains the values
	/// baseElements contains the mapping of base elements to requested elements
	/// cellPathes is the list of cell pathes
	/// buildCellPathes should be true for (cell/area) 
	/// updateConsolidationsStorage should be true
	////////////////////////////////////////////////////////////////////////////////

	void getCellValues (AreaStorage* storage, 
		vector<IdentifiersType>* area,
		vector<IdentifiersType>* cellPathes,
		bool buildCellPathes,
		User* user); 

	////////////////////////////////////////////////////////////////////////////////
	/// @brief gets cell/area and cell/values values from the cache
	///
	/// the storage contains the values
	/// cellPathes is the list of cell pathes
	/// buildCellPathes should be true for (cell/area) 
	////////////////////////////////////////////////////////////////////////////////

	void getCellValuesFromCache (AreaStorage* storage, 
		vector<IdentifiersType>* cellPathes,
		bool buildCellPathes,
		User* user); 


	// fill consolidations cache by a list of cell paths
	// return true for success     
	bool fillCacheByHistory (PaloSession* session);
	void addSiblingsAndParents(set<IdentifierType>* identifiers, Dimension* dimension, IdentifierType id);
	void addElementAndParents(set<IdentifierType>* identifiers, Dimension* dimension, Element* child);

	void checkExportPathAccessRight (ExportStorage* storage, User * user);
	void getExportRuleValues (ExportStorage* storage, vector<IdentifiersType>* area, User * user);

	Lock* lookupLockedArea (CellPath* cellPath, User* user);

	void setCellMarker (const uint32_t*);

	Lock* findCellLock(CellPath* cellPath);
public:
	CellLockInfo getCellLockInfo(CellPath* cellPath, IdentifierType userId);	

protected:
	uint32_t token;            // token for changes
	uint32_t clientCacheToken; // token for client cache changes
	IdentifierType identifier; // identifier of the cube
	string name;               // name of the cube

	vector<Dimension*> dimensions; // list of dimensions used for the cube
	vector<size_t> sizeDimensions; // list of dimensions sizes

	vector<Rule*> rules; // list of rules
	IdentifierType maxRuleIdentifier;

	Database * database; // database of cube

	bool deletable;
	bool renamable;

	CubeWorker* cubeWorker;
	bool hasArea;
	vector< string > workerAreaIdentifiers;
	vector< vector< set<IdentifierType> > > workerAreas;

	CubeStorage* storageDouble; // cell storage for NUMERIC values
	CubeStorage* storageString; // cell storage for STRING values
	CacheConsolidationsStorage* cacheConsolidationsStorage; // cell storage for consolidated values
	CacheRulesStorage* cacheRulesStorage; // cell storage for rule values

	CubeStatus status; // the status of the cell

	FileName * fileName; // file name of the cube
	FileName * ruleFileName; // file of the rules

	JournalFileWriter * journal; // journal writer of the cube

	uint32_t invalidateCacheCounter;

	static double   cacheBarrier;
	static uint32_t cacheClearBarrier;
	static double   cacheClearBarrierCells;


	set< vector<IdentifiersType> > cachedAreas;
	set< vector<IdentifiersType> > cachedCellValues;    

	static bool ignoreCellData;

	bool hasLock;
	vector<Lock*> locks;
	uint32_t maxLockId;

	set< RuleMarker* > fromMarkers;
	set< RuleMarker* > toMarkers;
	set< Rule* > newMarkerRules;
};

}

#endif 
