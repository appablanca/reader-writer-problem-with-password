#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <dispatch/dispatch.h>

#define MAX_THREADS 10
#define PASSWORD_LEN 6

typedef struct
{
    int id;
    char password[PASSWORD_LEN + 1];
    int is_real;
    int role; // 0 for reader, 1 for writer
} ThreadInfo;

dispatch_semaphore_t rw_mutex;     // Semaphore for writer
dispatch_semaphore_t mutex;        // Semaphore for reader count
int read_count = 0;                // Number of readers
int BUFFER = 0;                    // Shared resource
ThreadInfo readers[MAX_THREADS * 2];
ThreadInfo writers[MAX_THREADS * 2];

void *reader(void *param);
void *writer(void *param);
void init_passwords(char passwords[][PASSWORD_LEN + 1], int size);
int check_password(const char *password, char passwords[][PASSWORD_LEN + 1], int size);

char passwords[10][PASSWORD_LEN + 1];

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <number_of_readers> <number_of_writers>\n", argv[0]);
        return 1;
    }

    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);

    if (num_readers < 1 || num_readers > MAX_THREADS || num_writers < 1 || num_writers > MAX_THREADS || (num_readers + num_writers) > MAX_THREADS)
    {
        fprintf(stderr, "The number of readers and writers should be between 1 and 10, and their total should not exceed 10.\n");
        return 1;
    }

    init_passwords(passwords, 10);

    pthread_t r_threads[MAX_THREADS * 2], w_threads[MAX_THREADS * 2];
    int i;

    rw_mutex = dispatch_semaphore_create(1);
    mutex = dispatch_semaphore_create(1);

    printf("Number of readers: %d\n", num_readers);
    printf("Number of writers: %d\n\n", num_writers);
    printf("Thread No  Validity(real/dummy)  Role(reader/writer)  Value read/written\n");

    // Initialize readers
    for (i = 0; i < num_readers; i++)
    {
        readers[i].id = i + 1;
        strcpy(readers[i].password, passwords[i]);
        readers[i].is_real = 1;
        readers[i].role = 0;
        pthread_create(&r_threads[i], NULL, reader, (void *)&readers[i]);
    }
    for (i = num_readers; i < 2 * num_readers; i++)
    {
        readers[i].id = i - num_readers + 1;
        snprintf(readers[i].password, PASSWORD_LEN + 1, "%06d", rand() % 1000000);
        readers[i].is_real = 0;
        readers[i].role = 0;
        pthread_create(&r_threads[i], NULL, reader, (void *)&readers[i]);
    }

    // Initialize writers
    for (i = 0; i < num_writers; i++)
    {
        writers[i].id = i + 1;
        strcpy(writers[i].password, passwords[i + num_readers]);
        writers[i].is_real = 1;
        writers[i].role = 1;
        pthread_create(&w_threads[i], NULL, writer, (void *)&writers[i]);
    }
    for (i = num_writers; i < 2 * num_writers; i++)
    {
        writers[i].id = i - num_writers + 1;
        snprintf(writers[i].password, PASSWORD_LEN + 1, "%06d", rand() % 1000000);
        writers[i].is_real = 0;
        writers[i].role = 1;
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

    // Cleanup
    dispatch_release(rw_mutex);
    dispatch_release(mutex);

    return 0;
}

void *reader(void *param)
{
    ThreadInfo *info = (ThreadInfo *)param;

    for (int i = 0; i < 5; i++)
    {
        if (!info->is_real || !check_password(info->password, passwords, 10))
        {
            printf("%d          %s                    reader              No permission\n", info->id, info->is_real ? "real" : "dummy");
            sleep(1);
            continue;
        }

        dispatch_semaphore_wait(mutex, DISPATCH_TIME_FOREVER);
        read_count++;
        if (read_count == 1)
        {
            dispatch_semaphore_wait(rw_mutex, DISPATCH_TIME_FOREVER); // Lock the resource if first reader
        }
        dispatch_semaphore_signal(mutex);

        // Reading section
        printf("%d          real                    reader              %d\n", info->id, BUFFER);
        sleep(1);

        // Release read access
        dispatch_semaphore_wait(mutex, DISPATCH_TIME_FOREVER);
        read_count--;
        if (read_count == 0)
        {
            dispatch_semaphore_signal(rw_mutex); // Release the resource if last reader
        }
        dispatch_semaphore_signal(mutex);

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
            printf("%d          %s                    writer              No permission\n", info->id, info->is_real ? "real" : "dummy");
            sleep(1);
            continue;
        }

        dispatch_semaphore_wait(rw_mutex, DISPATCH_TIME_FOREVER);

        // Writing section
        BUFFER = rand() % 10000;
        printf("%d          real                    writer              %d\n", info->id, BUFFER);
        sleep(1);

        dispatch_semaphore_signal(rw_mutex);

        sleep(1); // Simulating time between writes
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
