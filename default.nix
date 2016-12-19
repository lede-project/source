# Nix is a powerful package manager for Linux and other Unix systems that makes
# package management reliable and reproducible: https://nixos.org/nix/.
# This file is intended to be used with `nix-shell`
# (https://nixos.org/nix/manual/#sec-nix-shell) to setup a fully-functional
# LEDE build environment by installing all required dependencies.
with import <nixpkgs> {};
stdenv.mkDerivation {
  name = "openwrt-dev-env";
  buildInputs = [
    # This list is more explicit then it have to be: it also includes utils
    # from stdenv. However including every dependencies
    # mentioned in include/prereq-build.mk makes future updates easier.
    gnumake
    gcc
    ncurses
    zlibStatic
    zlibStatic.static
    openssl
    perlPackages.ThreadQueue
    gnutar
    findutils
    bash
    patchutils
    diffutils
    coreutils # captures cp, md5sum, stat
    gawk
    gnugrep
  ] ++ [(if (stdenv.isDarwin) then getopt else utillinux)] ++ [
    unzip
    bzip2
    wget
    perl
    python2
    git
    file
    # additional dependencies from https://lede-project.org/docs/guide-developer/install-buildsystem
    asciidoc
    bc
    binutils
    fastjar
    flex
    intltool
    jikespg
    cdrkit
    perlPackages.ExtUtilsMakeMaker
    rsync
    ruby
    sdcc
    gettext
    libxslt
  ];
  NIX_LDFLAGS="-lncurses";
  hardeningDisable = "all";
}
