// $Id: tb_Errno.h,v 1.1 2004/05/12 22:04:49 plg Exp $
//====================================================================
/* Copyright (c) 1999-2004, Paul L. Gatille <paul.gatille@free.fr>
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

#ifndef TB_ERRNO_H
#define TB_ERRNO_H

int get_tb_errno(void);

#define tb_errno (get_tb_errno())

#define TB_ERR_BAD                   10
#define TB_ERR_IN_USE                11
#define TB_ERR_UNSET                 12

#define TB_ERR_OBJECT_IS_NULL       100
#define TB_ERR_INVALID_TB_OBJECT    101
#define TB_ERR_NO_SUCH_METHOD       102


#define TB_ERR_EMPTY_VALUE          300
#define TB_ERR_INVALID_HASH_FNC     301
#define TB_ERR_USER_SEARCH_ERROR    302
#define TB_ERR_OUT_OF_BOUNDS        303
#define TB_ERR_ALLREADY             304

#define TB_ERR_INVALID_PROTOCOL     400
#define TB_ERR_SOCKET_FAILED        401
#define TB_ERR_CONNECT_FAILED       402
#define TB_ERR_INVALID_HOSTNAME     403
#define TB_ERR_BIND_FAILED          404
#define TB_ERR_LISTEN_FAILED        405
#define TB_ERR_SELECT_FAILED        406
#define TB_ERR_FCNTL_FAILED         407
#define TB_ERR_DISCONNECTED         408
#define TB_ERR_TIMEDOUT             409
#define TB_ERR_INVALID_HOST         410

#define TB_ERR_SQL_NO_FREE_HANDLES  500
#define TB_ERR_SQL_BAD_SPOOL        501


#endif
