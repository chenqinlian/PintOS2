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

typedef uint32_t pid_t;

void sys_halt (void);
void sys_exit (int);
pid_t sys_exec (const char *cmdline);


//help function
void sys_write(int fd, const void *buffer, unsigned size);
int sys_badmemory_access(void);

//memory check function
bool check_addr (const uint8_t *uaddr);
bool check_buffer (void* buffer, unsigned size);
static int get_user (const uint8_t *uaddr);
//bool is_valid_string (const char *uaddr);
//void check_valid_string(const char *uaddr);


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
  
  /*
  int syscall_number;

  if (memread_user(f->esp, &syscall_number, sizeof(syscall_number)) == -1)
    sys_badmemory_access();
  */

  
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

      int return_code = sys_exec((const char*) cmdline);
      f->eax = (uint32_t) return_code;
      break;
    }

  case SYS_WAIT:
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
      if(!check_addr((const void*) local_buffer) || get_user(local_buffer)<0){
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


/*
bool is_valid_string (const char *uaddr)
{
  char ch;
  int i;
  if (!is_user_vaddr(uaddr))
    return false;
  for(i = 0; (ch = get_user((char *)(uaddr + i))) != -1 && ch != 0; i++)
  {
    if(!is_user_vaddr(uaddr + i + 1))
      return false;
  }
  return ch == 0;
}

void check_valid_string(const char *uaddr){
  
  if(!is_valid_string(uaddr)){
     sys_badmemory_access();
  }

}
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


