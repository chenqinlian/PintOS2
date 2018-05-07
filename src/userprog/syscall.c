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
int sys_read(int fd, void *buffer, unsigned size);
int sys_write(int fd, const void *buffer, unsigned size);
void sys_halt (void);
void sys_exit (int);
pid_t sys_exec (const char *cmdline);
int sys_wait(pid_t pid);
bool sys_create(char *filename, unsigned filesize);
bool sys_remove(char *filename);
int sys_open(char *filename);
void sys_close(int fdnumber);


void getfd(struct list *fd_list, struct file_descriptor **mrright, int fd);



//memory check function
bool check_addr (const uint8_t *uaddr);
bool check_buffer (void* buffer, unsigned size);
static int get_user (const uint8_t *uaddr);

static int
memread_user (void *src, void *dst, size_t bytes);

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

      int return_code = sys_wait(pid);

      f->eax = return_code;
      break;
    }

  case SYS_CREATE:{
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



  case SYS_REMOVE:
  {

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
  case SYS_OPEN:
  {
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
  {
      int fd = *(int *)(f->esp+4);
      void *buffer = (void *)(f->esp+8);
      unsigned size = *(unsigned *)(f->esp+12);

      //printf("fd:%d\n",fd);

      //printf("size:%d\n",size);

      int return_code = sys_read(fd, buffer, size);
      f->eax= return_code;
      break;



  }
  case SYS_WRITE:
    {
      int fd = *(int *)(f->esp+4);
      void *buffer = (void *)(f->esp+8);
      size_t size = *(size_t *)(f->esp+12);

      int return_code = sys_write(fd, buffer, size);
      f->eax= return_code;
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

int sys_read(int fd, void *buffer, unsigned size){

    
      if(fd == 0) { // stdin
        unsigned i;
        for(i = 0; i < size; ++i) {
        ((uint8_t *)buffer)[i] = input_getc();
        }
        return size;
      }


      else{ 
        //TODO


        struct file_descriptor* file_toread = NULL;

        struct thread *t = thread_current();  
        struct list *fd_list = &(t->file_descriptors);
  
        getfd(fd_list, &file_toread,fd); 

        //printf("reading from file\n");
        if(!list_empty(fd_list) && file_toread && file_toread->file) {
          int return_code = file_read(file_toread->file, *(char **)buffer, size);
          //return 239;
          return return_code; 
        }


        return -1;

      }
      
      return 239;
}

int sys_write(int fd, const void *buffer, unsigned size){
    
      
      //printf("sys_write\n");
      //printf("fd_numer:%d\n",fd);
																																																																																																																																																																				
      //printf("size:%d\n",size);

      //Case1: print to screem
      if(fd == STDOUT_FILENO)
      {
        putbuf (*(char **)buffer, size);
        return size;
      }
    
      //Case2: print to file 
      else{ 
        //TODO


        struct file_descriptor* file_towrite = NULL;

        struct thread *t = thread_current();  
        struct list *fd_list = &(t->file_descriptors);
  
        getfd(fd_list, &file_towrite,fd); 

        if(!list_empty(fd_list) && file_towrite && file_towrite->file) {
          int return_code = file_write(file_towrite->file, *(char **)buffer, size);
          //return 239;
          return return_code; 
        }


        return -1;

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

int sys_open(char* filename) {

  struct file* file_opened = filesys_open(filename);;

  struct file_descriptor* fd = malloc(sizeof(struct file_descriptor));



  if (file_opened!=NULL) {

    //TODO:rewrite sys_open

    fd->file = file_opened; //file save

    struct list* fd_list = &thread_current()->file_descriptors;
    if (list_empty(fd_list)) {
      // 0, 1, 2 are reserved for stdin, stdout, stderr
      fd->fd_number = 3;
    }
    else {
      fd->fd_number = (list_entry(list_back(fd_list), struct file_descriptor, elem)->fd_number) + 1;
    }
    list_push_back(fd_list, &(fd->elem));

    return fd->fd_number;
  }

  //palloc_free_page (fd);

  return -1;


}

void sys_close(int fd) {

  struct file_descriptor* file_toclose = NULL;

  struct thread *t = thread_current();  
  struct list *fd_list = &(t->file_descriptors);
  
  getfd(fd_list, &file_toclose,fd);  


  if(!list_empty(fd_list) && file_toclose && file_toclose->file) {
    file_close(file_toclose->file);
    list_remove(&(file_toclose->elem));
  }

  return;
}


void getfd(struct list *fd_list, struct file_descriptor **mrright, int fd)
{
  struct list_elem *iter = NULL;

  if(list_empty(fd_list)){
    return;
  }

  //printf("..getfd,list not empty\n");
  for(iter = list_begin(fd_list);iter != list_end(fd_list); iter = list_next(fd_list))
    {
      struct file_descriptor *desc = list_entry(iter, struct file_descriptor, elem);
      if(desc->fd_number == fd) {

        *mrright = desc;
        return;
      }
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

