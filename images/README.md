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

- [ ] The artwork must be submitted under a Libre License.
      Accepted licenses:
      [CC by-sa](https://creativecommons.org/licenses/by-sa/4.0/)
      [CC by](https://creativecommons.org/licenses/by/4.0/),
      [CC 0](https://creativecommons.org/publicdomain/zero/1.0/),
      or [Free Art](https://artlibre.org/licence/lal/en/).
- [ ] XCF file must be provided.
- [ ] Minimum size: full HD (splash images will be scaled down to 1/2
      of the main display when too big; but they won't be scaled up.
      Therefore anything smaller than fullHD will look tiny and
      unsuited on a 4K or higher res display). Though aspect ratio is not a hard
      requirement, the common 16:9 ratio is recommended.
- [ ] Loading text will appear in bottom quarter, either in black or in white
      depending on the overall surrounding lightness, so image contents must be
      adapted.

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

For nostalgic people, here are the previous [logos and About dialog images](logo-log.md).
