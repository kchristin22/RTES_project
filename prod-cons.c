/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#define QUEUESIZE 5;
#define LOOP 100000000 // change this
#define P 4
#define Q 4
#define NUM_TASKS 1
#define PERIOD 1 // 1 sec

void *producer(void *args);
void *consumer(void *args);

pthread_cond_t *end;
bool flag = false;

typedef struct
{
  void *(*work)(void *);
  void *arg;
  struct timeval *start;
  int id;
} workFunction;

typedef struct
{
  __uint32_t Period;               // period of task execution in usec
  sig_atomic_t TasksToExecute;     // number of tasks to be executed
  __uint8_t StartDelay;            // start executing tasks after a StartDelay delay
  void *(*StartFcn)(void *);       // initiate data to be used by TimerFcn
  void *(*StopFcn)(__uint32_t id); // function to be executed after the last call of the TimerFcn (TasksToExecute==0)
  void *(*TimerFcn)(void *);       // function to be executed at the start of each period
  void *arg;
  void *(*ErrorFcn)(void *, void *); // function to be executed in case the queue is full
  __uint32_t id;
  struct timeval *add_queue; // pointer to save the timestamp of adding the object to the queue
  struct timeval *del_queue; // pointer to save the timestamp of deleting the object from the queue
} Timer;

void *stop(__uint32_t id)
{
  printf("End of Timer with id: %d\n", id);
  return (NULL);
}

void *start(void *T)
{
  Timer *timer = (Timer *)T;
  usleep(timer->StartDelay);
  static struct timeval start;
  gettimeofday(&start, NULL);
  timer->TimerFcn(timer->arg);
  timer->TasksToExecute--;
  usleep(timer->Period);
  return (NULL);
}

void *startat(Timer T, __uint16_t y, __uint8_t m, __uint8_t d, __uint8_t h, __uint8_t min, __uint8_t sec)
{
  // Get the current system time
  time_t currentTime = time(NULL);

  // Convert the system time to a local time struct
  struct tm *localTime = localtime(&currentTime);

  // Extract the date components
  int year = localTime->tm_year + 1900; // Year since 1900
  int month = localTime->tm_mon + 1;    // Month
  int day = localTime->tm_mday;         // Day of the month

  // check if the timestamp has passed or is now
  if (year < y || month < m || day < d || localTime->tm_hour < h || localTime->tm_min < min || localTime->tm_sec < sec)
  { // add checks for overflows
    __uint32_t us_of_day = 24 * 3600 * 10 ^ 6;
    __uint32_t wait = (year - y) * 365 * us_of_day;
    wait += abs(month - m) * 30 * us_of_day;
    wait += abs(day - d) * us_of_day;
    usleep(wait);
  }
  for (__uint16_t iterations = 0; iterations < T.TasksToExecute; iterations++)
  {
    T.TimerFcn(T.arg);
    usleep(T.Period);
  }
  return (NULL);
}

typedef struct
{
  int *numbers;
  int size;
} find_primes_args;

bool is_prime(int n)
{
  if (n <= 1)
    return false;
  if (n <= 3)
    return true;
  if (n % 2 == 0 || n % 3 == 0)
    return false;
  for (int i = 5; i * i <= n; i += 6)
  {
    if (n % i == 0 || n % (i + 2) == 0)
      return false;
  }
  return true;
}

void *find_primes(void *args)
{
  find_primes_args fpa = *(find_primes_args *)args;

  int count = 0;
  int *prime_numbers = (int *)malloc(fpa.size * sizeof(int));

  for (int i = 0; i < fpa.size; i++)
  {
    if (is_prime(fpa.numbers[i]))
    {
      prime_numbers[count] = fpa.numbers[i];
      count++;
    }
  }
  // printf("Prime numbers: ");
  // for (int i = 0; i < count; i++) {
  //     printf("%d ", prime_numbers[i]);
  // }
  // printf("\n");
  printf("Function execution\n");
  free(prime_numbers);
  return (NULL);
}

typedef struct
{
  Timer *buf;
  long head, tail;
  int full, empty, size, num_tasks;
  pthread_mutex_t **prod_mut, *cons_mut, *queue_mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit(int size, __uint8_t num_tasks);
void queueDelete(queue *q);
void queueAdd(queue *q, Timer in);
void queueDel(queue *q, Timer *out);

typedef struct
{
  // Argument fields
  queue *fifo;
  Timer *T;
} Arguments;

void *errorFnc(void *q, void *id)
{
  queue *fifo = (queue *)q;
  __uint8_t *task = (__uint8_t *)id;
  while (fifo->full)
  {
    // printf ("waiting for queue to empty \n");
    pthread_cond_wait(fifo->notFull, fifo->prod_mut[*task - 1]);
    // printf ("signal acquired and fifo->full is: %d \n", fifo->full);
  }
  return (NULL);
}

int main(int argc, char *argv[])
{
  int p = P, q = Q, queuesize = QUEUESIZE;
  int num_tasks = NUM_TASKS;
  float period[3] = {PERIOD, PERIOD, PERIOD};

  if (argc > 1)
  {
    for (int i = 1; i < argc; i++)
    {
      printf("argv[%d]: %s\n", i, argv[i]);
      switch (argv[i][0])
      {
      case 'p':
        p = atoi(argv[i] + 2);
        printf("p: %d\n", p);
        break;
      case 'q':
        q = atoi(argv[i] + 2);
        printf("q: %d\n", q);
        break;
      case 's':
        queuesize = atoi(argv[i] + 2);
        break;
      case 't':
        period[num_tasks - 1] = atof(argv[i] + 2);
        num_tasks++;
        break;
      default:
        printf("Wrong argument, using default feature\n");
      }
    }
  }
  // num_tasks = 2;
  // period[0] = 3;
  // period[1] = 1;

  num_tasks--; // reverse the last increment of num_tasks
  pthread_t pro[p], con[q];
  queue *fifo;
  fifo = queueInit(queuesize, num_tasks);

  static int numbers[] = {17, 23, 34, 47, 53, 67, 79, 81, 97};

  static find_primes_args fpa;
  fpa.numbers = numbers;
  fpa.size = 9;

  Timer T[num_tasks];
  Arguments prod_args[num_tasks];
  for (int i = 0; i < num_tasks; i++)
  {
    T[i].TimerFcn = find_primes;
    T[i].arg = &fpa;
    T[i].Period = 1000000 * period[i];
    T[i].TasksToExecute = 11;
    T[i].id = i + 1;
    T[i].StartDelay = 1000000;
    T[i].ErrorFcn = errorFnc;
    T[i].StartFcn = start;
    T[i].StopFcn = stop;
    T[i].StartFcn(&T[i]);

    prod_args[i].fifo = fifo;
    prod_args[i].T = &T[i];
  }

  int thread_chunk = p / num_tasks;
  int limit = 0;

  if (fifo == NULL)
  {
    fprintf(stderr, "main: Queue Init failed.\n");
    exit(1);
  }
  for (int task = 0; task < num_tasks; task++)
  {

    if (task == (num_tasks - 1))
    {
      limit = limit + p - ((num_tasks - 1) * thread_chunk);
      printf("limit: %d\n", limit);
    }
    else
    {
      limit = (task + 1) * thread_chunk;
      printf("limit: %d\n", limit);
    }
    for (int i = task * thread_chunk; i < limit; ++i)
    {
      printf("Creating producer thread %d for task %d\n", i, task);
      if (pthread_create(&pro[i], NULL, producer, &prod_args[task]) != 0)
      {
        fprintf(stderr, "Failed to create producer thread %d\n", i);
        return 1;
      }
    }
  }
  for (int i = 0; i < q; ++i)
  {
    if (pthread_create(&con[i], NULL, consumer, fifo) != 0)
    {
      fprintf(stderr, "Failed to create consumer thread %d\n", i);
      return 1;
    }
  }
  for (int i = 0; i < p; ++i)
  {
    pthread_cond_broadcast(fifo->notFull);
    pthread_cond_broadcast(fifo->notEmpty);
    pthread_join(pro[i], NULL);
  }

  printf("All producers finished\n");

  flag = true;

  for (int i = 0; i < q; ++i)
  {
    pthread_cond_broadcast(fifo->notEmpty);
    pthread_join(con[i], NULL);
  }

  printf("deleting fifo\n");
  queueDelete(fifo);

  return 0;
}

void *producer(void *args)
{
  Arguments *prod_args = (Arguments *)args;
  queue *fifo = prod_args->fifo;
  Timer *T = prod_args->T;
  static struct timeval start;
  gettimeofday(&start, NULL);

  int i;
  for (i = 0; i < LOOP; i++)
  {
    pthread_mutex_lock(fifo->prod_mut[T->id - 1]);
    if (fifo->full)
    {
      printf("producer: queue FULL. \n");
      T->ErrorFcn(fifo, &T->id);
    }

    if (T->TasksToExecute <= 0)
    {
      printf("producer exits for id %d\n", T->id);
      pthread_mutex_unlock(fifo->prod_mut[T->id - 1]);
      return (NULL);
    }

    T->TasksToExecute--; // important order: before usleep
    printf("TasksToExecute of Timer with id %d: %d\n", T->id, (T->TasksToExecute + 1));
    struct timeval previous = start;
    gettimeofday(&start, NULL);
    pthread_mutex_lock(fifo->queue_mut);
    T->add_queue = &start;
    queueAdd(fifo, *T);
    pthread_mutex_unlock(fifo->queue_mut);
    size_t sleep = T->Period - 10000 * (start.tv_sec - previous.tv_sec) - (start.tv_usec - previous.tv_usec);
    // printf("sleep for %ld us\n", sleep);
    usleep(sleep); // move this into the mutex for a real timer
    pthread_mutex_unlock(fifo->prod_mut[T->id - 1]);
    pthread_cond_broadcast(fifo->notEmpty);
  }
  return (NULL);
}

void *consumer(void *q)
{
  queue *fifo;
  Timer d;
  int i;

  fifo = (queue *)q;

  for (i = 0;; i++)
  {
    pthread_mutex_lock(fifo->cons_mut);
    while (fifo->empty)
    {
      if (flag)
      {
        printf("consumer exits\n");
        pthread_mutex_unlock(fifo->cons_mut);
        return (NULL);
      }
      // printf("consumer: queue EMPTY.\n");
      pthread_cond_wait(fifo->notEmpty, fifo->cons_mut);
    }
    pthread_mutex_lock(fifo->queue_mut);
    queueDel(fifo, &d);
    pthread_mutex_unlock(fifo->queue_mut);
    pthread_mutex_unlock(fifo->cons_mut);
    // raise(SIGUSR1);
    pthread_cond_broadcast(fifo->notFull);
    // printf ("consumer: received %d.\n", i++);
    struct timeval end;
    gettimeofday(&end, NULL);
    time_t interval = 1000000 * (end.tv_sec - d.add_queue->tv_sec) + end.tv_usec - d.add_queue->tv_usec;
    // printf("Time interval: %ld us\n", interval);
    fflush(stdout);
    d.TimerFcn(d.arg);
    if (d.TasksToExecute == 0)
    {
      d.StopFcn(d.id);
    }
  }
  return (NULL);
}

queue *queueInit(int size, __uint8_t num_tasks)
{
  queue *q;

  q = (queue *)malloc(sizeof(queue));
  if (q == NULL)
    return (NULL);

  q->buf = (Timer *)malloc(size * sizeof(Timer));
  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->size = size;
  q->prod_mut = (pthread_mutex_t **)malloc(num_tasks * sizeof(pthread_mutex_t *));
  for (__uint8_t task = 0; task < num_tasks; task++)
  {
    q->prod_mut[task] = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->prod_mut[task], NULL);
  }
  q->cons_mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(q->cons_mut, NULL);
  q->queue_mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(q->queue_mut, NULL);
  q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notEmpty, NULL);

  return (q);
}

void queueDelete(queue *q)
{
  for (__uint8_t task = 0; task < q->num_tasks; task++)
  {
    pthread_mutex_destroy(q->prod_mut[task]);
    free(q->prod_mut[task]);
  }
  free(q->prod_mut);
  pthread_mutex_destroy(q->cons_mut);
  free(q->cons_mut);
  pthread_mutex_destroy(q->queue_mut);
  free(q->queue_mut);
  pthread_cond_destroy(q->notFull);
  free(q->notFull);
  pthread_cond_destroy(q->notEmpty);
  free(q->notEmpty);
  free(q->buf);
  free(q);
}

void queueAdd(queue *q, Timer in)
{
  q->buf[q->tail] = in;
  q->tail++; // atomic operation
  if (q->tail == q->size)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel(queue *q, Timer *out)
{
  *out = q->buf[q->head];

  q->head++;
  if (q->head == q->size)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}
