use Test;
use vars qw($EXTENSIVE_TESTS $GIMPTOOL);

# the most complicated thing is to set up a working gimp environment. its
# difficult at best...

BEGIN {
  plan tests => 17;
}

END {
  if($loaded and $dir) {
    system("rm","-rf",$dir); #d#FIXME
  } else {
    ok(0);
  }
}

use Cwd;

$dir=cwd."/test-dir";

do './config.pl';
ok(1);

$n=!$EXTENSIVE_TESTS;

skip($n,sub {($plugins = `$GIMPTOOL -n --install-admin-bin /bin/sh`) =~ s{^.*\s(.*?)(?:/+bin/sh)\r?\n?$}{$1}});
skip($n,sub {-d $plugins});
skip($n,sub {-x "$plugins/script-fu"});

use Gimp qw(:consts);
$loaded = 1;
ok(1);

sub net {
   my($i,$l);
   skip($n,sub{$i=new Image(300,300,RGB)});
   skip($n,ref $i);
   skip($n,sub{$l=$i->layer_new(300,300,RGBA_IMAGE,"new layer",100,VALUE_MODE)});
   skip($n,ref $l);
   skip($n,sub{$i->add_layer($l,0)||1});
   skip($n,sub{$l->get_name()},"new layer");
   skip($n,sub{$i->delete || 1});
}

system("rm","-rf",$dir); #d#FIXME
ok(sub {mkdir $dir,0700});
ok(sub {symlink "../Perl-Server","$dir/Perl-Server"});
skip($n,sub {symlink "$plugins/script-fu","$dir/script-fu"});

ok (
  open RC,">$dir/gimprc" and
  print RC "(show-tips no)\n" and
  print RC "(plug-in-path \"$dir\")\n" and
  close RC
);

$ENV{'GIMP_DIRECTORY'}=$dir;
$Gimp::host = "spawn/";

if(!$n) {
   skip($n,1);
   Gimp->main;
} else {
   skip($n,0);
   net();
}







