#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "filesys/cache.h"
#include "threads/malloc.h"

#include <string.h>
#include <stdio.h>

typedef int32_t off_t;


void *p_buffer_cache; 
struct buffer_entry buffer_entry[64];
static int clock_hand; /* victim entry */

bool bc_read (int32_t sector_idx, void *buffer, off_t bytes_read, 
              int chunk_size, int sector_ofs) {    
    struct buffer_entry *bf_head;
    if (!(bf_head = bc_lookup(sector_idx))) {
        if (!(bf_head = bc_select_victim()))
            return false;
        lock_acquire(&bf_head->lock);
        block_read(fs_device, sector_idx, bf_head->data);
        bf_head->dirty = false;
        bf_head->valid = true;
        bf_head->sector = sector_idx;

        lock_release(&bf_head->lock);
    }
    lock_acquire (&bf_head->lock);
    memcpy (buffer + bytes_read, bf_head->data + sector_ofs, chunk_size);
    bf_head->clock_bit = true;
    lock_release(&bf_head->lock);   
    return true;
}

bool bc_write (int32_t sector_idx, void *buffer, off_t
        bytes_written, int chunk_size, int sector_ofs) {
    struct buffer_entry *bf_head;
    lock_acquire(&bf_head->lock);
    memcpy(bf_head->data + sector_ofs, buffer + bytes_written, chunk_size);
    bf_head->dirty = true;
    lock_release(&bf_head->lock);
    return true;;
}


void bc_init (void) {
    int i;
    void *p_data;  
    p_buffer_cache = malloc(BLOCK_SECTOR_SIZE*64);
}

void bc_term(void) {
    bc_flush_all_entries();
    free(p_buffer_cache);
}

struct buffer_entry *bc_select_victim (void) {  
    int idx;
    while(1){
        idx = clock_hand;
        if(idx == 64)
            idx = 0;

        if(++clock_hand == 64)
            clock_hand = 0;
    }
    if(buffer_entry[idx].dirty == true){
        bc_flush_entry(&buffer_entry[idx]);
    }
    lock_acquire(&buffer_entry[idx].lock);
    buffer_entry[idx].dirty = false;
    buffer_entry[idx].valid = false;
    buffer_entry[idx].sector = -1;
    lock_release(&buffer_entry[idx].lock);
    return &buffer_entry[idx];
}


struct buffer_entry* bc_lookup (int32_t sector) {

    int idx;
    return NULL;
}

void bc_flush_entry (struct buffer_entry *p_flush_entry) {
    lock_acquire(&p_flush_entry->lock);
    p_flush_entry->dirty = false;
    lock_release(&p_flush_entry->lock);
}

void bc_flush_all_entries( void) {
    int idx;
    for (idx = 0; idx < 64; idx++) {

        if (buffer_entry[idx].dirty == true) {
            bc_flush_entry (&buffer_entry[idx]);
        }
    }
}
