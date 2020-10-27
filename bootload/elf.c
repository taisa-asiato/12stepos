#include "defines.h"
#include "elf.h"
#include "lib.h"

/* ELFヘッダ解析のための構造体 */
struct elf_header {
	struct {
		unsigned char magic[4]; // マジックナンバー
		unsigned char class; // 32bit or 64bit
		unsigned char format; // エンディアン情報
		unsigned char version; // ELFフォーマットのバージョン
		unsigned char abi; // OSの種別
		unsigned char abi_version; // OSのバージョン
		unsigned char reserve[7]; // 予約
	} id;

	short type; // ファイルの種類
	short arch; // CPUの種類
	long version; // ELF形式のバージョン
	long entry_point; // 実行開始アドレス
	long program_header_offset; // プログラムヘッダテーブルの位置
	long section_header_offset; // セクションヘッダテーブルの位置
	long flags; // フラグの値
	short header_size; // ELFヘッダのサイズ
	short program_header_size; // プログラムヘッダのサイズ
	short program_header_num; // プログラムヘッダの個数
	short section_header_size; // セクションヘッダのサイズ
	short section_header_num; // セクションヘッダの個数
	short section_name_index; // セクション名を格納するセクション
};

/* プログラムヘッダ解析のための構造体 */
struct elf_program_header {
	long type; // セグメントの種別
	long offset; // ファイル中の位置
	long virtual_addr; // 論理アドレス( VA )
	long physical_addr; // 物理アドレス( PA )
	long file_size; // ファイルサイズ
	long memory_size; // メモリ上のでサイズ
	long flags; // 各種フラグ
	long align; // アライメント
};

/* ELFヘッダのチェック */
static int elf_check(struct elf_header * header) {
	// マジックナンバをチェックする
	if (memcmp(header->id.magic, "\x7f" "ELF", 4))
		return -1;

	// 以降マジックナンバがELF形式のものだった場合の処理
	if (header->id.class	!= 1) return -1; /* ELF32? */
	if (header->id.format	!= 2) return -1; /* Big Endian? */
	if (header->id.version	!= 1) return -1; /* Version 1?*/
	if (header->type	!= 2) return -1; /* 実行可能ファイル? */
	if (header->version	!= 1) return -1; /* ELF形式バージョン1? */

	/* 該当マイコンのアーキテクチャ */
	if ((header->arch != 46) && (header->arch != 47)) return -1;

	return 0;
}

/* セグメント単位でロードを行う */
static int elf_load_program(struct elf_header * header) {
	int i;
	struct elf_program_header * phdr;

	// プログラムヘッダの分だけ読み込む
	for (i = 0; i < header->program_header_num; i++) {
		phdr = (struct elf_program_header *)
			((char *)header + header->program_header_offset + 
			 header->program_header_size * i);

		if (phdr->type != 1) // ロード可能セグメントか確認
			continue; 

		memcpy((char *)phdr->physical_addr, (char *)header + phdr->offset, phdr->file_size);
		// コピーしたサイズに応じ, phdr->memory_sizeに満たないデータ分は0fillする
		memset((char *)phdr->physical_addr + phdr->file_size, 0, phdr->memory_size - phdr->file_size);

	}
	return 0;
}

char * elf_load(char * buf) {
	struct elf_header * header = (struct elf_header *)buf;

	if (elf_check(header) < 0) /* ELFヘッダのチェック */
		return NULL;

	if (elf_load_program(header) < 0) /* セグメント単位でロード */
		return NULL;

	return (char * )header->entry_point;
}
