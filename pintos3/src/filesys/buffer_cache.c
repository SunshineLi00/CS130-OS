#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "filesys/buffer_cache.h"
#include "threads/malloc.h"

#include <string.h>
#include <stdio.h>

#define BUFFER_CACHE_ENTRY_NB 64

void *p_buffer_cache; 
struct buffer_entry buffer_entry[BUFFER_CACHE_ENTRY_NB]; //bufferhead array
static int clock_hand; 



bool buffer_cache_read (block_sector_t sector_i, void *buffer, off_t bytes_read, 
              int chunk_size, int sector_ofs) {
    
    struct buffer_entry *bf_head;
  
    
    if (!(bf_head = buffer_cache_look_up(sector_i))) {
        if (!(bf_head = buffer_cache_select_victim()))
            return false;
       
        //lock
        lock_acquire(&bf_head->lock);

        block_read(fs_device, sector_i, bf_head->data);

        //Alloc buffer head
        
		bf_head->valid = true;
        bf_head->sector = sector_i;
        bf_head->dirty = false;

        //unlock
        lock_release(&bf_head->lock);
    }
    //lock before setting
    lock_acquire (&bf_head->lock);

    memcpy (buffer + bytes_read, bf_head->data + sector_ofs, chunk_size);

    bf_head->clock_bit = true;
    //unlock
    lock_release(&bf_head->lock);
    
    return true;
}

bool buffer_cache_write (block_sector_t sector_i, void *buffer, off_t
        bytes_written, int chunk_size, int sector_ofs) {

    struct buffer_entry *bf_head;
    lock_acquire(&bf_head->lock);

    if (!(bf_head = buffer_cache_look_up (sector_i))) {
        if (!(bf_head = buffer_cache_select_victim()))
            return false;
        block_read (fs_device, sector_i, bf_head->data);
    }

   
    memcpy(bf_head->data + sector_ofs, buffer + bytes_written, chunk_size);

    /* update buffer*/
	bf_head->sector = sector_i;
	bf_head->valid = true;
    bf_head->clock_bit = true;
    bf_head->dirty = true;
    
    
    lock_release(&bf_head->lock);

    return true;;
}


void buffer_cache_init (void) {

    int i;
    void *p_data;  

    /* Allocation buffer cache in Memory */
    p_buffer_cache = malloc(BLOCK_SECTOR_SIZE*BUFFER_CACHE_ENTRY_NB);

    if (p_buffer_cache == NULL) {
        printf("\n[%s] Memory Allocation Fail.\n",__FUNCTION__);
        return;
    }
    else{
        p_data = p_buffer_cache;
    }
    for(i=0; i<BUFFER_CACHE_ENTRY_NB; i++){
        buffer_entry[i].dirty = false;
        buffer_entry[i].valid = false;
        buffer_entry[i].sector = -1;
        buffer_entry[i].clock_bit = 0;
        lock_init(&buffer_entry[i].lock);
        buffer_entry[i].data = p_data;
        p_data += BLOCK_SECTOR_SIZE;
    }
}

void buffer_cache_term(void) {

    buffer_cache_flush_entries();

    free(p_buffer_cache);
}

struct buffer_entry *buffer_cache_select_victim (void) {
    int i;
    while(true){
        i = clock_hand;
        if(buffer_entry[i].clock_bit){
            lock_acquire(&buffer_entry[i].lock);
            buffer_entry[i].clock_bit = 0;
            lock_release(&buffer_entry[i].lock);
        }
        else{
            lock_acquire(&buffer_entry[i].lock);
            buffer_entry[i].clock_bit = 1;
            lock_release(&buffer_entry[i].lock);
            break;
        }
    }    
    if(buffer_entry[i].dirty == true){
        buffer_cache_flush_entry(&buffer_entry[i]);
    }

    lock_acquire(&buffer_entry[i].lock);
    buffer_entry[i].dirty = false;
    buffer_entry[i].valid = false;
    buffer_entry[i].sector = -1;
    lock_release(&buffer_entry[i].lock);
   
    return &buffer_entry[i];
}


struct buffer_entry* buffer_cache_look_up (block_sector_t sector) {
    int i;
    for (i = 0; i < BUFFER_CACHE_ENTRY_NB; i++) {
        if (buffer_entry[i].sector == sector) {
            return &buffer_entry[i];
        }
    }
 
    return NULL;
}

void buffer_cache_flush_entry (struct buffer_entry *p_flush_entry) {
    lock_acquire(&p_flush_entry->lock);
    block_write(fs_device, p_flush_entry->sector, p_flush_entry->data);  
    p_flush_entry->dirty = false;
    lock_release(&p_flush_entry->lock);
}

void buffer_cache_flush_entries( void) {
    int i;
    for (i = 0; i < BUFFER_CACHE_ENTRY_NB; i++) {
        if (buffer_entry[i].dirty == true) {
            buffer_cache_flush_entry (&buffer_entry[i]);
            
        }
    }
}
