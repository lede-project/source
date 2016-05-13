package metadata;
use base 'Exporter';
use strict;
use warnings;
our @EXPORT = qw(%package %srcpackage %category %subdir %preconfig %features %overrides clear_packages parse_package_metadata parse_target_metadata get_multiline @ignore);

our %package;
our %preconfig;
our %srcpackage;
our %category;
our %subdir;
our %features;
our %overrides;
our @ignore;

sub get_multiline {
	my $fh = shift;
	my $prefix = shift;
	my $str;
	while (<$fh>) {
		last if /^@@/;
		$str .= (($_ and $prefix) ? $prefix . $_ : $_);
	}

	return $str ? $str : "";
}

sub confstr($) {
	my $conf = shift;
	$conf =~ tr#/\.\-/#___#;
	return $conf;
}

sub parse_target_metadata($) {
	my $file = shift;
	my ($target, @target, $profile);
	my %target;
	my $makefile;

	open FILE, "<$file" or do {
		warn "Can't open file '$file': $!\n";
		return;
	};
	while (<FILE>) {
		chomp;
		/^Source-Makefile: \s*((.+\/)([^\/]+)\/Makefile)\s*$/ and $makefile = $1;
		/^Target:\s*(.+)\s*$/ and do {
			my $name = $1;
			$target = {
				id => $name,
				board => $name,
				makefile => $makefile,
				boardconf => confstr($name),
				conf => confstr($name),
				profiles => [],
				features => [],
				depends => [],
				subtargets => []
			};
			push @target, $target;
			$target{$name} = $target;
			if ($name =~ /([^\/]+)\/([^\/]+)/) {
				push @{$target{$1}->{subtargets}}, $2;
				$target->{board} = $1;
				$target->{boardconf} = confstr($1);
				$target->{subtarget} = 1;
				$target->{parent} = $target{$1};
			}
		};
		/^Target-Name:\s*(.+)\s*$/ and $target->{name} = $1;
		/^Target-Arch:\s*(.+)\s*$/ and $target->{arch} = $1;
		/^Target-Arch-Packages:\s*(.+)\s*$/ and $target->{arch_packages} = $1;
		/^Target-Features:\s*(.+)\s*$/ and $target->{features} = [ split(/\s+/, $1) ];
		/^Target-Depends:\s*(.+)\s*$/ and $target->{depends} = [ split(/\s+/, $1) ];
		/^Target-Description:/ and $target->{desc} = get_multiline(*FILE);
		/^Target-Optimization:\s*(.+)\s*$/ and $target->{cflags} = $1;
		/^CPU-Type:\s*(.+)\s*$/ and $target->{cputype} = $1;
		/^Linux-Version:\s*(.+)\s*$/ and $target->{version} = $1;
		/^Linux-Release:\s*(.+)\s*$/ and $target->{release} = $1;
		/^Linux-Kernel-Arch:\s*(.+)\s*$/ and $target->{karch} = $1;
		/^Default-Subtarget:\s*(.+)\s*$/ and $target->{def_subtarget} = $1;
		/^Default-Packages:\s*(.+)\s*$/ and $target->{packages} = [ split(/\s+/, $1) ];
		/^Target-Profile:\s*(.+)\s*$/ and do {
			$profile = {
				id => $1,
				name => $1,
				priority => 999,
				packages => []
			};
			push @{$target->{profiles}}, $profile;
		};
		/^Target-Profile-Name:\s*(.+)\s*$/ and $profile->{name} = $1;
		/^Target-Profile-Priority:\s*(\d+)\s*$/ and do {
			$profile->{priority} = $1;
			$target->{sort} = 1;
		};
		/^Target-Profile-Packages:\s*(.*)\s*$/ and $profile->{packages} = [ split(/\s+/, $1) ];
		/^Target-Profile-Description:\s*(.*)\s*/ and $profile->{desc} = get_multiline(*FILE);
		/^Target-Profile-Config:/ and $profile->{config} = get_multiline(*FILE, "\t");
		/^Target-Profile-Kconfig:/ and $profile->{kconfig} = 1;
	}
	close FILE;
	foreach my $target (@target) {
		if (@{$target->{subtargets}} > 0) {
			$target->{profiles} = [];
			next;
		}
		@{$target->{profiles}} > 0 or $target->{profiles} = [
			{
				id => 'Default',
				name => 'Default',
				packages => []
			}
		];

		$target->{sort} and @{$target->{profiles}} = sort {
			$a->{priority} <=> $b->{priority} or
			$a->{name} cmp $b->{name};
		} @{$target->{profiles}};
	}
	return @target;
}

sub clear_packages() {
	%subdir = ();
	%preconfig = ();
	%package = ();
	%srcpackage = ();
	%category = ();
	%features = ();
	%overrides = ();
}

sub parse_package_metadata($) {
	my $file = shift;
	my $pkg;
	my $feature;
	my $makefile;
	my $preconfig;
	my $subdir;
	my $src;
	my $override;
	my %ignore = map { $_ => 1 } @ignore;

	open FILE, "<$file" or do {
		warn "Cannot open '$file': $!\n";
		return undef;
	};
	while (<FILE>) {
		chomp;
		/^Source-Makefile: \s*((.+\/)([^\/]+)\/Makefile)\s*$/ and do {
			$makefile = $1;
			$subdir = $2;
			$src = $3;
			$subdir =~ s/^package\///;
			$subdir{$src} = $subdir;
			$srcpackage{$src} = [];
			$override = "";
			undef $pkg;
		};
		/^Override: \s*(.+?)\s*$/ and do {
			$override = $1;
			$overrides{$src} = 1;
		};
		next unless $src;
		next if $ignore{$src};
		/^Package:\s*(.+?)\s*$/ and do {
			undef $feature;
			$pkg = {};
			$pkg->{src} = $src;
			$pkg->{makefile} = $makefile;
			$pkg->{name} = $1;
			$pkg->{title} = "";
			$pkg->{depends} = [];
			$pkg->{mdepends} = [];
			$pkg->{builddepends} = [];
			$pkg->{buildtypes} = [];
			$pkg->{subdir} = $subdir;
			$pkg->{tristate} = 1;
			$pkg->{override} = $override;
			$package{$1} = $pkg;
			push @{$srcpackage{$src}}, $pkg;
		};
		/^Feature:\s*(.+?)\s*$/ and do {
			undef $pkg;
			$feature = {};
			$feature->{name} = $1;
			$feature->{priority} = 0;
		};
		$feature and do {
			/^Target-Name:\s*(.+?)\s*$/ and do {
				$features{$1} or $features{$1} = [];
				push @{$features{$1}}, $feature;
			};
			/^Target-Title:\s*(.+?)\s*$/ and $feature->{target_title} = $1;
			/^Feature-Priority:\s*(\d+)\s*$/ and $feature->{priority} = $1;
			/^Feature-Name:\s*(.+?)\s*$/ and $feature->{title} = $1;
			/^Feature-Description:/ and $feature->{description} = get_multiline(\*FILE, "\t\t\t");
			next;
		};
		next unless $pkg;
		/^Version: \s*(.+)\s*$/ and $pkg->{version} = $1;
		/^Title: \s*(.+)\s*$/ and $pkg->{title} = $1;
		/^Menu: \s*(.+)\s*$/ and $pkg->{menu} = $1;
		/^Submenu: \s*(.+)\s*$/ and $pkg->{submenu} = $1;
		/^Submenu-Depends: \s*(.+)\s*$/ and $pkg->{submenudep} = $1;
		/^Source: \s*(.+)\s*$/ and $pkg->{source} = $1;
		/^License: \s*(.+)\s*$/ and $pkg->{license} = $1;
		/^LicenseFiles: \s*(.+)\s*$/ and $pkg->{licensefiles} = $1;
		/^Default: \s*(.+)\s*$/ and $pkg->{default} = $1;
		/^Provides: \s*(.+)\s*$/ and do {
			my @vpkg = split /\s+/, $1;
			foreach my $vpkg (@vpkg) {
				$package{$vpkg} or $package{$vpkg} = {
					name => $vpkg,
					vdepends => [],
					src => $src,
					subdir => $subdir,
					makefile => $makefile
				};
				push @{$package{$vpkg}->{vdepends}}, $pkg->{name};
			}
		};
		/^Menu-Depends: \s*(.+)\s*$/ and $pkg->{mdepends} = [ split /\s+/, $1 ];
		/^Depends: \s*(.+)\s*$/ and $pkg->{depends} = [ split /\s+/, $1 ];
		/^Conflicts: \s*(.+)\s*$/ and $pkg->{conflicts} = [ split /\s+/, $1 ];
		/^Hidden: \s*(.+)\s*$/ and $pkg->{hidden} = 1;
		/^Build-Variant: \s*([\w\-]+)\s*/ and $pkg->{variant} = $1;
		/^Default-Variant: .*/ and $pkg->{variant_default} = 1;
		/^Build-Only: \s*(.+)\s*$/ and $pkg->{buildonly} = 1;
		/^Build-Depends: \s*(.+)\s*$/ and $pkg->{builddepends} = [ split /\s+/, $1 ];
		/^Build-Depends\/(\w+): \s*(.+)\s*$/ and $pkg->{"builddepends/$1"} = [ split /\s+/, $2 ];
		/^Build-Types:\s*(.+)\s*$/ and $pkg->{buildtypes} = [ split /\s+/, $1 ];
		/^Repository:\s*(.+?)\s*$/ and $pkg->{repository} = $1;
		/^Category: \s*(.+)\s*$/ and do {
			$pkg->{category} = $1;
			defined $category{$1} or $category{$1} = {};
			defined $category{$1}->{$src} or $category{$1}->{$src} = [];
			push @{$category{$1}->{$src}}, $pkg;
		};
		/^Description: \s*(.*)\s*$/ and $pkg->{description} = "\t\t $1\n". get_multiline(*FILE, "\t\t ");
		/^Type: \s*(.+)\s*$/ and do {
			$pkg->{type} = [ split /\s+/, $1 ];
			undef $pkg->{tristate};
			foreach my $type (@{$pkg->{type}}) {
				$type =~ /ipkg/ and $pkg->{tristate} = 1;
			}
		};
		/^Config:\s*(.*)\s*$/ and $pkg->{config} = "$1\n".get_multiline(*FILE, "\t");
		/^Prereq-Check:/ and $pkg->{prereq} = 1;
		/^Preconfig:\s*(.+)\s*$/ and do {
			my $pkgname = $pkg->{name};
			$preconfig{$pkgname} or $preconfig{$pkgname} = {};
			if (exists $preconfig{$pkgname}->{$1}) {
				$preconfig = $preconfig{$pkgname}->{$1};
			} else {
				$preconfig = {
					id => $1
				};
				$preconfig{$pkgname}->{$1} = $preconfig;
			}
		};
		/^Preconfig-Type:\s*(.*?)\s*$/ and $preconfig->{type} = $1;
		/^Preconfig-Label:\s*(.*?)\s*$/ and $preconfig->{label} = $1;
		/^Preconfig-Default:\s*(.*?)\s*$/ and $preconfig->{default} = $1;
	}
	close FILE;
	return 1;
}

1;
