#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

static void 
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  pthread_mutex_lock(&bstate.barrier_mutex);
  bstate.nthread += 1;
  
  if(bstate.nthread % nthread != 0) {
    while (bstate.nthread % nthread)
      pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    round += 1;
    //if(round % nthread == 0) bstate.round += 1;
    bstate.round += 1;
    printf("thread %lu, round %d, bstate.nthread %d\n", (unsigned long)pthread_self(), bstate.round, bstate.nthread);
    pthread_mutex_unlock(&bstate.barrier_mutex);
  } else {
    round += 1;
    //if(round % nthread == 0) bstate.round += 1;
    //pthread_mutex_unlock(&bstate.barrier_mutex);
    
    pthread_cond_broadcast(&bstate.barrier_cond);
    printf("thread %lu, round %d, bstate.nthread %d\n", (unsigned long)pthread_self(), bstate.round, bstate.nthread);

    //while (round != bstate.nthread);
    //pthread_mutex_lock(&bstate.barrier_mutex);
    //bstate.round += 1;
    pthread_mutex_unlock(&bstate.barrier_mutex);
    for(;;){
      pthread_mutex_lock(&bstate.barrier_mutex);
      if (round % nthread == 0) {
        //bstate.round += 1;
        pthread_mutex_unlock(&bstate.barrier_mutex);
        break;
      }
      pthread_mutex_unlock(&bstate.barrier_mutex);
    }
  }
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  //long delay;
  int i;
  printf("thread %ld: tid: %lu\n", n, (unsigned long)pthread_self());
  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    printf("thread %ld, i %d, t %d\n", n, i, t);
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  //double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
