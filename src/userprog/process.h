#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

typedef uint32_t pid_t;
#define PID_ERROR         ((pid_t) -1)
#define PID_INITIALIZING  ((pid_t) -2)


tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct pcbtype {
  pid_t pid;			/* pid of current process*/
  const char* cmdline;		/* the comandline to be executed by current process*/
  struct list_elem elem;	/* element for thread->child_list */
  bool waiting;             /* indicates whether parent process is waiting on this. */
  bool exited;              /* indicates whether the process is done (exited). */
                            // TODO: use state enums (STOPPED, RUNNING, READY, ZOMBIE, ...)
  int32_t exitcode;         /* the exit code passed from exit(), when exited = true */

  /* Synchronization */
  struct semaphore sema_initialization;   /* the semaphore used between start_process() and process_execute() */
  struct semaphore sema_wait;             /* the semaphore used for wait() : parent blocks until child exits */

};


#endif /* userprog/process.h */
