#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// Constants
#define VIRTUAL_MEMORY_SIZE 1024 // Total size of virtual memory in bytes
#define PHYSICAL_MEMORY_SIZE 256 // Total size of physical memory in bytes
#define PAGE_SIZE 4              // Size of each page in bytes

// Data structures
typedef struct
{
    int frame_number; // Frame number in physical memory
    bool valid;       // Valid bit to indicate if the page is in physical memory
    int arrival_time; // Timestamp of page arrival (for FIFO)
} PageTableEntry;

typedef struct
{
    bool allocated; // Indicates whether the frame is allocated
} Frame;

// Global variables
unsigned char virtual_memory[VIRTUAL_MEMORY_SIZE];
unsigned char virtual_storage[VIRTUAL_MEMORY_SIZE]; // Represents virtual memory contents
unsigned char physical_memory[PHYSICAL_MEMORY_SIZE];
PageTableEntry *page_table;
Frame *frames;
int num_pages;
int num_frames;
int fifo_queue_size = 0; // Size of the FIFO queue for page replacement

// Statistics variables
int page_faults = 0;
int hits = 0;

//Function declarations
void initializeMemorySystem();
int translateAddress(int virtual_address);
void allocatePage(int virtual_page_number);
void deallocatePage(int virtual_page_number);
void handlePageFault(int virtual_page_number);
void handlePageReplacement(int virtual_page_number);
void simulateMemoryAccess();
void displayMemoryStatistics();
void freeMemory();


// Function to initialize the memory management system
void initializeMemorySystem(){
    // Calculate the number of pages and frames
    num_pages = VIRTUAL_MEMORY_SIZE / PAGE_SIZE;
    num_frames = PHYSICAL_MEMORY_SIZE / PAGE_SIZE;

    // Initialize page table
    page_table = (PageTableEntry *)malloc(num_pages * sizeof(PageTableEntry));
    if (page_table == NULL)
    {
        fprintf(stderr, "Memory allocation failed for page table.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize frames
    frames = (Frame *)malloc(num_frames * sizeof(Frame));
    if (frames == NULL){
        fprintf(stderr, "Memory allocation failed for frames.\n");
        free(page_table);
        exit(EXIT_FAILURE);
    }

    // Initialize page table and frames
    for (int i = 0; i < num_pages; ++i){
        page_table[i].valid = false;
    }

    for (int i = 0; i < num_frames; ++i){
        frames[i].allocated = false;
    }

    // Initialize virtual memory and virtual storage
    for (int i = 0; i < VIRTUAL_MEMORY_SIZE; ++i){
        virtual_memory[i] = rand() % 256; // Initialize with random data
        virtual_storage[i] = virtual_memory[i];
    }

    // Seed for random number generation
    srand(time(NULL));
}

// Function for address translation from virtual to physical addresses
int translateAddress(int virtual_address){
    int virtual_page_number = virtual_address / PAGE_SIZE;
    int offset = virtual_address % PAGE_SIZE;

    if (virtual_page_number >= num_pages || !page_table[virtual_page_number].valid){
        // Page fault
        handlePageFault(virtual_page_number);
        page_faults++;
    }
    else
    {
        // Page is in physical memory (hit)
        hits++;
    }

    int physical_frame_number = page_table[virtual_page_number].frame_number;
    int physical_address = physical_frame_number * PAGE_SIZE + offset;
    return physical_address;
}

// Function for page allocation (simulating memory requests)
void allocatePage(int virtual_page_number){
    // Check if the page is already in physical memory
    if (!page_table[virtual_page_number].valid){
        // Find a free frame
        for (int i = 0; i < num_frames; ++i){
            if (!frames[i].allocated){
                // Allocate the frame
                page_table[virtual_page_number].frame_number = i;
                page_table[virtual_page_number].valid = true;
                frames[i].allocated = true;

                // Enqueue the frame in the FIFO queue
                page_table[virtual_page_number].arrival_time = fifo_queue_size;
                fifo_queue_size++;

                return;
            }
        }

        // Handle page replacement using FIFO
        handlePageReplacement(virtual_page_number);
    }
}

// Function for page deallocation
void deallocatePage(int virtual_page_number){
    if (page_table[virtual_page_number].valid)
    {
        // Free the corresponding frame
        int frame_number = page_table[virtual_page_number].frame_number;
        frames[frame_number].allocated = false;
        page_table[virtual_page_number].valid = false;
    }
}

// Function for handling page faults (simulating fetching the required page from secondary storage)
void handlePageFault(int virtual_page_number){
    // Simulate fetching the required page from secondary storage (virtual memory)
    printf("Page fault! Fetching page %d from secondary storage.\n", virtual_page_number);
    for (int i = 0; i < PAGE_SIZE; ++i){
        virtual_memory[virtual_page_number * PAGE_SIZE + i] = virtual_storage[virtual_page_number * PAGE_SIZE + i];
    }

    // Allocate the page in physical memory
    allocatePage(virtual_page_number);
}

// Function for handling page replacement using FIFO
void handlePageReplacement(int virtual_page_number){
    // Find the oldest page in the FIFO queue
    int oldest_page_index = 0;
    for (int i = 1; i < num_pages; ++i)
    {
        if (page_table[i].arrival_time < page_table[oldest_page_index].arrival_time)
        {
            oldest_page_index = i;
        }
    }

    // Deallocate the oldest page
    deallocatePage(oldest_page_index);

    // Allocate the new page in physical memory
    allocatePage(virtual_page_number);
}

// Function to simulate processes accessing memory by generating random memory addresses
void simulateMemoryAccess()
{
    int virtual_address = rand() % VIRTUAL_MEMORY_SIZE;
    int physical_address = translateAddress(virtual_address);

    printf("Virtual Address: %d => Physical Address: %d\n", virtual_address, physical_address);
}

// Function to display memory statistics
void displayMemoryStatistics()
{
    printf("\nMemory Statistics:\n");
    printf("Page Faults: %d\n", page_faults);
    printf("Hits: %d\n", hits);
    printf("Hit Rate: %.2f%%\n", (hits / (float)(hits + page_faults)) * 100);
}

// Function to free allocated memory
void freeMemory()
{
    free(page_table);
    free(frames);
}

int main()
{
    initializeMemorySystem();

    // Simulate processes accessing memory
    for (int i = 0; i < 20; ++i)
    {
        simulateMemoryAccess();
    }

    // Display memory statistics
    displayMemoryStatistics();

    // Free allocated memory
    freeMemory();

    return 0;
}
