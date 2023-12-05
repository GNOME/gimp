# Merge Request tricks

By default, a Merge Request pipeline would only build GIMP with
meson for Debian and for Windows 64-bit (similarly to normal commits).

You might want to actually generate easy-to-install builds, in
particular if you want it to be testable for non-developers, or various
other reasons. Making a full flatpak, Windows installer or .msixbundle
can actually be quite time-consuming on a personal computer.

☣️  We remind that these packages are built on-top of development code
(i.e. work-in-progress and potentially unstable codebase likely
containing critical bugs) with additional code which can be contributed
by anyone (any anonymous person is allowed to propose patches as merge
requests not only known team members).
Therefore you should always check the merge request changes before
running the code and never blindly trust that it is harmless. In any
case, run these builds at your own risk. ☢️

## Generating a flatpak for merge request code

If you add the label `5. Flatpak package` in a MR, then trigger a
pipeline for instance by rebasing), it will add a flatpak creation to
the pipeline. Once the pipeline ends, the flatpak can be installed by:

- clicking the pipeline ID.
- In the "gimp" stage, click the "gimp-flatpak-x64" job.
- Then click the "Browse" button.
- Click the `gimp-git.flatpak` file to download it.
- Locally run: `flatpak install --user ./gimp-git.flatpak`
  It should propose you to install the flatpak, allowing you to test.
- After testing, you can uninstall with: `flatpak remove org.gimp.GIMP//master`

## Generating a Windows installer for merge request code

If you add the label `5. Windows Installer` in a MR, then trigger a
pipeline (for instance by rebasing), it will add a Windows installer
creation to the pipeline. Once the pipeline ends, the installer can be
found by:

- clicking the pipeline ID.
- In the "Distribution" stage, click the "dist-installer-weekly" job.
- Then click the "Browse" button.
- Navigate to `build/installer/_Output/`.
- Then click the `gimp-<version>-setup.exe` file to download the
  installer.

## Generating a .msixbundle for merge request code

If you add the label `5. Microsoft Store` in a MR, then trigger a
pipeline (for instance by rebasing), it will add a .msixbundle
creation to the pipeline. Once the pipeline ends, the file can be
found by:

- clicking the pipeline ID.
- In the "Distribution" stage, click the "dist-store-weekly" job.
- Then click the "Browse" button.
- Navigate to `build/windows/store/_Output/`.
- Then click the `gimp-<version>-local.msixbundle` file to download it.

To test the .msixbundle, you need to have previously installed `pseudo-gimp.pfx`
in the CA cert group. You can do this with Windows built-in cert installer.

## Reviewing MR branches

Reviewing merge requests on the Gitlab interface often leads to poor
review, because:

- It doesn't show tabs, trailing whitespaces and other space issues
  which a well-configured CLI git would usually emphasize with colors.
- The commit history is not emphasized, only the final results, but it's
  very important to check individual commits, as well as usable commit
  messages.
- It's anyway usually much easier to review patches on your usual
  workflow environment rather than in a hard-to-use web interface.

There are ways to work on your local environments.

### Fetching MR branches automatically (read-only)

This first one is more of a trick, but an incredibly useful one.
Unfortunately it is read-only, so it means you can review but not edit
the MR yourself. Nevertheless since having to edit a MR should be the
exception, not the rule, it's actually not too bad.

Edit your `.git/config` by adding a second "fetch =" rule to the
"origin" remote. It should read:

```
[remote "origin"]
	fetch = +refs/heads/*:refs/remotes/origin/*
	fetch = +refs/merge-requests/*/head:refs/remotes/origin/merge-requests/*
	url = git@ssh.gitlab.gnome.org:GNOME/gimp.git
```

From now on, when you `git pull` or `git fetch` the origin remote, any
new or updated merge request will also be fetched. \o/

### Pushing to a third-party MR branch

There are cases when you want to push to the MR branch. It should stay
rare occasions, but it can be for instance when the contributor seems
stuck and doesn't know how to do some things; or maybe one doesn't
understand instructions; sometimes also some contributors disappear
after pushing their patch and never answer to review anymore.

When this happen, you could merge the commit and fix it immediately
after (but it's never good to leave the repo in a bad state, even for
just a few minutes). You could also apply, fix and push the fixed
commits directly, but then the MR has to be closed and it doesn't look
like it was applied (which is not the end of the world, but it's still
nicer to show proper status on which patches were accepted or not).
Moreover you would not be able to pass the CI build.

So we will fetch the remote yet without naming the remote:

- Click the "Check out branch" button below the merge request
  description. Gitlab gives you instructions but we will only use the
  first step ("Fetch and check out the branch for this merge request").
  For instance if contributor `xyz` created the branch `fix-bug-123` on
  their own remote, you would run:

```
git fetch "git@ssh.gitlab.gnome.org:xyz/gimp.git" 'fix-bug-123'
git checkout -b 'xyz/fix-bug-123' FETCH_HEAD
```

- Now that you are in a local branch with their code, make your fix, add
  a local commit.

- Finally push to the contributor's own remote with the call:

```
git push git@ssh.gitlab.gnome.org:xyz/gimp.git xyz/fix-bug-123:fix-bug-123
```

  This assumes that the contributor checked the option "*Allow commits
  from members who can merge to the target branch.*" (which we ask
  contributors to check, and it's set by default)

- Now check the MR page. It will normally be updated with your new
  commit(s) and a new pipeline should be triggered.

- Finally if you don't need the local branch anymore, you may delete it
  locally. The nice thing is that since you didn't name the remote, it
  doesn't pollute your git output and all data will be simply disposed
  of the next time `git gc` runs (implicitly or explicitly).
