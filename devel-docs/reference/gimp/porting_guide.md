Title: GIMP 3 plug-in porting guide

GIMP 3 plug-in porting guide
============================
Here you'll find documentation useful for porting older GIMP
plug-ins, especially Python ones, to the GIMP 3.0 APIs.

Useful Modules/Classes in GIMP 3.0+
------------------------------------
Here's a guide to the modules you're likely to need.
It's a work in progress: feel free to add to it.

Eventually we'll have online documentation for these classes.
In the meantime, you can generate your own:

```sh
HTMLDOCDIR=/path/to/doc/dir
g-ir-doc-tool -I /path/to/share/gir-1.0/ --language=Python -o $HTMLDOCDIR Gimp-3.0.gir
```

Then browse $HTMLDOCDIR with yelp, or generate HTML from it:

```sh
cd $HTMLDOCDIR
yelp-build cache *.page
yelp-build html .
```

You can also get some information in GIMP's Python console with
*help(module)* or *help(object)*, and you can get a list of functions
with *dir(object)*.

### Gimp
The base module: almost everything is under Gimp.

### Gimp.Image
The image object.

Some operations that used to be PDB calls, like

```python
pdb.gimp_selection_layer_alpha(layer)
```

are now in the Image object, e.g.

```python
img.select_item(Gimp.ChannelOps.REPLACE, layer)
```

### Gimp.Layer
The layer object.

```python
fog = Gimp.Layer.new(image, name,
                     drawable.width(), drawable.height(), type, opacity,
                     Gimp.LayerMode.NORMAL)
```

### Gimp.Selection
Selection operations that used to be in the PDB, e.g.

```python
pdb.gimp_selection_none(img)
```

are now in the Gimp.Selection module, e.g.

```python
Gimp.Selection.none(img)
```

### Gimp.ImageType
A home for image types like RGBA, GRAY, etc:

```python
Gimp.ImageType.RGBA_IMAGE
```

### Gimp.FillType
e.g. `Gimp.FillType.TRANSPARENT`, `Gimp.FillType.BACKGROUND`

### Gimp.ChannelOps
The old channel op definitions in the gimpfu module, like

```
CHANNEL_OP_REPLACE
```
are now in their own module:

```
Gimp.ChannelOps.REPLACE
```

### Gimp.RGB
In legacy GIMP, plug-ins you could pass a simple list of integers,
like `(0, 0, 0)`.

In 3.0+, you should create a Gimp.RGB object:

```python
c = Gimp.RGB()
c.set(240.0, 180.0, 70.0)
```

or

```python
c.r = 0
c.g = 0
c.b = 0
c.a = 1
```

PDB equivalence
---------------
A table of old PDB calls, and their equivalents in the GIMP 3.0+ world.

This document is a work in progress. Feel free to add to it.

#### Undo/Context

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| gimp_undo_push_group_start       | image.undo_group_start() |
| gimp_undo_push_group_end         | image.undo_group_end() |
| gimp.context_push()              | Gimp.context_push()   |
| gimp.context_push()              | Gimp.context_push()   |
| gimp_context_get_background      | Gimp.context_get_background
| gimp_context_set_background      | Gimp.context_set_background

#### File load/save

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| gimp_file_load                   | Gimp.file_load |
| gimp_file_save                   | Gimp.file_save |

#### Selection operations

Selection operations are now in the Gimp.Selection class (except
a few in the Image class). E.g.

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| pdb.gimp_selection_invert(img)   | Gimp.Selection.invert(img) |
| pdb.gimp_selection_none(img)     | Gimp.Selection.none(img) |
| pdb.gimp_selection_layer_alpha(layer) | img.select_item(Gimp.ChannelOps.REPLACE, layer) |
| gimp_image_select_item           | img.select_item(channel_op, layer) |

#### Filling and Masks

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| Gimp.drawable_fill()             | layer.fill() |
| pdb.gimp_edit_fill(FILL_BACKGROUND)  | layer.edit_fill(Gimp.FillType.BACKGROUND) |
| gimp_layer_add_mask              | layer.add_mask
| gimp_layer_remove_mask           | layer.remove_mask

#### Miscellaneous and Non-PDB Calls

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| gimp_displays_flush              | Gimp.displays_flush
| gimp_image_insert_layer          | image.insert_layer


#### Plug-ins

Calling other plug-ins is trickier than before. The old

```python
pdb.script_fu_drop_shadow(img, layer, -3, -3, blur,
                          (0, 0, 0), 80.0, False)
```

becomes

```python
c = Gimp.RGB()
c.set(240.0, 180.0, 70.0)
Gimp.get_pdb().run_procedure('script-fu-drop-shadow',
                             [ Gimp.RunMode.NONINTERACTIVE,
                               GObject.Value(Gimp.Image, img),
                               GObject.Value(Gimp.Drawable, layer),
                               GObject.Value(GObject.TYPE_DOUBLE, -3),
                               GObject.Value(GObject.TYPE_DOUBLE, -3),
                               GObject.Value(GObject.TYPE_DOUBLE,blur),
                               c,
                               GObject.Value(GObject.TYPE_DOUBLE, 80.0),
                               GObject.Value(GObject.TYPE_BOOLEAN, False)
                             ])
```

Removed Functions
-----------------
These functions have been removed from GIMP 3. Most of them were deprecated
since GIMP 2.10.x or older versions. As we bump the major version, it is time
to start with a clean slate.

Below is a correspondence table with replacement function. The replacement is
not necessarily a direct search-and-replace equivalent. Some may have different
parameters, and in some case, it may require to think a bit about how things
work to reproduce the same functionality. Nevertheless everything which was
possible in the previous API is obviously still possible.

| Removed function                                | Replacement                                       |
| ----------------------------------------------- | ------------------------------------------------- |
| `gimp_attach_new_parasite()`                    | [func@Gimp.attach_parasite]                       |
| `gimp_brightness_contrast()`                    | [method@Gimp.Drawable.brightness_contrast]        |
| `gimp_brushes_get_brush()`                      | [func@Gimp.context_get_brush]                     |
| `gimp_brushes_get_brush_data()`                 | [func@Gimp.brush_get_pixels]                      |
| `gimp_brushes_get_spacing()`                    | [func@Gimp.brush_get_spacing]                     |
| `gimp_brushes_set_spacing()`                    | [func@Gimp.brush_set_spacing]                     |
| `gimp_by_color_select()`                        | [method@Gimp.Image.select_color]                  |
| `gimp_by_color_select_full()`                   | [method@Gimp.Image.select_color]                  |
| `gimp_channel_menu_new()`                       | `gimp_channel_combo_box_new()`                    |
| `gimp_color_balance()`                          | [method@Gimp.Drawable.color_balance]              |
| `gimp_color_display_convert()`                  | `gimp_color_display_convert_buffer()`             |
| `gimp_color_display_convert_surface()`          | `gimp_color_display_convert_buffer()`             |
| `gimp_color_display_stack_convert()`            | `gimp_color_display_stack_convert_buffer()`       |
| `gimp_color_display_stack_convert_surface()`    | `gimp_color_display_stack_convert_buffer()`       |
| `gimp_color_profile_combo_box_add()`            | `gimp_color_profile_combo_box_add_file()`         |
| `gimp_color_profile_combo_box_get_active()`     | `gimp_color_profile_combo_box_get_active_file()`  |
| `gimp_color_profile_combo_box_set_active()`     | `gimp_color_profile_combo_box_set_active_file()`  |
| `gimp_color_profile_store_add()`                | `gimp_color_profile_store_add_file()`             |
| `gimp_colorize()`                               | [method@Gimp.Drawable.colorize_hsl]               |
| `gimp_context_get_transform_recursion()`        | *N/A*                                             |
| `gimp_context_set_transform_recursion()`        | *N/A*                                             |
| `gimp_curves_explicit()`                        | [method@Gimp.Drawable.curves_explicit]            |
| `gimp_curves_spline()`                          | [method@Gimp.Drawable.curves_spline]              |
| `gimp_desaturate()`                             | [method@Gimp.Drawable.desaturate]                 |
| `gimp_desaturate_full()`                        | [method@Gimp.Drawable.desaturate]                 |
| `gimp_drawable_attach_new_parasite()`           | [method@Gimp.Item.attach_parasite]                |
| `gimp_drawable_bpp()`                           | [method@Gimp.Drawable.get_bpp]                    |
| `gimp_drawable_delete()`                        | [method@Gimp.Item.delete]                         |
| `gimp_drawable_get_image()`                     | [method@Gimp.Item.get_image]                      |
| `gimp_drawable_get_linked()`                    | *N/A*                                             |
| `gimp_drawable_get_name()`                      | [method@Gimp.Item.get_name]                       |
| `gimp_drawable_get_tattoo()`                    | [method@Gimp.Item.get_tattoo]                     |
| `gimp_drawable_get_visible()`                   | [method@Gimp.Item.get_visible]                    |
| `gimp_drawable_height()`                        | [method@Gimp.Drawable.get_height]                 |
| `gimp_drawable_is_channel()`                    | [method@Gimp.Item.is_channel]                     |
| `gimp_drawable_is_layer()`                      | [method@Gimp.Item.is_layer]                       |
| `gimp_drawable_is_layer_mask()`                 | [method@Gimp.Item.is_layer_mask]                  |
| `gimp_drawable_is_text_layer()`                 | [method@Gimp.Item.is_text_layer]                  |
| `gimp_drawable_is_valid()`                      | [method@Gimp.Item.is_valid]                       |
| `gimp_drawable_menu_new()`                      | `gimp_drawable_combo_box_new()`                   |
| `gimp_drawable_offsets()`                       | [method@Gimp.Drawable.get_offsets]                |
| `gimp_drawable_parasite_attach()`               | [method@Gimp.Item.attach_parasite]                |
| `gimp_drawable_parasite_detach()`               | [method@Gimp.Item.detach_parasite]                |
| `gimp_drawable_parasite_find()`                 | [method@Gimp.Item.get_parasite]                   |
| `gimp_drawable_parasite_list()`                 | [method@Gimp.Item.get_parasite_list]              |
| `gimp_drawable_preview_new()`                   | `gimp_drawable_preview_new_from_drawable()`       |
| `gimp_drawable_preview_new_from_drawable_id()`  | `gimp_drawable_preview_new_from_drawable()`       |
| `gimp_drawable_set_image()`                     | *N/A*                                             |
| `gimp_drawable_set_linked()`                    | *N/A*                                             |
| `gimp_drawable_set_name()`                      | [method@Gimp.Item.set_name]                       |
| `gimp_drawable_set_tattoo()`                    | [method@Gimp.Item.set_tattoo]                     |
| `gimp_drawable_set_visible()`                   | [method@Gimp.Item.set_visible]                    |
| `gimp_drawable_transform_2d()`                  | [method@Gimp.Item.transform_2d]                   |
| `gimp_drawable_transform_2d_default()`          | [method@Gimp.Item.transform_2d]                   |
| `gimp_drawable_transform_flip()`                | [method@Gimp.Item.transform_flip]                 |
| `gimp_drawable_transform_flip_default()`        | [method@Gimp.Item.transform_flip]                 |
| `gimp_drawable_transform_flip_simple()`         | [method@Gimp.Item.transform_flip_simple]          |
| `gimp_drawable_transform_matrix()`              | [method@Gimp.Item.transform_matrix]               |
| `gimp_drawable_transform_matrix_default()`      | [method@Gimp.Item.transform_matrix]               |
| `gimp_drawable_transform_perspective()`         | [method@Gimp.Item.transform_perspective]          |
| `gimp_drawable_transform_perspective_default()` | [method@Gimp.Item.transform_perspective]          |
| `gimp_drawable_transform_rotate()`              | [method@Gimp.Item.transform_rotate]               |
| `gimp_drawable_transform_rotate_default()`      | [method@Gimp.Item.transform_rotate]               |
| `gimp_drawable_transform_rotate_simple()`       | [method@Gimp.Item.transform_rotate_simple]        |
| `gimp_drawable_transform_scale()`               | [method@Gimp.Item.transform_scale]                |
| `gimp_drawable_transform_scale_default()`       | [method@Gimp.Item.transform_scale]                |
| `gimp_drawable_transform_shear()`               | [method@Gimp.Item.transform_shear]                |
| `gimp_drawable_transform_shear_default()`       | [method@Gimp.Item.transform_shear]                |
| `gimp_drawable_width()`                         | [method@Gimp.Drawable.get_width]                  |
| `gimp_edit_blend()`                             | [method@Gimp.Drawable.edit_gradient_fill]         |
| `gimp_edit_bucket_fill()`                       | [method@Gimp.Drawable.edit_bucket_fill]           |
| `gimp_edit_bucket_fill_full()`                  | [method@Gimp.Drawable.edit_bucket_fill]           |
| `gimp_edit_clear()`                             | [method@Gimp.Drawable.edit_clear]                 |
| `gimp_edit_fill()`                              | [method@Gimp.Drawable.edit_fill]                  |
| `gimp_edit_paste_as_new()`                      | `gimp_edit_paste_as_new_image()`                  |
| `gimp_edit_named_paste_as_new()`                | `gimp_edit_named_paste_as_new_image()`            |
| `gimp_edit_stroke()`                            | [method@Gimp.Drawable.edit_stroke_selection]      |
| `gimp_edit_stroke_vectors()`                    | [method@Gimp.Drawable.edit_stroke_item]           |
| `gimp_ellipse_select()`                         | [method@Gimp.Image.select_ellipse]                |
| `gimp_enum_combo_box_set_stock_prefix()`        | `gimp_enum_combo_box_set_icon_prefix()`           |
| `gimp_enum_stock_box_new()`                     | `gimp_enum_icon_box_new()`                        |
| `gimp_enum_stock_box_new_with_range()`          | `gimp_enum_icon_box_new_with_range()`             |
| `gimp_enum_stock_box_set_child_padding()`       | `gimp_enum_icon_box_set_child_padding()`          |
| `gimp_enum_store_set_stock_prefix()`            | `gimp_enum_store_set_icon_prefix()`               |
| `gimp_equalize()`                               | [method@Gimp.Drawable.equalize]                   |
| `gimp_flip()`                                   | [method@Gimp.Item.transform_flip_simple]          |
| `gimp_floating_sel_relax()`                     | *N/A*                                             |
| `gimp_floating_sel_rigor()`                     | *N/A*                                             |
| `gimp_free_select()`                            | [method@Gimp.Image.select_polygon]                |
| `gimp_fuzzy_select()`                           | [method@Gimp.Image.select_contiguous_color]       |
| `gimp_fuzzy_select_full()`                      | [method@Gimp.Image.select_contiguous_color]       |
| `gimp_gamma()`                                  | [method@Gimp.Drawable.get_format]                 |
| `gimp_get_icon_theme_dir()`                     | *N/A*                                             |
| `gimp_get_path_by_tattoo()`                     | [method@Gimp.Image.get_vectors_by_tattoo]         |
| `gimp_get_theme_dir()`                          | *N/A*                                             |
| `gimp_gradients_get_gradient_data()`            | [func@Gimp.gradient_get_uniform_samples]          |
| `gimp_gradients_sample_custom()`                | [func@Gimp.gradient_get_custom_samples]           |
| `gimp_gradients_sample_uniform()`               | [func@Gimp.gradient_get_uniform_samples]          |
| `gimp_histogram()`                              | [method@Gimp.Drawable.histogram]                  |
| `gimp_hue_saturation()`                         | [method@Gimp.Drawable.hue_saturation]             |
| `gimp_image_add_channel()`                      | [method@Gimp.Image.insert_channel]                |
| `gimp_image_add_layer()`                        | [method@Gimp.Image.insert_layer]                  |
| `gimp_image_add_vectors()`                      | [method@Gimp.Image.insert_vectors]                |
| `gimp_image_attach_new_parasite()`              | [method@Gimp.Image.attach_parasite]               |
| `gimp_image_base_type()`                        | [method@Gimp.Image.get_base_type]                 |
| `gimp_image_free_shadow()`                      | [method@Gimp.Drawable.free_shadow]                |
| `gimp_image_get_channel_position()`             | [method@Gimp.Image.get_item_position]             |
| `gimp_image_get_cmap()`                         | [method@Gimp.Image.get_colormap]                  |
| `gimp_image_get_layer_position()`               | [method@Gimp.Image.get_item_position]             |
| `gimp_image_get_vectors_position()`             | [method@Gimp.Image.get_item_position]             |
| `gimp_image_height()`                           | [method@Gimp.Image.get_height]                    |
| `gimp_image_lower_channel()`                    | [method@Gimp.Image.lower_item]                    |
| `gimp_image_lower_layer()`                      | [method@Gimp.Image.lower_item]                    |
| `gimp_image_lower_layer_to_bottom()`            | [method@Gimp.Image.lower_item_to_bottom]          |
| `gimp_image_lower_vectors()`                    | [method@Gimp.Image.lower_item]                    |
| `gimp_image_lower_vectors_to_bottom()`          | [method@Gimp.Image.lower_item_to_bottom]          |
| `gimp_image_menu_new()`                         | `gimp_image_combo_box_new()`                      |
| `gimp_image_parasite_attach()`                  | [method@Gimp.Image.attach_parasite]               |
| `gimp_image_parasite_detach()`                  | [method@Gimp.Image.detach_parasite]               |
| `gimp_image_parasite_find()`                    | [method@Gimp.Image.get_parasite]                  |
| `gimp_image_parasite_list()`                    | [method@Gimp.Image.get_parasite_list]             |
| `gimp_image_raise_channel()`                    | [method@Gimp.Image.raise_item]                    |
| `gimp_image_raise_layer()`                      | [method@Gimp.Image.raise_item]                    |
| `gimp_image_raise_layer_to_top()`               | [method@Gimp.Image.raise_item_to_top]             |
| `gimp_image_raise_vectors()`                    | [method@Gimp.Image.raise_item]                    |
| `gimp_image_raise_vectors_to_top()`             | [method@Gimp.Image.raise_item_to_top]             |
| `gimp_image_scale_full()`                       | [method@Gimp.Image.scale]                         |
| `gimp_image_set_cmap()`                         | [method@Gimp.Image.set_colormap]                  |
| `gimp_image_width()`                            | [method@Gimp.Image.get_width]                     |
| `gimp_install_cmap()`                           | *N/A*                                             |
| `gimp_invert()`                                 | [method@Gimp.Drawable.invert]                     |
| `gimp_item_get_linked()`                        | *N/A*                                             |
| `gimp_item_set_linked()`                        | *N/A*                                             |
| `gimp_layer_menu_new()`                         | `gimp_layer_combo_box_new()`                      |
| `gimp_layer_scale_full()`                       | `gimp_layer_scale()`                              |
| `gimp_layer_translate()`                        | [method@Gimp.Item.transform_translate]            |
| `gimp_levels()`                                 | [method@Gimp.Drawable.levels]                     |
| `gimp_levels_auto()`                            | [method@Gimp.Drawable.levels_stretch]             |
| `gimp_levels_stretch()`                         | [method@Gimp.Drawable.levels_stretch]             |
| `gimp_min_colors()`                             | *N/A*                                             |
| `gimp_palettes_get_palette()`                   | `gimp_context_get_palette()`                      |
| `gimp_palettes_get_palette_entry()`             | `gimp_palette_entry_get_color()`                  |
| `gimp_parasite_attach()`                        | `gimp_attach_parasite()`                          |
| `gimp_parasite_data()`                          | `gimp_parasite_get_data()`                        |
| `gimp_parasite_data_size()`                     | `gimp_parasite_get_data()`                        |
| `gimp_parasite_detach()`                        | `gimp_detach_parasite()`                          |
| `gimp_parasite_find()`                          | `gimp_get_parasite()`                             |
| `gimp_parasite_flags()`                         | `gimp_parasite_get_flags()`                       |
| `gimp_parasite_list()`                          | `gimp_get_parasite_list()`                        |
| `gimp_parasite_name()`                          | `gimp_parasite_get_name()`                        |
| `gimp_path_delete()`                            | [method@Gimp.Image.remove_vectors]                |
| `gimp_path_get_current()`                       | [method@Gimp.Image.get_active_vectors]            |
| `gimp_path_get_locked()`                        | *N/A*                                             |
| `gimp_path_get_points()`                        | `gimp_vectors_stroke_get_points()`                |
| `gimp_path_get_point_at_dist()`                 | `gimp_vectors_stroke_get_point_at_dist()`         |
| `gimp_path_get_tattoo()`                        | [method@Gimp.Item.get_tattoo]                     |
| `gimp_path_import()`                            | `gimp_vectors_import_from_file()`                 |
| `gimp_path_list()`                              | [method@Gimp.Image.get_vectors]                   |
| `gimp_path_set_current()`                       | [method@Gimp.Image.set_active_vectors]            |
| `gimp_path_set_locked()`                        | *N/A*                                             |
| `gimp_path_set_points()`                        | `gimp_vectors_stroke_new_from_points()`           |
| `gimp_path_set_tattoo()`                        | [method@Gimp.Item.set_tattoo]                     |
| `gimp_path_stroke_current()`                    | `gimp_edit_stroke_vectors()`                      |
| `gimp_path_to_selection()`                      | [method@Gimp.Image.select_item]                   |
| `gimp_patterns_get_pattern()`                   | `gimp_context_get_pattern()`                      |
| `gimp_patterns_get_pattern_data()`              | `gimp_pattern_get_pixels()`                       |
| `gimp_perspective()`                            | [method@Gimp.Item.transform_perspective]          |
| `gimp_posterize()`                              | [method@Gimp.Drawable.posterize]                  |
| `gimp_prop_enum_stock_box_new()`                | `gimp_prop_enum_icon_box_new()`                   |
| `gimp_prop_stock_image_new()`                   | `gimp_prop_icon_image_new()`                      |
| `gimp_prop_unit_menu_new()`                     | `gimp_prop_unit_combo_box_new()`                  |
| `gimp_rect_select()`                            | [method@Gimp.Image.select_rectangle]              |
| `gimp_rotate()`                                 | [method@Gimp.Item.transform_rotate]               |
| `gimp_round_rect_select()`                      | [method@Gimp.Image.select_round_rectangle]        |
| `gimp_scale()`                                  | [method@Gimp.Item.transform_scale]                |
| `gimp_selection_combine()`                      | [method@Gimp.Image.select_item]                   |
| `gimp_selection_layer_alpha()`                  | [method@Gimp.Image.select_item]                   |
| `gimp_selection_load()`                         | [method@Gimp.Image.select_item]                   |
| `gimp_shear()`                                  | [method@Gimp.Item.transform_shear]                |
| `gimp_stock_init()`                             | `gimp_icons_init()`                               |
| `gimp_text()`                                   | `gimp_text_fontname()`                            |
| `gimp_text_get_extents()`                       | `gimp_text_get_extents_fontname()`                |
| `gimp_text_layer_get_hinting()`                 | `gimp_text_layer_get_hint_style()`                |
| `gimp_text_layer_set_hinting()`                 | `gimp_text_layer_set_hint_style()`                |
| `gimp_threshold()`                              | [method@Gimp.Drawable.threshold]                  |
| `gimp_toggle_button_sensitive_update()`         | `g_object_bind_property()`                        |
| `gimp_transform_2d()`                           | [method@Gimp.Item.transform_2d]                   |
| `gimp_unit_menu_update()`                       | `#GimpUnitComboBox`                               |
| `gimp_vectors_get_image()`                      | [method@Gimp.Item.get_image]                      |
| `gimp_vectors_get_linked()`                     | *N/A*                                             |
| `gimp_vectors_get_name()`                       | [method@Gimp.Item.get_name]                       |
| `gimp_vectors_get_tattoo()`                     | [method@Gimp.Item.get_tattoo]                     |
| `gimp_vectors_get_visible()`                    | [method@Gimp.Item.get_visible]                    |
| `gimp_vectors_is_valid()`                       | [method@Gimp.Item.is_valid]                       |
| `gimp_vectors_parasite_attach()`                | [method@Gimp.Item.attach_parasite]                |
| `gimp_vectors_parasite_detach()`                | [method@Gimp.Item.detach_parasite]                |
| `gimp_vectors_parasite_find()`                  | [method@Gimp.Item.get_parasite]                   |
| `gimp_vectors_parasite_list()`                  | [method@Gimp.Item.get_parasite_list]              |
| `gimp_vectors_set_linked()`                     | *N/A*                                             |
| `gimp_vectors_set_name()`                       | [method@Gimp.Item.set_name]                       |
| `gimp_vectors_set_tattoo()`                     | [method@Gimp.Item.set_tattoo]                     |
| `gimp_vectors_set_visible()`                    | [method@Gimp.Item.set_visible]                    |
| `gimp_vectors_to_selection()`                   | [method@Gimp.Image.select_item]                   |
| `gimp_zoom_preview_get_drawable_id()`           | `gimp_zoom_preview_get_drawable()`                |
| `gimp_zoom_preview_new()`                       | `gimp_zoom_preview_new_from_drawable()`           |
| `gimp_zoom_preview_new_from_drawable_id()`      | `gimp_zoom_preview_new_from_drawable()`           |
| `gimp_zoom_preview_new_with_model()`            | `gimp_zoom_preview_new_with_model_from_drawable()`|
