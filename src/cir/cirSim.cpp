/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <bitset>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include "time.h"

using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
bitset<PatternSIZE> zero;



/**************************************/
/*   Static varaibles and functions   */
/**************************************/

void
CirMgr::simulate() {
  
  unordered_map<bitset<PatternSIZE>, vector<int> > tmpMap;
  unordered_map<bitset<PatternSIZE>, vector<int> >::iterator it;

  // update value
  for (CirGate* cur : netList) {
    if (!cur->isAig()) continue;
    bitset<PatternSIZE> bs[2];
    for (int i = 0; i < 2; ++i) {
      if (cur->fanIn[i]->gateType == UNDEF_GATE) {
        if (cur->fanIn_inv[0]) {
          bs[i] = ~zero;
        } else {
          bs[i] = zero; }
      } else {
        if (cur->fanIn_inv[i]) {
          bs[i] = ~cur->fanIn[i]->gateValue;
        } else {
          bs[i] = cur->fanIn[i]->gateValue; } }
    }
    cur->gateValue = (bitset<PatternSIZE>)(bs[0] & bs [1]);

  }

  fecGroupBetter();
  int preSize = fecGroup.size();
  if (preSize == 0) {
    tmpMap[zero].push_back(0);
    for (CirGate* cur : netList) {
      if (cur->isAig()) {
        // hash into tmpMap
        it = tmpMap.find(cur->gateValue);
        if (it != tmpMap.end()) {
          it->second.push_back(cur->gateId);
          continue;
        }
        it = tmpMap.find(~cur->gateValue);
        if (it != tmpMap.end()) {
          it->second.push_back(cur->gateId + e10);
          continue;
        }
        tmpMap[cur->gateValue].push_back(cur->gateId);
      }
    }

    // push into fecGroup
    for (it = tmpMap.begin(); it != tmpMap.end(); ++it) {
      int tmpSize = it->second.size();
      if (tmpSize < 2) continue;
      fecGroup.push_back(it->second);
    }
  }
  else {
    for (int i = 0; i < preSize; ++i) {
      int gSize = fecGroup[i].size();
      if (gSize < 2) continue;

      vector<int> invGpIdx;
      tmpMap.clear();
      for (int j = 0; j < gSize; ++j) {
        int curId = fecGroup[i][j];
        if (curId < e10) {
          bitset<PatternSIZE> tmpValue = Map[curId]->gateValue;
          it = tmpMap.find(tmpValue);
          if (it != tmpMap.end()) {
            it->second.push_back(curId);
            continue;
          }
          tmpMap[tmpValue].push_back(curId);        
        }
      }
      for (int j = 0; j < gSize; ++j) {
        int curId = fecGroup[i][j];
        if (curId >= e10) {
          bitset<PatternSIZE> tmpValue = Map[curId - e10]->gateValue;
          it = tmpMap.find(~tmpValue);
          if (it != tmpMap.end()) {
            it->second.push_back(curId);
            continue;
          }
          else {
            invGpIdx.push_back(j);
          }
        }
      }
      // push into fecGroup
      for (it = tmpMap.begin(); it != tmpMap.end(); ++it) {
        int tmpSize = it->second.size();
        if (tmpSize < 2) continue;
        fecGroup.push_back(it->second);
      }

      tmpMap.clear();
      for (int kId : invGpIdx) {
        int curId = fecGroup[i][kId] - e10;
        bitset<PatternSIZE> tmpValue = Map[curId]->gateValue;
        it = tmpMap.find(tmpValue);
        if (it != tmpMap.end()) {
          it->second.push_back(curId);
          continue;
        }
        it = tmpMap.find(~tmpValue);
        if (it != tmpMap.end()) {
          it->second.push_back(curId + e10);
          continue;
        }
        tmpMap[tmpValue].push_back(curId);
      }
      // push into fecGroup
      for (it = tmpMap.begin(); it != tmpMap.end(); ++it) {
        int tmpSize = it->second.size();
        if (tmpSize < 2) continue;
        fecGroup.push_back(it->second);
      }

      fecGroup[i].clear();
    }
  }

  int PoEnd = MILOA[1] + MILOA[3];
  for (int i = MILOA[1] + 1; i <= PoEnd; ++i) {
    assert(CirCuit[i]->gateType == PO_GATE);
    if (CirCuit[i]->fanIn_inv[0])
      CirCuit[i]->gateValue = ~CirCuit[i]->fanIn[0]->gateValue;
    else
      CirCuit[i]->gateValue = CirCuit[i]->fanIn[0]->gateValue;
  }
  
}

void
CirMgr::simOutput(string fiStr[]) {
  int n = patternNum % PatternSIZE;
  string foStr[MILOA[3]];
  for (int i = 0; i < MILOA[3]; ++i) {
    if (n == 0)
      foStr[i] = CirCuit[MILOA[1] + 1 + i]->gateValue.to_string();
    else
      foStr[i] = CirCuit[MILOA[1] + 1 + i]->gateValue.to_string().substr(PatternSIZE - n, n);
  }

  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < MILOA[1]; ++j)
      *_simLog << fiStr[j][i];
    *_simLog << ' ';
    for (int j = 0; j < MILOA[3]; ++j)
      *_simLog << foStr[j][i];
    *_simLog << endl;
  }
}

void
CirMgr::fecGroupBetter() {
  int fecSize = fecGroup.size();
  vector<int> delGroup;
  for (int i = 0; i < fecSize; ++i) {
    if (fecGroup[i].size() < 2) {
      delGroup.push_back(i);
      fecGroup[i].clear();
    }
  }
  int delSize = delGroup.size();
  for (int i = delSize-1; i > -1; --i) {
    swap(fecGroup[--fecSize], fecGroup[delGroup[i]]);
    fecGroup.pop_back();
  }
}

void
CirMgr::fecGroupSort_ReId() {
  int fecSize = fecGroup.size();
  for (int i = 0; i < fecSize; ++i) {
    int gSize = fecGroup[i].size();
    for (int j = 0; j < gSize; ++j)
      if (fecGroup[i][j] >= e10)
        fecGroup[i][j] -= e10;
  }

  for (int i =0; i < fecSize; ++i) {
    sort(fecGroup[i].begin(), fecGroup[i].end());
  }
  sort(fecGroup.begin(), fecGroup.end());
}

void
CirMgr::setFecIdx() {
  int fecSize = fecGroup.size();
  for (int i = 0; i < fecSize; ++i) {
    int gSize = fecGroup[i].size();
    for (int j = 0; j < gSize; ++j) {
      Map[fecGroup[i][j]]->fecIdx = i;
    }
  }
}

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
  srand(time(NULL));
  int col = MILOA[1]; // row = PatternSIZE
  patternNum = 0;
  zero.reset();
  string fi[col]; // try directlty stored in bitset
  int repeatT;
  if (col < 1000)
    repeatT = 1 + ((int)(col*col*0.381924) / PatternSIZE);
  else
    repeatT = (40000 / PatternSIZE);

  for (int t = 0; t < repeatT; ++t) {
    for (int i = 0; i < col; ++i) {
      for (int j = 0; j < PatternSIZE / 32; ++j) {
        bitset<32> tmp(rand());
        fi[i] += tmp.to_string();
      }
    }

    for (int i = 1; i <= MILOA[1]; ++i) {
      CirCuit[i]->gateValue = bitset<PatternSIZE>(fi[i - 1]);
    }
    simulate();

    if (_simLog)
        simOutput(fi);
  }

  fecGroupBetter();
  fecGroupSort_ReId();
  setFecIdx();
}

void
CirMgr::fileSim(ifstream& patternFile)
{
  int col = MILOA[1]; // row = PatternSIZE
  patternNum = 0;
  zero.reset();
  string tmp;
  string fi[col]; // try directlty stored in bitset

  // check pattern length
  patternFile >> tmp;
  if (tmp.length() != MILOA[1]) {
    cout << "Error: Pattern(" << tmp << ") length(" << tmp.length() <<
            ") does not match the number of inputs(" << MILOA[1] << 
            ") in a circuit!!" << endl;
    return;
  }
  // read in pattern(s)
  int bi = 0;
  while(true) {
    if (patternFile.eof()) break;
    for (int j = 0; j < col; ++j) {
      if((char)tmp[j] != '0' && (char)tmp[j] != '1') {
        cout << "Error: Pattern(" << tmp <<
        ") contains a non-0/1 character('" << (char)tmp[j] << "')." << endl;
        return;
      }
      fi[j] += (char)tmp[j];
      
    }
    patternFile >> tmp;
    ++patternNum;
    ++bi;

    // pattern full
    if (bi >= PatternSIZE) {
      for (int i = 1; i <= col; ++i) {
        CirCuit[i]->gateValue = bitset<PatternSIZE>(fi[i - 1]);
      }
      simulate();
        if (_simLog)
      simOutput(fi);

      // reset fi
      for (int f = 0; f < col; ++f)
        fi[f] = "";
      bi = 0;
    }
  }

  for (int i = 1; i <= MILOA[1]; ++i) {
    CirCuit[i]->gateValue = bitset<PatternSIZE>(fi[i - 1]);
  }
  simulate();
  if (_simLog)
      simOutput(fi);

  fecGroupBetter();
  fecGroupSort_ReId();
  setFecIdx();
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
