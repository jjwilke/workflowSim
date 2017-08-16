
#ifndef SST_CORE_INTERPROCESS_CIRCULARBUFFER_H
#define SST_CORE_INTERPROCESS_CIRCULARBUFFER_H

#include <mySSTMutex.h>
#include <queue>
#include <vector>

namespace SST {
namespace Core {
namespace Interprocess {

template <typename T>
class CircularBuffer {

private:
    SSTMutex bufferMutex;
    size_t buffSize;
    size_t readIndex;
    size_t writeIndex;
    T buffer[0];


public:
	CircularBuffer(size_t mSize = 0) {
		buffSize = mSize;
		readIndex = 0;
		writeIndex = 0;
	}

    void setBufferLength(const size_t bufferLength)
    {
        if ( buffSize != 0 )
        {
	            fprintf(stderr, "Already specified size for buffer\n");
        	    exit(1);
        }

        buffSize = bufferLength;
        __sync_synchronize();
    }

	//Read buffer->result vector after clearing result vector
	bool read(std::vector<T> &result) 
	{
		int loop_counter = 0;
        bool empty = true;
        bool atEndOfTrace = false;

        //Make sure the collection vector is empty
		result.clear();

		while( true ) 
        {
			//printf("Loop counter value = %d\n", loop_counter);
			//printf(" readIndex = %lu\n", readIndex);
			//printf("writeIndex = %lu\n", writeIndex);
            empty = (readIndex == writeIndex);

            if ( empty )
			{
                lockGuard g(bufferMutex);

                //printf("Inside [r]check #1\n");
				bufferMutex.processorPause(loop_counter++);
				continue;
			}

			printf("Reading data...\n");
            do
            {
                lockGuard g(bufferMutex);

                //Checks
                empty = (readIndex == writeIndex);
                atEndOfTrace = (buffer[readIndex] == 0);

                //Add data to vector and update readIndex
                result.push_back( buffer[readIndex] );
				readIndex = (readIndex + 1) % buffSize;	
            } while ( !empty && !atEndOfTrace );

            printf("The end has been reached...*dramatic music*...\n");
            //Mark the end last section being read
            result.push_back(-7777);
            return true;
		}
	}

    //NEED TO EDIT
	bool readNB(T* result) {
        if( bufferMutex.try_lock() )
        {
            if( readIndex != writeIndex )
            {
				*result = buffer[readIndex];
				readIndex = (readIndex + 1) % buffSize;

				bufferMutex.unlock();
				return true;
			} 

			bufferMutex.unlock();
		}

		return false;
	}

	//modify so write() takes a CONST
    //Takes a queue, and empties it while transferring to buffer
    int write(std::queue<T> &src)
	{
		int loop_counter = 0;
        int item_count = 0;
        bool full = false;

		printf("<<< 1 >>>\n");	
		while( true ) 
		{
            printf("Before full(1) [w]check #1\n");
            printf("writeIndex = %lu\n", writeIndex); //seg fault HERE w/ access of variables
            printf(" readIndex = %lu\n", readIndex);
            printf("  buffSize = %lu\n", buffSize);
            full = ( (writeIndex + 1) % buffSize ) == readIndex;

            printf("Inside [w]check #1\n");
            if ( full )
			{
                lockGuard g(bufferMutex);

                printf("Inside [w]check #2\n");
                bufferMutex.processorPause(loop_counter++);
				return 0;
			}
			
			printf("writing data...\n");
            while ( !full )
			{
                lockGuard g(bufferMutex);

                full = ( (writeIndex + 1) % buffSize ) == readIndex;

                if ( !src.empty() )
				{
                    buffer[writeIndex] = src.front();
                    src.pop();
				}
				else
				{
					buffer[writeIndex] = 0;
				}
				writeIndex = (writeIndex + 1) % buffSize;
                item_count++;

                __sync_synchronize();
			}
			
			printf("Write index = %lu\n", writeIndex);
			printf(" Read index = %lu\n", readIndex);
		
            return item_count;
		}
	}

    ~CircularBuffer() {}

    void clearBuffer()
    {
		bufferMutex.lock();
		readIndex = writeIndex;
		__sync_synchronize();
		bufferMutex.unlock();
	}


};

}
}
}

#endif
