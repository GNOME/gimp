#
# This package is loaded by the Gimp, and is !private!, so don't
# use it standalone, it won't work.
#
package Gimp::Net;

use strict 'vars';
use vars qw(
   $VERSION
   $default_tcp_port $default_unix_dir $default_unix_sock
   $server_fh $trace_level $trace_res $auth $gimp_pid
);
use subs qw(gimp_call_procedure);
use base qw(DynaLoader);

use Socket; # IO::Socket is _really_ slow, so don't use it!

use Gimp ('croak','__');

require DynaLoader;

$VERSION = $Gimp::VERSION;

bootstrap Gimp::Net $VERSION;

$default_tcp_port  = 10009;
$default_unix_dir  = "/tmp/gimp-perl-serv-uid-$>/";
$default_unix_sock = "gimp-perl-serv";

$trace_res = *STDERR;
$trace_level = 0;

my $initialized = 0;

sub initialized { $initialized }

sub import {
   my $pkg = shift;

   return if @_;

   # overwrite some destroy functions
   *Gimp::Tile::DESTROY=
   *Gimp::PixelRgn::DESTROY=
   *Gimp::GDrawable::DESTROY=sub {
      my $req="DTRY".args2net(0,@_);
      print $server_fh pack("N",length($req)).$req;
   };
}

sub _gimp_procedure_available {
   my $req="TEST".$_[0];
   print $server_fh pack("N",length($req)).$req;
   read($server_fh,$req,1);
   return $req;
}

# this is hardcoded into gimp_call_procedure!
sub response {
   my($len,$req);
   read($server_fh,$len,4) == 4 or die "protocol error (1)";
   $len=unpack("N",$len);
   read($server_fh,$req,$len) == $len or die "protocol error (2)";
   net2args(0,$req);
}

# this is hardcoded into gimp_call_procedure!
sub command {
   my $req=shift;
   $req.=args2net(0,@_);
   print $server_fh pack("N",length($req)).$req;
}

sub gimp_call_procedure {
   my($len,@args,$trace,$req);
   
   if ($trace_level) {
      $req="TRCE".args2net(0,$trace_level,@_);
      print $server_fh pack("N",length($req)).$req;
      do {
         read($server_fh,$len,4) == 4 or die "protocol error (3)";
         $len=unpack("N",$len);
         read($server_fh,$req,abs($len)) == $len or die "protocol error (4)";
         if ($len<0) {
            ($req,@args)=net2args(0,$req);
            print "ignoring callback $req\n";
            redo;
         }
         ($trace,$req,@args)=net2args(0,$req);
         if (ref $trace_res eq "SCALAR") {
            $$trace_res = $trace;
         } else {
            print $trace_res $trace;
         }
      } while 0;
   } else {
      $req="EXEC".args2net(0,@_);
      print $server_fh pack("N",length($req)).$req;
      do {
         read($server_fh,$len,4) == 4 or die "protocol error (5)";
         $len=unpack("N",$len);
         read($server_fh,$req,abs($len)) == $len or die "protocol error (6)";
         if ($len<0) {
            ($req,@args)=net2args(0,$req);
            print "ignoring callback $req\n";
            redo;
         }
         ($req,@args)=net2args(0,$req);
      } while 0;
   }
   croak $req if $req;
   wantarray ? @args : $args[0];
}

sub server_quit {
   print $server_fh pack("N",4)."QUIT";
   undef $server_fh;
}

sub lock {
   print $server_fh pack("N",12)."LOCK".pack("N*",1,0);
}

sub unlock {
   print $server_fh pack("N",12)."LOCK".pack("N*",0,0);
}

sub set_trace {
   my($trace)=@_;
   my $old_level = $trace_level;
   if(ref $trace) {
      $trace_res=$trace;
   } elsif (defined $trace) {
      $trace_level=$trace;
   }
   $old_level;
}

sub start_server {
   my $opt = shift;
   $opt = $Gimp::spawn_opts unless $opt;
   print __"trying to start gimp with options \"$opt\"\n" if $Gimp::verbose;
   $server_fh=local *SERVER_FH;
   my $gimp_fh=local *CLIENT_FH;
   socketpair $server_fh,$gimp_fh,AF_UNIX,SOCK_STREAM,PF_UNSPEC
      or socketpair $server_fh,$gimp_fh,AF_LOCAL,SOCK_STREAM,PF_UNSPEC
      or croak __"unable to create socketpair for gimp communications: $!";

   # do it here so it i done only once
   require Gimp::Config;
   $gimp_pid = fork;
   if ($gimp_pid > 0) {
      Gimp::ignore_functions(@Gimp::gimp_gui_functions) unless $opt=~s/(^|:)gui//;
      return $server_fh;
   } elsif ($gimp_pid == 0) {
      close $server_fh;
      delete $ENV{GIMP_HOST};
      unless ($Gimp::verbose) {
         open STDIN,"</dev/null";
         open STDOUT,">/dev/null";
         open STDERR,">&1";
      }
      my @args;
      my $args = &Gimp::RUN_NONINTERACTIVE." ".
                 (&Gimp::_PS_FLAG_BATCH | &Gimp::_PS_FLAG_QUIET)." ".
                 fileno($gimp_fh);
      push(@args,"--no-data") if $opt=~s/(^|:)no-?data//;
      push(@args,"-n") unless $opt=~s/(^|:)gui//;
      push(@args,"--verbose") if $Gimp::verbose;
      { # block to suppress warning with broken perls (e.g. 5.004)
         exec $Gimp::Config{GIMP},
              "--no-splash",
              @args,
              "-b",
              "(extension-perl-server $args)",
              "(gimp_quit 0)",
              "(gimp-quit 0)";
      }
      exit(255);
   } else {
      croak __"unable to fork: $!";
   }
}

sub try_connect {
   local $_=$_[0];
   my $fh;
   $auth = s/^(.*)\@// ? $1 : "";	# get authorization
   if ($_ ne "") {
      if (s{^spawn/}{}) {
         return start_server($_);
      } elsif (s{^unix/}{/}) {
         my $server_fh=local *FH;
         return socket($server_fh,PF_UNIX,SOCK_STREAM,AF_UNIX)
                && connect($server_fh,sockaddr_un $_)
                ? $server_fh : ();
      } else {
         s{^tcp/}{};
         my($host,$port)=split /:/,$_;
         $port=$default_tcp_port unless $port;
         my $server_fh=local *FH;
         return socket($server_fh,PF_INET,SOCK_STREAM,scalar getprotobyname('tcp') || 6)
                && connect($server_fh,sockaddr_in $port,inet_aton $host)
                ? $server_fh : ();
      }
   } else {
      return $fh if $fh = try_connect ("$auth\@unix$default_unix_dir$default_unix_sock");
      return $fh if $fh = try_connect ("$auth\@tcp/127.1:$default_tcp_port");
      return $fh if $fh = try_connect ("$auth\@spawn/");
   }
   undef $auth;
}

sub gimp_init {
   $Gimp::in_top=1;
   if (@_) {
      $server_fh = try_connect ($_[0]);
   } elsif (defined($Gimp::host)) {
      $server_fh = try_connect ($Gimp::host);
   } elsif (defined($ENV{GIMP_HOST})) {
      $server_fh = try_connect ($ENV{GIMP_HOST});
   } else {
      $server_fh = try_connect ("");
   }
   defined $server_fh or croak __"could not connect to the gimp server (make sure Net-Server is running)";
   { my $fh = select $server_fh; $|=1; select $fh }
   
   my @r = response;
   
   die __"expected perl-server at other end of socket, got @r\n"
      unless $r[0] eq "PERL-SERVER";
   shift @r;
   die __"expected protocol version $Gimp::_PROT_VERSION, but server uses $r[0]\n"
      unless $r[0] eq $Gimp::_PROT_VERSION;
   shift @r;
   
   for(@r) {
      if($_ eq "AUTH") {
         die __"server requests authorization, but no authorization available\n"
            unless $auth;
         my $req = "AUTH".$auth;
         print $server_fh pack("N",length($req)).$req;
         my @r = response;
         die __"authorization failed: $r[1]\n" unless $r[0];
         print __"authorization ok, but: $r[1]\n" if $Gimp::verbose and $r[1];
      }
   }

   $initialized = 1;
   Gimp::_initialized_callback;
}

sub gimp_end {
   $initialized = 0;

   #close $server_fh if $server_fh;
   undef $server_fh;
   kill 'KILL',$gimp_pid if $gimp_pid;
   undef $gimp_pid;
}

sub gimp_main {
   gimp_init;
   no strict 'refs';
   $Gimp::in_top=0;
   eval { Gimp::callback("-net") };
   if($@ && $@ ne "IGNORE THIS MESSAGE\n") {
      Gimp::logger(message => substr($@,0,-1), fatal => 1, function => 'DIE');
      gimp_end;
      -1;
   } else {
      gimp_end;
      0;
   }
}

sub get_connection() {
   [$server_fh,$gimp_pid];
}

sub set_connection($) {
   ($server_fh,$gimp_pid)=@{+shift};
}

END {
   gimp_end;
}

1;
__END__

=head1 NAME

Gimp::Net - Communication module for the gimp-perl server.

=head1 SYNOPSIS

  use Gimp;

=head1 DESCRIPTION

For Gimp::Net (and thus commandline and remote scripts) to work, you first have to
install the "Perl-Server" extension somewhere where Gimp can find it (e.g in
your .gimp/plug-ins/ directory). Usually this is done automatically while installing
the Gimp extension. If you have a menu entry C<<Xtns>/Perl-Server>
then it is probably installed.

The Perl-Server can either be started from the C<<Xtns>> menu in Gimp, or automatically
when a perl script can't find a running Perl-Server.

When started from within The Gimp, the Perl-Server will create a unix
domain socket to which local clients can connect. If an authorization
password is given to the Perl-Server (by defining the environment variable
C<GIMP_HOST> before starting The Gimp), it will also listen on a tcp port
(default 10009). Since the password is transmitted in cleartext, using the
Perl-Server over tcp effectively B<lowers the security of your network to
the level of telnet>. Even worse: the current Gimp::Net-protocol can be
used for denial of service attacks, i.e. crashing the Perl-Server. There
also *might* be buffer-overflows (although I do care a lot for these).

=head1 ENVIRONMENT

The environment variable C<GIMP_HOST> specifies the default server to
contact and/or the password to use. The syntax is
[auth@][tcp/]hostname[:port] for tcp, [auth@]unix/local/socket/path for unix
and spawn/ for a private gimp instance. Examples are:

 www.yahoo.com               # just kidding ;)
 yahoo.com:11100             # non-standard port
 tcp/yahoo.com               # make sure it uses tcp
 authorize@tcp/yahoo.com:123 # full-fledged specification
 
 unix/tmp/unx                # use unix domain socket
 password@unix/tmp/test      # additionally use a password
 
 authorize@                  # specify authorization only
 
 spawn/                      # use a private gimp instance
 spawn/nodata                # pass --no-data switch
 spawn/gui                   # don't pass -n switch

=head1 CALLBACKS

=over 4

=item net()

is called after we have succesfully connected to the server. Do your dirty
work in this function, or see L<Gimp::Fu> for a better solution.

=back

=head1 FUNCTIONS

=over 4

=item server_quit()

sends the perl server a quit command.

=item get_connection()

return a connection id which uniquely identifies the current connection.

=item set_connection(conn_id)

set the connection to use on subsequent commands. C<conn_id> is the
connection id as returned by get_connection().

=back

=head1 BUGS

(Ver 0.04) This module is much faster than it ought to be... Silly that I wondered
wether I should implement it in perl or C, since perl is soo fast.

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
