
# If not set in the environment, set the deployment target of what we
# usually use to build GIMP 2.8.
MACOSX_DEPLOYMENT_TARGET ?= 10.6

EXTRA_DIST = \
	info.plist \
	info.plist.in

all-local: info.plist

define sedscript
s/%GIMP_MAJOR_VERSION%/@GIMP_MAJOR_VERSION@/;\
s/%GIMP_MINOR_VERSION%/@GIMP_MINOR_VERSION@/;\
s/%GIMP_MICRO_VERSION%/@GIMP_MICRO_VERSION@/;\
s/%MACOSX_DEPLOYMENT_TARGET%/$(MACOSX_DEPLOYMENT_TARGET)/
endef

info.plist: info.plist.in
	@if test -d "$(top_srcdir)/.git"; then \
	  git_last_commit_year="`git --git-dir=$(top_srcdir)/.git log -n1 --reverse --pretty=%ci | cut -b 1-4`"; \
	else \
	  git_last_commit_year="`date -u '+%Y'`"; \
	fi; \
	sed -e "$(sedscript)" -e "s/%GIMP_GIT_LAST_COMMIT_YEAR%/$$git_last_commit_year/" $< > $@
