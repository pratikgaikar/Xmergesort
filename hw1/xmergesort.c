#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "xmergesort.h"

int main(int argc, char * const argv[])
{
int rc= 0;	
	struct input *data = malloc(sizeof(struct input));
	if(data==NULL)                     
    	{
        	printf("Error! memory not allocated.");
        	exit(0);
        }
	

	int opt,rc=0;
        int input_flag = 0x0000;
       
        while ((opt = getopt(argc, argv, "uaitd")) != -1) {
        
	switch (opt) {
               case 'u':
                   input_flag=input_flag | 0x0001;
                   break;
               case 'a':
                   input_flag=input_flag | 0x0002;
                   break;
               case 'i':
                   input_flag=input_flag | 0x0004;
                   break;
               case 't':
                   input_flag=input_flag | 0x0010;
                   break;
               case 'd':
                   input_flag=input_flag | 0x0020;
                   break;

               default:               fprintf(stderr, "Usage: %s [-u] [-a] [-i] [-t] [-d] only allowed\n",argv[0]);
                   exit(EXIT_FAILURE);
		}
	}	

	// Checking -au or -ua condtion fot the flag.
	if(input_flag == 3)
	{	
		printf("\n [-au] flags can not be used together.");
		exit(EXIT_FAILURE);
	}
	else if(input_flag == 0)
	{
		printf("\n Atleast one flag is expected.");
                exit(EXIT_FAILURE);
	}

	// Assigning input flag value to the structure.
	data->flag = input_flag;

	// Allcoating memory for N input files.
	data->input_files = malloc(100* sizeof(char *));
	
	//Assigning output file name.
	data->output_file_name = argv[optind++];
	printf("\nOutput file name %s", data->output_file_name);

	//Number of input files.
	int file_index=0;
	while(optind < argc)
	{
		data->input_files[file_index++] =  argv[optind++];
		printf("\nInput file name %s",data->input_files[file_index-1] );
	}
	char test[5];
	test[0] = '2';
	test[1] ='a';
	rc = syscall(329,test);
        if (rc == 0)
                printf("\nsyscall returned %d\n", rc);
        else
                printf("\nsyscall returned %d (errno=%d)\n", rc, errno);	
	exit(rc);
}
