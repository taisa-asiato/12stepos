#ifndef _TSOS_MEMORY_INCLUDED_
#define _TSOS_MEMORY_INCLUDED_

int tsmem_init(void);		/* 動的メモリの初期化 */
void * tsmem_alloc(int size);	/* 動的メモリの獲得 */
void tsmem_free(void *mem);	/* メモリの解放 */

#endif
