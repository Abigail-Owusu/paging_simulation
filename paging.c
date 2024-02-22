#include <stdio.h>
#include <stdlib.h>

#define VIRTUAL_MEMORY_SIZE 1024 // 1024 bytes
#define PHYSICAL_MEMORY_SIZE 256 // 256 bytes
#define PAGE_SIZE 4              // 4 bytes

typedef struct
{
    int frame_number; // Stores the frame number in physical memory
    int valid;        // Valid bit indicates whether a frame is present in physical memory
} PageTableEntry;

unsigned char VIRTUAL_MEMORY[VIRTUAL_MEMORY_SIZE];
unsigned char PHYSICAL_MEMORY[PHYSICAL_MEMORY_SIZE];
PageTableEntry *PAGE_TABLE;
int page_table_size = VIRTUAL_MEMORY_SIZE / PAGE_SIZE;

void initializePageTable()
{
    PAGE_TABLE = (PageTableEntry *)malloc(page_table_size * sizeof(PageTableEntry));

    if (PAGE_TABLE == NULL)
    {
        printf("Memory allocation for page table failed.");
        exit(0);
    }

    // Initializing page table with no pages in memory
    for (int i = 0; i < page_table_size; i++)
    {
        PAGE_TABLE[i].valid = 0;
    }
}

/**
 * Printing out the page table for visualization purposes
 */
void printPageTable()
{
    printf("Page Table:\n");
    printf("Page\tFrame\tValid\n");
    for (int i = 0; i < page_table_size; ++i)
    {
        printf("%d\t%d\t%s\n", i, PAGE_TABLE[i].frame_number,
               (PAGE_TABLE[i].valid) ? "Yes" : "No");
    }
    printf("\n");
}

void allocatePage(int virtual_page_number)
{
    // Checking if the page table is full and dynamically allocating more space

    if (virtual_page_number >= page_table_size)
    {
        // Resizing the page table
        PAGE_TABLE = (PageTableEntry *)realloc(PAGE_TABLE, (page_table_size * 2) * sizeof(PageTableEntry));
        
        //Checking if memory allocation was successful
        if (PAGE_TABLE == NULL)
        {
            printf("Memory allocation for page table failed.");
            exit(0);
        }

        for (int i = page_table_size; i < (page_table_size * 2); i++)
        {
            PAGE_TABLE[i].valid = 0;
        }

        page_table_size = page_table_size * 2;
    }

    //Checking for free frames and allocating in physical memory
    for (int i = 0; i < PHYSICAL_MEMORY_SIZE / PAGE_SIZE; i++){
        // Checking if frame is free
        if (!PAGE_TABLE[i].valid){
            PAGE_TABLE[i].frame_number = virtual_page_number;
            PAGE_TABLE[i].valid = 1;
            return;

        }
    }
}

int translateAddress(int virtualAddress){
    //Calculating the page number by dividing the address by the page size.
    int virtual_page_number = virtualAddress / PAGE_SIZE;


    //Checking if a frame is present or occupied in physical memory
    if (PAGE_TABLE[virtual_page_number].valid){
        int frame_number = PAGE_TABLE[virtual_page_number].frame_number;
        int offset = virtualAddress % PAGE_SIZE;
        int physical_address = frame_number *  PAGE_SIZE * 


    }
}

int main()
{
    initializePageTable();
    printPageTable();
}