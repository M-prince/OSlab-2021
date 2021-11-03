// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define  NBUCKETS 13

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
  struct buf hashbucket[NBUCKETS];
} bcache;

int 
hash (int n) {
  return n % NBUCKETS;
}

void
binit(void)
{
  struct buf *b;

  //initlock(&bcache.lock, "bcache");

  // Clear the pointer
  for(int i = 0; i < NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache.hashbucket");
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  //alloc all the buf to the first bucket
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = hash(blockno);
  acquire(&bcache.lock[index]);

  // Is the block already cached?
  for(b = bcache.hashbucket[index].next; b != &bcache.hashbucket[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  int index_next = (index + 1) % NBUCKETS;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  while (index != index_next)
  {
    acquire(&bcache.lock[index_next]);
    for(b = bcache.hashbucket[index_next].prev; b != &bcache.hashbucket[index_next]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        //insert the buf to the new bucket
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.hashbucket[index].next;
        b->prev = &bcache.hashbucket[index];
        bcache.hashbucket[index].next->prev = b;
        bcache.hashbucket[index].next = b;
        release(&bcache.lock[index_next]);
        release(&bcache.lock[index]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[index_next]);
    index_next = (index_next + 1) % NBUCKETS;
  }
  release(&bcache.lock[index]);
  panic("bget: no buffers");
}



// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int index = hash(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[index].next;
    b->prev = &bcache.hashbucket[index];
    bcache.hashbucket[index].next->prev = b;
    bcache.hashbucket[index].next = b;
  }
  
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  int index = hash(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void
bunpin(struct buf *b) {
  int index = hash(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}
