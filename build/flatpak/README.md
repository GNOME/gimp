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
nightly"](https://gitlab.gnome.org/GNOME/gimp/-/pipeline_schedules).

This job will build whatever is on GIMP's repository `master` branch (this
branch should be kept buildable and usable at all time, not only for scheduled
jobs, but also for all contributors to be able to improve GIMP at all time).

The nightly manifest file is: `build/flatpak/org.gimp.GIMP-nightly.json`

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
* A merge request with the label `5. Flatpak package` will contain the `flatpak`
  job, hence allowing theoretically to build a standalone flatpak (without being
  published to the nightly repository) for MR code. In practice, jobs have an
  1-hour timeout and our flatpak takes longer than 1 hour to build (there is an
  exception in our repository, but only for the `master` branch), so we often
  need to publish to `master` after mostly a visual review.

## Custom Flatpak builds (for development)

Most contributors simply build GIMP the "old-school" way, nevertheless some
projects are starting to use `flatpak` as a development environment. Here is how
this can be done, as far as our knowledge goes.

Note 1
: The below process is about using flatpak as a development environment, not for
  proper [releases to a
  repository](https://docs.flatpak.org/en/latest/flatpak-builder.html), neither
  for creating [standalone
  bundles](https://docs.flatpak.org/en/latest/single-file-bundles.html)

Note 2
: since we usually only use flatpak for releases, not for development, there may
  be better ways to make a flatpak development environment. We welcome any
  proposed improvement to this file.

* Dependencies:
  - flatpak (at least 0.9.5)
  - flatpak-builder (at least 0.9.5, when the option `--export-only` has been
    added to `flatpak-builder` so that the build and the export can be made in 2
    separate steps while using the high level procedure.)
  - appstream-compose

  Note 1
  : there are packages of `flatpak` and `flatpak-builder` for [most
  distributions](http://flatpak.org/getting.html).

  Note 2
  : `appstream-compose` is used to parse the appdata file and generate the
    appstream (metadata like comments, etc.).
    On Fedora, this is provided by the package `libappstream-glib`, on Ubuntu by
    `appstream-util`…

* Install the runtimes and the corresponding SDKs if you haven't already:

  ```sh
  flatpak remote-add --user --from gnome https://nightly.gnome.org/gnome-nightly.flatpakrepo
  flatpak install --user gnome org.gnome.Platform/x86_64/master org.gnome.Sdk/x86_64/master
  flatpak install --user gnome org.gnome.Platform/aarch64/master org.gnome.Sdk/aarch64/master
  ```

  Or simply update them if you have already installed them:

  ```sh
  flatpak update
  ```

* Setup some recommended build options:

  ```sh
  export BUILD_OPTIONS="--ccache --keep-build-dirs --force-clean"
  ```

  We recommend using `ccache` to improve build speed, and to keep build dirs
  (these will be found in `.flatpak-builder/build/` relatively to the work
  directory; you may manually delete these once you are done to save space) for
  later debugging if ever any configuration or build issue arises.

* Choose what architecture to build and where you will "install" your flatpak:

  ```sh
  # Architectures supported with GIMP flatpak are one of 'x86_64' or 'aarch64':
  export ARCH="x86_64"
  # Path where build files are stored
  export INSTALLDIR="`pwd`/${ARCH}"
  ```

* Build the flatpak up to GIMP itself (not included):

  ```sh
  flatpak-builder $BUILD_OPTIONS --arch="$ARCH" --stop-at=gimp \
                  "${INSTALLDIR}" org.gimp.GIMP-nightly.json 2>&1 \
                  | tee gimp-nightly-flatpak.log
  ```

  The build log will be outputted on `stdout` as well as being stored in a file
  `gimp-nightly-flatpak.log`.

* Now we'll want to build GIMP itself, yet not from a clean repository clone
  (which is what the manifest provides) but from your local checkout, so that
  you can include any custom code:

  ```sh
  flatpak build "${INSTALLDIR}" meson setup --prefix=/app/ --libdir=/app/lib/ _gimp ../../ 2>&1 | tee -a gimp-nightly-flatpak.log
  flatpak build "${INSTALLDIR}" ninja -C _gimp 2>&1 | tee -a gimp-nightly-flatpak.log
  flatpak build "${INSTALLDIR}" ninja -C _gimp install 2>&1 | tee -a gimp-nightly-flatpak.log
  ```

  This assumes you are currently within `build/flatpak/`, therefore the
  repository source is 2 parent-folders away (`../../`). The build artifacts
  will be inside the `_gimp/` subfolders, and finally it will be installed with
  the rest of the flatpak inside `$INSTALLDIR`.

* For development purpose, you don't need to export the flatpak to a repository
  or even install it. Just run it directly from your build directory:

  ```sh
  flatpak-builder --run "${INSTALLDIR}" org.gimp.GIMP-nightly.json gimp-2.99
  ```

## Maintaining the manifests

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

* On the other hand, if we increased the runtime version in particular (or when
  the `master` runtime evolves), some modules may no longer be necessary and can
  be removed from our manifest.

  A flatpak is a layered set of modules. Our GIMP build in particular is
  built over the GNOME runtime, itself built over the Freedesktop
  runtime, itself based on a yocto-built image.
  Other than by trial and error, you can find the installed dependencies
  by running:

```sh
flatpak run --devel --command=bash org.gnome.Sdk//master
```

Or if you already have a build:

```
flatpak run --devel --command=bash org.gimp.GIMP//master
```

Inside the flatpak sandbox, GIMP's manifest can be read with:

```sh
 less /app/manifest.json
```

GNOME and Freedesktop's module lists (generated manifest as the SDK is built
from BuildStream):

```sh
less /usr/manifest.json
```

* Some sources have set a `x-checker-data` property which makes it possible to
  check for updates using
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

* For the development releases and nightly builds, we added the
  `desktop-file-name-prefix` property. For a stable release, the property line
  can be removed from the manifest.

* For a stable release, set top `"branch":"stable"`, and inside the
  "gimp", "babl" and "gegl" modules, set "tag" to the git tag (ex:
  `GIMP_2_10_34`) and "commit" to the git commit hash for this tag.

* For a development release, set top `"branch":"beta"`, and inside the
  "gimp", "babl" and "gegl" modules, set "tag" to the git tag (ex:
  `GIMP_2_99_14`) and "commit" to the git commit hash for this tag.

* For a nightly build, set top "branch":"master", and inside the
  "gimp", "babl" and "gegl" modules, set "branch" to "master", and
  remove any "commit" line.
