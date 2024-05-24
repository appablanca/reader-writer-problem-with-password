#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

#define MAX_THREADS 9
#define PASSWORD_LEN 6
#define BUFFER_SIZE 1024

typedef struct
{
    int id;
    char password[PASSWORD_LEN + 1];
    int is_real;
} ThreadInfo;

sem_t rw_mutex;     // Semaphore for writer
sem_t mutex;        // Semaphore for reader count
int read_count = 0; // Number of readers
int BUFFER = 0;     // Shared resource
ThreadInfo readers[MAX_THREADS];
ThreadInfo writers[MAX_THREADS];

void *reader(void *param);
void *writer(void *param);
void init_passwords(char passwords[][PASSWORD_LEN + 1], int size);
int check_password(const char *password, char passwords[][PASSWORD_LEN + 1], int size);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <number_of_readers> <number_of_writers>\n", argv[0]);
        return 1;
    }

    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);

    if (num_readers < 1 || num_readers > MAX_THREADS || num_writers < 1 || num_writers > MAX_THREADS || (num_readers + num_writers > MAX_THREADS))
    {
        fprintf(stderr, "The number of readers and writers should be between 1 and 9, and their total should not exceed 9.\n");
        return 1;
    }

    char passwords[10][PASSWORD_LEN + 1];
    init_passwords(passwords, 10);

    pthread_t r_threads[MAX_THREADS], w_threads[MAX_THREADS];
    int i;

    sem_init(&rw_mutex, 0, 1);
    sem_init(&mutex, 0, 1);

    // Initialize readers
    for (i = 0; i < num_readers; i++)
    {
        readers[i].id = i;
        strcpy(readers[i].password, passwords[i]);
        readers[i].is_real = 1;
        pthread_create(&r_threads[i], NULL, reader, (void *)&readers[i]);
    }
    for (i = num_readers; i < 2 * num_readers; i++)
    {
        readers[i].id = i;
        snprintf(readers[i].password, PASSWORD_LEN + 1, "%06d", rand() % 1000000);
        readers[i].is_real = 0;
        pthread_create(&r_threads[i], NULL, reader, (void *)&readers[i]);
    }

    // Initialize writers
    for (i = 0; i < num_writers; i++)
    {
        writers[i].id = i;
        strcpy(writers[i].password, passwords[i + num_readers]);
        writers[i].is_real = 1;
        pthread_create(&w_threads[i], NULL, writer, (void *)&writers[i]);
    }
    for (i = num_writers; i < 2 * num_writers; i++)
    {
        writers[i].id = i;
        snprintf(writers[i].password, PASSWORD_LEN + 1, "%06d", rand() % 1000000);
        writers[i].is_real = 0;
        pthread_create(&w_threads[i], NULL, writer, (void *)&writers[i]);
    }

    // Join reader threads
    for (i = 0; i < 2 * num_readers; i++)
    {
        pthread_join(r_threads[i], NULL);
    }

    // Join writer threads
    for (i = 0; i < 2 * num_writers; i++)
    {
        pthread_join(w_threads[i], NULL);
    }

    sem_destroy(&rw_mutex);
    sem_destroy(&mutex);

    return 0;
}

void *reader(void *param)
{
    ThreadInfo *info = (ThreadInfo *)param;

    for (int i = 0; i < 5; i++)
    {
        if (!info->is_real || !check_password(info->password, passwords, 10))
        {
            printf("Thread %d Invalid (dummy) Reader %d attempting to read\n", info->id, info->id);
            sleep(1);
            continue;
        }

        sem_wait(&mutex);
        read_count++;
        if (read_count == 1)
        {
            sem_wait(&rw_mutex); // Lock the resource if first reader
        }
        sem_post(&mutex);

        // Reading section
        printf("Thread %d Valid Reader %d reads value %d\n", info->id, info->id, BUFFER);
        sleep(1);

        // Release read access
        sem_wait(&mutex);
        read_count--;
        if (read_count == 0)
        {
            sem_post(&rw_mutex); // Release the resource if last reader
        }
        sem_post(&mutex);

        sleep(1); // Simulating time between reads
    }
    pthread_exit(0);
}

void *writer(void *param)
{
    ThreadInfo *info = (ThreadInfo *)param;

    for (int i = 0; i < 5; i++)
    {
        if (!info->is_real || !check_password(info->password, passwords, 10))
        {
            printf("Thread %d Invalid (dummy) Writer %d attempting to write\n", info->id, info->id);
            sleep(1);
            continue;
        }

        sem_wait(&rw_mutex);

        // Writing section
        BUFFER = rand() % 10000;
        printf("Thread %d Valid Writer %d writes value %d\n", info->id, info->id, BUFFER);
        sleep(1);

        sem_post(&rw_mutex);

        sleep(3); // Simulating time between writes
    }
    pthread_exit(0);
}

void init_passwords(char passwords[][PASSWORD_LEN + 1], int size)
{
    for (int i = 0; i < size; i++)
    {
        snprintf(passwords[i], PASSWORD_LEN + 1, "%06d", rand() % 1000000);
    }
}

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
