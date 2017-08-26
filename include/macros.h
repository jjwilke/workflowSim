#ifndef MY_MACROS_H_
#define MY_MACROS_H_

#include <myCircularBuffer.h>

//typedef intptr_t trace_entry_t; //98
using trace_entry_t = intptr_t; //11
using cir_buf_t = CircularBuffer<trace_entry_t>;

//static const int END_OF_TRACE = -7777;
constexpr trace_entry_t const END_OF_TRACE = -7777;
constexpr int const NUM_OF_BUFFERS = 1;
constexpr int const WORKSPACE_SIZE = 7186;
constexpr int const TRACE_ENTRY_SIZE = sizeof(trace_entry_t);
constexpr int const WORKSPACE_LEN = (WORKSPACE_SIZE / TRACE_ENTRY_SIZE);
constexpr int const CIR_BUF_SIZE = sizeof(cir_buf_t);

#ifndef MMAP_PATH
    #define MMAP_PATH (const char*)"/Users/Garrett/Desktop/workflowProject/bin/pinMap.out"
#endif




#endif /* MY_MACROS_H_ */
