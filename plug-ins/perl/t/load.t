use Test;

BEGIN {
  plan tests => 2;
}

END {
  ok(0) unless $loaded;
}

use Gimp qw(:consts);
$loaded = 1;
ok(1);

ok(SHARPEN,1);






