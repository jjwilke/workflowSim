#ifndef MY_CIRCULARBUFFER_H
#define MY_CIRCULARBUFFER_H

using trace_entry_t = intptr_t;
constexpr trace_entry_t const END_OF_TRACE = -7777;

#include <mySSTMutex.h>
#include <queue>
#include <vector>


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

	//Read buffer->result vector after clearing result vector
	bool read(std::vector<T> &result) 
	{
        int loop_counter = 0;
        //Make sure the collection vector is empty
		result.clear();

		while( true ) 
        {
			//printf("Loop counter value = %d\n", loop_counter);
			//printf(" readIndex = %lu\n", readIndex);
			//printf("writeIndex = %lu\n", writeIndex);

            if ( bufferIsEmpty() )
			{
                lockGuard g(bufferMutex);

				bufferMutex.processorPause(loop_counter++);
				continue;
			}

			printf("reading data...\n");
            do
            {
                lockGuard g(bufferMutex);

                //Add data to vector and update readIndex
                result.push_back( buffer[readIndex] );
				readIndex = (readIndex + 1) % buffSize;	
            } while ( !bufferIsEmpty() && !atEndOfTrace() );

            printf("The end of read(vector) has been reached...*dramatic music*\n");
            //Mark the end last section being read
            //result.push_back(-7777);
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
	
		while( true ) 
		{
            printf("writeIndex = %zu\n", writeIndex);

            printf("Inside [w]check #1\n");
            if ( bufferIsFull() )
			{
                lockGuard g(bufferMutex);

                printf("Inside [w]check #2\n");
                bufferMutex.processorPause(loop_counter++);
				return 0;
			}
			
			printf("writing data...\n");
            while ( !bufferIsFull() )
            {
                lockGuard g(bufferMutex);

                if ( !src.empty() )
				{
                    buffer[writeIndex] = src.front();
                    src.pop();
                    item_count++;
				}
                /*MARK if at end of trace, so read() knows when to stop*/
				else
				{
					buffer[writeIndex] = END_OF_TRACE;
                    return item_count;
				}

				writeIndex = (writeIndex + 1) % buffSize;
                __sync_synchronize();
			}
			
            printf("Items written(queue) = %d\n", item_count);
            return item_count;
		}
	}

    ~CircularBuffer() {}

    /*
    * Helper functions
    */
    void clearBuffer()
    {
		lockGuard g(bufferMutex);
		readIndex = writeIndex;
		__sync_synchronize();
	}

    bool bufferIsEmpty()
    {
        if (readIndex == writeIndex)
        {
            return true;
        }
        return false;
    }

    bool bufferIsFull()
    {
        if ( ((writeIndex + 1) % buffSize) == readIndex )
        {
            return true;
        }
        return false;
    }

    bool atEndOfTrace()
    {
        if (buffer[readIndex] == END_OF_TRACE)
        {
            return true;
        }
        return false;
    }

    size_t getBufferLength()
    {
        return buffSize;
    }

    size_t getReadIndex()
    {
        return readIndex;
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


};


#endif
