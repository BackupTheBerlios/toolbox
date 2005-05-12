//==================================================
// $Id: futil.c,v 1.1 2005/05/12 21:53:01 plg Exp $
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Toolbox.h"
#include "Memory.h"

String_t tb_loadFile(char *file) {
	int rc=1, fd;
	String_t S = NULL;
	char buff[4096];

	if((fd = open(file, O_RDONLY)) != -1) {
		S = tb_String(NULL);
		while( rc > 0) {
			rc = read(fd, buff, 4095);
			if(rc >0 ) {
				buff[rc] = 0;
				tb_RawAdd(S, rc, -1, buff);
			}
		}
		close(fd);
	} else {
		tb_error("loadFile[%s]: %s\n", file, strerror_r(errno, buff, 4095));
	}
	return S;
}

retcode_t tb_saveFile(char *file, String_t data) {
	FILE *fh;
	char buff[128];
	int rc = TB_KO;


	if((fh = fopen(file, "w")) != NULL) {
		if(fwrite(tb_toStr(data), tb_getSize(data), 1, fh) == 1) {
			rc = TB_OK;
		} else {
			tb_error("saveFile[%s]: %s\n", file, strerror_r(errno, buff, 128));
		}
		fclose(fh);
	} else {
		tb_error("saveFile[%s]: %s\n", file, strerror_r(errno, buff, 128));
	}
	return rc;
}

retcode_t tb_fileExists(char *file) {
	FILE *fh;
	int rc = TB_KO;

	if((fh = fopen(file, "r")) != NULL) {
		rc = TB_OK;
		fclose(fh);
	} 
	return rc;
}
