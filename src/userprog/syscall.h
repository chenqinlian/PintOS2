#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define FD_BASE 888

void syscall_init (void);

struct lock filelock;
#endif /* userprog/syscall.h */
