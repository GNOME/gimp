#!/bin/sh

printf "\e[0Ksection_start:`date +%s`:branch_check[collapsed=false]\r\e[0KChecking for dead branches\n"
git branch -r | grep -v 'origin/HEAD' | grep -v "origin/$CI_DEFAULT_BRANCH" | while IFS= read remote_branch; do
  remote_branch=$(printf "%s\n" "$remote_branch" | sed 's/^ *//;s/ *$//')
  branch_name=$(printf "%s\n" "$remote_branch" | sed 's|origin/||')
  
  # NOT CHECKING
  ## Skip old stable branches
  if echo "$branch_name" | grep -q "^gimp-[0-9]-" || [ "$branch_name" = "gnome-2-2" ] || [ "$branch_name" = "gnome-2-4" ]; then
    printf "\033[33m(SKIP)\033[0m: $branch_name is a snapshot of $CI_DEFAULT_BRANCH but no problem\n"
    continue
  fi
  ## Skip recently created branches
  if [ "$(git rev-parse "$remote_branch")" = "$(git rev-parse "$CI_COMMIT_SHA")" ]; then
    printf "\033[33m(SKIP)\033[0m: $branch_name is identical to $CI_DEFAULT_BRANCH but no problem\n"
    continue
  fi

  # CHECKING
  ## Check: merge-base
  if git merge-base --is-ancestor "$remote_branch" "$CI_COMMIT_SHA"; then
    printf "\033[31m(ERROR)\033[0m: $branch_name is fully merged into $CI_DEFAULT_BRANCH (via git merge-base)\n"
    touch 'dead_branch'
    continue
  fi
  ## Fallback check: cherry
  cherry_output=$(git cherry "$CI_COMMIT_SHA" "$remote_branch")
  if [ -z "$(printf "%s\n" "$cherry_output" | grep '^+')" ]; then
    printf "\033[31m(ERROR)\033[0m: $branch_name is fully merged into $CI_DEFAULT_BRANCH (via git cherry)\n"
    touch 'dead_branch'
    continue
  fi
done

if [ -f "dead_branch" ]; then
  printf "         Please delete the merged branches\n"
  exit 1
else
  printf '(INFO): All branches are organized.\n'
fi
printf "\e[0Ksection_end:`date +%s`:branch_check\r\e[0K\n"
