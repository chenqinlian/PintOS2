#include "devices/shutdown.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);


//help function
int sys_badmemory_access(void);
void sys_write(int fd, const void *buffer, unsigned size);
void sys_halt (void);
void sys_exit (int);
pid_t sys_exec (const char *cmdline);
int sys_wait(pid_t pid);
bool sys_create(char *filename, unsigned filesize);
bool sys_remove(char *filename);
int sys_open(char *filename);
void sys_close(int fdnumber);
void getfd(struct list *fd_list, struct file_descriptor **fd_toremove_pointer, int fdnumber);

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



      //check buffer
      check_buffer(f->esp+4, sizeof(cmdline));

      //check cmdline
      if( get_user((const uint8_t *)cmdline)<0){
        sys_badmemory_access();
      }

      //printf("cmdline:%s\n", (const char*) cmdline);

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

      bool return_code = sys_wait(pid);

      f->eax = return_code;
      break;
    }

  case SYS_CREATE:
    {


      //check whether pointer is below PHYS_BASE
      if(!check_buffer(f->esp+4, sizeof(char*))){
        sys_badmemory_access();
      } 

      if(!check_buffer(f->esp+8, sizeof(unsigned))){
        sys_badmemory_access();
      } 

      char* filename = *(char **)(f->esp+4);
      unsigned filesize = *(unsigned **)(f->esp+8);
      
      //printf("filename:%s\n",filename);
      //printf("filesize:%d\n",filesize);

      //check valid memory access
      if( get_user((const uint8_t *)filename)<0){
        sys_badmemory_access();
      }

      int return_code = sys_create(filename, filesize);
      f->eax = return_code;
      break;

    }


  case SYS_REMOVE:{

      //check whether pointer is below PHYS_BASE
      if(!check_buffer(f->esp+4, sizeof(char*))){
        sys_badmemory_access();
      } 

      char* filename = *(char **)(f->esp+4);

      //check valid memory access
      if( get_user((const uint8_t *)filename)<0){
        sys_badmemory_access();
      }

      int return_code = sys_remove(filename);
      f->eax = return_code;
      
      break;
  }

  case SYS_OPEN:{
      //check whether pointer is below PHYS_BASE
      if(!check_buffer(f->esp+4, sizeof(char*))){
        sys_badmemory_access();
      } 

      char* filename = *(char **)(f->esp+4);

      //check valid memory access
      if( get_user((const uint8_t *)filename)<0){
        sys_badmemory_access();
      }

      int return_code = sys_open(filename);
      f->eax = return_code;
      
      break;
  }

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
    {
      if(!check_buffer(f->esp+4, sizeof(int))){
        sys_badmemory_access();
      } 

      int fdnumber = *(int *)(f->esp+4);

      //check valid memory access
      if( get_user((const uint8_t *)(f->esp+4))<0){
        sys_badmemory_access();
      }

      //printf("sys_close,fd_number%d\n", fdnumber);
      
      sys_close(fdnumber);

      break;

    }


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

bool sys_create(char *filename, unsigned filesize){
  bool return_code = false;

  return_code = filesys_create(filename, filesize);

  return return_code;
}

bool sys_remove(char *filename){
  bool return_code = false;

  return_code = filesys_remove(filename);

  return return_code;

}

int sys_open(char *filename){

  //read file being open  
  struct file* fileopen = filesys_open(filename);

  //load file descriptor
  struct thread *t = thread_current();
  struct file_descriptor *fd =  malloc(sizeof(struct file_descriptor));//palloc_get_page(0);

  //read current thread's file descriptor list
  struct list *fd_list = &(t->file_descriptors);

  if(fileopen!=NULL){

    //printf("fileopened!\n");
    

    //TODO: put fd to fd_list, check if there is repeat fd

      //fd_inlist = getfd(fd_list,fd);
    


    fd->fd_number =  3;

       



    list_push_back(fd_list, &(fd->elem));
   
    printf("sys_open,add file. fd_number:%d\n",fd->fd_number);
    
    return fd->fd_number;

  }



  return -1;

}

void sys_close(int fdnumber){

  if(fdnumber<3){
    return;
  }

  struct file_descriptor *fd_toremove = NULL;

  //read curreent thread's file descriptor list
  struct thread *t = thread_current();
  struct list *fd_list = &(t->file_descriptors);


  printf("sys_close,fd_number%d\n", fdnumber);


  getfd(fd_list, &fd_toremove, fdnumber);  





  if(fd_toremove!=NULL && !list_empty(fd_list)){
    file_close(fd_toremove->file);
    list_remove(&(fd_toremove->elem));
  }

  return;
}

void getfd(struct list *fd_list, struct file_descriptor **fd_toremove_pointer, int fdnumber){

  struct list_elem *iter = NULL;
 

  if(list_empty(fd_list)){
    return;
  }

  printf("..getfd,list not empty\n");
  
  for (iter = list_front(fd_list); iter != list_end(fd_list); iter = list_next(fd_list)){

    printf("..getfd,itering\n");
    /*
    struct file_descriptor *fd_temp = list_entry(iter, struct file_descriptor, elem);
    if(fd_temp->fd_number == fdnumber){
      *fd_toremove_pointer = fd_temp;
    }
    */
  }
 

  return;
}

/****************** Helper Functions on Memory Access ********************/

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

