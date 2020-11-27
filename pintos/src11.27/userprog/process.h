#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct child_finished_list{
    struct list_elem child_elem_f;  /* list element.*/
    tid_t tid;            /* thread id*/
    int exit_inf_f;       /*exit_infromation(finished)*/
    bool already_wait;       /*if already_wait*/
};
struct mapping
  {
    struct list_elem elem;      /* list element */
    int mid;                 /* mapping id*/
    struct file *file;          /* the object file*/
    uint8_t *start;              /* start of memory mapping*/
    size_t page_cnt;            /* the number of pages mapped*/
  };
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
static bool
set_arg(const char *,void **);

#endif /* userprog/process.h */

