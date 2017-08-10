#ifndef MACROS_H_
#define MACROS_H_


#ifndef TRACE_TYPE
  #define TRACE_TYPE intptr_t
  #define END_OF_TRACE -7777
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
#endif


#endif
