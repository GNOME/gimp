#!/bin/sh

dir=`basename \`pwd\``
subdirs=`find . -maxdepth 1 -type d`
local=`pwd | sed -e 's/^.*\/help\/C\\(.*\\)/\\1/'`

#
# Create dummy help pages
#

for file in $*
do

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

done
