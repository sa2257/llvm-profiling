// Initially taken from llvm-pass-skeleton/skeleton/Skeleton.cpp at https://github.com/sampsyo/llvm-pass-skeleton.git and later modified by Sachille Atapattu

/* Singleton: Combines Stilton which allocates and Shelton with checks dependencies
 * to make a placement pass */

#include "llvm/Pass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <string.h>
#include <vector>
#include <fstream>
#include <list>

#include "llvm/Support/CommandLine.h"

using namespace std;
using namespace llvm;

static cl::opt<string> InputModule("function", cl::desc("Specify function to run pass"), cl::value_desc("module"));
static cl::opt<bool> SwitchOn("select", cl::desc("Select to run pass"));
std::ofstream outfile; // Only for static information of course! 

namespace {
  struct SingletonPass : public ModulePass {
      static char ID;
      SingletonPass() : ModulePass(ID) {}
      
      virtual bool runOnModule(Module &M); //when there is a Module
      virtual bool runOnFunction(Function &F, Module &M); //called by runOnModule
      virtual bool runSorting(Function &F, std::list<Instruction*> Used);
      virtual bool runScheduling(std::list<Instruction*> &Sorted, std::list<string> &Schedule);
      virtual bool runMapping(std::list<string> &mapping);
      
      virtual int  getSizeFunction(Function &F);
      virtual bool checkInstrNotInList(std::list<Instruction*> List, Instruction &I);
      virtual bool checkInstrDependence(std::list<Instruction*> List, Instruction &I);
      virtual int  runDepthSearch(Instruction &I, int depth, Instruction &init);
      
      int d_ratio = 1; int m_ratio = 1; int a_ratio = 1; int l_ratio = 1;
      int f_ratio = 1; int i_ratio = 3;
      int dmal_total = d_ratio + m_ratio + a_ratio + l_ratio;
      int fi_total = f_ratio + i_ratio;
      int pe_len = 8;
  };
}

bool SingletonPass::runOnModule(Module &M)
{
    errs() << "Pass run status: " << SwitchOn << "\n";
    errs() << "Pass running on: " << InputModule << "\n";
    outfile.open("output.txt", std::ios_base::app);
    outfile << "Pass run status: " << SwitchOn << "\n";
    outfile << "Pass running on: " << InputModule << "\n";
    
    int idiv_pes = pe_len * pe_len * d_ratio * i_ratio / dmal_total / fi_total;
    int imul_pes = pe_len * pe_len * m_ratio * i_ratio / dmal_total / fi_total;
    int iari_pes = pe_len * pe_len * a_ratio * i_ratio / dmal_total / fi_total;
    int ilog_pes = pe_len * pe_len * l_ratio * i_ratio / dmal_total / fi_total;
    int fdiv_pes = pe_len * pe_len * d_ratio * f_ratio / dmal_total / fi_total;
    int fmul_pes = pe_len * pe_len * m_ratio * f_ratio / dmal_total / fi_total;
    int fari_pes = pe_len * pe_len * a_ratio * f_ratio / dmal_total / fi_total;
    int flog_pes = pe_len * pe_len * l_ratio * f_ratio / dmal_total / fi_total;
    int mem_units = (pe_len + 1) * (pe_len + 1);
    outfile << "Available resources: " << 
        idiv_pes << "," << imul_pes << "," << iari_pes << "," <<
        fdiv_pes << "," << fmul_pes << "," << fari_pes << "," <<
        ilog_pes << "," << flog_pes << "," << mem_units << "\n" ;
    
    for (auto &func: M) {
        bool modified = runOnFunction(func, M);
    }
    
    return true;
}

bool SingletonPass::runOnFunction(Function &F, Module &M)
{
    bool modified = false;
    //string funcName = F.getName();

    if (SwitchOn || F.getName() == InputModule) {
        std::list<Instruction*> Sorted;
        bool sorted    = runSorting(F, Sorted);

        std::list<string> Schedule;
        bool scheduled = runScheduling(Sorted, Schedule);


        bool placement = runMapping(Schedule);
        modified = true;
    }

    return modified;
}

bool SingletonPass::runSorting(Function &F, std::list<Instruction*> Used) { 
  int usedArr[getSizeFunction(F)][3];
  memset(usedArr, -1, sizeof(usedArr));
  int height = 0;
  while (Used.size() < getSizeFunction(F)) {
    std::list<Instruction*> Deps; 
    int ins = 0;
    for (auto &B: F) {
        for (auto &I: B) {
            if (checkInstrNotInList(Used, I)) {
              Deps.push_back(&I);
              if (checkInstrDependence(Deps, I)) {
                Used.push_back(&I);
                int depth = 1;
                depth = runDepthSearch(I, depth, I);
                usedArr[ins][0] = Used.size();
                usedArr[ins][1] = height;
                usedArr[ins][2] = depth;
              }
            }
            ins = ins + 1;
        }
    }
    height = height + 1;
  }
  return true;
}

bool SingletonPass::runScheduling(std::list<Instruction*> &Sorted, std::list<string> &Schedule)
{
    // available pe resources.
    int idiv_pes = pe_len * pe_len * d_ratio * i_ratio / dmal_total / fi_total;
    int imul_pes = pe_len * pe_len * m_ratio * i_ratio / dmal_total / fi_total;
    int iari_pes = pe_len * pe_len * a_ratio * i_ratio / dmal_total / fi_total;
    int ilog_pes = pe_len * pe_len * l_ratio * i_ratio / dmal_total / fi_total;
    int fdiv_pes = pe_len * pe_len * d_ratio * f_ratio / dmal_total / fi_total;
    int fmul_pes = pe_len * pe_len * m_ratio * f_ratio / dmal_total / fi_total;
    int fari_pes = pe_len * pe_len * a_ratio * f_ratio / dmal_total / fi_total;
    int flog_pes = pe_len * pe_len * l_ratio * f_ratio / dmal_total / fi_total;
    int mem_units = (pe_len + 1) * (pe_len + 1);
    bool fail = false;
    
    for (auto it: Sorted) {
        switch (it->getOpcode()) {
            case Instruction::Add: // addition
                iari_pes--;
                Schedule.push_back("arith");
                continue;
            case Instruction::Sub: // subtraction
                iari_pes--;
                Schedule.push_back("arith");
                continue;
            case Instruction::Mul: // multiplication
                imul_pes--;
                Schedule.push_back("mul");
                continue;
            case Instruction::UDiv: // division unsigned
            case Instruction::SDiv: // division signed
                idiv_pes--;
                Schedule.push_back("div");
                continue;
            case Instruction::URem: // remainder unsigned
            case Instruction::SRem: // remainder signed
                idiv_pes--;
                Schedule.push_back("div");
                continue;
            case Instruction::FAdd: // fp addition
                fari_pes--;
                Schedule.push_back("farith");
                continue;
            case Instruction::FSub: // fp subtraction
                fari_pes--;
                Schedule.push_back("farith");
                continue;
            case Instruction::FMul: // fp multiplication
                fmul_pes--;
                Schedule.push_back("fmul");
                continue;
            case Instruction::FDiv: // fp division
                fdiv_pes--;
                Schedule.push_back("fdiv");
                continue;
            case Instruction::FRem: // fp remainder
                fdiv_pes--;
                Schedule.push_back("fdiv");
                continue;
            case Instruction::And: // and
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::Or: // or
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::Xor: // xor
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::Trunc: // truncate
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::ZExt: // zero extend
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::SExt: // sign extend
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::Shl: // shift left
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::LShr: // logic shift right
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::AShr: // arith shift right
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::ICmp: // integer compare
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::PHI: // PHI node
                ilog_pes--;
                Schedule.push_back("logic");
                continue;
            case Instruction::FCmp: // fp compare
                flog_pes--;
                Schedule.push_back("flogic");
                continue;
            case Instruction::FPTrunc: // fp truncate
                flog_pes--;
                Schedule.push_back("flogic");
                continue;
            case Instruction::FPExt: // fp extend
                flog_pes--;
                Schedule.push_back("flogic");
                continue;
            case Instruction::FPToUI: // fp to unsigned int
                flog_pes--;
                Schedule.push_back("flogic");
                continue;
            case Instruction::FPToSI: // fp to signed int
                flog_pes--;
                Schedule.push_back("flogic");
                continue;
            case Instruction::UIToFP: // unsigned int to fp
                flog_pes--;
                Schedule.push_back("flogic");
                continue;
            case Instruction::SIToFP: // signed int to fp
                flog_pes--;
                Schedule.push_back("flogic");
                continue;
            case Instruction::Br: // branch
                Schedule.push_back("control");
                continue;
            case Instruction::Switch: // switch
                Schedule.push_back("control");
                continue;
            case Instruction::Store: // store
                mem_units--;
                Schedule.push_back("reg");
                continue;
            case Instruction::Load: // load
                mem_units--;
                Schedule.push_back("reg");
                continue;
            case Instruction::Call: // function call
                mem_units--;
                Schedule.push_back("control");
                continue;
            case Instruction::Alloca: // Ignore allocate stack
                continue;
            case Instruction::GetElementPtr: // Ignore get address
                Schedule.push_back("memory");
                continue;
            case Instruction::BitCast: // Ignore convert type without bit change
                continue;
            case Instruction::Ret: // Ignore return
                Schedule.push_back("control");
                continue;
            default:
                errs() << "Can't map bb, instruction " << it->getOpcodeName() << " not supported!" << "\n";
                fail = true;
                break;
        }
    }

    if(idiv_pes < 0) {
        errs() << "Can't map bb, out of int-division resources!" << "\n";
    } else if(imul_pes < 0) {
        errs() << "Can't map bb, out of int-multiply resources!" << "\n";
    } else if(iari_pes < 0) {
        errs() << "Can't map bb, out of int-arithmetic resources!" << "\n";
    } else if(fdiv_pes < 0) {
        errs() << "Can't map bb, out of fp-division resources!" << "\n";
    } else if(fmul_pes < 0) {
        errs() << "Can't map bb, out of fp-multiply resources!" << "\n";
    } else if(fari_pes < 0) {
        errs() << "Can't map bb, out of fp-arithmetic resources!" << "\n";
    } else if(ilog_pes < 0) {
        errs() << "Can't map bb, out of logic resources!" << "\n";
    } else if(flog_pes < 0) {
        errs() << "Can't map bb, out of fp-compare resources!" << "\n";
    } else if(mem_units < 0) {
        errs() << "Can't map bb, out of registers!" << "\n";
    }
    
    // utilization of pe resources.
    idiv_pes = pe_len * pe_len * d_ratio * i_ratio / dmal_total / fi_total - idiv_pes;
    imul_pes = pe_len * pe_len * m_ratio * i_ratio / dmal_total / fi_total - imul_pes;
    iari_pes = pe_len * pe_len * a_ratio * i_ratio / dmal_total / fi_total - iari_pes;
    ilog_pes = pe_len * pe_len * l_ratio * i_ratio / dmal_total / fi_total - ilog_pes;
    fdiv_pes = pe_len * pe_len * d_ratio * f_ratio / dmal_total / fi_total - fdiv_pes;
    fmul_pes = pe_len * pe_len * m_ratio * f_ratio / dmal_total / fi_total - fmul_pes;
    fari_pes = pe_len * pe_len * a_ratio * f_ratio / dmal_total / fi_total - fari_pes;
    flog_pes = pe_len * pe_len * l_ratio * f_ratio / dmal_total / fi_total - flog_pes;
    mem_units = (pe_len + 1) * (pe_len + 1) - mem_units;
    if (!fail) {
        outfile << "Global schedule passed with " << 
            idiv_pes << "," << imul_pes << "," << iari_pes << "," <<
            fdiv_pes << "," << fmul_pes << "," << fari_pes << "," <<
            ilog_pes << "," << flog_pes << "," << mem_units << "\n" ;
    }
    
    return true;
}

bool SingletonPass::runMapping(std::list<string> &mapping) {
    int slcUnits  = dmal_total * fi_total;
    int noOfSlcs  = (pe_len * pe_len) / slcUnits; // assert this is int
    int insToMap  = mapping.size();

    bool mapped = false;
    int nodeUnits = dmal_total + 1 * fi_total + 1;
    int placement[noOfSlcs][dmal_total][fi_total];
    int nodes[noOfSlcs][nodeUnits];
    memset(placement, -1, sizeof(placement));
    memset(nodes, -1, sizeof(nodes));
    
    int i = 0;
    for (int s = 0; s < noOfSlcs; s++) {
        int idivUnits = 0;
        int imulUnits = 0;
        int iariUnits = 0;
        int ilogUnits = 0;
        int fdivUnits = 0;
        int fmulUnits = 0;
        int fariUnits = 0;
        int flogUnits = 0;
        int nodeUnit = 0;
        
        bool sliceSkip = false;
        while(i < insToMap) {
            string S = mapping.front();
            mapping.pop_front();
            outfile << i << ": " << S << "\n";
            
            if (S == "arith") {
                if (iariUnits == 3) {
                    // errs() << "Ran out of int arith units to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    placement[s][iariUnits][1] = i;
                }
                iariUnits++;
            } else if (S == "mul") {
                if (imulUnits == 3) {
                    // errs() << "Ran out of int mul units to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    placement[s][imulUnits][2] = i;
                }
                imulUnits++;
            } else if (S == "div") {
                if (idivUnits == 3) {
                    // errs() << "Ran out of int div units to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    placement[s][idivUnits][3] = i;
                }
                idivUnits++;
            } else if (S == "logic") {
                if (ilogUnits == 3) {
                    // errs() << "Ran out of int logic units to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    placement[s][ilogUnits][0] = i;
                }
                ilogUnits++;
            } else if (S == "farith") {
                if (fariUnits == 1) {
                    // errs() << "Ran out of fp arith units to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    placement[s][3][1] = i;
                }
                fariUnits++;
            } else if (S == "fmul") {
                if (fmulUnits == 1) {
                    // errs() << "Ran out of fp mul units to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    placement[s][3][2] = i;
                }
                fmulUnits++;
            } else if (S == "fdiv") {
                if (fdivUnits == 1) {
                    // errs() << "Ran out of fp div units to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    placement[s][3][3] = i;
                }
                fdivUnits++;
            } else if (S == "flogic") {
                if (flogUnits == 1) {
                    // errs() << "Ran out of fp logic units to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    placement[s][3][0] = i;
                }
                flogUnits++;
            } else if (S == "control") {
                if (nodeUnit == nodeUnits) {
                    // errs() << "Ran out of nodes to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                   nodes[s][nodeUnit] = i;
                }
                nodeUnit++;
            } else if (S == "reg") {
                if (nodeUnit == nodeUnits) {
                    // errs() << "Ran out of nodes to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    nodes[s][nodeUnit] = i;
                }
                nodeUnit++;
            } else if (S == "memory") {
                if (nodeUnit == nodeUnits) {
                    // errs() << "Ran out of nodes to map!\n";
                    mapping.push_front(S);
                    sliceSkip = true;
                } else {
                    nodes[s][nodeUnit] = i;
                }
                nodeUnit++;
            }

            if (sliceSkip) {
                break;
            }
        
            i++;
        }
        
        if (i == insToMap) {
            break;
        }
    }

    if (i < insToMap) {
        errs() << "Ran out of fabric before mapping!\n";
        mapped = false;
    } else {
        for (int s = 0; s < noOfSlcs; s++) {
            outfile << "Slice: " << s << "\n";
            for (int j = 0; j < dmal_total; j++) {
                for (int k = 0; k < fi_total; k++) {
                    outfile << placement[s][j][k] << ", ";
                }
                outfile << "\n";
            }
            outfile << "\n";
        }
        mapped = true;
    }

    return mapped;
}

/* Support Functions */

int SingletonPass::getSizeFunction(Function &F) {
  int size = 0; 
  for (auto &B: F) {
      size = size + B.size();
  }
  return size;
}

bool SingletonPass::checkInstrNotInList(std::list<Instruction*> List, Instruction &I) {
    for (auto L: List) {
        if (L == &I) {
            return false;
        }
    }
    return true;
}

bool SingletonPass::checkInstrDependence(std::list<Instruction*> List, Instruction &I) {
    for (auto L: List) {
        for (auto& U : L->uses()) {
            User* user = U.getUser();
            if (user == &I) {
                return false;
            }
        }
    }
    return true;
}

int SingletonPass::runDepthSearch(Instruction &I, int depth, Instruction &init) {
  int initDepth = depth;
  int maxDepth = depth; // if no uses return input depth
  for (auto& U : I.uses()) {
      depth = initDepth;
      User* user = U.getUser();  // Find all the places I is used
      if (isa<Instruction>(user)){
          Instruction* ins = cast<Instruction>(user);
          depth++;
          if (ins == &init) {
            depth++;
          } else if (ins->getOpcode() == Instruction::PHI) {
            depth++;
          } else if (depth > 15) {
            errs() << "Uncaptured possibly cyclic behaviour!\n";
            errs() << "Instruction: " << I << " , next: " << *ins << " , start: " << init << "\n";
          } else {
            depth = runDepthSearch(*ins, depth, init);
          }
      } else {
          errs() << "Not an instruction!\n";
          depth++;
      }
      if (depth > maxDepth) {
          maxDepth = depth;
      }
  }
  return maxDepth;
}

char SingletonPass::ID = 0;

// Register the pass so `opt -singleton` runs it.
static RegisterPass<SingletonPass> X("singleton", "a simple placement pass");
