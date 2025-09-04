#!/bin/sh
# release-stats.sh
# Copyright (C) 2018-2025 Jehan
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

prev_date=`git log -1 --format=%ci $PREV`
cur_date=`git log -1 --format=%ci $CUR`

prevmajor=$(echo "$1" | cut -d'_' -f2)
prevminor=$(echo "$1" | cut -d'_' -f3)
prevmicro=$(echo "$1" | cut -d'_' -f4)

major=$(echo "$2" | cut -d'_' -f2)
minor=$(echo "$2" | cut -d'_' -f3)
micro=$(echo "$2" | cut -d'_' -f4)

get_issues_mrs()
{
  base_url=$1
  version="$major.$minor.$micro"

  closed_issues=
  merged_mrs=
  milestone=$(curl "$base_url-/milestones?search_title=$version&state=all" 2>/dev/null| grep "/-/milestones/[0-9]" | head -1 | sed 's$^.*/-/milestones/\([0-9]*\)".*$\1$')
  if [ -n "$milestone" ] && [ "$milestone" -eq "$milestone" ] 2>/dev/null; then
    # Milestone exists and is a valid integer.
    closed_issues=$(curl "$base_url-/milestones/$milestone" 2>/dev/null|grep 'issues.*Closed:' -A1 |tail -1)
    merged_mrs=$(curl "$base_url-/milestones/$milestone" 2>/dev/null|grep 'merge_requests.*Merged:' -A1 |tail -1)
  fi

  echo $closed_issues $merged_mrs
}

set -- $(get_issues_mrs "https://gitlab.gnome.org/GNOME/gimp/")
closed_issues=$1
merged_mrs=$2

set -- $(get_issues_mrs "https://gitlab.gnome.org/Teams/GIMP/Design/gimp-ux/")
ux_closed_issues=$1
ux_merged_mrs=$2

if [ -n "$ux_closed_issues" ] || [ "$ux_closed_issues" -eq "$ux_closed_issues" ] 2>/dev/null; then
  : # All good.
else
  echo "Failed to read Gitlab webpages."
  echo "It may be the anti-bot Anubis blocking us."
  echo "Try loading manually: https://gitlab.gnome.org/GNOME/gimp/-/milestones/"
  exit 1
fi

get_latest_from_meson()
{
  dep=$1
  tag=$2
  ver=`git blame $tag -- meson.build | grep "${dep}_minver *=" | sed "s/.*${dep}_minver *= *'\([0-9.]*\)' *$/\1/"`
  echo $ver
}

get_tag_from_version()
{
  prefix=$1
  ver=$2
  tag=`echo $ver | tr . _`
  tag=${prefix}_${tag}
  echo $tag
}

count_contributors()
{
  folders=$1
  text=$2

  contributors=`git --no-pager shortlog -sn $PREV..$CUR -- $folders`
  if [ -n "$contributors" ]; then
    contributors=`echo "$contributors" | cut -f2`
    n_contributors=`echo "$contributors" | wc -l`
    contributors_list=`echo "$contributors" | paste -s -d, | sed 's/,/, /g'`

    printf "* $text" "$n_contributors" "$contributors_list."
    echo
  fi
}

count_data_contributors()
{
  folders=$1
  text=$2

  cd gimp-data
  contributors=`git --no-pager shortlog -sn --since="$prev_date" --until="$cur_date" -- $folders`
  if [ -n "$contributors" ]; then
    contributors=`echo "$contributors" | cut -f2`
    n_contributors=`echo "$contributors" | wc -l`
    contributors_list=`echo "$contributors" | paste -s -d, | sed 's/,/, /g'`

    printf "  - $text" "$n_contributors" "$contributors_list."
    echo
  fi
  cd ..
}

count_repo_contributors()
{
  repo=$1
  branch=$2
  text=$3
  prev_tag=$4
  cur_tag=$5

  cd $repo
  git fetch origin > /dev/null 2>&1
  if [ -z "$cur_tag" ]; then
    contributors=`git --no-pager shortlog -sn --since="$prev_date" --until="$cur_date" origin/$branch`
  else
    contributors=`git --no-pager shortlog -sn $prev_tag..$cur_tag`
  fi

  if [ -n "$contributors" ]; then
    contributors=`echo "$contributors" | cut -f2`
    n_contributors=`echo "$contributors" | wc -l`
    contributors_list=`echo "$contributors" | paste -s -d, | sed 's/,/, /g'`

    if [ -z "$cur_tag" ]; then
      n_commits=`git --no-pager log --oneline --since="$prev_date" --until="$cur_date" origin/$branch | wc -l`
    else
      n_commits=`git --no-pager log --oneline $prev_tag..$cur_tag | wc -l`
    fi
    printf "* $text" "$n_commits" "$n_contributors" "$contributors_list."
    echo
  fi
  cd - > /dev/null
}

echo "Since [GIMP $prevmajor.$prevminor.$prevmicro](TODO), in the main GIMP repository:"
echo
echo "* $closed_issues reports were closed as FIXED."
echo "* $merged_mrs merge requests were merged."

# Main stats:
contribs=`git --no-pager shortlog -s -n $PREV..$CUR`
contribs_n=`printf "$contribs" | wc -l`
commits_n=`git log --oneline $PREV..$CUR | wc -l`

echo "* $commits_n commits were pushed."

# I think this is not very portable, and may be a bash-specific syntax
# for arithmetic operations. XXX
#days_n=$(( (`date -d  "$cur_date" +%s` - `date -d "$prev_date" +%s`)/(60*60*24) ))
#commits_rate=$(( $commits_n / $days_n ))

#echo "Start date: $prev_date - End date: $cur_date"
#echo "Between $PREV and $CUR, $contribs_n people contributed $commits_n commits to GIMP."
#echo "This is an average of $commits_rate commits a day."
#echo
#echo "Statistics on all files:" `git diff --shortstat $PREV..$CUR 2>/dev/null`
#echo
#echo "Total contributor list:"
#printf "$contribs"

# Translation stats:
i18n=`git --no-pager log --stat $PREV..$CUR -- po* | grep "Updated\? .* \(translation\|language\)"`
i18n=`printf "$i18n" | sed "s/ *Updated\? \(.*\) \(translation\|language\).*/\\1/" | sort | uniq`
i18n_n=`printf "$i18n" | wc -l`
# It seems that if the last line has no newline, wc does not count it.
# Add one line manually.
i18n_n=$(( $i18n_n + 1 ))
i18n_comma=`printf "$i18n" | paste -s -d, | sed 's/,/, /g'`

echo "* $i18n_n translations were updated: $i18n_comma."

#echo "Statistics on C files:" `git diff --shortstat $PREV..$CUR  -- "*.[ch]" 2>/dev/null`

echo
echo "$contribs_n people contributed changes or fixes to GIMP $major.$minor.$micro codebase (order
is determined by number of commits; some people are in several groups):"
echo
count_contributors 'app/ "libgimp*" pdb tools/pdbgen/' "%d developers to core code: %s"
count_contributors 'plug-ins/ modules/' "%d developers to plug-ins or modules: %s"
count_contributors 'po*/*.po' "%d translators: %s"
count_contributors 'themes/*/*.css themes/*/*png' "%d theme designers: %s"
# XXX For some reason using '*/meson.build' doesn't work out through the
# function. So I list all files explicitly.
meson_builds=`find . -name meson.build -not -path gimp-data`
count_contributors "meson_options.txt $meson_builds .gitlab-ci.yml build" "%d build, packaging or CI contributors: %s"
count_contributors 'data/ etc/ desktop/ menus/ docs/ devel-docs/ NEWS INSTALL.in' "%d contributors on other types of resources: %s"
count_repo_contributors "gimp-data" main "The gimp-data submodule had %d commits by %d contributors: %s"
count_data_contributors 'images' "%d image creators: %s"
count_data_contributors 'icons/*.svg icons/*.png' "%d icon designers: %s"
count_data_contributors 'cursors' "%d cursor designers: %s"
count_data_contributors 'brushes' "%d brush designers: %s"
count_data_contributors 'patterns' "%d pattern designers: %s"
echo "  - TODO: splash author."

echo
echo "Contributions on other repositories in the GIMPverse (order is determined by"
echo "number of commits):"
echo
echo "* Our UX tracker had $ux_closed_issues reports closed as FIXED."

babl_ver=`get_latest_from_meson babl $CUR`
prev_babl_ver=`get_latest_from_meson babl $PREV`
if [ "$babl_ver" != "$prev_babl_ver" ]; then
  prev_tag=`get_tag_from_version BABL $prev_babl_ver`
  cur_tag=`get_tag_from_version BABL $babl_ver`
  count_repo_contributors "../babl" master "babl $babl_ver is made of %d commits by %d contributors: %s" $prev_tag $cur_tag
fi

gegl_ver=`get_latest_from_meson gegl $CUR`
prev_gegl_ver=`get_latest_from_meson gegl $PREV`
if [ "$gegl_ver" != "$prev_gegl_ver" ]; then
  prev_tag=`get_tag_from_version GEGL $prev_gegl_ver`
  cur_tag=`get_tag_from_version GEGL $gegl_ver`
  count_repo_contributors "../gegl" master "GEGL $gegl_ver is made of %d commits by %d contributors: %s" $prev_tag $cur_tag
fi

count_repo_contributors "../ctx.graphics" dev "[ctx](https://ctx.graphics/) had %d commits since $prevmajor.$prevminor.$prevmicro release by %d contributors: %s"
count_repo_contributors "../gimp-test-images" main "The \`gimp-test-images\` (unit testing repository) repository had %d commits by %d contributors: %s"
count_repo_contributors "../gimp-macos-build" master "The \`gimp-macos-build\` (macOS packaging scripts) release had %d commits by %d contributors: %s"
# TODO:
# * flatpak often has bot commits. These should be filtered out.
# * detect a beta vs. stable release and tweak branch accordingly.
count_repo_contributors "../org.gimp.GIMP" master "The flatpak release had %d commits by %d contributors: %s"
count_repo_contributors "../gimp-web" testing "Our main website (what you are reading right now) had %d commits by %d contributors: %s"
count_repo_contributors "../gimp-web-devel" testing "Our [developer website](https://developer.gimp.org/) had %d commits by %d contributors: %s"
count_repo_contributors "../gimp-help" master "Our [3.0 documentation](https://docs.gimp.org/) had %d commits by %d contributors: %s"

echo
echo "Let's not forget to thank all the people who help us triaging in Gitlab, report"
echo "bugs and discuss possible improvements with us."
echo "Our community is deeply thankful as well to the internet warriors who manage our"
echo "various [discussion channels](https://www.gimp.org/discuss.html) or social"
echo "network accounts such as Ville Pätsi, Liam Quin, Michael Schumacher and Sevenix!"
echo
echo "*Note: considering the number of parts in GIMP and around, and how we"
echo "get statistics through \`git\` scripting, errors may slip inside these"
echo "stats. Feel free to tell us if we missed or mis-categorized some"
echo "contributors or contributions.*"

exit 0
