$|=1;
print "1..1\n";

# trick Gimp into using the Gimp::Lib-interface.
BEGIN { @ARGV = '-gimp' }

use Gimp qw(:consts);
print "ok 1\n";






