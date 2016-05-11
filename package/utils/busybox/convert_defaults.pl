#!/usr/bin/env perl

sub default_for_applet($$$) {
	my $skip_hash = shift;
	my $applet = shift;
	my $default = shift;
	my @newdefault = ();

	foreach my $skip_flag (keys %$skip_hash) {
		my $skip_applets = $skip_hash->{$skip_flag};
		foreach my $skip_applet (@$skip_applets) {
			if ($skip_applet eq $applet) {
				push @newdefault, "n if $skip_flag";
				break
			}
		}
	}
	push @newdefault, $default;

	return \@newdefault;
}

sub skip_applet_if_prefer_full($$) {
	my $applet = shift;
	my $default = shift;

	my %skip_hash = (
	   "BUSYBOX_USE_BRIDGE", [ "BRCTL" ],
	   "BUSYBOX_USE_BZIP2", [ "BUNZIP2" ],
	   "BUSYBOX_USE_CIFS_UTILS", [ "FEATURE_MOUNT_CIFS" ],
	   "BUSYBOX_USE_COREUTILS", [ "BASENAME",
	      "CAT",
	      "CHGRP",
	      "CHMOD",
	      "CHOWN",
	      "CHROOT",
	      "CP",
	      "CUT",
	      "DATE",
	      "DD",
	      "DIRNAME",
	      "DU",
	      "ECHO",
	      "EXPR",
	      "FALSE",
	      "HEAD",
	      "ID",
	      "LN",
	      "LS",
	      "MD5SUM",
	      "MKDIR",
	      "MKFIFO",
	      "MKNOD",
	      "MKTEMP",
	      "MV",
	      "NICE",
	      "PRINTF",
	      "PWD",
	      "READLINK",
	      "RM",
	      "RMDIR",
	      "SEQ",
	      "SLEEP",
	      "SORT",
	      "SYNC",
	      "TAIL",
	      "TEE",
	      "TEST",
	      "TOUCH",
	      "TR",
	      "TRUE",
	      "UNAME",
	      "UNIQ",
	      "UPTIME",
	      "WC",
	      "YES" ],
	   "BUSYBOX_USE_DIFFUTILS", [ "CMP" ],
	   "BUSYBOX_USE_FINDUTILS", [ "FIND",
	      "XARGS" ],
	   "BUSYBOX_USE_GREP", [ "GREP" ],
	   "BUSYBOX_USE_IPROUTE2", [ "IP" ],
	   "BUSYBOX_USE_IPUTILS", [ "PING",
	      "PING6" ],
	   "BUSYBOX_USE_LESS", [ "LESS" ],
	   "BUSYBOX_USE_TAR", [ "TAR" ],
	   "BUSYBOX_USE_PROCPS_NG", [ "FREE",
	      "KILL",
	      "PGREP",
	      "PS",
	      "TOP" ],
	   "BUSYBOX_USE_SHADOW_UTILS", [ "PASSWD" ],
	   "BUSYBOX_USE_NETCAT", [ "NC" ],
	   "BUSYBOX_USE_UTIL_LINUX", [ "DMESG" ,
	      "HWCLOCK",
	      "LOGGER",
	      "MOUNT",
	      "UMOUNT",
	      "MKSWAP" ],
	   "BUSYBOX_USE_VIM", [ "VI" ] );

	return default_for_applet(\%skip_hash, $applet, $default);
}

while (<>) {
	/^(# )?CONFIG_([^=]+)(=(.+)| is not set)/ and do {
		my $default = $4;
		$1 and $default = "n";
		my $name = $2;
		my $type = "bool";
		$default =~ /^\"/ and $type = "string";
		$default =~ /^\d/ and $type = "int";
		( $2 eq 'INSTALL_NO_USR' ) and $default = "y";
		$default = skip_applet_if_prefer_full($name, $default);
		print "config BUSYBOX_DEFAULT_$name\n\t$type\n";
		foreach my $defitem (@$default) {
			print "\tdefault $defitem\n";
		}
	};
}
