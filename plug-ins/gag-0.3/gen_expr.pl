$prefix= "ex_";

sub flush
{
    print "$def_str\n\n\n$arr_str\n\nint Number_of_Functions=$table_num;\n\
FUNC_INFO $table_name\[$table_num\] = {\n$table_str\n};\n\n\n$decl_str\n";
}

sub begin_table
{
    $ft= 0;
    $table_num=0;
    $table_name= $2;
    $table_str= "";
    $def_str= "";
    $decl_str= "";
    $arr_str= "";
}

sub parse_ptypes
{
    my ($s,$name)= @_;
    my ($c)= 0;
    my ($tmp);

    $name= $prefix . $name . "_ptypes";
    $tmp= "static char $name\[\]= {";

    while ($s ne "")
    {
 	  if ($c == 0) { $c=1; }
 	  else { $tmp .= ", "; }
 
 	  $s =~ /([134N])(.*)/;
	  if ($1 eq "N") { $tmp .= "-1"; }
	  else { $tmp .= " $1"; }	
	  $s= $2;
    }
     
     $arr_str.= $tmp."};\n";
     return $name;
}

sub subst_var 
{
    my ($tmp, $var) = @_;
    $tmp =~ s/P\?/*ptr/g;
    $tmp =~ s/R/ret/g;  
    $tmp =~ s/P(\d+)/$var\[$1\]/g;
    $tmp;
}

sub parse_pushes # (name, code)
{
    my  ($name,$s)= @_;
    my  $l=0;
    my  %policko;
    my $i;
    my $tmp=  "static expr_func  $prefix${name}_funcs\[\]={\n";

    while ($s ne "")
    {
	if ( ! ($s =~ /\s*PUSH(\d)\s*\(\s*(\w+)\s*\)\s*(.*)/) )
	{
	    print STDERR "Wrong push code! :", $_;
	    return "NULL";
	}
	$s= $3;
	$l= $1 if ($1 >=  $l);
	$policko[$1]= $2;
    }
    
    for ($i=0; $i <= $l; $i++)
    {
	$tmp .= ",\n" if ($i != 0) ;
	$policko[$i]= "NULL" if ( $policko[$i] eq "" );
	$tmp.= "  {" . $policko[$i] . ", " . $policko[$i] . ", " .  $policko[$i] . "}";
    }
    $tmp .= "\n};\n\n";
    $arr_str .= $tmp;
    sprintf "$prefix%s_funcs", $name;
}

sub create_function  # (name, nofp, init, body, return )
{
    my $tmp= "";
    my ($name, $nofp, $init, $body, $return)= @_;

    my $c, $i;
    @top_names= ("r_top", "g_top", "b_top", "alfa_top");
    my $tn;

    foreach $c (1,3,4)
    {
	$def_str .= "static void $prefix$name$c(void);\n";
	$tmp .= "static void $prefix$name$c(void)\n{";
        
	if (( $init ne "") || ($body ne ""))
	  { $tmp .= "\n  register DBL ret;"; }
        if ( $body ne "")
	{ $tmp .= "\n  register int i;\n  register DBL *ptr;"; }
	if ( $nofp eq "-1")
	{ $tmp .= "\n  register int num;\n\n  num=(*n_top)->noo - 1;"; }
	$tmp .= "\n";

        for ($i=0; $i < $c; $i++)
        {
	    $tn= $top_names[$i];
	    $tmp .= "\n";

	    if ($nofp eq "-1") { $tmp .= "  $tn -= num;\n"; }
	    elsif ($nofp ne "1") { $tmp .= "  $tn -= $nofp - 1;\n"; }

	    if (($init eq "") && ($body eq ""))
	    {
		$tmp .= " *$tn= ".subst_var($return,$tn).";\n";
		next;
	    }

	    if ($init ne "") 
	    { $tmp .= "  ".subst_var($init,$tn)."\n" }

	    if ($body ne "")
	    { $tmp .= "  for (i= num, ptr= ($tn+1); i; i--, ptr++)\n  {\n    ".subst_var($body,"")."\n  }\n"; }
	    
	    if ($return eq "") { $tmp .= "  *$tn= ret;\n"; }
	    else { $tmp .= " *$tn= ".subst_var($return,$tn).";\n"; }
        }

	$tmp .= "}\n\n";
    }

    $decl_str .= $tmp;
    return sprintf "{$prefix%s1, $prefix%s3, $prefix%s4}",$name, $name, $name;
}

$pt_w= "Warning, bad table line: ";

sub parse_table
{
    #            name    nofp    nofd  altstr    rettype     paramtype    code
    if ( ! /^\s*(\w+)\s+(N|\d)\s+(\d)\s+(\S+)\s+([-134N])\s+(-|[134N]+)\s+(.*)$/ ) 
       { print STDERR $pt_w, $_; return; }

    my ($name,$nofp,$nofd,$altstr,$rettype,$paramtype,$code)= ($1,$2,$3,$4,$5,$6,$7);
    my  $nstr= $name;
    my  $fptrs= "NULL";
    my  $lastfunc= "{NULL, NULL, NULL}";
    my  $funcs= "NULL";
    my  $c=0;

    if ($altstr =~ /"(.+)"/) { $nstr=$1; }
    if ($nofp eq "N") { $nofp= "-1"; }
    if ($rettype eq "N") { $rettype= "-1" }

    if  ($paramtype eq "-") { $paramtype= "NULL"; }
    else { $paramtype= parse_ptypes($paramtype, $name); }

    if ($code =~ /(INIT|BODY|RETURN)/ )
    {
	$f2= $prefix.$name;

	my  ($init, $fc, $body, $final) = ("","","","");;
	$c++;


	if ( $code =~ /^\s*INIT\{\{\s*(.*?)\s*\}\}(.*)$/ )
	{ $init= $1; $code= $2; }

	if ( $code =~ /^\s*BODY\{\{\s*(.*?)\s*,\s*(.*?)\s*\}\}(.*)$/ )
	{ 
	    $body= $2; $fc= $1; $code= $3; 
	    if ( $fc  ne "1..N-1") {
		print STDERR "Function body: not yet implemented:",$_;
		return;
	    }
	}

	if ( $code =~ /^\s*RETURN\{\{(.*?)\}\}\s*(.*)$/ )
	{ $final= $1; $code= $2; }

	if ($code ne "") { print "Warning: wrong code:",$_; return }

	$lastfunc= create_function( $name,$nofp,$init, $body, $final );
    }
    elsif ($code =~ /\s*LAST\s*\(\s*(\w+)\s*\)\s*/ )
	{
	    $lastfunc= "{$1,$1,$1}";
	}
        else
  	{  
	    $funcs= parse_pushes($name, $code);
	}

    $f1= $name if ($c==0);

    if ($c > 1) 
      { print STDERR "Warning - you have to specify exacly one function: ", $_; return; }

    $table_str .= ",\n" if ($ft != 0);
    $ft=1;

    $table_str .=  sprintf("  {\%16s\,\t%2d,\t$nofd,\t$rettype,\t$paramtype,\t$funcs,$lastfunc}", "\"$nstr\"", $nofp);    
    $table_num++;
}

##############################
######
##
#

open  IN, $ARGV[0];
$operation= 0;               # 0 - nothing, 1 - print, 2 - generate table
$END="";

while (<IN>)
{
    study;

    if ($operation) {
	if ($END ne "")
	  { if (/^\s*$END\s*$/) { $operation=0; next; }}
    }

    if ($operation==1) {print; next; }

    if (/^\s*$/) { next; }            # empty lines
    if (/^\s*\/\/.*$/) { next; }      # c-- like comments

    if ($operation == 0)
    {
	if ( ! /^\s*(\w+)\s*(\w*)\s*(<|)\s*(\w*)\s*$/ )
	{
	    print STDERR "Wrong format of line: ", $_;
	    next;
	}
	$command= $1;
	$comarg= $2;
	if ($3 ne "") {$END=$4; }

	if ($command eq "PRINT") { $operation=1;}
	if ($command eq "TABLE") { begin_table; $operation=2; }
	if ($command eq "FLUSH") { flush; }
	
	if ($command eq "END_OF_FILE") { break; }
	next;
    }

    if ($operation == 2) { parse_table; }
}

if ($operation != 0) { print STDERR "Warning: Unexpected end of file\n"; }

