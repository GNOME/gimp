#!/usr/bin/perl

# example for the gimp-perl-server (also called Net-Server)

use Gimp;

sub query {
   print STDERR "$0: this script is not intended to be run from within the gimp!\n";
}

sub net {
  # simple benchmark ;)

  $img=new Gimp::Image(600,300,RGB);
  # the is the same as $img = new Image(600,300,RGB)

  $bg=$img->layer_new(30,20,RGB_IMAGE,"Background",100,NORMAL_MODE);

  $bg->add_layer(1);

  new Gimp::Display($img);

  for $i (0..255) {
     Palette->set_background([$i,255-$i,$i]);
     $bg->edit_fill;
     Display->displays_flush ();
  }

#  Gimp::Net::server_quit;  # kill the gimp-perl-server-extension (ugly name)
}

exit main;

