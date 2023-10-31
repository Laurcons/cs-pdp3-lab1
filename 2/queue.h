#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct queue_t
{
  double *values;
  int size;
  int start;
  int end;
} queue_t;

queue_t* queue_init(int size);
void queue_destroy(queue_t* q);
int queue_push(queue_t* q, double value);
double queue_pull(queue_t* q);
int queue_count(queue_t* q);
int queue_remaining(queue_t* q);