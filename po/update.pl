#!/usr/bin/perl -w

#  GNOME po update utility.
#  (C) 2000 The Free Software Foundation
#
#  Author(s): Kenneth Christiansen


$VERSION = "1.2.5 beta 2";
$LANG    = $ARGV[0];
$PACKAGE  = "gimp";

if (! $LANG){
    print "update.pl:  missing file arguments\n";
    print "Try `update.pl --help' for more information.\n";
    exit;
}

if ($LANG=~/^-(.)*/){

    if ("$LANG" eq "--version" || "$LANG" eq "-V"){
        print "GNOME PO Updater $VERSION\n";
        print "Written by Kenneth Christiansen <kenneth\@gnome.org>, 2000.\n\n";
        print "Copyright (C) 2000 Free Software Foundation, Inc.\n";
        print "This is free software; see the source for copying conditions.  There is NO\n";
        print "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n";
	exit;
    }


    elsif ($LANG eq "--help" || "$LANG" eq "-H"){
        print "Usage: ./update.pl [OPTIONS] ...LANGCODE\n";
        print "Updates pot files and merge them with the translations.\n\n";
        print "  -V, --version                shows the version\n";
        print "  -H, --help                   shows this help page\n";
        print "  -P, --pot                    only generates the potfile\n";
        print "  -M, --maintain               search for missing files in POTFILES.in\n";
	print "\nExamples of use:\n";
        print "update.sh --pot    just creates a new pot file from the source\n";
	print "update.sh da       created new pot file and updated the da.po file\n\n";
        print "Report bugs to <kenneth\@gnome.org>.\n";
	exit;
    }

    elsif($LANG eq "--pot" || "$LANG" eq "-P"){
                
        print "Building the $PACKAGE.pot ...";

        $b="xgettext --default-domain\=$PACKAGE --directory\=\.\."
          ." --add-comments --keyword\=\_ --keyword\=N\_"
          ." --files-from\=\.\/POTFILES\.in ";
        $b1="test \! -f $PACKAGE\.po \|\| \( rm -f \.\/$PACKAGE\.pot "
           ."&& mv $PACKAGE\.po \.\/$PACKAGE\.pot \)";
         
        `$b`;
        `$b1`;

        print "...done\n";

        exit;
    }
        
    elsif ($LANG eq "--maintain" || "$LANG" eq "-M"){

        $a="find ../ -print | egrep '.*\\.(c|y|cc|c++|h|gob)' ";

        open(BUF2, "POTFILES.in") || die "update.pl:  there's not POTFILES.in!!!\n";
        print "Searching for missing _(\" \") entries...\n";
        open(BUF1, "$a|");


        @buf2 = <BUF2>;
        @buf1 = <BUF1>;

        if (-s "POTFILES.ignore"){
            open FILE, "POTFILES.ignore";
            while (<FILE>) {
                if ($_=~/^[^#]/o){
                    push @bup, $_;
                }
            }
            print "POTFILES.ignore found! Ignoring files...\n";
	    @buf2 = (@bup, @buf2);
        }

        foreach my $file (@buf1){
            open FILE, "<$file";
            while (<FILE>) {
                if ($_=~/_\(\"/o){
                    $file = unpack("x3 A*",$file) . "\n";
                    push @buff1, $file;
                    last;
                }
            }
        }

        @bufff1 = sort (@buff1);

        @bufff2 = sort (@buf2);

        my %in2;
        foreach (@bufff2) {
            $in2{$_} = 1;
        }

        foreach (@bufff1){
            if (!exists($in2{$_})){
                push @result, $_ }
            }

        if(@result){
            open OUT, ">POTFILES.in.missing";
            print OUT @result;
            print "\nHere are the results:\n\n", @result, "\n";
            print "File POTFILES.in.missing is being placed in directory...\n";
            print "Please add the files that should be ignored in POTFILES.ignore\n";
        }
        else{
            print "\nWell, it's all perfect! Congratulation!\n";
        }         
    }


    else{
        print "update.pl: invalid option -- $LANG\n";
        print "Try `update.pl --help' for more information.\n";
    }
    exit;
    }

elsif(-s "$LANG.po"){

    print "Building the $PACKAGE.pot ...";

    $c="xgettext --default-domain\=$PACKAGE --directory\=\.\."
      ." --add-comments --keyword\=\_ --keyword\=N\_"
      ." --files-from\=\.\/POTFILES\.in ";  
    $c1="test \! -f $PACKAGE\.po \|\| \( rm -f \.\/$PACKAGE\.pot "
       ."&& mv $PACKAGE\.po \.\/$PACKAGE\.pot \)";

    `$c`;
    `$c1`;

    print "...done";

    print "\nNow merging $LANG.po with $PACKAGE.pot, and creating an updated $LANG.po ...\n";

    
    $d="mv $LANG.po $LANG.po.old && msgmerge $LANG.po.old $PACKAGE.pot -o $LANG.po";

    $f="msgfmt --statistics $LANG.po";

    `$d`;
    `$f`;

    unlink "messages";
    unlink "$LANG.po.old";

    exit;
}

else{
    print "update.pl:  sorry $LANG.po does not exist!\n";
    print "Try `update.pl --help' for more information.\n";    
    exit;
}
