#include "smack/Naming.h"
#include "smack/TagSlicAssume.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Metadata.h"
#include <set>

namespace smack {

using namespace llvm;

bool TagSlicAssume::runOnFunction(Function& F) {
  std::set<unsigned> assertLines;

  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (CallInst* ci = dyn_cast<CallInst>(&*I)) {
      Function* func = ci->getCalledFunction();
      if (func && func->hasName() && func->getName().str() == "__VERIFIER_assert") {
        //assertedVals.push_back(stripCastAndNeg(ci->getArgOperand(0));
        unsigned line = getInstLine(ci);
        if (line != 0)
          assertLines.insert(line);
      }
    }
  }

  if (!assertLines.empty()) {
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (BranchInst* bi = dyn_cast<BranchInst>(&*I)) {
        unsigned line = getInstLine(bi);
        if (bi->isConditional() && assertLines.find(line) != assertLines.end()) {
          tag(bi, line);
        }
      }
    }
  }

  /*
  while (!assertedVals.empty()) {
    Value* assertedVal = assertedVals.front();
    assertedVals.pop_front();
    if (PHINode* phi = dyn_cast<PHINode>(assertedVal)) {
      for (unsigned i = 0; i < getNumIncomingValues(); ++i) {
        Value* iv = phi->getIncomingValue(i);
        BasicBlock* ib = phi->getIncomingBlock(i);
        if (isTruthValue(iv) || isComparison(iv))
          annotateBranch(ib);
        else
      }
    }
  }
  */
  return true;
}

/*
Value* stripCastAndNeg(Value* v) {
  if (ZExtInst* zi = dyn_cast<ZExtInst>(v))
    v = zi->getOperand(0);
  if (BinaryOperator::isNot(v))
    v = BinaryOperator::getNegArgument(v);
}

bool isTruthValue(Value* v) {
  if (ConstantInt* ci = dyn_cast<ConstantInt>(v))
    return ci->getBitWidth == 1;
  else
    return false;
}

bool isComparison(Value* v) {
  return isa<CompInst>(v);
}
*/

unsigned TagSlicAssume::getInstLine(Instruction* i) {
  if (i->getMetadata("dbg"))
    return i->getDebugLoc().getLine();
  else
    return 0U;
}

void TagSlicAssume::tag(Instruction* i, unsigned line) {
  auto& C = i->getContext();
  i->setMetadata(Naming::AV_SLIC_ASSUME,
                 MDNode::get(C, ConstantAsMetadata::get(
                  ConstantInt::get(IntegerType::get(C, 32U), line))));
}

char TagSlicAssume::ID = 0;
}
