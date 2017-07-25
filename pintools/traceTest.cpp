#include <stdio.h>
#include <queue>
#include <pin.H>
#include <string>
#include <cstdlib>
#include <iostream>
//#include <boost/interprocess/containers/vector.hpp>
//#include <boost/interprocess/managed_shared_memory.hpp>
//#include <boost/interprocess/allocators/allocator.hpp>

//using namespace boost::interprocess;

//Shm typedefs
//typedef boost::interprocess::allocator<intptr_t, boost::interprocess::managed_shared_memory::segment_manager> shmemAllocator;
//typedef boost::interprocess::vector<intptr_t, shmemAllocator> shmVector;

//Globals
std::queue<intptr_t> traceStorage;

//Additional Functions
void printVector(std::vector<intptr_t> &src)
{
	for (int i = 0; i < src.size(); i++)
	{
		std::cout << "[" << i << "]: " << src[i] << std::endl;
	}
}

//Pintool
void printMemRead(void *addr)
{
	intptr_t temp = (intptr_t)addr;
	traceStorage.push(temp);
	//fprintf(trace, "%p\n", addr);
}


void printMemWrite(void *addr)
{
	intptr_t temp = (intptr_t)addr;
	traceStorage.push(temp);
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
	// Move to shared memory for parent access *new function*
	//Open shm
	std::vector<intptr_t> pinShm;
	//boost::interprocess::managed_shared_memory pinSegment(boost::interprocess::open_only, "PinShm");
	//shmVector *pinShmVector = pinSegment.find<shmVector>("pinShmVector").first;

	for (int i = 0; i < traceStorage.size(); i++)
	{
		pinShm.push_back( traceStorage.front() );
		traceStorage.pop();
	}
	printVector(pinShm);
}


int main( int argc, char *argv[] )
{
	PIN_Init(argc, argv);
	
	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Fini, 0);

	
	// Never returns
	PIN_StartProgram();


	return 0;
}
