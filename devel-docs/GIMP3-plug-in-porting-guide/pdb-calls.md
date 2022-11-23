# PDB equivalence

A table of old PDB calls, and their equivalents in the LIGMA 3.0+ world.

This document is a work in progress. Feel free to add to it.

## Undo/Context

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| ligma_undo_push_group_start       | image.undo_group_start() |
| ligma_undo_push_group_end         | image.undo_group_end() |
| ligma.context_push()              | Ligma.context_push()   |
| ligma.context_push()              | Ligma.context_push()   |
| ligma_context_get_background      | Ligma.context_get_background
| ligma_context_set_background      | Ligma.context_set_background

## File load/save

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| ligma_file_load                   | Ligma.file_load |
| ligma_file_save                   | Ligma.file_save |

## Selection operations

Selection operations are now in the Ligma.Selection class (except
a few in the Image class). E.g.

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| pdb.ligma_selection_invert(img)   | Ligma.Selection.invert(img) |
| pdb.ligma_selection_none(img)     | Ligma.Selection.none(img) |
| pdb.ligma_selection_layer_alpha(layer) | img.select_item(Ligma.ChannelOps.REPLACE, layer) |
| ligma_image_select_item           | img.select_item(channel_op, layer) |

## Filling and Masks

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| Ligma.drawable_fill()             | layer.fill() |
| pdb.ligma_edit_fill(FILL_BACKGROUND)  | layer.edit_fill(Ligma.FillType.BACKGROUND) |
| ligma_layer_add_mask              | layer.add_mask
| ligma_layer_remove_mask           | layer.remove_mask

## Miscellaneous and Non-PDB Calls

| Removed function                 | Replacement                   |
| -------------------------------- | ----------------------------
| ligma_displays_flush              | Ligma.displays_flush
| ligma_image_insert_layer          | image.insert_layer


## Plug-ins

Calling other plug-ins is trickier than before. The old
```
pdb.script_fu_drop_shadow(img, layer, -3, -3, blur,
                              (0, 0, 0), 80.0, False)
```
becomes
```
        c = Ligma.RGB()
        c.set(240.0, 180.0, 70.0)
        Ligma.get_pdb().run_procedure('script-fu-drop-shadow',
                                     [ Ligma.RunMode.NONINTERACTIVE,
                                       GObject.Value(Ligma.Image, img),
                                       GObject.Value(Ligma.Drawable, layer),
                                       GObject.Value(GObject.TYPE_DOUBLE, -3),
                                       GObject.Value(GObject.TYPE_DOUBLE, -3),
                                       GObject.Value(GObject.TYPE_DOUBLE,blur),
                                       c,
                                       GObject.Value(GObject.TYPE_DOUBLE, 80.0),
                                       GObject.Value(GObject.TYPE_BOOLEAN, False)
                                     ])
```
