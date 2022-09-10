#!/bin/sh
# extract-vector-icon.sh
# Copyright (C) 2016 Jehan
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# This script generates a new SVG file by extracting a single object by
# its id, from a source SVG, and updating the viewBox (canvas) size and
# position.
usage ()
{
  printf "Usage: extract-vector-icon.sh source icon-name [width height]\n"
  printf "Create the file 'icon-name.svg' from the \`source\` SVG.\n"
}

if [ "$#" != 2 ]; then
  if [ "$#" != 4 ]; then
    usage
    exit 1
  fi
fi

# The script is run from $(top_builddir)/icons/*/
compute_viewbox="$(pwd)/../../tools/compute-svg-viewbox"
source="$1"
id="$2"
if [ "$#" = 4 ]; then
  # The expected display width/height for the image.
  width="$3"
  height="$4"
else
  # We base the design of our scalable icons on 16x16 pixels.
  width="16"
  height="16"
fi

# Extract the icon code.
#icon=`xmllint "$source" --xpath '//*[local-name()="g" and @id="'$id'"]'`
icon=`xmllint "$source" --xpath '//*[@id="'$id'"]' --noblanks`
# Get rid of any transform on the top node to help librsvg.
#icon=`echo $icon | sed 's/^\(<[^>]*\) transform="[^"]*"/\1/'`
if [ $? -ne 0 ]; then
  >&2 echo "extract-vector-icon.sh: object id \"$id\" not found in \"$source\" ";
  exit 1;
fi;

# Add !important to any object with label "color-important".
icon=`echo $icon | sed 's/<\([^<>]*\)style="\([^"]*\)fill:\([^;"]*\)\([^"]*\)"\([^<>]*\)inkscape:label="color-important"\([^>]*\)>/<\1style="\2fill:\3 !important\4"\5\6>/'`
icon=`echo $icon | sed 's/<\([^<>]*\)inkscape:label="color-important"\([^>]*\)style="\([^"]*\)fill:\([^;"]*\)\([^"]*\)"\([^<>]*\)>/<\1\2style="\3fill:\4 !important\5"\6>/'`

# The typical namespaces declared at start of a SVG made with Inkscape.
# Since we are not sure of what namespace will use the object XML, and
# since we don't want to end up with invalid XML, we just keep them all
# declared here.
svg_start='<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!-- Created by GIMP build. -->

<svg
  xmlns:osb="http://www.openswatchbook.org/uri/2009/osb"
  xmlns:dc="http://purl.org/dc/elements/1.1/"
  xmlns:cc="http://creativecommons.org/ns#"
  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:svg="http://www.w3.org/2000/svg"
  xmlns="http://www.w3.org/2000/svg"
  xmlns:xlink="http://www.w3.org/1999/xlink"
  xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
  xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
  version="1.1"
'

# Grab the defined color palette.
defs=`xmllint "$source" --xpath '//*[local-name()="defs"]'`

# Create a temporary SVG file with the main information.
svg_temp="`mktemp ./${id}-XXXX.svg`"
echo "$svg_start>$defs$icon</svg>" > $svg_temp

x=0
y=0
# In case the source SVG has a viewBox not starting at (0, 0), get
# the current origin coordinates.
#viewBox=`xmllint $svg_temp --xpath '/*[local-name()="svg"]/@viewBox'`
#if [ $? -eq 0 ]; then
#  x=`echo $viewBox| sed 's/ *viewBox *= *"\([0-9]\+\) .*$/\1/'`
#  y=`echo $viewBox| sed 's/ *viewBox *= *"[0-9]\+ \+\([0-9]\+\).*$/\1/'`
#fi;

# Compute the viewBox that we want to set to our generated SVG.
viewBox=`$compute_viewbox "$svg_temp" "$id" $x $y`
if [ $? -ne 0 ]; then
  >&2 echo "extract-vector-icon.sh: error running \`$compute_viewbox "$tmp" "$id" $x $y\`.";
  rm -f $svg_temp
  exit 1;
fi;
rm -f $svg_temp

# The finale SVG file with properly set viewBox.
svg="$svg_start  $viewBox"
if [ "$#" = 5 ]; then
  svg="$svg
  width=\"$width\"
  height=\"$height\""
fi
svg="$svg>
<title>$id</title>
$defs
$icon
</svg>"

echo "$svg"
