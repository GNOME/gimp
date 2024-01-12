#!/bin/sh

############################################
# Script gathering statistics on a release #
############################################

#### Usage ####
if [ "$#" -ne 2 -a "$#" -ne 1 ]; then
    echo "Usage: $0 <GIMP_TAG_PREV> <GIMP_TAG_CUR>"
    echo
    echo "  GIMP_TAG_PREV: last tag release or commit (non-included in stats)"
    echo "                 ex: GIMP_2_9_6"
    echo "  GIMP_TAG_CUR:  current tag release or commit (included in stats); ex: GIMP_2_9_8"
    echo "                 ex: GIMP_2_9_8."
    echo "                 Optional. If absent, statistics up to HEAD."
    exit 1
fi

PREV=$1
git --no-pager show $PREV >/dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "First tag is unknown: $PREV"
  exit 2
fi

if [ "$#" = 2 ]; then
  CUR=$2
  git --no-pager show $CUR >/dev/null 2>&1
  if [ $? -ne 0 ]; then
    echo "Second tag is unknown: $CUR"
    exit 2
  fi
else
  CUR='HEAD'
fi

echo "## GIMP contributions between $PREV and $CUR ##"
echo

# Main stats:
contribs=`git --no-pager shortlog -s -n $PREV..$CUR`
contribs_n=`printf "$contribs" | wc -l`
commits_n=`git log --oneline $PREV..$CUR | wc -l`

prev_date=`git log -1 --format=%ci $PREV`
cur_date=`git log -1 --format=%ci $CUR`
# I think this is not very portable, and may be a bash-specific syntax
# for arithmetic operations. XXX
days_n=$(( (`date -d  "$cur_date" +%s` - `date -d "$prev_date" +%s`)/(60*60*24) ))
commits_rate=$(( $commits_n / $days_n ))

echo "Start date: $prev_date - End date: $cur_date"
echo "Between $PREV and $CUR, $contribs_n people contributed $commits_n commits to GIMP."
echo "This is an average of $commits_rate commits a day."
echo
echo "Statistics on all files:" `git diff --shortstat $PREV..$CUR 2>/dev/null`
echo
echo "Total contributor list:"
printf "$contribs"

echo
echo
echo "## DEVELOPERS ##"
echo

echo "Statistics on C files:" `git diff --shortstat $PREV..$CUR  -- "*.[ch]" 2>/dev/null`
echo
echo "Core developers:"

git --no-pager shortlog -s -n $PREV..$CUR -- app/ "libgimp*" pdb tools/pdbgen/

echo "Plugin and module developers:"

git --no-pager shortlog -s -n $PREV..$CUR -- plug-ins/ modules/

echo
echo "## TRANSLATIONS ##"
echo
#git --no-pager log --stat $PREV..$CUR -- po* | grep "Updated\? .* translation"|sed "s/ *Updated\? \(.*\) translation.*/\\1/" | sort | uniq | paste -s -d, | sed 's/,/, /g'

i18n=`git --no-pager log --stat $PREV..$CUR -- po* | grep "Updated\? .* \(translation\|language\)"`
i18n=`printf "$i18n" | sed "s/ *Updated\? \(.*\) \(translation\|language\).*/\\1/" | sort | uniq`
i18n_n=`printf "$i18n" | wc -l`
# It seems that if the last line has no newline, wc does not count it.
# Add one line manually.
i18n_n=$(( $i18n_n + 1 ))
i18n_comma=`printf "$i18n" | paste -s -d, | sed 's/,/, /g'`

echo "$i18n_n translations were updated: $i18n_comma."
echo

echo "Translators:"
git --no-pager shortlog -sn $PREV..$CUR -- "po*/*.po"

echo
echo "## DESIGN ##"
echo

new_icons=`git log --name-status $PREV..$CUR -- icons/*.svg icons/*.png 2>/dev/null|grep "^A\s"|sed 's%^.*/\([^/]*\)\..*$%\1%' | sort | uniq`
icons_n=$((`printf "$new_icons" | wc -l` + 1))
icons_comma=`printf "$new_icons"  | paste -s -d, | sed 's/,/, /g'`

if [ "$icons_n" -lt 20 ]; then
  echo "$icons_n icons were added: $icons_comma"
else
  echo "$icons_n icons were added (too many to list them all)."
fi
echo
echo "Icon designers:"
git --no-pager shortlog -sn $PREV..$CUR -- "icons/*.svg" "icons/*.png"

echo "Theme designers:"
git --no-pager shortlog -sn $PREV..$CUR -- "themes/*/*.css" "themes/*/*png"

echo "Cursor creators:"
git --no-pager shortlog -sn $PREV..$CUR -- cursors/

echo "Resource creators:"
git --no-pager shortlog -sn $PREV..$CUR -- data/ etc/ desktop/ menus/

echo
echo "## Documenters ##"
echo

git --no-pager shortlog -sn $PREV..$CUR -- docs/ devel-docs/ NEWS INSTALL.in "*README*" HACKING

echo
echo "## Build maintainers ##"
echo

echo "Meson build system:"
git --no-pager shortlog -sn $PREV..$CUR -- meson_options.txt "*/meson.build"

echo "Gitlab CI:"
git --no-pager shortlog -sn $PREV..$CUR -- .gitlab-ci.yml

echo "Binary builds:"
git --no-pager shortlog -sn $PREV..$CUR -- build/
