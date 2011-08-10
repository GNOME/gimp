/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-offset.h"
#include "core/gimpitem.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "offset-dialog.h"

#include "gimp-intl.h"


#define WRAP_AROUND  (1 << 3)
#define FILL_MASK    (GIMP_OFFSET_BACKGROUND | GIMP_OFFSET_TRANSPARENT)


typedef struct
{
  GimpContext     *context;

  GtkWidget       *dialog;
  GimpUnitEntries *offset_entries;

  GimpOffsetType   fill_type;

  GimpImage       *image;
} OffsetDialog;


/*  local function prototypes  */

static void  offset_response         (GtkWidget    *widget,
                                      gint          response_id,
                                      OffsetDialog *dialog);
static void  offset_half_xy_callback (GtkWidget    *widget,
                                      OffsetDialog *dialog);
static void  offset_half_x_callback  (GtkWidget    *widget,
                                      OffsetDialog *dialog);
static void  offset_half_y_callback  (GtkWidget    *widget,
                                      OffsetDialog *dialog);
static void  offset_dialog_free      (OffsetDialog *dialog);


/*  public functions  */

GtkWidget *
offset_dialog_new (GimpDrawable *drawable,
                   GimpContext  *context,
                   GtkWidget    *parent)
{
  GimpItem       *item;
  OffsetDialog   *dialog;
  GtkWidget      *main_vbox;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *button;
  GtkWidget      *frame;
  GtkWidget      *radio_button;
  gdouble         xres;
  gdouble         yres;
  const gchar     *title = NULL;
  GimpUnitEntries *entries;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  dialog = g_slice_new0 (OffsetDialog);

  dialog->context   = context;
  dialog->fill_type = gimp_drawable_has_alpha (drawable) | WRAP_AROUND;
  item = GIMP_ITEM (drawable);
  dialog->image     = gimp_item_get_image (item);

  gimp_image_get_resolution (dialog->image, &xres, &yres);

  if (GIMP_IS_LAYER (drawable))
    title = _("Offset Layer");
  else if (GIMP_IS_LAYER_MASK (drawable))
    title = _("Offset Layer Mask");
  else if (GIMP_IS_CHANNEL (drawable))
    title = _("Offset Channel");
  else
    g_warning ("%s: unexpected drawable type", G_STRFUNC);

  dialog->dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (drawable), context,
                              _("Offset"), "gimp-drawable-offset",
                              GIMP_STOCK_TOOL_MOVE,
                              title,
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_LAYER_OFFSET,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              /*  offset, used as a verb  */
                              _("_Offset"),     GTK_RESPONSE_OK,

                              NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog->dialog),
                     (GWeakNotify) offset_dialog_free, dialog);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (offset_response),
                    dialog);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog->dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*  The offset frame  */
  frame = gimp_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  dialog->offset_entries = entries = gimp_unit_entries_new ();
  
  gimp_unit_entries_add_entry (entries, GIMP_UNIT_ENTRIES_OFFSET_X, _("X:"));
  gimp_unit_entries_add_entry (entries, GIMP_UNIT_ENTRIES_OFFSET_Y, _("X:"));

  gtk_box_pack_start (GTK_BOX (vbox), gimp_unit_entries_get_table (entries), FALSE, FALSE, 0);

  gimp_unit_entries_set_unit (entries, GIMP_UNIT_PIXEL);

  gimp_unit_entry_set_resolution (gimp_unit_entries_get_entry (entries, GIMP_UNIT_ENTRIES_OFFSET_X),
                                  xres);
  gimp_unit_entry_set_resolution (gimp_unit_entries_get_entry (entries, GIMP_UNIT_ENTRIES_OFFSET_Y),
                                  yres);

  gimp_unit_entry_set_bounds (gimp_unit_entries_get_entry (entries, GIMP_UNIT_ENTRIES_OFFSET_X),
                              GIMP_UNIT_PIXEL,
                              gimp_item_get_width (item),
                              - gimp_item_get_width (item));
  gimp_unit_entry_set_bounds (gimp_unit_entries_get_entry (entries, GIMP_UNIT_ENTRIES_OFFSET_Y),
                              GIMP_UNIT_PIXEL,
                              gimp_item_get_height (item),
                              - gimp_item_get_height (item));

  button = gtk_button_new_with_mnemonic (_("By width/_2, height/2"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_half_xy_callback),
                    dialog);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic ("By _width/2");
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_half_x_callback),
                    dialog);

  button = gtk_button_new_with_mnemonic ("By _height/2");
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (offset_half_y_callback),
                    dialog);

  /*  The edge behavior frame  */
  frame = gimp_int_radio_group_new (TRUE, _("Edge Behavior"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &dialog->fill_type, dialog->fill_type,

                                    _("W_rap around"),
                                    WRAP_AROUND, NULL,

                                    _("Fill with _background color"),
                                    GIMP_OFFSET_BACKGROUND, NULL,

                                    _("Make _transparent"),
                                    GIMP_OFFSET_TRANSPARENT, &radio_button,
                                    NULL);

  if (! gimp_drawable_has_alpha (drawable))
    gtk_widget_set_sensitive (radio_button, FALSE);

  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  return dialog->dialog;
}


/*  private functions  */

static void
offset_response (GtkWidget    *widget,
                 gint          response_id,
                 OffsetDialog *dialog)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpImage *image = dialog->image;

      if (image)
        {
          GimpDrawable *drawable = gimp_image_get_active_drawable (image);
          gint          offset_x;
          gint          offset_y;

          offset_x =
            RINT (gimp_unit_entries_get_pixels (GIMP_UNIT_ENTRIES (dialog->offset_entries),
                                                GIMP_UNIT_ENTRIES_OFFSET_X));
          offset_y =
            RINT (gimp_unit_entries_get_pixels (GIMP_UNIT_ENTRIES (dialog->offset_entries),
                                                GIMP_UNIT_ENTRIES_OFFSET_Y));

          gimp_drawable_offset (drawable,
                                dialog->context,
                                dialog->fill_type & WRAP_AROUND ? TRUE : FALSE,
                                dialog->fill_type & FILL_MASK,
                                offset_x, offset_y);
          gimp_image_flush (image);
        }
    }

  gtk_widget_destroy (dialog->dialog);
}

static void
offset_half_xy_callback (GtkWidget    *widget,
                         OffsetDialog *dialog)
{
  GimpImage *image = dialog->image;

  if (image)
    {
      GimpItem *item = GIMP_ITEM (gimp_image_get_active_drawable (image));

      gimp_unit_entries_set_pixels (GIMP_UNIT_ENTRIES (dialog->offset_entries),
                                    GIMP_UNIT_ENTRIES_OFFSET_X,
                                    gimp_item_get_width (item) / 2);
      gimp_unit_entries_set_pixels (GIMP_UNIT_ENTRIES (dialog->offset_entries),
                                    GIMP_UNIT_ENTRIES_OFFSET_Y,
                                    gimp_item_get_height (item) / 2);
   }
}

static void
offset_half_x_callback (GtkWidget    *widget,
                        OffsetDialog *dialog)
{
  GimpImage *image = dialog->image;

  if (image)
    {
      GimpItem *item = GIMP_ITEM (gimp_image_get_active_drawable (image));

      gimp_unit_entries_set_pixels (GIMP_UNIT_ENTRIES (dialog->offset_entries),
                                    GIMP_UNIT_ENTRIES_OFFSET_X,
                                    gimp_item_get_width (item) / 2);
   }
}

static void
offset_half_y_callback (GtkWidget    *widget,
                        OffsetDialog *dialog)
{
  GimpImage *image = dialog->image;

  if (image)
    {
      GimpItem *item = GIMP_ITEM (gimp_image_get_active_drawable (image));

      gimp_unit_entries_set_pixels (GIMP_UNIT_ENTRIES (dialog->offset_entries),
                                    GIMP_UNIT_ENTRIES_OFFSET_Y,
                                    gimp_item_get_height (item) / 2);
   }
}

static void
offset_dialog_free (OffsetDialog *dialog)
{
  g_slice_free (OffsetDialog, dialog);
}
