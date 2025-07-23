#!/bin/sh

if [ $(whoami) == 'root' ]; then
  # Install snapcraft and complementary tools
  #apt update
  #apt install snapd
  
  snap install snapd
  snap install snapcraft --classic
  snap install lxd
  
  # Start lxd
  lxd init --auto
  usermod -a -G lxd user
else
  cd build/linux/snap

  # Enter in lxd group and build
  newgrp lxd | snapcraft --verbosity=trace --build-for=$(dpkg --print-architecture)
fi
