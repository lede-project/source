#!/usr/bin/env perl
use FindBin;
use lib "$FindBin::Bin";
use strict;
use metadata;
use Getopt::Long;

my %board;

sub version_to_num($) {
	my $str = shift;
	my $num = 0;

	if (defined($str) && $str =~ /^\d+(?:\.\d+)+$/)
	{
		my @n = (split(/\./, $str), 0, 0, 0, 0);
		$num = ($n[0] << 24) | ($n[1] << 16) | ($n[2] << 8) | $n[3];
	}

	return $num;
}

sub version_filter_list(@) {
	my $cmpver = version_to_num(shift @_);
	my @items;

	foreach my $item (@_)
	{
		if ($item =~ s/@(lt|le|gt|ge|eq|ne)(\d+(?:\.\d+)+)\b//)
		{
			my $op = $1;
			my $symver = version_to_num($2);

			if ($symver > 0 && $cmpver > 0)
			{
				next unless (($op eq 'lt' && $cmpver <  $symver) ||
				             ($op eq 'le' && $cmpver <= $symver) ||
				             ($op eq 'gt' && $cmpver >  $symver) ||
				             ($op eq 'ge' && $cmpver >= $symver) ||
				             ($op eq 'eq' && $cmpver == $symver) ||
				             ($op eq 'ne' && $cmpver != $symver));
			}
		}

		push @items, $item;
	}

	return @items;
}

sub gen_kconfig_overrides() {
	my %config;
	my %kconfig;
	my $package;
	my $pkginfo = shift @ARGV;
	my $cfgfile = shift @ARGV;
	my $patchver = shift @ARGV;

	# parameter 2: build system config
	open FILE, "<$cfgfile" or return;
	while (<FILE>) {
		/^(CONFIG_.+?)=(.+)$/ and $config{$1} = 1;
	}
	close FILE;

	# parameter 1: package metadata
	open FILE, "<$pkginfo" or return;
	while (<FILE>) {
		/^Package:\s*(.+?)\s*$/ and $package = $1;
		/^Kernel-Config:\s*(.+?)\s*$/ and do {
			my @config = split /\s+/, $1;
			foreach my $config (version_filter_list($patchver, @config)) {
				my $val = 'm';
				my $override;
				if ($config =~ /^(.+?)=(.+)$/) {
					$config = $1;
					$override = 1;
					$val = $2;
				}
				if ($config{"CONFIG_PACKAGE_$package"} and ($config ne 'n')) {
					next if $kconfig{$config} eq 'y';
					$kconfig{$config} = $val;
				} elsif (!$override) {
					$kconfig{$config} or $kconfig{$config} = 'n';
				}
			}
		};
	};
	close FILE;

	foreach my $kconfig (sort keys %kconfig) {
		if ($kconfig{$kconfig} eq 'n') {
			print "# $kconfig is not set\n";
		} else {
			print "$kconfig=$kconfig{$kconfig}\n";
		}
	}
}

sub merge_package_lists($$) {
	my $list1 = shift;
	my $list2 = shift;
	my @l = ();
	my %pkgs;

	foreach my $pkg (@$list1, @$list2) {
		$pkgs{$pkg} = 1;
	}
	foreach my $pkg (keys %pkgs) {
		push @l, $pkg unless ($pkg =~ /^-/ or $pkgs{"-$pkg"});
	}
	return sort(@l);
}

sub target_config_features(@) {
	my $ret;

	while ($_ = shift @_) {
		/arm_v(\w+)/ and $ret .= "\tselect arm_v$1\n";
		/broken/ and $ret .= "\tdepends on BROKEN\n";
		/audio/ and $ret .= "\tselect AUDIO_SUPPORT\n";
		/display/ and $ret .= "\tselect DISPLAY_SUPPORT\n";
		/dt/ and $ret .= "\tselect USES_DEVICETREE\n";
		/gpio/ and $ret .= "\tselect GPIO_SUPPORT\n";
		/pci/ and $ret .= "\tselect PCI_SUPPORT\n";
		/pcie/ and $ret .= "\tselect PCIE_SUPPORT\n";
		/usb/ and $ret .= "\tselect USB_SUPPORT\n";
		/usbgadget/ and $ret .= "\tselect USB_GADGET_SUPPORT\n";
		/pcmcia/ and $ret .= "\tselect PCMCIA_SUPPORT\n";
		/rtc/ and $ret .= "\tselect RTC_SUPPORT\n";
		/squashfs/ and $ret .= "\tselect USES_SQUASHFS\n";
		/jffs2$/ and $ret .= "\tselect USES_JFFS2\n";
		/jffs2_nand/ and $ret .= "\tselect USES_JFFS2_NAND\n";
		/ext4/ and $ret .= "\tselect USES_EXT4\n";
		/targz/ and $ret .= "\tselect USES_TARGZ\n";
		/cpiogz/ and $ret .= "\tselect USES_CPIOGZ\n";
		/ubifs/ and $ret .= "\tselect USES_UBIFS\n";
		/fpu/ and $ret .= "\tselect HAS_FPU\n";
		/spe_fpu/ and $ret .= "\tselect HAS_SPE_FPU\n";
		/ramdisk/ and $ret .= "\tselect USES_INITRAMFS\n";
		/powerpc64/ and $ret .= "\tselect powerpc64\n";
		/nommu/ and $ret .= "\tselect NOMMU\n";
		/mips16/ and $ret .= "\tselect HAS_MIPS16\n";
		/rfkill/ and $ret .= "\tselect RFKILL_SUPPORT\n";
		/low_mem/ and $ret .= "\tselect LOW_MEMORY_FOOTPRINT\n";
		/nand/ and $ret .= "\tselect NAND_SUPPORT\n";
	}
	return $ret;
}

sub target_name($) {
	my $target = shift;
	my $parent = $target->{parent};
	if ($parent) {
		return $target->{parent}->{name}." - ".$target->{name};
	} else {
		return $target->{name};
	}
}

sub kver($) {
	my $v = shift;
	$v =~ tr/\./_/;
	if (substr($v,0,2) eq "2_") {
		$v =~ /(\d+_\d+_\d+)(_\d+)?/ and $v = $1;
	} else {
		$v =~ /(\d+_\d+)(_\d+)?/ and $v = $1;
	}
	return $v;
}

sub print_target($) {
	my $target = shift;
	my $features = target_config_features(@{$target->{features}});
	my $help = $target->{desc};
	my $confstr;

	chomp $features;
	$features .= "\n";
	if ($help =~ /\w+/) {
		$help =~ s/^\s*/\t  /mg;
		$help = "\thelp\n$help";
	} else {
		undef $help;
	}

	my $v = kver($target->{version});
	if (@{$target->{subtargets}} == 0) {
	$confstr = <<EOF;
config TARGET_$target->{conf}
	bool "$target->{name}"
	select LINUX_$v
EOF
	}
	else {
		$confstr = <<EOF;
config TARGET_$target->{conf}
	bool "$target->{name}"
EOF
	}
	if ($target->{subtarget}) {
		$confstr .= "\tdepends on TARGET_$target->{boardconf}\n";
	}
	if (@{$target->{subtargets}} > 0) {
		$confstr .= "\tselect HAS_SUBTARGETS\n";
		grep { /broken/ } @{$target->{features}} and $confstr .= "\tdepends on BROKEN\n";
	} else {
		$confstr .= $features;
		if ($target->{arch} =~ /\w/) {
			$confstr .= "\tselect $target->{arch}\n";
		}
	}

	foreach my $dep (@{$target->{depends}}) {
		my $mode = "depends on";
		my $flags;
		my $name;

		$dep =~ /^([@\+\-]+)(.+)$/;
		$flags = $1;
		$name = $2;

		next if $name =~ /:/;
		$flags =~ /-/ and $mode = "deselect";
		$flags =~ /\+/ and $mode = "select";
		$flags =~ /@/ and $confstr .= "\t$mode $name\n";
	}
	$confstr .= "$help\n\n";
	print $confstr;
}

sub gen_target_config() {
	my $file = shift @ARGV;
	my @target = parse_target_metadata($file);
	my %defaults;

	my @target_sort = sort {
		target_name($a) cmp target_name($b);
	} @target;


	print <<EOF;
choice
	prompt "Target System"
	default TARGET_ar71xx
	reset if !DEVEL
	
EOF

	foreach my $target (@target_sort) {
		next if $target->{subtarget};
		print_target($target);
	}

	print <<EOF;
endchoice

choice
	prompt "Subtarget" if HAS_SUBTARGETS
EOF
	foreach my $target (@target) {
		next unless $target->{def_subtarget};
		print <<EOF;
	default TARGET_$target->{conf}_$target->{def_subtarget} if TARGET_$target->{conf}
EOF
	}
	print <<EOF;

EOF
	foreach my $target (@target) {
		next unless $target->{subtarget};
		print_target($target);
	}

print <<EOF;
endchoice

choice
	prompt "Target Profile"

EOF

	foreach my $target (@target) {
		my $profiles = $target->{profiles};
		foreach my $profile (@{$target->{profiles}}) {
			print <<EOF;
config TARGET_$target->{conf}_$profile->{id}
	bool "$profile->{name}"
	depends on TARGET_$target->{conf}
EOF
			my @pkglist = merge_package_lists($target->{packages}, $profile->{packages});
			foreach my $pkg (@pkglist) {
				print "\tselect DEFAULT_$pkg\n";
				$defaults{$pkg} = 1;
			}
			my $help = $profile->{desc};
			if ($help =~ /\w+/) {
				$help =~ s/^\s*/\t  /mg;
				$help = "\thelp\n$help";
			} else {
				undef $help;
			}
			print "$help\n";
		}
	}

	print <<EOF;
endchoice

config HAS_SUBTARGETS
	bool

config TARGET_BOARD
	string

EOF
	foreach my $target (@target) {
		$target->{subtarget} or	print "\t\tdefault \"".$target->{board}."\" if TARGET_".$target->{conf}."\n";
	}
	print <<EOF;
config TARGET_SUBTARGET
	string
	default "generic" if !HAS_SUBTARGETS

EOF

	foreach my $target (@target) {
		foreach my $subtarget (@{$target->{subtargets}}) {
			print "\t\tdefault \"$subtarget\" if TARGET_".$target->{conf}."_$subtarget\n";
		}
	}
	print <<EOF;
config TARGET_PROFILE
	string
EOF
	foreach my $target (@target) {
		my $profiles = $target->{profiles};
		foreach my $profile (@$profiles) {
			print "\tdefault \"$profile->{id}\" if TARGET_$target->{conf}_$profile->{id}\n";
		}
	}

	print <<EOF;

config TARGET_ARCH_PACKAGES
	string
	
EOF
	foreach my $target (@target) {
		next if @{$target->{subtargets}} > 0;
		print "\t\tdefault \"".($target->{arch_packages} || $target->{board})."\" if TARGET_".$target->{conf}."\n";
	}
	print <<EOF;

config DEFAULT_TARGET_OPTIMIZATION
	string
EOF
	foreach my $target (@target) {
		next if @{$target->{subtargets}} > 0;
		print "\tdefault \"".$target->{cflags}."\" if TARGET_".$target->{conf}."\n";
	}
	print "\tdefault \"-Os -pipe -funit-at-a-time\"\n";
	print <<EOF;

config CPU_TYPE
	string
EOF
	foreach my $target (@target) {
		next if @{$target->{subtargets}} > 0;
		print "\tdefault \"".$target->{cputype}."\" if TARGET_".$target->{conf}."\n";
	}
	print "\tdefault \"\"\n";

	my %kver;
	foreach my $target (@target) {
		my $v = kver($target->{version});
		next if $kver{$v};
		$kver{$v} = 1;
		print <<EOF;

config LINUX_$v
	bool

EOF
	}
	foreach my $def (sort keys %defaults) {
		print "\tconfig DEFAULT_".$def."\n";
		print "\t\tbool\n\n";
	}
}

my %dep_check;
sub __find_package_dep($$) {
	my $pkg = shift;
	my $name = shift;
	my $deps = ($pkg->{vdepends} or $pkg->{depends});

	return 0 unless defined $deps;
	foreach my $dep (@{$deps}) {
		next if $dep_check{$dep};
		$dep_check{$dep} = 1;
		return 1 if $dep eq $name;
		return 1 if ($package{$dep} and (__find_package_dep($package{$dep},$name) == 1));
	}
	return 0;
}

# wrapper to avoid infinite recursion
sub find_package_dep($$) {
	my $pkg = shift;
	my $name = shift;

	%dep_check = ();
	return __find_package_dep($pkg, $name);
}

sub package_depends($$) {
	my $a = shift;
	my $b = shift;
	my $ret;

	return 0 if ($a->{submenu} ne $b->{submenu});
	if (find_package_dep($a, $b->{name}) == 1) {
		$ret = 1;
	} elsif (find_package_dep($b, $a->{name}) == 1) {
		$ret = -1;
	} else {
		return 0;
	}
	return $ret;
}

sub mconf_depends {
	my $pkgname = shift;
	my $depends = shift;
	my $only_dep = shift;
	my $res;
	my $dep = shift;
	my $seen = shift;
	my $parent_condition = shift;
	$dep or $dep = {};
	$seen or $seen = {};
	my @t_depends;

	$depends or return;
	my @depends = @$depends;
	foreach my $depend (@depends) {
		my $m = "depends on";
		my $flags = "";
		$depend =~ s/^([@\+]+)// and $flags = $1;
		my $vdep;
		my $condition = $parent_condition;

		next if $condition eq $depend;
		next if $seen->{"$parent_condition:$depend"};
		next if $seen->{":$depend"};
		$seen->{"$parent_condition:$depend"} = 1;
		if ($depend =~ /^(.+):(.+)$/) {
			if ($1 ne "PACKAGE_$pkgname") {
				if ($condition) {
					$condition = "$condition && $1";
				} else {
					$condition = $1;
				}
			}
			$depend = $2;
		}
		next if $package{$depend} and $package{$depend}->{buildonly};
		if ($flags =~ /\+/) {
			if ($vdep = $package{$depend}->{vdepends}) {
				my @vdeps = @$vdep;
				$depend = shift @vdeps;
				if (@vdeps > 1) {
					$condition = '!('.join("||", map { "PACKAGE_".$_ } @vdeps).')';
				} elsif (@vdeps > 0) {
					$condition = '!PACKAGE_'.$vdeps[0];
				}
			}

			# Menuconfig will not treat 'select FOO' as a real dependency
			# thus if FOO depends on other config options, these dependencies
			# will not be checked. To fix this, we simply emit all of FOO's
			# depends here as well.
			$package{$depend} and push @t_depends, [ $package{$depend}->{depends}, $condition ];

			$m = "select";
			next if $only_dep;
		} else {
			if ($vdep = $package{$depend}->{vdepends}) {
				$depend = join("||", map { "PACKAGE_".$_ } @$vdep);
			}
		}
		$flags =~ /@/ or $depend = "PACKAGE_$depend";
		if ($condition) {
			if ($m =~ /select/) {
				next if $depend eq $condition;
				$depend = "$depend if $condition";
			} else {
				$depend = "!($condition) || $depend" unless $dep->{$condition} eq 'select';
			}
		}
		$dep->{$depend} =~ /select/ or $dep->{$depend} = $m;
	}

	foreach my $tdep (@t_depends) {
		mconf_depends($pkgname, $tdep->[0], 1, $dep, $seen, $tdep->[1]);
	}

	foreach my $depend (keys %$dep) {
		my $m = $dep->{$depend};
		$res .= "\t\t$m $depend\n";
	}
	return $res;
}

sub mconf_conflicts {
	my $pkgname = shift;
	my $depends = shift;
	my $res = "";

	foreach my $depend (@$depends) {
		next unless $package{$depend};
		$res .= "\t\tdepends on m || (PACKAGE_$depend != y)\n";
	}
	return $res;
}

sub print_package_config_category($) {
	my $cat = shift;
	my %menus;
	my %menu_dep;

	return unless $category{$cat};

	print "menu \"$cat\"\n\n";
	my %spkg = %{$category{$cat}};

	foreach my $spkg (sort {uc($a) cmp uc($b)} keys %spkg) {
		foreach my $pkg (@{$spkg{$spkg}}) {
			next if $pkg->{buildonly};
			my $menu = $pkg->{submenu};
			if ($menu) {
				$menu_dep{$menu} or $menu_dep{$menu} = $pkg->{submenudep};
			} else {
				$menu = 'undef';
			}
			$menus{$menu} or $menus{$menu} = [];
			push @{$menus{$menu}}, $pkg;
		}
	}
	my @menus = sort {
		($a eq 'undef' ?  1 : 0) or
		($b eq 'undef' ? -1 : 0) or
		($a cmp $b)
	} keys %menus;

	foreach my $menu (@menus) {
		my @pkgs = sort {
			package_depends($a, $b) or
			($a->{name} cmp $b->{name})
		} @{$menus{$menu}};
		if ($menu ne 'undef') {
			$menu_dep{$menu} and print "if $menu_dep{$menu}\n";
			print "menu \"$menu\"\n";
		}
		foreach my $pkg (@pkgs) {
			my $title = $pkg->{name};
			my $c = (72 - length($pkg->{name}) - length($pkg->{title}));
			if ($c > 0) {
				$title .= ("." x $c). " ". $pkg->{title};
			}
			$title = "\"$title\"";
			print "\t";
			$pkg->{menu} and print "menu";
			print "config PACKAGE_".$pkg->{name}."\n";
			$pkg->{hidden} and $title = "";
			print "\t\t".($pkg->{tristate} ? 'tristate' : 'bool')." $title\n";
			print "\t\tdefault y if DEFAULT_".$pkg->{name}."\n";
			unless ($pkg->{hidden}) {
				my @def = ("ALL");
				if (!exists($pkg->{repository})) {
					push @def, "ALL_NONSHARED";
				}
				if ($pkg->{name} =~ /^kmod-/) {
					push @def, "ALL_KMODS";
				}
				$pkg->{default} ||= "m if " . join("||", @def);
			}
			if ($pkg->{default}) {
				foreach my $default (split /\s*,\s*/, $pkg->{default}) {
					print "\t\tdefault $default\n";
				}
			}
			print mconf_depends($pkg->{name}, $pkg->{depends}, 0);
			print mconf_depends($pkg->{name}, $pkg->{mdepends}, 0);
			print mconf_conflicts($pkg->{name}, $pkg->{conflicts});
			print "\t\thelp\n";
			print $pkg->{description};
			print "\n";

			$pkg->{config} and print $pkg->{config}."\n";
		}
		if ($menu ne 'undef') {
			print "endmenu\n";
			$menu_dep{$menu} and print "endif\n";
		}
	}
	print "endmenu\n\n";

	undef $category{$cat};
}

sub print_package_features() {
	keys %features > 0 or return;
	print "menu \"Package features\"\n";
	foreach my $n (keys %features) {
		my @features = sort { $b->{priority} <=> $a->{priority} or $a->{title} cmp $b->{title} } @{$features{$n}};
		print <<EOF;
choice
	prompt "$features[0]->{target_title}"
	default FEATURE_$features[0]->{name}
EOF

		foreach my $feature (@features) {
			print <<EOF;
	config FEATURE_$feature->{name}
		bool "$feature->{title}"
EOF
			$feature->{description} =~ /\w/ and do {
				print "\t\thelp\n".$feature->{description}."\n";
			};
		}
		print "endchoice\n"
	}
	print "endmenu\n\n";
}

sub print_package_overrides() {
	keys %overrides > 0 or return;
	print "\tconfig OVERRIDE_PKGS\n";
	print "\t\tstring\n";
	print "\t\tdefault \"".join(" ", keys %overrides)."\"\n\n";
}

sub gen_package_config() {
	parse_package_metadata($ARGV[0]) or exit 1;
	print "menuconfig IMAGEOPT\n\tbool \"Image configuration\"\n\tdefault n\n";
	foreach my $preconfig (keys %preconfig) {
		foreach my $cfg (keys %{$preconfig{$preconfig}}) {
			my $conf = $preconfig{$preconfig}->{$cfg}->{id};
			$conf =~ tr/\.-/__/;
			print <<EOF
	config UCI_PRECONFIG_$conf
		string "$preconfig{$preconfig}->{$cfg}->{label}" if IMAGEOPT
		depends on PACKAGE_$preconfig
		default "$preconfig{$preconfig}->{$cfg}->{default}"

EOF
		}
	}
	print "source \"package/*/image-config.in\"\n";
	if (scalar glob "package/feeds/*/*/image-config.in") {
	    print "source \"package/feeds/*/*/image-config.in\"\n";
	}
	print_package_features();
	print_package_config_category 'Base system';
	foreach my $cat (sort {uc($a) cmp uc($b)} keys %category) {
		print_package_config_category $cat;
	}
	print_package_overrides();
}

sub get_conditional_dep($$) {
	my $condition = shift;
	my $depstr = shift;
	if ($condition) {
		if ($condition =~ /^!(.+)/) {
			return "\$(if \$(CONFIG_$1),,$depstr)";
		} else {
			return "\$(if \$(CONFIG_$condition),$depstr)";
		}
	} else {
		return $depstr;
	}
}

sub gen_package_mk() {
	my %conf;
	my %dep;
	my %done;
	my $line;

	parse_package_metadata($ARGV[0]) or exit 1;
	foreach my $name (sort {uc($a) cmp uc($b)} keys %package) {
		my $config;
		my $pkg = $package{$name};
		my @srcdeps;

		next if defined $pkg->{vdepends};

		$config = "\$(CONFIG_PACKAGE_$name)";
		if ($config) {
			$pkg->{buildonly} and $config = "";
			print "package-$config += $pkg->{subdir}$pkg->{src}\n";
			if ($pkg->{variant}) {
				if (!defined($done{$pkg->{src}}) or $pkg->{variant_default}) {
					print "\$(curdir)/$pkg->{subdir}$pkg->{src}/default-variant := $pkg->{variant}\n";
				}
				print "\$(curdir)/$pkg->{subdir}$pkg->{src}/variants += \$(if $config,$pkg->{variant})\n"
			}
			$pkg->{prereq} and print "prereq-$config += $pkg->{subdir}$pkg->{src}\n";
		}

		next if $done{$pkg->{src}};
		$done{$pkg->{src}} = 1;

		if (@{$pkg->{buildtypes}} > 0) {
			print "buildtypes-$pkg->{subdir}$pkg->{src} = ".join(' ', @{$pkg->{buildtypes}})."\n";
		}

		foreach my $spkg (@{$srcpackage{$pkg->{src}}}) {
			foreach my $dep (@{$spkg->{depends}}, @{$spkg->{builddepends}}) {
				$dep =~ /@/ or do {
					$dep =~ s/\+//g;
					push @srcdeps, $dep;
				};
			}
		}
		foreach my $type (@{$pkg->{buildtypes}}) {
			my @extra_deps;
			my %deplines;

			next unless $pkg->{"builddepends/$type"};
			foreach my $dep (@{$pkg->{"builddepends/$type"}}) {
				my $suffix = "";
				my $condition;

				if ($dep =~ /^(.+):(.+)/) {
					$condition = $1;
					$dep = $2;
				}
				if ($dep =~ /^(.+)(\/.+)/) {
					$dep = $1;
					$suffix = $2;
				}

				my $idx = "";
				my $pkg_dep = $package{$dep};
				if (defined($pkg_dep) && defined($pkg_dep->{src})) {
					$idx = $pkg_dep->{subdir}.$pkg_dep->{src};
				} elsif (defined($srcpackage{$dep})) {
					$idx = $subdir{$dep}.$dep;
				} else {
					next;
				}
				my $depstr = "\$(curdir)/$idx$suffix/compile";
				my $depline = get_conditional_dep($condition, $depstr);
				if ($depline) {
					$deplines{$depline}++;
				}
			}
			my $depline = join(" ", sort keys %deplines);
			if ($depline) {
				$line .= "\$(curdir)/".$pkg->{subdir}."$pkg->{src}/$type/compile += $depline\n";
			}
		}

		my $hasdeps = 0;
		my %deplines;
		foreach my $deps (@srcdeps) {
			my $idx;
			my $condition;
			my $prefix = "";
			my $suffix = "";

			if ($deps =~ /^(.+):(.+)/) {
				$condition = $1;
				$deps = $2;
			}
			if ($deps =~ /^(.+)(\/.+)/) {
				$deps = $1;
				$suffix = $2;
			}

			my $pkg_dep = $package{$deps};
			my @deps;

			if ($pkg_dep->{vdepends}) {
				@deps = @{$pkg_dep->{vdepends}};
			} else {
				@deps = ($deps);
			}

			foreach my $dep (@deps) {
				$pkg_dep = $package{$deps};
				if (defined $pkg_dep->{src}) {
					($pkg->{src} ne $pkg_dep->{src}.$suffix) and $idx = $pkg_dep->{subdir}.$pkg_dep->{src};
				} elsif (defined($srcpackage{$dep})) {
					$idx = $subdir{$dep}.$dep;
				}
				undef $idx if $idx eq 'base-files';
				if ($idx) {
					$idx .= $suffix;

					my $depline;
					next if $pkg->{src} eq $pkg_dep->{src}.$suffix;
					next if $dep{$condition.":".$pkg->{src}."->".$idx};
					next if $dep{$pkg->{src}."->($dep)".$idx} and $pkg_dep->{vdepends};
					my $depstr;

					if ($pkg_dep->{vdepends}) {
						$depstr = "\$(if \$(CONFIG_PACKAGE_$dep),\$(curdir)/$idx/compile)";
						$dep{$pkg->{src}."->($dep)".$idx} = 1;
					} else {
						$depstr = "\$(curdir)/$idx/compile";
						$dep{$pkg->{src}."->".$idx} = 1;
					}
					$depline = get_conditional_dep($condition, $depstr);
					if ($depline) {
						$deplines{$depline}++;
					}
				}
			}
		}
		my $depline = join(" ", sort keys %deplines);
		if ($depline) {
			$line .= "\$(curdir)/".$pkg->{subdir}."$pkg->{src}/compile += $depline\n";
		}
	}

	if ($line ne "") {
		print "\n$line";
	}
	foreach my $preconfig (keys %preconfig) {
		my $cmds;
		foreach my $cfg (keys %{$preconfig{$preconfig}}) {
			my $conf = $preconfig{$preconfig}->{$cfg}->{id};
			$conf =~ tr/\.-/__/;
			$cmds .= "\techo \"uci set '$preconfig{$preconfig}->{$cfg}->{id}=\$(subst \",,\$(CONFIG_UCI_PRECONFIG_$conf))'\"; \\\n";
		}
		next unless $cmds;
		print <<EOF

ifndef DUMP_TARGET_DB
\$(TARGET_DIR)/etc/uci-defaults/$preconfig: FORCE
	( \\
$cmds \\
	) > \$@
	
ifneq (\$(IMAGEOPT)\$(CONFIG_IMAGEOPT),)
  package/preconfig: \$(TARGET_DIR)/etc/uci-defaults/$preconfig
endif
endif

EOF
	}
}

sub gen_package_source() {
	parse_package_metadata($ARGV[0]) or exit 1;
	foreach my $name (sort {uc($a) cmp uc($b)} keys %package) {
		my $pkg = $package{$name};
		if ($pkg->{name} && $pkg->{source}) {
			print "$pkg->{name}: ";
			print "$pkg->{source}\n";
		}
	}
}

sub gen_package_subdirs() {
	parse_package_metadata($ARGV[0]) or exit 1;
	foreach my $name (sort {uc($a) cmp uc($b)} keys %package) {
		my $pkg = $package{$name};
		if ($pkg->{name} && $pkg->{repository}) {
			print "Package/$name/subdir = $pkg->{repository}\n";
		}
	}
}

sub gen_package_license($) {
	my $level = shift;
	parse_package_metadata($ARGV[0]) or exit 1;
	foreach my $name (sort {uc($a) cmp uc($b)} keys %package) {
		my $pkg = $package{$name};
		if ($pkg->{name}) {
			if ($pkg->{license}) {
				print "$pkg->{name}: ";
				print "$pkg->{license}\n";
				if ($pkg->{licensefiles} && $level == 0) {
					print "\tFiles: $pkg->{licensefiles}\n";
				}
			} else {
				if ($level == 1) {
					print "$pkg->{name}: Missing license! ";
					print "Please fix $pkg->{makefile}\n";
				}
			}
		}
	}
}

sub gen_version_filtered_list() {
	foreach my $item (version_filter_list(@ARGV)) {
		print "$item\n";
	}
}

sub gen_profile_mk() {
	my $file = shift @ARGV;
	my $target = shift @ARGV;
	my @targets = parse_target_metadata($file);
	foreach my $cur (@targets) {
		next unless $cur->{id} eq $target;
		print "PROFILE_NAMES = ".join(" ", map { $_->{id} } @{$cur->{profiles}})."\n";
		foreach my $profile (@{$cur->{profiles}}) {
			print $profile->{id}.'_NAME:='.$profile->{name}."\n";
			print $profile->{id}.'_PACKAGES:='.join(' ', @{$profile->{packages}})."\n";
		}
	}
}

sub parse_command() {
	GetOptions("ignore=s", \@ignore);
	my $cmd = shift @ARGV;
	for ($cmd) {
		/^target_config$/ and return gen_target_config();
		/^profile_mk$/ and return gen_profile_mk();
		/^package_mk$/ and return gen_package_mk();
		/^package_config$/ and return gen_package_config();
		/^kconfig/ and return gen_kconfig_overrides();
		/^package_source$/ and return gen_package_source();
		/^package_subdirs$/ and return gen_package_subdirs();
		/^package_license$/ and return gen_package_license(0);
		/^package_licensefull$/ and return gen_package_license(1);
		/^version_filter$/ and return gen_version_filtered_list();
	}
	print <<EOF
Available Commands:
	$0 target_config [file] 		Target metadata in Kconfig format
	$0 profile_mk [file] [target]		Profile metadata in makefile format
	$0 package_mk [file]			Package metadata in makefile format
	$0 package_config [file] 		Package metadata in Kconfig format
	$0 kconfig [file] [config] [patchver]	Kernel config overrides
	$0 package_source [file] 		Package source file information
	$0 package_subdirs [file]		Package subdir information in makefile format
	$0 package_license [file] 		Package license information
	$0 package_licensefull [file] 		Package license information (full list)
	$0 version_filter [patchver] [list...]	Filter list of version tagged strings

Options:
	--ignore <name>				Ignore the source package <name>
EOF
}

parse_command();
