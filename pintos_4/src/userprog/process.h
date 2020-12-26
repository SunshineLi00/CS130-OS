#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct child_finished_list{
    struct list_elem child_elem_f; 
    tid_t tid;
    int exit_inf_f;
    bool already_wait;
};
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
static bool
set_arg(const char *,void **);

#endif /* userprog/process.h */
