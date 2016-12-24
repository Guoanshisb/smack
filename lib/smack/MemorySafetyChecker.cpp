//
// This file is distributed under the MIT License. See LICENSE for details.
//
#include "smack/MemorySafetyChecker.h"
#include "smack/Naming.h"
#include "smack/SmackOptions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

namespace smack {

using namespace llvm;

void insertMemoryLeakCheck(Function& F, Module& m) {
  Function* memoryLeakCheckFunction = m.getFunction(Naming::MEMORY_LEAK_FUNCTION);
  assert (memoryLeakCheckFunction != NULL && "Memory leak check function must be present.");
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    if (isa<ReturnInst>(&*I)) {
      CallInst::Create(memoryLeakCheckFunction, "", &*I);
    }
  }
}

Value* stripPointerCasts(Value* v) {
  if (auto ci = dyn_cast<CastInst>(v)) {
    if (ci->getSrcTy()->isPointerTy() && ci->getDestTy()->isPointerTy())
      return ci->getOperand(0);
  }
  return v;
}

void inserMemoryAccessCheck(Value* memoryPointer, Instruction* I, DataLayout* dataLayout, Function* memorySafetyFunction, Function* F) {
  if (auto gep = dyn_cast<GetElementPtrInst>(stripPointerCasts(memoryPointer))) {
    Value* base = stripPointerCasts(gep->getPointerOperand());
    if (auto ci = dyn_cast<CallInst>(base)) {
      Function* f = ci->getCalledFunction();
      if (f->hasName() && f->getName() == "malloc") {
        assert(ci->getNumArgOperands() == 1 && "Malloc has only one argument");
        if (auto arg = dyn_cast<ConstantInt>(ci->getArgOperand(0))) {
          uint64_t size = arg->getZExtValue();
          int64_t offset = 0;
          bool allConst = true;
          gep_type_iterator T = gep_type_begin(gep);
          for (unsigned i = 1; i < gep->getNumOperands(); ++i, ++T) {
            if (StructType* st = dyn_cast<StructType>(*T)) {
              uint64_t fieldNo = dyn_cast<ConstantInt>(gep->getOperand(i))->getZExtValue();
              offset += dataLayout->getStructLayout(st)->getElementOffset(fieldNo);
            } else {
              Type* et = dyn_cast<SequentialType>(*T)->getElementType();
              if (auto cint = dyn_cast<ConstantInt>(gep->getOperand(i))) {
                offset += cint->getSExtValue() * dataLayout->getTypeStoreSize(et);
              } else {
                allConst = false;
                break;
              }
            }
          }
          if (allConst) {
            if (offset < 0 || offset + dataLayout->getTypeStoreSize(cast<PointerType>(memoryPointer->getType())->getPointerElementType()) > size)
              inserMemoryAccessCheck(ConstantPointerNull::get(PointerType::getUnqual(IntegerType::getInt8Ty(F->getContext()))), I, dataLayout, memorySafetyFunction, F);
              return;
          }
        }
      }
    }
  }
  // Finding the exact type of the second argument to our memory safety function
  Type* sizeType = memorySafetyFunction->getFunctionType()->getParamType(1);
  PointerType* pointerType = cast<PointerType>(memoryPointer->getType());
  uint64_t storeSize = dataLayout->getTypeStoreSize(pointerType->getPointerElementType());
  Value* size = ConstantInt::get(sizeType, storeSize);
  Type *voidPtrTy = PointerType::getUnqual(IntegerType::getInt8Ty(F->getContext()));
  CastInst* castPointer = CastInst::Create(Instruction::BitCast, memoryPointer, voidPtrTy, "", &*I);
  Value* args[] = {castPointer, size};
  CallInst::Create(memorySafetyFunction, ArrayRef<Value*>(args, 2), "", &*I);
}

bool MemorySafetyChecker::runOnModule(Module& m) {
  DataLayout* dataLayout = new DataLayout(&m);
  Function* memorySafetyFunction = m.getFunction(Naming::MEMORY_SAFETY_FUNCTION);
  assert(memorySafetyFunction != NULL && "Memory safety function must be present.");
  for (auto& F : m) {
    if (!Naming::isSmackName(F.getName())) {
      if (SmackOptions::isEntryPoint(F.getName())) {
        insertMemoryLeakCheck(F, m);
      }
      for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        if (LoadInst* li = dyn_cast<LoadInst>(&*I)) {
          inserMemoryAccessCheck(li->getPointerOperand(), &*I, dataLayout, memorySafetyFunction, &F);
        } else if (StoreInst* si = dyn_cast<StoreInst>(&*I)) {
          inserMemoryAccessCheck(si->getPointerOperand(), &*I, dataLayout, memorySafetyFunction, &F);
        } else if (MemSetInst* memseti = dyn_cast<MemSetInst>(&*I)) {
	    Value* dest = memseti->getDest();
	    Value* size = memseti->getLength();
	    Type* voidPtrTy = PointerType::getUnqual(IntegerType::getInt8Ty(F.getContext()));
	    CastInst* castPtr = CastInst::Create(Instruction::BitCast, dest, voidPtrTy, "", &*I);
	    CallInst::Create(memorySafetyFunction, {castPtr, size}, "", &*I);
        } else if (MemTransferInst* memtrni = dyn_cast<MemTransferInst>(&*I)) {
            // MemTransferInst is abstract class for both MemCpyInst and MemMoveInst
	    Value* dest = memtrni->getDest();
	    Value* src = memtrni->getSource();
	    Value* size = memtrni->getLength();
	    Type* voidPtrTy = PointerType::getUnqual(IntegerType::getInt8Ty(F.getContext()));
	    CastInst* castPtrDest = CastInst::Create(Instruction::BitCast, dest, voidPtrTy, "", &*I);
	    CastInst* castPtrSrc = CastInst::Create(Instruction::BitCast, src, voidPtrTy, "", &*I);
	    CallInst::Create(memorySafetyFunction, {castPtrDest, size}, "", &*I);  
	    CallInst::Create(memorySafetyFunction, {castPtrSrc, size}, "", &*I);  
	}
      }
    }
  }
  return true;
}

// Pass ID variable
char MemorySafetyChecker::ID = 0;
}

