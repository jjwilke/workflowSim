#ifndef MACROS_H_
#define MACROS_H_

//typedef intptr_t trace_entry_t; //98
//using trace_entry_t = intptr_t; //11

//static const int END_OF_TRACE = -7777;
//constexpr int const END_OF_TRACE = -7777; 

#ifndef TRACE_TYPE
#endif

#ifdef TRACE_TYPE
  #define BUFFER CircularBuffer<TRACE_TYPE>
  #define WORKSPACE_SIZE 7168
  #define WORKSPACE_LEN (WORKSPACE_SIZE / sizeof(TRACE_TYPE))
#endif

#ifndef BUFFER_SIZE
  #define BUFFER_SIZE sizeof(BUFFER)
#endif

#ifndef PATH
  #define PATH "/Users/gvanmou/Desktop/workflowProject/bin/pinMap.out"
  #define EMPTY (const char*)"empty"
  #define FULL (const char*)"full"
#endif


#endif
