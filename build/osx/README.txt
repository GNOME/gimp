How to build gimp 2.8.0 on Mac OSX.

The dollar sign precedes terminal commands.

Download and install jhbuild from this location:
http://git.gnome.org/browse/gtk-osx/plain/gtk-osx-build-setup.sh

$ curl -O http://git.gnome.org/browse/gtk-osx/plain/gtk-osx-build-setup.sh
$ sh gtk-osx-build-setup.sh

If you're not running Lion, you'll have to either download git in the form of a binary package, or compile it yourself. If you want complete control, I would suggest downloading it and installing it from source, in ~/.local/.

Add ~/.local/ to your $PATH env either via terminal command:

$ export $PATH=$HOME/.local:$PATH

or by adding that command to your ~/.profile.

Download and place jhbuildrc-gimp and gimp.modules into your home directory, and rename jhbuildrc-gimp as .jhbuildrc-gimp.

Now you can download and install gimp:

$ JHB=gimp GIMP_SDK=10.6 jhbuild bootstrap --ignore-system
$ JHB=gimp GIMP_SDK=10.6 jhbuild build meta-gimp

In order to create a .app you'll need to download an install gtk-mac-bundler.

$ cd ~/Source/
$ git clone https://github.com/jralls/gtk-mac-bundler.git
$ cd gtk-mac-bundler/
$ make install

And then enter the gimp/build/osx directory and create the app:

$ gtk-mac-bundler gimp.bundle

