#include "stubs.h"
#include "ast.h" // BinOp
#include "macroassembler-x64.h" // Masm
#include "runtime.h"

namespace candor {

#define __ masm()->

BaseStub::BaseStub(Masm* masm, StubType type) : FFunction(masm),
                                                type_(type) {
}


void BaseStub::GeneratePrologue() {
  __ push(rbp);
  __ movq(rbp, rsp);
}


void BaseStub::GenerateEpilogue(int args) {
  __ movq(rsp, rbp);
  __ pop(rbp);

  // tag + size
  __ ret(args * 8);
}


void AllocateStub::Generate() {
  GeneratePrologue();
  // Align stack
  __ subq(rsp, Immediate(8));
  __ push(rbx);

  // Arguments
  Operand size(rbp, 24);
  Operand tag(rbp, 16);

  Label runtime_allocate(masm()), done(masm());

  Heap* heap = masm()->heap();
  Immediate heapref(reinterpret_cast<uint64_t>(heap));
  Immediate top(reinterpret_cast<uint64_t>(heap->new_space()->top()));
  Immediate limit(reinterpret_cast<uint64_t>(heap->new_space()->limit()));

  Operand scratch_op(scratch, 0);

  // Get pointer to current page's top
  // (new_space()->top() is a pointer to space's property
  // which is a pointer to page's top pointer
  // that's why we are dereferencing it here twice
  __ movq(scratch, top);
  __ movq(scratch, scratch_op);
  __ movq(rax, scratch_op);
  __ movq(rbx, size);
  __ Untag(rbx);

  // Add object size to the top
  __ addq(rbx, rax);
  __ jmp(kCarry, &runtime_allocate);

  // Check if we exhausted buffer
  __ movq(scratch, limit);
  __ movq(scratch, scratch_op);
  __ cmpq(rbx, scratch_op);
  __ jmp(kGt, &runtime_allocate);

  // We should allocate only even amount of bytes
  Label already_even(masm());

  __ testb(rbx, Immediate(0x01));
  __ jmp(kEq, &already_even);

  // Add one byte
  __ inc(rbx);

  __ bind(&already_even);

  // Update top
  __ movq(scratch, top);
  __ movq(scratch, scratch_op);
  __ movq(scratch_op, rbx);

  __ jmp(&done);

  // Invoke runtime allocation stub (and probably GC)
  __ bind(&runtime_allocate);

  // Remove junk from registers
  __ xorq(rax, rax);
  __ xorq(rbx, rbx);

  RuntimeAllocateCallback allocate = &RuntimeAllocate;

  {
    Label call(masm());

    Masm::Align a(masm());
    __ Pushad();

    // Three arguments: heap, size, top_stack
    __ movq(rdi, heapref);
    __ movq(rsi, size);
    __ movq(rdx, rsp);

    // Objects are allocated in pairs with maps
    // do not let GC run in the middle of the process
    __ cmpb(tag, Immediate(masm()->TagNumber(Heap::kTagMap)));
    __ jmp(kNe, &call);
    __ xorq(rdx, rdx);

    __ bind(&call);
    __ movq(scratch, Immediate(*reinterpret_cast<uint64_t*>(&allocate)));

    __ callq(scratch);
    __ Popad(rax);
  }

  // Voila result and result_end are pointers
  __ bind(&done);

  // Set tag
  Operand qtag(rax, 0);
  __ movq(scratch, tag);
  __ Untag(scratch);
  __ movq(qtag, scratch);

  // Rax will hold resulting pointer
  __ pop(rbx);
  GenerateEpilogue(2);
}


void ThrowStub::Generate() {
  Immediate pending_exception(
      reinterpret_cast<uint64_t>(masm()->heap()->pending_exception()));
  Immediate root_stack(
      reinterpret_cast<uint64_t>(masm()->heap()->root_stack()));

  // Arguments: rax - exception num

  // Set pending exception
  Operand scratch_op(scratch, 0);
  __ movq(scratch, pending_exception);
  __ movq(scratch_op, rax);

  // Unwind stack to the top handler
  __ movq(scratch, root_stack);
  __ movq(rsp, scratch_op);

  // Return NULL
  __ movq(rax, 0);

  // Leave to C++ land
  __ pop(r15);
  __ pop(r14);
  __ pop(r13);
  __ pop(r12);
  __ pop(rbx);
  __ pop(rbp);
  __ ret(0);
}


void LookupPropertyStub::Generate() {
  GeneratePrologue();
  RuntimeLookupPropertyCallback lookup = &RuntimeLookupProperty;

  // Arguments
  Operand object(rbp, 32);
  Operand property(rbp, 24);
  Operand change(rbp, 16);

  __ Pushad();

  // RuntimeLookupProperty(heap, stack_top, obj, key, change)
  // (returns addr of slot)
  __ movq(rdi, Immediate(reinterpret_cast<uint64_t>(masm()->heap())));
  __ movq(rsi, rsp);
  __ movq(rdx, object);
  __ movq(rcx, property);
  __ movq(r8, change);
  __ movq(rax, Immediate(*reinterpret_cast<uint64_t*>(&lookup)));
  __ callq(rax);

  __ Popad(rax);
  GenerateEpilogue(3);
}


void CoerceToBooleanStub::Generate() {
  GeneratePrologue();
  RuntimeCoerceCallback to_boolean = &RuntimeToBoolean;

  // Arguments
  Operand object(rbp, 16);

  __ Pushad();

  __ movq(rdi, Immediate(reinterpret_cast<uint64_t>(masm()->heap())));
  __ movq(rsi, rsp);
  __ movq(rdx, object);
  __ movq(rax, Immediate(*reinterpret_cast<uint64_t*>(&to_boolean)));
  __ callq(rax);

  __ Popad(rax);
  GenerateEpilogue(1);
}


void BinaryAddStub::Generate() {
  (new BinaryOpStub(masm(), BinOp::kAdd))->Generate();
}


void BinarySubStub::Generate() {
  (new BinaryOpStub(masm(), BinOp::kSub))->Generate();
}


void BinaryMulStub::Generate() {
  (new BinaryOpStub(masm(), BinOp::kMul))->Generate();
}


void BinaryDivStub::Generate() {
  (new BinaryOpStub(masm(), BinOp::kDiv))->Generate();
}


void BinaryOpStub::Generate() {
  GeneratePrologue();
  __ subq(rsp, Immediate(8));
  __ push(rbx);

  // Arguments
  Operand lhs(rbp, 24);
  Operand rhs(rbp, 16);

  Label call_runtime(masm()), nil_result(masm()), done(masm());

  __ movq(rax, lhs);
  __ movq(rbx, rhs);

  __ IsNil(rax, NULL, &call_runtime);
  __ IsNil(rbx, NULL, &call_runtime);

  __ IsHeapObject(Heap::kTagNumber, rax, &call_runtime, NULL);
  __ IsHeapObject(Heap::kTagNumber, rbx, &call_runtime, NULL);

  // We're adding two heap numbers
  Operand lvalue(rax, 8);
  Operand rvalue(rbx, 8);
  __ movq(rax, lvalue);
  __ movq(rbx, rvalue);
  __ movqd(xmm1, rax);
  __ movqd(xmm2, rbx);

  switch (type()) {
   case BinOp::kAdd: __ addqd(xmm1, xmm2); break;
   case BinOp::kSub: __ subqd(xmm1, xmm2); break;
   case BinOp::kMul: __ mulqd(xmm1, xmm2); break;
   case BinOp::kDiv: __ divqd(xmm1, xmm2); break;
   default: __ emitb(0xcc); break;
  }

  __ AllocateNumber(xmm1, rax);

  __ jmp(&done);
  __ bind(&call_runtime);

  RuntimeBinOpCallback cb;

  switch (type()) {
   case BinOp::kAdd: cb = &RuntimeBinOpAdd; break;
   default: __ emitb(0xcc); break;
  }

  {
    Label call(masm());

    Masm::Align a(masm());
    __ Pushad();

    Immediate heapref(reinterpret_cast<uint64_t>(masm()->heap()));

    // binop(heap, top_stack, lhs, rhs)
    __ movq(rdi, heapref);
    __ movq(rsi, rsp);
    __ movq(rdx, rax);
    __ movq(rcx, rbx);

    __ movq(scratch, Immediate(*reinterpret_cast<uint64_t*>(&cb)));
    __ callq(scratch);

    __ Popad(rax);
  }

  __ bind(&done);

  __ pop(rbx);

  // Caller should unwind stack
  GenerateEpilogue(0);
}

} // namespace candor
