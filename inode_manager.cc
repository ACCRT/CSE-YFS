#include "inode_manager.h"  
#include <iostream>
using namespace std;
// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  /*
   *your lab1 code goes here.
   *if id is smaller than 0 or larger than BLOCK_NUM 
   *or buf is null, just return.
   *put the content of target block into buf.
   *hint: use memcpy
  */
  if(id < 0 || id > BLOCK_NUM-1 || buf == NULL)
  	return;
  memcpy(buf,blocks[id],BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf)
{
  /*
   *your lab1 code goes here.
   *hint: just like read_block
  */
  if(id < 0 || id > BLOCK_NUM-1 || buf == NULL)
  	return;
  memcpy(blocks[id],buf,BLOCK_SIZE);
}

// block layer -----------------------------------------

#define BITMAP_NUM 8
#define BIT_PER_BYTE 8
// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your lab1 code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.

   *hint: use macro IBLOCK and BBLOCK.
          use bit operation.
          remind yourself of the layout of disk.
   */
  char *buf_bitmap = (char*)malloc(BLOCK_SIZE*BITMAP_NUM);
  int i;
  for(i = 0; i < BITMAP_NUM; i++)
  	read_block(2+i,buf_bitmap+i*BLOCK_SIZE);
  
  blockid_t nowid = 2+BITMAP_NUM+INODE_NUM;
  uint32_t offset = nowid / BIT_PER_BYTE;
  uint32_t bit_offset = nowid % BIT_PER_BYTE;
  
  while((((*(buf_bitmap+offset))>>((BIT_PER_BYTE-1-bit_offset))&1)) == 1)
  {
  	nowid++;
  	offset = nowid / BIT_PER_BYTE;
  	bit_offset = nowid % BIT_PER_BYTE;
  }
  
  *(buf_bitmap+offset) = *(buf_bitmap+offset) | 1<<(BIT_PER_BYTE-1-bit_offset);
  
  for(i = 0; i < BITMAP_NUM; i++)
  	write_block(2+i,buf_bitmap+i*BLOCK_SIZE);
  
  free(buf_bitmap);
  
  char* buf = (char*)malloc(BLOCK_SIZE);
  memset(buf,0,BLOCK_SIZE);
  write_block(nowid,buf);
  free(buf);
  
  return nowid;
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your lab1 code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  char *buf_bitmap = (char*)malloc(BLOCK_SIZE*BITMAP_NUM);
  int i;
  for(i = 0; i < BITMAP_NUM; i++)
  	read_block(2+i,buf_bitmap+i*BLOCK_SIZE);
  
  uint32_t offset = id / BIT_PER_BYTE;
  uint32_t bit_offset = id % BIT_PER_BYTE;
  
  *(buf_bitmap+offset) = *(buf_bitmap+offset) & 0<<(BIT_PER_BYTE-1-bit_offset);
  
  for(i = 0; i < BITMAP_NUM; i++)
  	write_block(2+i,buf_bitmap+i*BLOCK_SIZE);
  
  free(buf_bitmap);
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */

uint32_t using_inodes[INODE_NUM];
 
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your lab1 code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
    
   * if you get some heap memory, do not forget to free it.
   */
  inode_t* tmp = new inode_t;
  tmp->type = type;
  tmp->size = 0;
  
  uint32_t inum;
  for(inum = 1; inum < INODE_NUM; inum++)
  	if(using_inodes[inum] == 0)
  	{
  		using_inodes[inum] = 1;
  		break;
  	}
  
  put_inode(inum,tmp);
  
  char *buf_bitmap = (char*)malloc(BLOCK_SIZE*BITMAP_NUM);
  int i;
  for(i = 0; i < BITMAP_NUM; i++)
  	bm->read_block(2+i,buf_bitmap+i*BLOCK_SIZE);
  
  blockid_t nowid = 2+BITMAP_NUM+inum-1;
  uint32_t offset = nowid / 8;
  uint32_t bit_offset = nowid % 8;
  
  //cout<<int(offset)<<" "<<int(bit_offset)<<endl;
  
  *(buf_bitmap+offset) = *(buf_bitmap+offset) | 1<<(7-bit_offset);
  //cout<<(int)*(buf_bitmap+offset)<<endl;
  for(i = 0; i < BITMAP_NUM; i++)
  	bm->write_block(2+i,buf_bitmap+i*BLOCK_SIZE);
  
  free(buf_bitmap);
  
  return inum;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your lab1 code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   * do not forget to free memory if necessary.
   */
  inode_t* tmp = get_inode(inum);
  tmp->type = 0;
  tmp->size = 0;
  
  char *buf_bitmap = (char*)malloc(BLOCK_SIZE*BITMAP_NUM);
  int i;
  for(i = 0; i < BITMAP_NUM; i++)
  	bm->read_block(2+i,buf_bitmap+i*BLOCK_SIZE);
  
  uint32_t offset = (2+BITMAP_NUM+inum-1) / 8;
  uint32_t bit_offset = (2+BITMAP_NUM+inum-1) % 8;
  
  if(((*(buf_bitmap+offset)>>(7-bit_offset))&1) == 1)
  	*(buf_bitmap+offset) = *(buf_bitmap+offset) & 0<<(7-bit_offset);

  for(i = 0; i < BITMAP_NUM; i++)
  	bm->write_block(2+i,buf_bitmap+i*BLOCK_SIZE);
  
  free(buf_bitmap);
  put_inode(inum,tmp);
  free(tmp);
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your lab1 code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out
   */
  inode_t* tmp = get_inode(inum);
  *size = tmp->size;
  if(*size == 0)
  	return;
  uint32_t bnum = (*size-1)/BLOCK_SIZE+1;
  *buf_out = (char*)malloc(BLOCK_SIZE*bnum);
  memset(*buf_out,0,BLOCK_SIZE*bnum);
  
  if(bnum<=NDIRECT)
  {
  	int i;
  	for(i = 0; i < bnum ; i++)
  		bm->read_block(tmp->blocks[i], *buf_out+i*BLOCK_SIZE);
  }
  else
  {
  	int i;
  	for(i = 0; i < NDIRECT; i++)
  		bm->read_block(tmp->blocks[i], *buf_out+i*BLOCK_SIZE);
  	char *buf_ind = (char*)malloc(BLOCK_SIZE);
  	bm->read_block(tmp->blocks[NDIRECT],buf_ind);
  	for(i = 0; i < bnum-NDIRECT; i++)
  		bm->read_block(*(uint32_t*)(buf_ind+sizeof(uint32_t)*i),*buf_out+(i+NDIRECT)*BLOCK_SIZE);
  	free(buf_ind);
  }
  
  free(tmp);
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your lab1 code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode.
   * you should free some blocks if necessary.
   */
   inode_t* tmp = get_inode(inum);
   uint32_t ori_size = tmp->size;
   tmp->size = size;
   uint32_t bnum;
   if(size != 0) bnum = (size-1)/BLOCK_SIZE+1;
   uint32_t ori_bnum = (ori_size-1)/BLOCK_SIZE+1;
   
   if(ori_size > 0)
   {
	   if(ori_bnum<=NDIRECT)
	   {
	   	 int i;
		 for(i = 0; i < ori_bnum ; i++)
		 	if(tmp->blocks[i] != 0)
  	 			bm->free_block(tmp->blocks[i]);
	   }
	   else
	   {
	   	 int i;
	   	 for(i = 0; i < NDIRECT; i++)
		 	if(tmp->blocks[i] != 0)
  	 			bm->free_block(tmp->blocks[i]);
  	 			
  	 	 char *buf_ind = (char*)malloc(BLOCK_SIZE);
  	 	 bm->read_block(tmp->blocks[NDIRECT],buf_ind);
   	 	 for(i = 0; i < ori_bnum-NDIRECT; i++)
   	 	 	 if(*(uint32_t*)(buf_ind+sizeof(uint32_t)*i) != 0)
   	 	 		 bm->free_block(*(uint32_t*)(buf_ind+sizeof(uint32_t)*i));
   	 	 bm->free_block(tmp->blocks[NDIRECT]);
  	     free(buf_ind);
	   }
   }
   
   if(bnum<=NDIRECT)
   {
  	 int i;
  	 for(i = 0; i < bnum ; i++)
  	 {
  	 	tmp->blocks[i] = bm->alloc_block();
  		bm->write_block(tmp->blocks[i],buf+i*BLOCK_SIZE);
  	 }
   }
   else
   {
   	 int i;
   	 for(i = 0; i < NDIRECT; i++)
   	 {
   	 	tmp->blocks[i] = bm->alloc_block();
  		bm->write_block(tmp->blocks[i], buf+i*BLOCK_SIZE);
  	 }
  	 
  	 tmp->blocks[NDIRECT] = bm->alloc_block();
  	 char *buf_ind = (char*)malloc(BLOCK_SIZE);
   	 for(i = 0; i < bnum-NDIRECT; i++)
   	 {
   	 	 uint32_t block = bm->alloc_block();
   	 	 *(uint32_t*)(buf_ind+sizeof(uint32_t)*i) = block;
  		 bm->write_block(block,buf+(i+NDIRECT)*BLOCK_SIZE);
  	 }
  	 bm->write_block(tmp->blocks[NDIRECT],buf_ind);
  	 free(buf_ind);
   }
   
   put_inode(inum,tmp);
   free(tmp);
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your lab1 code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  inode* tmp = get_inode(inum);
  if(tmp == NULL) return;
  a.type = tmp->type;
  a.size = tmp->size;
  a.atime = tmp->atime;
  a.ctime = tmp->ctime;
  a.mtime = tmp->mtime;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your lab1 code goes here
   * note: you need to consider about both the data block and inode of the file
   * do not forget to free memory if necessary.
   */
   inode_t* tmp = get_inode(inum);
   uint32_t ori_size = tmp->size;
   uint32_t ori_bnum = (ori_size-1)/BLOCK_SIZE+1;
   
   if(ori_size > 0)
   {
	   if(ori_bnum<=NDIRECT)
	   {
	   	 int i;
		 for(i = 0; i < ori_bnum ; i++)
		 	if(tmp->blocks[i] != 0)
  	 			bm->free_block(tmp->blocks[i]);
	   }
	   else
	   {
	   	 int i;
	   	 for(i = 0; i < NDIRECT; i++)
		 	if(tmp->blocks[i] != 0)
  	 			bm->free_block(tmp->blocks[i]);
  	 			
  	 	 char *buf_ind = (char*)malloc(BLOCK_SIZE);
  	 	 bm->read_block(tmp->blocks[NDIRECT],buf_ind);
   	 	 for(i = 0; i < ori_bnum-NDIRECT; i++)
   	 	 	 if(*(uint32_t*)(buf_ind+sizeof(uint32_t)*i) != 0)
   	 	 		 bm->free_block(*(uint32_t*)(buf_ind+sizeof(uint32_t)*i));
   	 	 bm->free_block(tmp->blocks[NDIRECT]);
  	     free(buf_ind);
	   }
   }
   free(tmp);
   free_inode(inum);
}
