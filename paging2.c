#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <stdbool.h>

#define VIRTUAL_MEMORY_SIZE 1024
#define PHYSICAL_MEMORY_SIZE 256
#define PAGE_SIZE 4
#define PAGE_TABLE_SIZE 1024


typedef struct FrameNode
{
    int frame_number;
    struct FrameNode *next;
} FrameNode;

typedef struct PageTableEntry
{
    FrameNode *frames;
    bool valid;
    struct PageTableEntry *next;
} PageTableEntry;

typedef struct
{
    bool allocated;
} Frame;

unsigned char virtual_memory[VIRTUAL_MEMORY_SIZE];
unsigned char physical_memory[PHYSICAL_MEMORY_SIZE];
PageTableEntry *page_table[PAGE_TABLE_SIZE];


Queue *fifo_queue;
int num_frames;
int fifo_queue_size = 0;
int page_faults = 0;
int hits = 0;

// Define the node structure for the queue
typedef struct QueueNode
{
    int frame_number;
    struct QueueNode *next;
} QueueNode;

// Define the queue structure
typedef struct
{
    QueueNode *front;
    QueueNode *rear;
    int size;
} Queue;

// Function to initialize an empty queue
Queue *createQueue()
{
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (queue == NULL)
    {
        // Handle memory allocation error
        exit(EXIT_FAILURE);
    }
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return queue;
}

// Function to check if the queue is empty
bool isEmpty(Queue *queue)
{
    return (queue->front == NULL);
}

// Function to enqueue an element into the queue
void enqueue(Queue *queue, int frame_number)
{
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (new_node == NULL)
    {
        // Handle memory allocation error
        exit(EXIT_FAILURE);
    }
    new_node->frame_number = frame_number;
    new_node->next = NULL;
    if (isEmpty(queue))
    {
        queue->front = new_node;
    }
    else
    {
        queue->rear->next = new_node;
    }
    queue->rear = new_node;
    queue->size++;
}

// Function to dequeue an element from the queue
int dequeue(Queue *queue)
{
    if (isEmpty(queue))
    {
        // Handle underflow error
        exit(EXIT_FAILURE);
    }
    int frame_number = queue->front->frame_number;
    QueueNode *temp = queue->front;
    queue->front = queue->front->next;
    free(temp);
    queue->size--;
    return frame_number;
}

// Function to free memory allocated for the queue
void freeQueue(Queue *queue)
{
    while (!isEmpty(queue))
    {
        dequeue(queue);
    }
    free(queue);
}

void initializeMemorySystem();
int calculateHash(int virtual_address);
int translateAddress(int virtual_address);
void allocatePage(int virtual_page_number);
void deallocatePage(int virtual_page_number);
void handlePageFault(int virtual_page_number);
void handlePageReplacement(int virtual_page_number);
void simulateMemoryAccess();
void displayMemoryStatistics();
void freeMemory();
void printPhysicalMemory();
void displayVirtualMemory();

void initializeMemorySystem()
{
    num_frames = PHYSICAL_MEMORY_SIZE / PAGE_SIZE;
    frames = (Frame *)malloc(num_frames * sizeof(Frame));
    if (frames == NULL)
    {
        fprintf(stderr, "Memory allocation failed for frames.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_frames; ++i)
    {
        frames[i].allocated = false;
    }

    for (int i = 0; i < VIRTUAL_MEMORY_SIZE; ++i)
    {
        virtual_memory[i] = rand() % 256;
    }

    srand(time(NULL));
}

int calculateHash(int virtual_address)
{
    return virtual_address % PAGE_TABLE_SIZE;
}

bool frameExists(FrameNode *frames, int virtual_page_number)
{
    FrameNode *current_frame = frames;
    while (current_frame != NULL)
    {
        if (current_frame->frame_number == virtual_page_number)
        {
            return true;
        }
        current_frame = current_frame->next;
    }
    return false;
}

int translateAddress(int virtual_address)
{
    int virtual_page_number = virtual_address / PAGE_SIZE;
    int offset = virtual_address % PAGE_SIZE;

    int hash = calculateHash(virtual_page_number);
    PageTableEntry *entry = page_table[hash];
    while (entry != NULL)
    {
        if (entry->valid)
        {
            FrameNode *current_frame = entry->frames;
            while (current_frame != NULL)
            {
                if (current_frame->frame_number == virtual_page_number)
                {
                    hits++;
                    return current_frame->frame_number * PAGE_SIZE + offset;
                }
                current_frame = current_frame->next;
            }
        }
        entry = entry->next;
    }
    printPhysicalMemory();

    handlePageFault(virtual_page_number);
    printPhysicalMemory();
    page_faults++;
    return -1;
}

void allocatePage(int virtual_page_number)
{
    int hash = calculateHash(virtual_page_number);
    PageTableEntry *entry = page_table[hash];

    // Check if the page is already in physical memory
    while (entry != NULL)
    {
        if (entry->valid)
        {
            FrameNode *current_frame = entry->frames;
            while (current_frame != NULL)
            {
                if (current_frame->frame_number == virtual_page_number)
                {
                    // Page is already in physical memory
                    return;
                }
                current_frame = current_frame->next;
            }
        }
        entry = entry->next;
    }

    // Find a free frame in physical memory
    for (int i = 0; i < num_frames; ++i)
    {
        if (!frames[i].allocated)
        {
            // Allocate the frame
            frames[i].allocated = true;

            // Create a new frame node
            FrameNode *new_frame = (FrameNode *)malloc(sizeof(FrameNode));
            if (new_frame == NULL)
            {
                fprintf(stderr, "Memory allocation failed for frame node.\n");
                exit(EXIT_FAILURE);
            }
            new_frame->frame_number = virtual_page_number;
            new_frame->next = NULL;

            // Create a new page table entry
            PageTableEntry *new_entry = (PageTableEntry *)malloc(sizeof(PageTableEntry));
            if (new_entry == NULL)
            {
                fprintf(stderr, "Memory allocation failed for page table entry.\n");
                exit(EXIT_FAILURE);
            }
            new_entry->frames = new_frame;
            new_entry->valid = true;

            // Insert the new entry into the hash table
            new_entry->next = page_table[hash];
            page_table[hash] = new_entry;

            return;
        }
    }

    // If no free frames are available, handle page replacement
    handlePageReplacement(virtual_page_number);
}

void deallocatePage(int virtual_page_number)
{
    int hash = calculateHash(virtual_page_number);
    PageTableEntry *entry = page_table[hash];
    PageTableEntry *prev_entry = NULL;

    while (entry != NULL)
    {
        if (entry->valid)
        {
            FrameNode *current_frame = entry->frames;
            FrameNode *prev_frame = NULL;
            while (current_frame != NULL)
            {
                if (current_frame->frame_number == virtual_page_number)
                {
                    current_frame->frame_number = false;
                    free(current_frame);
                    if (prev_frame != NULL)
                    {
                        prev_frame->next = current_frame->next;
                    }
                    else
                    {
                        entry->frames = current_frame->next;
                    }
                    if (entry->frames == NULL)
                    {
                        entry->valid = false;
                    }
                    return;
                }
                prev_frame = current_frame;
                current_frame = current_frame->next;
            }
        }
        prev_entry = entry;
        entry = entry->next;
    }
}

void printPhysicalMemory()
{
    printf("Physical Memory:\n");
    for (int i = 0; i < PHYSICAL_MEMORY_SIZE; ++i)
    {
        printf("%d ", physical_memory[i]);
    }
    printf("\n");
}

void handlePageFault(int virtual_page_number)
{
    printf("Page fault! Fetching page %d from secondary storage.\n", virtual_page_number);
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        physical_memory[virtual_page_number * PAGE_SIZE + i] = virtual_memory[virtual_page_number * PAGE_SIZE + i];
    }
    allocatePage(virtual_page_number);
}

// void handlePageReplacement(int virtual_page_number)
// {
//     printf("Page replacement! Replacing page %d in physical memory.\n", virtual_page_number);
//     // Implement page replacement algorithm
//     // FIFO

//     // retrieves the frame number of the oldest page in physical memory according to the FIFO policy.
//     int frame_number = dequeue(fifo_queue);
//     deallocatePage(frame_number);
//     for (int i = 0; i < PAGE_SIZE; ++i)
//     {
//         physical_memory[frame_number * PAGE_SIZE + i] = virtual_memory[virtual_page_number * PAGE_SIZE + i];
//     }
//     allocatePage(virtual_page_number);
//     enqueue(fifo_queue, frame_number);
// }

void handlePageReplacement(int virtual_page_number)
{
    // Find the oldest page using the FIFO queue
    int oldest_frame_number = dequeue(fifo_queue);

    // Deallocate the oldest page
    deallocatePage(oldest_frame_number);

    // Allocate the new page in physical memory
    allocatePage(virtual_page_number);
}

void simulateMemoryAccess()
{
    int virtual_address = rand() % VIRTUAL_MEMORY_SIZE;
    int physical_address = translateAddress(virtual_address);

    if (physical_address != -1)
    {
        printf("Virtual Address: %d => Physical Address: %d\n", virtual_address, physical_address);
    }
    else
    {
        printf("Virtual Address: %d => Page fault occurred!\n", virtual_address);
    }
}

void displayMemoryStatistics()
{
    printf("\nMemory Statistics:\n");
    printf("Page Faults: %d\n", page_faults);
    printf("Hits: %d\n", hits);
    printf("Hit Rate: %.2f%%\n", (hits / (float)(hits + page_faults)) * 100);
}

void freeMemory()
{
    for (int i = 0; i < PAGE_TABLE_SIZE; ++i)
    {
        PageTableEntry *entry = page_table[i];
        while (entry != NULL)
        {
            FrameNode *current_frame = entry->frames;
            while (current_frame != NULL)
            {
                FrameNode *temp = current_frame;
                current_frame = current_frame->next;
                free(temp);
            }
            PageTableEntry *temp_entry = entry;
            entry = entry->next;
            free(temp_entry);
        }
    }
    free(frames);
}

//display content in virtual memory
void displayVirtualMemory()
{
    printf("Virtual Memory:\n");
    for (int i = 0; i < VIRTUAL_MEMORY_SIZE; ++i)
    {
        printf("%d ", virtual_memory[i]);
    }
    printf("\n");
}

int main()
{
    Queue *fifo_queue = createQueue(PHYSICAL_MEMORY_SIZE / PAGE_SIZE);
    initializeMemorySystem();

    // Simulate processes accessing memory
    for (int i = 0; i < 2000; ++i)
    {
        simulateMemoryAccess();
    }
    // displayVirtualMemory();

    // printPhysicalMemory();

    // Display memory statistics
    displayMemoryStatistics();

    // Free allocated memory
    freeMemory();

    return 0;
}
