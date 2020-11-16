#ifndef _LIB_H_INCLUDED_
#define _LIB_H_INCLUDED_

void *memset(void *b, int c, long len); /* メモリを特定のバイトで埋める */
void *memcpy(void *dst, const void *src, long len); /* メモリのコピー */
int memcmp(const void *b1, const void *b2, long len); /* メモリの比較を行う */
int strlen(const char *s); /* 文字列の長さを返す */
char *strcpy(char *dst, const char *src); /* 文字列のコピーを行う */
int strcmp(const char *s1, const char *s2); /* 文字列の比較 */
int strncmp(const char *s1, const char *s2, int len);

int putc(unsigned char c);	/* 1文字送信 */
unsigned char getc(void);	/* 1文字受信 */
int puts(unsigned char * str);	/* 文字列送信 */
int gets(unsigned char * buf);	/* 文字列受信 */
int putxval(unsigned long value, int column);
// int func(int a, int b); /* 実験用 */

#endif
