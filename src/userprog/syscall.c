#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"


static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{

  printf ("system call!\n");
  int syscall_number;


  /* Number of arguments that are used depends on syscall number.
     Max number of arguments is 3. */
  syscall_number = *(int *)(f->esp);
 
  printf ("system call! syscall number is : %d, ", syscall_number);
  switch(syscall_number)
  {
    case SYS_HALT:
      printf("SYS_HALT\n");
      break;
    case SYS_EXIT:
      printf("SYS_EXIT\n");
      break;
    case SYS_EXEC:
      printf("SYS_EXEC\n");
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
  ##################################
  #         Support Function       #
  ##################################
*/
bool
check_user(const uint8_t *uaddr){
  if ((void*)uaddr > PHYS_BASE){
    return false;
  }
  return true;

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

