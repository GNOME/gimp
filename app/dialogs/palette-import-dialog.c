/* The GIMP -- an image manipulation program
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpimage.h"
#include "core/gimppalette.h"
#include "core/gimppalette-import.h"

#include "widgets/gimpcontainermenuimpl.h"
#include "widgets/gimppreview.h"

#include "gradient-select.h"
#include "palette-import-dialog.h"

#include "libgimp/gimpintl.h"


#define IMPORT_PREVIEW_SIZE 80


typedef enum
{
  GRADIENT_IMPORT,
  IMAGE_IMPORT
} ImportType;


typedef struct _ImportDialog ImportDialog;

struct _ImportDialog
{
  GtkWidget      *dialog;

  GimpContext    *context;
  GradientSelect *gradient_select;
  ImportType      import_type;

  GtkWidget      *select_area;
  GtkWidget      *preview;
  GtkWidget      *select_button;
  GtkWidget      *image_menu;

  GtkWidget      *entry;
  GtkWidget      *image_menu_item_image;
  GtkWidget      *image_menu_item_gradient;
  GtkWidget      *type_option;
  GtkWidget      *threshold_scale;
  GtkWidget      *threshold_text;
  GtkAdjustment  *threshold;
  GtkAdjustment  *sample;
};


static ImportDialog *import_dialog = NULL;


/*****************************************************************************/
/*  palette import dialog functions  *****************************************/

/*  functions to create & update the import dialog's gradient selection  *****/

static void
palette_import_select_grad_callback (GtkWidget *widget,
				     gpointer   data)
{
  ImportDialog *import_dialog;
  GimpGradient *gradient;

  import_dialog = (ImportDialog *) data;

  gradient = gimp_context_get_gradient (import_dialog->context);

  import_dialog->gradient_select =
    gradient_select_new (import_dialog->context->gimp,
                         import_dialog->context,
                         _("Select a Gradient to Create a Palette from"),
                         GIMP_OBJECT (gradient)->name,
                         NULL,
                         0);

  g_object_add_weak_pointer (G_OBJECT (import_dialog->gradient_select->shell),
                             (gpointer *) &import_dialog->gradient_select);
}

static void
palette_import_gradient_changed (GimpContext  *context,
                                 GimpGradient *gradient,
                                 gpointer      data)
{
  ImportDialog *import_dialog;

  import_dialog = (ImportDialog *) data;

  if (gradient && import_dialog->import_type == GRADIENT_IMPORT)
    {
      gimp_preview_set_viewable (GIMP_PREVIEW (import_dialog->preview),
                                 GIMP_VIEWABLE (gradient));
      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry),
			  GIMP_OBJECT (gradient)->name);
    }
}

static void
palette_import_image_changed (GimpContext *context,
                              GimpImage   *gimage,
                              gpointer     data)
{
  ImportDialog *import_dialog;

  import_dialog = (ImportDialog *) data;

  if (gimage && import_dialog->import_type == IMAGE_IMPORT)
    {
      gchar *basename;
      gchar *label;

      gimp_preview_set_viewable (GIMP_PREVIEW (import_dialog->preview),
                                 GIMP_VIEWABLE (gimage));

      basename = g_path_get_basename (gimp_image_get_filename (gimage));
      label = g_strdup_printf ("%s-%d", basename, gimp_image_get_ID (gimage));
      g_free (basename);

      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), label);

      g_free (label);
    }
}


/*  the import source menu item callbacks  ***********************************/

static void
palette_import_grad_callback (GtkWidget *widget,
			      gpointer   data)
{
  ImportDialog *import_dialog;
  GimpGradient *gradient;

  import_dialog = (ImportDialog *) data;

  gradient = gimp_context_get_gradient (import_dialog->context);

  import_dialog->import_type = GRADIENT_IMPORT;

  gtk_widget_hide (import_dialog->image_menu);
  gtk_widget_show (import_dialog->select_button);

  gtk_widget_destroy (import_dialog->preview);

  import_dialog->preview = gimp_preview_new_full (GIMP_VIEWABLE (gradient),
                                                  IMPORT_PREVIEW_SIZE,
                                                  IMPORT_PREVIEW_SIZE,
                                                  1, FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX (import_dialog->select_area),
                      import_dialog->preview,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (import_dialog->select_area),
                         import_dialog->preview, 0);
  gtk_widget_show (import_dialog->preview);

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry),
                      GIMP_OBJECT (gradient)->name);
  gtk_widget_set_sensitive (import_dialog->threshold_scale, FALSE);
  gtk_widget_set_sensitive (import_dialog->threshold_text, FALSE);
}

static void
palette_import_image_callback (GtkWidget *widget,
			       gpointer   data)
{
  ImportDialog *import_dialog;
  GimpImage    *gimage;

  import_dialog = (ImportDialog *) data;

  gimage = gimp_context_get_image (import_dialog->context);

  if (! gimage)
    {
      gimage = (GimpImage *)
        gimp_container_get_child_by_index (import_dialog->context->gimp->images, 0);
    }

  import_dialog->import_type = IMAGE_IMPORT;

  gtk_widget_hide (import_dialog->select_button);
  gtk_widget_show (import_dialog->image_menu);

  gtk_widget_destroy (import_dialog->preview);

  import_dialog->preview = gimp_preview_new_full (GIMP_VIEWABLE (gimage),
                                                  IMPORT_PREVIEW_SIZE,
                                                  IMPORT_PREVIEW_SIZE,
                                                  1, FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX (import_dialog->select_area),
                      import_dialog->preview,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (import_dialog->select_area),
                         import_dialog->preview, 0);
  gtk_widget_show (import_dialog->preview);

  gtk_widget_set_sensitive (import_dialog->threshold_scale, TRUE);
  gtk_widget_set_sensitive (import_dialog->threshold_text, TRUE);
}


/*  functions & callbacks to keep the import dialog uptodate  ****************/

static void
palette_import_image_add (GimpContainer *container,
			  GimpImage     *gimage,
			  gpointer       data)
{
  ImportDialog *import_dialog;

  import_dialog = (ImportDialog *) data;

  if (! GTK_WIDGET_IS_SENSITIVE (import_dialog->image_menu_item_image))
    {
      gtk_widget_set_sensitive (import_dialog->image_menu_item_image, TRUE);
    }
}

static void
palette_import_image_remove (GimpContainer *container,
                             GimpImage     *gimage,
                             gpointer       data)
{
  ImportDialog *import_dialog;

  import_dialog = (ImportDialog *) data;

  if (! gimp_container_num_children (import_dialog->context->gimp->images))
    {
      gimp_option_menu_set_history (GTK_OPTION_MENU (import_dialog->type_option),
                                    GINT_TO_POINTER (GRADIENT_IMPORT));
      gtk_widget_set_sensitive (import_dialog->image_menu_item_image, FALSE);

      palette_import_grad_callback (NULL, import_dialog);
    }
}


/*  the palette import action area callbacks  ********************************/

static void
palette_import_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  if (import_dialog->gradient_select)
    gradient_select_free (import_dialog->gradient_select);

  gtk_widget_destroy (import_dialog->dialog);
  g_free (import_dialog);
  import_dialog = NULL;
}

static void
palette_import_import_callback (GtkWidget *widget,
				gpointer   data)
{
  ImportDialog *import_dialog;
  GimpPalette  *palette = NULL;
  gchar        *palette_name;
  GimpGradient *gradient;
  GimpImage    *gimage;
  gint          n_colors;
  gint          threshold;

  import_dialog = (ImportDialog *) data;

  palette_name = (gchar *) gtk_entry_get_text (GTK_ENTRY (import_dialog->entry));

  if (! (palette_name && strlen (palette_name)))
    palette_name = g_strdup (_("Untitled"));
  else
    palette_name = g_strdup (palette_name);

  gradient  = gimp_context_get_gradient (import_dialog->context);
  gimage    = gimp_context_get_image (import_dialog->context);

  n_colors  = (gint) import_dialog->sample->value;
  threshold = (gint) import_dialog->threshold->value;

  switch (import_dialog->import_type)
    {
    case GRADIENT_IMPORT:
      palette = gimp_palette_import_from_gradient (gradient,
						   palette_name,
						   n_colors);
      break;

    case IMAGE_IMPORT:
      if (gimp_image_base_type (gimage) == GIMP_INDEXED)
        {
          palette = gimp_palette_import_from_image (gimage,
                                                    palette_name,
                                                    n_colors,
                                                    threshold);
        }
      else
        {
          palette = 
            gimp_palette_import_from_indexed_image (gimage,
                                                    palette_name);
        }
      break;
    }

  g_free (palette_name);

  if (palette)
    gimp_container_add (import_dialog->context->gimp->palette_factory->container,
			GIMP_OBJECT (palette));

  palette_import_close_callback (NULL, NULL);
}

/*  the palette import dialog constructor  ***********************************/

static ImportDialog *
palette_import_dialog_new (Gimp *gimp)
{
  ImportDialog *import_dialog;
  GimpGradient *gradient;
  GtkWidget    *hbox;
  GtkWidget    *hbox2;
  GtkWidget    *frame;
  GtkWidget    *vbox;
  GtkWidget    *table;
  GtkWidget    *label;
  GtkWidget    *spinbutton;
  GtkWidget    *menu;

  gradient = gimp_context_get_gradient (gimp_get_user_context (gimp));

  import_dialog = g_new0 (ImportDialog, 1);

  import_dialog->context = gimp_context_new (gimp, "Palette Import",
                                             gimp_get_user_context (gimp));

  import_dialog->dialog =
    gimp_dialog_new (_("Import Palette"), "import_palette",
                     gimp_standard_help_func,
                     "dialogs/palette_editor/import_palette.html",
                     GTK_WIN_POS_NONE,
                     FALSE, TRUE, FALSE,

                     GTK_STOCK_CANCEL, palette_import_close_callback,
                     import_dialog, NULL, NULL, TRUE, TRUE,

                     _("Import"), palette_import_import_callback,
                     import_dialog, NULL, NULL, FALSE, FALSE,

                     NULL);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (import_dialog->dialog)->vbox),
                     hbox);
  gtk_widget_show (hbox);

  /*  The "Import" frame  */
  frame = gtk_frame_new (_("Import"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The source's name  */
  label = gtk_label_new (_("Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  import_dialog->entry = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), import_dialog->entry,
                             1, 2, 0, 1);
  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry),
                      gradient ? GIMP_OBJECT (gradient)->name : _("new_import"));
  gtk_widget_show (import_dialog->entry);

  /*  The source type  */
  label = gtk_label_new (_("Source:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  import_dialog->type_option =
    gimp_option_menu_new (FALSE,

                          _("Gradient"),
                          palette_import_grad_callback, import_dialog,
                          GINT_TO_POINTER (GRADIENT_IMPORT),
                          &import_dialog->image_menu_item_gradient,
                          TRUE,

                          _("Image"),
                          palette_import_image_callback, import_dialog,
                          GINT_TO_POINTER (IMAGE_IMPORT),
                          &import_dialog->image_menu_item_image,
                          FALSE,

                          NULL);

  gtk_widget_set_sensitive (import_dialog->image_menu_item_image,
			    gimp_container_num_children (gimp->images) > 0);

  gtk_table_attach_defaults (GTK_TABLE (table), import_dialog->type_option,
                             1, 2, 1, 2);
  gtk_widget_show (import_dialog->type_option);

  /*  The sample size  */
  label = gtk_label_new (_("Sample Size:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  import_dialog->sample =
    GTK_ADJUSTMENT(gtk_adjustment_new (256, 2, 10000, 1, 10, 10));
  spinbutton = gtk_spin_button_new (import_dialog->sample, 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 2, 3);
  gtk_widget_show (spinbutton);

  /*  The interval  */
  import_dialog->threshold_text = gtk_label_new (_("Interval:"));
  gtk_misc_set_alignment (GTK_MISC (import_dialog->threshold_text), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), import_dialog->threshold_text,
                    0, 1, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_sensitive (import_dialog->threshold_text, FALSE);
  gtk_widget_show (import_dialog->threshold_text);

  import_dialog->threshold = GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 128,
                                                                 1, 1, 1));

  import_dialog->threshold_scale = gtk_hscale_new (import_dialog->threshold);
  gtk_scale_set_value_pos (GTK_SCALE (import_dialog->threshold_scale),
                           GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (import_dialog->threshold_scale), 0);
  gtk_table_attach_defaults (GTK_TABLE (table), import_dialog->threshold_scale,
                             1, 2, 3, 4);
  gtk_widget_set_sensitive (import_dialog->threshold_scale, FALSE);
  gtk_widget_show (import_dialog->threshold_scale);

  /*  The preview frame  */
  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = import_dialog->select_area = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  import_dialog->preview = gimp_preview_new_full (GIMP_VIEWABLE (gradient),
                                                  IMPORT_PREVIEW_SIZE,
                                                  IMPORT_PREVIEW_SIZE,
                                                  1, FALSE, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), import_dialog->preview, FALSE, FALSE, 0);
  gtk_widget_show (import_dialog->preview);

  import_dialog->select_button = gtk_button_new_with_label (_("Select"));
  GTK_WIDGET_UNSET_FLAGS (import_dialog->select_button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (vbox), import_dialog->select_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (import_dialog->select_button);

  g_signal_connect (G_OBJECT (import_dialog->select_button), "clicked", 
		    G_CALLBACK (palette_import_select_grad_callback),
		    import_dialog);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  import_dialog->image_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox2), import_dialog->image_menu, TRUE, TRUE, 0);
  /* DON'T gtk_widget_show (import_dialog->image_menu); */

  menu = gimp_container_menu_new (gimp->images, import_dialog->context, 24);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (import_dialog->image_menu), menu);
  gtk_widget_show (menu);

  import_dialog->import_type = GRADIENT_IMPORT;

  /*  keep the dialog up-to-date  */

  g_signal_connect (G_OBJECT (gimp->images), "add",
                    G_CALLBACK (palette_import_image_add),
                    import_dialog);
  g_signal_connect (G_OBJECT (gimp->images), "remove",
                    G_CALLBACK (palette_import_image_remove),
                    import_dialog);

  g_signal_connect (G_OBJECT (import_dialog->context), "gradient_changed",
		    G_CALLBACK (palette_import_gradient_changed),
		    import_dialog);
  g_signal_connect (G_OBJECT (import_dialog->context), "image_changed",
		    G_CALLBACK (palette_import_image_changed),
		    import_dialog);

  return import_dialog;
}

void
palette_import_dialog_show (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! import_dialog)
    import_dialog = palette_import_dialog_new (gimp);

  if (! GTK_WIDGET_VISIBLE (import_dialog->dialog))
    {
      gtk_widget_show (import_dialog->dialog);
    }
  else if (import_dialog->dialog->window)
    {
      gdk_window_raise (import_dialog->dialog->window);
    }
}

void
palette_import_dialog_destroy (void)
{
  if (import_dialog)
    {
      gtk_widget_destroy (import_dialog->dialog);
      g_free (import_dialog);
      import_dialog = NULL;
    }
}
