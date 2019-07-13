#! /bin/sh

echo "Preparing package feeds..."
./scripts/feeds update -a
./scripts/feeds install -a
#./scripts/feeds install -a -p onion

# take care of Onion customized packages
./scripts/feeds uninstall bluez-examples bluez-libs bluez-utils pulseaudio-daemon pulseaudio-profiles pulseaudio-tools avrdude
./scripts/feeds install -p onion bluez-examples bluez-libs bluez-utils pulseaudio-daemon pulseaudio-profiles pulseaudio-tools avrdude

