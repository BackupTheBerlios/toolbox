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
use Data::Dumper;

sub loadParams($$) {
	my $template = shift;
	my $Params = shift;
	my $line;
	my $in_licence = 0;
	my $in_members = 0;

	$$Params{Parent} = "";

	open(TMPL, "$template") or die "template: $$!";
	while(<TMPL>) {
		chomp($_);
		$line = $_;
		next if(/^\s*\#/);


		if($in_licence) {
			if($line=~ /^\s*end_licence$/) {
				$in_licence = 0;
			} else {
				$$Params{Licence} .= "$line\n";
			}
		} elsif($in_members) {
			if($line=~ /^\s*end_members$/) {
				$in_members = 0;
			} elsif($line=~ /\s*(__(?:PRIVATE|PUBLIC)_)\s+(\w+)\s+(.*)$/ ) {
				$$Params{Member}{$2}{type} = $3;
				$$Params{Member}{$2}{visibility} = $1;
			}
		} elsif($line=~ /\s*TbxBuiltin/) {
			$$Params{TbxBuiltin} = 1;
		} elsif($line=~ /\s*ClassPrefix\s+(\w+)/) {
			$$Params{ClassPrefix} = $1;
		} elsif($line=~ /\s*ClassName\s+(\w+)/) {
			$$Params{ClassName} = $1;
		} elsif($line=~ /\s*ClassParent\s+(\w+)/ ) {
			$$Params{ClassParent} = $1;
		} elsif($line=~ /\s*Interface\s+(\w+)/) {
			push(@{$$Params{Interface}}, $1);
		} elsif($line=~ /\s*ClassMethod\s+
						(__(?:PRIVATE|PUBLIC)_)*\s*
						(__TB_(?:MUTATOR|INSPECTOR|FACTORY|CTOR|DTOR)_)*\s*
						(\w+)\s+
						(\w+)\s*
						[(]([^)]*)[)]/x ) {

    #ClassMethod __PUBLIC_ __TB_FACTORY_ String_t  tb_StrSub(String_t, int,  int)

      warn("$4: ($1) [$2] $3 ($5)\n");

      $$Params{CM}{$4}{visibility} = $1;
      $$Params{CM}{$4}{style}      = $2;
      $$Params{CM}{$4}{return}     = $3;
      $$Params{CM}{$4}{args}       = $5;

			if($2 ne "__TB_CTOR_") {
				push(@{$$Params{ClassMethod}}, $4);
			} else {
				warn("got a ctor<$4>\n");
				$$Params{CTOR} = $4;
			}
		} elsif($line=~ /\s*begin_licence/ ) {
			$in_licence = 1;
		} elsif($line=~ /\s*begin_members/ ) {
			$in_members = 1;
		} elsif($line=~ /\s*Author\s+(.*)$/) {
			$$Params{Author} = $1;
		}
	}
}


#################################################################################################"
#################################################################################################"

#################################################################################################"
#################################################################################################"



if(!defined $ARGV[0]) {
	print "usage:\ngenClass.pl template.file [tbx-builtin]\n";
  exit;
}

my %Params;

loadParams($ARGV[0], \%Params);


$class_name   = $Params{ClassName};
$class_parent = $Params{ClassParent};
$author       = $Params{Author};
$licence      = $Params{Licence};
%Members      = %{$Params{Member}};

@ClassMethods = @{$Params{ClassMethod}};
@Interfaces = @{$Params{Interface}};

if(defined $Params{CTOR}) {
	$Ctor = $Params{CTOR};
	warn(">$Ctor<\n");
	$ctor_args = $Params{CM}{$Ctor}{args};
  $list_ctor_args = ",".$ctor_args;
}

if(defined $class_parent) {
	$parent_h = "#include \"$class_parent.h\"";
	$parent_impl_h = "$class_parent"."_impl.h";
  $class_parent_id = uc($class_parent) . "_T";
} else {
	$class_parent_id = "NULL";
}

if(defined @Interfaces) {
	$got_interfaces = "Implements ".join(",", @Interfaces);
} else {
	$got_interfaces = "";
}

if(defined($Params{TbxBuiltin})) {
	$builtin = 1;
  $isBUILD = "#ifndef __BUILD\n#define __BUILD\n#endif\n"
} else {
	$builtin = 0;
}

$fnc_prefix = "";
if(defined($Params{ClassPrefix})) {
	$fnc_prefix = $Params{ClassPrefix};
}


$cvstag = "\%Id:\%";
$cvstag =~ s/\%/\$/g;

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
	for my $v (keys( %Members)) {
		$members_struct .= sprintf("\t%-25s %s; //%s\n",
																$Members{$v}{type},
																$v,
																$Members{$v}{visibility});
	}
	$members_struct .= "};\n";
	$members_struct .= "typedef struct $class_name_members * $class_name_members_t;\n";
	$setters = $setters_impl ="";
	$getters = $getters_impl = "";
	for my $v (keys(%Members)) {
		if($Members{$v}{visibility} eq "__PUBLIC_") {
			$pub_setters .= sprintf("%-15s %-25s %s\n",
															"retcode_t",
															$fnc_prefix."set".$v,
															"($typedef Self, $Members{$v}{type} value);");
			$pub_getters .= sprintf("%-15s %-25s %s\n",
															$Members{$v}{type},
															$fnc_prefix."get".$v,
															"($typedef Self);");
		} else {
			$priv_setters .= sprintf("%-15s %-25s %s\n",
															 "retcode_t",
															 $fnc_prefix."set".$v,
															 "($typedef Self, $Members{$v}{type} value);");
			$priv_getters .= sprintf("%-15s %-25s %s\n",
															 $Members{$v}{type},
															 $fnc_prefix."get".$v,
															 "($typedef Self);");
		}

    $setters_impl .= "#ifdef BEWARE_A_STUPID_CODE_GENERATOR_DONE_THIS\n";
		$setters_impl .= "retcode_t $fnc_prefix"."set".$v."($typedef Self, $Members{$v}{type} value) {\n";
    $setters_impl .= "\t if(tb_valid(Self, $typedef, __FUNCTION__)) {\n";
    $setters_impl .= "\t\t($class_name_members_t) m = $Xclass(Self);\n";
    $setters_impl .= "\t\tm->$v = value;\n";
    $setters_impl .= "\t\treturn TB_OK;\n\t}\n\treturn TB_KO;\n}\n#endif\n\n";

    $getters_impl .= "#ifdef BEWARE_A_STUPID_CODE_GENERATOR_DONE_THIS\n";
		$getters_impl .= "$Members{$v}{type} $fnc_prefix"."get".$v."($typedef Self) {\n";
    $getters_impl .= "\t if(tb_valid(Self, $typedef, __FUNCTION__)) {\n";
    $getters_impl .= "\t\t($class_name_members_t) m = $Xclass(Self);\n";
    $getters_impl .= "\t\treturn m->$v;\n\t}\n";
    $getters_impl .= "\treturn /*some_big_bad_err_value*/\n}\n#endif\n\n";

	}

} else {
	$members_struct = "";
	$setters = $setters_impl ="";
	$getters = $getters_impl = "";
}



if(defined @ClassMethods) {
	$meth_id = 0;
	for my $method (@ClassMethods) {

    my $mid = "OM_".uc($method);
		if(! $builtin) {
			$methodIdentifiers .= "int OM_".uc($method). "\n";
		} 

		$registerMethods .=	sprintf("\t%-15s = tb_registerNew_ClassMethod(%-15s, OID);\n",
																$mid,
																"\"".$method."\"");

		if($builtin) {
			$registeredMethods .= sprintf("#define %-15s %20s\n",
																		$mid,
																		$meth_id++);
		} else {
			$registeredMethods .= sprintf("extern int %s;\n",	$mid);
		}

		$classMeth_h .= sprintf("%-15s %-15s %s%s);\n", 
														$Params{CM}{$method}{style},
														$Params{CM}{$method}{return},
														" $method(",
														$Params{CM}{$method}{args});

		$debug_redef .= sprintf("%-15s dbg_%-15s %s%s);\n", 
														$Params{CM}{$method}{return},
														" $method(char *__dbg_func_, char *__dbg_file_, int __dbg_line_,",
														$Params{CM}{$method}{args});

		$debug_redef .= sprintf("#define %-15s dbg_%s\n",
														$method."(x...)", 
														$method."(__FUNCTION__,__FILE__,__LINE__,x)");
		my $args = (length($Params{CM}{$method}{args}) >0)? $Params{CM}{$method}{args}:"";
		$classMeth .= "$Params{CM}{$method}{return} $method($args) {\n";
		$classMeth .= "\tif(tb_valid(Self, $type_tag_T, __FUNCTION__)) {\n";
		$classMeth .= "\t\tvoid *p;\n\n";
		$classMeth .= "\t\tif((p = tb_getMethod(Self, $mid))) {\n";
		
		$namelist = mkNameList($Params{CM}{$method}{args});
		$typelist = mkTypeList($Params{CM}{$method}{args});

		$classMeth .= "\t\t\treturn ((".$Params{CM}{$method}{return}."(*)($typelist))p)($namelist);\n";
		$classMeth .= "\t\t} else { \n";
		$classMeth .= "\t\t\ttb_error(\"%p (%d) [no $method method]\", Self, tb_isA(Self));\n";
		$classMeth .= "\t\t\tset_tb_errno(TB_ERR_NO_SUCH_METHOD);\n";

		if($Params{CM}{$method}{return} eq "retcode_t") {
			$errcode = "TB_KO";
		} elsif($Params{CM}{$method}{return} eq "void") {
			$errcode = "";
		} elsif($Params{CM}{$method}{return} =~ /tb_\w/) {
			$errcode = "NULL";
		} else {
			$errcode = "<err_code>";
		}
		$classMeth .= "\t\t}\n\t}\n\treturn $errcode;\n}\n\n";
	}
}


if($builtin) {
	$init_from_parent = "pthread_once(&__class_registry_init_once, tb_classRegisterInit);";
} else {
	$init_from_parent = 
	"\tpthread_once(&__init_".$class_parent."_once, __init_".$class_parent."_once);";
}

sub mkNameList() {
	my $args = shift;

	my @rez = split(/,/, $args);
	my @list =();
	for my $pair (@rez) {
		$pair =~ s/^\s+//;
		@broken = split(/\s+/, $pair);
		$b = pop(@broken);
		push(@list, $b);
	}
	return join(",", @list);
}

sub mkTypeList() {
	my $args = shift;
	my @list = ();
	my @rez = split(/,/, $args);
	for my $pair (@rez) {
		$pair =~ s/^\s+//;
		my @broken = split(/\s+/, $pair);
		pop @broken;
		push(@list, join(" ", @broken));
	}
	return join(",", @list);
}

 ##########################################################################################

open(C, ">${class_name}.c");

print C << "EOFC"
/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//===========================================================
// $cvstag
//===========================================================

// created on $date by $author

$licence

/**
 * \@file ${class_name}.c
 */

/**
 * \@defgroup $type_tag $typedef
 * \@ingroup $class_parent
 * Class $class_name (extends $class_paren, $got_interfaces)
 * description :
 *
 * ... here the header description
 */


$isBUILD

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "$class_impl_h"
#include "tb_ClassBuilder.h"

int $type_tag_T;

$methodIdentifiers

static void *$free( $typedef Self);
//static  $typedef $clone($typedef Self);
//static  void $dump($typedef Self, int level);
//static  $typedef $clear($typedef Self);
//...

void $setup_once(int OID);

inline $class_name_members_t $Xclass($typedef Self) {
	return ($class_name_members_t)((__members_t)tb_getMembers(Self, $type_tag_T))->instance;
}

pthread_once_t __$init_once = PTHREAD_ONCE_INIT;
void $init_once() {

$init_from_parent
	// init also friend classes here if needed
	$type_tag_T = tb_registerNewClass("$typedef", $class_parent_id, $setup_once);
}

void $setup_once(int OID) {
$registerMethods
	/* OM_NEW and OM_FREE are mandatory methods */
	tb_registerMethod(OID, OM_NEW,                    ${class_name}_new);
	tb_registerMethod(OID, OM_FREE,                   $free);
	/*  others are optionnal: this ones are common and shown as example */
//	tb_registerMethod(OID, OM_CLONE,                  $clone);
//	tb_registerMethod(OID, OM_DUMP,                   $dump);
//	tb_registerMethod(OID, OM_CLEAR,                  $clear);
}


$typedef ${class_name}_ctor($typedef Self $list_ctor_args) {
	/* specific ctor code goes here */
	return Self;
}

$typedef dbg_$class_name(char *func, char *file, int line
	                       $list_ctor_args) {
	set_tb_mdbg(func, file, line);
  return ${class_name}_ctor(${class_name}_new() $list_ctor_args);
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

$typedef ${class_name}($ctor_args) {
  return ${class_name}_ctor(${class_name}_new() $list_ctor_args);
}



/** Generic $typedef contructor
 *
 * called from children
 * remember that this generic ctor must not use parameters (for inheritancy compliance)
 */
$typedef ${class_name}_new() {
	tb_Object_t Self;
	pthread_once(&__$init_once, $init_once);
	$class_name_members_t m;
	Self =  tb_newParent($type_tag_T);

	Self->isA  = $type_tag_T;

	m = ($class_name_members_t)tb_xcalloc(1, sizeof(struct $class_name_members));
	Self->members->instance = m;

	/*  [... generic ctor code here (members init)...]	*/

	if(fm->dbg) fm_addObject(Self);

	return Self;
}

void *$free($typedef Self) {
	if(tb_valid(Self, $type_tag_T, __FUNCTION__)) {
    //$class_name_members_t m = $Xclass(Self);
		fm_fastfree_on();

		/*[ your dtor code here ]
		tb_xfree(m->my_own_member);
		don\'t free m itself : tb_freeMembers(Self) will take care of it
		*/

		tb_freeMembers(Self);
		fm_fastfree_off();
		Self->isA = $type_tag_T; // requiered for introspection (as we are unwinding dtors stack)
    return tb_getParentMethod(Self, OM_FREE);
	}

  return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   start of Autogerated interfaces for inheritable functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

$classMeth

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  end of  Autogerated interfaces for inheritable functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////



/*
static $typedef $clone($typedef Self) {
}
*/

/*
static void $dump($typedef Self, int level) {
}
*/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//    specific public class and related methods goes here
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

$setters_impl

$getters_impl


EOFC

#'

	;

close C;


open(HI, ">$class_impl_h");

print HI << "EOFHI"
/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $cvstag
//======================================================

// created on $date by $author


$licence


// Class $class_name private and internal methods and members


#ifndef $htag_impl
#define $htag_impl

#include "Toolbox.h"
#include "$class_h"
#include "$parent_impl_h"

extern pthread_once_t __$init_once;
extern void $init_once();

$members_struct

// Private setters & getters
$priv_getters
$priv_setters

// Protected constructor
$typedef $class_name_ctor($typedef Self $ctor_args);
// Private default constructor
$typedef ${class_name}_new(); // mandatory default ctor (without params)

// members accessor
inline $class_name_members_t $Xclass($typedef Self);

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

$licence

// Class $class_name public methods

#ifndef $htag
#define $htag

#include "Toolbox.h"
$parent_h

typedef tb_Object_t           $typedef;

extern int $type_tag_T;
// Class methods Identifiers
$registeredMethods


// --[public methods goes here ]--

// constructors
$typedef $class_name($ctor_arg);
// factories (produce new object(s))
/*...*/
// manipulators (change self)
$pub_setters
/*...*/
// inspectors (don\'t change self)
$pub_getters
/*...*/

$classMeth_h



#if defined TB_MEM_DEBUG && (! defined NDEBUG) && (! defined __BUILD)

$typedef dbg_$class_name(char *func, char *file, int line);
#define $class_name(x...)      dbg_$class_name(__FUNCTION__,__FILE__,__LINE__,x)

$debug_redef

#endif


#endif

EOFH


