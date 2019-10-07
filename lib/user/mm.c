#include <memlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// 代码来自 csapp

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE 512

#define MAX(x, y)((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)p)
#define PUT(p, val) (*(unsigned int*)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE((HDRP(bp)) - DSIZE))

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *headp_listp = NULL;


/**
 * 合并块
 */
static void *coalease(void *bp){

	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if(prev_alloc && next_alloc){
		return bp;
	}

	else if(prev_alloc && !next_alloc){
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		//这里顺序很重要
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}

	else if(!prev_alloc && next_alloc){
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	else {
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + \
			GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	return bp;
}

static void *extend_heap(size_t words){

	char *bp;
	size_t size;

	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if((long)(bp = mem_sbrk(size)) == -1){
		return NULL;
	}

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

	return coalease(bp);
}

/**
 * first fit
 */
void *find_fit(size_t size){

	char *bp = headp_listp;
	size_t fit_size;
	
	while(1){
		fit_size = GET_SIZE(HDRP(bp));
		//printf("bp %x, fit_size %d---", bp, fit_size);
		if(!GET_ALLOC(HDRP(bp)) && fit_size >= size){
			return bp;
		}else if(fit_size == 0){
			return NULL;
		}
		bp = NEXT_BLKP(bp);
	}
	return NULL;
}

void place(void *bp, size_t asize){

	size_t fit_size = GET_SIZE(HDRP(bp));
	size_t extsize = fit_size - asize;

	if(extsize >= DSIZE * 2){
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		PUT(HDRP(NEXT_BLKP(bp)), PACK(extsize, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(extsize, 0));
	}
}


void mm_free(void *bp){

	size_t size = GET_SIZE(HDRP(bp));

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));

	coalease(bp);
}

void *mm_alloc(size_t size){

	size_t asize;
	size_t extsize;
	char *bp;

	if(size == 0){
		return NULL;
	}
	if(size <= DSIZE){
		asize = DSIZE;
	}else {
		asize = DSIZE + ((size & 0x7) ? 
			(size & ~0x7) + DSIZE : size);
	}
	//printf("asize : %d\n", asize);
	if((bp = find_fit(asize)) != NULL){
		place(bp, asize);
		return bp;
	}

	extsize = MAX(asize, CHUNKSIZE);
	if((bp = extend_heap(extsize / WSIZE)) == NULL){
		return NULL;
	}
	place(bp, asize);
	return bp;
}

void *mm_realloc(void *bp, size_t size){
	
	if(bp == NULL){
		return mm_alloc(size);
	}

	if(size == 0){
		free(bp);
		return NULL;
	}

	size_t oldsz = GET_SIZE((HDRP(bp)));

	//扩大
	if(oldsz < size){

		char *new_p = mm_alloc(size);
	       	if(new_p == NULL){
			return NULL;
		}	

		//复制旧数据过去
		memcpy(new_p, bp, oldsz);
		mm_free(bp);
		return new_p;

	//缩小
	}else if(oldsz > size){

		place(bp, size);
		return bp;
	}

	return bp;
}

void *mm_calloc(size_t lens, size_t size){

	return mm_alloc(lens * size);
}

void mm_print();

int mm_init(void){

	//TODO 这里判断有问题
	if((headp_listp = mem_sbrk(4 * WSIZE)) == (void *)-1){
		printf("mem_sbrk error\n");
		return -1;
	}

	PUT(headp_listp, 0);
	PUT(headp_listp + (1 * WSIZE), PACK(DSIZE, 1));
	PUT(headp_listp + (2 * WSIZE), PACK(DSIZE, 1));
	PUT(headp_listp + (3 * WSIZE), PACK(0, 1));
	headp_listp += (2 * WSIZE);

	if(extend_heap(CHUNKSIZE / WSIZE) == NULL){
		printf("extend_heap error\n");
		return -1;
	}
	mm_print();
	return 0;
}

void mm_print(){

	char *show[] = {"free", "alloc"};
	char *bp = headp_listp;
	size_t size;
	int idx = 0;
	int status;
	
	while(1){
		size = GET_SIZE(HDRP(bp));
		status = GET_ALLOC(HDRP(bp));
		if(!size){
			break;
		}
		if(idx)
			printf("block : %d size : %d status : %s\t", idx, size, show[status]);
		bp = NEXT_BLKP(bp);
		idx++;
	}
	printf("\n");
}	

