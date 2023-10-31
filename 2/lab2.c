#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "queue.h"

typedef struct vector_t
{
  double *values;
  int size;
} vector_t;

typedef struct thread_payload_t
{
  vector_t vectOne;
  vector_t vectTwo;
  queue_t *queue;
  pthread_mutex_t *mutex;
  pthread_cond_t *condPushed;
  pthread_cond_t *condPulled;
} thread_payload_t;

vector_t randomVector(int size)
{
  vector_t v;
  v.size = size;
  v.values = malloc(sizeof(double) * size);
  for (int i = 0; i < size; i++)
  {
    v.values[i] = rand() % 5;
  }
  return v;
}

void *thread_producer(void *arg)
{
  thread_payload_t *context = (thread_payload_t *)arg;
  for (int i = 0; i <= context->vectOne.size; i++)
  {
    double prod;
    if (i < context->vectOne.size)
    {
      prod = context->vectOne.values[i] * context->vectTwo.values[i];
      printf("Product #%d is %.2f * %.2f = %.2f\n", i, context->vectOne.values[i], context->vectTwo.values[i], prod);
    }
    else
    {
      printf("Pushing -1\n");
      prod = -1;
    }
    pthread_mutex_lock(context->mutex);
    while (queue_remaining(context->queue) <= 0)
    {
      pthread_cond_wait(context->condPulled, context->mutex);
    }
    queue_push(context->queue, prod);
    pthread_cond_signal(context->condPushed);
    pthread_mutex_unlock(context->mutex);
  }
  printf("Producer done\n");
}

void *thread_consumer(void *arg)
{
  thread_payload_t *context = (thread_payload_t *)arg;
  double sum = 0;
  int i = 0;
  while (1)
  {
    pthread_mutex_lock(context->mutex);
    while (queue_count(context->queue) <= 0)
    {
      pthread_cond_wait(context->condPushed, context->mutex);
    }
    double prod = queue_pull(context->queue);
    pthread_cond_signal(context->condPulled);
    pthread_mutex_unlock(context->mutex);
    if (prod == -1)
    {
      printf("Consumer done, dot product is %.2f\n", sum);
      return NULL;
    }
    printf("Adding to sum: %.2f + %.2f = %.2f\n", sum, prod, sum + prod);
    sum += prod;
  }
}

int main()
{
  srand(time(NULL));
  thread_payload_t *payload = malloc(sizeof(thread_payload_t));
  payload->queue = queue_init(1);
  payload->vectOne = randomVector(20);
  payload->vectTwo = randomVector(20);
  payload->mutex = malloc(sizeof(pthread_mutex_t));
  payload->condPulled = malloc(sizeof(pthread_cond_t));
  payload->condPushed = malloc(sizeof(pthread_cond_t));
  pthread_mutex_init(payload->mutex, NULL);
  pthread_cond_init(payload->condPulled, NULL);
  pthread_cond_init(payload->condPushed, NULL);

  pthread_t tP;
  pthread_t tC;
  pthread_create(&tP, NULL, thread_producer, payload);
  pthread_create(&tC, NULL, thread_consumer, payload);

  pthread_join(tP, NULL);
  pthread_join(tC, NULL);

  pthread_mutex_destroy(payload->mutex);
  pthread_cond_destroy(payload->condPulled);
  pthread_cond_destroy(payload->condPushed);
  free(payload->mutex);
  free(payload->condPulled);
  free(payload->condPushed);
  free(payload->vectOne.values);
  free(payload->vectTwo.values);
  queue_destroy(payload->queue);
  free(payload);
}