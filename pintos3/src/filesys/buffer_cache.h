#ifndef FILESYS_BUFFER_CACHE_H
#define FILESYS_BUFFER_CACHE_H

#include "threads/synch.h"


#define BUFFER_CACHE_ENTRY_NB 64

/* buffer cache entry */
struct buffer_entry
{
    bool dirty;  
    bool valid; 
    block_sector_t sector;  
    bool clock_bit;   
    struct lock lock;  
    void *data;       
};

bool buffer_cache_read (block_sector_t sector_idx, void *buffer, 
              off_t buffer_ofs, int chunk_size, int sector_ofs);
bool buffer_cache_write (block_sector_t sector_idx, void *buffer, 
               off_t buffer_ofs, int chunk_size, int sector_ofs);
void buffer_cache_init (void);
void buffer_cache_term (void);

struct buffer_entry *buffer_cache_look_up (block_sector_t sector);
struct buffer_entry *buffer_cache_select_victim (void);
void buffer_cache_flush_entry (struct buffer_entry*);
void buffer_cache_flush_entries (void);

#endif
