#include <stdio.h>
#include <iostream>

#include "pin.H"
#include "instlib.H"
#include "control_manager.H"

using namespace INSTLIB;
using namespace CONTROLLER;

FILE *trace;


#if defined(__GNUC__)
#  if defined(__APPLE__)
#    define ALIGN_LOCK __attribute__ ((aligned(16))) /* apple only supports 16B alignment */
#  else
#    define ALIGN_LOCK __attribute__ ((aligned(64)))
#  endif
#else
# define ALIGN_LOCK __declspec(align(64))
#endif

LOCALVAR PIN_LOCK  ALIGN_LOCK output_lock; 

// Track the number of instructions executed
ICOUNT icount;
int icountInitial;
int icountFinal;
int countReadWrite = 0;

// Contains knobs and instrumentation to recognize start/stop points
CONTROL_MANAGER control("controller_");


void printMemRead(void *addr)
{
        fprintf(trace, "[r]: %p, icount: %llu\n", addr, icount.Count());
		countReadWrite++;
}


void printMemWrite(void *addr)
{
        fprintf(trace, "[w]: %p, icount: %llu\n", addr, icount.Count());
		countReadWrite++;
}


void Instruction( INS ins, void *v )
{
        UINT32 memOperands = INS_MemoryOperandCount(ins);

        for (UINT32 memOp = 0; memOp < memOperands; memOp++)
        {
                if (INS_MemoryOperandIsRead(ins, memOp))
                {
                        INS_InsertPredicatedCall (
                                ins, IPOINT_BEFORE, (AFUNPTR)printMemRead,
                                IARG_MEMORYOP_EA, memOp, IARG_END);
                }
                if (INS_MemoryOperandIsWritten(ins, memOp))
                {
                        INS_InsertPredicatedCall (
                                ins, IPOINT_BEFORE, (AFUNPTR)printMemWrite,
                                IARG_MEMORYOP_EA, memOp, IARG_END);
                }
        }
}


void Fini( INT32 code, void *v )
{
		icountFinal = icount.Count();
        fprintf(trace, "--->EOF<---\n");
		fprintf(trace, "icountFinal: %d\n", icountFinal);
		fprintf(trace, "Read and Write Count: %d", countReadWrite);
        
		fclose(trace);
}


VOID Handler(EVENT_TYPE ev, VOID * v, CONTEXT * ctxt, VOID * ip, THREADID tid, BOOL bcast)
{
    PIN_GetLock(&output_lock, tid+1);

    std::cout << "tid: " << tid << " ";
    std::cout << "ip: "  << ip << " " << icount.Count() << " " ;

    switch(ev)
    {
      case EVENT_START:
        std::cout << "Start" << endl;
		
		// Start Region Instrumentation
		INS_AddInstrumentFunction(Instruction, 0);
		//icountFinal = icount.Count();
		//End Region Instrumentation
		
		break;

      case EVENT_STOP:
        std::cout << "Stop" << endl;
        break;

      case EVENT_THREADID:
        std::cout << "ThreadID" << endl;
        break;

      default:
        ASSERTX(false);
        break;
    }
    PIN_ReleaseLock(&output_lock);
}

INT32 Usage()
{
    cerr <<
        "This pin tool demonstrates use of CONTROL to identify start/stop points in a program, and tracks the read and write effective addresses in the designatied region\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char** argv)
{
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    PIN_InitLock(&output_lock);
   	trace = fopen("controlTest.out", "w"); 
    icount.Activate();
	icountInitial = icount.Count();
	fprintf(trace, "icountInitial: %d\n", icountInitial);

    // Activate alarm, must be done before PIN_StartProgram
    control.RegisterHandler(Handler, 0, FALSE);
    control.Activate();

    //INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);
   
    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
