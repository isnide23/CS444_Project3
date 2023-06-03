#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "eventbuf/eventbuf.h"

struct eventbuf *eb;
int event_count;

// initialized in main from sem_open_temp
sem_t *mutex;
sem_t *items;
sem_t *spaces;

sem_t *sem_open_temp(const char *name, int value)
{
    sem_t *sem;

    // Create the semaphore
    if ((sem = sem_open(name, O_CREAT, 0600, value)) == SEM_FAILED)
        return SEM_FAILED;

    // Unlink it so it will go away after this process exits
    if (sem_unlink(name) == -1) {
        sem_close(sem);
        return SEM_FAILED;
    }

    return sem;
}

int calc_event_number(int thread_id, int it_num) {
   return thread_id * 100 + it_num;
}

void *producer_run(void * thread_id) {
    int *p = thread_id;
    for(int i = 0; i < event_count; i++) {
        sem_wait(spaces);
        sem_wait(mutex);
        eventbuf_add(eb, calc_event_number(*p, i));
        printf("P%d: adding event %d", *p, calc_event_number(*p, i));
        sem_post(mutex);
        sem_post(items); 
    }
    printf("P%d: exiting\n", *p);
    return NULL;
}

void *consumer_run(void * thread_id) {
    int *p = thread_id;
    for(;;) {
        sem_wait(items);
        sem_wait(mutex);
        if(eventbuf_empty(eb)){
            sem_post(mutex);
            printf("C%d: exiting\n", *p);
            return NULL;
        }
        int event_num = eventbuf_get(eb);
        printf("C%d: got event: %d\n", *p, event_num);
        sem_post(mutex);
        sem_post(spaces);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // Parse command line
    if (argc != 5) {
        fprintf(stderr, "usage: pcseml producer_count consumer_count event_count queue_size\n");
        exit(1);
    }

    int producer_count = atoi(argv[1]);
    int consumer_count = atoi(argv[2]);
    event_count = atoi(argv[3]);
    int queue_size = atoi(argv[4]);

    // Create an Event Buffer
    eb = eventbuf_create();

    // Allocate thread for producers
    pthread_t *producer_thread = calloc(producer_count, sizeof *producer_thread);

    // Allocate thread ID array for all producers
    int *producer_thread_id = calloc(producer_count, sizeof *producer_thread_id);

    // Allocate thread for consumers
    pthread_t *consumer_thread = calloc(consumer_count, sizeof *consumer_thread);

    // Allocate thread ID array for all consumers
    int *consumer_thread_id = calloc(consumer_count, sizeof *consumer_thread_id);
    
    // Launch all producers
    for (int i = 0; i < producer_count; i++) {
        producer_thread_id[i] = i;
        pthread_create(producer_thread + i, NULL, producer_run, producer_thread_id + i);
    }

    //  Launch all consumers
    for (int i = 0; i < consumer_count; i++) {
        consumer_thread_id[i] = i;
        pthread_create(consumer_thread + i, NULL, consumer_run, consumer_thread_id + i);
    }

    // Wait for all producers to complete
    for (int i = 0; i < producer_count; i++)
        pthread_join(producer_thread[i], NULL);
}