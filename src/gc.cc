#include "gc.h"
#include "heap.h"

#include <sys/types.h> // off_t
#include <assert.h> // assert

namespace dotlang {

void GC::GCValue::Relocate(char* address) {
  if (slot_ != NULL) {
    *slot_ = address;
    value()->SetGCMark(address);
  }
}

void GC::CollectGarbage(char* stack_top) {
  assert(grey_items()->length() == 0);
  assert(black_items()->length() == 0);

  // Temporary space which will contain copies of all visited objects
  Space space(heap(), heap()->new_space()->page_size());

  // Go through the stack, down to the root_stack() address
  char* top = stack_top;
  for (; top < *heap()->root_stack(); top += sizeof(void*)) {
    char** slot = reinterpret_cast<char**>(top);
    char* value = *slot;

    // Skip NULL pointers, non-pointer values and rbp pushes
    if (value == NULL || (reinterpret_cast<off_t>(value) & 0x01 == 1) ||
        value > top && value <= *heap()->root_stack()) {
      continue;
    }

    grey_items()->Push(new GCValue(HValue::New(value), slot));
  }

  while (grey_items()->length() != 0) {
    GCValue* value = grey_items()->Shift();

    if (!value->value()->IsGCMarked()) {
      HValue* hvalue = value->value()->CopyTo(&space);
      value->Relocate(reinterpret_cast<char*>(hvalue));
      GC::VisitValue(hvalue);
      black_items()->Push(hvalue);
    } else {
      value->Relocate(value->value()->GetGCMark());
    }
  }

  // Remove marks on finish
  while (black_items()->length() != 0) {
    black_items()->Shift()->ResetGCMark();
  }

  heap()->new_space()->Swap(&space);
}


void GC::VisitValue(HValue* value) {
  switch (value->tag()) {
   case Heap::kTagContext:
    return VisitContext(value->As<HContext>());
   case Heap::kTagFunction:
    return VisitFunction(value->As<HFunction>());

   // String and numbers ain't referencing anyone
   case Heap::kTagString:
   case Heap::kTagNumber:
    return;
   default:
    assert(0 && "Not implemented yet");
  }
}


void GC::VisitContext(HContext* context) {
  for (uint32_t i = 0; i < context->slots(); i++) {
    HValue* value = context->GetSlot(i);
    grey_items()->Push(new GCValue(value, context->GetSlotAddress(i)));
  }
}


void GC::VisitFunction(HFunction* fn) {
  grey_items()->Push(new GCValue(HValue::New(fn->parent()), fn->parent_slot()));
}

} // namespace dotlang
