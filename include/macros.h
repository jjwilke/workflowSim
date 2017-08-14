#ifndef MACROS_H_
#define MACROS_H_

#include <myCircularBuffer.h>

using namespace SST::Core::Interprocess;

//typedef intptr_t trace_entry_t; //98
using trace_entry_t = intptr_t; //11
using cir_buf_t = CircularBuffer<trace_entry_t>;
using cir_buf_pair_t = std::pair<size_t, cir_buf_t*>;
using cir_buf_make_pair_t = std::make_pair<size_t, cir_buf_t*>;

//static const int END_OF_TRACE = -7777;
constexpr trace_entry_t const END_OF_TRACE = -7777;
constexpr int const WORKSPACE_SIZE = 7186;
constexpr int const TRACE_ENTRY_SIZE = sizeof(trace_entry_t);
constexpr int const WORKSPACE_LEN = (WORKSPACE_SIZE / TRACE_ENTRY_SIZE);
constexpr int const CIR_BUF_SIZE = sizeof(cir_buf_t);

#ifndef MMAP_PATH
    #define MMAP_PATH "/Users/gvanmou/Desktop/workflowProject/bin/pinMap.out"
#endif




#endif /* MACROS_H_ */
