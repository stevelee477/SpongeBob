#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif
// #define TEST_NRFS_IO
#define TEST_RAW_IO
#include "mpi.h"
#include "Client.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define BUFFER_SIZE 0x1000000
int myid, file_seq;
int numprocs;
std::shared_ptr<Client> client;
char buf[BUFFER_SIZE];
int mask = 0;
int collect_time(int cost)
{
	int i;
	char message[8];
	MPI_Status status;
	int *p = (int*)message;
	int max = cost;
	for(i = 1; i < numprocs; i++)
	{
		MPI_Recv(message, 8, MPI_CHAR, i, 99, MPI_COMM_WORLD, &status);
		if(*p > max)
			max = *p;
	}
	return max;
}

void write_test(int size, int op_time)
{
	char path[255];
	int i;
	double start, end, rate, num;
	int time_cost;
	char message[8];
	int *p = (int*)message;

	/* file open */
	sprintf(path, "file_%d", file_seq);
	// nrfsOpenFile(fs, path, O_CREAT);
    client->getMetaClient()->OpenFile(path);
	printf("create file: %s\n", path);
	memset(buf, 'a', BUFFER_SIZE);

	MPI_Barrier ( MPI_COMM_WORLD );
	
	start = MPI_Wtime();
	for(i = 0; i < op_time; i++)
	{
        client->write(path, buf, 0, size);
	}
	end = MPI_Wtime();

	MPI_Barrier ( MPI_COMM_WORLD );

	*p = (int)((end - start) * 1000000);

	if(myid != 0)
	{
		MPI_Send(message, 8, MPI_CHAR, 0, 99, MPI_COMM_WORLD);
	}
	else
	{
		time_cost = collect_time(*p);
		num = (double)(size * op_time * numprocs) / time_cost;
		rate = 1000000 * num / 1024 / 1024;
		printf("Write Bandwidth = %f MB/s TimeCost = %d\n", rate, (int)time_cost);
	}
	// client.getMetaClient()->CloseFile(path);

	file_seq += 1;
	if(file_seq == numprocs)
		file_seq = 0;
}

void read_test(int size, int op_time)
{
	char path[255];
	int i;
	double start, end, rate, num;
	int time_cost;
	char message[8];
	int *p = (int*)message;

	memset(buf, '\0', BUFFER_SIZE);
	memset(path, '\0', 255);
	sprintf(path, "file_%d", file_seq);

	MPI_Barrier ( MPI_COMM_WORLD );
	
	start = MPI_Wtime();
	for(i = 0; i < op_time; i++)
	{
        client->read(path, buf, 0, size);
	}
	end = MPI_Wtime();

	MPI_Barrier ( MPI_COMM_WORLD );

	*p = (int)((end - start) * 1000000);

	if(myid != 0)
	{
		MPI_Send(message, 8, MPI_CHAR, 0, 99, MPI_COMM_WORLD);
	}
	else
	{
		time_cost = collect_time(*p);
		num = (double)(size * op_time * numprocs) / time_cost;
		rate = 1000000 * num / 1024 / 1024;
		printf("Read Bandwidth = %f MB/s TimeCost = %d\n", rate, (int)time_cost);
	}
	MPI_Barrier ( MPI_COMM_WORLD );

	file_seq += 1;
	if(file_seq == myid)
		file_seq = 0;
}


int main(int argc, char **argv)
{
    client = std::make_shared<Client>();
	char path[255];
	if(argc < 3)
	{
		fprintf(stderr, "Usage: ./mpibw block_size\n");
		return -1;
	}
	int block_size = atoi(argv[1]);
	int op_time = atoi(argv[2]);
	MPI_Init( &argc, &argv);
	MPI_Comm_rank( MPI_COMM_WORLD, &myid );
	MPI_Comm_size( MPI_COMM_WORLD, &numprocs );
	file_seq = myid;
	MPI_Barrier ( MPI_COMM_WORLD );

	/* nrfs connection */
	// fs = nrfsConnect("default", 0, 0);

	// MPI_Barrier ( MPI_COMM_WORLD );

	write_test(1024 * block_size, op_time);
	read_test(1024 * block_size, op_time);

	MPI_Barrier ( MPI_COMM_WORLD );
	sprintf(path, "file_%d", myid);
	// nrfsDelete(fs, path);
	// nrfsDisconnect(fs);

	MPI_Finalize();
}
