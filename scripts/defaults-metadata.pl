#!/usr/bin/env perl
use FindBin;
use lib "$FindBin::Bin";
use strict;
use metadata;
use Getopt::Long;

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

sub gen_defaults_config() {
	my $file = shift @ARGV;
	parse_defaults_metadata($file);
	my %defaults = ();

	# Default packages for a profile (default n; becomes y when profile is enabled)
	foreach my $target (@defaultstargets) {
		foreach my $profile (@{$target->{profiles}}) {
			print "\tconfig DEFAULT_PACKAGES_TARGET_".$target->{conf}."_".$profile->{id}."\n";
			print "\t\tbool\n";
			print "\t\tdefault y\n";
			print "\t\tdepends on ( TARGET_".$target->{conf}."_".$profile->{id}." || TARGET_DEVICE_".$target->{conf}."_".$profile->{id}." )\n";
			my @pkglist = merge_package_lists($target->{packages}, $profile->{packages});
			foreach my $pkg (@pkglist) {
				print "\t\tselect DEFAULT_".$pkg."\n";
				$defaults{$pkg} = 1;
			}
			print "\n";
		}
	}

	# Default packages for a device type (on only if given device type is selected)
	foreach my $devicetypename (keys %devicetypes) {
		my $devicetype = $devicetypes{$devicetypename};
		foreach my $pkg (@{$devicetype->{packages}}) {
			print "\tconfig DEFAULT_".${devicetype}->{conf}."_".$pkg."\n";
			print "\t\tbool\n";
			print "\t\tdefault y\n";
			print "\t\tdepends on ".${devicetype}->{conf}."\n";
			print "\t\tselect DEFAULT_".$pkg."\n\n";
			$defaults{$pkg} = 1;
		}
	}

	# Default packages for all builds
	foreach my $pkg (@{$defaultpackages}) {
		print "\tconfig DEFAULT_".$pkg."\n";
		print "\t\tbool\n";
		print "\t\tdefault y\n\n";
		delete($defaults{pkg});
	}

	# Create the config KConfig option but don't set it to y
	# This lets us create a list of all possible defaults and use
	# select in profiles or device type config to actually set
	# the package as default on.
	foreach my $pkg (keys %defaults) {
		print "\tconfig DEFAULT_".$pkg."\n";
		print "\t\tbool\n";
	}
}

sub gen_defaults_mk() {
	my $file = shift @ARGV;
	my $target = shift @ARGV;
	parse_defaults_metadata($file);
	foreach my $cur (@defaultstargets) {
		next unless $cur->{id} eq $target;
		foreach my $profile (@{$cur->{profiles}}) {
			print $profile->{id}.'_PACKAGES:='.join(' ', @{$profile->{packages}})."\n";
		}
	}
}

sub parse_command() {
	GetOptions("ignore=s", \@ignore);
	my $cmd = shift @ARGV;
	for ($cmd) {
		/^config$/ and return gen_defaults_config();
		/^defaults_mk$/ and return gen_defaults_mk();
	}
	die <<EOF
Available Commands:
	$0 config [file] 			Target metadata in Kconfig format
	$0 profile_mk [file] [target]		Profile metadata in makefile format

EOF
}

parse_command();
