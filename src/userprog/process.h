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
};


#endif /* userprog/process.h */
