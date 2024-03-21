# GIMP logo: usage guidelines
## About this guide

This document explain how we should use GIMP logo in the software, various
documents or goodies. Other projects would usually call such a document
"branding guidelines", except that in GIMP community, we are not into enforcing
strong branding guidelines to others. So this document is for ourselves mostly
and how we could use our logo in and out of GIMP.

While these guidelines apply to us, we welcome anyone else to make their own
version of Wilber (the name of our logo/mascot), or even modify our logo for
their use case if they wish so (our logo is CC by-sa 4.0). Yet when reusing or
modifying our logo, we ask of people not to pretend to be the GIMP team if you
are not. We remind that impersonating others is illegal in most, if not all,
countries.

## Main logo

Our current logo (`gimp-logo.svg`) is flat and simple in its base form. Unlike
our [previous version](../logo-log.md), it doesn't simulate depth:

![Main (color) logo](gimp-logo.svg)

An important part was keeping the recognizability of Wilber, our mascot, as well
as its smile and friendliness. The goal is to show both professionalism through
a new logo following nowadays trends on minimalism, while keeping the welcoming
side of GIMP as a community project rather than showing a corporate image.

The 5 colors used in the main logo are:

1. Face and lower part of bristles: `#8c8073`
2. Brush wood (handle): `#f2840`
3. Brush metal ring (ferrule): `#bbbbbb`
4. Top part of brush hair (bristles), iris and nose: `#000000`
5. White of Wilber eye and light reflection (on eye, nose): `#ffffff`

There is some play with negative space in the separation between face
and nose.

This flat logo is used for desktop usage (menus, GNOME overview, title
bar, etc.).

### Usage on similar or unknown background color (e.g. in-Software)

When the background color is unknown, it may end up being the same (or
close) color as Wilber itself (or any other component of the logo). Therefore
part of the logo would blend in with the background and become invisible.

*Note: older versions of the previous logo used to have an [outlined
variant](https://gitlab.gnome.org/GNOME/gimp/-/blob/f59f8a3df261567bd8c59f0f3d82fac0c9e38a3b/icons/Legacy/scalable/gimp-wilber-outline.svg)
for similar use cases. We don't do this anymore. Having a shadow is much more
subtle and works a lot better, even in the extreme cases where the background
color was the exact same color as Wilber's face, and even for smaller icon
sizes.*

The typical case for this is when the logo is used in GIMP itself, since
the background color of the application can be anything (it depends on theme
colors and third-party themes are allowed).

In such a case, we use a variant of the logo with a slight shadow
(`gimp-logo-shadow.svg`):

![Color logo with a shadow](gimp-logo-shadow.svg)

This is the logo variant used in GIMP's About dialog or for the color icons of
GIMP.

### Bristles color is meant to change

The bristles color is meant to be interchangeable with any color you
wish, as though various paints was poured on it.

This concept is not currently used in GIMP itself, nor on any website, but this
is part of the base design.

## Specific OS variants

Some OSes have specific design guidelines.

### macOS

macOS recommends to embed icons in a [rounded-rectangle
shape](https://developer.apple.com/design/human-interface-guidelines/app-icons#macOS)
while giving some freedom on designing a bit said shape into familiar
skeuomorphic shapes.

On this OS, we make our rounded background shape look like a canvas supported on
an easel and we display Wilber over it.

![macOS logo](gimp-logo-macos.svg)

This logo doesn't render well on small sizes and in particular, the easel legs
would not be obvious. So for any icon under 48×48, we use a simplified variant,
only using the rounded shape, but no easel design:

![macOS logo under 48×48 pixels](gimp-logo-macos-small.svg)

### Windows

Our main logo is used as-is on Windows. The logo has been designed to
make sure it renders well according to Microsoft's [Design guidelines
for Windows app
icons](https://learn.microsoft.com/en-us/windows/apps/design/style/iconography/app-icon-design)
too.

## Grayscale/unicolor variant

In pure grayscale, we provide a simpler unicolor variant
(`gimp-wilber-unicolor.svg`) which is preferred to making the main logo
grayscale:

<img style="background-color:white" src="gimp-logo-unicolor.svg" alt="GIMP's Unicolor logo" />

In this variant, the brush handle is simply removed, relying even more
on negative space design, giving some further elegance to the logo.

This logo can render pretty well on documents meant to be grayscale, both
printed documents or on screen, or on goodies printed with a single color.

It would also work very well for silk-screen printing.

*Note: since the unicolor logo is made to be recolored, this participated a lot
to the design of the eyes of Wilber (in all versions) which needed to not look
scary whatever the color variant. Nevertheless it is designed in particular for
being printed as a dark color on a light background.*

## Outline variant

This logo variant can be used typically for simple usages when you want
to display or print logo as outlines. Except for the bristles, the whole shape
is made of connected lines.

<img style="background-color:rgb(54,53,58)" src="gimp-logo-outline.svg" alt="Outline variant of GIMP logo" />

A very good example would be to create embroidered t-shirts with the
official logo.

## Icon

The logo is sometimes used as an icon for buttons, tabs or other parts of the
graphical interface.

### Color Icon

While our `Legacy` icon theme still uses the old Wilber, with
pixel-perfect variants for the smallest size and simplified vector
variant for intermediate sizes, our new logo being much simpler on
purpose, a single SVG is used for every size in the Color variant of the
`Default` icon theme.

In particular, we use the [main logo with shadow](#usage-on-similar-or-unknown-background-color-eg-in-software)
(`gimp-logo-shadow.svg`), so that the icon is always visible, whatever the
background color of the chosen theme.

As example of icon resizing:

* Color icon in 48×48: <img src="gimp-logo-shadow.svg" width="48" height="48"/>
* Color icon in 16×16: <img src="gimp-logo-shadow.svg" width="16" height="16"/>

### Symbolic Icon

The Symbolic variant uses a [dedicated symbolic version of the
logo](#grayscaleunicolor-variant) (`gimp-wilber-symbolic.svg`) for all sizes.
The symbolic variant is similar to the unicolor variant except that the eyes and
noses are colored with the `!important` property, ensuring that GIMP does not
recolor these parts of the design.
This way, we avoid the problem of Wilber seemingly looking up because the
highlight in the eyes becomes black and therefore looks like an iris.

As example of icon resizing:

* Symbolic icon in 48×48: <img src="gimp-wilber-symbolic.svg" width="48" height="48"/>
* Symbolic icon in 16×16: <img src="gimp-wilber-symbolic.svg" width="16" height="16"/>

### Wilber-eek

For errors in GIMP, we have a specific variant of the logo used as icon:

TODO

## Background (accent) color preferences

Freedesktop's AppStream specifications introduced the concept of ["brand
colors"](https://freedesktop.org/software/appstream/docs/chap-Metadata.html#tag-branding)
(also called "accent colors" sometimes), which are basically background colors
to be used behind the logo in either dark or light color schemes.

GNOME [announced making use](https://docs.flathub.org/blog/introducing-app-brand-colors/)
of this metadata in 2024 and Elementary OS and Pop!OS have been using this
concept [since longer](https://github.com/ximion/appstream/issues/187).

Other desktop environments or OSes may have similar concepts.

We don't have such accent colors defined for GIMP logo yet. TODO.

## Fun variations of the logo

We also make fun "development logo" variants (`gimp-devel-logo.svg`), which are
only used in unstable builds, as the name implies. Our current development logo
is:

![Color development logo](gimp-devel-logo.svg)

It features rainbow colors being spilled on Wilber, like a scientific
experiment with colors, to represent the experimental side of Wilber. This logo
still features Wilber in gray with an older brush using negative space design,
as was originally proposed.

The development logo is swapped with the main logo in the About dialog
of unstable versions. It is not used as "app logo" in menus, title bars and so
on.

**Development logos are not to be considered "official".** They are only fun
experimental Wilbers, which can be swapped anytime as examples of variations of
Wilber.

*Note: currently the development logo only comes with a shadow since the
software is the only place we use it so far.*

## Licensing

The icons are authored by Aryeom, with feedbacks from various contributors, in
particular (in alphabetical order): Jehan, Øyvind Kolås, Simon Budig and Ville
Pätsi.

Remarks from people on Gitlab were also taken into account during design
editing.

The files are licensed under [Creative Commons Attribution-ShareAlike 4.0
International](https://creativecommons.org/licenses/by-sa/4.0/).

## References

* [Design guidelines for Windows app icons](https://learn.microsoft.com/en-us/windows/apps/design/style/iconography/app-icon-design)
* [Apple Human Interface Guidelines: App icons](https://developer.apple.com/design/human-interface-guidelines/app-icons#Specifications)
  (only macOS is supported so far)
* [GNOME Human Interface Guidelines: App Icons](https://developer.gnome.org/hig/guidelines/app-icons.html)
* [KDE Human Interface Guidelines: Icon Design](https://develop.kde.org/hig/style/icons/)
