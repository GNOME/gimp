# GIMP Snap HowTo

## Stable and Development releases

The Snap Store hosts our stable and development point releases:
https://snapcraft.io/gimp

Base rule to update the "GNU Image Manipulation Program" entry:

* Regularly, a .snap will be generated at each tagged commit.
  In the process, it will be auto submitted to Snap Store.
  (Any case, **please double-check on Snapcraft** if everything is done.)

## Maintaining the packages

* Similarly to our Flatpak, GIMP Snap uses a [GNOME runtime](https://github.com/ubuntu/gnome-sdk),
  which contains a base of libraries, some of which are dependencies of GIMP.
  The runtime version is determined by the `base:` snap version in snapcraft.yaml.
  It is recommended to update that key value following Ubuntu release cycle.
  (Don't forget to notify the 3P plugins developers on https://forum.snapcraft.io/c/snapcrafters
  and/or https://discourse.ubuntu.com/c/project/app-center)

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

Unlike the flatpak, we do not need to manually set babl, gegl or GIMP tags, just:

* For a **nightly** build, set "devel" on "grade",
  use "experimental" on build-id at gimp part "meson-parameters" and
  add "name: gimp-experimental".

* For a **development** series, set "stable" on "grade",
  use "preview" on build-id at gimp part "meson-parameters" and
  add "name: gimp".
  *Important: if the API points to a future GIMP major version, so
  bump gimp-plugins `content:` too*.

* For a **stable** series, set "stable" on "grade",
  use "latest" on build-id at gimp part "meson-parameters" and
  add "name: gimp".
  *Important: if GIMP major version was bumped, so
  bump gimp-plugins `content:` too*.
