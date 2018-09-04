//===- BuiltinGCs.cpp - Boilerplate for our built in GC types -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the boilerplate required to define our various built in
// gc lowering strategies.
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/CodeGen/GCs.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

namespace {

/// An example GC which attempts to be compatibile with Erlang/OTP garbage
/// collector.
///
/// The frametable emitter is in ErlangGCPrinter.cpp.
class ErlangGC : public GCStrategy {
public:
  ErlangGC() {
    InitRoots = false;
    NeededSafePoints = 1 << GC::PostCall;
    UsesMetadata = true;
    CustomRoots = false;
  }
};

/// An example GC which attempts to be compatible with Objective Caml 3.10.0
///
/// The frametable emitter is in OcamlGCPrinter.cpp.
class OcamlGC : public GCStrategy {
public:
  OcamlGC() {
    NeededSafePoints = 1 << GC::PostCall;
    UsesMetadata = true;
  }
};

/// A GC strategy for uncooperative targets.  This implements lowering for the
/// llvm.gc* intrinsics for targets that do not natively support them (which
/// includes the C backend). Note that the code generated is not quite as
/// efficient as algorithms which generate stack maps to identify roots.
///
/// In order to support this particular transformation, all stack roots are
/// coallocated in the stack. This allows a fully target-independent stack map
/// while introducing only minor runtime overhead.
class ShadowStackGC : public GCStrategy {
public:
  ShadowStackGC() {
    InitRoots = true;
    CustomRoots = true;
  }
};

/// A GCStrategy which serves as an example for the usage of a statepoint based
/// lowering strategy.  This GCStrategy is intended to suitable as a default
/// implementation usable with any collector which can consume the standard
/// stackmap format generated by statepoints, uses the default addrespace to
/// distinguish between gc managed and non-gc managed pointers, and has
/// reasonable relocation semantics.
class StatepointGC : public GCStrategy {
public:
  StatepointGC() {
    UseStatepoints = true;
    // These options are all gc.root specific, we specify them so that the
    // gc.root lowering code doesn't run.
    InitRoots = false;
    NeededSafePoints = 0;
    UsesMetadata = false;
    CustomRoots = false;
  }

  Optional<bool> isGCManagedPointer(const Type *Ty) const override {
    // Method is only valid on pointer typed values.
    const PointerType *PT = cast<PointerType>(Ty);
    // For the sake of this example GC, we arbitrarily pick addrspace(1) as our
    // GC managed heap.  We know that a pointer into this heap needs to be
    // updated and that no other pointer does.  Note that addrspace(1) is used
    // only as an example, it has no special meaning, and is not reserved for
    // GC usage.
    return (1 == PT->getAddressSpace());
  }
};

/// A GCStrategy for the CoreCLR Runtime. The strategy is similar to
/// Statepoint-example GC, but differs from it in certain aspects, such as:
/// 1) Base-pointers need not be explicitly tracked and reported for
///    interior pointers
/// 2) Uses a different format for encoding stack-maps
/// 3) Location of Safe-point polls: polls are only needed before loop-back
///    edges and before tail-calls (not needed at function-entry)
///
/// The above differences in behavior are to be implemented in upcoming
/// checkins.
class CoreCLRGC : public GCStrategy {
public:
  CoreCLRGC() {
    UseStatepoints = true;
    // These options are all gc.root specific, we specify them so that the
    // gc.root lowering code doesn't run.
    InitRoots = false;
    NeededSafePoints = 0;
    UsesMetadata = false;
    CustomRoots = false;
  }

  Optional<bool> isGCManagedPointer(const Type *Ty) const override {
    // Method is only valid on pointer typed values.
    const PointerType *PT = cast<PointerType>(Ty);
    // We pick addrspace(1) as our GC managed heap.
    return (1 == PT->getAddressSpace());
  }
};

} // end anonymous namespace

// Register all the above so that they can be found at runtime.  Note that
// these static initializers are important since the registration list is
// constructed from their storage.
static GCRegistry::Add<ErlangGC> A("erlang",
                                   "erlang-compatible garbage collector");
static GCRegistry::Add<OcamlGC> B("ocaml", "ocaml 3.10-compatible GC");
static GCRegistry::Add<ShadowStackGC>
    C("shadow-stack", "Very portable GC for uncooperative code generators");
static GCRegistry::Add<StatepointGC> D("statepoint-example",
                                       "an example strategy for statepoint");
static GCRegistry::Add<CoreCLRGC> E("coreclr", "CoreCLR-compatible GC");

// Provide hooks to ensure the containing library is fully loaded.
void llvm::linkErlangGC() {}
void llvm::linkOcamlGC() {}
void llvm::linkShadowStackGC() {}
void llvm::linkStatepointExampleGC() {}
void llvm::linkCoreCLRGC() {}
