#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/*the files we add*/
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "pagedir.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "process.h"
#include "vm/page.h"
#include "vm/frame.h"

static void syscall_handler (struct intr_frame *);

void halt (void);
void exit(int status);
pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
int munmap (int handle);
int mmap (int handle,void *start_addr);

void is_valid_addr (const void *addr);
void is_valid_block (void *buffer, unsigned size);
struct file_d *find_fd (int fd);

struct lock sys_lock;
/* initiallize */
void
syscall_init (void) 
{
  lock_init (&sys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int *syscall_value = (int *)f->esp;
  //printf("address%0x",syscall_value);
  is_valid_block((void *)syscall_value,3);
  //is_valid_addr (syscall_value);
  if (*syscall_value < 0 || *syscall_value > 19)
    exit(-1);
  
  int *sys_1 = (int *)f->esp + 1;
  int *sys_2 = (int *)f->esp + 2;
  int *sys_3 = (int *)f->esp + 3;
  is_valid_block((void *)sys_1,0);
  switch(*syscall_value){
     case SYS_HALT:
    {
      halt ();
      break;
    }
    case SYS_EXIT:
    {
      is_valid_block((void *)sys_1,3);
      exit(*sys_1);
      break;
    }
    case SYS_EXEC:
    {
      is_valid_block ((void *)sys_1, 3);
      is_valid_block ((void *)(*sys_1), 1);
      f->eax = exec ((const char*)(*sys_1));
      break;
    }
    case SYS_WAIT:
    {
      is_valid_block ((void *)sys_1, 3);
      f->eax = wait ((pid_t)(*sys_1));
      break;
    }
    case SYS_CREATE:
    {
      is_valid_block ((void *)sys_1, 3);
      is_valid_block ((void *)sys_2, 3);
      is_valid_block ((void *)(*sys_1), (unsigned)(*sys_2));
      f->eax = create ((const char*)(*sys_1),(unsigned)(*sys_2));
      break;
    }
    case SYS_REMOVE:
    {
      is_valid_block((void *)sys_1,3);
      is_valid_block ((void *)(*sys_1), 0);
      f->eax = remove ((const char*)(*sys_1));
      break;
    }
    case SYS_OPEN:
    {
      is_valid_block((void *)sys_1,3);
      is_valid_block ((void *)(*sys_1), 0);
      f->eax = open ((const char *)(*sys_1));
      break;
    }
  case SYS_FILESIZE:
    {
      is_valid_block((void *)sys_1,3);
      f->eax = filesize (*sys_1);
      break;
    }
    case SYS_READ:
    {
      is_valid_block((void *)sys_1,3);
      is_valid_block((void *)sys_2,3);
      is_valid_block((void *)sys_3,3);
      is_valid_block ((void *)(*sys_2), (unsigned)(*sys_3));
      f->eax = read (*sys_1, (void *)(*sys_2), (unsigned)(*sys_3));
      break;
    }

    case SYS_WRITE:
    {
      is_valid_block((void *)sys_1,3);
      is_valid_block((void *)sys_2,3);
      is_valid_block((void *)sys_3,3);
      is_valid_block ((void *)(*sys_2), (unsigned)(*sys_3));
      f->eax = write (*sys_1, (void *)(*sys_2), (unsigned)(*sys_3));

      break;
    }

    case SYS_SEEK:
    {
      is_valid_block((void *)sys_1,3);
      is_valid_block((void *)sys_2,3);
      seek(*sys_1, (unsigned)(*sys_2));
      break;
    }
    case SYS_TELL:
    {
      is_valid_block((void *)sys_1,3);
      f->eax = tell(*sys_1);
      break;
    }
    case SYS_CLOSE:
    {
      is_valid_block((void *)sys_1,3);
      close(*sys_1);
      break;
    }
    case SYS_MMAP:
    {
      is_valid_block((void *)sys_1,3);
      is_valid_block((void *)sys_2,3);
      mmap(*sys_1,(void*)*sys_2);
      break;
    }
    case SYS_MUNMAP:
    {
      is_valid_block((void *)sys_1,3);
      munmap(*sys_1);
      break;
    }
  }
}
/* judge the address is valid*/
void
is_valid_addr (const void *addr)
{
  if (addr == NULL /*|| addr<=0x08008400*/ || !is_user_vaddr (addr) || pagedir_get_page (thread_current ()->pagedir, addr) == NULL)
    {
      if (lock_held_by_current_thread (&sys_lock))
        lock_release (&sys_lock); /* release the lock */
        //printf("%0x",addr);
      exit (-1);
    }
}
/* judge the block is valid*/
void
is_valid_block (void *buffer, unsigned size)
{
  for (unsigned i = 0; i <= size; i++)
    is_valid_addr ((const char *)((const char *)buffer+i));
}

/*System Call: void halt (void)
Terminates Pintos by calling shutdown_power_off()*/
void
halt (void)
{
  shutdown_power_off ();
}

/*System Call: void halt (void)
Terminates Pintos by calling shutdown_power_off()*/
void
exit (int status)
{
  thread_current()->exit_inf=status;
  //if(status ==-1) printf("$^#$");
  printf ("%s: exit(%d)\n", thread_name(), status);
  thread_exit ();
}

/*System Call: pid_t exec (const char *cmd_line)
Runs the executable whose name is given in cmd_line, passing any given arguments, and returns the new process's program id (pid).*/
pid_t
exec (const char *cmd_line)
{
  lock_acquire (&sys_lock);
  pid_t pid = (pid_t)process_execute (cmd_line);
  lock_release (&sys_lock);
  return pid;
}

/*System Call: pid_t exec (const char *cmd_line)
Runs the executable whose name is given in cmd_line, passing any given arguments, and returns the new process's program id (pid).*/
int
wait (pid_t pid)
{
  //printf("wait:%d |",pid);
  return process_wait (pid);
}

/* System Call: bool create (const char *file, unsigned initial_size)
    Creates a new file called file initially initial_size bytes in size. */
bool
create (const char *file, unsigned initial_size)
{
  if (file==NULL) return false;
  bool temp;
  lock_acquire (&sys_lock);
  temp =  filesys_create (file, initial_size);
  lock_release (&sys_lock);
  return temp;
}

/*System Call: bool remove (const char *file)
    Deletes the file called file. */
bool
remove (const char *file)
{
  bool temp;
  lock_acquire (&sys_lock);
  temp =  filesys_remove (file);
  lock_release (&sys_lock);
  return temp;
}

/* System Call: int open (const char *file)
    Opens the file called file.*/
int
open (const char *file)
{
   
  struct file_d* file_fd = palloc_get_page(0);
  if (!file_fd)
    return -1;
  if(file==NULL) return -1;
  lock_acquire (&sys_lock);
  struct file *open_file=filesys_open(file);
  if (!open_file) {
    palloc_free_page ( file_fd );
    lock_release (&sys_lock);
    return -1;
  }
  file_fd ->file = open_file;
  
  struct list* fd_list = &thread_current()->fd_list;
  if (list_empty (fd_list))
    file_fd ->fd = 2;
  else
    file_fd ->fd = list_entry (list_back (fd_list), struct file_d, elem)->fd;
  file_fd ->fd ++;
  list_push_back(fd_list, &file_fd ->elem);
  lock_release (&sys_lock);
  return file_fd->fd;
}

/*System Call: int filesize (int fd)
Returns the size, in bytes, of the file open as fd.*/

int filesize (int fd){
  struct file_d *file_fd = NULL;
  int size;
  lock_acquire (&sys_lock);
  file_fd = find_fd (fd);
  if (file_fd == NULL) {
    lock_release (&sys_lock);
    return -1;
  }
  size = file_length(file_fd->file);
  lock_release (&sys_lock);
  return size;

}


struct file_d *
find_fd (int fd)
{
  struct file_d *file_fd =NULL;//(struct file_d *) malloc(sizeof(struct file_d));
  struct list*  fd_list = &(thread_current ()->fd_list);
  struct list_elem *e;
  /* Search file_fd in file_list */
  if (list_empty(fd_list ))
  { // free(file_fd);
    return NULL;}
  for(e = list_begin (fd_list ); e != list_end (fd_list ); e = list_next (e))
    {
      file_fd = list_entry(e, struct file_d, elem);
      if(file_fd->fd == fd)
        return file_fd;
    }
   // free(file_fd);
     // printf("survive");
  return NULL;
}
/*System Call: int read (int fd, void *buffer, unsigned size)
Reads size bytes from the file open as fd into buffer. 
Returns the number of bytes actually read (0 at end of file), 
or -1 if the file could not be read (due to a condition other than end of file). 
Fd 0 reads from the keyboard using input_getc().*/

int read (int fd, void *buffer, unsigned size){
  lock_acquire (&sys_lock);
  if(fd==STDIN_FILENO)
  {
    lock_release (&sys_lock);
    return -1;
  }
  if(fd == STDIN_FILENO)
    {
      for(unsigned i = 0; i < size; i++)
        *(uint8_t *)(buffer + i) = input_getc ();
      lock_release (&sys_lock);
      return (int)size;
    }

  struct file_d *file_fd = find_fd (fd);
  if (file_fd == NULL || file_fd->file == NULL)
    {
     lock_release (&sys_lock);
      return -1;
    }
  int t = file_read (file_fd->file, buffer, size);
  lock_release (&sys_lock);
  
  return t;

}

/*System Call: int write (int fd, const void *buffer, unsigned size)
Writes size bytes from buffer to the open file fd. Returns the number of bytes actually written, 
which may be less than size if some bytes could not be written.
Writing past end-of-file would normally extend the file, 
but file growth is not implemented by the basic file system.
The expected behavior is to write as many bytes as possible up to end-of-file and return the actual number written,
or 0 if no bytes could be written at all.*/

int write (int fd, const void *buffer, unsigned size){
   lock_acquire (&sys_lock);
  //printf("survive");
  if(fd==STDIN_FILENO) 
  {
    lock_release (&sys_lock);
    return -1;
  }
  if(fd == STDOUT_FILENO)
    {
      putbuf (buffer, size);
      lock_release (&sys_lock);
      return size;
    }
  /*if(fd==NULL)
  { 
    lock_release (&sys_lock);
    return -1;
    }*/
  struct file_d *file_fd = find_fd (fd);
  // printf("survive");
  if (file_fd == NULL || file_fd->file == NULL)
    {
      lock_release (&sys_lock);
      return -1;
    }
  int t = file_write (file_fd->file, buffer, size);
  lock_release (&sys_lock);
  //printf("survive");
  return t;

}


/*System Call: void seek (int fd, unsigned position)
Changes the next byte to be read or written in open file fd to position, expressed in bytes from the beginning of the file. */

void seek (int fd, unsigned position){
   lock_acquire (&sys_lock);/* acquire lock */
  struct file_d *file_fd = find_fd (fd);
  if (file_fd == NULL || file_fd->file == NULL)
    {
      lock_release (&sys_lock);/* release  lock */
      return;
    }
  file_seek(file_fd->file, position);
  lock_release (&sys_lock);/* release  lock */
}

/*System Call: unsigned tell (int fd)
Returns the position of the next byte to be read or written in open file fd, expressed in bytes from the beginning of the file.*/

unsigned tell (int fd){
lock_acquire (&sys_lock);/* acquire lock */
  struct file_d *file_fd = find_fd (fd);
  if (file_fd == NULL || file_fd->file == NULL)
    {
      lock_release (&sys_lock);/* release  lock */
      return -1;
    }
  unsigned t = file_tell (file_fd->file);
  lock_release (&sys_lock);/* release  lock */
  return t;
}


/*System Call: void close (int fd)
Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open file descriptors, as if by calling this function for each one.*/
void close (int fd){
lock_acquire (&sys_lock);/* acquire lock */
  struct file_d *file_fd = find_fd (fd);
  if (file_fd == NULL || file_fd->file == NULL)
    {
      lock_release (&sys_lock);
      return;
    }
  file_close(file_fd->file);
  list_remove(&file_fd->elem);
  //palloc_free_page (file_fd);
  lock_release (&sys_lock);/* release  lock */
}

struct mapping *find_mapping (int handle)
{
  struct mapping *map =NULL;
  struct list*  mp_list = &(thread_current ()->mp_list);
  struct list_elem *e;
  /* Search mapping in mp_list */
  if (list_empty(mp_list ))
  { 
    return NULL;
  }
  for(e = list_begin (mp_list ); e != list_end (mp_list ); e = list_next (e))
    {
      map = list_entry(e, struct mapping, elem);
      if(map->mid == handle)
        return map;
    }
  return NULL;
}
int mmap(int handle, void *start_addr)
{
  lock_acquire (&sys_lock);/* acquire lock */
  /*get the file with fd(handle here)*/
  struct file_d *file_fd = find_fd (handle);
  if (file_fd == NULL || file_fd->file == NULL || start_addr == NULL || pg_ofs (start_addr) != 0)
    {
      lock_release (&sys_lock);
      return -1;
    }
  struct mapping *m = palloc_get_page(0);
  if (m == NULL)
  {
      lock_release (&sys_lock);
      return -1;
  }
  struct list* mp_list = &thread_current()->mp_list;
  /*initial m*/
  if (list_empty (mp_list))
     m->mid= 1;
  else
     m->mid= list_entry (list_back (mp_list), struct mapping, elem)->mid+1;
  //m->file = file_reopen (file_fd->file);
  m->file=file_fd->file;
  if (m->file == NULL)
    {
      palloc_free_page ( m );
      lock_release (&sys_lock);
      return -1;
    }
  m->start =start_addr;
  m->page_cnt = 0;
  int i,length= file_length (m->file);
  /*allocate the pages for the file*/
  for(i=length; i>0; i=i-PGSIZE)
  {
      m->page_cnt++;
      struct page *p = page_allocate ((uint8_t *)m->start+length-i, false);
      if (p == NULL)
        {
          palloc_free_page ( m );
          lock_release (&sys_lock);
          return -1;
        } 
      p->file = m->file;
      if (i>= PGSIZE)
        p->file_bytes -= PGSIZE;
      else
        p->file_bytes -= i;
      p->file_offset = length-i;
      p->private = false;
      

  }
  /*insert to the mapping list*/
  list_push_back(mp_list, &m->elem);
  lock_release (&sys_lock);
  return m->mid;
}

int munmap (int handle)
{
  
  lock_acquire (&sys_lock);/* acquire lock */
  /* get the map by handle */
  struct mapping *m= find_mapping (handle);
  list_remove(&m->elem);
  int page_ad=m->start;
  /*write back to disk for each page*/
  for(int i = 0; i < m->page_cnt; i++)
  {
    
    /* use the dirty bit*/
    if (pagedir_is_dirty(thread_current()->pagedir, (const void *) (page_ad)))
         file_write_at(m->file, (const void *) page_ad, PGSIZE*(m->page_cnt), PGSIZE * i);
    page_deallocate((void *) page_ad);
    page_ad+= PGSIZE;
  }
  lock_release (&sys_lock);
  return 0;
}
