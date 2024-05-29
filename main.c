#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

// Constants
#define MAX_THREADS 10
#define PASSWORD_LEN 6

// Structure to store thread information
typedef struct
{
    int id;                     // Thread ID
    char password[PASSWORD_LEN + 1]; // Password
    int is_real;                // Flag to indicate if the thread is real (1) or dummy (0)
    int role;                   // Role: 0 for reader, 1 for writer
} ThreadInfo;

// Semaphores and shared variables
sem_t rw_mutex;       // Semaphore for writer
sem_t mutex;          // Semaphore for reader count
int read_count = 0;   // Number of readers
int BUFFER = 0;       // Shared resource
ThreadInfo readers[MAX_THREADS * 2]; // Array to store reader thread info
ThreadInfo writers[MAX_THREADS * 2]; // Array to store writer thread info

// Function declarations
void *reader(void *param);
void *writer(void *param);
void init_passwords(char passwords[][PASSWORD_LEN + 1], int size);
int check_password(const char *password, char passwords[][PASSWORD_LEN + 1], int size);

// Array to store passwords
char passwords[10][PASSWORD_LEN + 1];

int main(int argc, char *argv[])
{
    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);

    // Initialize passwords
    init_passwords(passwords, 10);

    pthread_t r_threads[MAX_THREADS * 2], w_threads[MAX_THREADS * 2];
    int i;

    // Initialize semaphores
    sem_init(&rw_mutex, 0, 1);
    sem_init(&mutex, 0, 1);

    printf("Number of readers: %d\n", num_readers);
    printf("Number of writers: %d\n\n", num_writers);
    printf("Thread No  Validity(real/dummy)  Role(reader/writer)  Value read/written\n");

    // Initialize real reader threads
    for (i = 0; i < num_readers; i++)
    {
        readers[i].id = i + 1;
        strcpy(readers[i].password, passwords[i]);
        readers[i].is_real = 1;
        readers[i].role = 0;
        pthread_create(&r_threads[i], NULL, reader, (void *)&readers[i]);
    }
    
    // Initialize dummy reader threads
    for (i = num_readers; i < 2 * num_readers; i++)
    {
        readers[i].id = i - num_readers + 1;
        snprintf(readers[i].password, PASSWORD_LEN + 1, "%06d", rand() % 1000000);
        readers[i].is_real = 0;
        readers[i].role = 0;
        pthread_create(&r_threads[i], NULL, reader, (void *)&readers[i]);
    }

    // Initialize real writer threads
    for (i = 0; i < num_writers; i++)
    {
        writers[i].id = i + 1;
        strcpy(writers[i].password, passwords[i + num_readers]);
        writers[i].is_real = 1;
        writers[i].role = 1;
        pthread_create(&w_threads[i], NULL, writer, (void *)&writers[i]);
    }
    
    // Initialize dummy writer threads
    for (i = num_writers; i < 2 * num_writers; i++)
    {
        writers[i].id = i - num_writers + 1;
        snprintf(writers[i].password, PASSWORD_LEN + 1, "%06d", rand() % 1000000);
        writers[i].is_real = 0;
        writers[i].role = 1;
        pthread_create(&w_threads[i], NULL, writer, (void *)&writers[i]);
    }

    // Wait for all reader threads to finish
    for (i = 0; i < 2 * num_readers; i++)
    {
        pthread_join(r_threads[i], NULL);
    }

    // Wait for all writer threads to finish
    for (i = 0; i < 2 * num_writers; i++)
    {
        pthread_join(w_threads[i], NULL);
    }

    // Cleanup semaphores
    sem_destroy(&rw_mutex);
    sem_destroy(&mutex);

    return 0;
}

// Reader thread function
void *reader(void *param)
{
    ThreadInfo *info = (ThreadInfo *)param;

    for (int i = 0; i < 5; i++)
    {
        // Check if thread is real and has valid password
        if (!info->is_real || !check_password(info->password, passwords, 10))
        {
            printf("%d          %s                    reader              No permission\n", info->id, info->is_real ? "real" : "dummy");
            sleep(1);
            continue;
        }

        // Enter critical section for readers
        sem_wait(&mutex);
        read_count++;
        if (read_count == 1)
        {
            sem_wait(&rw_mutex);
        }
        sem_post(&mutex);

        // Reading shared resource
        printf("%d          real                    reader              %d\n", info->id, BUFFER);

        // Exit critical section for readers
        sem_wait(&mutex);
        read_count--;
        if (read_count == 0)
        {
            sem_post(&rw_mutex);
        }
        sem_post(&mutex);

        sleep(2); // Simulate time between reads
    }
    pthread_exit(0);
}

// Writer thread function
void *writer(void *param)
{
    ThreadInfo *info = (ThreadInfo *)param;

    for (int i = 0; i < 5; i++)
    {
        // Check if thread is real and has valid password
        if (!info->is_real || !check_password(info->password, passwords, 10))
        {
            printf("%d          %s                    writer              No permission\n", info->id, info->is_real ? "real" : "dummy");
            sleep(1);
            continue;
        }

        // Enter critical section for writers
        sem_wait(&rw_mutex);

        // Writing to shared resource
        BUFFER = rand() % 420;
        printf("%d          real                    writer              %d\n", info->id, BUFFER);

        // Exit critical section for writers
        sem_post(&rw_mutex);

        sleep(2); // Simulate time between writes
    }
    pthread_exit(0);
}

// Function to initialize passwords
void init_passwords(char passwords[][PASSWORD_LEN + 1], int size)
{
    for (int i = 0; i < size; i++)
    {
        snprintf(passwords[i], PASSWORD_LEN + 1, "%06d", rand() % 42000);
    }
}

// Function to check if a password is valid
int check_password(const char *password, char passwords[][PASSWORD_LEN + 1], int size)
{
    for (int i = 0; i < size; i++)
    {
        if (strcmp(password, passwords[i]) == 0)
        {
            return 1;
        }
    }
    return 0;
}
