#! /usr/bin/perl

# HOWTO:
# compile your prog w/ -DTB_MEM_DEBUG
# link w/ -ltbx
# EXPORT fm_debug="file.name"
# exec the prog
# analyze leaks (perl leak.pl file.name)
#

while(<>) {
	chomp;
	$line = $_;
	($action, $addr, @vals) = split(/:/, $_);

	if($action =~ /^MARK/) {

	}	elsif($action =~ /^[+*][MCS]/) {
		$MEM_ALLOC{$addr} = $line;
	} elsif( $action =~ /^\+R/ ) {
		if( $addr eq "(nil)" ) {
			$MEM_ALLOC{$vals[0]} = $line;
		} else {
			delete($MEM_ALLOC{$addr});
			$MEM_ALLOC{$vals[0]} = $line;
		}
	} elsif( $action =~ /^\+O/ ) {
		$OBJ_ALLOC{$addr} = $line;
	} elsif( $action =~ /^[+*]F/ ) {
		delete($MEM_ALLOC{$addr});
	} elsif( $action =~ /^-O/ ) {
		delete($OBJ_ALLOC{$addr});
	}	else {
		print "?? $_\n";
	}
}
@M =  keys(%MEM_ALLOC);
$size = 0;
print $#M +1 . " chunks not freed\n";
for $a ( @M ) {
	$alloc{"*"} = "(libc)";
	$alloc{"+"} = "(fastmem)";

	$line = $MEM_ALLOC{ $a };
	($action, $addr, $sz, @val) = split(/:/, $line);
	if( $action =~ /R/) {
		$size += $val[0];
		$line =~ s/^([+*])R\{(\d+)\}.*/Thread $2 realloc $alloc{$1} $val[0] bytes (in $val[$#val -2]() \@ $val[$#val -1]:$val[$#val])/;
	} else {
		$size += $sz;
		$line =~ s/^([+*])M\{(\d+)\}.*/Thread $2 malloc $alloc{$1} $sz bytes (in $val[$#val -2]() \@ $val[$#val -1]:$val[$#val])/;
		$line =~ s/^([+*])+C\{(\d+)\}.*/Thread $2 calloc $alloc{$1} $sz bytes (in $val[$#val -2]() \@ $val[$#val -1]:$val[$#val])/;
		$line =~ s/^([+*])S\{(\d+)\}.*/Thread $2 strdup $alloc{$1} $sz bytes (in $val[$#val -2]() \@ $val[$#val -1]:$val[$#val])/;
	}
	print "$a - $line\n";
}
print "$size bytes leaked\n";


@O =  keys(%OBJ_ALLOC);
print $#O +1 . " objects not freed\n";
for $a ( @O ) {
	$line = $OBJ_ALLOC{ $a };
	($action, $addr, $type, @val) = split(/:/, $line);
	$line =~ s/^\+O\{(\d+)\}.*/$type \@ $addr : Thread $1 in $val[$#val -2]() \@ $val[$#val -1]:$val[$#val]/;

	print "$line\n";
}




