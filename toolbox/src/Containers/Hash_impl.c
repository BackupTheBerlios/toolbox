//=======================================================
// $Id: Hash_impl.c,v 1.4 2004/06/15 15:08:27 plg Exp $
//=======================================================
/* Copyright (c) 1999-2004, Paul L. Gatille. All rights reserved.
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
 *
 */



#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "tb_global.h"
#include "Toolbox.h"
#include "Hash.h"

#include "tb_ClassBuilder.h"
#include "Serialisable_interface.h"

#include "Memory.h"
#include "Error.h"




inline hash_extra_t  XHASH(Hash_t H) {
	return (hash_extra_t)((__members_t)tb_getMembers(H, TB_HASH))->instance;
}

// from R.E. str library (see copyright below)
static unsigned long hash_djbx33  (register unsigned char *key, register size_t len);

/*CLEAN-ME: not used anymore.
static unsigned long hash_bjddj   (register unsigned char *key, register size_t len);
static unsigned long hash_macrc32 (register unsigned char *key, register size_t len);
// from gtk's glib library 
static unsigned long hash_glib    (register unsigned char *key, register size_t len);
*/

static void        * tb_hash_free    (tb_Object_t O);
static tb_Object_t   tb_hash_clear   (tb_Object_t O);
static int           tb_hash_getsize (tb_Object_t O);
static void          tb_hash_dump    (tb_Object_t O, int level);
static tb_Object_t   tb_hash_get     (Hash_t H, tb_Key_t key);
static retcode_t     tb_hash_replace (Hash_t H, tb_Object_t value, tb_Key_t key);
static retcode_t     tb_hash_insert  (Hash_t H, tb_Object_t value, tb_Key_t key);
static retcode_t     tb_hash_remove  (Hash_t H, tb_Key_t key);
static tb_Object_t   tb_hash_take    (Hash_t H, tb_Key_t key);
static int           tb_hash_exists  (Hash_t H, tb_Key_t key);
static Hash_t        tb_hash_clone   (Hash_t H);
static void          tb_hash_marshall(String_t marshalled, Hash_t O, int level);
static Hash_t        tb_hash_unmarshall(XmlElt_t xml_element);

static void          tb_node_dump    (tb_hash_node_t O, int level, int kt);
static tb_hash_node_t tb_node_new    (tb_Key_t key, tb_Object_t value, int kt, int hasDups);
static void          tb_node_free    (tb_hash_node_t N, int kt);

//static char        * tb_hash_stringify(Hash_t O);
static String_t      tb_hash_stringify(Hash_t O);

void __build_hash_once(int OID) {
	tb_registerMethod(OID, OM_NEW,                    tb_hash_new_default);
	tb_registerMethod(OID, OM_FREE,                   tb_hash_free);
	tb_registerMethod(OID, OM_GETSIZE,                tb_hash_getsize);
	tb_registerMethod(OID, OM_CLONE,                  tb_hash_clone);
	tb_registerMethod(OID, OM_DUMP,                   tb_hash_dump);
	tb_registerMethod(OID, OM_CLEAR,                  tb_hash_clear);
	tb_registerMethod(OID, OM_REPLACE,                tb_hash_replace);
	tb_registerMethod(OID, OM_INSERT,                 tb_hash_insert);
	tb_registerMethod(OID, OM_EXISTS,                 tb_hash_exists);
	tb_registerMethod(OID, OM_TAKE,                   tb_hash_take);
	tb_registerMethod(OID, OM_GET,                    tb_hash_get);
	tb_registerMethod(OID, OM_REMOVE,                 tb_hash_remove);

	tb_registerMethod(OID, OM_STRINGIFY,              tb_hash_stringify);

	// interface Iterable already defined in Container (inherited)
	// -> no need to call implementsIterface. 
	// now register interface's methods
	register_Hash_Iterable_once(OID);

	tb_implementsInterface(OID, "Serialisable", 
												 &__serialisable_build_once, build_serialisable_once);

	tb_registerMethod(OID, OM_MARSHALL,     tb_hash_marshall);
	tb_registerMethod(OID, OM_UNMARSHALL,   tb_hash_unmarshall);
}


static const ulong _nprimes = sizeof (_primes) / sizeof (_primes[0]);

static ulong        _spaced_primes_closest      (ulong num);
static void         tb_hash_resize              (Hash_t H);


static ulong _spaced_primes_closest(ulong num) {
  ulong i;

  for(i = 0; i < _nprimes; i++) {
    if(_primes[i] > num) return _primes[i];
	}

  return _primes[_nprimes - 1];
}

inline static unsigned long __hash_me(Hash_t H, tb_Key_t K) {
	unsigned long ul = 0;
	hash_extra_t members = XHASH(H);
	void *p = members->hash_fnc;

	switch(members->kt) {
	case KT_STRING:
	case KT_STRING_I:
		ul = ((unsigned long (*)(register unsigned char *, register size_t))p)(K.key, strlen(K.key));
		break;
	case KT_INT:
		ul = ((unsigned long (*)(int))p)(K.ndx);
		break;
	case KT_POINTER:
		ul = ((unsigned long (*)(void *))p)(K.user);
	}
	return ul;
}
	




inline static unsigned long hash_int(int val) { return (unsigned long)val; }
inline static unsigned long hash_ptr(void *val) { return (unsigned long)val; }

static int tb_hash_getsize(Hash_t H) {
	return XHASH(H)->size;
}


Hash_t tb_hash_new_default() {
	return  tb_hash_new(KT_STRING, 0);
}

Hash_t tb_hash_new(int key_type, int allow_duplicates) {
	tb_Object_t This;
  hash_extra_t members;

	pthread_once(&__class_registry_init_once, tb_classRegisterInit);

	This = tb_newParent(TB_HASH);
	This->isA   = TB_HASH;

	members = (hash_extra_t)tb_xcalloc(1, sizeof(struct hash_extra));
  This->members->instance = members;

	members->buckets  = HASH_MIN_SIZE;
	members->kt    = key_type;
	members->allow_duplicates = allow_duplicates;

	switch(key_type) {
	case KT_STRING:
	case KT_STRING_I:
		members->hash_fnc = hash_djbx33; // default hash fnc
		break;
	case KT_INT:
		members->hash_fnc = hash_int;  // default int's hash fnc
		break;
	case KT_POINTER:
		members->hash_fnc = hash_ptr;
		break;
	}

	members->nodes = tb_xcalloc((size_t)members->buckets, sizeof(struct tb_hash_node));
	if(fm->dbg) fm_addObject(This);

	return This;
}





static tb_hash_node_t tb_node_new(tb_Key_t key, tb_Object_t value, int kt, int hasDups) {
  tb_hash_node_t N = tb_xcalloc(1, sizeof(struct tb_hash_node));
  N->key   = kt_getKcp(kt)(key);
	if(hasDups) {
		N->values = tb_xcalloc(1, sizeof(tb_Object_t));
		N->nb = 1;
		N->values[0] = value;
	} else {
		N->value = value;
	}

  return N;
}

// destructors

static void tb_node_free(tb_hash_node_t N, int kt) {
  kt_getKfree(kt)(N->key);
	if(N->nb >0) {
		int i;
		for(i=0; i<N->nb; i++) {
			TB_UNDOCK(N->values[i]);
			tb_Free(N->values[i]);
		}
		tb_xfree(N->values);
	} else {
		TB_UNDOCK(N->value);
		tb_Free(N->value);
	}
  tb_xfree(N);
}

static void *tb_hash_free(Hash_t H) {
  int i;
  hash_extra_t members = XHASH(H);

	fm_fastfree_on();
  for (i = 0; i < members->buckets; i++) {
    tb_hash_node_t node, next;
    for (node = ((tb_hash_node_t *)(members->nodes))[i]; node; node = next) {
      next = node->next;
      tb_node_free(node, XHASH(H)->kt);
    }
  }

	tb_freeMembers(H);
	fm_fastfree_off();
	H->isA = TB_HASH;
	return tb_getParentMethod(H, OM_FREE);
}


// fixme: tb_Add ?? tb_Insert?
static Hash_t tb_hash_clone(Hash_t O) {
  int           i;
  hash_extra_t  members    = XHASH(O);
	Hash_t        clone      = tb_HashX(members->kt, members->allow_duplicates);
	no_error;

  for (i = 0; i < members->buckets; i++) {
    tb_hash_node_t node, next;
    for (node = ((tb_hash_node_t *)(members->nodes))[i]; node; node = next) {
      next = node->next;
			if(node->nb >0) {
				int i;
				for(i=0; i<node->nb; i++) {
					tb_Replace(clone, tb_Clone(node->values[i]), node->key);
				}
			} else {
				tb_Replace(clone, tb_Clone(node->value), node->key);
			}
    }
  }

	return clone;
}



static tb_Object_t tb_hash_clear(tb_Object_t O) {
  int i;
  hash_extra_t members = XHASH(O);
	no_error;
	if(!tb_getSize(O)) return O;

	fm_fastfree_on();
  for (i = 0; i < members->buckets; i++) {
    tb_hash_node_t node, next;
    for (node = members->nodes[i]; node; node = next) {
      next = node->next;
      tb_node_free(node, members->kt);
    }
  }
  tb_xfree(members->nodes);
	fm_fastfree_off();

	members->buckets = HASH_MIN_SIZE;

  members->nodes = tb_xcalloc((size_t)members->buckets, sizeof(tb_hash_node_t));

	return O;
}



static void tb_hash_dump(Hash_t O, int level) {
  int             i;
  hash_extra_t    members = XHASH(O);
	char          * dupl[]  = {"No", "Yes"};

	no_error;
  for(i = 0; i<level; i++) fprintf(stderr, " ");

	if(members->size == 0) {

		fprintf(stderr, "<TB_HASH size=\"%d\" type=\"%s\" duplicates=\"%s\" hashslots=\"%d\" addr=\"%p\" refcnt=\"%d\" docked=\"%d\" />\n",
						members->size, kt_getKname(members->kt), dupl[members->allow_duplicates], 
						members->buckets, O, O->refcnt, O->docked);
	} else {

		fprintf(stderr, "<TB_HASH size=\"%d\" type=\"%s\" duplicates=\"%s\" hashslots=\"%d\" addr=\"%p\" refcnt=\"%d\" docked=\"%d\">\n",
						members->size, kt_getKname(members->kt), dupl[members->allow_duplicates],
						members->buckets, O, O->refcnt, O->docked);

		for (i = 0; i < members->buckets; i++) {
			tb_hash_node_t node;
			for(node = members->nodes[i]; node != NULL;) {
				if(node) {
					tb_node_dump(node ,level+1, members->kt); 
				}
				node = node->next;
			}
		}
		for(i = 0; i<level; i++) fprintf(stderr, " ");
		fprintf(stderr, "</TB_HASH>\n");

	}
}


static void tb_node_dump(tb_hash_node_t N, int level, int kt) {
  int        i;
	k2sz_t     k2sz       = kt_getK2sz(kt);
	char       buff[20]; // fixme: may overflow
	no_error;
  for(i = 0; i<level; i++) fprintf(stderr, " ");
	if(N->nb >0) {
		fprintf(stderr, "<NODE addr=\"%p\" duplicates=\"%d\" key=\"%s\" >\n",
						N, N->nb,  k2sz(N->key, buff));
	} else {
		fprintf(stderr, "<NODE addr=\"%p\" key=\"%s\" value=\"%s\" >\n",
						N,  k2sz(N->key, buff), tb_nameOf(tb_isA(N->value)));
	}

	if(N->nb >0) {
		int i;
		for(i=0; i<N->nb; i++) {
			void *p = tb_getMethod(N->values[i], OM_DUMP);
			if(p) ((void(*)(tb_Object_t,int))p)(N->values[i] ,level+2);
		}
	} else {
		void *p = tb_getMethod(N->value, OM_DUMP);
		if(p) ((void(*)(tb_Object_t,int))p)(N->value ,level+2);
  } 
  for(i = 0; i<level; i++) fprintf(stderr, " ");
	fprintf(stderr, "</NODE>\n");
}



static void tb_hash_resize (Hash_t H) {
  tb_hash_node_t        * new_nodes;
  tb_hash_node_t          node, next, prev;
  float                   nodes_per_list;
  ulong                   hash_val;
  int                     new_size;
  int                     i;
  hash_extra_t            members = XHASH(H);

  nodes_per_list = (float) members->size / (float) members->buckets;

  if ((nodes_per_list > 0.3 || members->buckets <= HASH_MIN_SIZE) &&
      (nodes_per_list < 3.0 || members->buckets >= HASH_MAX_SIZE))
    return;

  new_size = TB_CLAMP(_spaced_primes_closest(members->size),
                   HASH_MIN_SIZE, HASH_MAX_SIZE);

	tb_info("hash_resize needed (%d nodes, old size=%d, new size=%d\n",
					members->size, members->buckets, new_size);

  new_nodes = (tb_hash_node_t *)tb_xcalloc(new_size, sizeof(struct tb_hash_node));

  for (i = 0; i < members->buckets; i++) {
		prev = NULL;
    for (node = members->nodes[i]; node; node = next) {
      next = node->next;
      hash_val = __hash_me(H, node->key) % new_size;
			if(new_nodes[hash_val]) (new_nodes[hash_val])->prev = node;

      node->next = new_nodes[hash_val];
			node->prev = NULL;
			prev       = node;
      new_nodes[hash_val] = node;
    }
  }
  tb_xfree(members->nodes);
  members->nodes = new_nodes;
  members->buckets = new_size;
}                       

tb_hash_node_t *tb_hash_lookup_node(Hash_t H, tb_Key_t the_key) {
	char            * key;
  tb_hash_node_t  * node    = NULL;
  hash_extra_t      members = XHASH(H);

	no_error;

	if(members->kt == KT_STRING_I) { // upcase all
		char *s, *k;
		key = k = tb_xstrdup(the_key.key);
		
		for(s = key; *s != 0; s++) {
			*s = toupper((unsigned char)*(k++));
		}
		node = & members->nodes[ __hash_me(H, s2K(key)) % members->buckets];
		while ((*node) && ! (strcasecmp((*node)->key.key, key) == 0))	{
			node = & (*node)->next;
		}
		tb_xfree(key);
	} else {
		kcmp_t kcmp = kt_getKcmp(members->kt);
		node = & members->nodes[ __hash_me(H, the_key) % members->buckets];
		while ((*node) && ! (kcmp((*node)->key, the_key) == 0))	{
			node = & (*node)->next;
		}
	}

  return node;
}



static tb_Object_t tb_hash_get(Hash_t H, tb_Key_t key) {
  tb_hash_node_t   * node;
	tb_Object_t        O = NULL;

	no_error;

  if((node = tb_hash_lookup_node(H, key))) {
		if(*node) O = ((*node)->nb >0) ? (*node)->values[0] : (*node)->value;

	} else {
		set_tb_errno(TB_ERR_EMPTY_VALUE);
	}

  return O;
}




static int tb_hash_exists(Hash_t H, tb_Key_t key) { // fixme: dupes !
	if(XHASH(H)->allow_duplicates) {
		tb_hash_node_t *node = tb_hash_lookup_node(H, key);
		return ((*node) != NULL) ? (*node)->nb : 0 ;
	} 
	return (tb_hash_get(H, key)) ? 1 : 0;
}


static retcode_t tb_hash_replace(Hash_t H, tb_Object_t value, tb_Key_t key) {
  tb_hash_node_t     * node;
	tb_hash_node_t       prev    = NULL;
	tb_hash_node_t       next    = NULL;
  hash_extra_t         members = XHASH(H);

	node = tb_hash_lookup_node(H, key);

  if(*node) {     // overwrites value for this key
		char buff[20]; // fixme: may overflow
		tb_debug("tb_Hash::Replace: key <%s> overwritten\n", kt_getK2sz(members->kt)(key, buff));

		if((*node)->nb >0) {
			int i;
			for(i=0; i<(*node)->nb; i++) {
				TB_UNDOCK((*node)->values[i]);
				tb_Free((*node)->values[i]);
				
			}
			(*node)->nb = 1;
			(*node)->values[0] = value;
			TB_DOCK(value);
		} else {
			TB_UNDOCK((*node)->value);
			tb_Free((*node)->value);
			(*node)->value = value;
			TB_DOCK(value);
		}

  } else {
    *node = tb_node_new(key, value, members->kt, members->allow_duplicates);
		TB_DOCK(value);
		if(prev) {
			prev->next  = *node;
			(*node)->prev = prev;
		}
		if(next) {
			next->prev = *node;
			(*node)->next = next;
		}

    members->size++;
    if(!members->frozen) tb_hash_resize(H);
  }

	return TB_OK;
}


static retcode_t tb_hash_insert(Hash_t H, tb_Object_t value, tb_Key_t key) {
  tb_hash_node_t     * node;
	tb_hash_node_t       prev    = NULL;
	tb_hash_node_t       next    = NULL;
  hash_extra_t         members = XHASH(H);
	int                  retcode = TB_OK;

	no_error;

	node = tb_hash_lookup_node(H, key);

  if(*node) {     // _don't_ overwrites value for this key (except
									// when dupes are allowed
		if((*node)->nb >0) {
			(*node)->nb++;
			(*node)->values = tb_xrealloc((*node)->values, sizeof(tb_Object_t)* (*node)->nb);
			(*node)->values[(*node)->nb -1] = value;
			TB_DOCK(value);
		} else {
			char buff[100];
			tb_warn("tb_Hash::insert: key <%s> collision\n", kt_getK2sz(members->kt)(key, buff));
			set_tb_errno(TB_ERR_ALLREADY);
			retcode = TB_KO;
		}
  } else {
    *node = tb_node_new(key, value, members->kt, members->allow_duplicates);
		TB_DOCK(value);
		if(prev) {
			prev->next  = *node;
			(*node)->prev = prev;
		}
		if(next) {
			next->prev = *node;
			(*node)->next = next;
		} 

    members->size++;
    if(!members->frozen) tb_hash_resize(H);
  }

	return retcode;
}


static retcode_t tb_hash_remove(Hash_t H, tb_Key_t key) {
  tb_hash_node_t       * node;
	tb_hash_node_t         n, p, t;
  hash_extra_t           members = XHASH(H);

	no_error;

  node = tb_hash_lookup_node(H, key);

  if(*node) {
		n = (*node)->next;
		p = (*node)->prev;

		t = (p != NULL) ? p : n;

		tb_node_free(*node, XHASH(H)->kt);
		*node = NULL;

    if(p != NULL || n != NULL) {
      if(p) p->next = n;
      if(n) n->prev = p;
    }

		while(t && t->prev) t = t->prev; // ??? fixme  

    members->size--;

    members->nodes[ __hash_me(H, key) % members->buckets] = t;

		if(!members->frozen)  tb_hash_resize(H);
  }                                     

	return TB_OK;
}

static tb_Object_t tb_hash_take(Hash_t H, tb_Key_t key) {
  tb_hash_node_t    * node;
	tb_hash_node_t      n, p, t;
  hash_extra_t        members   = XHASH(H);
	tb_Object_t         O         = NULL;

	no_error;

  node = tb_hash_lookup_node(H, key);
  if(*node) {
		n = (*node)->next;
		p = (*node)->prev;

		t = (p != NULL) ? p : n;

		if((*node)->nb >0) {
			(*node)->nb--;
			O = (*node)->values[(*node)->nb];
			TB_UNDOCK(O);
			if((*node)->nb ==0) {
				kt_getKfree(members->kt)((*node)->key);
				tb_xfree(*node);
				*node = NULL;

				if(p) p->next = n;
				if(n) n->prev = p;

				while(t && t->prev) t = t->prev; 

				members->nodes[ __hash_me(H, key) % members->buckets] = t;

				members->size--;
				if(!members->frozen) tb_hash_resize(H);
			}			
		} else {
			O = (*node)->value;
			TB_UNDOCK(O);
			kt_getKfree(members->kt)((*node)->key);
			tb_xfree(*node);
			*node = NULL;

			if(p) p->next = n;
			if(n) n->prev = p;

			while(t && t->prev) t = t->prev; 

			members->nodes[ __hash_me(H, key) % members->buckets] = t;

			members->size--;
			if(!members->frozen) tb_hash_resize(H);
		}
  }

	return O;
}


#ifdef OBSOLETE_NOW
static char *tb_hash_stringify(Hash_t O) {
  int           i, nb=0;
  hash_extra_t  m = XHASH(O);
	tb_Clear(m->Stringified);

	if(m->size == 0) {
		tb_StrAdd(m->Stringified, -1, "{}");
	} else {
		tb_StrAdd(m->Stringified, -1, "{");

		for (i = 0; i < m->buckets; i++) {
			tb_hash_node_t node;
			for(node = m->nodes[i]; node != NULL;) {
				if(node) {
					char *(*p)(tb_Object_t) = tb_getMethod(node->value, OM_STRINGIFY);
					if(p) {
						char buff[20]; // fixme: may overflow
						tb_StrAdd(m->Stringified, -1, "%s=%s", 
											kt_getK2sz(m->kt)(node->key, buff),
											p(node->value));
						if(nb++ <m->size-1) {
							tb_StrAdd(m->Stringified, -1, ", ");
						}
					}
				}
				node = node->next;
			}
		}
		tb_StrAdd(m->Stringified, -1, "}");
	}
	return tb_toStr(m->Stringified);
}
#endif

static String_t tb_hash_stringify(Hash_t O) {
  int           i, nb =0;
  hash_extra_t  m     = XHASH(O);
	String_t      str   = tb_String(NULL);

	if(m->size == 0) {
		tb_StrAdd(str, -1, "{}");
	} else {
		tb_StrAdd(str, -1, "{");

		for (i = 0; i < m->buckets; i++) {
			tb_hash_node_t node;
			for(node = m->nodes[i]; node != NULL;) {
				if(node) {
					String_t(*p)(tb_Object_t) = tb_getMethod(node->value, OM_STRINGIFY);
					if(p) {
						char buff[64]; // fixme: may overflow
						String_t Rez = p(node->value);
						tb_StrAdd(str, -1, "%s=%S", 
											kt_getK2sz(m->kt)(node->key, buff),
											Rez);
						tb_Free(Rez);
						if(nb++ <m->size-1) {
							tb_StrAdd(str, -1, ", ");
						}
					}
				}
				node = node->next;
			}
		}
		tb_StrAdd(str, -1, "}");
	}
	return str;
}


static void tb_hash_marshall(String_t marshalled, Hash_t O, int level) {
  int           i;
  hash_extra_t  members              = XHASH(O);
	char          indent[level+3];

	if(marshalled == NULL) return;

	memset(indent, ' ', level+2);
	indent[level+2] = 0;

	tb_StrFill(marshalled, level, -1, ' ');

	if(members->size == 0) {
		tb_StrAdd(marshalled, -1, "<struct />\n");
	} else {
		tb_StrAdd(marshalled, -1, "<struct>\n");

		for (i = 0; i < members->buckets; i++) {
			tb_hash_node_t node;
			for(node = members->nodes[i]; node != NULL;) {
				if(node) {
					void(*p)(String_t, tb_Object_t, int) = tb_getMethod(node->value, OM_MARSHALL);
					if(p) {
						char buff[20]; // fixme: may overflow
						tb_StrAdd(marshalled, -1, "%s<member>\n", indent);
						tb_StrAdd(marshalled, -1, "%s  <name>%s</name>\n", indent, 
											kt_getK2sz(members->kt)(node->key, buff));
						tb_StrAdd(marshalled, -1, "%s  <value>\n", indent);
						p(marshalled, node->value, level+6);
						tb_StrAdd(marshalled, -1, "%s  </value>\n%s</member>\n", indent, indent);
					}
				}
				node = node->next;
			}
		}
		tb_StrFill(marshalled, level, -1, ' ');
		tb_StrAdd(marshalled, -1, "</struct>\n");
	}
}

static Hash_t tb_hash_unmarshall(XmlElt_t xml_element) {
	Vector_t   Ch;
	Hash_t     Rez;
	int        i, children;
 
	if(! streq(tb_toStr(XELT_getName(xml_element)), "struct")) {
		tb_warn("tb_hash_unmarshall: not a STRUCT Elmt\n");
		return NULL;
	}
	Rez = tb_Hash();
	Ch = XELT_getChildren(xml_element);
	children = tb_getSize(Ch);

	for(i = 0; i<children; i++) {
		XmlElt_t member = tb_Get(Ch, i);
		if(streq(tb_toStr(XELT_getName(member)), "member")) {
			if(tb_getSize(XELT_getChildren(member)) == 2) {
				XmlElt_t Key   = tb_Get(XELT_getChildren(member), 0);
				XmlElt_t Value = tb_Get(XELT_getChildren(member), 1);
					
				if(streq(S2sz(XELT_getName(Key)), "name") &&
					 streq(S2sz(XELT_getName(Value)), "value")) {

					tb_Object_t O = tb_XmlunMarshall(tb_Get(XELT_getChildren(Value), 0));

					if(O) {
						tb_Insert(Rez, O, tb_toStr(XELT_getText(tb_Get(XELT_getChildren(Key), 0)))); 
					}
				} else {
					tb_warn("unmarshalling pb (2)\n");
				}
			} else {
				tb_warn("unmarshalling pb (3)\n");
			}
		} 
	}

	return Rez;
}


#ifndef DOXYGEN_SHOULD_SKIP_THIS


/*
**  Str - String Library
**  Copyright (c) 1999-2000 Ralf S. Engelschall <rse@engelschall.com>
**
**  This file is part of Str, a string handling and manipulation 
**  library which can be found at http://www.engelschall.com/sw/str/.
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
**
**  str_hash.c: hashing functions 
*/





/**
 * DJBX33A (Daniel J. Bernstein, Times 33 with Addition)
 *
 * This is Daniel J. Bernstein's popular `times 33' hash function as
 * posted by him years ago on comp.lang.c. It basically uses a function
 * like ``hash(i) = hash(i-1) * 33 + string[i]''. This is one of the
 * best hashing functions for strings. Because it is both computed very
 * fast and distributes very well.
 *
 * The magic of the number 33, i.e. why it works better than many other
 * constants, prime or not, has never been adequately explained by
 * anyone. So I try an own RSE-explanation: if one experimentally tests
 * all multipliers between 1 and 256 (as I did it) one detects that
 * even numbers are not useable at all. The remaining 128 odd numbers
 * (except for the number 1) work more or less all equally well. They
 * all distribute in an acceptable way and this way fill a hash table
 * with an average percent of approx. 86%. 
 *
 * If one compares the Chi/2 values resulting of the various
 * multipliers, the 33 not even has the best value. But the 33 and a
 * few other equally good values like 17, 31, 63, 127 and 129 have
 * nevertheless a great advantage over the remaining values in the large
 * set of possible multipliers: their multiply operation can be replaced
 * by a faster operation based on just one bit-wise shift plus either a
 * single addition or subtraction operation. And because a hash function
 * has to both distribute good and has to be very fast to compute, those
 * few values should be preferred and seems to be also the reason why
 * Daniel J. Bernstein also preferred it.
 * \ingroup Hash
 */
static unsigned long hash_djbx33(register unsigned char *key, register size_t len) {
    register unsigned long hash = 5381;

    /* the hash unrolled eight times */
    for (; len >= 8; len -= 8) {
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
    }
    switch (len) {
        case 7: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 6: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 5: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 4: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 3: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 2: hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 1: hash = ((hash << 5) + hash) + *key++; break;
        default: /* case 0: */ break;
    }
    return hash;
}


#endif //doxygen










