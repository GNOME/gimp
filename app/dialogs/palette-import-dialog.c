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

#include <string.h>

#include <gegl.h>
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


typedef struct _ImportDialog ImportDialog;

struct _ImportDialog
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
  GtkWidget     *num_colors;
  GtkWidget     *columns;
  GtkWidget     *threshold;

  GtkWidget     *preview;
  GtkWidget     *no_colors_label;
};


static void   palette_import_free                 (ImportDialog   *private);
static void   palette_import_response             (GtkWidget      *dialog,
                                                   gint            response_id,
                                                   ImportDialog   *private);
static void   palette_import_gradient_changed     (GimpContext    *context,
                                                   GimpGradient   *gradient,
                                                   ImportDialog   *private);
static void   palette_import_image_changed        (GimpContext    *context,
                                                   GimpImage      *image,
                                                   ImportDialog   *private);
static void   palette_import_layer_changed        (GimpImage      *image,
                                                   ImportDialog   *private);
static void   palette_import_mask_changed         (GimpImage      *image,
                                                   ImportDialog   *private);
static void   palette_import_filename_changed     (GtkFileChooser *button,
                                                   ImportDialog   *private);
static void   import_dialog_drop_callback         (GtkWidget      *widget,
                                                   gint            x,
                                                   gint            y,
                                                   GimpViewable   *viewable,
                                                   gpointer        data);
static void   palette_import_grad_callback        (GtkWidget      *widget,
                                                   ImportDialog   *private);
static void   palette_import_image_callback       (GtkWidget      *widget,
                                                   ImportDialog   *private);
static void   palette_import_file_callback        (GtkWidget      *widget,
                                                   ImportDialog   *private);
static void   palette_import_columns_changed      (GimpLabelSpin  *columns,
                                                   ImportDialog   *private);
static void   palette_import_image_add            (GimpContainer  *container,
                                                   GimpImage      *image,
                                                   ImportDialog   *private);
static void   palette_import_image_remove         (GimpContainer  *container,
                                                   GimpImage      *image,
                                                   ImportDialog   *private);
static void   palette_import_make_palette         (ImportDialog   *private);
static void   palette_import_file_set_filters     (GtkFileChooser *file_chooser);

/*  public functions  */

GtkWidget *
palette_import_dialog_new (GimpContext *context)
{
  ImportDialog *private;
  GimpGradient *gradient;
  GtkWidget    *dialog;
  GtkWidget    *main_hbox;
  GtkWidget    *frame;
  GtkWidget    *vbox;
  GtkWidget    *grid;
  GtkSizeGroup *size_group;
  GSList       *group = NULL;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  gradient = gimp_context_get_gradient (context);

  private = g_slice_new0 (ImportDialog);

  private->import_type = GRADIENT_IMPORT;
  private->context     = gimp_context_new (context->gimp, "Palette Import",
                                          context);

  dialog = private->dialog =
    gimp_dialog_new (_("Import a New Palette"),
                     "gimp-palette-import", NULL, 0,
                     gimp_standard_help_func,
                     GIMP_HELP_PALETTE_IMPORT,

                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                     _("_Import"), GTK_RESPONSE_OK,

                     NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) palette_import_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (palette_import_response),
                    private);

  gimp_dnd_viewable_dest_add (dialog,
                              GIMP_TYPE_GRADIENT,
                              import_dialog_drop_callback,
                              private);
  gimp_dnd_viewable_dest_add (dialog,
                              GIMP_TYPE_IMAGE,
                              import_dialog_drop_callback,
                              private);

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_hbox, TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);


  /*  The "Source" frame  */

  frame = gimp_frame_new (_("Select Source"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  private->gradient_radio =
    gtk_radio_button_new_with_mnemonic (group, _("_Gradient"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (private->gradient_radio));
  gtk_grid_attach (GTK_GRID (grid), private->gradient_radio, 0, 0, 1, 1);
  gtk_widget_show (private->gradient_radio);

  g_signal_connect (private->gradient_radio, "toggled",
                    G_CALLBACK (palette_import_grad_callback),
                    private);

  private->image_radio =
    gtk_radio_button_new_with_mnemonic (group, _("I_mage"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (private->image_radio));
  gtk_grid_attach (GTK_GRID (grid), private->image_radio, 0, 1, 1, 1);
  gtk_widget_show (private->image_radio);

  g_signal_connect (private->image_radio, "toggled",
                    G_CALLBACK (palette_import_image_callback),
                    private);

  gtk_widget_set_sensitive (private->image_radio,
                            ! gimp_container_is_empty (context->gimp->images));

  private->sample_merged_toggle =
    gtk_check_button_new_with_mnemonic (_("Sample _Merged"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->sample_merged_toggle),
                                TRUE);
  gtk_grid_attach (GTK_GRID (grid), private->sample_merged_toggle, 1, 2, 1, 1);
  gtk_widget_show (private->sample_merged_toggle);

  g_signal_connect_swapped (private->sample_merged_toggle, "toggled",
                            G_CALLBACK (palette_import_make_palette),
                            private);

  private->selection_only_toggle =
    gtk_check_button_new_with_mnemonic (_("_Selected Pixels only"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->selection_only_toggle),
                                FALSE);
  gtk_grid_attach (GTK_GRID (grid), private->selection_only_toggle, 1, 3, 1, 1);
  gtk_widget_show (private->selection_only_toggle);

  g_signal_connect_swapped (private->selection_only_toggle, "toggled",
                            G_CALLBACK (palette_import_make_palette),
                            private);

  private->file_radio =
    gtk_radio_button_new_with_mnemonic (group, _("Palette _file"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (private->image_radio));
  gtk_grid_attach (GTK_GRID (grid), private->file_radio, 0, 4, 1, 1);
  gtk_widget_show (private->file_radio);

  g_signal_connect (private->file_radio, "toggled",
                    G_CALLBACK (palette_import_file_callback),
                    private);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  /*  The gradient menu  */
  private->gradient_combo =
    gimp_container_combo_box_new (gimp_data_factory_get_container (context->gimp->gradient_factory),
                                  private->context, 24, 1);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            NULL, 0.0, 0.5, private->gradient_combo, 1);
  gtk_size_group_add_widget (size_group, private->gradient_combo);

  /*  The image menu  */
  private->image_combo =
    gimp_container_combo_box_new (context->gimp->images, private->context,
                                  24, 1);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            NULL, 0.0, 0.5, private->image_combo, 1);
  gtk_size_group_add_widget (size_group, private->image_combo);

  /*  Palette file name entry  */
  private->file_chooser = gtk_file_chooser_button_new (_("Select Palette File"),
                                                       GTK_FILE_CHOOSER_ACTION_OPEN);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 4,
                            NULL, 0.0, 0.5, private->file_chooser, 1);
  gtk_size_group_add_widget (size_group, private->file_chooser);

  /* Set valid palette files filters */
  palette_import_file_set_filters (GTK_FILE_CHOOSER (private->file_chooser));

  g_object_unref (size_group);

  /*  The "Import" frame  */

  frame = gimp_frame_new (_("Import Options"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  /*  The source's name  */
  private->entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (private->entry),
                      gradient ?
                      gimp_object_get_name (gradient) : _("New import"));
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Palette _name:"), 0.0, 0.5,
                            private->entry, 2);

  /*  The # of colors  */
  private->num_colors = gimp_scale_entry_new (_("N_umber of colors:"),
                                              256, 2, 10000, 0);
  gimp_grid_attach_aligned (GTK_GRID (grid), -1, 1,
                            NULL, 0.0, 0.5,
                            private->num_colors, 3);
  gimp_scale_entry_set_logarithmic (GIMP_SCALE_ENTRY (private->num_colors), TRUE);

  g_signal_connect_swapped (private->num_colors,
                            "value-changed",
                            G_CALLBACK (palette_import_make_palette),
                            private);

  /*  The columns  */
  private->columns = gimp_scale_entry_new (_("C_olumns:"), 16, 0, 64, 0);
  gimp_grid_attach_aligned (GTK_GRID (grid), -1, 2,
                            NULL, 0.0, 0.5,
                            private->columns, 3);

  g_signal_connect (private->columns, "value-changed",
                    G_CALLBACK (palette_import_columns_changed),
                    private);

  /*  The interval  */
  private->threshold = gimp_scale_entry_new (_("I_nterval:"), 1, 1, 128, 0);
  gimp_grid_attach_aligned (GTK_GRID (grid), -1, 3,
                            NULL, 0.0, 0.5,
                            private->threshold, 3);

  g_signal_connect_swapped (private->threshold, "value-changed",
                            G_CALLBACK (palette_import_make_palette),
                            private);


  /*  The "Preview" frame  */
  frame = gimp_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  private->preview = gimp_view_new_full_by_types (private->context,
                                                 GIMP_TYPE_VIEW,
                                                 GIMP_TYPE_PALETTE,
                                                 192, 192, 1,
                                                 TRUE, FALSE, FALSE);
  gtk_widget_set_halign (private->preview, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), private->preview, FALSE, FALSE, 0);
  gtk_widget_show (private->preview);

  private->no_colors_label =
    gtk_label_new (_("The selected source contains no colors."));
  gtk_widget_set_size_request (private->no_colors_label, 194, -1);
  gtk_label_set_line_wrap (GTK_LABEL (private->no_colors_label), TRUE);
  gimp_label_set_attributes (GTK_LABEL (private->no_colors_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), private->no_colors_label, FALSE, FALSE, 0);
  gtk_widget_show (private->no_colors_label);


  /*  keep the dialog up-to-date  */

  g_signal_connect (context->gimp->images, "add",
                    G_CALLBACK (palette_import_image_add),
                    private);
  g_signal_connect (context->gimp->images, "remove",
                    G_CALLBACK (palette_import_image_remove),
                    private);

  g_signal_connect (private->context, "gradient-changed",
                    G_CALLBACK (palette_import_gradient_changed),
                    private);
  g_signal_connect (private->context, "image-changed",
                    G_CALLBACK (palette_import_image_changed),
                    private);
  g_signal_connect (private->file_chooser, "selection-changed",
                    G_CALLBACK (palette_import_filename_changed),
                    private);

  palette_import_grad_callback (private->gradient_radio, private);

  return dialog;
}


/*  private functions  */

static void
palette_import_free (ImportDialog *private)
{
  Gimp *gimp = private->context->gimp;

  g_signal_handlers_disconnect_by_func (gimp->images,
                                        palette_import_image_add,
                                        private);
  g_signal_handlers_disconnect_by_func (gimp->images,
                                        palette_import_image_remove,
                                        private);

  if (private->palette)
    g_object_unref (private->palette);

  g_object_unref (private->context);

  g_slice_free (ImportDialog, private);
}


/*  the palette import response callback  ************************************/

static void
palette_import_response (GtkWidget    *dialog,
                         gint          response_id,
                         ImportDialog *private)
{
  palette_import_image_changed (private->context, NULL, private);

  if (response_id == GTK_RESPONSE_OK)
    {
      Gimp *gimp = private->context->gimp;

      if (private->palette &&
          gimp_palette_get_n_colors (private->palette) > 0)
        {
          const gchar *name = gtk_entry_get_text (GTK_ENTRY (private->entry));
          GError      *error = NULL;

          if (name && *name)
            gimp_object_set_name (GIMP_OBJECT (private->palette), name);

          if (! gimp_data_factory_data_save_single (gimp->palette_factory,
                                                    GIMP_DATA (private->palette),
                                                    &error))
            {
              gimp_message (gimp, G_OBJECT (dialog), GIMP_MESSAGE_ERROR,
                            _("The palette was not imported: %s"),
                            error->message);
              g_clear_error (&error);
              return;
            }

          gimp_container_add (gimp_data_factory_get_container (gimp->palette_factory),
                              GIMP_OBJECT (private->palette));
        }
      else
        {
          gimp_message_literal (gimp, G_OBJECT (dialog), GIMP_MESSAGE_ERROR,
                                _("There is no palette to import."));
          return;
        }
    }

  gtk_widget_destroy (dialog);
}


/*  functions to create & update the import dialog's gradient selection  *****/

static void
palette_import_gradient_changed (GimpContext  *context,
                                 GimpGradient *gradient,
                                 ImportDialog *private)
{
  if (gradient && private->import_type == GRADIENT_IMPORT)
    {
      gtk_entry_set_text (GTK_ENTRY (private->entry),
                          gimp_object_get_name (gradient));

      palette_import_make_palette (private);
    }
}

static void
palette_import_image_changed (GimpContext  *context,
                              GimpImage    *image,
                              ImportDialog *private)
{
  if (private->image)
    {
      g_signal_handlers_disconnect_by_func (private->image,
                                            palette_import_layer_changed,
                                            private);
      g_signal_handlers_disconnect_by_func (private->image,
                                            palette_import_mask_changed,
                                            private);
    }

  private->image = image;

  if (private->import_type == IMAGE_IMPORT)
    {
      gboolean sensitive = FALSE;

      if (image)
        {
          gchar *label;

          label = g_strdup_printf ("%s-%d",
                                   gimp_image_get_display_name (image),
                                   gimp_image_get_id (image));

          gtk_entry_set_text (GTK_ENTRY (private->entry), label);
          g_free (label);

          palette_import_make_palette (private);

          if (gimp_image_get_base_type (image) != GIMP_INDEXED)
            sensitive = TRUE;
        }

      gtk_widget_set_sensitive (private->sample_merged_toggle, sensitive);
      gtk_widget_set_sensitive (private->selection_only_toggle, sensitive);
      gtk_widget_set_sensitive (private->threshold, sensitive);
      gtk_widget_set_sensitive (private->num_colors, sensitive);
    }

  if (private->image)
    {
      g_signal_connect (private->image, "selected-layers-changed",
                        G_CALLBACK (palette_import_layer_changed),
                        private);
      g_signal_connect (private->image, "mask-changed",
                        G_CALLBACK (palette_import_mask_changed),
                        private);
    }
}

static void
palette_import_layer_changed (GimpImage    *image,
                              ImportDialog *private)
{
  if (private->import_type == IMAGE_IMPORT &&
      ! gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON (private->sample_merged_toggle)))
    {
      palette_import_make_palette (private);
    }
}

static void
palette_import_mask_changed (GimpImage    *image,
                             ImportDialog *private)
{
  if (private->import_type == IMAGE_IMPORT &&
      gtk_toggle_button_get_active
      (GTK_TOGGLE_BUTTON (private->selection_only_toggle)))
    {
      palette_import_make_palette (private);
    }
}

static void
palette_import_filename_changed (GtkFileChooser *button,
                                 ImportDialog   *private)
{
  gchar *filename;

  if (private->import_type != FILE_IMPORT)
    return;

  filename = gtk_file_chooser_get_filename (button);

  if (filename)
    {
      gchar *basename = g_filename_display_basename (filename);

      /* TODO: strip filename extension */
      gtk_entry_set_text (GTK_ENTRY (private->entry), basename);
      g_free (basename);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (private->entry), "");
    }

  g_free (filename);

  palette_import_make_palette (private);
}

static void
import_dialog_drop_callback (GtkWidget    *widget,
                             gint          x,
                             gint          y,
                             GimpViewable *viewable,
                             gpointer      data)
{
  ImportDialog *private = data;

  gimp_context_set_by_type (private->context,
                            G_TYPE_FROM_INSTANCE (viewable),
                            GIMP_OBJECT (viewable));

  if (GIMP_IS_GRADIENT (viewable) &&
      private->import_type != GRADIENT_IMPORT)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->gradient_radio),
                                    TRUE);
    }
  else if (GIMP_IS_IMAGE (viewable) &&
           private->import_type != IMAGE_IMPORT)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->image_radio),
                                    TRUE);
    }
}


/*  the import source menu item callbacks  ***********************************/

static void
palette_import_set_sensitive (ImportDialog *private)
{
  gboolean gradient = (private->import_type == GRADIENT_IMPORT);
  gboolean image    = (private->import_type == IMAGE_IMPORT);
  gboolean file     = (private->import_type == FILE_IMPORT);

  gtk_widget_set_sensitive (private->gradient_combo,        gradient);
  gtk_widget_set_sensitive (private->image_combo,           image);
  gtk_widget_set_sensitive (private->sample_merged_toggle,  image);
  gtk_widget_set_sensitive (private->selection_only_toggle, image);
  gtk_widget_set_sensitive (private->file_chooser,          file);

  gtk_widget_set_sensitive (private->num_colors, ! file);
  gtk_widget_set_sensitive (private->columns,    ! file);
  gtk_widget_set_sensitive (private->threshold,  image);
}

static void
palette_import_grad_callback (GtkWidget    *widget,
                              ImportDialog *private)
{
  GimpGradient *gradient;

  if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    return;

  private->import_type = GRADIENT_IMPORT;

  gradient = gimp_context_get_gradient (private->context);

  gtk_entry_set_text (GTK_ENTRY (private->entry),
                      gimp_object_get_name (gradient));

  palette_import_set_sensitive (private);

  palette_import_make_palette (private);
}

static void
palette_import_image_callback (GtkWidget    *widget,
                               ImportDialog *private)
{
  GimpImage *image;

  if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    return;

  private->import_type = IMAGE_IMPORT;

  image = gimp_context_get_image (private->context);

  if (! image)
    {
      GimpContainer *images = private->context->gimp->images;

      image = GIMP_IMAGE (gimp_container_get_first_child (images));
    }

  palette_import_set_sensitive (private);

  palette_import_image_changed (private->context, image, private);
}

static void
palette_import_file_callback (GtkWidget    *widget,
                              ImportDialog *private)
{
  gchar *filename;

  if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    return;

  private->import_type = FILE_IMPORT;

  filename =
    gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (private->file_chooser));

  if (filename)
    {
      gchar *basename = g_filename_display_basename (filename);

      /* TODO: strip filename extension */
      gtk_entry_set_text (GTK_ENTRY (private->entry), basename);
      g_free (basename);

      g_free (filename);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (private->entry), "");
    }

  palette_import_set_sensitive (private);
}

static void
palette_import_columns_changed (GimpLabelSpin *columns,
                                ImportDialog  *private)
{
  if (private->palette)
    gimp_palette_set_columns (private->palette,
                              ROUND (gimp_label_spin_get_value (columns)));
}


/*  functions & callbacks to keep the import dialog uptodate  ****************/

static void
palette_import_image_add (GimpContainer *container,
                          GimpImage     *image,
                          ImportDialog  *private)
{
  if (! gtk_widget_is_sensitive (private->image_radio))
    {
      gtk_widget_set_sensitive (private->image_radio, TRUE);
      gimp_context_set_image (private->context, image);
    }
}

static void
palette_import_image_remove (GimpContainer *container,
                             GimpImage     *image,
                             ImportDialog  *private)
{
  if (! gimp_container_get_n_children (private->context->gimp->images))
    {
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (private->image_radio)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->gradient_radio),
                                      TRUE);

      gtk_widget_set_sensitive (private->image_radio, FALSE);
    }
}

static void
palette_import_make_palette (ImportDialog *private)
{
  GimpPalette  *palette = NULL;
  const gchar  *palette_name;
  gint          n_colors;
  gint          n_columns;
  gint          threshold;

  palette_name = gtk_entry_get_text (GTK_ENTRY (private->entry));

  if (! palette_name || ! strlen (palette_name))
    palette_name = _("Untitled");

  n_colors  = ROUND (gimp_label_spin_get_value (GIMP_LABEL_SPIN (private->num_colors)));
  n_columns = ROUND (gimp_label_spin_get_value (GIMP_LABEL_SPIN (private->columns)));
  threshold = ROUND (gimp_label_spin_get_value (GIMP_LABEL_SPIN (private->threshold)));

  switch (private->import_type)
    {
    case GRADIENT_IMPORT:
      {
        GimpGradient *gradient;

        gradient = gimp_context_get_gradient (private->context);

        palette = gimp_palette_import_from_gradient (gradient,
                                                     private->context,
                                                     FALSE,
                                                     GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL,
                                                     palette_name,
                                                     n_colors);
      }
      break;

    case IMAGE_IMPORT:
      {
        GimpImage *image = gimp_context_get_image (private->context);
        gboolean   sample_merged;
        gboolean   selection_only;

        sample_merged =
          gtk_toggle_button_get_active
          (GTK_TOGGLE_BUTTON (private->sample_merged_toggle));

        selection_only =
          gtk_toggle_button_get_active
          (GTK_TOGGLE_BUTTON (private->selection_only_toggle));

        if (gimp_image_get_base_type (image) == GIMP_INDEXED)
          {
            palette = gimp_palette_import_from_indexed_image (image,
                                                              private->context,
                                                              palette_name);
          }
        else if (sample_merged)
          {
            palette = gimp_palette_import_from_image (image,
                                                      private->context,
                                                      palette_name,
                                                      n_colors,
                                                      threshold,
                                                      selection_only);
          }
        else
          {
            GList *drawables;

            drawables = gimp_image_get_selected_layers (image);

            if (drawables)
              palette = gimp_palette_import_from_drawables (drawables,
                                                            private->context,
                                                            palette_name,
                                                            n_colors,
                                                            threshold,
                                                            selection_only);
          }
      }
      break;

    case FILE_IMPORT:
      {
        GFile  *file;
        GError *error = NULL;

        file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (private->file_chooser));

        palette = gimp_palette_import_from_file (private->context,
                                                 file,
                                                 palette_name, &error);
        g_object_unref (file);

        if (! palette)
          {
            gimp_message_literal (private->context->gimp,
                                  G_OBJECT (private->dialog), GIMP_MESSAGE_ERROR,
                                  error->message);
            g_error_free (error);
          }
        else
          {
            gint columns = gimp_palette_get_columns (palette);

            if (columns > 0)
              {
                n_columns = columns;

                gimp_label_spin_set_value (GIMP_LABEL_SPIN (private->columns),
                                           n_columns);
              }
          }
      }
      break;
    }

  if (private->palette)
    g_object_unref (private->palette);

  private->palette = palette;

  if (palette)
    {
      gimp_palette_set_columns (palette, n_columns);

      gimp_view_set_viewable (GIMP_VIEW (private->preview),
                              GIMP_VIEWABLE (palette));

    }

  gtk_widget_set_visible (private->no_colors_label,
                          ! (palette &&
                             gimp_palette_get_n_colors (palette) > 0));
}

static void
palette_import_file_set_filters (GtkFileChooser *file_chooser)
{
  GtkFileFilter *filter;

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All palette files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (file_chooser, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("GIMP Palette (*.gpl)"));
  gtk_file_filter_add_pattern (filter, "*.gpl");
  gtk_file_filter_add_pattern (filter, "*.GPL");
  gtk_file_chooser_add_filter (file_chooser, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Adobe Color Table (*.act)"));
  gtk_file_filter_add_pattern (filter, "*.act");
  gtk_file_filter_add_pattern (filter, "*.ACT");
  gtk_file_chooser_add_filter (file_chooser, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Adobe Color Swatch (*.aco)"));
  gtk_file_filter_add_pattern (filter, "*.aco");
  gtk_file_filter_add_pattern (filter, "*.ACO");
  gtk_file_chooser_add_filter (file_chooser, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Adobe Color Book (*.acb)"));
  gtk_file_filter_add_pattern (filter, "*.acb");
  gtk_file_filter_add_pattern (filter, "*.ACB");
  gtk_file_chooser_add_filter (file_chooser, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Adobe Swatch Exchange (*.ase)"));
  gtk_file_filter_add_pattern (filter, "*.ase");
  gtk_file_filter_add_pattern (filter, "*.ASE");
  gtk_file_chooser_add_filter (file_chooser, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Cascading Style Sheet (*.css)"));
  gtk_file_filter_add_pattern (filter, "*.css");
  gtk_file_filter_add_pattern (filter, "*.CSS");
  gtk_file_chooser_add_filter (file_chooser, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("JASC or RIFF Palette (*.pal)"));
  gtk_file_filter_add_pattern (filter, "*.pal");
  gtk_file_filter_add_pattern (filter, "*.PAL");
  gtk_file_chooser_add_filter (file_chooser, filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("SwatchBooker (*.sbz)"));
  gtk_file_filter_add_pattern (filter, "*.sbz");
  gtk_file_filter_add_pattern (filter, "*.SBZ");
  gtk_file_chooser_add_filter (file_chooser, filter);
}
