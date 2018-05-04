#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"


typedef uint32_t pid_t;

static void syscall_handler (struct intr_frame *);

//help function
void sys_write(int fd, const void *buffer, unsigned size);
void sys_halt (void);
void sys_exit (int);
int sys_badmemory_access(void);
pid_t sys_exec (const char *cmdline);
int sys_wait(pid_t pid);


//memory check function
bool check_addr (const uint8_t *uaddr);
bool check_buffer (void* buffer, unsigned size);
static int get_user (const uint8_t *uaddr);



void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
  if(!check_addr(f->esp)){
    thread_exit();
  }
  
  
  if(!check_buffer(f->esp,4)){
    sys_badmemory_access();
  }
  

  int syscall_number = *(int *)(f->esp);

  switch (syscall_number) {
  case SYS_HALT:
    {
      sys_halt();
      NOT_REACHED();
      break;
    }

  case SYS_EXIT:
    {
      int exitcode = *(int *)(f->esp + 4);

      //TODO: need fix
      if(exitcode<-1000){
        sys_badmemory_access();
      }
     

      sys_exit(exitcode);
      NOT_REACHED();
      break;
    }

  case SYS_EXEC:
    {
      void* cmdline = *(char **)(f->esp+4);

      //check_valid_string(cmdline);
      //TODO: add check

      int return_code = sys_exec((const char*) cmdline);
      f->eax = (uint32_t) return_code;
      break;
    }

  case SYS_WAIT:{
    if(!check_buffer(f->esp+4, sizeof(pid_t))){
      sys_badmemory_access();
    }    

    int pid = *(int *)(f->esp+4);

    int return_code = sys_wait(pid);

    f->eax = return_code;

  }

  case SYS_CREATE:
  case SYS_REMOVE:
  case SYS_OPEN:
  case SYS_FILESIZE:
  case SYS_READ:
    goto unhandled;

  case SYS_WRITE:
    {

      
      if (!check_addr(f->esp+4)){
        thread_exit();
      }
      if (!check_addr(f->esp+8)){
        thread_exit();
      }
      if (!check_addr(f->esp+12)){
        thread_exit();
      }

      int fd = *(int *)(f->esp+4);
      void *buffer = (void *)(f->esp+8);
      size_t size = *(size_t *)(f->esp+12);


      if(!check_buffer (buffer, size)){
        thread_exit();
      }

      sys_write(fd, buffer, size);



      break;
    }

  case SYS_SEEK:
  case SYS_TELL:
  case SYS_CLOSE:

  /* unhandled case */
  default:
    unhandled:
    printf("[ERROR] system call %d is unimplemented!\n", syscall_number);
    thread_exit();
    break;
  }
}

void sys_halt(void) {
  shutdown_power_off();
}

void sys_exit(int status UNUSED) {
  printf("%s: exit(%d)\n", thread_current()->name, status);

  
  struct pcbtype *pcb = thread_current()->pcb;
  ASSERT (pcb != NULL);

  pcb->exited = true;
  pcb->exitcode = status;
  sema_up (&pcb->sema_wait);
  

  thread_exit();
}

int sys_badmemory_access(void) {
  sys_exit (-1);
  NOT_REACHED();
}

pid_t sys_exec(const char *cmdline) {

  tid_t child_tid = process_execute(cmdline);
  return child_tid;
}


void sys_write(int fd, const void *buffer, unsigned size){
    
      if(!check_buffer ((void *)buffer, size)){
        thread_exit();
      }
      

      //Case1: print to screem
      if(fd == 1)
      {
        putbuf (*(char **)buffer, size);
        return;
      }
    
      //Case2: print to file 
      else{ 
        //TODO



      }

}

int sys_wait(pid_t pid){
  
  //printf("sys_wait\n");
  return process_wait(pid);
  
}


//memory check function


bool
check_addr(const uint8_t *uaddr){
  if ((void*)uaddr > PHYS_BASE){
    //thread_exit();
    return false;
  }

  return true;
}

bool
check_buffer (void* buffer, unsigned size){

  unsigned i;
  char* local_buffer = (char *) buffer;
  for (i = 0; i < size; i++)
    {
      if(!check_addr((const void*) local_buffer) || get_user((const uint8_t *)local_buffer)<0){
        return false;
      }
      local_buffer++;
    }

  return true;
}



static int
get_user (const uint8_t *uaddr) {
   int result;
   asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
   return result;
}



