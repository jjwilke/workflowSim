#include <iostream>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <fcntl.h>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


// Function Declarations
bool exeProg(int argc, const char **argv);
bool pinCallFini(pid_t &pid, int &status, int &timeout, int argc, const char **argv);
std::vector<intptr_t> inputTrace(std::string pipeDir);
void printVector(std::vector<intptr_t> &src);


int main (int argc, char** argv)
{
	//VARIABLES
	// To modify
	std::string home = "/Users/gvanmou"; //would use $HOME, but execve does not read
	std::string pinRoot = home + "/install/pin-3.2-81205-clang-mac/";
	std::string pintool = "traceTest";
	std::string progToTrace = "forkTest";

	std::string pinCall = pinRoot + "pin";
	std::string pinOptions = "-t";
	std::string pintoolCall = "bin/" + pintool;
	std::string progCall = "bin/" + progToTrace;
		

	// Pin call
	int my_argc = 5;
	const char *programCall[100] = {pinCall.c_str(), pinOptions.c_str(), 
					pintoolCall.c_str(), "--", progCall.c_str(), 
					nullptr};
	bool callSuccess = exeProg(my_argc, programCall);		
	if (callSuccess)
	{
		//-----------------------
		//collect trace from mmap
		//-----------------------
		
	
		//-----------------------
		//print collected trace data
		//-----------------------
		

		//-----------------------
		//printVector(traceData);
		//-----------------------


		//-----------------------
		//cache sim
		//-----------------------
		
	}
	
	return 0;
}


bool exeProg(int argc, const char **argv)
{
	pid_t pid;
	int status;
	int timeout;
	
	pid = fork();
	if (pid < 0)
	{
		perror("Error: Fork failed");
		exit(1);
	}
	if (pid == 0)
	{
		// *****************************************
		// Child Process
		// *****************************************
		if (execve(argv[0], (char **)argv, nullptr))
		{
			perror("Error: Program failed to execute");
			return false;
		}
	}

	// *****************************************
	// Parent Process
	// *****************************************
	
	//Wait for child and summarize fork call
	return pinCallFini(pid, status, timeout, argc, argv);	
		
}


bool pinCallFini(pid_t &pid, int &status, int &timeout, int argc, const char **argv)
{
	timeout = 1000;
	while (0 == waitpid(pid, &status, WNOHANG))
	{
		if (--timeout < 0)
		{
			perror("Timeout...");
			return false;
		}
		sleep(1);
	}

	std::cout << std::endl << "Fork() Summary:" << std::endl;
	for (int i = 0; i < argc; i++)
	{
		std::cout << "argv[" << i << "]= " << argv[i] << std::endl;
	}
	std::cout << "ExitStatus=" << WEXITSTATUS(status);
	std::cout << " WIFEXITED=" << WIFEXITED(status) << " Status=";
	std::cout << status << std::endl;

	if (0 != WEXITSTATUS(status) || 1 != WIFEXITED(status))
	{
		perror("Call failed");
		return false;
	}
	return true;	
}

/*
std::vector<intptr_t> inputTrace()
{
	int pinfd = open(pipeDir.c_str(), O_RDONLY);
	std::vector<intptr_t> traceOutput;	
	
	//INIT MMAP READ

	if (pinfd != -1)
	{
		intptr_t temp;
		while ( read(pinfd,&temp,sizeof(intptr_t)) )
		{
			traceOutput.push_back(temp);
		}
		close(pinfd);	
	}
	else
	{
		perror("Error: mmap file did not open");
	}

	return traceOutput;
}
*/

void printVector(std::vector<intptr_t> &src)
{
	for (int i = 0; i < src.size(); i++)
	{
		std::cout << "[" << i << "]: " << src[i] << std::endl;
	}
}
