#!/bin/sh

### sign packages with a custom key

if [ "$1" == "" ]; then
	echo "ERROR: expecting key file!"
	exit 0
fi
keyFile="$1"

generateSignature () {
	echo "Generating new signature for packages in $1"
	# remove the local signature file
	rm -rf $1/Packages.sig

	# generate new signature
	./staging_dir/host/bin/usign -S -m $1/Packages -s $2
}

dirs="bin/targets/ramips/mt76x8/packages bin/packages/mipsel_24kc/base bin/packages/mipsel_24kc/packages bin/packages/mipsel_24kc/onion bin/packages/mipsel_24kc/routing"

for dir in $dirs;
do
	generateSignature "$dir" "$keyFile"
done

