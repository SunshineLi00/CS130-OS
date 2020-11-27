#include "vm/page.h"
#include <stdio.h>
#include <string.h>
#include "vm/frame.h"
#include "vm/swap.h"
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

/* Maximum size of process stack, in bytes. */
/* Right now it is 1 megabyte. */
#define STACK_MAX (1024 * 1024)

/* Destroys a page, which must be in the current process's
   page table.  Used as a callback for hash_destroy(). */
static void 
destroy_page (struct hash_elem *elem)
{
  struct page *p = hash_entry (elem, struct page, hash_elem);
  frame_lock (p);
  if (p->frame) frame_free (p->frame);
  free (p);
}
/* Destroys the current process's page table. */
void
page_exit (void)
{
  if (thread_current ()->pages != NULL) hash_destroy (thread_current ()->pages, destroy_page);
}
/* Returns the page containing the given virtual ADDRESS,
   or a null pointer if no such page exists.
   Allocates stack pages as necessary. */
static struct page *
get_page (const void *addr)
{
  if (addr >= PHYS_BASE) return NULL;
  struct page p;
  struct hash_elem *e;
  /* Find the page stores address. */
  p.addr = (void *) pg_round_down (addr);
  e = hash_find (thread_current ()->pages, &p.hash_elem);
  if (e != NULL) return hash_entry (e, struct page, hash_elem);
  if ((p.addr > PHYS_BASE - STACK_MAX) && ((void *)thread_current()->user_esp - 32 < addr))
          return page_allocate (p.addr, false);
  return NULL;
}

/* Locks a frame for page P and pages it in.
   Returns true if successful, false on failure. */
static bool
do_page_in (struct page *p)
{
  /* Get a frame for the page. */
  p->frame = frame_alloc_and_lock (p);
  if (p->frame == NULL)
    return false;

  /* Copy data into the frame. */
  if (p->sector != (block_sector_t) -1)
    {
      /* Get data from swap. */
      swap_in (p);
    }
  else if (p->file != NULL)
    {
      /* Get data from file. */
      off_t read_bytes = file_read_at (p->file, p->frame->base,
                                        p->file_bytes, p->file_offset);
      off_t zero_bytes = PGSIZE - read_bytes;
      memset (p->frame->base + read_bytes, 0, zero_bytes);
      if (read_bytes != p->file_bytes)
        printf ("bytes read (%"PROTd") != bytes requested (%"PROTd")\n",
                read_bytes, p->file_bytes);
    }
  else
    {
      /* Provide all-zero page. */
      memset (p->frame->base, 0, PGSIZE);
    }

  return true;
}

/* Faults in the page containing FAULT_ADDR.
   Returns true if successful, false on failure. */
bool
page_in (void *fault_addr)
{
  struct page *p;
  bool success;

  /* Can't handle page faults without a hash table. */
  if (thread_current ()->pages == NULL)
    return false;

  p = get_page (fault_addr);
  if (p == NULL)
    return false;

  frame_lock (p);
  if (p->frame == NULL)
    {
      if (!do_page_in (p))
        return false;
    }
  ASSERT (lock_held_by_current_thread (&p->frame->lock));

  /* Install frame into page table. */
  success = pagedir_set_page (thread_current ()->pagedir, p->addr,
                              p->frame->base, !p->read_only);

  /* Release frame. */
  frame_unlock (p->frame);

  return success;
}

/* Evicts page P.
   P must have a locked frame.
   Return true if successful, false on failure. */
bool
page_out (struct page *p)
{
  bool dirty;
  bool ok = false;

  ASSERT (p->frame != NULL);
  ASSERT (lock_held_by_current_thread (&p->frame->lock));

  pagedir_clear_page(p->thread->pagedir, (void *) p->addr);

  /* Has the frame been modified? */
  /* If the frame has been modified, set 'dirty' to true. */
  dirty = pagedir_is_dirty (p->thread->pagedir, (const void *) p->addr);

  /* If the frame is not dirty (and file != NULL), we have sucsessfully evicted the page. */
  if(!dirty) ok = true;
  if (p->file == NULL)
  {
    ok = swap_out(p);
  }
  else
  {
    if (dirty)
    {
      if(p->private)
      {
        ok = swap_out(p);
      }
      else
      {
        ok = file_write_at(p->file, (const void *) p->frame->base, p->file_bytes, p->file_offset);
      }
    }
  }
  /* empty the frame held by the page. */
  if(ok) p->frame = NULL;
  return ok;
}

/* Returns true if page P's data has been accessed recently,
   false otherwise.
   P must have a frame locked into memory. */
bool
page_accessed_recently (struct page *p)
{
  bool was_accessed;

  ASSERT (p->frame != NULL);
  ASSERT (lock_held_by_current_thread (&p->frame->lock));

  was_accessed = pagedir_is_accessed (p->thread->pagedir, p->addr);
  if (was_accessed)
    pagedir_set_accessed (p->thread->pagedir, p->addr, false);
  return was_accessed;
}

/* Adds a mapping for user virtual address VADDR to the page hash
   table.  Fails if VADDR is already mapped or if memory
   allocation fails. */
struct page *
page_allocate (void *vaddr, bool read_only)
{
  struct thread *t = thread_current ();
  struct page *p = malloc (sizeof *p);
  if (p != NULL)
    {
      p->addr = pg_round_down (vaddr);
      p->read_only = read_only;
      p->private = !read_only;
      p->frame = NULL;
      p->sector = (block_sector_t) -1;
      p->file = NULL;
      p->file_offset = 0;
      p->file_bytes = 0;
      p->thread = thread_current ();
      if (hash_insert (t->pages, &p->hash_elem) != NULL)
        {
          /* Already mapped. */
          free (p);
          p = NULL;
        }
    }
  return p;
}

/* Evicts the page containing address VADDR
   and removes it from the page table. */
void
page_deallocate (void *vaddr)
{
  struct page *p = get_page (vaddr);
  ASSERT (p != NULL);
  frame_lock (p);
  if (p->frame)
    {
      struct frame *f = p->frame;
      if (p->file && !p->private)
        page_out (p);
      frame_free (f);
    }
  hash_delete (thread_current ()->pages, &p->hash_elem);
  free (p);
}

/* Returns a hash value for the page that E refers to. */
unsigned
page_hash (const struct hash_elem *e, void *aux UNUSED)
{
  struct page *p = hash_entry (e, struct page, hash_elem);
  return ((uintptr_t) p->addr) >> PGBITS;
}

/* Returns true if page A precedes page B. */
bool
page_less (const struct hash_elem *x,const struct hash_elem *y, void *aux UNUSED)
{
  struct page *a = hash_entry (x, struct page, hash_elem);
  struct page *b = hash_entry (y, struct page, hash_elem);
  return a->addr < b->addr;
}

/* Tries to lock the page containing ADDR into physical memory.
   If WILL_WRITE is true, the page must be writeable;
   otherwise it may be read-only.
   Returns true if successful, false on failure. */
bool
page_lock (const void *addr, bool will_write)
{
  struct page *p = get_page (addr);
  if (p == NULL || (p->read_only && will_write)) return false;
  frame_lock (p);
  if (p->frame != NULL) return true;  
  bool flag = pagedir_set_page (thread_current ()->pagedir,p->addr,p->frame->base,!p->read_only);
  return (do_page_in (p) && flag);    
}

/* Unlocks a page locked with page_lock(). */
void
page_unlock (const void *addr)
{
  struct page *p = get_page (addr);
  ASSERT (p != NULL);
  frame_unlock (p->frame);
}
