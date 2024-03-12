# PDB equivalence

A table of old PDB calls, and their equivalents in the GIMP 3.0+ world.

This document is a work in progress. Feel free to add to it.

## Undo/Context

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| gimp_undo_push_group_start       | image.undo_group_start() |
| gimp_undo_push_group_end         | image.undo_group_end() |
| gimp.context_push()              | Gimp.context_push()   |
| gimp.context_push()              | Gimp.context_push()   |
| gimp_context_get_background      | Gimp.context_get_background
| gimp_context_set_background      | Gimp.context_set_background

## File load/save

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| gimp_file_load                   | Gimp.file_load |
| gimp_file_save                   | Gimp.file_save |

## Selection operations

Selection operations are now in the Gimp.Selection class (except
a few in the Image class). E.g.

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| pdb.gimp_selection_invert(img)   | Gimp.Selection.invert(img) |
| pdb.gimp_selection_none(img)     | Gimp.Selection.none(img) |
| pdb.gimp_selection_layer_alpha(layer) | img.select_item(Gimp.ChannelOps.REPLACE, layer) |
| gimp_image_select_item           | img.select_item(channel_op, layer) |

## Filling and Masks

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| Gimp.drawable_fill()             | layer.fill() |
| pdb.gimp_edit_fill(FILL_BACKGROUND)  | layer.edit_fill(Gimp.FillType.BACKGROUND) |
| gimp_layer_add_mask              | layer.add_mask
| gimp_layer_remove_mask           | layer.remove_mask

## Miscellaneous and Non-PDB Calls

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| gimp_displays_flush              | Gimp.displays_flush
| gimp_image_insert_layer          | image.insert_layer


## Plug-ins

Calling other plug-ins is trickier than before. The old
```
pdb.script_fu_drop_shadow(img, layer, -3, -3, blur,
                              (0, 0, 0), 80.0, False)
```
becomes
```
        c = Gegl.Color.new("black")
        c.set_rgba(0.94, 0.71, 0.27, 1.0)
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
