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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "item-options-dialog.h"

#include "gimp-intl.h"


typedef struct _ItemOptionsDialog ItemOptionsDialog;

struct _ItemOptionsDialog
{
  GimpImage               *image;
  GimpItem                *item;
  GimpContext             *context;
  gboolean                 visible;
  gboolean                 linked;
  GimpColorTag             color_tag;
  gboolean                 lock_content;
  gboolean                 lock_position;
  GimpItemOptionsCallback  callback;
  gpointer                 user_data;

  GtkWidget               *left_vbox;
  GtkWidget               *left_grid;
  gint                     grid_row;
  GtkWidget               *name_entry;
  GtkWidget               *right_frame;
  GtkWidget               *right_vbox;
  GtkWidget               *lock_position_toggle;
};


/*  local function prototypes  */

static void        item_options_dialog_free     (ItemOptionsDialog *private);
static void        item_options_dialog_response (GtkWidget         *dialog,
                                                 gint               response_id,
                                                 ItemOptionsDialog *private);
static GtkWidget * check_button_with_icon_new    (const gchar      *label,
                                                  const gchar      *icon_name,
                                                  GtkBox           *vbox);


/*  public functions  */

GtkWidget *
item_options_dialog_new (GimpImage               *image,
                         GimpItem                *item,
                         GimpContext             *context,
                         GtkWidget               *parent,
                         const gchar             *title,
                         const gchar             *role,
                         const gchar             *icon_name,
                         const gchar             *desc,
                         const gchar             *help_id,
                         const gchar             *name_label,
                         const gchar             *lock_content_icon_name,
                         const gchar             *lock_content_label,
                         const gchar             *lock_position_label,
                         const gchar             *item_name,
                         gboolean                 item_visible,
                         gboolean                 item_linked,
                         GimpColorTag             item_color_tag,
                         gboolean                 item_lock_content,
                         gboolean                 item_lock_position,
                         GimpItemOptionsCallback  callback,
                         gpointer                 user_data)
{
  ItemOptionsDialog *private;
  GtkWidget         *dialog;
  GimpViewable      *viewable;
  GtkWidget         *main_hbox;
  GtkWidget         *grid;
  GtkWidget         *button;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (item == NULL || GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (ItemOptionsDialog);

  private->image         = image;
  private->item          = item;
  private->context       = context;
  private->visible       = item_visible;
  private->linked        = item_linked;
  private->color_tag     = item_color_tag;
  private->lock_content  = item_lock_content;
  private->lock_position = item_lock_position;
  private->callback      = callback;
  private->user_data     = user_data;

  if (item)
    viewable = GIMP_VIEWABLE (item);
  else
    viewable = GIMP_VIEWABLE (image);

  dialog = gimp_viewable_dialog_new (viewable, context,
                                     title, role, icon_name, desc,
                                     parent,
                                     gimp_standard_help_func, help_id,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_OK"),     GTK_RESPONSE_OK,

                                     NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (item_options_dialog_response),
                    private);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) item_options_dialog_free, private);

  g_object_set_data (G_OBJECT (dialog), "item-options-dialog-private", private);

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_hbox, TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  private->left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), private->left_vbox, TRUE, TRUE, 0);
  gtk_widget_show (private->left_vbox);

  private->left_grid = grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (private->left_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /*  The name label and entry  */
  if (name_label)
    {
      GtkWidget *hbox;
      GtkWidget *radio;
      GtkWidget *radio_box;
      GList     *children;
      GList     *list;

      private->name_entry = gtk_entry_new ();
      gtk_entry_set_activates_default (GTK_ENTRY (private->name_entry), TRUE);
      gtk_entry_set_text (GTK_ENTRY (private->name_entry), item_name);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, private->grid_row++,
                                name_label, 0.0, 0.5,
                                private->name_entry, 1);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, private->grid_row++,
                                _("Color tag:"), 0.0, 0.5,
                                hbox, 1);

      radio_box = gimp_enum_radio_box_new (GIMP_TYPE_COLOR_TAG,
                                           G_CALLBACK (gimp_radio_button_update),
                                           &private->color_tag,
                                           &radio);
      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (radio),
                                       private->color_tag);

      children = gtk_container_get_children (GTK_CONTAINER (radio_box));

      for (list = children;
           list;
           list = g_list_next (list))
        {
          GimpColorTag  color_tag;
          GimpRGB       color;
          GtkWidget    *image;

          radio = list->data;

          g_object_ref (radio);
          gtk_container_remove (GTK_CONTAINER (radio_box), radio);
          g_object_set (radio, "draw-indicator", FALSE, NULL);
          gtk_box_pack_start (GTK_BOX (hbox), radio, FALSE, FALSE, 0);
          g_object_unref (radio);

          gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (radio)));

          color_tag = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (radio),
                                                          "gimp-item-data"));

          if (gimp_get_color_tag_color (color_tag, &color, FALSE))
            {
              gint w, h;

              image = gimp_color_area_new (&color, GIMP_COLOR_AREA_FLAT, 0);
              gimp_color_area_set_color_config (GIMP_COLOR_AREA (image),
                                                context->gimp->config->color_management);
              gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
              gtk_widget_set_size_request (image, w, h);
            }
          else
            {
              image = gtk_image_new_from_icon_name (GIMP_ICON_CLOSE,
                                                    GTK_ICON_SIZE_MENU);
            }

          gtk_container_add (GTK_CONTAINER (radio), image);
          gtk_widget_show (image);
        }

      g_list_free (children);
      gtk_widget_destroy (radio_box);
    }

  /*  The switches frame & vbox  */

  private->right_frame = gimp_frame_new (_("Switches"));
  gtk_box_pack_start (GTK_BOX (main_hbox), private->right_frame,
                      FALSE, FALSE, 0);
  gtk_widget_show (private->right_frame);

  private->right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (private->right_frame), private->right_vbox);
  gtk_widget_show (private->right_vbox);

  button = check_button_with_icon_new (_("_Visible"),
                                       GIMP_ICON_VISIBLE,
                                       GTK_BOX (private->right_vbox));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->visible);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->visible);

  button = check_button_with_icon_new (_("_Linked"),
                                       GIMP_ICON_LINKED,
                                       GTK_BOX (private->right_vbox));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->linked);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->linked);

  button = check_button_with_icon_new (lock_content_label,
                                       lock_content_icon_name,
                                       GTK_BOX (private->right_vbox));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->lock_content);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->lock_content);

  button = check_button_with_icon_new (lock_position_label,
                                       GIMP_ICON_TOOL_MOVE,
                                       GTK_BOX (private->right_vbox));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->lock_position);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->lock_position);

  private->lock_position_toggle = button;

  return dialog;
}

GtkWidget *
item_options_dialog_get_vbox (GtkWidget *dialog)
{
  ItemOptionsDialog *private;

  g_return_val_if_fail (GIMP_IS_VIEWABLE_DIALOG (dialog), NULL);

  private = g_object_get_data (G_OBJECT (dialog),
                               "item-options-dialog-private");

  g_return_val_if_fail (private != NULL, NULL);

  return private->left_vbox;
}

GtkWidget *
item_options_dialog_get_grid (GtkWidget *dialog,
                              gint      *next_row)
{
  ItemOptionsDialog *private;

  g_return_val_if_fail (GIMP_IS_VIEWABLE_DIALOG (dialog), NULL);
  g_return_val_if_fail (next_row != NULL, NULL);

  private = g_object_get_data (G_OBJECT (dialog),
                               "item-options-dialog-private");

  g_return_val_if_fail (private != NULL, NULL);

  *next_row = private->grid_row;

  return private->left_grid;
}

GtkWidget *
item_options_dialog_get_name_entry (GtkWidget *dialog)
{
  ItemOptionsDialog *private;

  g_return_val_if_fail (GIMP_IS_VIEWABLE_DIALOG (dialog), NULL);

  private = g_object_get_data (G_OBJECT (dialog),
                               "item-options-dialog-private");

  g_return_val_if_fail (private != NULL, NULL);

  return private->name_entry;
}

GtkWidget *
item_options_dialog_get_lock_position (GtkWidget *dialog)
{
  ItemOptionsDialog *private;

  g_return_val_if_fail (GIMP_IS_VIEWABLE_DIALOG (dialog), NULL);

  private = g_object_get_data (G_OBJECT (dialog),
                               "item-options-dialog-private");

  g_return_val_if_fail (private != NULL, NULL);

  return private->lock_position_toggle;
}

void
item_options_dialog_add_widget (GtkWidget   *dialog,
                                const gchar *label,
                                GtkWidget   *widget)
{
  ItemOptionsDialog *private;

  g_return_if_fail (GIMP_IS_VIEWABLE_DIALOG (dialog));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  private = g_object_get_data (G_OBJECT (dialog),
                               "item-options-dialog-private");

  g_return_if_fail (private != NULL);

  gimp_grid_attach_aligned (GTK_GRID (private->left_grid),
                            0, private->grid_row++,
                            label, 0.0, 0.5,
                            widget, 1);
}

GtkWidget *
item_options_dialog_add_switch (GtkWidget   *dialog,
                                const gchar *icon_name,
                                const gchar *label)
{
  ItemOptionsDialog *private;

  g_return_val_if_fail (GIMP_IS_VIEWABLE_DIALOG (dialog), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (label != NULL, NULL);

  private = g_object_get_data (G_OBJECT (dialog),
                               "item-options-dialog-private");

  g_return_val_if_fail (private != NULL, NULL);

  return check_button_with_icon_new (label, icon_name,
                                     GTK_BOX (private->right_vbox));
}

void
item_options_dialog_set_switches_visible (GtkWidget *dialog,
                                          gboolean   visible)
{
  ItemOptionsDialog *private;

  g_return_if_fail (GIMP_IS_VIEWABLE_DIALOG (dialog));

  private = g_object_get_data (G_OBJECT (dialog),
                               "item-options-dialog-private");

  g_return_if_fail (private != NULL);

  gtk_widget_set_visible (private->right_frame, visible);
}


/*  private functions  */

static void
item_options_dialog_free (ItemOptionsDialog *private)
{
  g_slice_free (ItemOptionsDialog, private);
}

static void
item_options_dialog_response (GtkWidget         *dialog,
                              gint               response_id,
                              ItemOptionsDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *name = NULL;

      if (private->name_entry)
        name = gtk_entry_get_text (GTK_ENTRY (private->name_entry));

      private->callback (dialog,
                         private->image,
                         private->item,
                         private->context,
                         name,
                         private->visible,
                         private->linked,
                         private->color_tag,
                         private->lock_content,
                         private->lock_position,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static GtkWidget *
check_button_with_icon_new (const gchar *label,
                            const gchar *icon_name,
                            GtkBox      *vbox)
{
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *image;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  button = gtk_check_button_new_with_mnemonic (label);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  return button;
}
