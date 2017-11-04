#include <stdio.h>
#include <sys/types.h>
#include <pin.H>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>

#include <PinTunnel.h>
#include <testClass.h>


//Globals
std::queue<trace_entry_t> traceData;
UINT64 icount = 0;
//PinTunnel tunnel;
// void *tunnel_void = nullptr;
// int *test;
PinTunnel *tunnel = nullptr;
int COUNT = 0;
const size_t buffer = 0;
static std::ostream *Output = &std::cerr;

//Declarations
void recordMemRD( void *addr );
void recordMemWR( void *addr );
void Instruction( INS ins, void *v );
void Fini( INT32 code, void *v );
bool bufferCheckAndClear();
bool bufferTransfer();
INT32 Usage();
static bool signalHandler(unsigned int tid, int sig, LEVEL_VM::CONTEXT *ctx, bool hasHandler,
                          const LEVEL_BASE::EXCEPTION_INFO *exceptInfo, void *);


int main( int argc, char *argv[] )
{
    // PIN_InitSymbols() must be called before PIN_Init()
    PIN_InitSymbols();
    if ( PIN_Init(argc, argv) ) { return Usage(); }

    // LEVEL_PINCLIENT::PIN_InterceptSignal(SIGSEGV,
    //                                      signalHandler, nullptr);
    // newTest = new TEST();
    //PinTunnel tunnel_void; //THIS LINE as a global causes PIN to fail...
    // tunnel_void = &tunnelInit;
    // printf("Tunnel address [pinMain] = %p <----- (should match)\n", (void*)&tunnel_void);
    // printf("Tunnel buffer size (size != 0) = %zu\n", tunnel_void.getTunnelBufferLen(buffer) );
    // printf("Tunnel buffer readIndex (== 0) = %zu\n", tunnel_void.getTunnelReadIndex(buffer) );

    // int intInit = 0;
    // test = &intInit;
    // printf("Test int value = %d\n", *test );

	INS_AddInstrumentFunction(Instruction, 0);
    printf("Between INS and FINI...\n");
    printf("...AFTER...\n");
    PIN_AddFiniFunction(Fini, 0);


	// Never returns
	PIN_StartProgram();
	return 0;
}


//Pintool
void Instruction( INS ins, void *v )
{   
    UINT32 memOperands = INS_MemoryOperandCount(ins);
    // printf("in Instruction()... {%d}\n", COUNT);
    
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall ( 
                ins, IPOINT_BEFORE, (AFUNPTR)recordMemRD,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall (
                ins, IPOINT_BEFORE, (AFUNPTR)recordMemWR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
    }
}

void recordMemRD( void *addr )
{
    if ( bufferCheckAndClear() )
    {
        // printf("Buffer has been written to and cleared\n");
        return;
    }
	traceData.push( (trace_entry_t)addr );
	icount++;
	//fprintf(trace, "%p\n", addr);
}

void recordMemWR( void *addr )
{
    if ( bufferCheckAndClear() )
    {
        // printf("Buffer has been written to and cleared\n");
        return;
    }
	traceData.push( (trace_entry_t)addr );
	icount++;
	//fprintf(trace, "%p\n", addr);
}

INT32 Usage()
{
    std::cerr << "Effective address trace to sstmutex buffer" << std::endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

void Fini( INT32 code, void *v )
{
	//One last transfer to clear up any straglers
	printf("Before final buffer transfer...\n");
    bufferTransfer();

	printf("\nPIN is finished...goodbye...\n");
}

// static bool signalHandler(unsigned int tid, int sig, LEVEL_VM::CONTEXT *ctx, bool hasHandler,
//                           const LEVEL_BASE::EXCEPTION_INFO *exceptInfo, void *)
// {
//     printf("Process[%d] Thread[%d] recieved signal %d\n...waiting...\n", getpid(), tid, sig);
//     sleep(10);

//     exit(0);
//     return true;
// }


//Additional Functions
bool bufferCheckAndClear()
{
    //printf("IN bufferCheckAndClear()... [%llu]\n", icount);
    if ( icount < (WORKSPACE_LEN) )
    {
        return false;
    }

    //WRITE TO BUFFER
    return bufferTransfer();
}

bool bufferTransfer()
{
    printf("IN bufferTransfer()...\n");
    printf("Tunnel address [bufferTransfer] = %p <----- (should match)\n", (void*)&tunnel);

    if ( tunnel == nullptr )
    {
        printf("Tunnel address [bufferTransfer] = %p <----- (NULL)\n", (void*)&tunnel);
        printf("\nassigning new PinTunnel\n");
        tunnel = new PinTunnel();
    }
    // printf("Tunnel buffer size (size != 0) and again... = %zu\n", test.getTunnelBufferLen(buffer) );

    // write to buffer
    // printf("IN bufferTransfer()...\n");
    // printf("Tunnel address [bufferTransfer] = %p <----- (should match)\n", (void*)&tunnel);
    printf("Elements to add to buffer = %zu  [bufferTransfer()]\n", traceData.size() );
    // printf("Test int value [bufferTransfer] = %d\n", *test );
    printf("Tunnel buffer size = %zu\n", tunnel->getTunnelBufferLen(buffer) );
    int transferCount = tunnel->writeTraceSegment(buffer, traceData);

    size_t transferSizeActual = transferCount * sizeof(trace_entry_t);
    printf("Actual number of bytes transferred = %lu\n", transferSizeActual);
    icount = 0;
    return true;
}
