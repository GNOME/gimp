# Images displayed by GIMP

This directory contains images displayed by GIMP, such as the About Dialog
image, the splash images and the software logo itself.

## Splash image

The splash image is displayed at startup, during resources and plug-in loading.
It is also showcased in the first tab of the Welcome dialog, which is always
shown at least in a first installation or the first run after an update.

Previous splash images can be admired in the following page: [splash images history](splash-log.md)

### Requirements

Any new splash image shipped officially with GIMP must follow these
requirements:

- [ ] The artwork **MUST** be submitted under a Libre License.
      Accepted licenses:
      [CC by-sa](https://creativecommons.org/licenses/by-sa/4.0/)
      [CC by](https://creativecommons.org/licenses/by/4.0/),
      [CC 0](https://creativecommons.org/publicdomain/zero/1.0/),
      or [Free Art](https://artlibre.org/licence/lal/en/).
- [ ] XCF file **MUST** be provided.
- [ ] Minimum size: full HD (splash images will be scaled down to 1/2
      of the main display when too big; but they won't be scaled up.
      Therefore anything smaller than fullHD will look tiny and
      unsuited on a 4K or higher res display). Though aspect ratio is not a hard
      requirement, the common 16:9 ratio is recommended.
- [ ] Loading text will appear in bottom quarter, either in black or in
      white depending on the overall surrounding lightness, so image
      contents must be adapted.
- [ ] Integrated Text:
  * [ ] The splash **MUST** contain as text the full software name "GNU
        Image Manipulation Program" and ideally also the acronym "GIMP".
  * [ ] The `major.minor` version **MUST** also be displayed, but never
        the micro version (for instance "3.0" but not "3.0.0").
  * [ ] The fonts used in all text **MUST** be under a Libre license.
  * [ ] A splash meant for a RC or stable release **MUST** also display
        the text "RC", in its own dedicated text layer containing only
        this text, and without the RC number.

### Update procedure

From GIMP 3 and onward, all stable point releases will have a new splash (even
micro version releases). This is not a competition and we do not want artists to
submit splash images unless you were asked to. Images are chosen by core team
members and we will try to vary the type of image (photography, illustration,
design even, and within subtypes too), the styles, origins and more.

Our process when updating it is to:

* delete the previous version: `git rm images/gimp-splash.xcf`;
* add the new splash image in its source format, such as XCF or SVG (not
  generated export images);
* add permalinks to the [splash history](splash-log.md) to the new splash image,
  mentionning the author and the license, to keep an easily accessible trace to
  the old splash image;
* commit and push the change, not forgetting to mention again the author and the
  license in the commit message.

## Logo

The logo is used on the website, as application icon, inside the software
itself (sometimes in bigger format, such as in the About dialog, sometimes as
icon, etc.) and on various communication channels.

For the current logo and its usage rules, ckeck out our [logo usage
guidelines](logo/README.md).

For nostalgic people, here are the previous [logos and About dialog images](logo-log.md).

### Update procedure

When updating the logo, we also need to regenerate some files which are
committed in this repository because of circular dependencies (GIMP needs them
at compilation, but they need GIMP to be generated). Therefore a few additional
steps are needed:

1. Update the source image (TODO: exact file name(s) not chosen yet) in this
   repository and push your change.
2. From the main source repository, make sure that `gimp-data` subproject is
   updated by running: `meson subprojects update`
3. Build GIMP entirely locally from the main source repository: `ninja`
4. Install your local GIMP: `ninja install`
5. Still from the main source repository, run:
```sh
ninja gimp-data/images/logo/gimp.ico
ninja gimp-data/images/logo/fileicon.ico
ninja gimp-data/images/logo/plug-ins.ico
```
6. The command will have updated `gimp.ico`, `fileicon.ico` and `plug-ins.ico`
   directly in the source repository, under `images/logo/`. Verify the created
   files.
7. If the `.ico` files look fine, commit these updated files and push.
