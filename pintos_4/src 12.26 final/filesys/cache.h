#ifndef FILESYS_BUFFER_CACHE_H
#define FILESYS_BUFFER_CACHE_H

#include "threads/synch.h"
typedef int32_t off_t;


struct buffer_entry
{
    bool dirty; 
    bool valid;      
    int32_t sector;  
    bool clock_bit;     
    struct lock lock;   
    void *data;        
};

bool bc_read (int32_t sector_idx, void *buffer, 
              off_t buffer_ofs, int chunk_size, int sector_ofs);
bool bc_write (int32_t sector_idx, void *buffer, 
               off_t buffer_ofs, int chunk_size, int sector_ofs);
void bc_init (void);
void bc_term (void);
struct buffer_entry *bc_lookup (int32_t sector);
struct buffer_entry *bc_select_victim (void);

void bc_flush_entry (struct buffer_entry*);
void bc_flush_all_entries (void);


#endif
