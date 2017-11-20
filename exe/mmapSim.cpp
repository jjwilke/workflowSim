#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

bool mmapInit(char *, struct stat &, void *, int &);
bool mmapExit(void *, struct stat &);

int main (int argc, char** argv)
{
	char fileName[] = "mmap.out";
	struct stat memBuf;
	off_t len;
	char *map;
	int fd;

	bool initSuccess = mmapInit(fileName, memBuf, len, map, fd);
	if (!initSuccess)
	{
		printf("exiting program...\n");
		close(fd);
		return 1;	
	}
	
	//Do something with mmap
	
	
	
	if (mmapExit(map, memBuf))
	{
		printf("mmap closed...\n");
	}
	return 0;
}


bool mmapInit(char *fName, struct stat &memBuf, void *map, int &fd)
{
	printf("File: %s\n", fName);	
	fd = open(fName, O_RDWR);

	if (fd == -1)
	{
		perror("File did not *open* properly");
		return false;
	}
	if (fstat(fd, &memBuf) == -1)
	{
		perror("File status error (fstat)");
		return false;
	}
	
	map = mmap(NULL, memBuf.st_size, PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED)
	{
		perror("mmap failed to open");
		return false;
	}
	if (close(fd) == -1)
	{
		perror("File did not *close* properly");
		return false;
	}
		
	return true;
}

bool mmapExit(void *map, struct stat &buf)
{
	if (munmap(map, buf.st_size) == -1)
	{
		perror("munmap failed");
		return false;
	}	

	return true;
}
