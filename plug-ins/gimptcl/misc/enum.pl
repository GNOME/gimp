#!/usr/local/bin/perl

Top: while (<>) {
    /^(\s|typedef)*enum/ and do {
	while (<>){
	    /^\s*(\w+)/ and do {
		print "   Gtcl_CAdd($1);\n";
	    };
	    /\}.*\;/ and next Top;

	};
    };
};
