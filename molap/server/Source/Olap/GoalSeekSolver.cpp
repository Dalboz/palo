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

#include "GoalSeekSolver.h"

#include <time.h>

namespace palo {
	namespace goalseeksolver 
	{
		namespace eps {
			//double eps = 1e-7;
			bool equal(const double& a, const double& b) { return fabs(a-b)<eps; }

			bool zero(const double& a) {return equal(a, 0);}
		}
		
		struct State {
			Problem p;	
			std::vector<std::vector<double> > dimensionElementSum;	
			int fixedIndex;			
			int variableCount;
			std::vector<int> variableIndex;
			M2D<double> pm;
			std::vector<std::map<int, std::vector<double> > > pm_rows;
			std::vector<double> variable;
			std::vector<bool> variable_set;
			time_t timeoutExpiryTime;
			bool checkTimeout;

			void CheckTimeout() const {
				if (checkTimeout && clock() > timeoutExpiryTime)
					throw CalculationTimeoutException();
			}
			
		};

		void iterate(State& s, int d, int dc, double w, std::vector<int>& coord) {
			if (d<dc) {
				for (int e = 0; e<(int)s.p.dimensionElementWeight[d].size(); e++) {
					coord.push_back(e);
					iterate(s, d+1, dc, s.p.dimensionElementWeight[d][e]*w, coord);
					coord.pop_back();
				}
			} else {		
				int cellInd = s.p.cellValue.get_index(coord);
				int variableIndex = s.variableIndex[cellInd];
				if (variableIndex!=-1) {
					for (d = 0; d<dc; d++) {
						int dcc = coord[d];
						coord[d] = 0;
						int startCellInd = s.p.cellValue.get_index(coord);
						std::vector<double>& weights = s.pm_rows[d][startCellInd];
						if (weights.size()==0) {//new row
							weights.resize(s.variableCount+1);							
						}
						weights[variableIndex] = s.p.dimensionElementWeight[d][dcc];					
						weights[s.variableCount] += weights[variableIndex] * s.p.cellValue[cellInd];										
						s.dimensionElementSum[d][dcc] += weights[variableIndex] * s.p.cellValue[cellInd];				
						coord[d] = dcc;
					}
				}		
			}
		};

		double estimate(State& s, int d, const std::vector<int>& change_coord, double new_value, const std::vector<int>& estimate_coord) {
			double old_value = s.p.cellValue[change_coord];
			double delta = new_value - old_value;
			double dsum = s.dimensionElementSum[d][change_coord[d]];
			double factor;
			if (eps::equal(dsum, old_value))
				factor = 1.0/s.p.dimensionElementWeight[d].size();
			else
				factor = (dsum-new_value)/(dsum-old_value);
			return s.p.cellValue[estimate_coord] * factor;
		}

		double estimate(State& s, const std::vector<int>& change_coord, double new_value, const std::vector<int>& estimate_coord) {
			int dc = (int)s.p.dimensionElementWeight.size();
			double value = new_value;
			std::vector<int> coord = change_coord;

			for (int d = 0; d<dc; d++) 
				if (change_coord[d]!=estimate_coord[d])
				{
					std::vector<int> ncoord = coord;
					ncoord[d] = estimate_coord[d];
					value = estimate(s, d, coord, value, ncoord);
					coord = ncoord;
				}
			return value;
		}

		void estimate_all_paths_avg_req(State& s, int ddif, int dindex, const std::vector<int>& coord, double new_value, const std::vector<int>& estimate_coord, double& sum, int& perm_count) 
		{	
			int dc = (int)s.p.dimensionElementWeight.size();
			for (int d = 0; d<dc; d++) 
				if (coord[d]!=estimate_coord[d])
					if (dindex==0)
					{
						std::vector<int> ncoord = coord;
						ncoord[d] = estimate_coord[d];
						double value = estimate(s, d, coord, new_value, ncoord);
						ddif--;
						if (ddif>0)
							for (int i = 0; i<ddif; i++)
								estimate_all_paths_avg_req(s, ddif, i, ncoord, value, estimate_coord, sum, perm_count);
						else {
							sum+=value;
							perm_count++;
						}
						return;
					} else 
						dindex--;
		}

		double estimate_all_paths_avg(State& s, const std::vector<int>& change_coord, double new_value, const std::vector<int>& estimate_coord) {
			int ddif = 0;
			int dc = (int)s.p.dimensionElementWeight.size();
			for (int d = 0; d<dc; d++) 
				if (change_coord[d]!=estimate_coord[d])
					ddif++;
			if (ddif==0) 
				return new_value;
			double sum = 0;
			int perm_count = 0;
			for (int i = 0; i<ddif; i++)
				estimate_all_paths_avg_req(s, ddif, i, change_coord, new_value, estimate_coord, sum, perm_count);

			return sum/perm_count;
		}

		void make_diagonal(State& s) {	
			M2D<double>& m = s.pm;
			int cr = 0;
			for (int c = 0; c+1<m.col_count(); c++) 		
				if (!s.variable_set[c]) {

					//check timeout
					s.CheckTimeout();

					bool found = false;
					for (int r = cr; r<m.row_count(); r++)
						if (!eps::zero(m[r][c])) {
							found = true;
							m.swap_rows(r, cr);
							for (int r = cr+1; r<m.row_count(); r++) {
								m.eliminate_column(cr, r, c);
							}
							cr++;
							break;
						}							
				}
		}

		bool check_state(State& s, std::vector<int>& variable_state) {
			M2D<double>& m = s.pm;
			int rc = m.row_count();
			int sc = m.col_count()-1;
			variable_state.clear();
			variable_state.resize(sc);
			for (int r = rc-1; r>=0; r--) {
				int pc = r;//0
				while (pc<sc && eps::zero(m[r][pc]))
					pc++;
				if (pc==sc && !eps::zero(m[r][sc]))
				{
					return false;//not solvable
				}
				variable_state[pc] = 1;
				double v = 0;//
				bool all = true;
				for (int c = pc+1; c<sc; c++)
					if (!eps::zero(m[r][c]))
					{
						if (variable_state[c]==0)
							variable_state[c] = 2;

						all &= s.variable_set[c];
						if (all)
							v+= s.variable[c]*m[r][c];
					}
					if (all) {
						s.variable[pc] = (m[r][sc]-v)/m[r][pc];			
						s.variable_set[pc] = true;
					}
			}
			return true;
		}

		void set_variable(State& s, int c, double v, bool diagonal) {
			s.variable[c] = v;
			s.variable_set[c] = true;
			M2D<double>& m = s.pm;
			int sc = m.col_count()-1;
			int rc = diagonal ? std::min(m.row_count(), c+1) : m.row_count();
			for (int i = 0; i<rc; i++) {
				m[i][sc] -= m[i][c] * v;
				m[i][c] = 0;
			}
		}

		Result solve(const Problem& p, int timeoutMiliSec) {

			State s; 
			s.p = p;
			s.fixedIndex = p.cellValue.get_index(p.fixedCoord);			
			int pc = p.cellValue.count();
			s.variableCount = 0;
			for (int i = 0; i<pc; i++)
				s.variableIndex.push_back(s.variableCount++);
			s.variable.resize(s.variableCount);
			s.variable_set = std::vector<bool>(s.variableCount, false);
			s.pm.col_count(s.variableCount+1);

			s.checkTimeout = timeoutMiliSec > 0;
			s.timeoutExpiryTime = (clock_t)(clock() + timeoutMiliSec / 1000.0 * CLOCKS_PER_SEC);

			bool simple_estimate = s.variableCount > 50;

			s.dimensionElementSum.resize(s.p.dimensionElementWeight.size());
			for (int i =0; i<(int)s.p.dimensionElementWeight.size(); i++)
				s.dimensionElementSum[i].resize(p.dimensionElementWeight[i].size());
			

			s.pm_rows.resize(s.p.dimensionElementWeight.size());

			std::vector<int> c;
			iterate(s, 0, (int)s.p.dimensionElementWeight.size(), 1.0, c);

			for (int i = 0; i<(int)s.p.dimensionElementWeight.size(); i++)  
				for (std::map<int, std::vector<double> >::iterator it = s.pm_rows[i].begin(); it!=s.pm_rows[i].end(); it++)
					s.pm.append(it->second);
			

			set_variable(s, s.fixedIndex, s.p.fixedValue, false);	
			make_diagonal(s);
			s.pm.remove_trailing_zero_rows();	

			Result res; 	
			res.valid = false;
			
			std::vector<int> mv;
			if (check_state(s, mv)) 
			{
				for (int i = 0; i<(int)mv.size(); i++)
					if (mv[i]==2) {
						double nv = simple_estimate ? estimate(s, s.p.fixedCoord, s.p.fixedValue, s.p.cellValue.get_coords(i)) : estimate_all_paths_avg(s, s.p.fixedCoord, s.p.fixedValue, s.p.cellValue.get_coords(i));
						set_variable(s, i, nv, true);				
					}
				make_diagonal(s);
				s.pm.remove_trailing_zero_rows();						
				if (check_state(s, mv)) 
				{
					res.cellValue = p.cellValue;
					for (int i = 0; i<(int)s.variable.size(); i++)
						res.cellValue[res.cellValue.get_coords(i)] = s.variable[i];
					res.valid = true;
				}
			}
			return res;
		}
	}
}
