use Test;

BEGIN {
  plan tests => 2;
}

END {
  ok(0) unless $loaded;
}

use Gimp qw(:consts);
use Gimp::Lib;
$loaded = 1;
ok(1);

ok(SHARPEN,1);






