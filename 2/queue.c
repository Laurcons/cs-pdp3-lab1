#include "queue.h"

queue_t *queue_init(int size)
{
  queue_t *q = malloc(sizeof(queue_t));
  q->values = malloc(sizeof(double) * size);
  q->size = size;
  q->start = -1;
  q->end = -1;
}

void queue_destroy(queue_t *q)
{
  free(q->values);
  free(q);
}

int queue_push(queue_t *q, double value)
{
  if (q->start == -1)
  {
    q->start = 0;
    q->end = 0;
  }
  else
  {
    q->end++;
  }
  if (q->end >= q->size)
  {
    q->end = 0;
  }
  q->values[q->end] = value;
  return 1;
}

double queue_pull(queue_t *q)
{
  double v = q->values[q->start];
  if (q->start == q->end)
  {
    q->start = -1;
    q->end = -1;
  }
  else
  {
    q->start++;
    if (q->start >= q->size)
    {
      q->start = 0;
    }
  }
  return v;
}

int queue_count(queue_t *q)
{
  if (q->start == -1)
    return 0;
  if (q->start < q->end)
    return q->end - q->start + 1;
  return q->start - q->end + 1;
}

int queue_remaining(queue_t *q)
{
  return q->size - queue_count(q);
}