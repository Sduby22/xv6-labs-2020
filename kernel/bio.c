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

#define NBUCKET 13

struct entry {
  struct buf head;
  struct spinlock lock;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct entry bucket[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = 0;
    b->prev = 0;
    b->timestamp = 0;
    initsleeplock(&b->lock, "buffer");
  }

  for(int i = 0; i != NBUCKET; ++i) {
    initlock(&bcache.bucket[i].lock, "bcache.bucket");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *evict=0;

  uint bnum = blockno % NBUCKET;

  acquire(&bcache.bucket[bnum].lock);

  // Is the block already cached?
  for(b = bcache.bucket[bnum].head.next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket[bnum].lock);
      /*release(&bcache.lock);*/
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  release(&bcache.bucket[bnum].lock);
  acquire(&bcache.lock);
  acquire(&bcache.bucket[bnum].lock);

  // Is the block already cached?
  for(b = bcache.bucket[bnum].head.next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket[bnum].lock);
      release(&bcache.lock);
      /*release(&bcache.lock);*/
      acquiresleep(&b->lock);
      return b;
    }
  }
    
  while(1)
  {
    uint64 min = (uint64)1 << 62;
    for(b = bcache.buf; b != bcache.buf+NBUF; b++){
      if(b->refcnt == 0 && b->timestamp < min) {
        // b is less recently used free buffer.
        evict = b;
        min = b->timestamp;
      }
    }

    if (!evict) {
      panic("bget: no buffers");
    }

    uint bnum2 = evict->blockno % NBUCKET;

    if (bnum2 != bnum) {
      acquire(&bcache.bucket[bnum2].lock);
    }

    if (evict->refcnt != 0) {
      if (bnum2 != bnum) {
        release(&bcache.bucket[bnum2].lock);
      }
      continue;
    }

    evict->dev = dev;
    evict->blockno = blockno;
    evict->valid = 0;
    evict->refcnt = 1;

    if (evict->next)
      evict->next->prev = evict->prev;
    if (evict->prev)
      evict->prev->next = evict->next;

    if (bnum2 != bnum) {
      release(&bcache.bucket[bnum2].lock);
    }

    evict->next = bcache.bucket[bnum].head.next;
    evict->prev = &bcache.bucket[bnum].head;
    bcache.bucket[bnum].head.next = evict;

    release(&bcache.bucket[bnum].lock);
    release(&bcache.lock);
    /*release(&bcache.lock);*/
    acquiresleep(&evict->lock);

    return evict;
  }
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

  /*acquire(&bcache.lock);*/
  int bnum = b->blockno % NBUCKET;
  acquire(&bcache.bucket[bnum].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    /*b->next->prev = b->prev;*/
    /*b->prev->next = b->next;*/
    /*b->next = bcache.head.next;*/
    /*b->prev = &bcache.head;*/
    /*bcache.head.next->prev = b;*/
    /*bcache.head.next = b;*/
    b->timestamp = ticks;
  }
  release(&bcache.bucket[bnum].lock);
  
  /*release(&bcache.lock);*/
}

void
bpin(struct buf *b) {
  int bnum = b->blockno % NBUCKET;
  acquire(&bcache.bucket[bnum].lock);
  b->refcnt++;
  release(&bcache.bucket[bnum].lock);
}

void
bunpin(struct buf *b) {
  int bnum = b->blockno % NBUCKET;
  acquire(&bcache.bucket[bnum].lock);
  b->refcnt--;
  release(&bcache.bucket[bnum].lock);
}


