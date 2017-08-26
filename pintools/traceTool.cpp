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
static PinTunnel<trace_entry_t> *tunnel;

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
    if ( PIN_Init(argc, argv) ) { return Usage(); }

    LEVEL_PINCLIENT::PIN_InterceptSignal(SIGSEGV,
                                         signalHandler, nullptr);

    PinTunnel<trace_entry_t> tunnelInit; //THIS LINE as a global causes PIN to fail...
    tunnel = &tunnelInit;
    size_t buffer = 0;
    printf("Tunnel address [pinMain] = %p\n", (void*)&tunnel);
    printf("Tunnel buffer size [0] = %zu\n", tunnel->getTunnelBufferLen(buffer) );

	INS_AddInstrumentFunction(Instruction, 0);

	PIN_AddFiniFunction(Fini, 0);

	// Never returns
	PIN_StartProgram();


	return 0;
}


//Pintool
void recordMemRD( void *addr )
{
    if ( bufferCheckAndClear() )
    {
        printf("Buffer has been written to and cleared\n");
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
        printf("Buffer has been written to and cleared\n");
        return;
    }
	traceData.push( (trace_entry_t)addr );
	icount++;
	//fprintf(trace, "%p\n", addr);
}

void Instruction( INS ins, void *v )
{	
	UINT32 memOperands = INS_MemoryOperandCount(ins);
	
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

INT32 Usage()
{
    std::cerr << "Effective address trace to sstmutex buffer" << std::endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

void Fini( INT32 code, void *v )
{
	//One last transfer to clear up any straglers
	printf("Before buffer transfer...\n");
    bufferTransfer();

	printf("\nPIN is finished...goodbye...\n");
}

static bool signalHandler(unsigned int tid, int sig, LEVEL_VM::CONTEXT *ctx, bool hasHandler,
                          const LEVEL_BASE::EXCEPTION_INFO *exceptInfo, void *)
{
    printf("Process[%d] Thread[%d] recieved signal %d\n...waiting...\n", getpid(), tid, sig);
    sleep(10);

    exit(0);
    return true;
}


//Additional Functions
bool bufferCheckAndClear()
{
    if ( icount < (WORKSPACE_LEN) )
    {
        return false;
    }

    //WRITE TO BUFFER
    return bufferTransfer();
}

bool bufferTransfer()
{
    //write to buffer
    printf("\nSize of traceData = %lu\n", traceData.size());
    printf("Tunnel address [bufferTransfer] = %p\n", (void*)&tunnel);
    size_t bufferIndex = 0;
    int transferCount = tunnel->writeTraceSegment(bufferIndex, traceData);

    size_t transferSizeActual = transferCount * sizeof(trace_entry_t);
    printf("Actual number of bytes transferred = %lu\n", transferSizeActual);
    icount = 0;
    return true;
}


