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
void recordMemRD(void *addr, void *pinTunnel);
void recordMemWR(void *addr, void *pinTunnel);
void Instruction( INS ins, void *v );
void Fini( INT32 code, void *v );
bool bufferCheckAndClear();
bool bufferTransfer();


int main( int argc, char *argv[] )
{
	PIN_Init(argc, argv);

    PinTunnel<trace_entry_t> temp; //THIS LINE as a global causes PIN to fail...
    tunnel = &temp;
    printf("Tunnel address [main] = %p\n", (void*)&tunnel);
    printf("Before INS_AddInstrumentFunction()_____\n");

	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);

	
	// Never returns
	PIN_StartProgram();


	return 0;
}


//Pintool
void recordMemRD(void *addr, void *pinTunnel)
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


void recordMemWR(void *addr, void *pinTunnel)
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

void Instruction( INS ins, void *v)
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

void Fini( INT32 code, void *v )
{
	//One last transfer to clear up any straglers
	printf("Before buffer transfer...\n");
    bufferTransfer();

	printf("\nPIN is finished...goodbye...\n");
}


//Additional Functions
bool bufferCheckAndClear()
{
    if ( icount < (WORKSPACE_LEN-1) )
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


