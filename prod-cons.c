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
 *	Revised	and tailroed by: Christina Koutsou
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

#define QUEUESIZE 1;
#define LOOP 100000000
#define P 1
#define Q 30
#define NUM_TASKS 1            // number of timers
#define PERIOD 1               // 1 sec
#define MAX_YEARS_DELAY 584941 // Max value of years in the future the timer can be set to due to overflow

void *producer(void *args);
void *consumer(void *args);

pthread_cond_t *end;
bool exit_flag = false;

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
  __uint32_t TasksToExecute;       // number of tasks to be executed
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
  // printf("End of Timer with id: %d\n", id);
  return (NULL);
}

void *start(void *T)
{
  Timer *timer = (Timer *)T;
  usleep(1000000 * timer->StartDelay);
  timer->TimerFcn(timer->arg);
  timer->TasksToExecute--;
  return (NULL);
}

void *startat(Timer *T, __uint16_t y, __uint8_t m, __uint8_t d, __uint8_t h, __uint8_t min, __uint8_t sec)
{

  if (m > 12 || d > 31 || h > 24 || min > 60 || sec > 60)
  {
    printf("Not a valid timestamp\n");
    return (NULL);
  }

  time_t currentTime = time(NULL);

  // Convert the system time to a local time struct
  struct tm *localTime = localtime(&currentTime);

  // Extract the date components
  int year = localTime->tm_year + 1900; // Year since 1900
  int month = localTime->tm_mon + 1;    // Month
  int day = localTime->tm_mday;         // Day of the month

  // check if the timestamp has passed or is now
  if (year < y || (year == y &&
                   (month < m || (month == m &&
                                  (day < d || (day == d &&
                                               (localTime->tm_hour < h || (localTime->tm_hour == h &&
                                                                           (localTime->tm_min < min || (localTime->tm_min == min &&
                                                                                                        localTime->tm_sec < sec))))))))))
  {
    if ((year - y) > MAX_YEARS_DELAY) // avoid overflow
    {
      printf("The timestamp is too far in the future\n");
      return (NULL);
    }
    else
    {
      __uint64_t us_of_day = 24 * 3600 * (1e6);
      __uint64_t wait = (year - y) * 365 * us_of_day;
      wait += abs(month - m) * 30 * us_of_day;
      wait += abs(day - d) * us_of_day;
      wait += abs(localTime->tm_hour - h) * 3600 * (int)(1e6);
      wait += abs(localTime->tm_min - min) * 60 * (int)(1e6);
      wait += abs(localTime->tm_sec - sec) * (int)(1e6);
      printf("wait for %ld us\n", wait);
      usleep(wait);
      T->TimerFcn(T->arg); // first call of the TimerFcn
      T->TasksToExecute--;
    }
  }
  else
  {
    // printf("The timestamp has passed\n");
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
  // printf("Function execution\n");
  free(prime_numbers);
  return (NULL);
}

typedef struct
{
  Timer *buf;
  long head, tail;
  int full, empty, size, num_tasks;
  pthread_mutex_t **prod_mut, *queue_mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit(int size, __uint8_t num_tasks);
void queueDelete(queue *q);
void queueAdd(queue *q, Timer in);
void queueDel(queue *q, Timer *out);

// Arguments for the producer thread
typedef struct
{
  queue *fifo;
  Timer *T;
  bool start_at;
  __uint16_t y;
  __uint8_t m, d, h, min, sec;
} Arguments;

void *errorFnc(void *q, void *id)
{
  queue *fifo = (queue *)q;
  __uint8_t *task = (__uint8_t *)id;
  while (fifo->full) // `while` to ensure that a check after the signal happens as well
  {
    pthread_cond_wait(fifo->notFull, fifo->prod_mut[*task - 1]);
  }
  return (NULL);
}

int main(int argc, char *argv[])
{
  // Initialize the arguments
  int p = P, q = Q, queuesize = QUEUESIZE;
  __uint8_t num_tasks = NUM_TASKS;
  time_t currentTime = time(NULL);
  struct tm *localTime = localtime(&currentTime);
  __uint16_t year = localTime->tm_year + 1900;
  __uint8_t month = localTime->tm_mon + 1, day = localTime->tm_mday,
            hour = localTime->tm_hour, min = localTime->tm_min, sec = (localTime->tm_sec + 5) > 60 ? 2 : localTime->tm_sec + 5;
  float period[3] = {PERIOD, PERIOD, PERIOD};
  bool start_at = false; // used to choose a start function for the timer

  if (argc > 1)
  {
    for (__uint8_t i = 1; i < argc; i++)
    {
      switch (argv[i][0])
      {
      case 'p':
        p = atoi(argv[i] + 2); // the arguments is in the form of p=$number$
        break;
      case 'q':
        q = atoi(argv[i] + 2);
        break;
      case 'n':
        queuesize = atoi(argv[i] + 2);
        break;
      case 't':
        period[num_tasks - 1] = atof(argv[i] + 2);
        num_tasks++;
        break;
      case 'y':
        year = atoi(argv[i] + 2);
        start_at = true;
        break;
      case 'm':
        month = atoi(argv[i] + 2);
        start_at = true;
        break;
      case 'd':
        day = atoi(argv[i] + 2);
        start_at = true;
        break;
      case 'h':
        hour = atoi(argv[i] + 2);
        start_at = true;
        break;
      case 'i':
        min = atoi(argv[i] + 2);
        start_at = true;
        break;
      case 's':
        sec = atoi(argv[i] + 2);
        start_at = true;
        break;
      default:
        printf("Wrong argument, using default feature\n");
      }
    }
  }

  // period[0] = 1;
  // period[1] = 2;
  // num_tasks = 3;
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

  // Initiate Timer characteristics
  for (__uint8_t i = 0; i < num_tasks; i++)
  {
    T[i].TimerFcn = find_primes;
    T[i].arg = &fpa;
    T[i].Period = 1000000 * period[i];
    T[i].TasksToExecute = 11;
    T[i].id = i + 1;
    T[i].ErrorFcn = errorFnc;
    if (start_at)
    {
      prod_args[i].start_at = true;
      prod_args[i].y = year;
      prod_args[i].m = month;
      prod_args[i].d = day;
      prod_args[i].h = hour;
      prod_args[i].min = min;
      prod_args[i].sec = sec;
    }
    else
    {
      prod_args[i].start_at = false;
      T[i].StartFcn = start;
      T[i].StartDelay = 1; // use delay only when time to start the timer is not specified
    }
    T[i].StopFcn = stop;

    prod_args[i].fifo = fifo;
    prod_args[i].T = &T[i];
  }

  int thread_chunk = p / num_tasks; // equal number of threads for each task
  int limit = 0;                    // used to set the limit of threads for each task

  if (fifo == NULL)
  {
    fprintf(stderr, "main: Queue Init failed.\n");
    exit(1);
  }
  for (__uint8_t task = 0; task < num_tasks; task++)
  {

    if (task == (num_tasks - 1))
    {
      limit = limit + p - ((num_tasks - 1) * thread_chunk); // the last task gets the remaining threads in case the division is not exact
    }
    else
    {
      limit = (task + 1) * thread_chunk;
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
    pthread_cond_broadcast(fifo->notFull); // broadcasting signals is used to wake any producer or consumer threads that are waiting for a condition that
                                           // has been met due to a producer exiting
    pthread_cond_broadcast(fifo->notEmpty);
    pthread_join(pro[i], NULL);
  }

  // printf("All producers finished\n");

  exit_flag = true; // set the exit flag to true to signal the consumer threads that from now on no item will be added to the queue

  for (int i = 0; i < q; ++i)
  {
    pthread_cond_broadcast(fifo->notEmpty); // broadcasting signals is used to wake any consumer threads that are waiting for a condition that
                                            // has been met due to the producers or a consumer exiting
    pthread_join(con[i], NULL);
  }

  // printf("deleting fifo\n");
  queueDelete(fifo);

  return 0;
}

void *producer(void *args)
{
  Arguments *prod_args = (Arguments *)args;
  queue *fifo = prod_args->fifo;
  Timer *T = prod_args->T;
  __uint8_t total_tasks = T->TasksToExecute; // initialize timer executions/iterations
  static struct timeval start;               // initialize start time
  gettimeofday(&start, NULL);

  int i;
  for (i = 0; i < LOOP; i++)
  {
    pthread_mutex_lock(fifo->prod_mut[T->id - 1]); // lock the mutex that corresponds to this producer thread,
                                                   // so not any other producer thread of this timer can access the queue
    if (T->TasksToExecute == total_tasks)          // checks if the timer has not yet started
    {
      // printf("Start function of timer %d\n", T->id);
      if (prod_args->start_at)
      {
        startat(T, prod_args->y, prod_args->m, prod_args->d, prod_args->h, prod_args->min, prod_args->sec);
      }
      else
      {
        T->StartFcn(T);
      }
      gettimeofday(&start, NULL); // re-calculate the current time since the start functions induce a delay
      T->TasksToExecute--;        // update the number of tasks to be executed
    }
    if (fifo->full)
    {
      // printf("producer: queue FULL. \n");
      T->ErrorFcn(fifo, &T->id); // wait for signal from consumer
    }

    // if (T->TasksToExecute <= 0) // this timer is finished so this thread serves no purpose anymore
    // {
    //   // printf("producer exits for id %d\n", T->id);
    //   pthread_mutex_unlock(fifo->prod_mut[T->id - 1]);
    //   return (NULL);
    // }

    // printf("TasksToExecute of Timer with id %d: %d\n", T->id, (T->TasksToExecute + 1));
    struct timeval previous = start; // save the previous time this producer thread was executed
    gettimeofday(&start, NULL);
    if (T->TasksToExecute != (total_tasks - 1)) // avoid adding the first item to the queue since it is already executed by the start function
    {
      pthread_mutex_lock(fifo->queue_mut); // this mutex is used among all producers and consumers to ensure that each operation on the queue is atomic
      T->add_queue = &start;               // note the time the item is passed on the queue to calculate afterwards the time it spents in the queue
      queueAdd(fifo, *T);
      pthread_mutex_unlock(fifo->queue_mut);
      pthread_cond_broadcast(fifo->notEmpty);
    }
    long int drift = (1000000 * (start.tv_sec - previous.tv_sec) + (start.tv_usec - previous.tv_usec)) - T->Period; // calculate the drift of the timer due to the mutex locks
    if (drift < 0)
    {
      drift = 0; // the drift was fixed previously and the producer was called earlier
    }
    printf("Drift for Timer with period %d us: %ld us\n", T->Period, drift);
    // printf("sleep for %ld us\n", T->Period - drift);
    long int sleep = T->Period - drift; // fix the drift
    if (sleep > 0)
      usleep(sleep); // add the delay before the next execution of the timer
    else
      usleep(0);

    pthread_mutex_unlock(fifo->prod_mut[T->id - 1]); // let another thread of this timer access the queue after a period passes
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
    pthread_mutex_lock(fifo->queue_mut);
    while (fifo->empty)
    {
      if (exit_flag)
      {
        // printf("consumer exits\n"); // the queue is empty and no more items will be added to it from now on
        pthread_mutex_unlock(fifo->queue_mut);
        return (NULL);
      }
      // printf("consumer: queue EMPTY.\n");
      pthread_cond_wait(fifo->notEmpty, fifo->queue_mut);
    }
    queueDel(fifo, &d);
    pthread_mutex_unlock(fifo->queue_mut);
    pthread_cond_broadcast(fifo->notFull);
    // printf ("consumer: received %d.\n", i++);
    struct timeval end;
    gettimeofday(&end, NULL);
    time_t interval = 1000000 * (end.tv_sec - d.add_queue->tv_sec) + end.tv_usec - d.add_queue->tv_usec; // calculate the time the item spents in the queue
    printf("Time spent in the queue of Timer with period %d us: %ld us\n", d.Period, interval);
    fflush(stdout);
    d.TimerFcn(d.arg);
    if (d.TasksToExecute == 0) // the queue is a FIFO queue so the last item to be added signifies that no tasks are left
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
  // Allocate and init each mutex for each timer
  for (__uint8_t task = 0; task < num_tasks; task++)
  {
    q->prod_mut[task] = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->prod_mut[task], NULL);
  }
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
  // Destroy and free the space of each mutex for each timer
  for (__uint8_t task = 0; task < q->num_tasks; task++)
  {
    pthread_mutex_destroy(q->prod_mut[task]);
    free(q->prod_mut[task]);
  }
  free(q->prod_mut);
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
