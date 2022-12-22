///////////////////////////////////////
/// Audio
/// compile with
/// gcc audio.c -o audio -lm
///////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/mman.h>
#include <time.h>
#include "address_map_arm_brl4.h"

#define float2fix30(a) ((int)((a)*1073741824)) // 2^30
#define SWAP(X,Y) do{int temp=X; X=Y; Y=temp;}while(0) 

// audio connections
volatile unsigned int * audio_base_ptr = NULL ;
volatile unsigned int * audio_fifo_data_ptr = NULL ; //4bytes
volatile unsigned int * audio_left_data_ptr = NULL ; //8bytes
volatile unsigned int * audio_right_data_ptr = NULL ; //12bytes

// phase accumulator
unsigned int phase_acc, dds_incr ;
int sine_table[256];

// tones in Hz
float notes[9] = {262, 294, 330, 349, 392, 440, 494, 523, 0};
clock_t note_time ;

// the light weight bus base
void *h2p_lw_virtual_base;

// /dev/mem file descriptor
int fd;

// shared memory 
key_t mem_key=0xf0;
int shared_mem_id; 
int *shared_ptr;
int audio_time;
 
int main(void)
{
	

	// === get FPGA addresses ==================
    // Open /dev/mem
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) 	
	{
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}
    
    // get virtual addr that maps to physical
	h2p_lw_virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );	
	if( h2p_lw_virtual_base == MAP_FAILED ) 
	{
		printf( "ERROR: mmap1() failed...\n" );
		close( fd );
		return(1);
	}
    
	// audio addresses
	audio_base_ptr = (unsigned int *)(h2p_lw_virtual_base + AUDIO_BASE);
	audio_fifo_data_ptr  = audio_base_ptr  + 1 ; // word
	audio_left_data_ptr = audio_base_ptr  + 2 ; // words
	audio_right_data_ptr = audio_base_ptr  + 3 ; // words

	// build the DDS sine table
	int i;
	for(i=0; i<256; i++)
		sine_table[i] = float2fix30(sin(6.28*(float)i/256));
	dds_incr = 0 ;
	
	note_time = clock();
	i = 0 ;

	int arr[24]; 
    int j; 
    printf("%s" "\n" , "____________________________");
  	printf("%s" "\n", "|  | | | |  |  | | | | | |  |");
  	printf("%s" "\n", "|  | | | |  |  | | | | | |  |");
  	printf("%s" "\n", "|  | | | |  |  | | | | | |  |");
  	printf("%s" "\n", "|  |_|_|_|  |  |_|_|_|_|_|  |");
  	printf("%s" "\n", "|   |   |   |   |   |   |   |");
  	printf("%s" "\n", "| C | D | E | F | G | A | B |");
  	printf("%s" "\n", "| 0 | 1 | 2 | 3 | 4 | 5 | 6 |");
  	printf("%s" "\n", "|___|___|___|___|___|___|___|"); 
    printf("\n\nWelcome! Create your own song or pick one from the menu by entering the notes below:\n");
    printf("*(Use numbers from 0-6, use 9 for blank)*\n\n");
    printf("Happy Birthday to You! :        0,9,0,1,0,3,2,9,0,9,0,1,0,4,3,9,0,9,0,5,4,2,1,0.\n");
    printf("Hot Cross Buns :                2,1,0,2,1,0,9,0,9,0,9,0,9,0,1,9,1,9,1,9,1,2,1,0.\n");
    printf("We Wish You a Merry Christmas : 0,3,9,3,4,3,2,1,9,1,9,1,4,9,4,5,4,3,2,9,1,9,1,9.\n");
    printf("Rudolph the Red Nose Reindeer : 4,5,4,2,6,5,4,9,4,5,4,5,4,7,6,9,3,4,3,1,6,5,4,9.\n");
    printf("--------------------------------------------------------------------------------\n");	
  
    printf("Input 24 notes :\n");  
    for(j=0; j<24; j++)  
    {  
	    printf("Note - %d : ",j+1);
        scanf("%d", &arr[j]);  
    }  
  
    printf("\nNotes in song are: ");  
    for(j=0; j<25; j++)  
    {  
        printf("%d  ", arr[j]);  
    } 
    printf("\n");	



	while(1){	

		// generate a sine wave
		int s_index ;
		// load the FIFO until it is full
		while (((*audio_fifo_data_ptr>>24)& 0xff) > 1) {
			phase_acc = (phase_acc + dds_incr);
			s_index = phase_acc>>24;
			*audio_left_data_ptr = sine_table[s_index];
			*audio_right_data_ptr = sine_table[s_index];
			// share the time
			audio_time++ ;
			*shared_ptr = audio_time/48000 ;
		} // end while 
		if (clock()- note_time > 1000000) {
			dds_incr = (int) (notes[arr[j]] * 89478.5 );
			*(shared_ptr+1) = notes[arr[j]];
			j++;
			if (j>24) j = 0;
			note_time = clock();
		}
	} // end while(1)
} // end main