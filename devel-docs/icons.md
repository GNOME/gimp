# Icon themes for GIMP

## Released Themes

GIMP 3.0 comes with 3 icon themes:

1. **Symbolic**: the default icon theme which is vector and which will
   be automatically recolored to your theme colors.

   We follow [GNOME
   guidelines](https://developer.gnome.org/hig/guidelines/ui-icons.html)
   when possible.

2. **Color**: the color icon theme, also designed with vector graphics,
   yet it won't be recolored.

3. **Legacy**: icon theme which contains the old GIMP 2.8's raster
   icons (mostly untouched ever since GIMP 2.10). It is not maintained
   anymore and we are not expecting new icons for Legacy. Yet since we
   keep them in the source tree for now, we would accept updates.

The Symbolic icon theme is our main target since they are considered
better suited for graphics work (less visual distraction). Color icons
are kept as fall-back since some users prefer them.

Vector icons are now prefered because they are much less maintenance.
For instance, we do not need to double, triple (or more) every icon for
the various sizes they are needed in, and double this amount again to
handle high density displays.
Yet if anyone cared enough for a complete raster icon theme to get the
*Legacy* icon theme back in shape, add high density icon variants and
stay around, it could even get back to maintenance state. Be aware it is
a lot of work and we'd expect contributors ready to maintain support to
the icons as the software evolve.

## Adding new icons

- Add new icons in the single SVG file inside their respective
  directories, i.e.
  [symbolic-scalable.svg](/icons/Symbolic/symbolic-scalable.svg) for
  symbolic icons and
  [color-scalable.svg](icons/Color/color-scalable.svg) for color icons.

  A single file allows easier reuse of material, and easy overview of
  all existing icons which simplifies consistent styling…

- The contents of the SVG file should be organized for easy management
  and easy contribution. You can visually group similar icons, make use
  of layers, whatever is necessary for organization.

- You should group all parts of a single icon into a single object and
  id this object with the icon name. For instance the object containing
  the Move Tool icon should be id-ed: "gimp-tool-move".

- Make sure the object has the right expected size. A good trick is to
  group with a square of the right size, made invisible.

- Export the icon as SVG into the `scalable/` directory.

Ideally this step should be done at build time, but we could not find
yet a reliable way to extract icons out of the single SVG file without
using crazy build dependencies (like Inkscape). So this is done by hand
for the time being.

Please make sure that you provide both the Symbolic as well as the Color
icons. You are welcome to add a raster version for Legacy, but this is
not mandatory anymore.

- Add the icons in relevant listing files in `icons/icon-lists/` then run
  `touch icons/Color/meson.build icons/Symbolic/meson.build` to force-trigger
  their re-processing (hence re-configuration) at next build. Otherwise even
  with image list changed, meson might not see it as it uses the list from the
  last configuration.

### Pixel perfection

Even as vector images, icons could be pixel-perfect when possible.
Therefore the first step before making an icon is to determine which
size it is supposed to appear at.
If the icon could appear in several sizes:

- if the sizes are multiples, just design the smaller size. The bigger
icon will stay pixel-perfect when scaled by a multiple. So for instance,
if you want the icon to be 12x12 and 24x24, just design the 12x12 icon.

- of course, if the size difference is big enough, you may want to
create a new version with added details, even when this is a multiple
(i.e. 12x12 and 192x192 may be different designs). These are design
choices.

- when sizes are no multiple (i.e. 16x16 and 24x24), it is preferred to
have 2 pixel-perfect versions.

- if time is missing, creating the smaller size only is a first step
and is acceptable.

Note that since our maintained icons are currently vector, we only
design them once and scale the icon for all sizes at runtime. This is an
easy-maintenance choice. We are not against pixel-perfection, even of
vector icons, but once again if a contributor wants to embark in such a
journey, we'd expect them to stay for continuous maintenance.

### Colors in Symbolic icon theme

By default, colors in the Symbolic icon theme don't matter as they will
be changed by the foreground and background colors of the theme. Yet it
is still a good idea to use the same colors for all icons in
`icons/Symbolic/symbolic-scalable.svg` to keep visual consistency when
reviewing icons.

Furthermore, there is a trick to apply hard-coded colors (i.e. which
won't be recolored): add the `!important` flag to the color in the
`style` SVG parameter. Then GTK will not recolor this color.

It is to be noted that (last we tested), Inkscape was not able to keep
this flag, so you will likely have to edit the file manually in a text
or XML editor.

For instance
"[gimp-default-colors](icons/Symbolic/scalable/gimp-default-colors-symbolic.svg)"
![gimp-default-colors](icons/Symbolic/scalable/gimp-default-colors-symbolic.svg)
and
"[gimp-toilet-paper](icons/Symbolic/scalable/gimp-toilet-paper-symbolic.svg)"
![gimp-toilet-paper](icons/Symbolic/scalable/gimp-toilet-paper-symbolic.svg)
icons contain such tricks.
For the first one, the default colors was black and white in this
specific order (it made no sense to invert them or worse to transform
them into whatever other colors the theme might be using). For the
second, it was considered inappropriate by some contributors to generate
black toilet papers.

Other such examples are
[gimp-color-picker-black](icons/Symbolic/scalable/gimp-color-picker-black-symbolic.svg),
![gimp-color-picker-black](icons/Symbolic/scalable/gimp-color-picker-black-symbolic.svg)
[gimp-color-picker-gray](icons/Symbolic/scalable/gimp-color-picker-gray-symbolic.svg)
![gimp-color-picker-gray](icons/Symbolic/scalable/gimp-color-picker-gray-symbolic.svg)
and
[gimp-color-picker-white](icons/Symbolic/scalable/gimp-color-picker-white-symbolic.svg).
![gimp-color-picker-white](icons/Symbolic/scalable/gimp-color-picker-white-symbolic.svg).
Since they are designing specific colors, it doesn't make sense to let
any recoloring happen.

### Sizes

Some known sizes:

- tool icons: 16x16 and 22x22.
- dock tab icons: 16x16 and 24x24.
- menu icons: 16x16.
[…]

## Testing icons
### Showing menu icons and buttons

Menu items and buttons are not supposed to have icons any longer (except
for buttons with no label at all). Yet our actions have icons and some
desktop environments would enable them in menus and buttons regardless.
To test how it looks on systems which do so, set the environment
variable `GIMP_ICONS_LIKE_A_BOSS`.

For instance, start GIMP like this:

    GIMP_ICONS_LIKE_A_BOSS=1 gimp-2.99

### Playing with low/high density

To test high (or low) density icons, without having to change the
scaling factor of your whole desktop, just change the `GDK_SCALE`
environment variable.
For instance, run GIMP like this to simulate a scaling factor of 2
(every icons and text would typically double):

    GDK_SCALE=2 gimp-2.99
