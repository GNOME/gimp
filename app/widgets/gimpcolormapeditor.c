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

#include "widgets-types.h"

#ifdef __GNUC__
#warning FIXME #include "display/display-types.h"
#endif
#include "display/display-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpmarshal.h"

#include "display/gimpdisplayshell-render.h"

#include "gimpcolormapeditor.h"
#include "gimpdnd.h"
#include "gimpitemfactory.h"

#include "gui/color-notebook.h"

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


static void   gimp_colormap_editor_class_init (GimpColormapEditorClass *klass);
static void   gimp_colormap_editor_init       (GimpColormapEditor      *colormap_editor);

static void   gimp_colormap_editor_destroy         (GtkObject          *object);
static void   gimp_colormap_editor_unmap           (GtkWidget          *widget);

static void   gimp_colormap_editor_draw            (GimpColormapEditor *editor);
static void   gimp_colormap_editor_draw_cell       (GimpColormapEditor *editor,
                                                    gint                col);
static void   gimp_colormap_editor_clear           (GimpColormapEditor *editor,
                                                    gint                start_row);
static void   gimp_colormap_editor_update_entries  (GimpColormapEditor *editor);
static void   gimp_colormap_editor_set_index       (GimpColormapEditor *editor,
                                                    gint                i);

static void   gimp_colormap_preview_size_allocate  (GtkWidget          *widget,
                                                    GtkAllocation      *allocation,
                                                    GimpColormapEditor *editor);
static gboolean
              gimp_colormap_preview_button_press   (GtkWidget          *widget,
                                                    GdkEventButton     *bevent,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_preview_drag_color     (GtkWidget          *widget,
                                                    GimpRGB            *color,
                                                    gpointer            data);
static void   gimp_colormap_preview_drop_color     (GtkWidget          *widget,
                                                    const GimpRGB      *color,
                                                    gpointer            data);

static void   gimp_colormap_adjustment_changed     (GtkAdjustment      *adjustment,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_hex_entry_activate     (GtkEntry           *entry,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_hex_entry_focus_out    (GtkEntry           *entry,
                                                    GdkEvent           *event,
                                                    GimpColormapEditor *editor);

static void   gimp_colormap_image_mode_changed     (GimpImage          *gimage,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_image_colormap_changed (GimpImage          *gimage,
                                                    gint                ncol,
                                                    GimpColormapEditor *editor);


static guint editor_signals[LAST_SIGNAL] = { 0 };

static GimpEditorClass *parent_class = NULL;

static GtkTargetEntry color_palette_target_table[] =
{
  GIMP_TARGET_COLOR
};


GType
gimp_colormap_editor_get_type (void)
{
  static GType gcd_type = 0;

  if (! gcd_type)
    {
      static const GTypeInfo gcd_info =
      {
        sizeof (GimpColormapEditorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_colormap_editor_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColormapEditor),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_colormap_editor_init,
      };

      gcd_type = g_type_register_static (GIMP_TYPE_EDITOR,
                                         "GimpColormapEditor",
                                         &gcd_info, 0);
    }

  return gcd_type;
}

static void
gimp_colormap_editor_class_init (GimpColormapEditorClass* klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_signals[SELECTED] =
    g_signal_new ("selected",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpColormapEditorClass, selected),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy = gimp_colormap_editor_destroy;

  widget_class->unmap   = gimp_colormap_editor_unmap;

  klass->selected       = NULL;
}

static void
gimp_colormap_editor_init (GimpColormapEditor *colormap_editor)
{
  colormap_editor->gimage           = NULL;
  colormap_editor->col_index        = 0;
  colormap_editor->dnd_col_index    = 0;
  colormap_editor->palette          = NULL;
  colormap_editor->xn               = 0;
  colormap_editor->yn               = 0;
  colormap_editor->cellsize         = 0;
  colormap_editor->index_adjustment = NULL;
  colormap_editor->index_spinbutton = NULL;
  colormap_editor->color_entry      = NULL;
  colormap_editor->color_notebook   = NULL;
}

static void
gimp_colormap_editor_destroy (GtkObject *object)
{
  GimpColormapEditor *editor;

  editor = GIMP_COLORMAP_EDITOR (object);

  if (editor->color_notebook)
    {
      color_notebook_free (editor->color_notebook);
      editor->color_notebook = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_colormap_editor_unmap (GtkWidget *widget)
{
  GimpColormapEditor *editor;

  editor = GIMP_COLORMAP_EDITOR (widget);

  if (editor->color_notebook)
    {
      color_notebook_hide (editor->color_notebook);
    }

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}


/*  public functions  */

GtkWidget *
gimp_colormap_editor_new (GimpImage *gimage)
{
  GimpColormapEditor *editor;
  GtkWidget          *frame;
  GtkWidget          *table;

  g_return_val_if_fail (! gimage || GIMP_IS_IMAGE (gimage), NULL);

  editor = g_object_new (GIMP_TYPE_COLORMAP_EDITOR, NULL);

  /*  The palette frame  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor->palette = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_widget_set_size_request (editor->palette, -1, 60);
  gtk_preview_set_expand (GTK_PREVIEW (editor->palette), TRUE);
  gtk_widget_add_events (editor->palette, GDK_BUTTON_PRESS_MASK);
  gtk_container_add (GTK_CONTAINER (frame), editor->palette);
  gtk_widget_show (editor->palette);

  g_signal_connect_after (G_OBJECT (editor->palette), "size_allocate",
			  G_CALLBACK (gimp_colormap_preview_size_allocate),
			  editor);

  g_signal_connect (G_OBJECT (editor->palette), "button_press_event",
		    G_CALLBACK (gimp_colormap_preview_button_press),
		    editor);

  /*  dnd stuff  */
  gtk_drag_source_set (editor->palette,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_palette_target_table,
                       G_N_ELEMENTS (color_palette_target_table),
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (editor->palette, gimp_colormap_preview_drag_color,
                             editor);

  gtk_drag_dest_set (editor->palette,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     color_palette_target_table,
                     G_N_ELEMENTS (color_palette_target_table),
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (editor->palette, gimp_colormap_preview_drop_color,
                           editor);

  /* some helpful hints */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_end (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  editor->index_spinbutton =
    gimp_spin_button_new ((GtkObject **) &editor->index_adjustment,
			  0, 0, 0, 1, 10, 10, 1.0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Color Index:"), 1.0, 0.5,
			     editor->index_spinbutton, 1, TRUE);

  g_signal_connect (G_OBJECT (editor->index_adjustment), "value_changed",
		    G_CALLBACK (gimp_colormap_adjustment_changed),
		    editor);

  editor->color_entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (editor->color_entry), 7);
  gtk_entry_set_max_length (GTK_ENTRY (editor->color_entry), 7);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Hex Triplet:"), 1.0, 0.5,
			     editor->color_entry, 1, TRUE);

  g_signal_connect (G_OBJECT (editor->color_entry), "activate",
		    G_CALLBACK (gimp_colormap_hex_entry_activate),
		    editor);
  g_signal_connect (G_OBJECT (editor->color_entry), "focus_out_event",
		    G_CALLBACK (gimp_colormap_hex_entry_focus_out),
		    editor);

  gimp_colormap_editor_set_image (editor, gimage);

  return GTK_WIDGET (editor);
}

void
gimp_colormap_editor_selected (GimpColormapEditor *editor)
{
  g_return_if_fail (GIMP_IS_COLORMAP_EDITOR (editor));

  g_signal_emit (G_OBJECT (editor), editor_signals[SELECTED], 0);
}

void
gimp_colormap_editor_set_image (GimpColormapEditor *editor,
				GimpImage          *gimage)
{
  g_return_if_fail (GIMP_IS_COLORMAP_EDITOR (editor));
  g_return_if_fail (! gimage || GIMP_IS_IMAGE (gimage));

  if (editor->gimage)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (editor->gimage),
					    gimp_colormap_image_mode_changed,
					    editor);
      g_signal_handlers_disconnect_by_func (G_OBJECT (editor->gimage),
					    gimp_colormap_image_colormap_changed,
					    editor);

      if (editor->color_notebook)
        color_notebook_hide (editor->color_notebook);

      if (! gimage || (gimp_image_base_type (gimage) != GIMP_INDEXED))
	{
	  editor->index_adjustment->upper = 0;
	  gtk_adjustment_changed (editor->index_adjustment);

	  if (GTK_WIDGET_MAPPED (GTK_WIDGET (editor)))
	    gimp_colormap_editor_clear (editor, -1);
	}
    }

  editor->gimage        = gimage;
  editor->col_index     = 0;
  editor->dnd_col_index = 0;

  if (gimage)
    {
      g_signal_connect (G_OBJECT (gimage), "mode_changed",
			G_CALLBACK (gimp_colormap_image_mode_changed),
			editor);
      g_signal_connect (G_OBJECT (gimage), "colormap_changed",
			G_CALLBACK (gimp_colormap_image_colormap_changed),
			editor);

      if (gimp_image_base_type (gimage) == GIMP_INDEXED)
	{
	  gimp_colormap_editor_draw (editor);

	  editor->index_adjustment->upper = editor->gimage->num_cols - 1;
	  gtk_adjustment_changed (editor->index_adjustment);
	}
    }

  gimp_colormap_editor_update_entries (editor);
}

GimpImage *
gimp_colormap_editor_get_image (GimpColormapEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), NULL);

  return editor->gimage;
}

gint
gimp_colormap_editor_col_index (GimpColormapEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), 0);

  return editor->col_index;
}


/*  private functions  */

#define MIN_CELL_SIZE 4

static void
gimp_colormap_editor_draw (GimpColormapEditor *editor)
{
  GimpImage *gimage;
  gint       i, j, k, l, b;
  gint       col;
  guchar    *row;
  gint       cellsize, ncol, xn, yn, width, height;
  GtkWidget *palette;

  gimage = editor->gimage;

  palette = editor->palette;
  width   = palette->allocation.width;
  height  = palette->allocation.height;
  ncol    = gimage->num_cols;

  if (! ncol)
    {
      gimp_colormap_editor_clear (editor, -1);
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

  /* We used to render just multiples of "cellsize" here, but the
   *  colormap as dockable looks better if it always fills the
   *  available allocation->width (which should always be larger than
   *  "xn * cellsize"). Defensively, we use MAX(width,xn*cellsize)
   *  below   --Mitch
   *

   width  = xn * cellsize;
   height = yn * cellsize; */

  editor->xn       = xn;
  editor->yn       = yn;
  editor->cellsize = cellsize;

  row = g_new (guchar, MAX (width, xn * cellsize) * 3);
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

	  gtk_preview_draw_row (GTK_PREVIEW (editor->palette), row, 0,
				i * cellsize + k,
				width);
	}
    }

  gimp_colormap_editor_clear (editor, yn * cellsize);

  gimp_colormap_editor_draw_cell (editor, editor->col_index);

  g_free (row);
  gtk_widget_queue_draw (palette);
}

static void
gimp_colormap_editor_draw_cell (GimpColormapEditor *editor,
                                gint                col)
{
  guchar *row;
  gint    cellsize, x, y, k;
  
  g_assert (editor);
  g_assert (editor->gimage);
  g_assert (col < editor->gimage->num_cols);

  cellsize = editor->cellsize;
  row = g_new (guchar, cellsize * 3);
  x = (col % editor->xn) * cellsize;
  y = (col / editor->xn) * cellsize;

  if (col == editor->col_index)
    {
      for(k = 0; k < cellsize; k++)
	row[k*3] = row[k*3+1] = row[k*3+2] = (k & 1) * 255;
      gtk_preview_draw_row (GTK_PREVIEW (editor->palette), row, x, y, cellsize);

      if (!(cellsize & 1))
	for (k = 0; k < cellsize; k++)
	  row[k*3] = row[k*3+1] = row[k*3+2] = ((x+y+1) & 1) * 255;
      gtk_preview_draw_row (GTK_PREVIEW (editor->palette), row,
			    x, y+cellsize-1, cellsize);

      row[0]=row[1]=row[2]=255;
      row[cellsize*3-3] = row[cellsize*3-2] = row[cellsize*3-1]
	= 255 * (cellsize & 1);
      for (k = 1; k < cellsize - 1; k++)
	{
	  row[k*3]   = editor->gimage->cmap[col * 3];
	  row[k*3+1] = editor->gimage->cmap[col * 3 + 1];
	  row[k*3+2] = editor->gimage->cmap[col * 3 + 2];
	}
      for (k = 1; k < cellsize - 1; k+=2)
	gtk_preview_draw_row (GTK_PREVIEW (editor->palette), row,
			      x, y+k, cellsize);

      row[0] = row[1] = row[2] = 0;
      row[cellsize*3-3] = row[cellsize*3-2] = row[cellsize*3-1]
	= 255 * ((cellsize+1) & 1);
      for (k = 2; k < cellsize - 1; k += 2)
	gtk_preview_draw_row (GTK_PREVIEW (editor->palette), row,
			      x, y+k, cellsize);
    }
  else
    {
      for (k = 0; k < cellsize; k++)
	{
	  row[k*3]   = editor->gimage->cmap[col * 3];
	  row[k*3+1] = editor->gimage->cmap[col * 3 + 1];
	  row[k*3+2] = editor->gimage->cmap[col * 3 + 2];
	}
      for (k = 0; k < cellsize; k++)
	gtk_preview_draw_row (GTK_PREVIEW (editor->palette), row,
			      x, y+k, cellsize);
    }

  gtk_widget_queue_draw_area (editor->palette,
                              x, y,
                              cellsize, cellsize);
}

static void
gimp_colormap_editor_clear (GimpColormapEditor *editor,
                            gint                start_row)
{
  gint       i, j;
  gint       offset;
  gint       width, height;
  guchar    *row = NULL;
  GtkWidget *palette;

  palette = editor->palette;

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
	gtk_preview_draw_row (GTK_PREVIEW (editor->palette), row,
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
	gtk_preview_draw_row (GTK_PREVIEW (editor->palette), row,
			      0, i + j, width);
    }

  if (width > 0)
    g_free (row);

  gtk_widget_queue_draw (palette);
}

static void
gimp_colormap_editor_update_entries (GimpColormapEditor *editor)
{
  if (! editor->gimage ||
      (gimp_image_base_type (editor->gimage) != GIMP_INDEXED))
    {
      gtk_widget_set_sensitive (editor->index_spinbutton, FALSE);
      gtk_widget_set_sensitive (editor->color_entry, FALSE);

      gtk_adjustment_set_value (editor->index_adjustment, 0);
      gtk_entry_set_text (GTK_ENTRY (editor->color_entry), "");
    }
  else
    {
      gchar  *string;
      guchar *col;

      gtk_adjustment_set_value (editor->index_adjustment, editor->col_index);

      col = &editor->gimage->cmap[editor->col_index * 3];

      string = g_strdup_printf ("#%02x%02x%02x", col[0], col[1], col[2]);
      gtk_entry_set_text (GTK_ENTRY (editor->color_entry), string);
      g_free (string);

      gtk_widget_set_sensitive (editor->index_spinbutton, TRUE);
      gtk_widget_set_sensitive (editor->color_entry, TRUE);
    }
}

static void
gimp_colormap_editor_set_index (GimpColormapEditor *editor,
                                gint                i)
{
  if (i != editor->col_index)
    {
      gint old = editor->col_index;

      editor->col_index     = i;
      editor->dnd_col_index = i;

      gimp_colormap_editor_draw_cell (editor, old);
      gimp_colormap_editor_draw_cell (editor, i);

      gimp_colormap_editor_update_entries (editor);
    }
}

static void
gimp_colormap_preview_size_allocate (GtkWidget          *widget,
                                     GtkAllocation      *alloc,
                                     GimpColormapEditor *editor)
{
  if (editor->gimage && (gimp_image_base_type (editor->gimage) == GIMP_INDEXED))
    gimp_colormap_editor_draw (editor);
  else
    gimp_colormap_editor_clear (editor, -1);
}


static gboolean
gimp_colormap_preview_button_press (GtkWidget          *widget,
                                    GdkEventButton     *bevent,
                                    GimpColormapEditor *editor)
{
  GimpImage *gimage;
  guchar     r, g, b;
  guint      col;

  gimage = editor->gimage;

  if (! (gimage && (gimp_image_base_type (gimage) == GIMP_INDEXED)))
    return TRUE;

  if (!(bevent->y < editor->cellsize * editor->yn
	&& bevent->x < editor->cellsize * editor->xn))
    return TRUE;

  col = (editor->xn * ((gint) bevent->y / editor->cellsize) +
         ((gint) bevent->x / editor->cellsize));

  if (col >= editor->gimage->num_cols)
    return TRUE;

  r = gimage->cmap[col * 3 + 0];
  g = gimage->cmap[col * 3 + 1];
  b = gimage->cmap[col * 3 + 2];

  switch (bevent->button)
    {
    case 1:
      gimp_colormap_editor_set_index (editor, col);
      gimp_colormap_editor_selected (editor);
      break;

    case 2:
      editor->dnd_col_index = col;
      break;

    case 3:
      {
        GimpItemFactory *factory;

        factory = gimp_item_factory_from_path ("<ColormapEditor>");

        gimp_colormap_editor_set_index (editor, col);
        gimp_item_factory_popup_with_data (factory, editor, NULL);
      }
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_colormap_preview_drag_color (GtkWidget *widget,
                                  GimpRGB   *color,
                                  gpointer   data)
{
  GimpColormapEditor *editor;
  GimpImage          *gimage;

  editor = GIMP_COLORMAP_EDITOR (data);

  gimage = editor->gimage;

  if (gimage && (gimp_image_base_type (gimage) == GIMP_INDEXED))
    {
      guint col;

      col = editor->dnd_col_index;

      gimp_rgba_set_uchar (color,
			   gimage->cmap[col * 3 + 0],
			   gimage->cmap[col * 3 + 1],
			   gimage->cmap[col * 3 + 2],
			   OPAQUE_OPACITY);
    }
}

static void
gimp_colormap_preview_drop_color (GtkWidget     *widget,
                                  const GimpRGB *color,
                                  gpointer       data)
{
  GimpColormapEditor *editor;
  GimpImage          *gimage;

  editor = GIMP_COLORMAP_EDITOR (data);

  gimage = editor->gimage;

  if ((gimage && gimp_image_base_type (gimage) == GIMP_INDEXED))
    {
      if (gimage->num_cols < 256)
        {
          gimp_rgb_get_uchar (color,
                              &gimage->cmap[editor->gimage->num_cols * 3],
                              &gimage->cmap[editor->gimage->num_cols * 3 + 1],
                              &gimage->cmap[editor->gimage->num_cols * 3 + 2]);

          gimage->num_cols++;
          gimp_image_colormap_changed (gimage, -1);
        }
    }
}

static void
gimp_colormap_adjustment_changed (GtkAdjustment      *adjustment,
                                  GimpColormapEditor *editor)
{
  if (editor->gimage)
    {
      gimp_colormap_editor_set_index (editor, (gint) (adjustment->value + 0.5));

      gimp_colormap_editor_update_entries (editor);
    }
}

static void
gimp_colormap_hex_entry_activate (GtkEntry           *entry,
                                  GimpColormapEditor *editor)
{
  if (editor->gimage)
    {
      const gchar *s;
      gulong       i;

      s = gtk_entry_get_text (entry);

      if (sscanf (s, "#%lx", &i))
        {
          guchar *c = &editor->gimage->cmap[3 * editor->col_index];
          c[0] = (i & 0xFF0000) >> 16;
          c[1] = (i & 0x00FF00) >> 8;
          c[2] = (i & 0x0000FF);
          gimp_image_colormap_changed (editor->gimage, editor->col_index);
        }

      gimp_colormap_editor_update_entries (editor);
    }
}

static void
gimp_colormap_hex_entry_focus_out (GtkEntry           *entry,
                                   GdkEvent           *event,
                                   GimpColormapEditor *editor)
{
  gimp_colormap_hex_entry_activate (entry, editor);
}

static void
gimp_colormap_image_mode_changed (GimpImage          *gimage,
                                  GimpColormapEditor *editor)
{
  gimp_colormap_image_colormap_changed (gimage, -1, editor);
}

static void
gimp_colormap_image_colormap_changed (GimpImage          *gimage,
                                      gint                ncol,
                                      GimpColormapEditor *editor)
{
  if (gimp_image_base_type (gimage) == GIMP_INDEXED)
    {
      if (ncol < 0)
	{
	  gimp_colormap_editor_draw (editor);
	}
      else
	{
	  gimp_colormap_editor_draw_cell (editor, ncol);
	}

      if ((editor->index_adjustment->upper + 1) < gimage->num_cols)
	{
	  editor->index_adjustment->upper = gimage->num_cols - 1;
	  gtk_adjustment_changed (editor->index_adjustment);
	}
    }
  else
    {
      gimp_colormap_editor_clear (editor, -1);
    }

  if (ncol == editor->col_index || ncol == -1)
    gimp_colormap_editor_update_entries (editor);
}
