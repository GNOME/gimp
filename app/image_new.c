/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "image_new.h"

#include "appenv.h"
#include "gimprc.h"
#include "file_new_dialog.h"
#include "tile_manager_pvt.h"
#include "gdisplay.h"
#include "gimpcontext.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"
#include "libgimp/parasite.h"

#include "pixmaps/wilber2.xpm"

static GList              *image_base_type_names = NULL;
static GList              *fill_type_names = NULL;
static GimpImageNewValues  last_values;
static gboolean            current_cut_buffer = FALSE;

extern TileManager        *global_buf;

static void
image_new_init (void)
{
  static gboolean image_new_inited = FALSE;
  GimpImageBaseTypeName *new_type;
  GimpFillTypeName *new_fill_type;

  if (image_new_inited)
    return;
  else
    image_new_inited = TRUE;

  /* Available Image Base Types */
  new_type = g_new (GimpImageBaseTypeName, 1);
  new_type->type = RGB;
  new_type->name = _("RGB");
  image_base_type_names = g_list_append (image_base_type_names, new_type);

  new_type = g_new (GimpImageBaseTypeName, 1);
  new_type->type = GRAY;
  new_type->name = _("Grayscale");
  image_base_type_names = g_list_append (image_base_type_names, new_type);
  
  /* Available Fill Types */
  new_fill_type = g_new (GimpFillTypeName, 1);
  new_fill_type->type = FOREGROUND_FILL;
  new_fill_type->name = _("Foreground");
  fill_type_names = g_list_append (fill_type_names, new_fill_type);

  new_fill_type = g_new (GimpFillTypeName, 1);
  new_fill_type->type = BACKGROUND_FILL;
  new_fill_type->name = _("Background");
  fill_type_names = g_list_append (fill_type_names, new_fill_type);

  new_fill_type = g_new (GimpFillTypeName, 1);
  new_fill_type->type = WHITE_FILL;
  new_fill_type->name = _("White");
  fill_type_names = g_list_append (fill_type_names, new_fill_type);

  new_fill_type = g_new (GimpFillTypeName, 1);
  new_fill_type->type = TRANSPARENT_FILL;
  new_fill_type->name = _("Transparent");
  fill_type_names = g_list_append (fill_type_names, new_fill_type);

  /* Set the last values used to default values. */
  last_values.width = default_width;
  last_values.height = default_height;
  last_values.unit = default_units;
  last_values.xresolution = default_xresolution;
  last_values.yresolution = default_yresolution;
  last_values.res_unit = default_resolution_units;
  last_values.type = default_type;
  last_values.fill_type = BACKGROUND_FILL;
}

GList*
image_new_get_image_base_type_names (void)
{
  image_new_init ();

  return image_base_type_names;
}

GList*
image_new_get_fill_type_names (void)
{
  image_new_init ();

  return fill_type_names;
}

void
image_new_create_window (const GimpImageNewValues *create_values,
                         const GimpImage          *image)
{
  GimpImageNewValues *values;

  image_new_init ();

  values = image_new_values_new (create_values);
  
  if (image)
    {
      values->width = gimp_image_get_width (image);
      values->height = gimp_image_get_height (image);
      values->unit = gimp_image_get_unit (image);
      
      gimp_image_get_resolution (image, 
				 &values->xresolution, 
				 &values->yresolution);

      values->type = gimp_image_base_type (image);

      if (values->type == INDEXED)
        values->type = RGB; /* no indexed images */
    }

  /*  If a cut buffer exists, default to using its size for the new image
   *  also check to see if a new_image has been opened
   */
  if (global_buf && current_cut_buffer)
    {
      values->width = global_buf->width;
      values->height = global_buf->height;
    }

  ui_new_image_window_create (values);

  image_new_values_free (values);
}

void
image_new_set_default_values (const GimpImageNewValues *values)
{
  g_return_if_fail (values != NULL);

  image_new_init ();

  memcpy(&last_values, values, sizeof (GimpImageNewValues));

  current_cut_buffer = FALSE;
}

GimpImageNewValues*
image_new_values_new (const GimpImageNewValues *src_values)
{
  GimpImageNewValues *values;

  image_new_init ();

  values = g_new (GimpImageNewValues, 1);

  if (src_values)
    memcpy(values, src_values, sizeof (GimpImageNewValues));
  else
    memcpy(values, &last_values, sizeof (GimpImageNewValues));

  return values;
}

void
image_new_values_free (GimpImageNewValues *values)
{
  g_return_if_fail (values != NULL);

  g_free (values);
}

static gint
image_new_rotate_the_shield_harmonics (GDisplay *display)
{
  static GdkPixmap *pixmap = NULL;
  static GdkBitmap *mask   = NULL;
  static gint width  = 0;
  static gint height = 0;

  while (gtk_events_pending ())
    gtk_main_iteration ();

  if (pixmap == NULL)
    {
      GtkStyle *style;

      style = gtk_widget_get_style (display->canvas);

      pixmap =
	gdk_pixmap_create_from_xpm_d (display->canvas->window,
				      &mask,
				      &style->bg[GTK_STATE_NORMAL],
				      wilber2_xpm);

      gdk_window_get_size (pixmap, &width, &height);
    }

  if (display->canvas->allocation.width  >= width &&
      display->canvas->allocation.height >= height)
    {
      gint x, y;

      x = (display->canvas->allocation.width  - width) / 2;
      y = (display->canvas->allocation.height - height) / 2;

      gdk_gc_set_clip_mask (display->canvas->style->black_gc, mask);
      gdk_gc_set_clip_origin (display->canvas->style->black_gc, x, y);

      gdk_draw_pixmap (display->canvas->window,
		       display->canvas->style->black_gc,
		       pixmap, 0, 0,
		       x, y,
		       width, height);

      gdk_gc_set_clip_mask (display->canvas->style->black_gc, NULL);
      gdk_gc_set_clip_origin (display->canvas->style->black_gc, 0, 0);
    }

  return FALSE;
}

void 
image_new_create_image (const GimpImageNewValues *values)
{
  GimpImage *image;
  GDisplay *display;
  Layer *layer;
  GimpImageType type;
  GimpParasite *comment_parasite;
  gint width, height;

  g_return_if_fail (values != NULL);

  image_new_set_default_values (values);

  switch (values->fill_type)
    {
    case FOREGROUND_FILL:
    case BACKGROUND_FILL:
    case WHITE_FILL:
      type = (values->type == RGB) ? RGB_GIMAGE : GRAY_GIMAGE;
      break;
    case TRANSPARENT_FILL:
      type = (values->type == RGB) ? RGBA_GIMAGE : GRAYA_GIMAGE;
      break;
    default:
      type = RGB_GIMAGE; 
      break;
    }

  image = gimage_new (values->width, values->height, values->type);
  
  gimp_image_set_resolution (image, values->xresolution, values->yresolution);
  gimp_image_set_unit (image, values->unit);

  if (default_comment)
    {
      comment_parasite = gimp_parasite_new ("gimp-comment",
					    GIMP_PARASITE_PERSISTENT,
					    strlen (default_comment) + 1,
					    (gpointer) default_comment);
      gimp_image_parasite_attach (image, comment_parasite);
      gimp_parasite_free (comment_parasite);
    }
  
  /*  Make the background (or first) layer  */
  width = gimp_image_get_width (image);
  height = gimp_image_get_height (image);
  layer = layer_new (image, width, height,
                     type, _("Background"),
		     OPAQUE_OPACITY, NORMAL_MODE);
 
  if (layer)
    {
      /*  add the new layer to the gimage  */
      gimp_image_undo_disable (image);
      gimp_image_add_layer (image, layer, 0);
      gimp_image_undo_enable (image);

      drawable_fill (GIMP_DRAWABLE (layer), values->fill_type);

      gimp_image_clean_all (image);

      display = gdisplay_new (image, 0x0101);

      gimp_context_set_display (gimp_context_get_user (), display);

      if (double_speed)
	gtk_idle_add ((GtkFunction) image_new_rotate_the_shield_harmonics,
		      display);
    }
}

gdouble
image_new_calculate_size (GimpImageNewValues *values)
{
  gdouble width, height, size;

  width = (gdouble) values->width;
  height = (gdouble) values->height;

  size = 
    width * height *
      ((values->type == RGB ? 3 : 1) +                   /* bytes per pixel */
       (values->fill_type == TRANSPARENT_FILL ? 1 : 0)); /* alpha channel */

  return size;
}

gchar *
image_new_get_size_string (gdouble size)
{
  if (size < 4096)
    return g_strdup_printf (_("%d Bytes"), (gint) size);
  else if (size < 1024 * 10)
    return g_strdup_printf (_("%.2f KB"), size / 1024);
  else if (size < 1024 * 100)
    return g_strdup_printf (_("%.1f KB"), size / 1024);
  else if (size < 1024 * 1024)
    return g_strdup_printf (_("%d KB"), (gint) size / 1024);
  else if (size < 1024 * 1024 * 10)
    return g_strdup_printf (_("%.2f MB"), size / 1024 / 1024);
  else
    return g_strdup_printf (_("%.1f MB"), size / 1024 / 1024);
}

void
image_new_reset_current_cut_buffer (void)
{
  /* This function just changes the status of current_cut_buffer
     if there hass been a cut/copy since the last file new */
  current_cut_buffer = TRUE;
}

