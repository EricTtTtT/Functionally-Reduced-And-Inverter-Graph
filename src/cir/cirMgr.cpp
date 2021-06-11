/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <stack>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
      case EXTRA_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Extra space character is detected!!" << endl;
         break;
      case MISSING_SPACE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing space character!!" << endl;
         break;
      case ILLEGAL_WSPACE: // for non-space white space character
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal white space char(" << errInt
              << ") is detected!!" << endl;
         break;
      case ILLEGAL_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
              << errMsg << "!!" << endl;
         break;
      case ILLEGAL_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
              << errMsg << "\"!!" << endl;
         break;
      case ILLEGAL_SYMBOL_TYPE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Illegal symbol type (" << errMsg << ")!!" << endl;
         break;
      case ILLEGAL_SYMBOL_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Symbolic name contains un-printable char(" << errInt
              << ")!!" << endl;
         break;
      case MISSING_NUM:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Missing " << errMsg << "!!" << endl;
         break;
      case MISSING_IDENTIFIER:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
              << errMsg << "\"!!" << endl;
         break;
      case MISSING_NEWLINE:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": A new line is expected here!!" << endl;
         break;
      case MISSING_DEF:
         cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
              << " definition!!" << endl;
         break;
      case CANNOT_INVERTED:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": " << errMsg << " " << errInt << "(" << errInt/2
              << ") cannot be inverted!!" << endl;
         break;
      case MAX_LIT_ID:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
              << endl;
         break;
      case REDEF_GATE:
         cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
              << "\" is redefined, previously defined as "
              << errGate->getTypeStr() << " in line " << errGate->getLineNo()
              << "!!" << endl;
         break;
      case REDEF_SYMBOLIC_NAME:
         cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
              << errMsg << errInt << "\" is redefined!!" << endl;
         break;
      case REDEF_CONST:
         cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
              << ": Cannot redefine constant (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_SMALL:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too small (" << errInt << ")!!" << endl;
         break;
      case NUM_TOO_BIG:
         cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
              << " is too big (" << errInt << ")!!" << endl;
         break;
      default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/

bool
CirMgr::readCircuit(const string& fileName)
{
   fstream fp;
   fp.open(fileName, ios::in);
   lineNo = 0;
   // check file is opened
   if (!fp.is_open()) {
      cerr << "Cannot open design \"" << fileName << "\"!!" << endl;
      return false;
   }

   // vector that record the input information
   vector<int> po;
   vector<pair<int,int>> aig;

   // call helper function
   if (!readHeader(fp))
      return false;
   readInputDef(fp);
   readOutputDef(fp, po);
   readAIGDef(fp, aig);
   readSymbol(fp);

   fp.close();
   // connect circuit
   connect(po, aig);
   
   string enum_str[5] = {"UNDEF_GATE", "PI_GATE", "PO_GATE", "AIG_GATE", "CONST_GATE"};
   // try to print
   // cout << "below is the vecotr-Graph inside" << endl;
   // for (int i = 0; i < CirCuit.size(); i++) {
   //    cout << "[" << i << "] ";
   //    printf("gateIdx(%d) ", CirCuit[i]->gateId);
   //    for (int j = 0; j < CirCuit[i]->fanIn.size(); j++)
   //       printf("fanIn[%d]=%d, ", j, CirCuit[i]->fanIn[j]->gateId);
   //    printf("\n");
   // }
   
   // construct netlist
   unordered_set<CirGate *> visted;
   for (int i = 1; i <= MILOA[3]; i++)
      genNetList(Map[MILOA[0] + i], visted);

   // mantain def_notused (only for PI、AIG)
   for (int i = 1; i <= MILOA[1]; i++) { // don't need to check constant0
      if (CirCuit[i]->fanOut.size() == 0)
         def_notused.push_back(CirCuit[i]->gateId);
   }
   for (int i = 1; i <= MILOA[4]; i++) {
      int idx = MILOA[1] + MILOA[3] + i;
      if (CirCuit[idx]->fanOut.size() == 0)
         def_notused.push_back(CirCuit[idx]->gateId);
   }

   // mantain with_float (only for PO、AIG)
   // by checking UNDEF's fanOut
   int Csize = CirCuit.size();
   visted.clear();
   for (int i = 1 + MILOA[1] + MILOA[3] + MILOA[4]; i < Csize; i++) {
      int fanOutSize = CirCuit[i]->fanOut.size();
      for (int j = 0; j < fanOutSize; j++) {
         if (visted.find(CirCuit[i]->fanOut[j]) == visted.end()) {
            with_float.push_back(CirCuit[i]->fanOut[j]->gateId);
            visted.insert(CirCuit[i]->fanOut[j]);
         }
      }
   }

   sort(def_notused.begin(), def_notused.end());
   sort(with_float.begin(), with_float.end());
   
   delCirIdHash = new bool[MILOA[0] + MILOA[3] + 1] {false};
   return true;
}

bool 
CirMgr::readHeader(fstream& fstr)
{
   string str;

   lineNo++;

   fstr >> str; // read "aag"
   if (str != "aag") {
      cerr << "[ERROR] Line " << lineNo << ": Illegal identifier \"" << str << "\"!!" << endl;
      return false;
   }
   for (int i = 0; i < 5; i++) // read M I L O A
      fstr >> MILOA[i];
   return true;
}
bool 
CirMgr::readInputDef(fstream& fstr)
{
   int num, gateID;

   for (int i = 0; i < MILOA[1]; i++) {
      fstr >> num;
      assert(num%2 == 0);
      gateID = num/2;
      CirGate *c = new CirGate(PI_GATE, gateID, ++lineNo, 1);

      Map[gateID] = c;
      CirCuit.push_back(c);
   }
   return true;
}
bool 
CirMgr::readOutputDef(fstream& fstr, vector<int>& po)
{
   int num, gateID;

   for (int i = 1; i <= MILOA[3]; i++) {
      fstr >> num;
      gateID = MILOA[0] + i;
      CirGate *c = new CirGate(PO_GATE, gateID, ++lineNo, 1);
      po.push_back(num);

      Map[gateID] = c;
      CirCuit.push_back(c);
   }
   return true;
}
bool 
CirMgr::readAIGDef(fstream& fstr, vector<pair<int,int>>& aig)
{
   int num, fanin1, fanin2, gateID;

   for (int i = 0; i < MILOA[4]; i++) {
      fstr >> num >> fanin1 >> fanin2;
      assert(num%2 == 0);
      gateID = num/2;
      CirGate *c = new CirGate(AIG_GATE, gateID, ++lineNo, 1);
      aig.push_back(pair<int,int>(fanin1, fanin2));

      Map[gateID] = c;
      CirCuit.push_back(c);
   }
   return true;
}
bool
CirMgr::readSymbol(fstream& fstr)
{
   // [ilo][position] <symbolic name>
   string str, name;
   int num, gateIdx;

   while (fstr >> str && str[0] != 'c') {
      num = stoi(str.substr(1));
      gateIdx = (str[0] == 'i')?(num+1):(MILOA[1] + num + 1);

      getline(fstr, name);
      name.erase(0, name.find_first_not_of(" "));

      CirCuit[gateIdx]->name = name;
   }
   return true;
}

bool
CirMgr::connect(vector<int>& po, vector<pair<int,int>>& aig)
{
   // handle po
   for (int i = 0; i < MILOA[3]; i++) {
      CirGate *fanInGate, *thisGate;
      int fanInId = po[i]/2;
      bool isflo;

      thisGate = CirCuit[MILOA[1] + i + 1];

      if(Map.find(fanInId) == Map.end()) { // UNDEF_GATE
         fanInGate = new CirGate(UNDEF_GATE, fanInId, 0, 0);
         CirCuit.push_back(fanInGate);
         Map[fanInId] = fanInGate;
      } else {
         fanInGate = Map[fanInId];
      }
      isflo = (fanInGate->gateType == UNDEF_GATE);
      thisGate->fanIn.push_back(fanInGate);
      thisGate->fanIn_inv.push_back(po[i] & 1);
      thisGate->fanIn_flo.push_back(isflo);

      fanInGate->fanOut.push_back(thisGate);
      fanInGate->fanOut_inv.push_back(po[i] & 1);
   }
   // handle aig
   for (int i = 0; i < MILOA[4]; i++) {
      CirGate *fanInGate, *thisGate;
      int fanInId[2] = {aig[i].first/2, aig[i].second/2};
      bool isflo;

      thisGate = CirCuit[MILOA[1] + MILOA[3] + i + 1];
      // handle fanIn1
      if(Map.find(fanInId[0]) == Map.end()) { // UNDEF_GATE
         fanInGate = new CirGate(UNDEF_GATE, fanInId[0], 0, 0);
         CirCuit.push_back(fanInGate);
         Map[fanInId[0]] = fanInGate;
      } else {
         fanInGate = Map[fanInId[0]];
      }
      isflo = (fanInGate->gateType == UNDEF_GATE);
      thisGate->fanIn.push_back(fanInGate);
      thisGate->fanIn_inv.push_back(aig[i].first & 1);
      thisGate->fanIn_flo.push_back(isflo);

      fanInGate->fanOut.push_back(thisGate);
      fanInGate->fanOut_inv.push_back(aig[i].first & 1);
      // handle fanIn2
      if(Map.find(fanInId[1]) == Map.end()) { // UNDEF_GATE
         fanInGate = new CirGate(UNDEF_GATE, fanInId[1], 0, 0);
         CirCuit.push_back(fanInGate);
         Map[fanInId[1]] = fanInGate;
      } else {
         fanInGate = Map[fanInId[1]];
      }
      isflo = (fanInGate->gateType == UNDEF_GATE);
      thisGate->fanIn.push_back(fanInGate);
      thisGate->fanIn_inv.push_back(aig[i].second & 1);
      thisGate->fanIn_flo.push_back(isflo);

      fanInGate->fanOut.push_back(thisGate);
      fanInGate->fanOut_inv.push_back(aig[i].second & 1);
   }

   return true;
}
void
CirMgr::genNetList(CirGate *gate, unordered_set<CirGate *>& visted)
{
   if (visted.find(gate) != visted.end()) // 已經遍歷過了
      return;
   if (gate->gateType == UNDEF_GATE)
      return;
   
   if (gate->gateType == PO_GATE) {
      genNetList(gate->fanIn[0], visted);
   } else if (gate->gateType == AIG_GATE) {
      genNetList(gate->fanIn[0], visted);
      genNetList(gate->fanIn[1], visted);
   }

   visted.insert(gate);
   netList.push_back(gate);
   return;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
   int PI_num = MILOA[1];
   int PO_num = MILOA[3];
   int AIG_num = MILOA[4];
   int Total = PI_num + PO_num + AIG_num;

   int PI_digit = to_string(PI_num).size();
   int PO_digit = to_string(PO_num).size();
   int AIG_digit = to_string(AIG_num).size();
   int Total_digit = to_string(Total).size();

   cout << endl;
   cout << "Circuit Statistics" << endl;
   cout << "==================" << endl;
   cout << "  PI";
   for (int i = 0; i < 12 - PI_digit; i++)
      cout << " ";
   cout << PI_num << endl;
   cout << "  PO";
   for (int i = 0; i < 12 - PO_digit; i++)
      cout << " ";
   cout << PO_num << endl;
   cout << "  AIG";
   for (int i = 0; i < 11 - AIG_digit; i++)
      cout << " ";
   cout << AIG_num << endl;
   cout << "------------------" << endl;
   cout << "  Total";
   for (int i = 0; i < 9 - Total_digit; i++)
      cout << " ";
   cout << Total << endl;
}

void
CirMgr::printNetlist() const
{
   int nsize = netList.size();

   cout << endl;
   for (int i = 0; i < nsize; i++) {
      cout << "[" << i << "] ";
      cout << *netList[i];
   }
}

void
CirMgr::printPIs() const
{
   cout << "PIs of the circuit:";
   for (int i = 1; i <= MILOA[1]; i++) 
      cout << " " << CirCuit[i]->gateId;
   cout << endl;
}

void
CirMgr::printPOs() const
{
   cout << "POs of the circuit:";
   for (int i = 1; i <= MILOA[3]; i++)
      cout << " " << CirCuit[MILOA[1] + i]->gateId;
   cout << endl;
}

void
CirMgr::printFloatGates() const
{
   int size;
   if (size = with_float.size()) {
      cout << "Gates with floating fanin(s):";
      for (int i = 0; i < size; i++)
         cout << " " << with_float[i];
      cout << endl;
   }
   if (size = def_notused.size()) {
      cout << "Gates defined but not used  :";
      for (int i = 0; i < size; i++)
         cout << " " << def_notused[i];
      cout << endl;
   }
}
// TODO
void
CirMgr::printFECPairs() const
{  
   int fecSize = fecGroup.size();
   for (int i = 0; i < fecSize; ++i) {
      int groupSize = fecGroup[i].size();
      assert(groupSize > 1);
      bitset<PatternSIZE> compValue = cirMgr->Map[fecGroup[i][0]]->gateValue;

      cout << "[" << i << "]";
      for (int j = 0; j < groupSize; ++j) {
         int curId = fecGroup[i][j];
         cout << ' ';
         if (cirMgr->Map[curId]->gateValue != compValue) cout << '!';
         cout << curId;
      }
      cout << endl;
   }
}

void
CirMgr::writeAag(ostream& outfile) const
{
   #define pii pair<int,int>
   #define piii pair<int,pair<int,int>>
   vector<string> pi_sym;
   vector<string> po_sym;
   vector<piii> aig;
   int lb, hb;

   // traverse netlist to get symbol & aig
   int nsize = netList.size();
   for (int i = 0; i < nsize; i++) {
      if (netList[i]->gateType == AIG_GATE) {
         int n1 = 2*netList[i]->gateId;
         int n2 = 2*netList[i]->fanIn[0]->gateId
                  + (int)netList[i]->fanIn_inv[0];
         int n3 = 2*netList[i]->fanIn[1]->gateId
                  + (int)netList[i]->fanIn_inv[1];
         aig.push_back(piii(n1, pii(n2, n3)));
      }
   }

   outfile << "aag";
   for (int i = 0; i < 4; i++)
      outfile << " " << MILOA[i];
   outfile << " " << aig.size();
   outfile << endl;

   // pi
   for (int i = 1; i <= MILOA[1]; i++) {
      outfile << 2*CirCuit[i]->gateId << endl;
      // pi_sym
      if (!CirCuit[i]->name.empty())
         pi_sym.push_back(CirCuit[i]->name);
   }

   // po
   lb = MILOA[1] + 1;
   hb = MILOA[1] + 1 + MILOA[3];
   for (int i = lb; i < hb; i++) {
      int po_value = 2*CirCuit[i]->fanIn[0]->gateId
                  + (int)CirCuit[i]->fanIn_inv[0];
      outfile << po_value << endl;
      // po_sym
      if (!CirCuit[i]->name.empty())
         po_sym.push_back(CirCuit[i]->name);
   }

   // aig
   nsize = aig.size();
   for (int i = 0; i < nsize; i++) {
      outfile << aig[i].first << " ";
      outfile << aig[i].second.first << " ";
      outfile << aig[i].second.second << endl;
   }

   nsize = pi_sym.size();
   for (int i = 0; i < nsize; i++)
      outfile << "i" << i << " " << pi_sym[i] << endl;

   nsize = po_sym.size();
   for (int i = 0; i < nsize; i++)
      outfile << "o" << i << " " << po_sym[i] << endl;

   outfile << "c" << endl;
   outfile << "AAG output by EricTT" << endl;
}
// TODO
void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
   
   outfile << "c" << endl;
   outfile << "Write gate" << g->gateId << " output by EricTT" << endl;
}
