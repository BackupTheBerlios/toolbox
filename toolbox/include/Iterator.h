// $Id: Iterator.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//===========================================================
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
#ifndef TB_ITERATOR_H
#define TB_ITERATOR_H

#include "Toolbox.h"
#include "iterators.h"

extern int OM_NEW_ITERATOR_CTX;
extern int OM_FREE_ITERATOR_CTX;
extern int OM_GET_ITERATOR_CTX;
extern int OM_GONEXT;
extern int OM_GOPREV;
extern int OM_GOFIRST;
extern int OM_GOLAST;
extern int OM_CURKEY;
extern int OM_CURVAL;


_iterator_ctx_t _Iterator_getIterCtx     (Iterator_t It);
retcode_t       _Iterator_goFirst        (Iterator_t It);
retcode_t       _Iterator_goLast         (Iterator_t It);
tb_Key_t        _Iterator_goPrev         (Iterator_t It);
tb_Key_t        _Iterator_goNext         (Iterator_t It);
tb_Key_t        _Iterator_curKey         (Iterator_t It);
tb_Object_t     _Iterator_curVal         (Iterator_t It);

void         *  tb_iterator_free        (Iterator_t C);
int             tb_iterator_getsize     (Iterator_t C);

struct iterator_members {
	_iterator_ctx_t Ctx;
	tb_Object_t target;
	int size;
};
typedef struct iterator_members *iterator_members_t;

inline iterator_members_t XIterator(Iterator_t);



#endif
