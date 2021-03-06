#ifndef _SRC_X64_MARCOASSEMBLER_H_
#define _SRC_X64_MARCOASSEMBLER_H_

#include "assembler-x64.h"
#include "assembler-x64-inl.h"
#include "ast.h" // AstNode
#include "heap.h" // HeapValue

namespace candor {

// Forward declaration
class BaseStub;
class Stubs;

class Masm : public Assembler {
 public:
  Masm(Heap* heap);

  // Save/restore all valuable register
  void Pushad();
  void Popad(Register preserve);

  class Align {
   public:
    Align(Masm* masm);
    ~Align();
   private:
    Masm* masm_;
    int32_t align_;
  };

  // Skip some bytes to make code aligned
  void AlignCode();

  // Alignment helpers
  inline void ChangeAlign(int32_t slots) { align_ += slots; }

  // Allocate some space in heap's new space current page
  // Jmp to runtime_allocate label on exhaust or fail
  void Allocate(Heap::HeapTag tag,
                Register size_reg,
                uint32_t size,
                Register result);

  // Allocate context and function
  void AllocateContext(uint32_t slots);
  void AllocateFunction(Register addr, Register result);

  // Allocate heap numbers
  void AllocateNumber(DoubleRegister value, Register result);

  // Allocate boolean value, `value` should be either 0 or 1
  void AllocateBoolean(Register value, Register result);

  // Allocate heap string (symbol)
  void AllocateString(const char* value, uint32_t length, Register result);

  // Allocate object&map
  void AllocateObjectLiteral(Register size, Register result);

  // Fills memory segment with immediate value
  void Fill(Register start, Register end, Immediate value);

  // Fill stack slots with nil
  void FillStackSlots(uint32_t slots);

  void IsNil(Register reference, Label* not_nil, Label* is_nil);
  void IsUnboxed(Register reference, Label* not_unboxed, Label* unboxed);

  // Checks if object has specific type
  void IsHeapObject(Heap::HeapTag tag,
                    Register reference,
                    Label* mismatch,
                    Label* match);
  void IsTrue(Register reference, Label* is_false, Label* is_true);

  // Unboxing routines
  void UnboxNumber(Register number);

  // Store stack pointer into heap
  void StoreRootStack();

  // Runtime errors
  void Throw(Heap::Error error);

  // Sets correct environment and calls function
  void Call(Register addr);
  void Call(Operand& addr);
  void Call(Register fn, uint32_t args);
  void Call(BaseStub* stub);

  inline void Push(Register src);
  inline void Pop(Register src);
  inline void PushTagged(Register src);
  inline void PopTagged(Register src);
  inline void PreservePop(Register src, Register preserve);
  inline void Save(Register src);
  inline void Restore(Register src);
  inline void Result(Register src);
  inline uint64_t TagNumber(uint64_t number);
  inline void TagNumber(Register src);
  inline void Untag(Register src);

  // See VisitForSlot and VisitForValue in fullgen for disambiguation
  inline Register result() { return result_; }
  inline Operand* slot() { return slot_; }
  inline Heap* heap() { return heap_; }
  inline Stubs* stubs() { return stubs_; }

  Register result_;
  Operand* slot_;

 protected:
  Heap* heap_;
  Stubs* stubs_;

  int32_t align_;

  friend class Align;
};

} // namespace candor

#endif // _SRC_X64_MARCOASSEMBLER_H_
