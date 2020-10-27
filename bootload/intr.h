#ifndef _INTR_H_INCLUDED_
#define _INTR_H_INCLUDED_

/* ソフトウェア・割込みベクタの定義 */
#define SOFTVEC_TYPE_NUM	3 // ソフトウェア割込みベクタの種類

#define SOFTVEC_TYPE_SOFTERR	0 // ソフトウェアエラー
#define SOFTVEC_TYPE_SYSCALL	1 // システムコール割込み
#define SOFTVEC_TYPE_SERINTR	2 // シリアル割込み

#endif 
