#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"

namespace smack {

class TagSlicAssume: public llvm::FunctionPass {
public:
  static char ID;
  TagSlicAssume() : llvm::FunctionPass(ID) {}
  virtual bool runOnFunction(llvm::Function& F);

private:
  unsigned getInstLine(llvm::Instruction* i);
  void tag(llvm::Instruction* i, unsigned line);
};

}
