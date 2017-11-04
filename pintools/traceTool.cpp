#include <stdio.h>
#include <sys/types.h>
#include <pin.H>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>

#include <PinTunnel.h>


//Globals
std::queue<trace_entry_t> traceData;
UINT64 icount = 0;
//PinTunnel tunnel;
// void *tunnel_void = nullptr;
// int *test;
const size_t buffer = 0;
static std::ostream *Output = &std::cerr;

//Declarations
void recordMemRD( void *addr, void *tunnel_void );
void recordMemWR( void *addr, void *tunnel_void );
void Instruction( INS ins, void *v );
void Fini( INT32 code, void *tunnel_void );
bool bufferCheckAndClear(PinTunnel &tunnel);
bool bufferTransfer(PinTunnel &tunnel);
INT32 Usage();
static bool signalHandler(unsigned int tid, int sig, LEVEL_VM::CONTEXT *ctx, bool hasHandler,
                          const LEVEL_BASE::EXCEPTION_INFO *exceptInfo, void *);


int main( int argc, char *argv[] )
{
    if ( PIN_Init(argc, argv) ) { return Usage(); }

    // LEVEL_PINCLIENT::PIN_InterceptSignal(SIGSEGV,
    //                                      signalHandler, nullptr);

    PinTunnel tunnel_void; //THIS LINE as a global causes PIN to fail...
    // tunnel_void = &tunnelInit;
    // printf("Tunnel address [pinMain] = %p <-----\n", (void*)&tunnel);
    // printf("Tunnel buffer size (size != 0) = %zu\n", tunnel->getTunnelBufferLen(buffer) );
    // printf("Tunnel buffer readIndex (== 0) = %zu\n", tunnel->getTunnelReadIndex(buffer) );

    // int intInit = 0;
    // test = &intInit;
    // printf("Test int value = %d\n", *test );

	INS_AddInstrumentFunction(Instruction, &tunnel_void);

	PIN_InitSymbols();
    PIN_AddFiniFunction(Fini, &tunnel_void);

	// Never returns
	PIN_StartProgram();


	return 0;
}


//Pintool
void Instruction( INS ins, void *tunnel_void )
{   
    UINT32 memOperands = INS_MemoryOperandCount(ins);
    
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall ( 
                ins, IPOINT_BEFORE, (AFUNPTR)recordMemRD,
                IARG_MEMORYOP_EA, memOp,
                IARG_PTR, tunnel_void,
                IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall (
                ins, IPOINT_BEFORE, (AFUNPTR)recordMemWR,
                IARG_MEMORYOP_EA, memOp,
                IARG_PTR, tunnel_void,
                IARG_END);
        }
    }
}

void recordMemRD( void *addr, void *tunnel_void )
{
    PinTunnel *tunnel = reinterpret_cast<PinTunnel*>(tunnel_void);

    if ( bufferCheckAndClear(*tunnel) )
    {
        // printf("Buffer has been written to and cleared\n");
        return;
    }
	traceData.push( (trace_entry_t)addr );
	icount++;
	//fprintf(trace, "%p\n", addr);
}

void recordMemWR( void *addr, void *tunnel_void )
{
    PinTunnel *tunnel = reinterpret_cast<PinTunnel*>(tunnel_void);

    if ( bufferCheckAndClear(*tunnel) )
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

void Fini( INT32 code, void *tunnel_void )
{
    PinTunnel *tunnel = reinterpret_cast<PinTunnel*>(tunnel_void);

	//One last transfer to clear up any straglers
	printf("Before final buffer transfer...\n");
    bufferTransfer(*tunnel);

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
bool bufferCheckAndClear(PinTunnel &tunnel)
{
    //printf("IN bufferCheckAndClear()... [%llu]\n", icount);
    if ( icount < (WORKSPACE_LEN) )
    {
        return false;
    }

    //WRITE TO BUFFER
    return bufferTransfer(tunnel);
}

bool bufferTransfer(PinTunnel &tunnel)
{
    // write to buffer
    printf("IN bufferTransfer()...\n");
    printf("Tunnel address [bufferTransfer] = %p <-----\n", (void*)&tunnel);
    printf("Elements to add to buffer = %zu\n", traceData.size() );
    
    // printf("Test int value [bufferTransfer] = %d\n", *test );
    printf("Tunnel buffer size (size != 0) and again... = %zu\n", tunnel.getTunnelBufferLen(buffer) );
    int transferCount = tunnel.writeTraceSegment(buffer, traceData);

    size_t transferSizeActual = transferCount * sizeof(trace_entry_t);
    printf("Actual number of bytes transferred = %lu\n", transferSizeActual);
    icount = 0;
    return true;
}
