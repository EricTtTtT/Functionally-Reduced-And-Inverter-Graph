/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include <algorithm>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  bool isInNetList[ MILOA[0] + MILOA[3] + 1] = {false};
  int netSize = netList.size();
  for (int i = 0; i < netSize; ++i)
    isInNetList[netList[i]->gateId] = true;
  // collect del Id(s)
  int cirSize = CirCuit.size();
  vector<int> delCirId;
  for (int i = 1; i < cirSize; ++i) {
    if (isInNetList[CirCuit[i]->gateId] || CirCuit[i]->gateType == PI_GATE) continue;
    delCirId.push_back(i);
  }

  // deal with def_notused
  def_notused.clear();
  for (int i = 1; i < MILOA[1]+1; ++i)
    if (!isInNetList[CirCuit[i]->gateId])
      def_notused.push_back(CirCuit[i]->gateId);

  // deal with with_floating
  int flSize = with_float.size();
  for (int i = 0; i < flSize; ++i ) {
    if (with_float.empty()) break;
    if (!isInNetList[with_float[i]]) {
      with_float.erase(with_float.begin() + i);
      --i;
    }
  }
  
  // deleting
  int delSize = delCirId.size();
  int notAIG = 0;
  for (int id : delCirId)
    if (CirCuit[id]->gateType != AIG_GATE)
      ++notAIG;
  MILOA[4] -= (delSize - notAIG);
  for (int i = delSize - 1; i >= 0; --i) {
    CirCuit[delCirId[i]]->delFi_Fout();
    Map.erase(CirCuit[delCirId[i]]->gateId);
    swap(CirCuit[delCirId[i]], CirCuit[--cirSize]);
    delete CirCuit[cirSize];
    CirCuit.pop_back();
  }
}


// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  bool checked[ MILOA[0] + MILOA[3] + 1] = {false};
  memset(delCirIdHash, 0, (MILOA[0] + MILOA[3] + 1) * sizeof(bool));

  for (int i = 0; i < MILOA[3]; ++i)
    optRecHelper(CirCuit[ i + MILOA[1] + 1], checked);

  // handle netList
  unordered_set<CirGate *> visted;
  netList.clear();
  for (int i = 1; i <= MILOA[3]; i++)
    genNetList(Map[MILOA[0] + i], visted);

  // deleting Gates
  delHelper();

  // handle with_float
  gen_with_float();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
void
CirMgr::optRecHelper(CirGate* curGate, bool* checked)
{
  if (checked[curGate->gateId])
    return;
  checked[curGate->gateId] = true;

  int curFiSize = curGate->fanIn.size();
  for (int i = 0; i < curFiSize; ++i)
    optRecHelper(curGate->fanIn[i], checked);
  
  if (curGate->gateType != AIG_GATE || curGate->fanIn.empty())
    return;
  
  assert(curFiSize == 2);
  CirGate* fanin0 = curGate->fanIn[0];
  CirGate* fanin1 = curGate->fanIn[1];
  CirGate* newGate = curGate;
  bool newInv = false;
  bool newFl = false;
  int case0a = 0; // 0: nothing,  1: a0 || !a0 , 2: 0a || 0!a  , 3: a!a || !aa

  // check if curGate optimizable
  if (fanin0 == fanin1) {
    if (curGate->fanIn_inv[0] == curGate->fanIn_inv[1]) {
      newGate = fanin0;     // a a
      if (curGate->fanIn_inv[0])
        newInv = true;      // !a !a
    }
    else {
      case0a = 3;
      newGate = CirCuit[0];   // a !a || !a a
    }
  }
  else if (fanin0->gateId == 0) {
    if (curGate->fanIn_inv[0]) {
      newGate = fanin1;     // 1 a
      if (curGate->fanIn_inv[1])
        newInv = true;     // 1 !a
    }
    else {
      case0a = 2;
      newGate = CirCuit[0];   // 0 a || 0 !a
    }
  }
  else if (fanin1->gateId == 0) {
    if (curGate->fanIn_inv[1]) {
      newGate = fanin0;     // a 1
      if (curGate->fanIn_inv[0])
        newInv = true;      // !a 1
    }
    else {
      case0a = 1;
      newGate = CirCuit[0];    // a 0 || !a 0
    }
  }

  if (newGate == curGate)
    return;

  // debug
  cout << "Simplifying: " << newGate->gateId <<" merging ";
  if (newInv) cout << "!";
  cout << curGate->gateId<<"..." << endl;

  // handle cur's fanin part
  curGate->delFi_Fout();
  int curFoSize = curGate->fanOut.size();
  for (int i = 0; i < curFoSize; ++i) {
    newGate->fanOut.push_back(curGate->fanOut[i]);
    newGate->fanOut_inv.push_back(curGate->fanOut_inv[i]);
  }
  // UNDEF case
  if (case0a == 1) { // a0 || !a0
    if (fanin0->fanOut.empty()) {
      if (fanin0->gateType == UNDEF_GATE) {
        delCirIdHash[fanin0->gateId] = true;
      }
      else {
        def_notused.push_back(fanin0->gateId);
      }
    }
  }
  else if (case0a == 2 || case0a == 3) { // 0a || 0!a || a!a || !aa
    if (fanin1->fanOut.empty()) {
      if (fanin1->gateType == UNDEF_GATE) {
        delCirIdHash[fanin1->gateId] = true;
      }
      else {
        def_notused.push_back(fanin1->gateId);
      }
    }
  }

  // handle cur's fanout part
  for (int i = 0; i < curFoSize; ++i) {
    CirGate* tmp = curGate->fanOut[i];
    int tmpFiSize = tmp->fanIn.size();
    for (int j = 0; j < tmpFiSize; ++j) {
      if (tmp->fanIn[j] == curGate) {
        tmp->fanIn[j] = newGate;
        if (tmp->fanIn_inv[j])
          tmp->fanIn_inv[j] = !newInv;
        else
          tmp->fanIn_inv[j] = newInv;       
        tmp->fanIn_flo[j] = newFl;
      }
    }
  }

  // delete curGate and handle Circuit, Map
  --MILOA[4];
  delCirIdHash[curGate->gateId] = true;
}
