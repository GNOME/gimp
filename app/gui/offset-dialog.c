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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-offset.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay-foreach.h"

#include "libgimp/gimpintl.h"


#define ENTRY_WIDTH  60


typedef struct _OffsetDialog OffsetDialog;

struct _OffsetDialog
{
  GtkWidget      *dlg;
  GtkWidget      *off_se;

  gboolean        wrap_around;
  GimpOffsetType  fill_type;

  GimpImage      *gimage;
};


/*  Forward declarations  */
static void  offset_ok_callback         (GtkWidget *widget,
					 gpointer   data);
static void  offset_cancel_callback     (GtkWidget *widget,
					 gpointer   data);

static void  offset_halfheight_callback (GtkWidget *widget,
					 gpointer   data);


void
offset_dialog_create (GimpDrawable *drawable)
{
  OffsetDialog *off_d;
  GtkWidget    *label;
  GtkWidget    *check;
  GtkWidget    *push;
  GtkWidget    *vbox;
  GtkWidget    *table;
  GtkObject    *adjustment;
  GtkWidget    *spinbutton;
  GtkWidget    *frame;
  GtkWidget    *radio_button;

  off_d = g_new0 (OffsetDialog, 1);

  off_d->wrap_around = TRUE;
  off_d->fill_type   = gimp_drawable_has_alpha (drawable);
  off_d->gimage      = gimp_drawable_gimage (drawable);

  off_d->dlg = gimp_dialog_new (_("Offset"), "offset",
				gimp_standard_help_func,
				"dialogs/offset.html",
				GTK_WIN_POS_NONE,
				FALSE, TRUE, FALSE,

				GTK_STOCK_OK, offset_ok_callback,
				off_d, NULL, NULL, TRUE, FALSE,
				GTK_STOCK_CANCEL, offset_cancel_callback,
				off_d, NULL, NULL, FALSE, TRUE,

				NULL);
				
  /*  The vbox for first column of options  */
  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (off_d->dlg)->vbox), vbox);

  /*  The table for the offsets  */
  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  The offset labels  */
  label = gtk_label_new (_("Offset X:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Y:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /*  The offset sizeentry  */
  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 1);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  
  off_d->off_se = gimp_size_entry_new (1, off_d->gimage->unit, "%a",
				       TRUE, TRUE, FALSE, 75,
				       GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (off_d->off_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (off_d->off_se), spinbutton,
                             1, 2, 0, 1);
  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), off_d->off_se, 1, 2, 0, 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (off_d->off_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (off_d->off_se), GIMP_UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (off_d->off_se), 0,
                                  off_d->gimage->xresolution, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (off_d->off_se), 1,
                                  off_d->gimage->yresolution, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (off_d->off_se), 0,
                                         -off_d->gimage->width,
					 off_d->gimage->width);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (off_d->off_se), 1,
					 -off_d->gimage->height,
					 off_d->gimage->height);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (off_d->off_se), 0,
                            0, off_d->gimage->width);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (off_d->off_se), 1,
                            0, off_d->gimage->height);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (off_d->off_se), 0, 0);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (off_d->off_se), 1, 0);

  gtk_widget_show (table);

  /*  The wrap around option  */
  check = gtk_check_button_new_with_label (_("Wrap Around"));
  gtk_box_pack_start (GTK_BOX (vbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);

  /*  The fill options  */
  frame =
    gimp_radio_group_new2 (TRUE, _("Fill Type"),
			   G_CALLBACK (gimp_radio_button_update),
			   &off_d->fill_type,
			   GINT_TO_POINTER (off_d->fill_type),

			   _("Background"),
			   GINT_TO_POINTER (OFFSET_BACKGROUND), NULL,
			   _("Transparent"),
			   GINT_TO_POINTER (OFFSET_TRANSPARENT), &radio_button,

			   NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  if (! gimp_drawable_has_alpha (drawable))
    gtk_widget_set_sensitive (radio_button, FALSE);

  /*  The by half height and half width option */
  push = gtk_button_new_with_label (_("Offset by (x/2),(y/2)"));
  gtk_container_set_border_width (GTK_CONTAINER (push), 2);
  gtk_box_pack_start (GTK_BOX (vbox), push, FALSE, FALSE, 0);
  gtk_widget_show (push);

  /*  Hook up the wrap around  */
  g_signal_connect (G_OBJECT (check), "toggled",
		    G_CALLBACK (gimp_toggle_button_update),
		    &off_d->wrap_around);
  g_object_set_data (G_OBJECT (check), "inverse_sensitive", frame);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), off_d->wrap_around);

  /*  Hook up the by half  */
  g_signal_connect (G_OBJECT (push), "clicked",
		    G_CALLBACK (offset_halfheight_callback),
		    off_d);

  gtk_widget_show (vbox);
  gtk_widget_show (off_d->dlg);
}


/*  private function  */

static void
offset_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  OffsetDialog *off_d;
  GimpImage    *gimage;
  GimpDrawable *drawable;
  gint          offset_x;
  gint          offset_y;

  off_d = (OffsetDialog *) data;

  if ((gimage = off_d->gimage) != NULL)
    {
      drawable = gimp_image_active_drawable (gimage);

      offset_x = (gint)
	RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (off_d->off_se), 0));
      offset_y = (gint)
	RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (off_d->off_se), 1));

      gimp_drawable_offset (drawable,
			    off_d->wrap_around, off_d->fill_type,
			    offset_x, offset_y);
      gdisplays_flush ();
    }

  gtk_widget_destroy (off_d->dlg);
  g_free (off_d);
}

static void
offset_cancel_callback (GtkWidget *widget,
			gpointer   data)
{
  OffsetDialog *off_d;

  off_d = (OffsetDialog *) data;
  gtk_widget_destroy (off_d->dlg);
  g_free (off_d);
}

static void
offset_halfheight_callback (GtkWidget *widget,
			    gpointer   data)
{
  OffsetDialog *off_d;
  GimpImage    *gimage;

  off_d = (OffsetDialog *) data;
  gimage = off_d->gimage;

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (off_d->off_se),
			      0, gimage->width / 2);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (off_d->off_se),
			      1, gimage->height / 2);
}
