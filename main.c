#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

#define MAX_NUM (1000)

//run program print results in terminal
//gcc -o main main.c -lpthread && ./main

struct thread_arguments
{
    int input[MAX_NUM];
    int first_or_second;
    int sum;
};

//server arguments
struct server_arguments
{
    struct thread_arguments *args[2];
};

sem_t server_sem;

//server function
void *server_function(void *p_struct)
{
    struct server_arguments *arg = (struct server_arguments *)p_struct; //cast to server_arguments struct

    printf("Server sends a start signal to worker thread 1\n");
    printf("Server sends a start signal to worker thread 2\n");

    sem_wait(&server_sem);
    sem_wait(&server_sem);

    printf("Server receives a completion signal from Worker thread 1\n");
    printf("Server receives a completion signal from Worker thread 2\n");

    printf("Worker thread 1 result: %d\n", arg->args[0]->sum);
    printf("Worker thread 2 result: %d\n", arg->args[1]->sum);
    return NULL;
}

//worker function
void *worker_function(void *p_struct)
{
    struct thread_arguments *arg = (struct thread_arguments *)p_struct;

    printf("Worker thread %d receives a start signal from Server\n", arg->first_or_second);

    int local_sum = 0;
    if (arg->first_or_second == 1)
    {
        for (int i = 0; i < MAX_NUM / 2; i++)
        {
            local_sum += arg->input[i];
        }
    }
    else if (arg->first_or_second == 2)
    {
        for (int i = MAX_NUM / 2; i < MAX_NUM; i++)
        {
            local_sum += arg->input[i];
        }
    }

    arg->sum = local_sum;
    sem_post(&server_sem);

    printf("Worker thread %d sends a completion signal to Server\n", arg->first_or_second);
    return NULL;
}

//Where the magic happens
int main(void)
{
    struct thread_arguments arg1, arg2;
    struct server_arguments sarg;
    sarg.args[0] = &arg1;
    sarg.args[1] = &arg2;

    FILE *in = fopen("data1.dat", "r"); // open file for reading
    if (in == NULL)
    {
        fprintf(stderr, "Cannot open data1.dat\n"); // print error message
        return -1;
    }

    for (int i = 0; i < MAX_NUM; i++)
    {
        char line[256] = { 0 };
        if (fgets(line, 255, in) == NULL)
        {
            fprintf(stderr, "Cannot read number from data1.dat\n"); // print error message
            return -2;
        }

        char *temp;
        int input = strtol(line, &temp, 10);
        if (temp == line)
        {
            fprintf(stderr, "Cannot parse number from data1.dat\n"); // print error message
            return -3;
        }

        arg1.input[i] = arg2.input[i] = input;
    }

    arg1.sum = arg2.sum = 0; // initialize sum to 0
    arg1.first_or_second = 1; // first thread
    arg2.first_or_second = 2; // second thread

    sem_init(&server_sem, 0, 0); // initialize semaphore
    
    pthread_t server, worker1, worker2; // create threads

    if (pthread_create(&server, NULL, server_function, &sarg) == -1)
    {
        fprintf(stderr, "Cannot create server thread\n");
        return -4;
    }

    if (pthread_create(&worker1, NULL, worker_function, &arg1) == -1)
    {
        fprintf(stderr, "Cannot create worker thread 1\n"); // print error message
        return -4;
    }

    if (pthread_create(&worker2, NULL, worker_function, &arg2) == -1)
    {
        fprintf(stderr, "Cannot create worker thread 2\n"); // print error message
        return -4;
    }

    pthread_join(worker1, NULL); // wait for worker thread 1 to finish
    pthread_join(worker2, NULL); // wait for worker thread 2 to finish
    pthread_join(server, NULL); // wait for server thread to finish

    sem_destroy(&server_sem); // destroy semaphore
    fclose(in); // close file
    return 0;
}