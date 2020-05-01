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
#include "core/gimpcontainer-filter.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimppalette.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "convert-indexed-dialog.h"

#include "gimp-intl.h"


typedef struct _IndexedDialog IndexedDialog;

struct _IndexedDialog
{
  GimpImage                  *image;
  GimpConvertPaletteType      palette_type;
  gint                        max_colors;
  gboolean                    remove_duplicates;
  GimpConvertDitherType       dither_type;
  gboolean                    dither_alpha;
  gboolean                    dither_text_layers;
  GimpPalette                *custom_palette;
  GimpConvertIndexedCallback  callback;
  gpointer                    user_data;

  GtkWidget                  *dialog;
  GimpContext                *context;
  GimpContainer              *container;
  GtkWidget                  *duplicates_toggle;
};


static void        convert_dialog_free            (IndexedDialog *private);
static void        convert_dialog_response        (GtkWidget     *widget,
                                                   gint           response_id,
                                                   IndexedDialog *private);
static GtkWidget * convert_dialog_palette_box     (IndexedDialog *private);
static gboolean    convert_dialog_palette_filter  (GimpObject    *object,
                                                   gpointer       user_data);
static void        convert_dialog_palette_changed (GimpContext   *context,
                                                   GimpPalette   *palette,
                                                   IndexedDialog *private);
static void        convert_dialog_type_update     (GtkWidget     *widget,
                                                   IndexedDialog *private);



/*  public functions  */

GtkWidget *
convert_indexed_dialog_new (GimpImage                  *image,
                            GimpContext                *context,
                            GtkWidget                  *parent,
                            GimpConvertPaletteType      palette_type,
                            gint                        max_colors,
                            gboolean                    remove_duplicates,
                            GimpConvertDitherType       dither_type,
                            gboolean                    dither_alpha,
                            gboolean                    dither_text_layers,
                            GimpPalette                *custom_palette,
                            GimpConvertIndexedCallback  callback,
                            gpointer                    user_data)
{
  IndexedDialog *private;
  GtkWidget     *dialog;
  GtkWidget     *button;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkAdjustment *adjustment;
  GtkWidget     *spinbutton;
  GtkWidget     *frame;
  GtkWidget     *toggle;
  GtkWidget     *palette_box;
  GtkWidget     *combo;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (custom_palette == NULL ||
                        GIMP_IS_PALETTE (custom_palette), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (IndexedDialog);

  private->image              = image;
  private->palette_type       = palette_type;
  private->max_colors         = max_colors;
  private->remove_duplicates  = remove_duplicates;
  private->dither_type        = dither_type;
  private->dither_alpha       = dither_alpha;
  private->dither_text_layers = dither_text_layers;
  private->custom_palette     = custom_palette;
  private->callback           = callback;
  private->user_data          = user_data;

  private->dialog = dialog =
    gimp_viewable_dialog_new (g_list_prepend (NULL, image), context,
                              _("Indexed Color Conversion"),
                              "gimp-image-convert-indexed",
                              GIMP_ICON_CONVERT_INDEXED,
                              _("Convert Image to Indexed Colors"),
                              parent,
                              gimp_standard_help_func,
                              GIMP_HELP_IMAGE_CONVERT_INDEXED,

                              _("_Cancel"),  GTK_RESPONSE_CANCEL,
                              _("C_onvert"), GTK_RESPONSE_OK,

                              NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) convert_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (convert_dialog_response),
                    private);

  palette_box = convert_dialog_palette_box (private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);


  /*  palette  */

  frame =
    gimp_enum_radio_frame_new_with_range (GIMP_TYPE_CONVERT_PALETTE_TYPE,
                                          GIMP_CONVERT_PALETTE_GENERATE,
                                          (palette_box ?
                                           GIMP_CONVERT_PALETTE_CUSTOM :
                                           GIMP_CONVERT_PALETTE_MONO),
                                          gtk_label_new (_("Colormap")),
                                          G_CALLBACK (convert_dialog_type_update),
                                          private, NULL,
                                          &button);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button),
                                   private->palette_type);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  max n_colors  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gimp_enum_radio_frame_add (GTK_FRAME (frame), hbox,
                             GIMP_CONVERT_PALETTE_GENERATE, TRUE);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Maximum number of colors:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (private->max_colors == 256 && gimp_image_has_alpha (image))
    private->max_colors = 255;

  adjustment = gtk_adjustment_new (private->max_colors, 2, 256, 1, 8, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &private->max_colors);

  /*  custom palette  */
  if (palette_box)
    {
      gimp_enum_radio_frame_add (GTK_FRAME (frame), palette_box,
                                 GIMP_CONVERT_PALETTE_CUSTOM, TRUE);
      gtk_widget_show (palette_box);
    }

  vbox = gtk_bin_get_child (GTK_BIN (frame));

  private->duplicates_toggle = toggle =
    gtk_check_button_new_with_mnemonic (_("_Remove unused and duplicate "
                                          "colors from colormap"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                private->remove_duplicates);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 3);
  gtk_widget_show (toggle);

  if (private->palette_type == GIMP_CONVERT_PALETTE_GENERATE ||
      private->palette_type == GIMP_CONVERT_PALETTE_MONO)
    gtk_widget_set_sensitive (toggle, FALSE);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->remove_duplicates);

  /*  dithering  */

  frame = gimp_frame_new (_("Dithering"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Color _dithering:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_enum_combo_box_new (GIMP_TYPE_CONVERT_DITHER_TYPE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              private->dither_type,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &private->dither_type, NULL);

  toggle =
    gtk_check_button_new_with_mnemonic (_("Enable dithering of _transparency"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                private->dither_alpha);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->dither_alpha);


  toggle =
    gtk_check_button_new_with_mnemonic (_("Enable dithering of text _layers"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                private->dither_text_layers);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->dither_text_layers);

  gimp_help_set_help_data (toggle,
                           _("Dithering text layers will make them uneditable"),
                           NULL);

  return dialog;
}


/*  private functions  */

static void
convert_dialog_free (IndexedDialog *private)
{
  if (private->container)
    g_object_unref (private->container);

  if (private->context)
    g_object_unref (private->context);

  g_slice_free (IndexedDialog, private);
}

static void
convert_dialog_response (GtkWidget     *dialog,
                         gint           response_id,
                         IndexedDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      private->callback (dialog,
                         private->image,
                         private->palette_type,
                         private->max_colors,
                         private->remove_duplicates,
                         private->dither_type,
                         private->dither_alpha,
                         private->dither_text_layers,
                         private->custom_palette,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static GtkWidget *
convert_dialog_palette_box (IndexedDialog *private)
{
  Gimp        *gimp = private->image->gimp;
  GList       *list;
  GimpPalette *web_palette  = NULL;
  gboolean     custom_found = FALSE;

  /* We can't dither to > 256 colors */
  private->container =
    gimp_container_filter (gimp_data_factory_get_container (gimp->palette_factory),
                           convert_dialog_palette_filter,
                           NULL);

  if (gimp_container_is_empty (private->container))
    {
      g_object_unref (private->container);
      private->container = NULL;
      return NULL;
    }

  private->context = gimp_context_new (gimp, "convert-dialog", NULL);

  for (list = GIMP_LIST (private->container)->queue->head;
       list;
       list = g_list_next (list))
    {
      GimpPalette *palette = list->data;

      /* Preferentially, the initial default is 'Web' if available */
      if (web_palette == NULL &&
          g_ascii_strcasecmp (gimp_object_get_name (palette), "Web") == 0)
        {
          web_palette = palette;
        }

      if (private->custom_palette == palette)
        custom_found = TRUE;
    }

  if (! custom_found)
    {
      if (web_palette)
        private->custom_palette = web_palette;
      else
        private->custom_palette = GIMP_LIST (private->container)->queue->head->data;
    }

  gimp_context_set_palette (private->context, private->custom_palette);

  g_signal_connect (private->context, "palette-changed",
                    G_CALLBACK (convert_dialog_palette_changed),
                    private);

  return gimp_palette_box_new (private->container, private->context, NULL, 4);
}

static gboolean
convert_dialog_palette_filter (GimpObject *object,
                               gpointer    user_data)
{
  GimpPalette *palette = GIMP_PALETTE (object);

  return (gimp_palette_get_n_colors (palette) > 0 &&
          gimp_palette_get_n_colors (palette) <= 256);
}

static void
convert_dialog_palette_changed (GimpContext   *context,
                                GimpPalette   *palette,
                                IndexedDialog *private)
{
  if (! palette)
    return;

  if (gimp_palette_get_n_colors (palette) > 256)
    {
      gimp_message (private->image->gimp, G_OBJECT (private->dialog),
                    GIMP_MESSAGE_WARNING,
                    _("Cannot convert to a palette "
                      "with more than 256 colors."));
    }
  else
    {
      private->custom_palette = palette;
    }
}

static void
convert_dialog_type_update (GtkWidget     *widget,
                            IndexedDialog *private)
{
  gimp_radio_button_update (widget, &private->palette_type);

  if (private->duplicates_toggle)
    gtk_widget_set_sensitive (private->duplicates_toggle,
                              private->palette_type !=
                              GIMP_CONVERT_PALETTE_GENERATE &&
                              private->palette_type !=
                              GIMP_CONVERT_PALETTE_MONO);
}
