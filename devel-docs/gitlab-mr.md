# Merge Request tricks

By default, a Merge Request pipeline would only build GIMP with
autotools, meson and for Windows 64-bit (similarly to normal commits).

You might want to actually generate easy-to-install builds, in
particular if you want it to be testable for non-developers, or various
other reasons. Making a full flatpak or Windows installer can actually
be quite time-consuming on a personal computer.

☣️  We remind that these packages are built on-top of development code
(i.e. work-in-progress and potentially unstable codebase likely
containing critical bugs) with additional code which can be contributed
by anyone (any anonymous person is allowed to propose patches as merge
requests not only known team members).
Therefore you should always check the merge request changes before
running the code and never blindly trust that it is harmless. In any
case, run these builds at your own risk. ☢️

## Generating a Windows installer for merge request code

If you add the label `5. Windows Installer` in a MR, then trigger a
pipeline (for instance by rebasing), it will add a Windows installer
creation to the pipeline. Once the pipeline ends, the installer can be
found by:

- clicking the pipeline ID.
- In the "Distribution" stage, click the "win-installer-nightly" job.
- Then click the "Browse" button.
- Navigate to `build/installer/_Output/`.
- Then click the `gimp-<version>-setup.exe` file to download the
  installer.

## Generating a flatpak for merge request code

If you add the label `5. Flatpak package` in a MR, then trigger a
pipeline for instance by rebasing), it will add a flatpak creation to
the pipeline. Once the pipeline ends, the flatpak can be installed by:

- clicking the pipeline ID.
- In the "Gimp" stage, click the "flatpak" job.
- Then click the "Browse" button.
- Click the `gimp-git.flatpak` file to download it.
- Locally run: `flatpak install --user ./gimp-git.flatpak`
  It should propose you to install the flatpak, allowing you to test.
- After testing, you can uninstall with: `flatpak remove org.gimp.GIMP//master`
