/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimppalette.h"
#include "core/gimppalette-import.h"

#include "file/file-utils.h"

#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpview.h"
#include "widgets/gimpwidgets-utils.h"

#include "palette-import-dialog.h"

#include "gimp-intl.h"


typedef enum
{
  GRADIENT_IMPORT,
  IMAGE_IMPORT,
  FILE_IMPORT
} ImportType;


typedef struct
{
  GtkWidget     *dialog;

  ImportType     import_type;
  GimpContext   *context;
  GimpImage     *image;

  GimpPalette   *palette;

  GtkWidget     *gradient_radio;
  GtkWidget     *image_radio;
  GtkWidget     *file_radio;

  GtkWidget     *gradient_combo;
  GtkWidget     *image_combo;
  GtkWidget     *file_chooser;

  GtkWidget     *sample_merged_toggle;
  GtkWidget     *selection_only_toggle;

  GtkWidget     *entry;
  GtkAdjustment *num_colors;
  GtkAdjustment *columns;
  GtkAdjustment *threshold;

  GtkWidget     *preview;
  GtkWidget     *no_colors_label;
} ImportDialog;


static void   palette_import_free                 (ImportDialog   *dialog);
static void   palette_import_response             (GtkWidget      *widget,
                                                   gint            response_id,
                                                   ImportDialog   *dialog);
static void   palette_import_gradient_changed     (GimpContext    *context,
                                                   GimpGradient   *gradient,
                                                   ImportDialog   *dialog);
static void   palette_import_image_changed        (GimpContext    *context,
                                                   GimpImage      *image,
                                                   ImportDialog   *dialog);
static void   palette_import_layer_changed        (GimpImage      *image,
                                                   ImportDialog   *dialog);
static void   palette_import_mask_changed         (GimpImage      *image,
                                                   ImportDialog   *dialog);
static void   palette_import_filename_changed     (GtkFileChooser *button,
                                                   ImportDialog   *dialog);
static void   import_dialog_drop_callback         (GtkWidget      *widget,
                                                   gint            x,
                                                   gint            y,
                                                   GimpViewable   *viewable,
                                                   gpointer        data);
static void   palette_import_grad_callback        (GtkWidget      *widget,
                                                   ImportDialog   *dialog);
static void   palette_import_image_callback       (GtkWidget      *widget,
                                                   ImportDialog   *dialog);
static void   palette_import_file_callback        (GtkWidget      *widget,
                                                   ImportDialog   *dialog);
static void   palette_import_columns_changed      (GtkAdjustment  *adjustment,
                                                   ImportDialog   *dialog);
static void   palette_import_image_add            (GimpContainer  *container,
                                                   GimpImage      *image,
                                                   ImportDialog   *dialog);
static void   palette_import_image_remove         (GimpContainer  *container,
                                                   GimpImage      *image,
                                                   ImportDialog   *dialog);
static void   palette_import_make_palette         (ImportDialog   *dialog);


/*  public functions  */

GtkWidget *
palette_import_dialog_new (GimpContext *context)
{
  ImportDialog *dialog;
  GimpGradient *gradient;
  GtkWidget    *button;
  GtkWidget    *main_hbox;
  GtkWidget    *frame;
  GtkWidget    *vbox;
  GtkWidget    *table;
  GtkWidget    *abox;
  GtkSizeGroup *size_group;
  GSList       *group = NULL;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  gradient = gimp_context_get_gradient (context);

  dialog = g_slice_new0 (ImportDialog);

  dialog->import_type = GRADIENT_IMPORT;
  dialog->context     = gimp_context_new (context->gimp, "Palette Import",
                                          context);

  dialog->dialog = gimp_dialog_new (_("Import a New Palette"),
                                    "gimp-palette-import", NULL, 0,
                                    gimp_standard_help_func,
                                    GIMP_HELP_PALETTE_IMPORT,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                                    NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog->dialog),
                                  _("_Import"), GTK_RESPONSE_OK);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GTK_STOCK_CONVERT,
                                                  GTK_ICON_SIZE_BUTTON));

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_set_data_full (G_OBJECT (dialog->dialog), "palette-import-dialog",
                          dialog, (GDestroyNotify) palette_import_free);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (palette_import_response),
                    dialog);

  gimp_dnd_viewable_dest_add (dialog->dialog,
                              GIMP_TYPE_GRADIENT,
                              import_dialog_drop_callback,
                              dialog);
  gimp_dnd_viewable_dest_add (dialog->dialog,
                              GIMP_TYPE_IMAGE,
                              import_dialog_drop_callback,
                              dialog);

  main_hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog->dialog)->vbox),
                     main_hbox);
  gtk_widget_show (main_hbox);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);


  /*  The "Source" frame  */

  frame = gimp_frame_new (_("Select Source"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  dialog->gradient_radio =
    gtk_radio_button_new_with_mnemonic (group, _("_Gradient"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog->gradient_radio));
  gtk_table_attach (GTK_TABLE (table), dialog->gradient_radio,
                    0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (dialog->gradient_radio);

  g_signal_connect (dialog->gradient_radio, "toggled",
                    G_CALLBACK (palette_import_grad_callback),
                    dialog);

  dialog->image_radio =
    gtk_radio_button_new_with_mnemonic (group, _("I_mage"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog->image_radio));
  gtk_table_attach (GTK_TABLE (table), dialog->image_radio,
                    0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (dialog->image_radio);

  g_signal_connect (dialog->image_radio, "toggled",
                    G_CALLBACK (palette_import_image_callback),
                    dialog);

  gtk_widget_set_sensitive (dialog->image_radio,
                            ! gimp_container_is_empty (context->gimp->images));

  dialog->sample_merged_toggle =
    gtk_check_button_new_with_mnemonic (_("Sample _Merged"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->sample_merged_toggle),
                                TRUE);
  gtk_table_attach (GTK_TABLE (table), dialog->sample_merged_toggle,
                    1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (dialog->sample_merged_toggle);

  g_signal_connect_swapped (dialog->sample_merged_toggle, "toggled",
                            G_CALLBACK (palette_import_make_palette),
                            dialog);

  dialog->selection_only_toggle =
    gtk_check_button_new_with_mnemonic (_("_Selected Pixels only"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->selection_only_toggle),
                                FALSE);
  gtk_table_attach (GTK_TABLE (table), dialog->selection_only_toggle,
                    1, 2, 3, 4, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (dialog->selection_only_toggle);

  g_signal_connect_swapped (dialog->selection_only_toggle, "toggled",
                            G_CALLBACK (palette_import_make_palette),
                            dialog);

  dialog->file_radio =
    gtk_radio_button_new_with_mnemonic (group, _("Palette _file"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (dialog->image_radio));
  gtk_table_attach (GTK_TABLE (table), dialog->file_radio,
                    0, 1, 4, 5, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (dialog->file_radio);

  g_signal_connect (dialog->file_radio, "toggled",
                    G_CALLBACK (palette_import_file_callback),
                    dialog);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  /*  The gradient menu  */
  dialog->gradient_combo =
    gimp_container_combo_box_new (context->gimp->gradient_factory->container,
                                  dialog->context, 24, 1);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             NULL, 0.0, 0.5, dialog->gradient_combo, 1, FALSE);
  gtk_size_group_add_widget (size_group, dialog->gradient_combo);

  /*  The image menu  */
  dialog->image_combo =
    gimp_container_combo_box_new (context->gimp->images, dialog->context,
                                  24, 1);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             NULL, 0.0, 0.5, dialog->image_combo, 1, FALSE);
  gtk_size_group_add_widget (size_group, dialog->image_combo);

  /*  Palette file name entry  */
  dialog->file_chooser = gtk_file_chooser_button_new (_("Select Palette File"),
                                                      GTK_FILE_CHOOSER_ACTION_OPEN);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
                             NULL, 0.0, 0.5, dialog->file_chooser, 1, FALSE);
  gtk_size_group_add_widget (size_group, dialog->file_chooser);

  g_object_unref (size_group);


  /*  The "Import" frame  */

  frame = gimp_frame_new (_("Import Options"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  /*  The source's name  */
  dialog->entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (dialog->entry),
                      gradient ?
                      GIMP_OBJECT (gradient)->name : _("New import"));
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Palette _name:"), 0.0, 0.5,
                             dialog->entry, 2, FALSE);

  /*  The # of colors  */
  dialog->num_colors =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                          _("N_umber of colors:"), -1, 5,
                                          256, 2, 10000, 1, 10, 0,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect_swapped (dialog->num_colors,
                            "value-changed",
                            G_CALLBACK (palette_import_make_palette),
                            dialog);

  /*  The columns  */
  dialog->columns =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
                                          _("C_olumns:"), -1, 5,
                                          16, 0, 64, 1, 8, 0,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (dialog->columns, "value-changed",
                    G_CALLBACK (palette_import_columns_changed),
                    dialog);

  /*  The interval  */
  dialog->threshold =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
                                          _("I_nterval:"), -1, 5,
                                          1, 1, 128, 1, 8, 0,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect_swapped (dialog->threshold, "value-changed",
                            G_CALLBACK (palette_import_make_palette),
                            dialog);


  /*  The "Preview" frame  */
  frame = gimp_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  abox = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  dialog->preview = gimp_view_new_full_by_types (dialog->context,
                                                 GIMP_TYPE_VIEW,
                                                 GIMP_TYPE_PALETTE,
                                                 192, 192, 1,
                                                 TRUE, FALSE, FALSE);
  gtk_container_add (GTK_CONTAINER (abox), dialog->preview);
  gtk_widget_show (dialog->preview);

  dialog->no_colors_label =
    gtk_label_new (_("The selected source contains no colors."));
  gtk_widget_set_size_request (dialog->no_colors_label, 194, -1);
  gtk_label_set_line_wrap (GTK_LABEL (dialog->no_colors_label), TRUE);
  gimp_label_set_attributes (GTK_LABEL (dialog->no_colors_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->no_colors_label, FALSE, FALSE, 0);
  gtk_widget_show (dialog->no_colors_label);


  /*  keep the dialog up-to-date  */

  g_signal_connect (context->gimp->images, "add",
                    G_CALLBACK (palette_import_image_add),
                    dialog);
  g_signal_connect (context->gimp->images, "remove",
                    G_CALLBACK (palette_import_image_remove),
                    dialog);

  g_signal_connect (dialog->context, "gradient-changed",
                    G_CALLBACK (palette_import_gradient_changed),
                    dialog);
  g_signal_connect (dialog->context, "image-changed",
                    G_CALLBACK (palette_import_image_changed),
                    dialog);
  g_signal_connect (dialog->file_chooser, "selection-changed",
                    G_CALLBACK (palette_import_filename_changed),
                    dialog);

  palette_import_grad_callback (dialog->gradient_radio, dialog);

  return dialog->dialog;
}


/*  private functions  */

static void
palette_import_free (ImportDialog *dialog)
{
  Gimp *gimp = dialog->context->gimp;

  g_signal_handlers_disconnect_by_func (gimp->images,
                                        palette_import_image_add,
                                        dialog);
  g_signal_handlers_disconnect_by_func (gimp->images,
                                        palette_import_image_remove,
                                        dialog);

  if (dialog->palette)
    g_object_unref (dialog->palette);

  g_object_unref (dialog->context);

  g_slice_free (ImportDialog, dialog);
}


/*  the palette import response callback  ************************************/

static void
palette_import_response (GtkWidget    *widget,
                         gint          response_id,
                         ImportDialog *dialog)
{
  Gimp *gimp = dialog->context->gimp;

  palette_import_image_changed (dialog->context, NULL, dialog);

  if (dialog->palette)
    {
      if (response_id == GTK_RESPONSE_OK)
        {
          const gchar *name = gtk_entry_get_text (GTK_ENTRY (dialog->entry));

          if (name && *name)
            gimp_object_set_name (GIMP_OBJECT (dialog->palette), name);

          gimp_container_add (gimp->palette_factory->container,
                              GIMP_OBJECT (dialog->palette));
        }
    }

  gtk_widget_destroy (dialog->dialog);
}


/*  functions to create & update the import dialog's gradient selection  *****/

static void
palette_import_gradient_changed (GimpContext  *context,
                                 GimpGradient *gradient,
                                 ImportDialog *dialog)
{
  if (gradient && dialog->import_type == GRADIENT_IMPORT)
    {
      gtk_entry_set_text (GTK_ENTRY (dialog->entry),
                          GIMP_OBJECT (gradient)->name);

      palette_import_make_palette (dialog);
    }
}

static void
palette_import_image_changed (GimpContext  *context,
                              GimpImage    *image,
                              ImportDialog *dialog)
{
  if (dialog->image)
    {
      g_signal_handlers_disconnect_by_func (dialog->image,
                                            palette_import_layer_changed,
                                            dialog);
      g_signal_handlers_disconnect_by_func (dialog->image,
                                            palette_import_mask_changed,
                                            dialog);
    }

  dialog->image = image;

  if (dialog->import_type == IMAGE_IMPORT)
    {
      gboolean sensitive = FALSE;

      if (image)
        {
          gchar *name;
          gchar *label;

          name = file_utils_uri_display_basename (gimp_image_get_uri (image));
          label = g_strdup_printf ("%s-%d", name, gimp_image_get_ID (image));
          g_free (name);

          gtk_entry_set_text (GTK_ENTRY (dialog->entry), label);
          g_free (label);

          palette_import_make_palette (dialog);

          if (gimp_image_base_type (image) != GIMP_INDEXED)
            sensitive = TRUE;
        }

      gtk_widget_set_sensitive (dialog->sample_merged_toggle, sensitive);
      gtk_widget_set_sensitive (dialog->selection_only_toggle, sensitive);
      gimp_scale_entry_set_sensitive (GTK_OBJECT (dialog->threshold),
                                      sensitive);
      gimp_scale_entry_set_sensitive (GTK_OBJECT (dialog->num_colors),
                                      sensitive);
    }

  if (dialog->image)
    {
      g_signal_connect (dialog->image, "active-layer-changed",
                        G_CALLBACK (palette_import_layer_changed),
                        dialog);
      g_signal_connect (dialog->image, "mask-changed",
                        G_CALLBACK (palette_import_mask_changed),
                        dialog);
    }
}

static void
palette_import_layer_changed (GimpImage    *image,
                              ImportDialog *dialog)
{
  if (dialog->import_type == IMAGE_IMPORT &&
      ! gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON (dialog->sample_merged_toggle)))
    {
      palette_import_make_palette (dialog);
    }
}

static void
palette_import_mask_changed (GimpImage    *image,
                             ImportDialog *dialog)
{
  if (dialog->import_type == IMAGE_IMPORT &&
      gtk_toggle_button_get_active
      (GTK_TOGGLE_BUTTON (dialog->selection_only_toggle)))
    {
      palette_import_make_palette (dialog);
    }
}

static void
palette_import_filename_changed (GtkFileChooser *button,
                                 ImportDialog   *dialog)
{
  gchar *filename;

  if (dialog->import_type != FILE_IMPORT)
    return;

  filename = gtk_file_chooser_get_filename (button);

  if (filename)
    {
      gchar *basename = g_filename_display_basename (filename);

      /* TODO: strip filename extension */
      gtk_entry_set_text (GTK_ENTRY (dialog->entry), basename);
      g_free (basename);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (dialog->entry), "");
    }

  g_free (filename);

  palette_import_make_palette (dialog);
}

static void
import_dialog_drop_callback (GtkWidget    *widget,
                             gint          x,
                             gint          y,
                             GimpViewable *viewable,
                             gpointer      data)
{
  ImportDialog *dialog = data;

  gimp_context_set_by_type (dialog->context,
                            G_TYPE_FROM_INSTANCE (viewable),
                            GIMP_OBJECT (viewable));

  if (GIMP_IS_GRADIENT (viewable) &&
      dialog->import_type != GRADIENT_IMPORT)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->gradient_radio),
                                    TRUE);
    }
  else if (GIMP_IS_IMAGE (viewable) &&
           dialog->import_type != IMAGE_IMPORT)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->image_radio),
                                    TRUE);
    }
}


/*  the import source menu item callbacks  ***********************************/

static void
palette_import_set_sensitive (ImportDialog *dialog)
{
  gboolean gradient = (dialog->import_type == GRADIENT_IMPORT);
  gboolean image    = (dialog->import_type == IMAGE_IMPORT);
  gboolean file     = (dialog->import_type == FILE_IMPORT);

  gtk_widget_set_sensitive (dialog->gradient_combo,        gradient);
  gtk_widget_set_sensitive (dialog->image_combo,           image);
  gtk_widget_set_sensitive (dialog->sample_merged_toggle,  image);
  gtk_widget_set_sensitive (dialog->selection_only_toggle, image);
  gtk_widget_set_sensitive (dialog->file_chooser,          file);

  gimp_scale_entry_set_sensitive (GTK_OBJECT (dialog->num_colors), ! file);
  gimp_scale_entry_set_sensitive (GTK_OBJECT (dialog->columns),    ! file);
  gimp_scale_entry_set_sensitive (GTK_OBJECT (dialog->threshold),  image);
}

static void
palette_import_grad_callback (GtkWidget    *widget,
                              ImportDialog *dialog)
{
  GimpGradient *gradient;

  if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    return;

  dialog->import_type = GRADIENT_IMPORT;

  gradient = gimp_context_get_gradient (dialog->context);

  gtk_entry_set_text (GTK_ENTRY (dialog->entry),
                      GIMP_OBJECT (gradient)->name);

  palette_import_set_sensitive (dialog);

  palette_import_make_palette (dialog);
}

static void
palette_import_image_callback (GtkWidget    *widget,
                               ImportDialog *dialog)
{
  GimpImage *image;

  if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    return;

  dialog->import_type = IMAGE_IMPORT;

  image = gimp_context_get_image (dialog->context);

  if (! image)
    image = (GimpImage *)
      gimp_container_get_child_by_index (dialog->context->gimp->images, 0);

  palette_import_set_sensitive (dialog);

  palette_import_image_changed (dialog->context, image, dialog);
}

static void
palette_import_file_callback (GtkWidget    *widget,
                              ImportDialog *dialog)
{
  gchar *filename;

  if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    return;

  dialog->import_type = FILE_IMPORT;

  filename =
    gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog->file_chooser));

  if (filename)
    {
      gchar *basename = g_filename_display_basename (filename);

      /* TODO: strip filename extension */
      gtk_entry_set_text (GTK_ENTRY (dialog->entry), basename);
      g_free (basename);

      g_free (filename);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (dialog->entry), "");
    }

  palette_import_set_sensitive (dialog);
}

static void
palette_import_columns_changed (GtkAdjustment *adj,
                                ImportDialog  *dialog)
{
  if (dialog->palette)
    gimp_palette_set_columns (dialog->palette, ROUND (adj->value));
}

/*  functions & callbacks to keep the import dialog uptodate  ****************/

static void
palette_import_image_add (GimpContainer *container,
                          GimpImage     *image,
                          ImportDialog  *dialog)
{
  if (! GTK_WIDGET_IS_SENSITIVE (dialog->image_radio))
    {
      gtk_widget_set_sensitive (dialog->image_radio, TRUE);
      gimp_context_set_image (dialog->context, image);
    }
}

static void
palette_import_image_remove (GimpContainer *container,
                             GimpImage     *image,
                             ImportDialog  *dialog)
{
  if (! gimp_container_num_children (dialog->context->gimp->images))
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->image_radio)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->gradient_radio),
                                      TRUE);

      gtk_widget_set_sensitive (dialog->image_radio, FALSE);
    }
}

static void
palette_import_make_palette (ImportDialog *dialog)
{
  GimpPalette  *palette = NULL;
  const gchar  *palette_name;
  gint          n_colors;
  gint          n_columns;
  gint          threshold;

  palette_name = gtk_entry_get_text (GTK_ENTRY (dialog->entry));

  if (! palette_name || ! strlen (palette_name))
    palette_name = _("Untitled");

  n_colors  = ROUND (dialog->num_colors->value);
  n_columns = ROUND (dialog->columns->value);
  threshold = ROUND (dialog->threshold->value);

  switch (dialog->import_type)
    {
    case GRADIENT_IMPORT:
      {
        GimpGradient *gradient;

        gradient = gimp_context_get_gradient (dialog->context);

        palette = gimp_palette_import_from_gradient (gradient,
                                                     dialog->context,
                                                     FALSE,
                                                     palette_name,
                                                     n_colors);
      }
      break;

    case IMAGE_IMPORT:
      {
        GimpImage *image = gimp_context_get_image (dialog->context);
        gboolean   sample_merged;
        gboolean   selection_only;

        sample_merged =
          gtk_toggle_button_get_active
          (GTK_TOGGLE_BUTTON (dialog->sample_merged_toggle));

        selection_only =
          gtk_toggle_button_get_active
          (GTK_TOGGLE_BUTTON (dialog->selection_only_toggle));

        if (gimp_image_base_type (image) == GIMP_INDEXED)
          {
            palette = gimp_palette_import_from_indexed_image (image,
                                                              palette_name);
          }
        else if (sample_merged)
          {
            palette = gimp_palette_import_from_image (image,
                                                      palette_name,
                                                      n_colors,
                                                      threshold,
                                                      selection_only);
          }
        else
          {
            GimpDrawable *drawable;

            drawable = GIMP_DRAWABLE (gimp_image_get_active_layer (image));

            palette = gimp_palette_import_from_drawable (drawable,
                                                         palette_name,
                                                         n_colors,
                                                         threshold,
                                                         selection_only);
          }
      }
      break;

    case FILE_IMPORT:
      {
        gchar  *filename;
        GError *error = NULL;

        filename =
          gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog->file_chooser));

        palette = gimp_palette_import_from_file (filename,
                                                 palette_name, &error);
        g_free (filename);

        if (! palette)
          {
            gimp_message (dialog->context->gimp, G_OBJECT (dialog->dialog),
                          GIMP_MESSAGE_ERROR,
                          "%s", error->message);
            g_error_free (error);
          }
      }
      break;
    }

  if (palette)
    {
      if (dialog->palette)
        g_object_unref (dialog->palette);

      palette->n_columns = n_columns;

      gimp_view_set_viewable (GIMP_VIEW (dialog->preview),
                              GIMP_VIEWABLE (palette));

      dialog->palette = palette;
    }

  if (dialog->palette && dialog->palette->n_colors > 0)
    gtk_widget_hide (dialog->no_colors_label);
  else
    gtk_widget_show (dialog->no_colors_label);
}
