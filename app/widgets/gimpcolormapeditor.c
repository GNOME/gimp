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

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpmarshal.h"

#include "gimpcolormapeditor.h"
#include "gimpdnd.h"
#include "gimprender.h"
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

#define EPSILON 1e-10

#define HAVE_COLORMAP(image) \
        (image != NULL && \
         gimp_image_base_type (image) == GIMP_INDEXED && \
         gimp_image_get_colormap (image) != NULL)


static GObject * gimp_colormap_editor_constructor  (GType               type,
                                                    guint               n_params,
                                                    GObjectConstructParam *params);

static void   gimp_colormap_editor_destroy         (GtkObject          *object);
static void   gimp_colormap_editor_unmap           (GtkWidget          *widget);

static void   gimp_colormap_editor_set_image       (GimpImageEditor    *editor,
                                                    GimpImage          *image);

static void   gimp_colormap_editor_draw            (GimpColormapEditor *editor);
static void   gimp_colormap_editor_draw_cell       (GimpColormapEditor *editor,
                                                    gint                col);
static void   gimp_colormap_editor_clear           (GimpColormapEditor *editor,
                                                    gint                start_row);
static void   gimp_colormap_editor_update_entries  (GimpColormapEditor *editor);

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
                                                    gint                x,
                                                    gint                y,
                                                    const GimpRGB      *color,
                                                    gpointer            data);

static void   gimp_colormap_adjustment_changed     (GtkAdjustment      *adjustment,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_hex_entry_changed      (GimpColorHexEntry  *entry,
                                                    GimpColormapEditor *editor);

static void   gimp_colormap_image_mode_changed     (GimpImage          *image,
                                                    GimpColormapEditor *editor);
static void   gimp_colormap_image_colormap_changed (GimpImage          *image,
                                                    gint                ncol,
                                                    GimpColormapEditor *editor);


G_DEFINE_TYPE (GimpColormapEditor, gimp_colormap_editor,
               GIMP_TYPE_IMAGE_EDITOR)

#define parent_class gimp_colormap_editor_parent_class

static guint editor_signals[LAST_SIGNAL] = { 0 };


static void
gimp_colormap_editor_class_init (GimpColormapEditorClass* klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GtkObjectClass       *gtk_object_class   = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

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
  GtkObject *adj;

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

  editor->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_widget_set_size_request (editor->preview, -1, 60);
  gtk_preview_set_expand (GTK_PREVIEW (editor->preview), TRUE);
  gtk_widget_add_events (editor->preview, GDK_BUTTON_PRESS_MASK);
  gtk_container_add (GTK_CONTAINER (frame), editor->preview);
  gtk_widget_show (editor->preview);

  g_signal_connect_after (editor->preview, "size-allocate",
                          G_CALLBACK (gimp_colormap_preview_size_allocate),
                          editor);

  g_signal_connect (editor->preview, "button-press-event",
                    G_CALLBACK (gimp_colormap_preview_button_press),
                    editor);

  gimp_dnd_color_source_add (editor->preview, gimp_colormap_preview_drag_color,
                             editor);
  gimp_dnd_color_dest_add (editor->preview, gimp_colormap_preview_drop_color,
                           editor);

  /*  Some helpful hints  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_box_pack_end (GTK_BOX (editor), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  editor->index_spinbutton = gimp_spin_button_new (&adj,
                                                   0, 0, 0, 1, 10, 10, 1.0, 0);
  editor->index_adjustment = GTK_ADJUSTMENT (adj);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Color index:"), 0.0, 0.5,
                             editor->index_spinbutton, 1, TRUE);

  g_signal_connect (editor->index_adjustment, "value-changed",
                    G_CALLBACK (gimp_colormap_adjustment_changed),
                    editor);

  editor->color_entry = gimp_color_hex_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (editor->color_entry), 12);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("HTML notation:"), 0.0, 0.5,
                             editor->color_entry, 1, TRUE);

  g_signal_connect (editor->color_entry, "color-changed",
                    G_CALLBACK (gimp_colormap_hex_entry_changed),
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
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap",
                                   "colormap-edit-color",
                                   NULL);

  editor->add_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "colormap",
                                   "colormap-add-color-from-fg",
                                   "colormap-add-color-from-bg",
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
                                GimpImage       *image)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (image_editor);

  if (image_editor->image)
    {
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_colormap_image_mode_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_colormap_image_colormap_changed,
                                            editor);

      if (editor->color_dialog)
        gtk_widget_hide (editor->color_dialog);

      if (! HAVE_COLORMAP (image))
        {
          editor->index_adjustment->upper = 0;
          gtk_adjustment_changed (editor->index_adjustment);

          if (GTK_WIDGET_MAPPED (GTK_WIDGET (editor)))
            gimp_colormap_editor_clear (editor, -1);
        }
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  editor->col_index     = 0;
  editor->dnd_col_index = 0;

  if (image)
    {
      g_signal_connect (image, "mode-changed",
                        G_CALLBACK (gimp_colormap_image_mode_changed),
                        editor);
      g_signal_connect (image, "colormap-changed",
                        G_CALLBACK (gimp_colormap_image_colormap_changed),
                        editor);

      if (HAVE_COLORMAP (image))
        {
          gimp_colormap_editor_draw (editor);

          editor->index_adjustment->upper =
            gimp_image_get_colormap_size (image) - 1;

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
                       "menu-identifier", "<Colormap>",
                       "ui-path",         "/colormap-popup",
                       NULL);
}

gint
gimp_colormap_editor_get_index (GimpColormapEditor *editor,
                                const GimpRGB      *search)
{
  GimpImage *image;
  gint       index;

  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), 01);

  image = GIMP_IMAGE_EDITOR (editor)->image;

  if (! HAVE_COLORMAP (image))
    return -1;

  index = editor->col_index;

  if (search)
    {
      GimpRGB temp;

      gimp_image_get_colormap_entry (image, index, &temp);

      if (gimp_rgb_distance (&temp, search) > EPSILON)
        {
          gint n_colors = gimp_image_get_colormap_size (image);
          gint i;

          for (i = 0; i < n_colors; i++)
            {
              gimp_image_get_colormap_entry (image, i, &temp);

              if (gimp_rgb_distance (&temp, search) < EPSILON)
                {
                  index = i;
                  break;
                }
            }
        }
    }

  return index;
}

gboolean
gimp_colormap_editor_set_index (GimpColormapEditor *editor,
                                gint                index,
                                GimpRGB            *color)
{
  GimpImage *image;
  gint       size;

  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), FALSE);

  image = GIMP_IMAGE_EDITOR (editor)->image;

  if (! HAVE_COLORMAP (image))
    return FALSE;

  size = gimp_image_get_colormap_size (image);

  if (size < 1)
    return FALSE;

  index = CLAMP (index, 0, size - 1);

  if (index != editor->col_index)
    {
      gint old = editor->col_index;

      editor->col_index     = index;
      editor->dnd_col_index = index;

      gimp_colormap_editor_draw_cell (editor, old);
      gimp_colormap_editor_draw_cell (editor, index);

      gimp_colormap_editor_update_entries (editor);
    }

  if (color)
    gimp_image_get_colormap_entry (GIMP_IMAGE_EDITOR (editor)->image,
                                   index, color);

  return TRUE;
}

gint
gimp_colormap_editor_max_index (GimpColormapEditor *editor)
{
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_COLORMAP_EDITOR (editor), -1);

  image = GIMP_IMAGE_EDITOR (editor)->image;

  if (! HAVE_COLORMAP (image))
    return -1;

  return MAX (0, gimp_image_get_colormap_size (image) - 1);
}


/*  private functions  */

#define MIN_CELL_SIZE 4

static void
gimp_colormap_editor_draw (GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;
  gint       i, j, k, l, b;
  gint       col;
  guchar    *row;
  gint       cellsize, ncol, xn, yn, width, height;

  width  = editor->preview->allocation.width;
  height = editor->preview->allocation.height;
  ncol   = gimp_image_get_colormap_size (image);

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

  yn = ((ncol + xn - 1) / xn);

  /* We used to render just multiples of "cellsize" here, but the
   *  colormap as dockable looks better if it always fills the
   *  available allocation->width (which should always be larger than
   *  "xn * cellsize"). Defensively, we use MAX (width, xn * cellsize)
   *  below   --Mitch
   */

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
              row[(j * cellsize + k) * 3 + b] = image->cmap[col * 3 + b];
        }

      for (k = 0; k < cellsize; k++)
        {
          for (l = j * cellsize; l < width; l++)
            for (b = 0; b < 3; b++)
              row[l * 3 + b] = (((((i * cellsize + k) & 0x4) ?
                                  (l) :
                                  (l + 0x4)) & 0x4) ?
                                gimp_render_blend_light_check[0] :
                                gimp_render_blend_dark_check[0]);

          gtk_preview_draw_row (GTK_PREVIEW (editor->preview), row, 0,
                                i * cellsize + k,
                                width);
        }
    }

  gimp_colormap_editor_clear (editor, yn * cellsize);

  gimp_colormap_editor_draw_cell (editor, editor->col_index);

  g_free (row);
  gtk_widget_queue_draw (editor->preview);
}

static void
gimp_colormap_editor_draw_cell (GimpColormapEditor *editor,
                                gint                col)
{
  GimpImage *image    = GIMP_IMAGE_EDITOR (editor)->image;
  gint       cellsize = editor->cellsize;
  guchar    *row      = g_alloca (cellsize * 3);
  gint       x, y, k;

  if (! editor->xn)
    return;

  x = (col % editor->xn) * cellsize;
  y = (col / editor->xn) * cellsize;

  if (col == editor->col_index)
    {
      for (k = 0; k < cellsize; k++)
        row[k*3] = row[k*3+1] = row[k*3+2] = (k & 1) * 255;

      gtk_preview_draw_row (GTK_PREVIEW (editor->preview), row, x, y, cellsize);

      if (!(cellsize & 1))
        for (k = 0; k < cellsize; k++)
          row[k*3] = row[k*3+1] = row[k*3+2] = ((x+y+1) & 1) * 255;

      gtk_preview_draw_row (GTK_PREVIEW (editor->preview), row,
                            x, y+cellsize-1, cellsize);

      row[0] = row[1] = row[2] = 255;
      row[cellsize*3-3] = row[cellsize*3-2] = row[cellsize*3-1] = 255 * (cellsize & 1);

      for (k = 1; k < cellsize - 1; k++)
        {
          row[k*3]   = image->cmap[col * 3];
          row[k*3+1] = image->cmap[col * 3 + 1];
          row[k*3+2] = image->cmap[col * 3 + 2];
        }
      for (k = 1; k < cellsize - 1; k+=2)
        gtk_preview_draw_row (GTK_PREVIEW (editor->preview), row,
                              x, y+k, cellsize);

      row[0] = row[1] = row[2] = 0;
      row[cellsize*3-3] = row[cellsize*3-2] = row[cellsize*3-1]
        = 255 * ((cellsize+1) & 1);
      for (k = 2; k < cellsize - 1; k += 2)
        gtk_preview_draw_row (GTK_PREVIEW (editor->preview), row,
                              x, y+k, cellsize);
    }
  else
    {
      for (k = 0; k < cellsize; k++)
        {
          row[k*3]   = image->cmap[col * 3];
          row[k*3+1] = image->cmap[col * 3 + 1];
          row[k*3+2] = image->cmap[col * 3 + 2];
        }
      for (k = 0; k < cellsize; k++)
        gtk_preview_draw_row (GTK_PREVIEW (editor->preview), row,
                              x, y+k, cellsize);
    }

  gtk_widget_queue_draw_area (editor->preview,
                              x, y,
                              cellsize, cellsize);
}

static void
gimp_colormap_editor_clear (GimpColormapEditor *editor,
                            gint                start_row)
{
  gint    i, j;
  gint    offset;
  gint    width, height;
  guchar *row;

  width  = editor->preview->allocation.width;
  height = editor->preview->allocation.height;

  if (start_row < 0)
    start_row = 0;

  if (start_row >= height)
    return;

  row = g_alloca (width * 3);

  if (start_row & 0x3)
    {
      offset = (start_row & 0x4) ? 0x4 : 0x0;

      for (j = 0; j < width; j++)
        {
          row[j * 3 + 0] = row[j * 3 + 1] = row[j * 3 + 2] =
            ((j + offset) & 0x4) ?
            gimp_render_blend_dark_check[0] : gimp_render_blend_light_check[0];
        }

      for (j = 0; j < (4 - (start_row & 0x3)) && start_row + j < height; j++)
        gtk_preview_draw_row (GTK_PREVIEW (editor->preview), row,
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
            gimp_render_blend_dark_check[0] : gimp_render_blend_light_check[0];
        }

      for (j = 0; j < 4 && i + j < height; j++)
        gtk_preview_draw_row (GTK_PREVIEW (editor->preview), row,
                              0, i + j, width);
    }

  gtk_widget_queue_draw (editor->preview);
}

static void
gimp_colormap_editor_update_entries (GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (! HAVE_COLORMAP (image) ||
      ! gimp_image_get_colormap_size (image))
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

      col = image->cmap + editor->col_index * 3;

      string = g_strdup_printf ("%02x%02x%02x", col[0], col[1], col[2]);
      gtk_entry_set_text (GTK_ENTRY (editor->color_entry), string);
      g_free (string);

      gtk_widget_set_sensitive (editor->index_spinbutton, TRUE);
      gtk_widget_set_sensitive (editor->color_entry, TRUE);
    }
}

static void
gimp_colormap_preview_size_allocate (GtkWidget          *widget,
                                     GtkAllocation      *alloc,
                                     GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (HAVE_COLORMAP (image))
    gimp_colormap_editor_draw (editor);
  else
    gimp_colormap_editor_clear (editor, -1);
}


static gboolean
gimp_colormap_preview_button_press (GtkWidget          *widget,
                                    GdkEventButton     *bevent,
                                    GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;
  guint      col;

  if (! HAVE_COLORMAP (image))
    return TRUE;

  if (!(bevent->y < editor->cellsize * editor->yn
        && bevent->x < editor->cellsize * editor->xn))
    return TRUE;

  col = (editor->xn * ((gint) bevent->y / editor->cellsize) +
         ((gint) bevent->x / editor->cellsize));

  if (col >= gimp_image_get_colormap_size (image))
    return TRUE;

  switch (bevent->button)
    {
    case 1:
      gimp_colormap_editor_set_index (editor, col, NULL);
      g_signal_emit (editor, editor_signals[SELECTED], 0, bevent->state);

      if (bevent->type == GDK_2BUTTON_PRESS)
        gimp_ui_manager_activate_action (GIMP_EDITOR (editor)->ui_manager,
                                         "colormap",
                                         "colormap-edit-color");
      return TRUE;

    case 2:
      editor->dnd_col_index = col;
      break;

    case 3:
      gimp_colormap_editor_set_index (editor, col, NULL);
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
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (data);
  GimpImage          *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (HAVE_COLORMAP (image))
    gimp_image_get_colormap_entry (image, editor->dnd_col_index, color);
}

static void
gimp_colormap_preview_drop_color (GtkWidget     *widget,
                                  gint           x,
                                  gint           y,
                                  const GimpRGB *color,
                                  gpointer       data)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (data);
  GimpImage          *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (HAVE_COLORMAP (image) && gimp_image_get_colormap_size (image) < 256)
    {
      gimp_image_add_colormap_entry (image, color);
    }
}

static void
gimp_colormap_adjustment_changed (GtkAdjustment      *adjustment,
                                  GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (HAVE_COLORMAP (image))
    {
      gimp_colormap_editor_set_index (editor, adjustment->value + 0.5, NULL);

      gimp_colormap_editor_update_entries (editor);
    }
}

static void
gimp_colormap_hex_entry_changed (GimpColorHexEntry  *entry,
                                 GimpColormapEditor *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;

  if (image)
    {
      GimpRGB color;

      gimp_color_hex_entry_get_color (entry, &color);

      gimp_image_set_colormap_entry (image, editor->col_index, &color, TRUE);
      gimp_image_flush (image);
    }
}

static void
gimp_colormap_image_mode_changed (GimpImage          *image,
                                  GimpColormapEditor *editor)
{
  if (editor->color_dialog)
    gtk_widget_hide (editor->color_dialog);

  gimp_colormap_image_colormap_changed (image, -1, editor);
}

static void
gimp_colormap_image_colormap_changed (GimpImage          *image,
                                      gint                ncol,
                                      GimpColormapEditor *editor)
{
  if (HAVE_COLORMAP (image))
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
          (gimp_image_get_colormap_size (image) - 1))
        {
          editor->index_adjustment->upper =
            gimp_image_get_colormap_size (image) - 1;

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
