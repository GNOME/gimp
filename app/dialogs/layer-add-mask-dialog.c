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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "layer-add-mask-dialog.h"

#include "gimp-intl.h"


typedef struct _LayerAddMaskDialog LayerAddMaskDialog;

struct _LayerAddMaskDialog
{
  GList               *layers;
  GimpAddMaskType      add_mask_type;
  GimpChannel         *channel;
  gboolean             invert;
  GimpAddMaskCallback  callback;
  gpointer             user_data;
};


/*  local function prototypes  */

static void       layer_add_mask_dialog_free             (LayerAddMaskDialog *private);
static void       layer_add_mask_dialog_response         (GtkWidget          *dialog,
                                                          gint                response_id,
                                                          LayerAddMaskDialog *private);
static gboolean   layer_add_mask_dialog_channel_selected (GimpContainerView  *view,
                                                          GList              *viewables,
                                                          GList              *paths,
                                                          LayerAddMaskDialog *private);


/*  public functions  */

GtkWidget *
layer_add_mask_dialog_new (GList               *layers,
                           GimpContext         *context,
                           GtkWidget           *parent,
                           GimpAddMaskType      add_mask_type,
                           gboolean             invert,
                           GimpAddMaskCallback  callback,
                           gpointer             user_data)
{
  LayerAddMaskDialog *private;
  GtkWidget          *dialog;
  GtkWidget          *vbox;
  GtkWidget          *frame;
  GtkWidget          *combo;
  GtkWidget          *button;
  GimpImage          *image;
  GimpChannel        *channel;
  GList              *channels;
  gchar              *title;
  gchar              *desc;
  gint                n_layers = g_list_length (layers);

  g_return_val_if_fail (layers, NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  private = g_slice_new0 (LayerAddMaskDialog);

  private->layers        = layers;
  private->add_mask_type = add_mask_type;
  private->invert        = invert;
  private->callback      = callback;
  private->user_data     = user_data;

  title = ngettext ("Add Layer Mask", "Add Layer Masks", n_layers);
  title = g_strdup_printf (title, n_layers);
  desc  = ngettext ("Add a Mask to the Layer", "Add Masks to %d Layers", n_layers);
  desc  = g_strdup_printf (desc, n_layers);

  dialog = gimp_viewable_dialog_new (layers, context,
                                     title,
                                     "gimp-layer-add-mask",
                                     GIMP_ICON_LAYER_MASK,
                                     desc,
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_LAYER_MASK_ADD,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Add"),    GTK_RESPONSE_OK,

                                     NULL);

  g_free (title);
  g_free (desc);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) layer_add_mask_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (layer_add_mask_dialog_response),
                    private);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame =
    gimp_enum_radio_frame_new (GIMP_TYPE_ADD_MASK_TYPE,
                               gtk_label_new (_("Initialize Layer Mask to:")),
                               G_CALLBACK (gimp_radio_button_update),
                               &private->add_mask_type, NULL,
                               &button);
  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button),
                                   private->add_mask_type);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  image = gimp_item_get_image (GIMP_ITEM (layers->data));

  combo = gimp_container_combo_box_new (gimp_image_get_channels (image),
                                        context,
                                        GIMP_VIEW_SIZE_SMALL, 1);
  gimp_enum_radio_frame_add (GTK_FRAME (frame), combo,
                             GIMP_ADD_MASK_CHANNEL, TRUE);
  gtk_widget_show (combo);

  g_signal_connect (combo, "select-items",
                    G_CALLBACK (layer_add_mask_dialog_channel_selected),
                    private);

  channels = gimp_image_get_selected_channels (image);
  if (channels)
    /* Mask dialog only requires one channel. Just take any of the
     * selected ones randomly.
     */
    channel = channels->data;
  else
    channel = GIMP_CHANNEL (gimp_container_get_first_child (gimp_image_get_channels (image)));

  gimp_container_view_select_item (GIMP_CONTAINER_VIEW (combo),
                                   GIMP_VIEWABLE (channel));

  button = gtk_check_button_new_with_mnemonic (_("In_vert mask"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), private->invert);
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->invert);

  return dialog;
}


/*  private functions  */

static void
layer_add_mask_dialog_free (LayerAddMaskDialog *private)
{
  g_slice_free (LayerAddMaskDialog, private);
}

static void
layer_add_mask_dialog_response (GtkWidget          *dialog,
                                gint                response_id,
                                LayerAddMaskDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (private->layers->data));

      if (private->add_mask_type == GIMP_ADD_MASK_CHANNEL &&
          ! private->channel)
        {
          gimp_message_literal (image->gimp,
                                G_OBJECT (dialog), GIMP_MESSAGE_WARNING,
                                _("Please select a channel first"));
          return;
        }

      private->callback (dialog,
                         private->layers,
                         private->add_mask_type,
                         private->channel,
                         private->invert,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static gboolean
layer_add_mask_dialog_channel_selected (GimpContainerView  *view,
                                        GList              *viewables,
                                        GList              *paths,
                                        LayerAddMaskDialog *private)
{
  g_return_val_if_fail (g_list_length (viewables) < 2, FALSE);

  private->channel = viewables? GIMP_CHANNEL (viewables->data) : NULL;

  return TRUE;
}
