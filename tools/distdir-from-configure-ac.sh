#!/bin/sh

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: `basename $0` PACKAGE_NAME CONFIGURE_AC"
    echo
    echo "Prints the distdir for the given package based on version information"
    echo "in configure.ac. Useful for example in a buildbot buildstep to remove "
    echo "a stray distdir from a failed distcheck"
    exit 1
fi

package_name=$1
configure_ac=$2

if [ ! -f $configure_ac ]; then
    echo "File '$configure_ac' does not exist"
    exit 2
elif ! grep $package_name $configure_ac &>/dev/null; then
    echo "No occurance of '$package_name' in '$configure_ac'"
    exit 3
fi

number ()
{
    number_type=$1
    regexp=".*m4_define.*\[${package_name}_${number_type}_version\].*\[\(.\)\].*"
    grep $regexp $configure_ac | sed s/$regexp/\\1/
}

echo $package_name-`number major`.`number minor`.`number micro`
