use Config;
use vars qw($EXTENSIVE_TESTS $GIMPTOOL);

# the most complicated thing is to set up a working gimp environment. its
# difficult at best...

BEGIN {
  $|=1;

  if ($ENV{DISPLAY}) {
     print "1..26\n";
     $count=0;
     $Gimp::host = "spawn/";
  } else {
     print "1..0\n";
     exit;
  }
}

sub ok($;$) {
   print((@_==1 ? shift : $_[0] eq &{$_[1]}) ?
         "ok " : "not ok ", ++$count, "\n");
}

sub skip($$;$) {
   shift() ? print "ok ",++$count," # skip\n" : &ok;
}

END {
  system("rm","-rf",$dir);#d##FIXME#
}

use Cwd;

$dir=cwd."/test-dir";

do './config.pl';
ok(1);

$n=!$EXTENSIVE_TESTS;

skip($n,1,sub {($plugins = `$GIMPTOOL -n --install-admin-bin /bin/sh`) =~ s{^.*\s(.*?)(?:/+bin/sh)\r?\n?$}{$1}});
skip($n,1,sub {-d $plugins});
skip($n,1,sub {-x "$plugins/script-fu"});

use Gimp;
ok(1);

ok(RGBA_IMAGE || RGB_IMAGE);
ok(RGB_IMAGE ? 1 : 1); #  check for correct prototype

sub tests {
   my($i,$l);
   skip($n,1,sub{0 != ($i=new Image(10,10,RGB))});
   skip($n,1,sub {!!ref $i});
   skip($n,1,sub{0 != ($l=$i->layer_new(10,10,RGBA_IMAGE,"new layer",100,VALUE_MODE))});
   skip($n,1,sub {!!ref $l});
   
   skip($n,1,sub{Gimp->image_add_layer($l,0) || 1});
   skip($n,"new layer",sub{$l->get_name()});
   
   skip($n,1,sub{$l->paintbrush(50,[1,1,2,2,5,3,7,4,2,8],CONTINUOUS,0) || 1});
   skip($n,1,sub{$l->paintbrush(30,4,[5,5,8,1],CONTINUOUS,0) || 1});
   
   skip($n,1,sub{Plugin->sharpen(RUN_NONINTERACTIVE,$i,$l,10) || 1});
   skip($n,1,sub{$l->sharpen(10) || 1});
   skip($n,1,sub{Gimp->plug_in_sharpen($i,$l,10) || 1});
   
   skip($n,1,sub{$i->delete || 1});
}

system("rm","-rf",$dir); #d#FIXME
ok(1,sub {mkdir $dir,0700});

# copy the Perl-Server
{
   local(*X,*Y,$/);
   open X,"<Perl-Server" or die "unable to read the Perl-Server";
   my $s = <X>;
   open Y,">$dir/Perl-Server.pl" or die "unable to write the Perl-Server";
   print Y $Config{startperl},"\n",$s,<X>;
   ok(1);
}
ok(1,sub { chmod 0700,"$dir/Perl-Server.pl" });

skip($n,1,sub {symlink "$plugins/script-fu","$dir/script-fu"});
skip($n,1,sub {symlink "$plugins/sharpen","$dir/sharpen"});

ok (
  open RC,">$dir/gimprc" and
  print RC "(show-tips no)\n" and
  print RC "(gimp_data_dir \"\")\n" and
  print RC "(script-fu-path \"\")\n" and
  print RC "(plug-in-path \"$dir\")\n" and
  close RC
);

$ENV{GIMP_DIRECTORY}=$dir;
$ENV{PERL5LIB}=cwd."/blib/lib:".cwd."/blib/arch";

if(!$n) {
   skip($n,1);
   Gimp::init;
   tests;
} else {
   skip($n,0);
   tests;
}







