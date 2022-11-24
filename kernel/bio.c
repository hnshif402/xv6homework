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

#define HSIZE 13
#define MAX_TICKS (1UL << (sizeof(uint)*8)) - 1

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  //struct buf head;
} bcache;

struct {
  struct spinlock lock;
  struct buf head;
} bucket[HSIZE];  //buf buckets

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  //bcache.head.prev = &bcache.head;
  //bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    //b->next = bcache.head.next;
    //b->prev = &bcache.head;
    b->next = b->prev = b;
    b->ts = 0;
    b->refcnt = 0;
    initsleeplock(&b->lock, "buffer");
    //bcache.head.next->prev = b;
    //bcache.head.next = b;
  }

  for(int i = 0; i < HSIZE;  i++) {
    initlock(&bucket[i].lock, "bcache.bucket");
    bucket[i].head.prev = &bucket[i].head;
    bucket[i].head.next = &bucket[i].head;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  //printf("bget start.\n");
  struct buf *b, *lru=0;
  int i, bi;
  uint min = MAX_TICKS;
  
  i = blockno % HSIZE;
  
  acquire(&bucket[i].lock);
  // Is the block already cached?
  for(b = bucket[i].head.next; b != &bucket[i].head; b = b->next) {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket[i].lock);
      acquiresleep(&b->lock);
      return b;
    }
    // if not cached , need to find lru unused buf in this bucket chain.
    if(b->refcnt == 0) {
      if(b->ts < min) {
        min = b->ts;
        lru = b;
      }
    }
  }
  b = lru;
  if(b && b->refcnt == 0){
    //find an unused buf in this bucket chain.
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bucket[i].lock);
    acquiresleep(&b->lock);
    return b;
  }
  // not cached and not found an unused buf from this bucket chain.
  acquire(&bcache.lock);
  // find an unused buf from bcache.
  for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
    if(b->refcnt == 0) {
      // an unused buf is first used, not belong to any buckets.
      if(b->ts == 0) {
        //b->refcnt = 1;
        //release(&bcache.lock);
        break;
      } else {
        // an unused buf is linked in a bucket. need to remove from the bucket.
        bi = b->blockno % HSIZE;
        acquire(&bucket[bi].lock);
        b->prev->next = b->next;
        b->next->prev = b->prev;
        //b->next = b->prev = b;
        release(&bucket[bi].lock);
        //b->refcnt = 1;
        //release(&bcache.lock);
        break;
      }
    }
  }
  

  // not found an unused buf from bcache.
  if(b->refcnt > 0)
    panic("bget: no buffers");

  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;
  b->prev = &bucket[i].head;
  b->next = bucket[i].head.next;
  bucket[i].head.next->prev = b;
  bucket[i].head.next = b;
  release(&bcache.lock);
  release(&bucket[i].lock);
  acquiresleep(&b->lock);
  return b;  
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;
  //printf("bread start.\n");
  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  //printf("bread end.\n");
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  //printf("bwrite start.\n");
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
  // printf("bwrite end.\n");
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  //check if holding the buf's lock;
  if(!holdingsleep(&b->lock))
    panic("brelse");  
  releasesleep(&b->lock);

  int bi = b->blockno % HSIZE;
  acquire(&bucket[bi].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    //stick the timestamp to the buf
    acquire(&tickslock);
    uint tick0 = ticks;
    release(&tickslock);
    b->ts = tick0;
  }
  release(&bucket[bi].lock);
}

void
bpin(struct buf *b) {
  int bi = b->blockno % HSIZE;
  acquire(&bucket[bi].lock);
  b->refcnt++;
  release(&bucket[bi].lock);
}

void
bunpin(struct buf *b) {
  int bi = b->blockno % HSIZE;
  acquire(&bucket[bi].lock);
  b->refcnt--;
  release(&bucket[bi].lock);
}
