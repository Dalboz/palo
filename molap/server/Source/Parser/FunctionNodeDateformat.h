////////////////////////////////////////////////////////////////////////////////
/// @brief function node Dateformat
///
/// @file
///
/// Developed by Marko Stijak, Banja Luka on behalf of Jedox AG.
/// Copyright and exclusive worldwide exploitation right has
/// Jedox AG, Freiburg.
///
/// @author Marko Stijak, Banja Luka, Bosnia and Herzegovina
////////////////////////////////////////////////////////////////////////////////

#ifndef PARSER_FUNCTION_NODE_DATEFORMAT_H
#define PARSER_FUNCTION_NODE_DATEFORMAT_H 1

#include "palo.h"

#include "math.h"
#include "Collections/StringUtils.h"

#include <string>
#include <sstream>
#include "Parser/FunctionNode.h"

namespace palo {

	////////////////////////////////////////////////////////////////////////////////
	/// @brief parser function node datevalue
	////////////////////////////////////////////////////////////////////////////////

	class SERVER_CLASS FunctionNodeDateformat : public FunctionNode {
	public:
		static FunctionNode* createNode (const string& name, vector<Node*> *params) {
			return new FunctionNodeDateformat(name, params);
		}

	public:
		FunctionNodeDateformat () : FunctionNode() {
		}

		FunctionNodeDateformat (const string& name,  vector<Node*> *params) : FunctionNode(name, params) {
		}

		Node * clone () {
			FunctionNodeDateformat * cloned = new FunctionNodeDateformat(name, cloneParameters());
			cloned->valid = this->valid;
			return cloned;
		}

	public:
		ValueType getValueType () {
			return Node::STRING;
		}      

		bool validate (Server* server, Database* database, Cube* cube, Node* destination, string& error) {
			if (!params || params->size() != 2) {
				error = "function '" + name + "' needs two parameter";
				valid = false;
				return valid;
			}

			Node* param1 = params->at(0);
			Node* param2 = params->at(1);

			// validate parameter
			if (!param1->validate(server, database, cube, destination, error) || !param2->validate(server, database, cube, destination, error)) {
				return valid = false;         
			}

			// check data type param1
			if (param1->getValueType() != Node::NUMERIC && 
				param1->getValueType() != Node::UNKNOWN) {
					error = "parameter1 of function '" + name + "' has wrong data type";        
					return valid = false;
			}

			// check data type param2
			if (param2->getValueType() != Node::STRING && 
				param2->getValueType() != Node::UNKNOWN) {
					error = "parameter2 of function '" + name + "' has wrong data type";        
					return valid = false;
			}

			return valid = true;  
		}


		RuleValueType getValue (CellPath* cellPath, User* user, bool* isCachable, set< pair<Rule*, IdentifiersType> >* ruleHistory) {
			RuleValueType result;
			result.type = Node::STRING; 

			if (valid) {
				RuleValueType date = params->at(0)->getValue(cellPath,user,isCachable, ruleHistory);
				RuleValueType fmt = params->at(1)->getValue(cellPath,user,isCachable, ruleHistory);

				char* oldLocale = setlocale(LC_ALL, NULL);

				if (date.type == Node::NUMERIC && fmt.type==Node::STRING) {
					try {
						time_t tt = (time_t) date.doubleValue;

						/*
						\y the last two digits of the year (97, 98, etc.)
						\Y the four digits of the year (1997, 1998, etc.)
						\m the two digits of the month (01 through 12)
						\M the abbreviation of the month (JAN, FEB, etc.)
						\d the two digits of the day (01 through 31)
						\D the digit of the day (1 through 31)
						\h the hour in military time (00 through 23)
						\H the standard hour (1 through 12)
						\i the minute (00 through 59)
						\s the second (00 through 59)
						\p a.m. or p.m. */

						
						setlocale(LC_NUMERIC, "");

						string format = fmt.stringValue;
						stringstream ss;

						if (format.empty())
							ss<<"%m-%d-%y";
						else {
							size_t i;
							for (i = 0; i+1<format.length(); i++)
							{
								bool es = false;
								if (format[i]=='\\')
									switch (format[i+1]) {
										case '\\': ss<<"\\";es = true; break;
										case 'Y': ss<<"%Y"; es = true;break;
										case 'y': ss<<"%y"; es = true;break;
										case 'm': ss<<"%m"; es = true;break;
										case 'M': ss<<"%b"; es = true;break;
										case 'd': ss<<"%d"; es = true;break;
										case 'D': ss<<"%#d"; es = true;break;
										case 'h': ss<<"%H"; es = true;break;
										case 'H': ss<<"%I"; es = true;break;
										case 'i': ss<<"%M"; es = true;break;
										case 's': ss<<"%S"; es = true;break;
										case 'p': ss<<"%p"; es = true;break;
								}
								if (es)
									i++;
								else
									ss<<format[i];
							}
							if (i<format.length())
								ss<<format[i];
						}
						int max = 256;
						char * s = new char[max];
						struct tm* t = localtime(&tt);
						strftime(s,  max, ss.str().c_str(), t);
						result.stringValue = s;            
						delete[] s;

					}
					catch (ParameterException e) {
						result.stringValue = "";
					}


					//restore old locale
					setlocale(LC_NUMERIC, oldLocale); 

					return result;
				}          
			}

			result.stringValue = "";
			return result;
		}
	};

}
#endif

