#!/bin/sh

dir=`basename \`pwd\``
subdirs=`find . -maxdepth 1 -type d`
local=`pwd | sed -e 's/^.*\/help\/C\\(.*\\)/\\1/'`

#
# Create the index for this directory
#

cat << EOF > index.html
<!doctype html public "-//w3c//dtd html 4.0 transitional//en">
<html>
<head>
  <META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=iso-8859-1">
EOF

echo "  <title>Index for $dir</title>" >> index.html

cat << EOF >> index.html
</head>

<body text="#000000" bgcolor="#FFFFFF" link="#0000FF"
      vlink="#FF0000" alink="#000088">

  <table width="100%" cellspacing="1" cellpadding="1">
    <tr bgcolor="black">
      <td width="100%" align="center">
EOF

echo "        <font size=\"+2\" color=\"white\">$dir Index</font>" >> index.html

cat << EOF >> index.html
      </td>
    </tr>  
    <tr bgcolor="white" >
      <td width="100%" align="left">
	<p>
	($local/index.html)<p>
EOF

echo "          <a href=\"../index.html\">Top index</a><p>" >> index.html

set $subdirs

if [ "x$3" != "x" ]; then

echo "          Subtopics available:<p>" >> index.html

for dir in $subdirs
do

if [ $dir != "." ]; then
if [ $dir != "./CVS" ]; then

echo "          <a href=\"`basename $dir`/index.html\">`basename $dir`</a><br>" >> index.html

fi
fi

done

fi

echo "          <p>Topics in this directory:<p>" >> index.html

for file in *.html 
do

if [ $file != index.html ]; then

name=`basename $file .html`

echo "          <a href=\"$file\">$name</a><br>" >> index.html

fi

done

cat << EOF >> index.html
	<p>
	/Karin & Olof
	<p>
      </td>
    </tr>
  </table>

</body>
</html>
EOF

#
# Create the dummy help pages
#

for file in *.html 
do

if [ $file != index.html ]; then

name=`basename $file .html`

cat << EOF > $file
<!doctype html public "-//w3c//dtd html 4.0 transitional//en">
<html>
<head>
  <META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=iso-8859-1">
EOF

echo "  <title>Help Page for $name</title>" >> $file

cat << EOF >> $file
</head>

<body text="#000000" bgcolor="#FFFFFF" link="#0000FF"
      vlink="#FF0000" alink="#000088">

  <table width="100%" cellspacing="1" cellpadding="1">
    <tr bgcolor="black">
      <td width="100%" align="center">
EOF

echo "        <font size=\"+2\" color=\"white\">$name help page</font>" >> $file

cat << EOF >> $file
      </td>
    </tr>  
    <tr bgcolor="white" >
      <td width="100%" align="left">
	<p>
	<a href="index.html">Index</a><p>
	($local/$file)<p>
EOF

echo "        Sorry but the help file for $name is not yet done." >> $file

cat << EOF >> $file
	<p>
	/Karin & Olof
	<p>
      </td>
    </tr>
  </table>

</body>
</html>
EOF

fi

done
