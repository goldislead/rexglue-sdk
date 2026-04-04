/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <rex/assert.h>
#include <rex/kernel.h>
#include <rex/logging.h>
#include <rex/system/thread_state.h>
#include <rex/thread.h>

namespace rex::runtime {

thread_local ThreadState* thread_state_ = nullptr;

#ifdef _WIN32
extern "C" __declspec(dllexport) ThreadState* rexglue_get_thread_state() {
  return thread_state_;
}

namespace {

using GetThreadStateFn = ThreadState* (*)();

ThreadState* GetMainModuleThreadState() {
  HMODULE main_module = GetModuleHandleW(nullptr);
  if (!main_module) {
    return nullptr;
  }
  auto get_thread_state = reinterpret_cast<GetThreadStateFn>(
      GetProcAddress(main_module, "rexglue_get_thread_state"));
  return get_thread_state ? get_thread_state() : nullptr;
}

}  // namespace
#endif

ThreadState::ThreadState(uint32_t thread_id, uint32_t stack_base, uint32_t pcr_address,
                         memory::Memory* memory)
    : memory_(memory), pcr_address_(pcr_address), thread_id_(thread_id) {
  if (thread_id_ == UINT_MAX) {
    // System thread. Assign the system thread ID with a high bit
    // set so people know what's up.
    uint32_t system_thread_handle = rex::thread::current_thread_system_id();
    thread_id_ = 0x80000000 | system_thread_handle;
  }

  // Initialize the PPCContext (context_ already points to context_storage_)
  std::memset(context_, 0, sizeof(::PPCContext));

  // Set initial registers
  context_->r1.u64 = stack_base;    // Stack pointer
  context_->r13.u64 = pcr_address;  // PCR address

  // Note(tomc): the rexglue abi passes memory base via 'base' parameter to each function call,
  // so we don't need to store it in the context like JIT did.
}

ThreadState::~ThreadState() {
  if (thread_state_ == this) {
    thread_state_ = nullptr;
  }
}

void ThreadState::Bind(ThreadState* thread_state) {
  thread_state_ = thread_state;
}

ThreadState* ThreadState::Get() {
  if (thread_state_) {
    return thread_state_;
  }

#ifdef _WIN32
  return GetMainModuleThreadState();
#else
  return nullptr;
#endif
}

uint32_t ThreadState::GetThreadID() {
  auto* thread_state = Get();
  return thread_state ? thread_state->thread_id_ : 0xFFFFFFFF;
}

}  // namespace rex::runtime
