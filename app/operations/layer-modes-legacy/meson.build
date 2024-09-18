# We do not test algorithm correctness but algorithm stability: an old
# XCF should look the same when loaded with a newer GIMP.
#
# Current hashes were made based on renders in GIMP 2.10. To check how
# they were made, look at the script tools/compute-layer-mode-digests.py
# in `gimp-2-10` branch.
#
# In other words: DO NOT just change a hash to make the test pass. If
# one of the test starts failing, this is very bad as it will break
# renders of existing XCF files.
# If bugs need to be fixed in one of the blend modes, they need to be
# fixed for further versions while still loading XCF done with older
# versions of GIMP the same way as they used to.
#
# When adding a new mode, here is how to add a new hash:
#
# 1. First add it without any hash and run the test.
# 2. The unit testing will fail telling you which keys are needed (it
#    can be keys like "auto:auto:auto" for non-mutable modes, or
#    "linear:union:perceptual", etc. for every possible case). Add this
#    key with an empty hash.
# 3. Run the test again.
# 4. The test again will fail again, this time telling you the found
#    hash. If you consider your implementation to be correct, this hash
#    is what you want to store in this dictionnary.
#
# If somehow, you want the hash to come from an older implementation,
# you can do this:
#
# 1. In the second test run, the test will generate a XCF on failure
#    (and will display its path in the test log).
# 2. Load this XCF with your reference implementations. E.g. initially
#    I loaded them with latest GIMP 2.10.
# 3. Export as data in "Contiguous" configuration.
# 4. Call sha256sum on the data file.
# 5. The returned checksum is your reference.
legacy_layer_modes = {
  'addition-legacy': {
    'auto:auto:auto': [ '5294fa2036be51d218a6c980cb9cff149c9b3f6934d1de1fcc2cd0785ce29a05' ]
  },
  'burn-legacy': {
    'auto:auto:auto': [ 'ae23f7a31fd31f197b89d03d1018e970baae32517a6f95f8772641fbe6d8b041' ]
  },
  'darken-only-legacy': {
    'auto:auto:auto': [ 'ac27e33b4832709fb39aa7631bd035683a6bbf32a275e325f144093b04fcc5e7' ]
  },
  'difference-legacy': {
    'auto:auto:auto': [ '4feed6e676a3f4d83bcf5457bdc8cdf804b1996c36e38ebaeff014b1e197bf3f',
                        '55a1b0ef78c91a6b78e652433d5bca3178f2d594197ca2531355bac8f0ffbc52' ]
  },
  'divide-legacy': {
    'auto:auto:auto': [ 'd946379074f86378088e35524f956a1c4280d81ac6081e546197229b49deb20c' ]
  },
  'dodge-legacy': {
    'auto:auto:auto': [ '45d34e25cf6dd194ce944bdc404091892b86c7e77b860250b1d1c41b4d0ad076' ]
  },
  'grain-extract-legacy': {
    'auto:auto:auto': [ '578a6f958eddb8af5a4d85dc71b85e27df411935ebd880021cb779e0c37daac1' ]
  },
  'grain-merge-legacy': {
    'auto:auto:auto': [ '5dc0fb0e4b7243e0bb917595c4146eb63282d402341d25dc136957df56eefd41' ]
  },
  'hardlight-legacy': {
    'auto:auto:auto': [ '956db3eeae96a9cddaddcbe5aa71787e6f546b0b1a5c298da04ec91b22c9ec5c' ]
  },
  'hsl-color-legacy': {
    'auto:auto:auto': [ '5126b7eebbdaa0315f0a94fb6184fef41a95cb79577d5b633c2d1a6dc5680661',
                        '31f182e50286b6088a819c6a81d70f145c0ca7307c15726515cfb6ac51efe2d5',
                        '70be349d74a1c21b446b67064dde5a620b4181df600ae64e10ecad30b95de1a8',
                        '877860c90b20a83c1a374cc811af60767d0ba78511ed0033f4d25ef9665e24c1' ]
  },
  'hsv-hue-legacy': {
    'auto:auto:auto': [ '07f188536bf2e2da2341424e8a9c54156cfce11af419f806a5d0c63e0c84c8d0',
                        'f395f8eec0248abd3e541aed569d73504c7ea569134753cfc4743594ca8ec83b',
                        '157ccba9bc9091702574aef4300db460d0a998b67f58cbb56175ad1cbe04c0e3' ]
  },
  'hsv-saturation-legacy': {
    'auto:auto:auto': [ '2192d50ed4a2eeb07400c44144e54dc027dcba6e90a86dd5c84015f7885a3eab',
                        '466465f04dcb260626b8ba7734578a262365a13c6e99af436d81ec8385e0db02',
                        '1e28b43fa8ac650a2f78d7d664512b3e446d73569c04d2626a143cf4130d2e61' ]
  },
  'hsv-value-legacy': {
    'auto:auto:auto': [ 'a2e2be54ef11400fb8a48657a939fa36ef634b5eef103fe3c1fa498bf60207d9',
                        '778de64b0117ad23dc1976a830f6f10eb5e6f98b783afc6704c2add181eed59f' ]
  },
  'lighten-only-legacy': {
    'auto:auto:auto': [ '77b6589a27083b4ce8c70ccf83c9240e58f207df08e597fad2d05bfdcd2efa78' ]
  },
  'multiply-legacy': {
    'auto:auto:auto': [ 'ba64bd7e41883302d47b2c2af63efcce47ff478f2c69e17119955bba7e345bd5' ]
  },
  'screen-legacy': {
    'auto:auto:auto': [ 'e36091c1d5ecf0ed4f299087755cd7154551cc66d975898ee15acb98248e7800' ]
  },
  'softlight-legacy': {
    'auto:auto:auto': [ '0e2dd784a05cfe23bfcb01098d9a1145ec2163a36b1435bddc58e04a071e3c51' ]
  },
  'subtract-legacy': {
    'auto:auto:auto': [ 'cb7ebdd57b6648bf9dafdc927f162ad40917b15fee7e608a4630806dffa00d24',
                        '695359aea6a932a4db11bf44d679763120a975de8b7acdc023969e97ba3f63be' ]
  },
}

libapplayermodeslegacy_sources = []
foreach mode, _ : legacy_layer_modes
  libapplayermodeslegacy_sources += [ 'gimpoperation' + mode.replace('-', '') + '.c' ]
endforeach

libapplayermodeslegacy = static_library('applayermodeslegacy',
  libapplayermodeslegacy_sources + [ 'gimpcolor-legacy.c' ],
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Layer-Modes-Legacy"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)

# These modes do not make additional source includes and are only listed
# for unit testing.
dup_legacy_layer_modes = {
  # "Normal (legacy)" is in fact the same as "Normal" in "auto:union:perceptual".
  # I still test it separately in order not to miss any bug.
  'normal-legacy': {
    'auto:auto:auto': [ '298ded67bad7508caf29eb2718712ecbfece87592e6e4b006e460a03070a0ebf' ]
  },
  # A.k.a. "Old broken Overlay": this is the same as softlight-legacy and is not visible in the GUI.
  'overlay-legacy': {
    'auto:auto:auto': [ '0e2dd784a05cfe23bfcb01098d9a1145ec2163a36b1435bddc58e04a071e3c51' ]
  },
}
