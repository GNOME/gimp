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

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpmarshal.h"

#include "display/gimpdisplayshell-render.h"

#include "gimpcolormapeditor.h"
#include "gimpdnd.h"
#include "gimpmenufactory.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


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


#define HAVE_COLORMAP(gimage) \
        (gimage != NULL && \
         gimp_image_base_type (gimage) == GIMP_INDEXED && \
         gimp_image_get_colormap (gimage) != NULL)


static void   gimp_colormap_editor_class_init (GimpColormapEditorClass *klass);
static void   gimp_colormap_editor_init       (GimpColormapEditor      *colormap_editor);

static GObject * gimp_colormap_editor_constructor  (GType               type,
                                                    guint               n_params,
                                                    GObjectConstructParam *params);

static void   gimp_colormap_editor_destroy         (GtkObject          *object);
static void   gimp_colormap_editor_unmap           (GtkWidget          *widget);

static void   gimp_colormap_editor_set_image       (GimpImageEditor    *editor,
                                                    GimpImage          *gimage);

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
static gboolean gimp_colormap_hex_entry_focus_out  (GtkEntry           *entry,
                                                    GdkEvent           *event,
                                                    GimpColormapEditor *editor);

static void   gimp_colormap_image_mode_changed     (GimpImage          *gimage,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_image_colormap_changed (GimpImage          *gimage,
                                                    gint                ncol,
                                                    GimpColormapEditor *editor);


static guint editor_signals[LAST_SIGNAL] = { 0 };

static GimpImageEditorClass *parent_class = NULL;


GType
gimp_colormap_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
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

      type = g_type_register_static (GIMP_TYPE_IMAGE_EDITOR,
                                     "GimpColormapEditor",
                                     &info, 0);
    }

  return type;
}

static void
gimp_colormap_editor_class_init (GimpColormapEditorClass* klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GtkObjectClass       *gtk_object_class   = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_signals[SELECTED] =
    g_signal_new ("selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColormapEditorClass, selected),
                  NULL, NULL,
                  gimp_marshal_VOID__FLAGS,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_MODIFIER_TYPE);

  object_class->constructor     = gimp_colormap_editor_constructor;

  gtk_object_class->destroy     = gimp_colormap_editor_destroy;

  widget_class->unmap           = gimp_colormap_editor_unmap;

  image_editor_class->set_image = gimp_colormap_editor_set_image;

  klass->selected               = NULL;
}

static void
gimp_colormap_editor_init (GimpColormapEditor *editor)
{
  GtkWidget *frame;
  GtkWidget *table;

  editor->col_index     = 0;
  editor->dnd_col_index = 0;
  editor->xn            = 0;
  editor->yn            = 0;
  editor->cellsize      = 0;

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

  g_signal_connect_after (editor->palette, "size_allocate",
                          G_CALLBACK (gimp_colormap_preview_size_allocate),
                          editor);

  g_signal_connect (editor->palette, "button_press_event",
                    G_CALLBACK (gimp_colormap_preview_button_press),
                    editor);

  gimp_dnd_color_source_add (editor->palette, gimp_colormap_preview_drag_color,
                             editor);
  gimp_dnd_color_dest_add (editor->palette, gimp_colormap_preview_drop_color,
                           editor);

  /*  Some helpful hints  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_end (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  editor->index_spinbutton =
    gimp_spin_button_new ((GtkObject **) &editor->index_adjustment,
                          0, 0, 0, 1, 10, 10, 1.0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Color index:"), 0.0, 0.5,
                             editor->index_spinbutton, 1, TRUE);

  g_signal_connect (editor->index_adjustment, "value_changed",
                    G_CALLBACK (gimp_colormap_adjustment_changed),
                    editor);

  editor->color_entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (editor->color_entry), 8);
  gtk_entry_set_max_length (GTK_ENTRY (editor->color_entry), 6);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("HTML notation:"), 0.0, 0.5,
                             editor->color_entry, 1, TRUE);

  g_signal_connect (editor->color_entry, "activate",
                    G_CALLBACK (gimp_colormap_hex_entry_activate),
                    editor);
  g_signal_connect (editor->color_entry, "focus_out_event",
                    G_CALLBACK (gimp_colormap_hex_entry_focus_out),
                    editor);
}

static GObject *
gimp_colormap_editor_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GObject            *object;
  GimpColormapEditor *editor;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_COLORMAP_EDITOR (object);

  editor->edit_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap-editor",
                                   "colormap-editor-edit-color",
                                   NULL);

  editor->add_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap-editor",
                                   "colormap-editor-add-color-from-fg",
                                   "colormap-editor-add-color-from-bg",
                                   GDK_CONTROL_MASK,
                                   NULL);

  return object;
}

static void
gimp_colormap_editor_destroy (GtkObject *object)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (object);

  if (editor->color_dialog)
    {
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_colormap_editor_unmap (GtkWidget *widget)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (widget);

  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_colormap_editor_set_image (GimpImageEditor *image_editor,
                                GimpImage       *gimage)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (image_editor);

  if (image_editor->gimage)
    {
      g_signal_handlers_disconnect_by_func (image_editor->gimage,
                                            gimp_colormap_image_mode_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->gimage,
                                            gimp_colormap_image_colormap_changed,
                                            editor);

      if (editor->color_dialog)
        gtk_widget_hide (editor->color_dialog);

      if (! HAVE_COLORMAP (gimage))
        {
          editor->index_adjustment->upper = 0;
          gtk_adjustment_changed (editor->index_adjustment);

          if (GTK_WIDGET_MAPPED (GTK_WIDGET (editor)))
            gimp_colormap_editor_clear (editor, -1);
        }
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, gimage);

  editor->col_index     = 0;
  editor->dnd_col_index = 0;

  if (gimage)
    {
      g_signal_connect (gimage, "mode_changed",
                        G_CALLBACK (gimp_colormap_image_mode_changed),
                        editor);
      g_signal_connect (gimage, "colormap_changed",
                        G_CALLBACK (gimp_colormap_image_colormap_changed),
                        editor);

      if (HAVE_COLORMAP (gimage))
        {
          gimp_colormap_editor_draw (editor);

          editor->index_adjustment->upper =
            gimp_image_get_colormap_size (gimage) - 1;

          gtk_adjustment_changed (editor->index_adjustment);
        }
    }

  gimp_colormap_editor_update_entries (editor);
}


/*  public functions  */

GtkWidget *
gimp_colormap_editor_new (GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_COLORMAP_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<ColormapEditor>",
                       "ui-path",         "/colormap-editor-popup",
                       NULL);
}

void
gimp_colormap_editor_selected (GimpColormapEditor *editor,
                               GdkModifierType     state)
{
  g_return_if_fail (GIMP_IS_COLORMAP_EDITOR (editor));

  g_signal_emit (editor, editor_signals[SELECTED], 0, state);
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

  gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  palette = editor->palette;
  width   = palette->allocation.width;
  height  = palette->allocation.height;
  ncol    = gimp_image_get_colormap_size (gimage);

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
  GimpImage *gimage;
  guchar    *row;
  gint       cellsize, x, y, k;

  gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

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
          row[k*3]   = gimage->cmap[col * 3];
          row[k*3+1] = gimage->cmap[col * 3 + 1];
          row[k*3+2] = gimage->cmap[col * 3 + 2];
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
          row[k*3]   = gimage->cmap[col * 3];
          row[k*3+1] = gimage->cmap[col * 3 + 1];
          row[k*3+2] = gimage->cmap[col * 3 + 2];
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

  width  = palette->allocation.width;
  height = palette->allocation.height;

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
  GimpImage *gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (! HAVE_COLORMAP (gimage))
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

      col = &gimage->cmap[editor->col_index * 3];

      string = g_strdup_printf ("%02x%02x%02x", col[0], col[1], col[2]);
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
  GimpImage *gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (HAVE_COLORMAP (gimage))
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
  guint      col;

  gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (! HAVE_COLORMAP (gimage))
    return TRUE;

  if (!(bevent->y < editor->cellsize * editor->yn
        && bevent->x < editor->cellsize * editor->xn))
    return TRUE;

  col = (editor->xn * ((gint) bevent->y / editor->cellsize) +
         ((gint) bevent->x / editor->cellsize));

  if (col >= gimp_image_get_colormap_size (gimage))
    return TRUE;

  switch (bevent->button)
    {
    case 1:
      gimp_colormap_editor_set_index (editor, col);
      gimp_colormap_editor_selected (editor, bevent->state);

      if (bevent->type == GDK_2BUTTON_PRESS)
        {
          GtkAction *action;

          action = gimp_ui_manager_find_action (GIMP_EDITOR (editor)->ui_manager,
                                                "colormap-editor",
                                                "colormap-editor-edit-color");

          if (action)
            gtk_action_activate (action);
        }
      return TRUE;

    case 2:
      editor->dnd_col_index = col;
      break;

    case 3:
      gimp_colormap_editor_set_index (editor, col);
      gimp_editor_popup_menu (GIMP_EDITOR (editor), NULL, NULL);
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
  gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (HAVE_COLORMAP (gimage))
    gimp_image_get_colormap_entry (gimage, editor->dnd_col_index, color);
}

static void
gimp_colormap_preview_drop_color (GtkWidget     *widget,
                                  const GimpRGB *color,
                                  gpointer       data)
{
  GimpColormapEditor *editor;
  GimpImage          *gimage;

  editor = GIMP_COLORMAP_EDITOR (data);
  gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (HAVE_COLORMAP (gimage) && gimp_image_get_colormap_size (gimage) < 256)
    {
      gimp_image_add_colormap_entry (gimage, color);
    }
}

static void
gimp_colormap_adjustment_changed (GtkAdjustment      *adjustment,
                                  GimpColormapEditor *editor)
{
  GimpImage *gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (HAVE_COLORMAP (gimage))
    {
      gimp_colormap_editor_set_index (editor, adjustment->value + 0.5);

      gimp_colormap_editor_update_entries (editor);
    }
}

static void
gimp_colormap_hex_entry_activate (GtkEntry           *entry,
                                  GimpColormapEditor *editor)
{
  GimpImage *gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  if (gimage)
    {
      const gchar *s;
      gulong       i;

      s = gtk_entry_get_text (entry);

      if (sscanf (s, "%lx", &i))
        {
          GimpRGB color;
          guchar  r, g, b;

          r = (i & 0xFF0000) >> 16;
          g = (i & 0x00FF00) >> 8;
          b = (i & 0x0000FF);

          gimp_rgb_set_uchar (&color, r, g, b);

          gimp_image_set_colormap_entry (gimage, editor->col_index, &color,
                                         TRUE);
          gimp_image_flush (gimage);
        }
    }
}

static gboolean
gimp_colormap_hex_entry_focus_out (GtkEntry           *entry,
                                   GdkEvent           *event,
                                   GimpColormapEditor *editor)
{
  gimp_colormap_hex_entry_activate (entry, editor);

  return FALSE;
}

static void
gimp_colormap_image_mode_changed (GimpImage          *gimage,
                                  GimpColormapEditor *editor)
{
  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  gimp_colormap_image_colormap_changed (gimage, -1, editor);
}

static void
gimp_colormap_image_colormap_changed (GimpImage          *gimage,
                                      gint                ncol,
                                      GimpColormapEditor *editor)
{
  if (HAVE_COLORMAP (gimage))
    {
      if (ncol < 0)
        {
          gimp_colormap_editor_draw (editor);
        }
      else
        {
          gimp_colormap_editor_draw_cell (editor, ncol);
        }

      if (editor->index_adjustment->upper !=
          (gimp_image_get_colormap_size (gimage) - 1))
        {
          editor->index_adjustment->upper =
            gimp_image_get_colormap_size (gimage) - 1;

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
