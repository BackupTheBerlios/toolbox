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
use FileHandle;

sub loadParams($$) {
	my $template = shift;
	my $Params = shift;
	my $line;
	my $in_licence = 0;
	my $in_members = 0;
	
	my $fh = new FileHandle;
	$fh->open($template) or die "$template : $! \n";

	while(<$fh>) {
		$$Params{Template} .= $_;
		chomp($_);
		my $line = $_;

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
		} elsif($line=~ /\s*InterfaceName\s+(\w+)/) {
			$$Params{InterfaceName} = $1;
		} elsif($line=~ /\s*ClassName\s+(\w+)/) {
			$$Params{ClassName} = $1;
		} elsif($line=~ /\s*ClassPrefix\s+(\w+)/) {
			$$Params{ClassPrefix} = $1;
		} elsif($line=~ /\s*ClassParent\s+(\w+)/ ) {
			$$Params{ClassParent} = $1;
			my $parent = $1;
			my %H = ();
			warn("try to load $parent.tmpl\n");
			loadParams($parent.".tmpl", \%H);
			$$Params{Parent} = \%H;
			warn("loaded $parent.tmpl\n");

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
				warn("found a ctor : $4: ($1) [$2] $3 ($5)\n");
				push(@{$$Params{ClassMethod}}, $4) if(defined $$Params{CTORS});
				push(@{$$Params{CTORS}}, $4);
			}


		} elsif($line=~ /\s*Method\s+
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
				push(@{$$Params{Method}}, $4);
			} else {
				warn("found a ctor : $4: ($1) [$2] $3 ($5)\n");
				push(@{$$Params{CTORS}}, $4);
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
	print "usage:\ngenClass.pl template.tmpl\n";
  exit;
}

my %Params;

loadParams($ARGV[0], \%Params);

#print Dumper(%Params);


$TOK{class_name}   = $Params{ClassName};
$TOK{class_parent} = $Params{ClassParent};
$TOK{author}       = $Params{Author};
$TOK{licence}      = $Params{Licence};
%Members      = %{$Params{Member}};


@Interfaces = @{$Params{Interface}};
@Ctors = @{$Params{CTORS}};


if(defined $TOK{class_parent}) {
	$TOK{parent_h} = "#include \"$TOK{class_parent}.h\"";
  $TOK{class_parent_id} = uc($TOK{class_parent})."_T";
} else {
	$TOK{class_parent_id} = "NULL";
}

if(defined $Params{ClassPrefix}) {
	$TOK{fnc_prefix} = $Params{ClassPrefix};
} else {
	$TOK{fnc_prefix} = $TOK{class_name};
}

if(defined @Interfaces) {
	$TOK{got_interfaces} = ", Implements ".join(",", @Interfaces);
} else {
	$TOK{got_interfaces} = "";
}


$TOK{cvstag} = "\%Id:\%";
$TOK{cvstag} =~ s/\%/\$/g;

$TOK{class_c} = $TOK{class_name} . ".c";
$TOK{class_h} = $TOK{class_name} . ".h";
$TOK{class_impl_h} = $TOK{class_name} . "_impl.h";

$TOK{type_tag} = uc($TOK{class_name});
$TOK{type_tag_T} = $TOK{type_tag}. "_T";
$TOK{typedef}  =$TOK{class_name}."_t";
$TOK{htag_impl} = "__".$TOK{type_tag}."_IMPL_H";
$TOK{htag} = "__".$TOK{type_tag}."_H";

$TOK{class_name_members_t} = $TOK{class_name}."_members_t";
$TOK{class_name_members}   = $TOK{class_name}."_members";
$TOK{Xclass} = "X$TOK{class_name}";

$TOK{setup_once} = "setup_".$TOK{class_name}."_once";
$TOK{init_once}  = "init_".$TOK{class_name}."_once";

$TOK{new}   = $TOK{class_name}."_new";
$TOK{free}  = $TOK{class_name}."_free";
$TOK{clone} = $TOK{class_name}."_clone";
$TOK{dump} = $TOK{class_name}."_dump";
$TOK{clear} = $TOK{class_name}."_clear";

if(defined($Params{TbxBuiltin})) {
	$TOK{builtin} = 1;
  $TOK{isBUILD} = "#ifndef __BUILD\n#define __BUILD\n#endif\n";
  $TOK{define_classid_storage} = "//$TOK{type_tag_T} defined in Toolbox.h";
  $TOK{instanced_class_it_storage} = ""
} else {
	$TOK{define_classid_storage} = "extern int $TOK{type_tag_T}; // global dynamic storage for class id";
  $TOK{instanced_class_it_storage} = "int $TOK{type_tag_T};"
}


$TOK{date} = scalar(localtime(time));
@stuff = localtime(time);
$TOK{year} = $stuff[5] + 1900;

if(defined($Params{TbxBuiltin})) {
	$TOK{builtin} = 1;
  $TOK{isBUILD} = "#ifndef __BUILD\n#define __BUILD\n#endif\n";
  $TOK{define_classid_storage} = "//$type_tag_T defined in Toolbox.h";
} else {
	$TOK{define_classid_storage} = "extern int $type_tag_T; // global dynamic storage for class id";
}

if(defined @Ctors) {
	$TOK{ctor_args} = $Params{CM}{$Ctors[0]}{args};
  $TOK{list_ctor_args} = ",".$TOK{ctor_args} if(split(/,/, $TOK{ctor_args}));
	$TOK{class_name_ctor} = $Ctors[0];
}


if(defined %Members) {
	$TOK{members_struct}  = "struct $TOK{class_name_members} {\n";
	for my $v (keys( %Members)) {
		$TOK{members_struct} .= sprintf("\t%-25s %s; //%s\n",
																$Members{$v}{type},
																$v,
																$Members{$v}{visibility});
	}
	$TOK{members_struct} .= "};\n";
	$TOK{members_struct} .= "typedef struct $TOK{class_name_members} * $TOK{class_name_members_t};\n";
	$TOK{setters} = $TOK{setters_impl} ="";
	$TOK{getters} = $TOK{getters_impl} = "";
	for my $v (keys(%Members)) {
		if($Members{$v}{visibility} eq "__PUBLIC_") {
			$TOK{pub_setters} .= sprintf("%-15s %-25s %s\n",
															"retcode_t",
															$TOK{fnc_prefix}."set_".$v,
															"($TOK{typedef} Self, $Members{$v}{type} value);");
			$TOK{pub_getters} .= sprintf("%-15s %-25s %s\n",
															$Members{$v}{type},
															$TOK{fnc_prefix}."get_".$v,
															"($TOK{typedef} Self);");
		} else {
			$TOK{priv_setters} .= sprintf("%-15s %-25s %s\n",
															 "retcode_t",
															 $TOK{fnc_prefix}."set_".$v,
															 "($TOK{typedef} Self, $Members{$v}{type} value);");
			$TOK{priv_getters} .= sprintf("%-15s %-25s %s\n",
															 $Members{$v}{type},
															 $TOK{fnc_prefix}."get_".$v,
															 "($TOK{typedef} Self);");
		}

    $TOK{setters_impl} .= "#ifdef BEWARE_A_STUPID_CODE_GENERATOR_DONE_THIS\n";
		if($Members{$v}{visibility} eq "__PUBLIC_") {
			$TOK{setters_impl} .= "// Public method : must check type\n";
			$TOK{setters_impl} .= "retcode_t $TOK{fnc_prefix}"."set_".$v."($TOK{typedef} Self, $Members{$v}{type} value) {\n";
			$TOK{setters_impl} .= "\tif(tb_valid(Self, $TOK{typedef}, __FUNCTION__)) {\n";
			$TOK{setters_impl} .= "\t\t$TOK{class_name_members_t} m = $TOK{Xclass}(Self);\n";
			$TOK{setters_impl} .= "\t\tm->$v = value;\n";
			$TOK{setters_impl} .= "\t\treturn TB_OK;\n\t}\n";
			$TOK{setters_impl} .= "\treturn TB_KO;\n}\n";
		} else {
			$TOK{setters_impl} .= "// Private method : assuming type checking already done \n";
			$TOK{setters_impl} .= "retcode_t $TOK{fnc_prefix}"."set_".$v."($TOK{typedef} Self, $Members{$v}{type} value) {\n";
			$TOK{setters_impl} .= "\t$TOK{class_name_members_t} m = $TOK{Xclass}(Self);\n";
			$TOK{setters_impl} .= "\tm->$v = value;\n";
			$TOK{setters_impl} .= "\treturn TB_OK;\n";
			$TOK{setters_impl} .= "}\n";
		}
		$TOK{setters_impl} .= "#endif\n\n";


    $TOK{getters_impl} .= "#ifdef BEWARE_A_STUPID_CODE_GENERATOR_DONE_THIS\n";
		if($Members{$v}{visibility} eq "__PUBLIC_") {
			$TOK{getters_impl} .= "// Public method : must type check \n";
			$TOK{getters_impl} .= "$Members{$v}{type} $TOK{fnc_prefix}"."get_".$v."($TOK{typedef} Self) {\n";
			$TOK{getters_impl} .= "\tif(tb_valid(Self, $TOK{typedef}, __FUNCTION__)) {\n";
			$TOK{getters_impl} .= "\t\t$TOK{class_name_members_t} m = $TOK{Xclass}(Self);\n";
			$TOK{getters_impl} .= "\t\treturn m->$v;\n\t}\n";
			$TOK{getters_impl} .= "\treturn /*some_big_bad_err_value*/\n}\n";
		} else {
			$TOK{getters_impl} .= "// Private method : assuming type checking already done \n";
			$TOK{getters_impl} .= "$Members{$v}{type} $TOK{fnc_prefix}"."get_".$v."($TOK{typedef} Self) {\n";
			$TOK{getters_impl} .= "\t$TOK{class_name_members_t} m = $TOK{Xclass}(Self);\n";
			$TOK{getters_impl} .= "\treturn m->$v;\n}\n";
		}
		$TOK{getters_impl} .= "#endif\n\n";

	}

} else {
	$TOK{members_struct} = "";
	$TOK{setters} = $TOK{setters_impl} ="";
	$TOK{getters} = $TOK{getters_impl} = "";
}


$Tmp = \%Params;
do {
	$meth_id = 0;
  @ClassMethods = @{$$Tmp{ClassMethod}};
  $overload_func = 0;

  if($$Tmp{ClassName} ne $Params{ClassName}) {
	  $TOK{declareClassMethods}.= "//-----------------------------------------------------\n";
    $TOK{declareClassMethods}.= "// $$Tmp{ClassName} Class overloaded methods\n";
    $TOK{declareClassMethods}.= "//-----------------------------------------------------\n";
    $TOK{registerClassMethods}.="\t//---------------------------------------------------\n";
    $TOK{registerClassMethods}.="\t// $$Tmp{ClassName} Class overloaded methods\n";
    $TOK{registerClassMethods}.="\t//---------------------------------------------------\n";
    $TOK{method_local_impl}.=    "//----------------------------------------------------\n";
    $TOK{method_local_impl}.= "// $$Tmp{ClassName} Class overloaded methods\n";
    $TOK{method_local_impl}.=    "//----------------------------------------------------\n";
    $overload_func = 1;
  } else {
	  if($Params{CM}{$method}{visibility} eq "__PUBLIC_") {
		  $TOK{declareClassMethods}.= "//-----------------------------------------------------\n";
		  $TOK{declareClassMethods}.= "// $$Tmp{ClassName} Class methods implementation\n";
		  $TOK{declareClassMethods}.= "//-----------------------------------------------------\n";
		}
  }

	for my $method (@ClassMethods) {

    my $mid = uc("OM_".$TOK{fnc_prefix}.$method);
    


		$mid =~ s/TB_//;
		$TOK{class_method}= "$TOK{fnc_prefix}"."$method";
		$TOK{class_method}=~ s/tb_//;

			
		
  if($$Tmp{CM}{$method}{visibility} eq "__PUBLIC_" ) {
		$TOK{registerClassMethods}.= "// " if($overload_func);
		$TOK{registerClassMethods}.= sprintf("\ttb_registerMethod(OID, %-15s, %-20s);\n",
																		 $mid,
																		 "$TOK{class_method}");
	}

  if($$Tmp{CM}{$method}{visibility} eq "__PUBLIC_" ) {
		$TOK{declareClassMethods}.= "// " if($overload_func);
		$TOK{declareClassMethods}.= sprintf("static %-15s %s%s);\n", 
																		$$Tmp{CM}{$method}{return},
																		"$TOK{class_method}(",
																		$$Tmp{CM}{$method}{args});
 	}

		my $args = (length($$Tmp{CM}{$method}{args}) >0)? $$Tmp{CM}{$method}{args}:"";
		if($$Tmp{ClassName} eq $Params{ClassName}) {

			if($Params{CM}{$method}{visibility} eq "__PUBLIC_") {
				$TOK{methodIdentifiers}.= "int $mid\n";

				$TOK{registerMethods}.=	sprintf("\t%-15s = tb_registerNew_ClassMethod(%-15s, OID);\n",
																				$mid,
																				"\"".$method."\"");

				$TOK{registeredMethods}.= sprintf("extern int %s;\n",	$mid);
  		}

			if($Params{CM}{$method}{style} eq "__TB_FACTORY_") {
				$TOK{factory_classMeth_h}.= sprintf("%-15s %-25s(%s);\n", 
																				$$Tmp{CM}{$method}{return},
																				"$TOK{class_method}",
																				$$Tmp{CM}{$method}{args});
			} elsif($Params{CM}{$method}{style} eq "__TB_INSPECTOR_") {
				$TOK{inspector_classMeth_h}.= sprintf("%-15s %-25s(%s);\n", 
																					$$Tmp{CM}{$method}{return},
																					"$TOK{class_method}",
																					$$Tmp{CM}{$method}{args});
			} elsif($Params{CM}{$method}{style} eq "__TB_MUTATOR_") {
				$TOK{mutator_classMeth_h}.= sprintf("%-15s %-25s(%s);\n", 
																				$$Tmp{CM}{$method}{return},
																				"$TOK{class_method}",
																				$$Tmp{CM}{$method}{args});
			} elsif($Params{CM}{$method}{style} eq "__TB_CTOR_") {
				$TOK{ctor_classMeth_h}.= sprintf("%-15s %-25s(%s);\n", 
																		 $$Tmp{CM}{$method}{return},
																		 "$TOK{class_method}",
																		 $$Tmp{CM}{$method}{args});
			} else {
				$TOK{classMeth_h}.= sprintf("%-15s %-15s %-25s(%s);\n", 
																$$Tmp{CM}{$method}{style},
																$$Tmp{CM}{$method}{return},
																" $TOK{class_method}(",
																$$Tmp{CM}{$method}{args});
			}



			$namelist = mkNameList($$Tmp{CM}{$method}{args});
			$typelist = mkTypeList($$Tmp{CM}{$method}{args});


			if($Params{CM}{$method}{visibility} eq "__PUBLIC_") {
				$TOK{debug_redef}.= sprintf("%-15s dbg_%-15s %s%s);\n", 
																		$$Tmp{CM}{$method}{return},
																		"$TOK{class_method}(char *__dbg_func_, char *__dbg_file_, int __dbg_line_,",
																		$$Tmp{CM}{$method}{args});

				$TOK{debug_redef}.= sprintf("#define %-15s dbg_%s\n",
																		$TOK{class_method}."(x...)", 
																		$TOK{class_method}."(__FUNCTION__,__FILE__,__LINE__,x)");
				$TOK{dbgClassMeth} .= sprintf("%s dbg_%s %s%s) {\n", 
																			$$Tmp{CM}{$method}{return},
																			"$TOK{class_method}(char *__dbg_func_, char *__dbg_file_, int __dbg_line_,",
																			$$Tmp{CM}{$method}{args});
				$TOK{dbgClassMeth} .= "\tset_tb_mdbg(func, file, line);\n",
				$TOK{dbgClassMeth} .= sprintf("\treturn %s(%s);\n}\n",
																			"$TOK{class_method}", $namelist);
			}
																		




			if($Params{CM}{$method}{visibility} eq "__PUBLIC_") {
				$TOK{classMeth}.= "/**\n * Class method $TOK{class_method} (oneliner description)\n * \n * (longer description here)\n";
				for my $arg (split(/,/,$namelist)) {
					$TOK{classMeth}.= " * \@param $arg description\n";
				}
				if($$Tmp{CM}{$method}{return} ne "void") {
					$TOK{classMeth}.= " * \@return description\n";
				}
				$TOK{classMeth}.= " *\n * \@ingroup $TOK{class_name}\n";
				$TOK{classMeth}.= " */\n";

				$TOK{classMeth}.= "$$Tmp{CM}{$method}{return} $TOK{class_method}($args) {\n";
				$TOK{classMeth}.= "\tif(tb_valid(Self, $type_tag_T, __FUNCTION__)) {\n";
				$TOK{classMeth}.= "\t\tvoid *p;\n\n";
				$TOK{classMeth}.= "\t\tif((p = tb_getMethod(Self, $mid))) {\n";


				$TOK{classMeth} .= "\t\t\treturn ((".$$Tmp{CM}{$method}{return}."(*)($typelist))p)($namelist);\n";
				$TOK{classMeth} .= "\t\t}\n";

				if($$Tmp{CM}{$method}{return} eq "retcode_t") {
					$TOK{errcode} = "TB_KO";
				} elsif($$Tmp{CM}{$method}{return} eq "void") {
					$TOK{errcode} = "";
				} elsif($$Tmp{CM}{$method}{return} =~ /tb_\w/) {
					$TOK{errcode} = "NULL";
				} else {
					$TOK{errcode} = "<err_code>";
				}
				$TOK{classMeth} .= "\t}\n\treturn $errcode;\n}\n\n";
			}
		}

    $TOK{method_local_impl}.= "// " if($overload_func);
		$TOK{method_local_impl} .= "$$Tmp{CM}{$method}{return} $TOK{class_method}($args) { }\n";

	}

 if(defined($$Tmp{Parent})) {
  $Tmp = $$Tmp{Parent};
 } else {
	 $Tmp = undef;
 }
} while(defined $Tmp);


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

open(C, "canvas_c") or die "$!";
open(OUT, ">$TOK{class_name}.c");
while(<C>) {
	my $line = $_;
	$line =~ s/~(\w+)~/$TOK{$1}/ge;
	print OUT $line;
}
close C;

open(IMPL, "canvas_impl_h") or die "$!";
open(OUT, ">$TOK{class_name}_impl.h");
while(<IMPL>) {
	my $line = $_;
	$line =~ s/~(\w+)~/$TOK{$1}/ge;
	print OUT $line;
}
close IMPL;

open(H, "canvas_h") or die "$!";
open(OUT, ">$TOK{class_name}.h");
while(<H>) {
	my $line = $_;
	$line =~ s/~(\w+)~/$TOK{$1}/ge;
	print OUT $line;
}
close H;


exit;



