#!/usr/bin/perl

# this extension shows some oo-like calls

# it's really easy

use Gimp;

# the extension that's called.
sub plug_in_example_oo {
  my $img=new Image(300,200,RGB);

  my $bg=new Layer($img,300,200,RGB_IMAGE,"Background",100,NORMAL_MODE);

  Palette->set_background([200,200,100]);

  $bg->fill(BG_IMAGE_FILL);
#  Palette->set_background([200,100,200]);
#  gimp_drawable_fill ($bg,BG_IMAGE_FILL);
  $img->add_layer($bg,1);

  new Display($img);
}

sub net {
  plug_in_example_oo;
}

sub query {
  gimp_install_procedure("plug_in_example_oo", "a test plug-in in perl",
                         "try it out", "Marc Lehmann", "Marc Lehmann", "1998-04-27",
                         N_"<Toolbox>/Xtns/Perl Example Plug-in", "*", PROC_EXTENSION,
                         [[PARAM_INT32, "run_mode", "Interactive, [non-interactive]"]], []);
}

exit main;

