#!/bin/sh

dir=`basename \`pwd\``
subdirs=`find . -maxdepth 1 -type d`
local=`pwd | sed -e 's/^.*\/help\/C\\(.*\\)/\\1/'`

#
# Create the index for this directory
#

cat << EOF > index.html
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
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
