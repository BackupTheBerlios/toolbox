#! /usr/bin/perl
#=====================================================================================
#
# Copyright (c) 1999-2004, Paul L. Gatille <paul.gatille@free.fr>
#
# This file is part of Toolbox, an object-oriented utility library
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the "Artistic License" which comes with this Kit.
#
# This software is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the Artistic License for more
# details.
#
#=====================================================================================
#
$author = "Paul L. Gatille <gatille\@audemat-aztec.com>";
#$licence = "Copyright (c) 2004, Audemat-Aztec http://audemat-aztec.com";
$licence = "Copyright (c) 2004, Paul Gatille";

%Members = (
#						"Default", "tb_Object_t",
						"Templates", "Hash_t",
	);

$fnc_prefix = "Rec_";
#
#=====================================================================================






if(!defined $ARGV[0] || !defined $ARGV[1]) {
	print "usage:\nperl new_class.pl <class_name> <parent_identifier> [author's name and email]\n";
	print "ex: perl new_class.pl myKillerString TB_STRING 'Pierre Tramo <aa\@bbbbbbb.ccc>'\n";
	exit;
}

$class_name   = $ARGV[0];
$class_parent = $ARGV[1];
if(defined $ARGV[2]) {
	$author = $ARGV[2];
}

$cvstag = "\$Id: new_class.pl,v 1.2 2004/05/24 16:37:53 plg Exp $";

$class_c = $class_name . ".c";
$class_h = $class_name . ".h";
$class_impl_h = $class_name . "_impl.h";

$type_tag = uc($class_name);
$type_tag_T = $type_tag. "_T";
$typedef  =$class_name."_t";
$htag_impl = "__".$type_tag."_IMPL_H";
$htag = "__".$type_tag."_H";

$class_name_members_t = $class_name."_members_t";
$class_name_members   = $class_name."_members";
$Xclass = "X$class_name";

$setup_once = "setup_".$class_name."_once";
$init_once  = "init_".$class_name."_once";

$new = $class_name."_new";
$free= $class_name."_free";
$clone = $class_name."_clone";
$dump = $class_name."_dump";
$clear = $class_name."_clear";

$date = scalar(localtime(time));
@stuff = localtime(time);
$year = $stuff[5] + 1900;




if(defined %Members) {
	$members_struct  = "struct $class_name_members {\n";
	for my $v (keys(%Members)) {
		$members_struct .= "  $Members{$v} $v;\n";
	}
	$members_struct .= "};\n";
	$members_struct .= "typedef struct $class_name_members * $class_name_members_t;\n";
	$setters = $setters_impl ="";
	$getters = $getters_impl = "";
	for my $v (keys(%Members)) {
		$setters .= "retcode_t $fnc_prefix"."set".$v."($typedef T, $Members{$v} val);\n";
		$setters_impl .= "retcode_t $fnc_prefix"."set".$v."($typedef T, $Members{$v} val) {\n}\n";
		$getters .= "$Members{$v} $fnc_prefix"."get".$v."($typedef T);\n";
		$getters_impl .= "$Members{$v} $fnc_prefix"."get".$v."($typedef T) {\n}\n";
	}

} else {
	$members_struct = "";
	$setters = $setters_impl ="";
	$getters = $getters_impl = "";
}




open(C, ">${class_name}_impl.c");

print C << "EOFC"
/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $cvstag
//======================================================

// created on $date by $author

/* 
 * $licence
 */

// Class $class_name (extends $class_parent, [implements interface ...])
// description :


/* uncomment only for internal Toolbox Class 
#ifndef BUILD
#define BUILD
#endif
*/

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "$class_h"
#include "$class_impl_h"
#include "tb_ClassBuilder.h"

int $type_tag_T;

static void *$free( $typedef Obj);
//static  $typedef $clone($typedef Obj);
//static  void $dump($typedef Obj);
//static  $typedef $clear($typedef Obj);
//...

void $setup_once(int OID);

inline $class_name_members_t $Xclass($typedef T) {
	return ($class_name_members_t)((__members_t)tb_getMembers(T, $type_tag_T))->instance;
}

pthread_once_t __$init_once = PTHREAD_ONCE_INIT;
void $init_once() {
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	// if you doesn\'t extends directly from a toolbox class, you must call your anscestor init
	$type_tag_T = tb_registerNewClass("$type_tag_T", $class_parent, $setup_once);
}

void $setup_once(int OID) {
	/* OM_NEW and OM_FREE are mandatory methods */
	tb_registerMethod(OID, OM_NEW,                    ${class_name}_new);
	tb_registerMethod(OID, OM_FREE,                   $free);
	/*  others are optionnal: this ones are common and shown as example */
//	tb_registerMethod(OID, OM_CLONE,                  $clone);
//	tb_registerMethod(OID, OM_DUMP,                   $dump);
//	tb_registerMethod(OID, OM_CLEAR,                  $clear);
}


$typedef dbg_$class_name(char *func, char *file, int line, /*[your args here]*/) {
	set_tb_mdbg(func, file, line);
	return ${class_name}_new( /*[your args]*/ );
}


 /** $typedef constructor
 *
 * oneliner header function description
 *
 * longer description/explanation
 *
 * \@param arg1 desc of arg1
 * \@param argn desc of argn
 * \@return descrition of return values
 *
 * \@see related entries
 * \\ingroup $type_tag
 */

$typedef ${class_name}( /*[ your args here ]*/ ) {
	return ${class_name}_new( /*[your args]*/ );
}



$typedef ${class_name}_new( /*[ your args here ]*/ ) {
	tb_Object_t This;
	pthread_once(&__$init_once, $init_once);
	$class_name_members_t m;
	This =  tb_newParent($type_tag_T); 
	
	This->isA  = $type_tag_T;



	m = ($class_name_members_t)tb_xcalloc(1, sizeof(struct $class_name_members));
	This->members->instance = m;

	/*  [... add your ctor code here ...]	*/

	if(fm->dbg) fm_addObject(This);

	return This;
}

void *$free($typedef Obj) {
	if(tb_valid(Obj, $type_tag_T, __FUNCTION__)) {

		fm_fastfree_on();

		/*[ your dtor code here ]
			m = $Xclass(Obj);
		tb_xfree(m->my_own_member);
		don\'t free m itself : tb_freeMembers(Obj) will take care of it
		*/
	
		tb_freeMembers(Obj);
		fm_fastfree_off();
		Obj->isA = $type_tag_T; // requiered for introspection (as we are unwinding dtors stack)
    return tb_getParentMethod(Obj, OM_FREE);
	}

  return NULL;
}

/*
static $typedef $clone($typedef This) {
}
*/

/*
static void $dump($typedef This) {
}
*/



EOFC

#'

	;



open(CLASS, ">${class_name}.c");

print CLASS << "EOFC"
/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $cvstag
//======================================================

// created on $date by $author

/* 
 * $licence
 */

/**
 * \@file ${class_name}.c
 */

/**
 * \@defgroup $type_tag $typedef
 * \@ingroup $class_parent
 * Class $class_name (extends $class_parent, [implements interface ...])
 * description :
 *
 * ... here the header description
 */

/* uncomment only for internal Toolbox Class 
#ifndef __BUILD
#define __BUILD
#endif
*/

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "$class_h"
#include "$class_impl_h"
#include "tb_ClassBuilder.h"


/* 

	specific public class and related methods goes here

*/
$setters_impl

$getters_impl

EOFC

#'

	;




open(HI, ">$class_impl_h");

print HI << "EOFHI"
/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $cvstag
//======================================================

// created on $date by $author

/* 
 * $licence
 */

// Class $class_name private and internal methods and members


#ifndef $htag_impl
#define $htag_impl

#include "Toolbox.h"
#include "$class_h"


$members_struct


inline $class_name_members_t $Xclass($typedef T);

#if defined TB_MEM_DEBUG && (! defined NDEBUG) && (! defined __BUILD)
$typedef dbg_$class_name(char *func, char *file, int line /*, [ your args here ]*/);
#define $class_name(x...)      dbg_$class_name(__FUNCTION__,__FILE__,__LINE__,x)
#endif

#endif

EOFHI
;

#'

	

open(H, ">$class_h");

print H << "EOFH"
/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $cvstag
//======================================================

// created on $date

/* Copyright (c) $year, $author
 *
$licence
 *
 */

// Class $class_name public methods

#ifndef $htag
#define $htag

#include "Toolbox.h"
#include "->[ here add parent\'s .h ]"

typedef tb_Object_t           $typedef;

extern int $type_tag_T;

// --[public methods goes here ]--

// constructors
$typedef $class_name( /*[ your args here ]*/ );
$typedef ${class_name}_new( /*[ your args here ]*/ );
// factories (produce new object(s))
/*...*/
// manipulators (change self) 
$setters
/*...*/
// inspectors (don\'t change self) 
$getters
/*...*/

#endif

EOFH
