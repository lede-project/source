#!/usr/bin/env perl
use strict;

while (<>) {
	chomp;
	next if /^CONFIG_SIGNED_PACKAGES/;
	next unless /^CONFIG_([^=]+)=(.*)$/;

	my $var = $1;
	my $val = $2;
	my $type;

	next if $var eq 'ALL';

	if ($val eq 'y') {
		$type = "bool";
	} elsif ($val eq 'm') {
		$type = "tristate";
	} elsif ($val =~ /^".*"$/) {
		$type = "string";
	} elsif ($val =~ /^\d+$/) {
		$type = "int";
	} else {
		warn "WARNING: no type found for symbol CONFIG_$var=$val\n";
		next;
	}

	print <<EOF;
config $var
	$type
	default $val

EOF
}
