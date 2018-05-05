#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#ifdef DEBUG
#define _DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define _DEBUG_PRINTF(...) /* do nothing */
#endif

static void syscall_handler (struct intr_frame *);

static int memread_user (void *src, void *des, size_t bytes);

void sys_halt (void);
void sys_exit (int);
pid_t sys_exec (const char *cmdline);
int sys_wait(pid_t pid);




//help function
int sys_badmemory_access(void);
void sys_write(int fd, const void *buffer, unsigned size);

//memory check function
bool check_addr (const uint8_t *uaddr);
bool check_buffer (void* buffer, unsigned size);
static int get_user (const uint8_t *uaddr);


void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// in case of invalid memory access, fail and exit.
static int fail_invalid_access(void) {
  sys_exit (-1);
  NOT_REACHED();
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
      shutdown_power_off();
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
      break;
    }

  case SYS_EXEC: // 2
    {
      void* cmdline = *(char **)(f->esp+4);

      //check_valid_string(cmdline);
      //TODO: add check

      int return_code = sys_exec((const char*) cmdline);
      f->eax = (uint32_t) return_code;
      break;
    }

  case SYS_WAIT: // 3
    {

      if(!check_buffer(f->esp+4, sizeof(pid_t))){
        sys_badmemory_access();
      }    

      pid_t pid = *(int *)(f->esp+4);

      int return_code = sys_wait(pid);

      f->eax = return_code;
      break;
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

      //TODO:
      //f->eax= ?

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



void sys_exit(int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);

  // The process exits.
  // wake up the parent process (if it was sleeping) using semaphore,
  // and pass the return code.
  struct process_control_block *pcb = thread_current()->pcb;
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
  _DEBUG_PRINTF ("[DEBUG] Exec : %s\n", cmdline);

  // cmdline is an address to the character buffer, on user memory
  // so a validation check is required
  if (get_user((const uint8_t*) cmdline) == -1) fail_invalid_access();

  tid_t child_tid = process_execute(cmdline);
  return child_tid;
}

int sys_wait(pid_t pid) {
  return process_wait(pid);
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

/****************** Helper Functions on Memory Access ********************/

static int32_t
get_user (const uint8_t *uaddr) {
  // check that a user pointer `uaddr` points below PHYS_BASE
  if (! ((void*)uaddr < PHYS_BASE)) {
    // TODO distinguish with result -1 (convert into another handler)
    return -1; // invalid memory access
  }

  // as suggested in the reference manual, see (3.1.5)
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
      : "=&a" (result) : "m" (*uaddr));
  return result;
}

/**
 * Reads a consecutive `bytes` bytes of user memory with the
 * starting address `src` (uaddr), and writes to dst.
 * Returns the number of bytes read, or -1 on page fault (invalid memory access)
 */
static int
memread_user (void *src, void *dst, size_t bytes)
{
  int32_t value;
  size_t i;
  for(i=0; i<bytes; i++) {
    value = get_user(src + i);
    if(value < 0) return -1; // invalid memory access.
    *(char*)(dst + i) = value & 0xff;
  }
  return (int)bytes;
}

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

