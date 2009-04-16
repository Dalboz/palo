////////////////////////////////////////////////////////////////////////////////
/// @brief palo enterprise rule
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

#ifndef OLAP_RULE_H
#define OLAP_RULE_H 1

#include "palo.h"

#include "Olap/Cube.h"
#include "Olap/RuleMarker.h"

#include "Parser/RuleNode.h"
#include "Parser/RuleOptimizer.h"

namespace palo {
  class Dimension;
  class Element;
  
  ////////////////////////////////////////////////////////////////////////////////
  /// @brief OLAP enterprise rule
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS Rule { 
  public:
    static const IdentifierType NO_RULE = ~0;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates new rule from rule node
    ////////////////////////////////////////////////////////////////////////////////

    Rule (IdentifierType identifier,
          Cube* cube,
          RuleNode * ruleNode,
          const string& external,
          const string& comment,
          bool activeRule)
      : cube(cube),
        identifier(identifier),
        rule(ruleNode),
        comment(comment),
        external(external),
        activeRule(activeRule),
        isOptimized(false),
        restrictedRule(0),
        restrictedDimension(0),
        ruleOptimizer(cube),
        markersComputed(false) {
      ::time(&timestamp);
      cubeToken = cube->getToken() + 1;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief creates new rule from rule node
    ////////////////////////////////////////////////////////////////////////////////

    Rule (IdentifierType identifier,
          Cube* cube,
          RuleNode * ruleNode,
          const string& external,
          const string& comment,
          time_t timestamp,
          bool activeRule) 
      : cube(cube),
        identifier(identifier),
        rule(ruleNode),
        comment(comment),
        external(external),
        timestamp(timestamp),
        activeRule(activeRule),
        isOptimized(false), 
        restrictedRule(0),
        restrictedDimension(0),
        ruleOptimizer(cube),
        markersComputed(false) {
      cubeToken = cube->getToken() + 1;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Destructor
    ////////////////////////////////////////////////////////////////////////////////

    virtual ~Rule ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @{
    /// @name getter and setter
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets cube of rule
    ////////////////////////////////////////////////////////////////////////////////

    Cube* getCube () const {
      return cube;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets identifier of rule
    ////////////////////////////////////////////////////////////////////////////////

    IdentifierType getIdentifier () const {
      return identifier;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets external identifier of rule
    ////////////////////////////////////////////////////////////////////////////////

    const string& getExternal () const {
      return external;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets external identifier of rule
    ////////////////////////////////////////////////////////////////////////////////

    void setExternal (const string& external) {
      this->external = external;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets comment for a rule
    ////////////////////////////////////////////////////////////////////////////////

    const string& getComment () const {
      return comment;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets comment for a rule
    ////////////////////////////////////////////////////////////////////////////////

    void setComment (const string& comment) {
      this->comment = comment;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief gets timestamp for a rule
    ////////////////////////////////////////////////////////////////////////////////

    time_t getTimeStamp () const {
      return timestamp;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief sets a new definition
    ////////////////////////////////////////////////////////////////////////////////

    void setDefinition (RuleNode* node) {
      delete rule;
      rule = node;

      ::time(&timestamp);

      // force computation of contains are and optimization
      cubeToken = cube->getToken() + 1;
      checkCubeToken();
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief return true if the rule is active
    ////////////////////////////////////////////////////////////////////////////////

    bool isActive () const {
      return activeRule;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief activate or deactivate rule
    ////////////////////////////////////////////////////////////////////////////////

    void setActive (bool isActive) {
      activeRule = isActive;
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the rule option
    ////////////////////////////////////////////////////////////////////////////////

    RuleNode::RuleOption getRuleOption () {
      return rule->getRuleOption();
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief linear rule
    ////////////////////////////////////////////////////////////////////////////////

    bool isLinearRule ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief restricted rule
    ////////////////////////////////////////////////////////////////////////////////

    bool isRestrictedRule ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @}
    ////////////////////////////////////////////////////////////////////////////////

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if cell path is within destination area
    ////////////////////////////////////////////////////////////////////////////////

    bool withinArea (CellPath* cellPath);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if cell path is within restricted area of optimized rule
    ////////////////////////////////////////////////////////////////////////////////

    bool withinRestricted (CellPath* cellPath);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if cell path is within destination area and of correct type
    ////////////////////////////////////////////////////////////////////////////////

    bool within (CellPath* cellPath);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if cell path is touched by destination area
    ///
    /// Returns true if the destination area is within a the sub-cube identified
    /// by a consolidated path.
    ////////////////////////////////////////////////////////////////////////////////

    bool contains (CellPath* cellPath);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if cell path is touched by restricted area of optimized rule
    ////////////////////////////////////////////////////////////////////////////////

    bool containsRestricted (CellPath* cellPath);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns value of computation
    ////////////////////////////////////////////////////////////////////////////////

    Cube::CellValueType getValue( CellPath* cellPath,
		                              User* user,
		                              bool* skipRule,
		                              bool* skipAllRules,
		                              bool* usesDifferentDatabase,
		                              set< pair<Rule*, IdentifiersType> > * ruleHistory ) const;


    ////////////////////////////////////////////////////////////////////////////////
    /// @brief appends a representation to the string buffer
    ////////////////////////////////////////////////////////////////////////////////

    void appendRepresentation (StringBuffer* sb, bool names = false) const {
      return rule->appendRepresentation(sb, names ? cube : 0);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if elements is contained in rule
    ////////////////////////////////////////////////////////////////////////////////

    bool hasElement (Dimension* dimension, IdentifierType element) const {
      return rule->hasElement(dimension, element);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief checks if rule has markers
    ////////////////////////////////////////////////////////////////////////////////

    bool hasMarkers () const {
      if (markersComputed) {
        return ! markers.empty();
      }
      else {
        return !(rule->getExternalMarkers().empty() && rule->getInternalMarkers().empty());
      }
    }
    
    void computeMarkers ();

    void removeMarkers ();

  private:
    void checkCubeToken ();

    void computeContains ();

    void computeContainsRestricted ();

    void optimizeRule ();

  private:
    Cube* cube;
    IdentifierType identifier;
    RuleNode * rule;
    string comment;
    string external;
    time_t timestamp;
    uint32_t cubeToken;
    bool activeRule;
    bool markersComputed;

    bool isOptimized;
    ExprNode* restrictedRule;
    Dimension* restrictedDimension;
    set<IdentifierType> restrictedIdentifiers;
    RuleOptimizer ruleOptimizer;
    bool linearRule;

    AreaNode::Area restrictedArea;
    AreaNode::Area containsArea;
    AreaNode::Area containsRestrictedArea;

    vector<RuleMarker*> markers;
  };
}

#endif
