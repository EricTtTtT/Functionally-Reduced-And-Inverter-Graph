/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <algorithm>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"
#define hkey unsigned long long int
#define pii pair<int, int>
using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
hkey hkeyGen(unsigned int a, unsigned int b, bool aInv, bool bInv)
{
  if (a > b) {
    unsigned int c = a;
    a = b; b = c;
  }
  hkey ret = 0ull;
  ret += (((hkey)a<<1) + (hkey)aInv);
  ret += (((hkey)b<<33) + ((hkey)bInv<<32));
  return ret;
}

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
  memset(delCirIdHash, 0, (MILOA[0] + MILOA[3] + 1) * sizeof(bool));
  unordered_map<hkey, CirGate*> strHash;

  // check through netlist
  for (CirGate* curGate : netList) {
    if (curGate->gateType != AIG_GATE) continue;
    hkey k = hkeyGen(
      (unsigned int)curGate->fanIn[0]->gateId, (unsigned int)curGate->fanIn[1]->gateId,
      curGate->fanIn_inv[0], curGate->fanIn_inv[1]);
    CirGate* mergeGate;
    if (strHash.find(k) != strHash.end()) {
      mergeGate = strHash[k];
      mergeGate->merge(curGate, false);
      --MILOA[4];
      delCirIdHash[curGate->gateId] = true;
      // cout << "Simplifying: " << mergeGate->gateId <<" merging " << curGate->gateId<<"..." << endl;
    }
    else {
      strHash[k] = curGate;
    }
  }
  // handle netlist
  unordered_set<CirGate *> visted;
  netList.clear();
  for (int i = 1; i <= MILOA[3]; i++)
    genNetList(Map[MILOA[0] + i], visted);

  // deleting Gates
  delHelper();;
  
  // handle with_float
  gen_with_float();
}

void
CirMgr::fraig()
{
  SatSolver solver;
  solver.initialize();
  int fecSize = fecGroup.size();
  bool visited[fecSize] {false};

  vector<int> delGateVec;
  vector<pii> delPairs;

  CirCuit[0]->satVar = solver.newVar();
  solver.assertProperty(CirCuit[0]->satVar, false);

  for (CirGate* cur : netList) {
    if (cur->gateType == PO_GATE) {
      continue;
    } else if (cur->gateType == PI_GATE) {
      cur->satVar = solver.newVar();
    } else if (cur->isAig()) {
      solver.addAigCNF(cur->satVar, cur->fanIn[0]->satVar, cur->fanIn_inv[0],
          cur->fanIn[1]->satVar, cur->fanIn_inv[1]);
    }
  }

  for (CirGate* cur : netList) {
    if (!cur->isAig()) continue;
    int fId = cur->fecIdx;
    if (fId == -1) continue;
    if (visited[fId]) continue;
    visited[fId] = true;

    delPairs.clear();
    Var fV = solver.newVar();
    int gSize = fecGroup[cur->fecIdx].size();
    bool deleted[gSize] {false};

    for (int i = 0; i < gSize - 1; ++i) {
      if (deleted[i]) continue;
      for (int j = i+1; j < gSize; ++j) {
        CirGate* aPtr = Map[fecGroup[fId][i]];
        CirGate* bPtr = Map[fecGroup[fId][j]];
        bool invAB = true;
        if (aPtr->gateValue != bPtr->gateValue)
          invAB = false;

        solver.addXorCNF(fV, aPtr->satVar, true, bPtr->satVar, invAB);
        solver.assumeProperty(fV, true);
        if ((!solver.assumpSolve())) {
          delPairs.push_back(pii(i, j));
          deleted[j] = true;
        }
        solver.assumeRelease();
      }
    }

    for (int i = 0; i < delPairs.size(); ++i) {
      bool invG = false;
      int a = fecGroup[fId][ delPairs[i].first ];
      int b = fecGroup[fId][ delPairs[i].second ];
      if (Map[a] ->gateValue != Map[b] -> gateValue)
        invG = true;
      Map[a] -> merge (Map[b], invG);
      // cout << a << "merging" << b << endl;
      delGateVec.push_back(b);
    }
  }


  // handle cirMgr

  delete[] delCirIdHash;
  int CSize = CirCuit.size();
  delCirIdHash = new bool[CSize] {false};
  int delSize = delGateVec.size();
  MILOA[4] -= delSize;
  
  for (int i = 0; i < CSize; ++i) {
    for (int j = 0; j < delSize; ++j) {
      if (CirCuit[i]->gateId == delGateVec[j]) {
        delCirIdHash[delGateVec[j]] = true;
      }
    }
  }

  // handle netlist
  unordered_set<CirGate *> visted;
  netList.clear();
  for (int i = 1; i <= MILOA[3]; i++)
    genNetList(Map[MILOA[0] + i], visted);

  // deleting Gates
  delHelper();;
  
  // handle with_float
  gen_with_float();
  
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
