#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

typedef int pid_t;

#define PID_ERROR         ((pid_t) -1)
#define PID_INITIALIZING  ((pid_t) -2)


tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* Help function for process_wait*/
void getchild(struct list *child_list, struct process_control_block **child_toexit, struct list_elem **list_elem_toremove,tid_t child_tid);

/* PCB : see initialization at process_execute(). */
struct process_control_block {

  pid_t pid;                /* process pid */

  const char* cmdline;      /* command line that current process will execute */

  struct list_elem elem;    /* element for thread.child_list */

  bool waiting;             /* if parent process is waiting on current thread. */
  bool exited;              /* if current is exited */
  int32_t exitcode;         /* the exit code */

  /* Synchronization */
  struct semaphore sema_initialization;   /* the semaphore used between start_process() and process_execute() */
  struct semaphore sema_wait;             /* the semaphore used for wait() : parent blocks until child exits */

};

#endif /* userprog/process.h */
