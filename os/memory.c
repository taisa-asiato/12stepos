#include "defines.h"
#include "tsos.h"
#include "lib.h"
#include "memory.h"

/* メモリブロック構造体 */
// メモリヘッダ
typedef struct _tsmem_block {
	struct _tsmem_block * next;	/* 次のメモリへのリンク */
	int size; 			/* メモリサイズ*/
} tsmem_block;

/* メモリプール構造体 */
typedef struct _tsmem_pool {
	int size;
	int num;
	tsmem_block * free; // 解放済み領域のリンクリストへのポインタ 
} tsmem_pool;

/* メモリプールの定義 */
static tsmem_pool pool[] = {
	{16, 8, NULL}, {32, 8, NULL}, {64, 4, NULL},
};

#define MEMORY_AREA_NUM (sizeof(pool)/sizeof(*pool))

/* メモリプール初期化 */
static int tsmem_init_pool(tsmem_pool * p) {
	int i;
	tsmem_block * mp;
	tsmem_block ** mpp;
	extern char freearea; // 領域はリンカスクリプトで定義
	static char * area = &freearea;

	mp = (tsmem_block *)area;

	/* 個々の領域をすべて解放済みリンクリストに繋ぐ */
	mpp = &p->free;
	for (i = 0; i < p->num; i++) {
		*mpp = mp;
		memset(mp, 0, sizeof(*mp));
		mp->size = p->size;
		mpp = &(mp->next);
		mp = (tsmem_block *)((char *)mp + p->size);
		area += p->size;
	}
	return 0;
}

/* 動的メモリの初期化 */
int tsmem_init(void) {
	int i;
	for (i = 0; i < MEMORY_AREA_NUM; i++) {
		tsmem_init_pool(&pool[i]);	// 各メモリプールを初期化する
	}
	return 0;
}

/* 動的メモリの確保 */
void * tsmem_alloc(int size) {
	int i;
	tsmem_block * mp;
	tsmem_pool * p;

	for (i = 0; i < MEMORY_AREA_NUM; i++) {
		p = &pool[i];

		if (size <= p->size - sizeof(tsmem_block)) {
			if (p->free == NULL) {	// 解放済み領域がない場合
				ts_sysdown();
				return NULL;
			}
			/* 解放済みリンクリストからメモリ領域を取得する */
			mp = p->free;
			p->free = p->free->next;
			mp->next = NULL;

			/* 実際に利用可能なメモリ領域は+1したアドレスからスタートする */
			return mp+1;
		}

	}
	ts_sysdown();
	return NULL;
}

/* メモリの解放 */
void tsmem_free(void *mem) {
	int i;
	tsmem_block * mp;
	tsmem_pool * p;

	mp =((tsmem_block *)mem - 1);	// 領域の直前にあるメモリブロック構造体にアクセスする 

	for (i = 0; i < MEMORY_AREA_NUM; i++) {
		p = &pool[i];
		if (mp->size == p->size) {	// 同一サイズのメモリプールを検索する
			mp->next = p->free;
			p->free = mp;
			return;
		}
	}
	ts_sysdown();
}
