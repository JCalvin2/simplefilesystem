/*CPM File System by Jacob Calvin Version 1.0*/
/*Run "make" on the command line to compile this program*/
/*Input ./cpmRun on the command line to execute this code*/
/*This code is designed based on Professor Xiao Qin's pseudocode and lectures*/

#include <stdint.h> 
#include <stdlib.h> 
#include "cpmfsys.h"
#include "diskSimulator.h"
#include  <stdbool.h> 
#include <string.h> 
#include <stdio.h> 
#include <malloc.h>
#include <ctype.h>

#define EXTENT_SIZE 32
#define BLOCKS_PER_EXTENT 16 
#define debugging false

//Initializing the Free List of 256 blocks. We make it static so it cannot be changed (size). It is a bool because it returns true or false whether the block is used or not
static bool FreeList[256];

//function to allocate memory for a DirStructType (see above), and populate it, given a
//pointer to a buffer of memory holding the contents of disk block 0 (e), and an integer index
// which tells which extent from block zero (extent numbers start with 0) to use to make the
// DirStructType value to return. 
DirStructType *mkDirStruct(int index,uint8_t *e)
{
	//initialize DirStuct 	
	DirStructType *d = (DirStructType*)malloc(sizeof(DirStructType)); 

	//initialize the type extent
	Extent dir_addr;
	
	//Must create a memeory space for dir
	//Must fill each extent
	dir_addr[0] = *(e + index * EXTENT_SIZE); 
	d->status = dir_addr[0];

	int i;
	for(i = 1; i < EXTENT_SIZE; i++)
	{
		dir_addr[i] = *(e + index * EXTENT_SIZE + i);
	}	
	

	//Now fill in the name, extension, and blocks

	for(i = 0; i < 8; i++) // only want name of length < 8
	{
		d->name[i] = (char) dir_addr[i+1]; //if you dont type cast this it will throw an error/not work
		
		if(d->name[i] == ' ')	//the end
		{		
			break;
		}

	}
	d->name[i] = '\0'; //null terminator 

	for(i = 0; i < 3; i++) // only want extension of length < 3
	{
		d->extension[i] = (char) dir_addr[i+9]; //if you dont type cast this it will throw an error/not work
		
		if(d->extension[i] == ' ')	//the end
		{		
			break;
		}
	}
	d->extension[i] = '\0'; //null terminator

	d->XL = dir_addr[12];
	d->BC = dir_addr[13];
	d->XH = dir_addr[14];
	d->RC = dir_addr[15];
	
	for(i = 0; i < BLOCKS_PER_EXTENT; i++)
	{
		d->blocks[i] = dir_addr[i + 16];
	}

	return d;	
}

// function to write contents of a DirStructType struct back to the specified index of the extent
// in block of memory (disk block 0) pointed to by e
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e)
{	
	//Start by using the reverse of what we did in mkDirStruct & continue from there
	//must create our extent as before in the mkdirstruct
	Extent dir_addr;
	
	/*The following code is the flip of mkdirstruct*/

	dir_addr[0] = d->status;
	

	int i;
	for(i = 0; i < 8; i++) // only want name of lentgh < 8
	{
		if(d->name[i] != '\0') 
		{
			//there is going to be an error if we dont type cast this correctly with uint8_t
			//when we write we need to type cast like this
			dir_addr[i + 1] = (uint8_t) d->name[i];

		}

		else // then its the null terminator and we are done
		{
			break;
		}
	}

	for(i = 0; i < 3; i++) // only want extension with length < 3
	{
		if(d->extension[i] != '\0') 
		{
			//there is going to be an error if we dont type cast this correctly with uint8_t
			//when we write we need to type cast like this
			dir_addr[i + 9] = (uint8_t) d->extension[i];
		}
		
		else // then its the null terminator and we are done
		{
			break; 
		}
	}

	dir_addr[12] = d->XL; 
	dir_addr[13] = d->BC; 
	dir_addr[14] = d->XH;
	dir_addr[15] = d->RC;

	for(i = 0; i < 16; i++) //now blocks 
	{
		dir_addr[i + 16] = d->blocks[i];
	}

	//Now we have to put this value into the memory
	for (i = 0; i < EXTENT_SIZE; i++) 
	{
    		*(e + ((int) index) * EXTENT_SIZE + i) = dir_addr[i]; //make sure to type cast it correctly or there will be an error
  	}
}

// populate the FreeList global data structure. freeList[i] == true means 
// that block i of the disk is free. block zero is never free, since it hold
// the directory. freeList[i] == false means the block is in use. 
void makeFreeList()
{
	uint8_t e[1024]; //since there are 1024 bytes per block
	blockRead(e, 0);
	FreeList[0] = false; //since the block zero is never free
	
	//creating the freelist of 256 blocks skipping block 0
	int i;
	for(i = 1; i < 256; i++)
	{
		FreeList[i] = true; //set all to true initially
	}

	int j;
	//32 for 32 extents in block 0
	for(i = 0;  i < 32; i++)
	{
		if(*(e + i * 32) != 0xe5) //if the extent is unused then pass by it, we want used
		{
			for(j = 0; j < 16; j++) //16 since there are 16 blocks per extent
			{
				int flindex = *(e + i * 32 + 16 + j);
				FreeList[flindex] = false; //now it is used
			}
		}
	}

	
}

// debugging function, print out the contents of the free list in 16 rows of 16, with each 
// row prefixed by the 2-digit hex address of the first block in that row. Denote a used
// block with a *, a free block with a .
void printFreeList()
{
	printf("FREE BLOCK LIST: (* means in-use)\n"); //as specified in our output file from the instructions
	int i;
	int j;
	
	//The FreeList is techincally a 16x16 matrix of 256 blocks so we do 2 for loops of 16 and 16	

	for(i = 0; i < 16; i++)
	{
		printf("%2X", i * 16); //https://gamedev.net/forums/topic/313486-quick-way-to-printf-a-2-byte-hex-characters/3006627/ &&  https://stackoverflow.com/questions/18438946/format-specifier-02x
		for(j = 0; j < 16; j++)
		{
			if(FreeList[i * 16 + j] == false)
			{
				printf(" * "); //used
			}
			
			else
			{
				printf(" . "); //free
			}
		}
		
		printf("\n"); //space them out so it looks correct
	}
}

// print the file directory to stdout. Each filename should be printed on its own line, 
// with the file size, in base 10, following the name and extension, with one space between
// the extension and the size. If a file does not have an extension it is acceptable to print
// the dot anyway, e.g. "myfile. 234" would indicate a file whose name was myfile, with no 
// extension and a size of 234 bytes. This function returns no error codes, since it should
// never fail unless something is seriously wrong with the disk 
void cpmDir()
{
	printf("DIRECTORY LISTING\n"); //as specified in our output file from the instructions
	uint8_t e[1024]; //since there are 1024 bytes per block
	blockRead(e, 0);
	DirStructType *d;
	
	//taken from Professor Xiao's pseudocode
	int index;
	int b_index;
	for(index = 0; index < 32; index++)
	{		
		if(*(e + index * 32) != 0xe5) //0xe5 is unused
		{
			d = mkDirStruct(index , e);
			int block_number = 0;
			for(b_index = 0; b_index < 16; b_index++)
			{
				if(d->blocks[b_index] != 0)
				{
					block_number++;
				}
			}
			
			//the last block is 100% used 
			int file_length;
			if((d->RC = 0) && (d->BC == 0)) //"a bug" as said in the lecture
			{
				printf("Error: RC & BC are both 0");
				exit(1);
			}
			
			else
			{
				file_length = (block_number-1) *1024 + d->RC * 128 + d->BC; //calculates the correct file length
			}

			fprintf(stdout, "%s.%s %d\n", d->name, d->extension, file_length); //ex file.txt 10245
		}
	}
}

// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
bool checkLegalName(char *name)
{
	//need a pointer to file name and their extensions to check
	//need a pointer to file name and their extensions to check
	char *fname = "";
	char *fextension = "";
	char copy[100]; //creating this as a placeholder for name
	strcpy(copy, name);

	//Checking if the File's name or extension are NULL, we don't care about NULL
	fname = strtok(copy, "."); //broke up copy with deliminator "."

	//if filename is null
	if(fname == NULL || fname == "")
	{
		return false; //error because the name is empty or NULL
	}

	fname = fname + '\0'; //as per the class lecture 
	fextension = strtok(0, "."); //extension is after the "." deliminator
	//if extention is null
	if(fextension == NULL || fextension == "")
	{
		return false; //error because the ext is empty or NULL
	}
		
	fextension = fextension + '\0'; //as per the class lecture 

	//Checking if the filename or extension are too long or not at all
	if((strlen(fname) == 0) || (strlen(fname) > 8) || (strlen(fextension) > 3))
	{
		//ERROR
		return false;
	}

	//Checking if there are invalid characters
	int i;
	for(i = 0; i < strlen(fname); i++)
	{
		int character = (int) fname[i]; //have to type cast for it to work properly
		if(!isalnum(character)) //checking for invalid character, dont want anything besides numbers and letters (alphabet)
		{
			return false; 
		}
	}
	
	return true; //if there are no errors that are found, valid name
}

// internal function, returns -1 for illegal name or name not found
// otherwise returns extent nunber 0-31
int findExtentWithName(char *name, uint8_t *block0)
{
	//if illegal name
	if(checkLegalName(name) == false)
	{
		return -1;
	}	

	char *fname = "";
	char *fextension = "";
	char copy[100]; //creating this as a placeholder for name
	strcpy(copy, name);
	fname = strtok(copy, ".");
	fname = fname + '\0';
	fextension = strtok(0, ".");
	fextension = fextension + '\0';

	//now find the extent based on the file name 
	//32 for 32 extents
	int i; 
	DirStructType *d;
	for(i = 0; i < 32; i++)
	{
		d = mkDirStruct(i, block0);
		if((!strcmp(d->name, fname)) && (!strcmp(d->extension, fextension)) && (d->status != 0xe5)) //if a match, then return extent
		{
			return i;
		}
	}
	
	
	//else you find nothing
	return -1;
}

//read directory block, 
// modify the extent for file named oldName with newName, and write to the disk
int cpmRename(char *oldName, char * newName)
{
	/* 0 denotes no error
	   1 denotes same name
	   2 denotes an error*/

	uint8_t e[1024]; //since 1024 bytes per block
	blockRead(e, 0); //read block 0
	
	//Checking if the name is valid or not
	if(checkLegalName(newName) == false)
	{
		return 2;
	}
	
	//checking if the names are the same
	if(!strcmp(oldName, newName))
	{
		return 1;
	}

	//finding the oldName file extent, if return is -1 , found nothing
	if(findExtentWithName(oldName, e) == -1)
	{
		return 2;
	}

	//if file already exists
	if(findExtentWithName(newName, e) != -1)
	{
		return 2;
	}	

	char *fname = "";
	char *fextension = "";
	char copy[100]; //creating this as a placeholder for name
	strcpy(copy, newName);
	fname = strtok(copy, ".");
	fname = fname + '\0';
	fextension = strtok(0, ".");
	fextension = fextension + '\0';

	//Now we can replace the name
	int extent = findExtentWithName(oldName, e); //the old extent
	DirStructType *d = mkDirStruct(extent, e);
	//now just copy
	strcpy(d->name, fname); //swap names
	strcpy(d->extension, fextension); //swap extensions

	//there is going to be an error if we dont type cast this correctly with uint8_t
	//when we write we need to type cast like this
	writeDirStruct(d, (uint8_t) extent, e); //have to make sure to write it or else the change is useless
	blockWrite(e, 0);
	
	return 0;
}

// delete the file named name, and free its disk blocks in the free list
int  cpmDelete(char * name)
{
	/* 0 denotes no error
	   1 denotes same name
	   2 denotes an error*/

	uint8_t e[1024]; //since there are 1024 bytes per block
	blockRead(e, 0);

	//find the extent
	int extent = findExtentWithName(name, e);
	if(extent == -1) //found no name or error
	{
		return 2;
	}

	DirStructType *d = mkDirStruct(extent, e);
	//only way to delete is to make the status equal to 0xe5 (unused)
	d->status = 0xe5;

	//now we have to make all the blocks associated with this in the freelist... free
	int i;
	for(i = 0; i < BLOCKS_PER_EXTENT; i++)
	{
		int index = d->blocks[i];
		if(index != 0) //only care about the ones that are not zero, so the used ones
		{
			FreeList[index] = true; //change to true which means free
		}
	}

	//there is going to be an error if we dont type cast this correctly with uint8_t
	//when we write we need to type cast like this
	writeDirStruct(d, (uint8_t) extent, e);//have to make sure to write it or the delete is useless
	blockWrite(e, 0);
	return 0;
}




