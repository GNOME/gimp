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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpmarshal.h"

#include "display/gimpdisplayshell-render.h"

#include "widgets/gimpdnd.h"

#include "color-notebook.h"
#include "colormap-dialog.h"
#include "color-area.h"

#include "libgimp/gimpintl.h"


/*  Add these features:
 *
 *  load/save colormaps
 *  requantize
 *  add color--by clicking in the checked region
 *  all changes need to flush colormap lookup cache
 */


enum
{
  SELECTED,
  LAST_SIGNAL
};


static void   gimp_colormap_dialog_class_init (GimpColormapDialogClass *klass);
static void   gimp_colormap_dialog_init       (GimpColormapDialog      *colormap_dialog);

static void   ipal_create_popup_menu     (GimpColormapDialog *ipal);

/*  indexed palette routines  */
static void   ipal_draw                  (GimpColormapDialog *ipal);
static void   ipal_clear                 (GimpColormapDialog *ipal,
					  gint                start_row);
static void   ipal_update_entries        (GimpColormapDialog *ipal);
static void   ipal_set_index             (GimpColormapDialog *ipal,
				          gint                i);
static void   ipal_draw_cell             (GimpColormapDialog *ipal,
				          gint                col);

/*  indexed palette menu callbacks  */
static void   ipal_add_callback          (GtkWidget          *widget,
					  gpointer            data);
static void   ipal_edit_callback         (GtkWidget          *widget,
					  gpointer            data);
static void   ipal_select_callback       (ColorNotebook      *color_notebook,
					  const GimpRGB      *color,
					  ColorNotebookState  state,
					  gpointer            data);

/*  event callback  */
static gint   ipal_area_button_press     (GtkWidget          *widget,
					  GdkEventButton     *bevent,
					  GimpColormapDialog *ipal);

/*  create image menu  */
static void   ipal_area_size_allocate    (GtkWidget          *widget,
					  GtkAllocation      *allocation,
					  GimpColormapDialog *ipal);

static void   index_adjustment_change_cb (GtkAdjustment      *adjustment,
					  GimpColormapDialog *ipal);
static void   hex_entry_change_cb        (GtkEntry           *entry,
					  GimpColormapDialog *ipal);

static void   image_mode_changed_cb      (GimpImage          *gimage,
					  GimpColormapDialog *ipal);
static void   image_colormap_changed_cb  (GimpImage          *gimage,
					  gint                ncol,
					  GimpColormapDialog *ipal);

static void   ipal_drag_color            (GtkWidget          *widget,
					  GimpRGB            *color,
					  gpointer            data);
static void   ipal_drop_color            (GtkWidget          *widget,
					  const GimpRGB      *color,
					  gpointer            data);


static guint gimp_colormap_dialog_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;


/*  dnd stuff  */
static GtkTargetEntry color_palette_target_table[] =
{
  GIMP_TARGET_COLOR
};


GType
gimp_colormap_dialog_get_type (void)
{
  static GType gcd_type = 0;

  if (! gcd_type)
    {
      static const GTypeInfo gcd_info =
      {
        sizeof (GimpColormapDialogClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_colormap_dialog_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColormapDialog),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_colormap_dialog_init,
      };

      gcd_type = g_type_register_static (GTK_TYPE_VBOX,
                                         "GimpColormapDialog",
                                         &gcd_info, 0);
    }

  return gcd_type;
}

static void
gimp_colormap_dialog_class_init (GimpColormapDialogClass* klass)
{
  parent_class = g_type_class_peek_parent (klass);

  gimp_colormap_dialog_signals[SELECTED] =
    g_signal_new ("selected",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpColormapDialogClass, selected),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  klass->selected = NULL;
}

static void
gimp_colormap_dialog_init (GimpColormapDialog *colormap_dialog)
{
  colormap_dialog->image            = NULL;
  colormap_dialog->col_index        = 0;
  colormap_dialog->dnd_col_index    = 0;
  colormap_dialog->palette          = NULL;
  colormap_dialog->image_menu       = NULL;
  colormap_dialog->popup_menu       = NULL;
  colormap_dialog->option_menu      = NULL;
  colormap_dialog->xn               = 0;
  colormap_dialog->yn               = 0;
  colormap_dialog->cellsize         = 0;
  colormap_dialog->index_adjustment = NULL;
  colormap_dialog->index_spinbutton = NULL;
  colormap_dialog->color_entry      = NULL;
  colormap_dialog->add_item         = NULL;
  colormap_dialog->color_notebook   = NULL;
}

GtkWidget *
gimp_colormap_dialog_new (GimpImage *gimage)
{
  GimpColormapDialog *ipal;
  GtkWidget          *frame;
  GtkWidget          *table;

  g_return_val_if_fail (! gimage || GIMP_IS_IMAGE (gimage), NULL);

  ipal = g_object_new (GIMP_TYPE_COLORMAP_DIALOG, NULL);

  gtk_box_set_spacing (GTK_BOX (ipal), 2);

  ipal_create_popup_menu (ipal);

  /*  The palette frame  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (ipal), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  ipal->palette = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_widget_set_size_request (ipal->palette, -1, 60);
  gtk_preview_set_expand (GTK_PREVIEW (ipal->palette), TRUE);
  gtk_widget_add_events (ipal->palette, GDK_BUTTON_PRESS_MASK);
  gtk_container_add (GTK_CONTAINER (frame), ipal->palette);
  gtk_widget_show (ipal->palette);

  g_signal_connect_after (G_OBJECT (ipal->palette), "size_allocate",
			  G_CALLBACK (ipal_area_size_allocate),
			  ipal);

  g_signal_connect (G_OBJECT (ipal->palette), "button_press_event",
		    G_CALLBACK (ipal_area_button_press),
		    ipal);

  /*  dnd stuff  */
  gtk_drag_source_set (ipal->palette,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_palette_target_table,
                       G_N_ELEMENTS (color_palette_target_table),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (ipal->palette, ipal_drag_color, ipal);

  gtk_drag_dest_set (ipal->palette,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     color_palette_target_table,
                     G_N_ELEMENTS (color_palette_target_table),
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (ipal->palette, ipal_drop_color, ipal);

  /* some helpful hints */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_end (GTK_BOX (ipal), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  ipal->index_spinbutton =
    gimp_spin_button_new ((GtkObject **) &ipal->index_adjustment,
			  0, 0, 0, 1, 10, 10, 1.0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Color Index:"), 1.0, 0.5,
			     ipal->index_spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (ipal->index_adjustment), "value_changed",
		    G_CALLBACK (index_adjustment_change_cb),
		    ipal);

  ipal->color_entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (ipal->color_entry), 7);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Hex Triplet:"), 1.0, 0.5,
			     ipal->color_entry, 1, TRUE);

  g_signal_connect (G_OBJECT (ipal->color_entry), "activate",
		    G_CALLBACK (hex_entry_change_cb),
		    ipal);

  gimp_colormap_dialog_set_image (ipal, gimage);

  return GTK_WIDGET (ipal);
}

void
gimp_colormap_dialog_selected (GimpColormapDialog *colormap_dialog)
{
  g_return_if_fail (GIMP_IS_COLORMAP_DIALOG (colormap_dialog));

  g_signal_emit (G_OBJECT (colormap_dialog),
		 gimp_colormap_dialog_signals[SELECTED] ,0);
}

void
gimp_colormap_dialog_set_image (GimpColormapDialog *ipal,
				GimpImage          *gimage)
{
  g_return_if_fail (GIMP_IS_COLORMAP_DIALOG (ipal));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  if (ipal->image)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (ipal->image),
					    image_mode_changed_cb,
					    ipal);
      g_signal_handlers_disconnect_by_func (G_OBJECT (ipal->image),
					    image_colormap_changed_cb,
					    ipal);

      if (! gimage || (gimp_image_base_type (gimage) != GIMP_INDEXED))
	{
	  if (ipal->color_notebook)
	    color_notebook_hide (ipal->color_notebook);

	  ipal->index_adjustment->upper = 0;
	  gtk_adjustment_changed (ipal->index_adjustment);

	  if (GTK_WIDGET_MAPPED (GTK_WIDGET (ipal)))
	    ipal_clear (ipal, -1);
	}
    }

  ipal->image         = gimage;
  ipal->col_index     = 0;
  ipal->dnd_col_index = 0;

  if (gimage)
    {
      g_signal_connect (G_OBJECT (gimage), "mode_changed",
			G_CALLBACK (image_mode_changed_cb),
			ipal);
      g_signal_connect (G_OBJECT (gimage), "colormap_changed",
			G_CALLBACK (image_colormap_changed_cb),
			ipal);

      if (gimp_image_base_type (gimage) == GIMP_INDEXED)
	{
	  ipal_draw (ipal);

	  ipal->index_adjustment->upper = ipal->image->num_cols - 1;
	  gtk_adjustment_changed (ipal->index_adjustment);

	  gtk_widget_set_sensitive (ipal->add_item,
				    gimage->num_cols < 256);
	}
    }

  ipal_update_entries (ipal);
}

GimpImage *
gimp_colormap_dialog_get_image (GimpColormapDialog *colormap_dialog)
{
  g_return_val_if_fail (GIMP_IS_COLORMAP_DIALOG (colormap_dialog), NULL);

  return colormap_dialog->image;
}

gint
gimp_colormap_dialog_col_index (GimpColormapDialog *colormap_dialog)
{
  g_return_val_if_fail (GIMP_IS_COLORMAP_DIALOG (colormap_dialog), 0);

  return colormap_dialog->col_index;
}


/*  private functions  */

static void
ipal_create_popup_menu (GimpColormapDialog *ipal)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  ipal->popup_menu = menu = gtk_menu_new ();

  menu_item = gtk_menu_item_new_with_label (_("Add"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  g_signal_connect (G_OBJECT (menu_item), "activate", 
		    G_CALLBACK (ipal_add_callback),
		    ipal);

  ipal->add_item = menu_item;

  menu_item = gtk_menu_item_new_with_label (_("Edit"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);

  g_signal_connect (G_OBJECT (menu_item), "activate", 
		    G_CALLBACK (ipal_edit_callback),
		    ipal);
}

static void
ipal_drag_color (GtkWidget *widget,
		 GimpRGB   *color,
		 gpointer   data)
{
  GimpColormapDialog *ipal;
  GimpImage          *gimage;

  ipal   = (GimpColormapDialog *) data;
  gimage = ipal->image;

  if (gimage && (gimp_image_base_type (gimage) == GIMP_INDEXED))
    {
      guint col;

      col = ipal->dnd_col_index;

      gimp_rgba_set_uchar (color,
			   gimage->cmap[col * 3 + 0],
			   gimage->cmap[col * 3 + 1],
			   gimage->cmap[col * 3 + 2],
			   255);
    }
}

static void
ipal_drop_color (GtkWidget     *widget,
		 const GimpRGB *color,
		 gpointer       data)
{
  GimpColormapDialog *ipal;
  GimpImage          *gimage;

  ipal   = (GimpColormapDialog *) data;
  gimage = ipal->image;

  if (gimage && (gimp_image_base_type (gimage) == GIMP_INDEXED) &&
      gimage->num_cols < 256)
    {
      gimp_rgb_get_uchar (color,
			  &gimage->cmap[ipal->image->num_cols * 3],
			  &gimage->cmap[ipal->image->num_cols * 3 + 1],
			  &gimage->cmap[ipal->image->num_cols * 3 + 2]);

      gimage->num_cols++;
      gimp_image_colormap_changed (gimage, -1);
    }
}

static void
ipal_area_size_allocate (GtkWidget          *widget,
			 GtkAllocation      *alloc,
			 GimpColormapDialog *ipal)
{
  if (ipal->image && (gimp_image_base_type (ipal->image) == GIMP_INDEXED))
    ipal_draw (ipal);
  else
    ipal_clear (ipal, -1);
}

#define MIN_CELL_SIZE 4

static void
ipal_draw (GimpColormapDialog *ipal)
{
  GimpImage *gimage;
  gint       i, j, k, l, b;
  gint       col;
  guchar    *row;
  gint       cellsize, ncol, xn, yn, width, height;
  GtkWidget *palette;

  g_return_if_fail (ipal);
  g_return_if_fail (ipal->image);

  gimage = ipal->image;

  palette = ipal->palette;
  width   = palette->allocation.width;
  height  = palette->allocation.height;
  ncol    = gimage->num_cols;

  if (! ncol)
    {
      ipal_clear (ipal, -1);
      return;
    }

  cellsize = sqrt (width * height / ncol);
  while (cellsize >= MIN_CELL_SIZE
	 && (xn = width / cellsize) * (yn = height / cellsize) < ncol)
    cellsize--;

  if (cellsize < MIN_CELL_SIZE)
    {
      cellsize = MIN_CELL_SIZE;
      xn = yn = ceil (sqrt (ncol));
    }

  yn     = ((ncol + xn - 1) / xn);

  /*
    width  = xn * cellsize;
    height = yn * cellsize;
  */

  ipal->xn       = xn;
  ipal->yn       = yn;
  ipal->cellsize = cellsize;

  row = g_new (guchar, width * 3);
  col = 0;
  for (i = 0; i < yn; i++)
    {
      for (j = 0; j < xn && col < ncol; j++, col++)
	{
	  for (k = 0; k < cellsize; k++)
	    for (b = 0; b < 3; b++)
	      row[(j * cellsize + k) * 3 + b] = gimage->cmap[col * 3 + b];
	}

      for (k = 0; k < cellsize; k++)
	{
	  for (l = j * cellsize; l < width; l++)
	    for (b = 0; b < 3; b++)
	      row[l * 3 + b] = (((((i * cellsize + k) & 0x4) ?
				  (l) :
				  (l + 0x4)) & 0x4) ?
				render_blend_light_check[0] :
				render_blend_dark_check[0]);

	  gtk_preview_draw_row (GTK_PREVIEW (ipal->palette), row, 0,
				i * cellsize + k,
				width);
	}
    }

  ipal_clear (ipal, yn * cellsize);

  ipal_draw_cell (ipal, ipal->col_index);

  g_free (row);
  gtk_widget_queue_draw (palette);
}

static void
ipal_draw_cell (GimpColormapDialog *ipal,
		gint                col)
{
  guchar *row;
  gint    cellsize, x, y, k;
  
  g_assert (ipal);
  g_assert (ipal->image);
  g_assert (col < ipal->image->num_cols);

  cellsize = ipal->cellsize;
  row = g_new (guchar, cellsize * 3);
  x = (col % ipal->xn) * cellsize;
  y = (col / ipal->xn) * cellsize;

  if (col == ipal->col_index)
    {
      for(k = 0; k < cellsize; k++)
	row[k*3] = row[k*3+1] = row[k*3+2] = (k & 1) * 255;
      gtk_preview_draw_row (GTK_PREVIEW (ipal->palette), row, x, y, cellsize);

      if (!(cellsize & 1))
	for (k = 0; k < cellsize; k++)
	  row[k*3] = row[k*3+1] = row[k*3+2] = ((x+y+1) & 1) * 255;
      gtk_preview_draw_row (GTK_PREVIEW (ipal->palette), row,
			    x, y+cellsize-1, cellsize);

      row[0]=row[1]=row[2]=255;
      row[cellsize*3-3] = row[cellsize*3-2] = row[cellsize*3-1]
	= 255 * (cellsize & 1);
      for (k = 1; k < cellsize - 1; k++)
	{
	  row[k*3]   = ipal->image->cmap[col * 3];
	  row[k*3+1] = ipal->image->cmap[col * 3 + 1];
	  row[k*3+2] = ipal->image->cmap[col * 3 + 2];
	}
      for (k = 1; k < cellsize - 1; k+=2)
	gtk_preview_draw_row (GTK_PREVIEW (ipal->palette), row,
			      x, y+k, cellsize);

      row[0] = row[1] = row[2] = 0;
      row[cellsize*3-3] = row[cellsize*3-2] = row[cellsize*3-1]
	= 255 * ((cellsize+1) & 1);
      for (k = 2; k < cellsize - 1; k += 2)
	gtk_preview_draw_row (GTK_PREVIEW (ipal->palette), row,
			      x, y+k, cellsize);
    }
  else
    {
      for (k = 0; k < cellsize; k++)
	{
	  row[k*3]   = ipal->image->cmap[col * 3];
	  row[k*3+1] = ipal->image->cmap[col * 3 + 1];
	  row[k*3+2] = ipal->image->cmap[col * 3 + 2];
	}
      for (k = 0; k < cellsize; k++)
	gtk_preview_draw_row (GTK_PREVIEW (ipal->palette), row,
			      x, y+k, cellsize);
    }

  gtk_widget_queue_draw_area (ipal->palette,
                              x, y,
                              cellsize, cellsize);
}
    
static void
ipal_clear (GimpColormapDialog *ipal,
	    gint                start_row)
{
  gint       i, j;
  gint       offset;
  gint       width, height;
  guchar    *row = NULL;
  GtkWidget *palette;

  g_return_if_fail (ipal);

  palette = ipal->palette;

  /* Watch out for negative values (at least on Win32) */
  width  = (int) (gint16) palette->allocation.width;
  height = (int) (gint16) palette->allocation.height;

  if (start_row < 0)
    start_row = 0;

  if (start_row >= height)
    return;

  if (width > 0)
    row = g_new (guchar, width * 3);

  if (start_row & 0x3)
    {
      offset = (start_row & 0x4) ? 0x4 : 0x0;

      for (j = 0; j < width; j++)
	{
	  row[j * 3 + 0] = row[j * 3 + 1] = row[j * 3 + 2] =
	    ((j + offset) & 0x4) ?
	    render_blend_dark_check[0] : render_blend_light_check[0];
	}

      for (j = 0; j < (4 - (start_row & 0x3)) && start_row + j < height; j++)
	gtk_preview_draw_row (GTK_PREVIEW (ipal->palette), row,
			      0, start_row + j, width);

      start_row += (4 - (start_row & 0x3));
    }

  for (i = start_row; i < height; i += 4)
    {
      offset = (i & 0x4) ? 0x4 : 0x0;

      for (j = 0; j < width; j++)
	{
	  row[j * 3 + 0] = row[j * 3 + 1] = row[j * 3 + 2] =
	    ((j + offset) & 0x4) ?
	    render_blend_dark_check[0] : render_blend_light_check[0];
	}

      for (j = 0; j < 4 && i + j < height; j++)
	gtk_preview_draw_row (GTK_PREVIEW (ipal->palette), row,
			      0, i + j, width);
    }

  if (width > 0)
    g_free (row);

  gtk_widget_queue_draw (palette);
}

static void
ipal_update_entries (GimpColormapDialog *ipal)
{
  if (! ipal->image || (gimp_image_base_type (ipal->image) != GIMP_INDEXED))
    {
      gtk_widget_set_sensitive (ipal->index_spinbutton, FALSE);
      gtk_widget_set_sensitive (ipal->color_entry, FALSE);

      gtk_adjustment_set_value (ipal->index_adjustment, 0);
      gtk_entry_set_text (GTK_ENTRY (ipal->color_entry), "");
    }
  else
    {
      gchar  *string;
      guchar *col;

      gtk_adjustment_set_value (ipal->index_adjustment, ipal->col_index);

      col = &ipal->image->cmap[ipal->col_index * 3];

      string = g_strdup_printf ("#%02x%02x%02x", col[0], col[1], col[2]);
      gtk_entry_set_text (GTK_ENTRY (ipal->color_entry), string);
      g_free (string);

      gtk_widget_set_sensitive (ipal->index_spinbutton, TRUE);
      gtk_widget_set_sensitive (ipal->color_entry, TRUE);
    }
}

static void
ipal_set_index (GimpColormapDialog *ipal,
		gint                i)
{
  if (i != ipal->col_index)
    {
      gint old = ipal->col_index;

      ipal->col_index     = i;
      ipal->dnd_col_index = i;

      ipal_draw_cell (ipal, old);
      ipal_draw_cell (ipal, i);

      ipal_update_entries (ipal);
    }
}

static void
index_adjustment_change_cb (GtkAdjustment      *adjustment,
			    GimpColormapDialog *ipal)
{
  g_return_if_fail (ipal);

  if (!ipal->image)
    return;

  ipal_set_index (ipal, (gint) (adjustment->value + 0.5));

  ipal_update_entries (ipal);
}

static void
hex_entry_change_cb (GtkEntry           *entry,
		     GimpColormapDialog *ipal)
{
  const gchar *s;
  gulong       i;

  g_return_if_fail (ipal);
  g_return_if_fail (ipal->image);

  s = gtk_entry_get_text (entry);

  if (sscanf (s, "#%lx", &i))
    {
      guchar *c = &ipal->image->cmap[3 * ipal->col_index];
      c[0] = (i & 0xFF0000) >> 16;
      c[1] = (i & 0x00FF00) >> 8;
      c[2] = (i & 0x0000FF);
      gimp_image_colormap_changed (ipal->image, ipal->col_index);
    }
  
  ipal_update_entries (ipal);
}

static void
image_mode_changed_cb (GimpImage          *gimage,
		       GimpColormapDialog *ipal)
{
  image_colormap_changed_cb (gimage, -1, ipal);
}

static void
image_colormap_changed_cb (GimpImage          *gimage,
			   gint                ncol,
			   GimpColormapDialog *ipal)
{
  if (gimp_image_base_type (gimage) == GIMP_INDEXED)
    {
      if (ncol < 0)
	{
	  ipal_draw (ipal);
	}
      else
	{
	  ipal_draw_cell (ipal, ncol);
	}

      gtk_widget_set_sensitive (ipal->add_item, (gimage->num_cols < 256));

      if ((ipal->index_adjustment->upper + 1) < gimage->num_cols)
	{
	  ipal->index_adjustment->upper = gimage->num_cols - 1;
	  gtk_adjustment_changed (ipal->index_adjustment);
	}
    }
  else
    {
      ipal_clear (ipal, -1);
    }

  if (ncol == ipal->col_index || ncol == -1)
    ipal_update_entries (ipal);
}

static void
ipal_add_callback (GtkWidget *widget,
		   gpointer   data)
{
  GimpColormapDialog *ipal = data;

  g_return_if_fail (ipal);
  g_return_if_fail (ipal->image->num_cols < 256);

  memcpy (&ipal->image->cmap[ipal->image->num_cols * 3],
	  &ipal->image->cmap[ipal->col_index * 3],
	  3);
  ipal->image->num_cols++;

  gimp_image_colormap_changed (ipal->image, -1);
}

static void
ipal_edit_callback (GtkWidget *widget,
		    gpointer   data)
{
  GimpColormapDialog *ipal = data;
  GimpRGB             color;

  g_return_if_fail (ipal);

  gimp_rgba_set_uchar (&color,
		       ipal->image->cmap[ipal->col_index * 3],
		       ipal->image->cmap[ipal->col_index * 3 + 1],
		       ipal->image->cmap[ipal->col_index * 3 + 2],
		       255);

  if (! ipal->color_notebook)
    {
      ipal->color_notebook
	= color_notebook_new (_("Edit Indexed Color"),
			      (const GimpRGB *) &color,
			      ipal_select_callback, ipal, FALSE, FALSE);
    }
  else
    {
      color_notebook_show (ipal->color_notebook);
      color_notebook_set_color (ipal->color_notebook, &color);
    }
}

static void
ipal_select_callback (ColorNotebook      *color_notebook,
		      const GimpRGB      *color,
		      ColorNotebookState  state,
		      gpointer            data)
{
  GimpImage          *gimage;
  GimpColormapDialog *ipal = data;
  
  g_return_if_fail (ipal);
  g_return_if_fail (ipal->image);
  g_return_if_fail (ipal->color_notebook);

  gimage = ipal->image;

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      break;
    case COLOR_NOTEBOOK_OK:
      gimp_rgb_get_uchar (color,
			  &gimage->cmap[ipal->col_index * 3 + 0],
			  &gimage->cmap[ipal->col_index * 3 + 1],
			  &gimage->cmap[ipal->col_index * 3 + 2]);

      gimp_image_colormap_changed (gimage, ipal->col_index);
      /* Fall through */
    case COLOR_NOTEBOOK_CANCEL:
      color_notebook_hide (ipal->color_notebook);
    }
}

static gint
ipal_area_button_press (GtkWidget          *widget,
			GdkEventButton     *bevent,
			GimpColormapDialog *ipal)
{
  GimpImage      *gimage;
  guchar          r, g, b;
  guint           col;

  gimage = ipal->image;

  if (! (gimage && (gimp_image_base_type (gimage) == GIMP_INDEXED)))
    return FALSE;

  if (bevent->button < 1 || bevent->button > 3)
    return FALSE;

  if (!(bevent->y < ipal->cellsize * ipal->yn
	&& bevent->x < ipal->cellsize * ipal->xn))
    return FALSE;

  col = ipal->xn * ((int)bevent->y / ipal->cellsize)
    + ((int)bevent->x / ipal->cellsize);

  if (col >= ipal->image->num_cols)
    return FALSE;

  r = gimage->cmap[col * 3 + 0];
  g = gimage->cmap[col * 3 + 1];
  b = gimage->cmap[col * 3 + 2];

  switch (bevent->button)
    {
    case 1:
      ipal_set_index (ipal, col);
      gimp_colormap_dialog_selected (ipal);
      break;

    case 2:
      ipal->dnd_col_index = col;
      break;

    case 3:
      ipal_set_index (ipal, col);
      gtk_menu_popup (GTK_MENU (ipal->popup_menu), NULL, NULL, 
		      NULL, NULL, 3,
		      bevent->time);
      break;

    default:
      break;
    }

  return FALSE;
}
