# GIMP Flatpak HowTo

## Stable and Development releases

The Flathub repository hosts our stable and development point releases:
https://github.com/flathub/org.gimp.GIMP

We recommend to look at the `README.md` file in respectively the `master` or
`beta` branches of this repository to know more about release procedures.

## Nightly builds

Flathub does not host nightly builds, therefore we publish them on GNOME's
Nightly repository. Our "nightlies" are actually "weeklies" through a [Gitlab job
schedule named "Flatpak
nightly"](https://gitlab.gnome.org/GNOME/gimp/-/pipeline_schedules), which builds
whatever is on GIMP's repository `master` branch. The nightly manifest file is:
`build/linux/flatpak/org.gimp.GIMP-nightly.json`

This file should remain as close as possible to the development manifest
(`org.gimp.GIMP.json` file on the `beta` branch of the Flathub repository) which
itself should remain as close as possible to the stable manifest
(`org.gimp.GIMP.json` file on the `master` branch of the Flathub repository),
since the nightly manifest is meant to become beta eventually, which itself is
meant to become stable eventually.

Base rule to update the nightly build manifest:

* Regularly `org.gimp.GIMP-nightly.json` should be diffed and synced with
  development and stable `org.gimp.GIMP.json`, in particular for all the
  dependencies (which are mostly the same across all 3 builds).
* A merge request with the label `5. Flatpak package` will contain the `*flatpak*`
  jobs, hence allowing theoretically to build a standalone .flatpak (without being
  published to the nightly repository) for MR code be tested.

## Maintaining the modules

* GIMP uses Flatpak's [GNOME runtime](http://flatpak.org/runtimes.html), which
  contains a base of libraries, some of which are dependencies of GIMP.
  While both the stable and development versions should use the latest stable
  runtime version, the nightly manifest uses the `master` version, which is more
  of a *rolling release*.

* Other GIMP dependencies which are not available in the GNOME runtime
  should be built along as modules within GIMP's flatpak.
  Check format in `org.gimp.GIMP-nightly.json` and add modules if
  necessary. For more options, check [flatpak builder's manifest
  format](http://flatpak.org/flatpak/flatpak-docs.html#flatpak-builder).

* Among the dependencies we self-build, some sources on GIMP manifest have set a
  `x-checker-data` property which makes it possible to check for updates using
  [flatpak-external-data-checker](https://github.com/flathub/flatpak-external-data-checker).
  To run the tool either install it locally, via flatpak or via OCI image.

  The OCI image is not straightforward at first but is the least intrusive
  if you already have docker or podman installed:

```sh
cd <path-to-gimp-repo>/flatpak/build
podman run --rm --privileged -v "$(pwd):/run/host:rw" ghcr.io/flathub/flatpak-external-data-checker:latest /run/host/org.gimp.GIMP-nightly.json
```

  Our prefered backend for the checker is Anitya, a database maintained
  by the Fedora project. To set up a new dependency check by Anitya:

  1. verify it is available in the database: https://release-monitoring.org/
  2. then copy the project ID which is the number in the project URI
     within the database.
  3. Finally add a "x-checker-data" field within the "source" dictionary
     in the manifest with type "anitya", the "project-id" and a
     "url-template".
  4. We usually want to depend on stable releases only, i.e. set
     "stable-only to `true`. On exceptional cases, for very valid
     reasons only, we might bypass this limitation, adding a comment
     explaining why we use an unstable release.

* On the other hand, if we increased the runtime version in particular (or when
  the `master` runtime evolves), some modules may no longer be necessary and can
  be removed from our manifest.

  After all, a flatpak is a layered set of modules. Our GIMP build in particular
  is built over the GNOME runtime, itself built over the Freedesktop
  runtime, itself based on a yocto-built image.
  Other than by trial and error, you can find the installed dependencies
  by running:

```sh
flatpak run --devel --command=bash org.gnome.Sdk//master
```

Inside the flatpak sandbox, GNOME and Freedesktop's module lists
(generated manifest as the SDK is built from BuildStream) can be read with::

```sh
less /usr/manifest.json
```

## Versioning the flatpaks

* For the development releases and nightly builds, we added the
  `desktop-file-name-prefix` property. For a stable release, the property line
  can be removed from the manifest.

* For a stable release, set top `"branch":"stable"`, and inside the
  "gimp", "babl" and "gegl" modules, set "tag" to the git tag (ex:
  `GIMP_2_10_34`) and "commit" to the git commit hash for this tag.

* For a development release, set top `"branch":"beta"`, and inside the
  "gimp", "babl" and "gegl" modules, set "tag" to the git tag (ex:
  `GIMP_2_99_14`) and "commit" to the git commit hash for this tag.

* For a nightly build, set top `"branch":"master"`, and inside the
  "gimp", "babl" and "gegl" modules, set "branch" to "master", and
  remove any "commit" line.
