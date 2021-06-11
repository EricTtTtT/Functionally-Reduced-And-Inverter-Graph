/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include <bitset>
#include "cirDef.h"
#include "sat.h"
#define PatternSIZE 2048 // 32768
#define e10 1000000000
#define ull unsigned long long int
using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
public:
  CirGate(){}
  CirGate(GateType t, int id, int l, int c):
        gateType(t), gateId(id), line(l), column(c) {
        gateValue.reset(); fecIdx = -1;}
  ~CirGate() {}
  
  // Basic access methods
  string getTypeStr() const {
    switch (gateType)
    {
      case UNDEF_GATE:
        return "UNDEF";
      case PI_GATE:
        return "PI";
      case PO_GATE:
        return "PO";
      case AIG_GATE:
        return "AIG";
      case CONST_GATE:
        return "CONST";
      default:
        return "errow!! in CirGate::getTypeStr() ";
    }
  }
  unsigned getLineNo() const { return 0; }
  bool isAig() const { if (gateType == AIG_GATE) return true; return false; }

  // Printing functions
  void printGate() const {}
  void reportGate() const;
  void reportFanin(int level) const;
  void reportFanout(int level) const;
  
  // operator overload
  friend ostream& operator << (ostream& os, const CirGate& gate);

  friend class CirMgr;

private:
  GateType gateType;
  int gateId;
  int line, column;
  int fecIdx;
  bitset<PatternSIZE> gateValue;
  Var satVar;

  vector<CirGate *> fanIn;
  vector<bool> fanIn_inv;
  vector<bool> fanIn_flo;

  vector<CirGate *> fanOut;
  vector<bool> fanOut_inv;

  string name; // symbol name

  // helper functions by TT
  void merge(CirGate*, bool);
  void delFi_Fout();
  void delFout_Fin(CirGate*);

protected:
};

#endif // CIR_GATE_H
