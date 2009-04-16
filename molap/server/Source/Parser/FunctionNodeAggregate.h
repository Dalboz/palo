////////////////////////////////////////////////////////////////////////////////
/// @brief
///
/// @file
///
/// Developed by Marko Stijak, Banja Luka on behalf of Jedox AG.
/// Copyright and exclusive worldwide exploitation right has
/// Jedox AG, Freiburg.
///
/// @author Marko Stijak, Banja Luka, Bosnia and Herzegovina
////////////////////////////////////////////////////////////////////////////////


#ifndef PARSER_FUNCTION_NODE_AGGREGATE_H
#define PARSER_FUNCTION_NODE_AGGREGATE_H 1

#include "palo.h"

#include <string>
#include "Parser/SourceNode.h"
#include "Olap/AreaStorage.h"

namespace palo {

	////////////////////////////////////////////////////////////////////////////////
	/// @brief parser function node str
	////////////////////////////////////////////////////////////////////////////////

	class SERVER_CLASS FunctionNodeAggregate : public FunctionNode 
	{
		enum AggregateFunction { Sum, Product, Min, Max, Count, First, Last, Average, And, Or, Unknown } aggregateFunction;

	public:
		static FunctionNode* createNode (const string& name, vector<Node*> *params) {
			return new FunctionNodeAggregate(name, params);
		}

	public:
		FunctionNodeAggregate () : FunctionNode() {};
		FunctionNodeAggregate (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
			if (name=="sum")
				aggregateFunction = Sum;
			else if (name=="product")
				aggregateFunction = Product;
			else if (name=="min")
				aggregateFunction = Min;
			else if (name=="max")
				aggregateFunction = Max;
			else if (name=="count")
				aggregateFunction = Count;
			else if (name=="first")
				aggregateFunction = First;
			else if (name=="last")
				aggregateFunction = Last;
			else if (name=="average")
				aggregateFunction = Average;
			else if (name=="and")
				aggregateFunction = And;
			else if (name=="or")
				aggregateFunction = Or;
			else 
				aggregateFunction = Unknown;
		};


	public:

		ValueType getValueType () {
			return Node::NUMERIC;
		}      

		Node * clone () {
			FunctionNodeAggregate * cloned = new FunctionNodeAggregate(name, cloneParameters());
			cloned->valid = this->valid;
			return cloned;
		}

		bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) 
		{

			// add has two parameters
			if (!params || params->size()==0) {
				error = "function '" + name + "' needs at least one parameter";
				return valid = false;
			}	

			p.resize(params->size());
			for (size_t i = 0; i<params->size(); i++) {
				Node* p1 = params->at(i);        

				// validate parameters
				if (!p1->validate(server, database, cube, destination, error)) {
					return valid = false;         
				}

				if (p1->getValueType() != Node::NUMERIC && 
					p1->getValueType() != Node::UNKNOWN) {
						error = "first parameter of function '" + name + "' has wrong data type";        
						return valid = false;
				}

				p[i] = p1;
			}
			return valid = true;  
		}

		RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {        

			RuleValueType result;
			result.type = Node::NUMERIC; 

			double acc = 0;
			double count = 0;			

			if (valid) {
				for (size_t pi = 0; pi<p.size(); pi++) {
					Node* p1 = p[pi];

					//disabled until Jedox comes with specification
					//if (p1->getNodeType()== Node::NODE_SOURCE_NODE) {
					//	Cube* cube = const_cast<Cube*>(cellPath->getCube());
					//	SourceNode* sourceNode = dynamic_cast<SourceNode*>(p1);						
					//	AreaNode::Area * sourceArea = sourceNode->getSourceArea();
					//	std::vector<IdentifiersType> area(sourceArea->size());
					//	bool contains = true;
					//	for (size_t d = 0; d<sourceArea->size(); d++) {
					//		for (set<IdentifierType>::iterator it = (*sourceArea)[d].begin(); it!=(*sourceArea)[d].end(); it++)
					//			area[d].push_back(*it);
					//		contains &= (*sourceArea)[d].find(cellPath->getPathIdentifier()->at(d))!=(*sourceArea)[d].end();
					//	}
					//	if (contains) //source area contains cellPath = infinite recursion....
					//		continue;

					//	int numResult = 1;
					//	for (AreaNode::Area::iterator it = sourceArea->begin(); it!=sourceArea->end(); it++)
					//		numResult*=(int)it->size();
					//	AreaStorage storage(cube->getDimensions(), &area, numResult, false);

					//	vector<IdentifiersType> resultPaths;
					//	bool storageFilled = cube->getAreaCellValues(&storage, &area, &resultPaths, user);

					//	if (storageFilled) {    
					//		size_t last = storage.size();
					//		vector<IdentifiersType>::iterator iter = resultPaths.begin();

					//		for (size_t i = 0; i < last; i++, iter++) {
					//			Cube::CellValueType* value = storage.getCell(i); 							
					//			aggregateValue(value->doubleValue, acc, count);
					//		} 
					//		
					//	}
					//} else { block below }
					RuleValueType v1  = p1->getValue(cellPath,user,isCachable, ruleHistory); 
					if (v1.type == Node::NUMERIC) {
						aggregateValue(v1.doubleValue, acc, count);
					}					
				}
			}

			result.doubleValue = getResult(acc, count);
			return result;
		}
	protected:
		std::vector<Node*> p;

		void aggregateValue(const double& value, double& acc, double& count) {
			count+=1;
			switch (aggregateFunction) {
				case Sum:
				case Average:
					acc += value;
					break;
				case Product:
					acc *= value;
					break;
				case Min:
					if (count==1 || value<acc)
						acc = value;
					break;
				case Max:
					if (count==1 || value>acc)
						acc = value;
					break;
				case First:
					if (count==1)
						acc = value;
					break;
				case Last:
					acc = value;
					break;
				case Count:
					break;
				case And: //inverse logic
					if (value==0)
						acc = 1.0;
					break;
				case Or:
					if (value!=0)
						acc = 1.0;
					break;
				case Unknown:
					acc = 0;
					break;
			}
		}

		double getResult(double& acc, double& count) {			
			switch (aggregateFunction) {				
				case Average:
					return count>0 ? acc/count : 0;
				case Count:
					return count;
				case And: //inverse logic
					return 1.0-acc;
			}
			return acc;
		}
	};

}
#endif
