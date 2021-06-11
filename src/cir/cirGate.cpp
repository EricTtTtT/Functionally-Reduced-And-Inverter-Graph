/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include <stack>
#include <unordered_set>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/

void
CirGate::delFout_Fin(CirGate* newgate) {
    int foSize = fanOut.size();
    for (int i = 0; i < foSize; ++i) {
      int foFiSize = fanOut[i]->fanIn.size();
      for (int j = 0; j < foFiSize; ++j) {
        if (fanOut[i]->fanIn[j] == this) {
          fanOut[i]->fanIn[j] = newgate;
          break;
        }
      }
    }
  }

void
CirGate::delFi_Fout() {
   int fiSize = fanIn.size();
   for (int i = 0; i < fiSize; ++i) {
      int preFoSize = fanIn[i]->fanOut.size();
      for (int j = 0;  j < preFoSize; ++j) {
         if (fanIn[i]->fanOut[j] == this) {
            fanIn[i]->fanOut.erase(fanIn[i]->fanOut.begin() + j);
            break;
         }
      }
   }
  }

void
CirGate::merge(CirGate* gate, bool invG)
{
   gate->delFi_Fout();
   gate->delFout_Fin(this);
   int size = gate->fanOut.size();

   for (int i =  0; i < size; ++i) {
      fanOut.push_back(gate->fanOut[i]);
      if (invG)
         fanOut_inv.push_back(~gate->fanOut_inv[i]);
      else
         fanOut_inv.push_back(gate->fanOut_inv[i]);
   }
}

void
CirGate::reportGate() const
{
   cout << "================================================================================" << endl;
   cout << "= " << getTypeStr() << "(" << gateId << ")";
   if (!name.empty())
      cout << "\"" << name << "\"";
   cout << ", line " << line << endl;
   cout << "= FECs:";
   if (fecIdx != -1) {
      int size = cirMgr->fecGroup[fecIdx].size();
      for (int i = 0; i < size; ++i) {
         int curId = cirMgr->fecGroup[fecIdx][i];
         if (curId == gateId) continue;
         cout << " " ;
         if (cirMgr->Map[curId]->gateValue != gateValue) cout << '!';
         cout << curId;
      }
   }
   cout << endl;
   cout << "= Value : ";
   string tmp = gateValue.to_string().substr(PatternSIZE - 64, 64);
   for (int i = 63; i > -1; --i) {
      cout << tmp[i];
      if (i % 8 == 0 && i > 5) cout << '_';
   }
   cout << endl;
   cout << "================================================================================" << endl;
}

void
CirGate::reportFanin(int level) const
{
   #define pCi pair<size_t,int>
   assert (level >= 0);

   stack<pCi> s;
   unordered_set<CirGate *> visted;

   cout << getTypeStr() + " ";
   cout << gateId << endl;
   if (level > 0) {
      for (int i = fanIn.size() - 1; i >= 0; i--)
         s.push(pCi(size_t(fanIn[i]) + size_t(fanIn_inv[i]), 1));
   }

   while(!s.empty()) {
      CirGate *gate = (CirGate *)(s.top().first & ~0x1);
      bool inv = s.top().first & 0x1;
      bool unfold;
      int glevel = s.top().second;
      s.pop();

      assert(gate != NULL); // debug

      unfold = (visted.find(gate) == visted.end());

      for (int i = 0; i < glevel; i++)
         cout << "  ";
      cout << (inv?"!":"") << gate->getTypeStr() << " ";
      cout << gate->gateId;
      if (glevel != level && gate->fanIn.size() != 0)
         cout << (!unfold?" (*)":"");
      cout << endl;
      
      if (glevel == level || !unfold || fanIn.size() == 0) // 看還要不要繼續放進去
         continue;

      visted.insert(gate);

      for (int i = gate->fanIn.size() - 1; i >= 0; i--)
         s.push(pCi(size_t(gate->fanIn[i]) + size_t(gate->fanIn_inv[i]), glevel+1));
   }
   return;
}

void
CirGate::reportFanout(int level) const
{
   #define pCi pair<size_t,int>
   assert (level >= 0);

   stack<pCi> s;
   unordered_set<CirGate *> visted;

   cout << getTypeStr() + " ";
   cout << gateId << endl;
   if (level > 0) {
      for (int i = fanOut.size() - 1; i >= 0; i--)
         s.push(pCi(size_t(fanOut[i]) + size_t(fanOut_inv[i]), 1));
   }

   while(!s.empty()) {
      CirGate *gate = (CirGate *)(s.top().first & ~0x1);
      bool inv = s.top().first & 0x1;
      bool unfold;
      int glevel = s.top().second;
      s.pop();

      assert(gate != NULL); // debug

      unfold = (visted.find(gate) == visted.end());

      for (int i = 0; i < glevel; i++)
         cout << "  ";
      cout << (inv?"!":"") << gate->getTypeStr() << " ";
      cout << gate->gateId;
      if (glevel != level && gate->fanOut.size() != 0)
         cout << (!unfold?" (*)":"");
      cout << endl;
      
      if (glevel == level || !unfold || fanOut.size() == 0) // 看還要不要繼續放進去
         continue;

      visted.insert(gate);

      for (int i = gate->fanOut.size() - 1; i >= 0; i--)
         s.push(pCi(size_t(gate->fanOut[i]) + size_t(gate->fanOut_inv[i]), glevel+1));
   }
   return;
}

ostream& 
operator << (ostream& os, const CirGate& gate)
{
   switch(gate.gateType) {
      case CONST_GATE:
         os << "CONST0" << endl;
         break;

      case PI_GATE:
         os << "PI  " << gate.gateId;
         if (!gate.name.empty())
            cout << " (" << gate.name << ")";
         cout << endl;
         break;

      case AIG_GATE:
         os << "AIG " << gate.gateId << " ";
         os << (gate.fanIn_flo[0]?"*":"") << (gate.fanIn_inv[0]?"!":"") << gate.fanIn[0]->gateId << " ";
         os << (gate.fanIn_flo[1]?"*":"") << (gate.fanIn_inv[1]?"!":"") << gate.fanIn[1]->gateId << endl;
         break;

      case PO_GATE:
         os << "PO  " << gate.gateId << " ";
         os << (gate.fanIn_flo[0]?"*":"") << (gate.fanIn_inv[0]?"!":"") << gate.fanIn[0]->gateId;
         if (!gate.name.empty())
            cout << " (" << gate.name << ")";
         cout << endl;
         break;

      case UNDEF_GATE:
         os << "undef_gate" << endl;
         break;

      default:
         os << "shouldn't have default" << endl;
         break;
   }
   return os;
}

