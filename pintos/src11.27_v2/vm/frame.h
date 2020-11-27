#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdbool.h>
#include "threads/synch.h"

/* A physical frame. */
struct frame 
  {
    struct page *page;    /* need a physical page */
    struct lock lock;     /* lock of the physical frame */      
    void *base;                         
  };

void frame_init (void);
struct frame *frame_alloc_and_lock (struct page *);
void frame_lock (struct page *);
void frame_unlock (struct frame *);
void frame_free (struct frame *);
#endif /* vm/frame.h */
