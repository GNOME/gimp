#
# This package is loaded by the Gimp, and is !private!, so don't
# use it standalone, it won't work.
#
package Gimp::Net;

use strict 'vars';
use Carp;
use vars qw(
   $VERSION
   $default_tcp_port $default_unix_dir $default_unix_sock
   $server_fh $trace_level $trace_res $auth $gimp_pid
);
use subs qw(gimp_call_procedure);
use Socket; # IO::Socket is _really_ slow

$default_tcp_port  = 10009;
$default_unix_dir  = "/tmp/gimp-perl-serv-uid-$>/";
$default_unix_sock = "gimp-perl-serv";

$trace_res = *STDERR;
$trace_level = 0;

my $initialized = 0;
my $new_handle = "HANDLE0000";

sub initialized { $initialized }

sub import {
   my $pkg = shift;

   return if @_;

   *Gimp::Tile::DESTROY=
   *Gimp::PixelRgn::DESTROY=
   *Gimp::GDrawable::DESTROY=sub {
      my $req="DTRY".args2net(@_);
      print $server_fh pack("N",length($req)).$req;
   };
}

# network to array
sub net2args($) {
   no strict 'subs';
   sub b($$) { bless \(my $x=$_[0]),$_[1] }
   eval "($_[0])";
}

sub args2net {
   my($res,$v);
   for $v (@_) {
      if(ref($v) eq "ARRAY" or ref($v) eq "Gimp::Color" or ref($v) eq "Gimp::Parasite") {
         $res.="[".join(",",map { "qq[".quotemeta($_)."]" } @$v)."],";
      } elsif(ref($v)) {
         $res.="b(".$$v.",".ref($v)."),";
      } elsif(defined $v) {
         $res.="qq[".quotemeta($v)."],";
      } else {
         $res.="undef,";
      }
   }
   substr($res,0,-1); # may not be worth the effort
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
   read($server_fh,$len,4) == 4 or die "protocol error";
   $len=unpack("N",$len);
   read($server_fh,$req,$len) == $len or die "protocol error";
   net2args($req);
}

# this is hardcoded into gimp_call_procedure!
sub command {
   my $req=shift;
   $req.=args2net(@_);
   print $server_fh pack("N",length($req)).$req;
}

sub gimp_call_procedure {
   my($len,@args,$trace,$req);
   
   if ($trace_level) {
      $req="TRCE".args2net($trace_level,@_);
      print $server_fh pack("N",length($req)).$req;
      read($server_fh,$len,4) == 4 or die "protocol error";
      $len=unpack("N",$len);
      read($server_fh,$req,$len) == $len or die "protocol error";
      ($trace,$req,@args)=net2args($req);
      if (ref $trace_res eq "SCALAR") {
         $$trace_res = $trace;
      } else {
         print $trace_res $trace;
      }
   } else {
      $req="EXEC".args2net(@_);
      print $server_fh pack("N",length($req)).$req;
      read($server_fh,$len,4) == 4 or die "protocol error";
      $len=unpack("N",$len);
      read($server_fh,$req,$len) == $len or die "protocol error";
      ($req,@args)=net2args($req);
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
   print "trying to start gimp\n" if $Gimp::verbose;
   $server_fh=*{$new_handle++};
   socketpair $server_fh,GIMP_FH,PF_UNIX,SOCK_STREAM,AF_UNIX
      or croak "unable to create socketpair for gimp communications: $!";
   $gimp_pid = fork;
   if ($gimp_pid > 0) {
      Gimp::ignore_functions(@Gimp::gimp_gui_functions);
      return $server_fh;
   } elsif ($gimp_pid == 0) {
      close $server_fh;
      delete $ENV{GIMP_HOST};
      unless ($Gimp::verbose) {
         open STDOUT,">/dev/null";
         open STDERR,">&1";
         close STDIN;
      }
      my $args = &Gimp::RUN_NONINTERACTIVE." ".
                 (&Gimp::_PS_FLAG_BATCH | &Gimp::_PS_FLAG_QUIET)." ".
                 fileno(GIMP_FH);
      { # block to suppress warning with broken perls (e.g. 5.004)
         exec "gimp","-n","-b","(extension-perl-server $args)",
                               "(extension_perl_server $args)"
      }
      exit(255);
   } else {
      croak "unable to fork: $!";
   }
}

sub try_connect {
   $_=$_[0];
   my $fh;
   $auth = s/^(.*)\@// ? $1 : "";	# get authorization
   if ($_ ne "") {
      if (s{^spawn/}{}) {
         return start_server;
      } elsif (s{^unix/}{/}) {
         my $server_fh=*{$new_handle++};
         return socket($server_fh,PF_UNIX,SOCK_STREAM,AF_UNIX)
                && connect($server_fh,sockaddr_un $_)
                ? $server_fh : ();
      } else {
         s{^tcp/}{};
         my($host,$port)=split /:/,$_;
         $port=$default_tcp_port unless $port;
         my $server_fh=*{$new_handle++};
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
   if (@_) {
      $server_fh = try_connect ($_[0]);
   } elsif (defined($Gimp::host)) {
      $server_fh = try_connect ($Gimp::host);
   } elsif (defined($ENV{GIMP_HOST})) {
      $server_fh = try_connect ($ENV{GIMP_HOST});
   } else {
      $server_fh = try_connect ("");
   }
   defined $server_fh or croak "could not connect to the gimp server server (make sure Net-Server is running)";
   { my $fh = select $server_fh; $|=1; select $fh }
   
   my @r = response;
   
   die "expected perl-server at other end of socket, got @r\n"
      unless $r[0] eq "PERL-SERVER";
   shift @r;
   die "expected protocol version $Gimp::_PROT_VERSION, but server uses $r[0]\n"
      unless $r[0] eq $Gimp::_PROT_VERSION;
   shift @r;
   
   for(@r) {
      if($_ eq "AUTH") {
         die "server requests authorization, but no authorization available\n"
            unless $auth;
         my $req = "AUTH".$auth;
         print $server_fh pack("N",length($req)).$req;
         my @r = response;
         die "authorization failed: $r[1]\n" unless $r[0];
         print "authorization ok, but: $r[1]\n" if $Gimp::verbose and $r[1];
      }
   }

   $initialized = 1;
   Gimp::_initialized_callback;
}

sub gimp_end {
   $initialized = 0;

   undef $server_fh;
   kill 'KILL',$gimp_pid if $gimp_pid;
   undef $gimp_pid;
}

sub gimp_main {
   gimp_init;
   no strict 'refs';
   eval { &{caller(1)."::net"} };
   die $@ if $@ && $@ ne "BE QUIET ABOUT THIS DIE\n";
   gimp_end;
   return 0;
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

When started from within The Gimp, the Perl-Server will create a unix domain
socket to which local clients can connect. If an authorization password is
given to the Perl-Server (by defining the environment variable C<GIMP_HOST>
before starting The Gimp), it will also listen on a tcp port (default
10009). Since the password is transmitted in cleartext, using the Perl-Server
over tcp effectively B<lowers the security of your network to the level of
telnet>.

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
