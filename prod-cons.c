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

#define QUEUESIZE 5;
#define LOOP 100000000 // change this
#define P 5
#define Q 3
#define NUM_TASKS 1
#define PERIOD 1000000 // 1 sec

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
  void *(*ErrorFcn)(void *); // function to be executed in case the queue is full
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
  int full, empty, size;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit(int size);
void queueDelete(queue *q);
void queueAdd(queue *q, Timer in);
void queueDel(queue *q, Timer *out);

typedef struct
{
  // Argument fields
  queue *fifo;
  Timer *T;
} Arguments;

void *errorFnc(void *q)
{
  queue *fifo = (queue *)q;
  while (fifo->full)
  {
    // printf ("waiting for queue to empty \n");
    pthread_cond_wait(fifo->notFull, fifo->mut);
    // printf ("signal acquired and fifo->full is: %d \n", fifo->full);
  }
  return (NULL);
}

int main(int argc, char *argv[])
{
  int p = P, q = Q, queuesize = QUEUESIZE;
  int num_tasks = NUM_TASKS - 1;
  float period[3];
  switch (argc)
  {
  case 7:
    num_tasks = 3;
    period[0] = atof(argv[4]);
    period[1] = atof(argv[5]);
    period[2] = atof(argv[6]);

    p = atoi(argv[1]);
    q = atoi(argv[2]);
    queuesize = atoi(argv[3]);
    break;
  case 6:
    q = atoi(argv[2]);
    period[num_tasks] = atof(argv[5]);
    num_tasks++;
  case 5:
    p = atoi(argv[1]);
    period[num_tasks] = atof(argv[4]);
    num_tasks++;
  case 4:
    if (fmod(atof(argv[3]), 1) > 0)
    {
      period[num_tasks] = atof(argv[3]);
      num_tasks++;
    }
    else
    {
      period[num_tasks] = PERIOD;
      queuesize = atoi(argv[3]);
    }
    if (argc == 6)
      break;
  case 3:
    if (fmod(atof(argv[2]), 1) > 0)
    {
      period[num_tasks] = atof(argv[1]);
      num_tasks++;
    }
    else
    {
      period[num_tasks] = PERIOD;
      q = atoi(argv[2]);
    }
    if (argc == 5)
      break;
  case 2:
    if (fmod(atof(argv[1]), 1) > 0)
    {
      period[num_tasks] = atof(argv[1]);
      num_tasks++;
    }
    else
    {
      period[num_tasks] = PERIOD;
      p = atoi(argv[1]);
    }
    break;
  default:
    printf("Wrong number of arguments\n");
  }

  pthread_t pro[p], con[q];

  queue *fifo;
  fifo = queueInit(queuesize);

  static int numbers[] = {17, 23, 34, 47, 53, 67, 79, 81, 97};

  static find_primes_args fpa;
  fpa.numbers = numbers;
  fpa.size = 9;

  Timer T;
  T.TimerFcn = find_primes;
  T.arg = &fpa;
  T.StartDelay = 2;
  T.Period = 1000000; // change this
  T.ErrorFcn = errorFnc;
  // signal(SIGUSR1, T.ErrorFcn);
  T.StartFcn = start;
  T.TasksToExecute = 11;
  T.id = 1;
  T.StopFcn = stop;

  T.StartFcn(&T);

  Arguments prod_args;
  prod_args.fifo = fifo;
  prod_args.T = &T;

  int thread_chunk = fmod(p, num_tasks) == 0 ? p / num_tasks : (p / num_tasks) + 1;

  if (fifo == NULL)
  {
    fprintf(stderr, "main: Queue Init failed.\n");
    exit(1);
  }
  for (int task = 0; task < num_tasks; task++)
  {
    int limit = (task + 1) * thread_chunk;
    if (task == (num_tasks - 1))
    {
      limit = p - (num_tasks - 1) * thread_chunk;
    }
    for (int i = task * thread_chunk; i <= limit; ++i)
    {
      if (pthread_create(&pro[i], NULL, producer, &prod_args) != 0)
      {
        fprintf(stderr, "Failed to create producer thread %d\n", i);
        return 1;
      }
    }
    // change T.Period
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
    pthread_mutex_lock(fifo->mut);
    if (fifo->full)
    {
      printf("producer: queue FULL. \n");
      T->ErrorFcn(fifo);
    }

    if (T->TasksToExecute <= 0)
    {
      printf("producer exits\n");
      pthread_mutex_unlock(fifo->mut);
      return (NULL);
    }

    T->TasksToExecute--; // important order: before usleep
    printf("TasksToExecute: %d\n", (T->TasksToExecute + 1));
    struct timeval previous = start;
    gettimeofday(&start, NULL);
    T->add_queue = &start;
    queueAdd(fifo, *T);
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_broadcast(fifo->notEmpty);
    size_t sleep = T->Period - 10000 * (start.tv_sec - previous.tv_sec) - (start.tv_usec - previous.tv_usec);
    usleep(sleep);
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
    pthread_mutex_lock(fifo->mut);
    while (fifo->empty)
    {
      if (flag)
      {
        printf("consumer exits\n");
        pthread_mutex_unlock(fifo->mut);
        return (NULL);
      }
      printf("consumer: queue EMPTY.\n");
      pthread_cond_wait(fifo->notEmpty, fifo->mut);
    }
    queueDel(fifo, &d);
    pthread_mutex_unlock(fifo->mut);
    // raise(SIGUSR1);
    pthread_cond_broadcast(fifo->notFull);
    // printf ("consumer: received %d.\n", i++);
    struct timeval end;
    gettimeofday(&end, NULL);
    time_t interval = 1000000 * (end.tv_sec - d.add_queue->tv_sec) + end.tv_usec - d.add_queue->tv_usec;
    printf("Time interval: %ld us\n", interval);
    fflush(stdout);
    d.TimerFcn(d.arg);
    if (d.TasksToExecute == 0)
    {
      d.StopFcn(d.id);
    }
  }
  return (NULL);
}

queue *queueInit(int size)
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
  q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(q->mut, NULL);
  q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notEmpty, NULL);

  return (q);
}

void queueDelete(queue *q)
{
  pthread_mutex_destroy(q->mut);
  free(q->mut);
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
  q->tail++;
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
