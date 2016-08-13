#!/usr/bin/env perl

while (<>) {
	/^(# )?CONFIG_([^=]+)(=(.+)| is not set)/ and do {
		my $default = $4;
		$1 and $default = "n";
		my $name = $2;
		my $type = "bool";
		$default =~ /^\"/ and $type = "string";
		$default =~ /^\d/ and $type = "int";
		( $2 eq 'INSTALL_NO_USR' ) and $default = "y";
		print "config BUSYBOX_DEFAULT_$name\n\t$type\n\tdefault $default\n";
	};
}
