# GIMP Snap HowTo

## Stable and Development releases

The Snap Store hosts our stable and development point releases:
https://snapcraft.io/gimp

Base rule to update the "GNU Image Manipulation Program" entry:

* Regularly, a .snap will be generated at each tagged commit.
  In the process, it will be auto submitted to Snap Store.

## Maintaining the packages

* Similarly to our Flatpak, GIMP Snap uses a [GNOME runtime](https://github.com/ubuntu/gnome-sdk),
  which contains a base of libraries, some of which are dependencies of GIMP.
  The runtime version is determined by the `base:` snap version in snapcraft.yaml.
  It is recommended to update that key value following Ubuntu release cycle.

* Other GIMP dependencies which are not available in the GNOME runtime snap
  should be listed in `build-packages:` and/or `stage-packages:` keys if
  gimp-meson-log.tar artifact shows as necessary. You can take a look at
  the `deps-debian` job from the in .gitlab-ci.yml as reference.

* On the other hand, if we increased the base version in particular (so as
  consequence the GNOME runtime too), some packages may no longer be
  necessary and can be removed from our manifest.

  You can find the installed dependencies by running:

```sh
ls "$(echo /snap/gnome*-sdk/current/usr/lib/$(gcc -print-multiarch)/pkgconfig)"
```

## Versioning the snap

Aside from (un)commenting and bumping babl and gegl source-tag and
setting the version of gimp, we:

* For a **nightly** build, set "devel" on "grade" and
  use "experimental" on build-id at gimp part "meson-parameters".

* For a new **development** series, set "stable" on "grade" and
  use "preview" on build-id at gimp part "meson-parameters".

* For a new **stable** series, set "stable" on "grade" and
  use "latest" on build-id at gimp part "meson-parameters".
