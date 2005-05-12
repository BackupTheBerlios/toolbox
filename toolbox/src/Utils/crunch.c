//==================================================
// $Id: crunch.c,v 1.1 2005/05/12 21:53:01 plg Exp $
//==================================================
/* Copyright (c) 1999-2005, Paul L. Gatille <paul.gatille@free.fr>
 *
 * This file is part of Toolbox, an object-oriented utility library
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the "Artistic License" which comes with this Kit.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the Artistic License for more
 * details.
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "crc32.h"
#include "zlib.h"

#include "Toolbox.h"
#include "Memory.h"

static void zerr(int ret);

struct zhead {
	unsigned char  id1;
	unsigned char  id2;
	unsigned char  cm;
	unsigned char  flg;
	unsigned int   mtime;
	unsigned char  xfl;
	unsigned char  os;
	//	unsigned char  xlen;
};

struct ztail {
	unsigned int crc32;
	unsigned int isize;
};

#define CHUNK 16384 * 4 

/*
gzip header : (RFC 1952)
char ID1: 0x1f : gzip format (1/2)
char ID2: 0x8b : gzip format (2/2)

char CM:  0x8 : deflate

char FLG: 
 BIT0 : FTEXT (-> text file)
 BIT1 : FCRC  (-> CRC16 present)
 BIT2 : FEXTRA
 BIT3 : FNAME (->file name present)
 BIT4 : FCOMMENT
 BIT5-7 : RFU

int MTIME:
char XFL:  
  0x2 (=slow/max compress)
  0x4 (=fast/less compress)
char OS:
  0 - FAT filesystem (MS-DOS, OS/2, NT/Win32)
  1 - Amiga
  2 - VMS (or OpenVMS)
  3 - Unix
  4 - VM/CMS
  5 - Atari TOS
  6 - HPFS filesystem (OS/2, NT)
  7 - Macintosh
  8 - Z-System
  9 - CP/M
 10 - TOPS-20
 11 - NTFS filesystem (NT)
 12 - QDOS
 13 - Acorn RISCOS
255 - unknown
short XLEN: 0

[data]...

CRC32: 
ISIZE: uncompressed file size
*/



int tb_Crunch(String_t Source, String_t Dest) {

	if(! TB_VALID(Dest, TB_STRING) || tb_getSize(Source) == 0) return TB_KO;
	
	int ret, flush;
	unsigned have;
	z_stream strm;
	char out[CHUNK];
	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit2(&strm, -1, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);

	char *in;
	char *in_start = tb_toStr(Source);
	int in_step = 0;
	int in_size = tb_getSize(Source);
	int in_remaining = in_size;
	struct zhead zh;
	struct ztail zt;

	zh.id1   = 0x1f;
	zh.id2   = 0x8b;
	zh.cm    = 0x08;
	zh.flg   = 0x00;
	zh.mtime = time(NULL);
	zh.xfl   = 0x00;
	zh.os    = 0x03;

	zt.crc32 = tb_crc32((const unsigned char *)in_start, in_size);
	zt.isize = in_size;

	tb_Clear(Dest);
	tb_RawAdd(Dest, 10, 0, (void *)&zh);

	if (ret != Z_OK) {
		zerr(ret);
		return ret;
	}

	/* compress until end of file */
	do {

		if(in_remaining >= CHUNK) {
			strm.avail_in = CHUNK;
		} else {
			strm.avail_in = in_remaining;
		}
		in_remaining -= strm.avail_in;
		in = in_start+(in_step*CHUNK);
		in_step++;

		flush = (in_remaining <=0) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = in;

		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);    
			assert(ret != Z_STREAM_ERROR); 
			have = CHUNK - strm.avail_out;
			tb_RawAdd(Dest, have, -1, out);
		} while (strm.avail_out == 0);
	} while (flush != Z_FINISH);
	(void)deflateEnd(&strm);

	tb_RawAdd(Dest, sizeof(struct ztail), -1, (void *)&zt);

	return TB_OK;
}



int tb_Decrunch(String_t Source, String_t Dest) {
	//	if(! TB_VALID(Dest, TB_STRING) || tb_getSize(Source) == 0) return TB_KO;

	int ret;
	unsigned have;
	z_stream strm;
	char out[CHUNK];

	char *in;
	char *in_start = tb_toStr(Source);
	int in_step = 0;
	int in_size = tb_getSize(Source);
	int in_remaining = in_size;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	//	ret = inflateInit(&strm);
	ret = inflateInit2(&strm, 31);
	if (ret != Z_OK) {
		zerr(ret);
		return ret;
	}

	do {

		if(in_remaining >= CHUNK) {
			strm.avail_in = CHUNK;
		} else {
			strm.avail_in = in_remaining;
		}
		in_remaining -= strm.avail_in;
		in = in_start+(in_step*CHUNK);
		in_step++;

		strm.next_in = in;

		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;     /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				zerr(ret);
				return ret;
			}
			have = CHUNK - strm.avail_out;
			tb_RawAdd(Dest, have, -1, out);
		} while (strm.avail_out == 0);

	}	while(in_remaining >0);

	(void)inflateEnd(&strm);

	return (ret == Z_STREAM_END) ? TB_OK : TB_KO;
}


static void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
				break;
		default:
			fputs("unknown zlib error!\n", stderr);
    }
}
