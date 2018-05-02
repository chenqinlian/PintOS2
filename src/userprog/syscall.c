#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"


static void syscall_handler (struct intr_frame *);

//define const
typedef uint32_t pid_t;

//help function
bool check_user(const uint8_t *uaddr);
bool check_Bytes(void *pointer, size_t bytes);
static int32_t get_user (const uint8_t *uaddr);
void get_bytes (void *src, void *dst, size_t bytes);

//minor functions in system call

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{

  printf ("system call!\n");

  // Step1: check f->esp validation
  if(!check_user(f->esp)){
    thread_exit ();//to modify, add sys_halt
    return;
  }

  // Step2: get f->esp value
  int syscall_number;
  syscall_number = get_user(f->esp);
  printf ("system call! syscall number is : %d, ", syscall_number);


  switch(syscall_number)
  {
    case SYS_HALT:
      printf("SYS_HALT\n");
      sys_halt();
      break;

    case SYS_EXIT:
      printf("SYS_EXIT\n");
      sys_exit();
      break;

    case SYS_EXEC:
      printf("SYS_EXEC\n");
      
      void *cmdline= (void *)f->esp+4;
      
      //TODO:check cmdline validation
      pid_t sys_exec(const char *cmdline);

      break;

    case SYS_WAIT:
      printf("SYS_WAIT\n");
      break;
    case SYS_CREATE:
      printf("SYS_CREATE\n");
      break;
    case SYS_REMOVE:
      printf("SYS_REMOVE\n");
      break;
    case SYS_OPEN:
      printf("SYS_OPEN\n");
      break;
    case SYS_FILESIZE:
      printf("SYS_FILESIZE\n");
      break;
    case SYS_READ:
      printf("SYS_READ\n");
      break;

    case SYS_WRITE:
      printf("SYS_WRITE\n");
      
      bool judge = !(check_Bytes(f->esp+4,4) && check_Bytes(f->esp+8,4) && check_Bytes(f->esp+12,4) );
      printf("judge?%d\n", judge);

      if(!(check_Bytes(f->esp+4,4) && check_Bytes(f->esp+8,4) && check_Bytes(f->esp+12,4) )){
        thread_exit();
      }
      
  
      int fd = (int)get_user(f->esp+4);
      const void *buffer = (const void *)get_user(f->esp+8);
      int size   = (int)get_user(f->esp+12);

      printf("fd=%d\n", fd);
      printf("size=%d\n", fd);


      sys_write(fd, buffer, size); 
      f->eax = size;

      printf("cql_SYS_WRIT:successfully");
      break;

    case SYS_SEEK:
      printf("SYS_SEEK\n");
      break;
    case SYS_TELL:
      printf("SYS_TELL\n");
      break;
    case SYS_CLOSE:
      printf("SYS_CLOSE\n");
      break;
    default:
      printf("cql_TODO: syscall number: %d, not implemented!", syscall_number);

  }
  thread_exit ();
}


/*
  ##############################################
  #         Minor Function in SystemCalls      #
  ##############################################
*/
void sys_halt(void){
  shutdown_power_off();
}

void sys_exit(void){
  thread_exit();
}


void sys_write(int fd, const void *buffer, int size){
  printf("fd=%d\n",fd);
  printf("check_user((const uint8_t*) buffer):%d\n",check_user((const uint8_t*) buffer));

  printf("cql_SYS_check0\n");
  if(size<0 || !check_user((const uint8_t*) buffer)){
    printf("cql_SYS_check1\n");
    thread_exit();
  }
  
  printf("cql_SYS_check2\n");
  if(fd==1){
    printf("cql_SYS_check3\n");
    putbuf(buffer, size);
    printf("cql_SYS_check4\n");
  }
  else{
    printf("cql_warning for SYS_WRITE: fd!=1\n");
  }

  return;
}

pid_t sys_exec(const char *cmdline){
   tid_t child_tid = process_execute(cmdline);
   return child_tid;

}




/*
  ##################################
  #         Support Function       #
  ##################################
*/
bool
check_user(const uint8_t *uaddr){
  if ((void*)uaddr < PHYS_BASE){
    return true;
  }
  return false;
}

bool
check_Bytes(void *pointer, size_t bytes){
  for(int i=0; i<bytes; i++){
    
    //TODO?  check_user((const uint8_t)(pointer +i))
    if(!check_user(pointer +i)){
      return false;
    }
  }

  return true;
}

static int32_t
get_user (const uint8_t *uaddr) {

   int result;
   asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
   return result;
}

void
get_bytes (void *src, void *dst, size_t bytes) {

  //graphic:  memcpy(dst, src, size_t bytes)
  memcpy(dst, src, sizeof(char)*bytes); 

  return;
}


