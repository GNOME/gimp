#!/bin/sh

dir=`pwd`
subdirs=`find . -maxdepth 1 -type d`
local=`pwd | sed -e 's/^.*\/help\/C\\(.*\\)/\\1/'`

#
# Create the Makefile.am for this directory
#

cat << EOF > Makefile.am
## Process this file with automake to produce Makefile.in

EOF

set $subdirs

if [ "x$3" != "x" ]; then

echo -n "SUBDIRS = " >> Makefile.am

for dir in $subdirs
do

if [ $dir != "." ]; then
if [ $dir != "./CVS" ]; then

echo -n "`basename $dir` " >> Makefile.am

fi
fi

done

echo >> Makefile.am
echo >> Makefile.am

fi

echo "helpdatadir = \$(gimpdatadir)/help/C$local" >> Makefile.am

echo >> Makefile.am
echo -n "helpdata_DATA = " >> Makefile.am

for file in *.html
do

echo "\\" >> Makefile.am
echo -n "        $file " >> Makefile.am

done

cat << EOF >> Makefile.am


EXTRA_DIST = \$(helpdata_DATA)

.PHONY: files

files:
	@files=\`ls \$(DISTFILES) 2> /dev/null\`; for p in \$\$files; do \\
	  echo \$\$p; \\
	done
EOF

if [ "x$3" != "x" ]; then

cat << EOF >> Makefile.am
	@for subdir in \$(SUBDIRS); do \\
	  files=\`cd \$\$subdir; \$(MAKE) files | grep -v "make\[[1-9]\]"\`; \\
	  for file in \$\$files; do \\
	    echo \$\$subdir/\$\$file; \\
	  done; \\
	done
EOF

fi
