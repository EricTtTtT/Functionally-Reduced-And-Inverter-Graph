/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <set>
#include <bitset>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"
#include "cirGate.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr()
   { 
      CirCuit.push_back(new CirGate(CONST_GATE, 0, 0, 0)); 
      Map[0] = CirCuit[0]; 
   }
   ~CirMgr() 
   {
      for (CirGate* cptr : CirCuit)
         delete cptr;
   }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const
   {
      if (Map.find(gid) == Map.end())
         return 0;
      return Map.at(gid);
   }

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }
   void simulate();

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

   friend class CirGate;

private:
   ofstream           *_simLog;

   vector<vector<int> > fecGroup;
   int patternNum;
   void fecGroupBetter();
   void fecGroupSort_ReId();
   void setFecIdx();
   void simOutput(string*);
   
   // the Graph data
   int MILOA[5];
   vector<CirGate *> CirCuit;
   unordered_map<int,CirGate *> Map; // store the cptr in map[gateID]
   vector<CirGate *> netList;
   vector<int> with_float;
   vector<int> def_notused;

   // function for parser
   bool readHeader(fstream& fstr);
   bool readInputDef(fstream& fstr);
   bool readOutputDef(fstream& fstr, vector<int>& po);
   bool readAIGDef(fstream& fstr, vector<pair<int,int>>& aig);
   bool readSymbol(fstream& fstr);
   bool readComment(fstream& fstr);

   // delete helper
   bool* delCirIdHash;
   void delHelper() {
      int CSize = CirCuit.size();
      for (int i = --CSize; i > 0; --i) {
         if (!delCirIdHash[CirCuit[i]->gateId]) continue;
         Map.erase(CirCuit[i]->gateId);
         swap(CirCuit[i], CirCuit[CSize]);
         delete CirCuit[CSize--];
         CirCuit.pop_back();
      }
   }
   void gen_with_float() {
      int CSize = CirCuit.size();
      with_float.clear();
      CSize = CirCuit.size();
      for (int i = 1; i < CSize; ++i) {
         if (CirCuit[i]->gateType == AIG_GATE) {
            for (CirGate* fi : CirCuit[i]->fanIn)
            if (fi->gateType == UNDEF_GATE)
               with_float.push_back(CirCuit[i]->gateId);
         }
         else if (CirCuit[i]->gateType == PO_GATE) {
            if (CirCuit[i]->fanIn[0]->gateType == UNDEF_GATE)
            with_float.push_back(CirCuit[i]->gateId);
         }
      }
      sort(with_float.begin(), with_float.end());
   }

   // connect
   bool connect(vector<int>& po, vector<pair<int,int>>& aig);

   // netList
   void genNetList(CirGate *gate, unordered_set<CirGate *>& visted);

   // optimize() helper
   void optRecHelper(CirGate*, bool*);
};

#endif // CIR_MGR_H
