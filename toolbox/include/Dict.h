// $Id: Dict.h,v 1.2 2004/05/24 16:37:50 plg Exp $
//==========================================================
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

#ifndef __DICT_H
#define __DICT_H

#include "Containers.h"

enum dict_dup {
	REPLACE_DUPLICATES = 0,
	IGNORE_DUPLICATES,
	ALLOW_DUPLICATES,
};

typedef int (*cmp_t)(const void *, const void *);
typedef void *(*cp_key_t)(const void *key);

cp_key_t cp_str(void *n);



struct dict_extra { //FIXME: remove those
	//	enum tree_type;
	//	union internal {
	void *BplusTree;
	//		struct avltree * avl_Tree;
	//	}
};
typedef struct dict_extra *dict_extra_t;

// Constructors

Dict_t tb_dict_new         (int KT, int allow_dupes);
Dict_t tb_dict_new_default ();


#endif
