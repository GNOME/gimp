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

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimptoolinfo.h"


static void      gimp_tool_info_class_init      (GimpToolInfoClass *klass);
static void      gimp_tool_info_init            (GimpToolInfo      *tool_info);

static void      gimp_tool_info_finalize        (GObject           *object);

static TempBuf * gimp_tool_info_get_new_preview (GimpViewable      *viewable,
						 gint               width,
						 gint               height);


static GimpDataClass *parent_class = NULL;


GType
gimp_tool_info_get_type (void)
{
  static GType tool_info_type = 0;

  if (! tool_info_type)
    {
      static const GTypeInfo tool_info_info =
      {
        sizeof (GimpToolInfoClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_tool_info_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpToolInfo),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_tool_info_init,
      };

      tool_info_type = g_type_register_static (GIMP_TYPE_DATA,
					       "GimpToolInfo",
					       &tool_info_info, 0);
    }

  return tool_info_type;
}

static void
gimp_tool_info_class_init (GimpToolInfoClass *klass)
{
  GObjectClass      *object_class;
  GimpViewableClass *viewable_class;

  object_class   = G_OBJECT_CLASS (klass);
  viewable_class = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize          = gimp_tool_info_finalize;

  viewable_class->get_new_preview = gimp_tool_info_get_new_preview;
}

void
gimp_tool_info_init (GimpToolInfo *tool_info)
{
  tool_info->tool_type    = G_TYPE_NONE;

  tool_info->blurb        = NULL;
  tool_info->help         = NULL;

  tool_info->menu_path    = NULL;
  tool_info->menu_accel   = NULL;

  tool_info->help_domain  = NULL;
  tool_info->help_data    = NULL;

  tool_info->stock_id     = NULL;
  tool_info->stock_pixbuf = NULL;

  tool_info->context      = NULL;

  tool_info->tool_options = NULL;
}

static void
gimp_tool_info_finalize (GObject *object)
{
  GimpToolInfo *tool_info;

  tool_info = (GimpToolInfo *) object;

  if (tool_info->blurb)
    {
      g_free (tool_info->blurb);
      tool_info->blurb = NULL;
    }
  if (tool_info->help)
    {
      g_free (tool_info->help);
      tool_info->blurb = NULL;
    }

  if (tool_info->menu_path)
    {
      g_free (tool_info->menu_path);
      tool_info->menu_path = NULL;
    }
  if (tool_info->menu_accel)
    {
      g_free (tool_info->menu_accel);
      tool_info->menu_accel = NULL;
    }

  if (tool_info->help_domain)
    {
      g_free (tool_info->help_domain);
      tool_info->help_domain = NULL;
    }
  if (tool_info->help_data)
    {
      g_free (tool_info->help_data);
      tool_info->help_data = NULL;
    }

  if (tool_info->stock_pixbuf)
    {
      g_object_unref (G_OBJECT (tool_info->stock_pixbuf));
      tool_info->stock_pixbuf = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static TempBuf *
temp_buf_new_from_pixbuf (GdkPixbuf *pixbuf,
			  gint       width,
			  gint       height)
{
  TempBuf   *temp_buf;
  gint       pixbuf_width;
  gint       pixbuf_height;
  guchar     transparent[4] = { 0, 0, 0, 0 };
  guchar    *p_data;
  guchar    *t_data;
  guchar    *p;
  guchar    *t;
  gint       x, y;

  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);

  pixbuf_width  = gdk_pixbuf_get_width (pixbuf);
  pixbuf_height = gdk_pixbuf_get_height (pixbuf);

  if (pixbuf_width != width || pixbuf_height != height)
    {
      GdkPixbuf *scaled_pixbuf;

      scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
					       width, height,
					       GDK_INTERP_BILINEAR);

      pixbuf = scaled_pixbuf;
    }
  else
    {
      g_object_ref (G_OBJECT (pixbuf));
    }

  temp_buf = temp_buf_new (width, height, 4, 0, 0, transparent);

  p_data = gdk_pixbuf_get_pixels (pixbuf);
  t_data = temp_buf_data (temp_buf);

  for (y = 0; y < height; y++)
    {
      p = p_data;
      t = t_data;

      for (x = 0; x < width; x++)
	{
	  *t++ = *p++;
	  *t++ = *p++;
	  *t++ = *p++;
	  *t++ = *p++;
	}

      p_data += gdk_pixbuf_get_rowstride (pixbuf);
      t_data += width * 4;
    }

  g_object_unref (G_OBJECT (pixbuf));

  return temp_buf;
}

static TempBuf *
gimp_tool_info_get_new_preview (GimpViewable *viewable,
				gint          width,
				gint          height)
{
  GimpToolInfo *tool_info;

  tool_info = GIMP_TOOL_INFO (viewable);

  return temp_buf_new_from_pixbuf (tool_info->stock_pixbuf, width, height);
}

GimpToolInfo *
gimp_tool_info_new (GimpContext  *context,
		    GType         tool_type,
                    gboolean      tool_context,
		    const gchar  *identifier,
		    const gchar  *blurb,
		    const gchar  *help,
		    const gchar  *menu_path,
		    const gchar  *menu_accel,
		    const gchar  *help_domain,
		    const gchar  *help_data,
		    const gchar  *pdb_string,
		    const gchar  *stock_id,
		    GdkPixbuf    *stock_pixbuf)
{
  GimpToolInfo *tool_info;

  g_return_val_if_fail (! context || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (blurb != NULL, NULL);
  g_return_val_if_fail (help != NULL, NULL);
  g_return_val_if_fail (menu_path != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (GDK_IS_PIXBUF (stock_pixbuf), NULL);

  tool_info = g_object_new (GIMP_TYPE_TOOL_INFO, NULL);

  gimp_object_set_name (GIMP_OBJECT (tool_info), identifier);

  tool_info->tool_type     = tool_type;

  tool_info->blurb         = g_strdup (blurb);
  tool_info->help          = g_strdup (help);

  tool_info->menu_path     = g_strdup (menu_path);
  tool_info->menu_accel    = g_strdup (menu_accel);

  tool_info->help_domain   = g_strdup (help_domain);
  tool_info->help_data     = g_strdup (help_data);

  tool_info->pdb_string    = (pdb_string ?
			      g_strdup (pdb_string) : "gimp_paintbrush_default");

  tool_info->stock_id      = stock_id;
  tool_info->stock_pixbuf  = stock_pixbuf;

  g_object_ref (G_OBJECT (stock_pixbuf));

  if (tool_context)
    {
      tool_info->context = gimp_create_context (context->gimp,
						identifier,
						context);
    }

  return tool_info;
}

void
gimp_tool_info_set_standard (Gimp         *gimp,
			     GimpToolInfo *tool_info)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! tool_info || GIMP_IS_TOOL_INFO (tool_info));

  if (gimp->standard_tool_info)
    {
      g_object_unref (G_OBJECT (gimp->standard_tool_info));
    }

  gimp->standard_tool_info = tool_info;

  if (gimp->standard_tool_info)
    {
      g_object_ref (G_OBJECT (gimp->standard_tool_info));
    }
}

GimpToolInfo *
gimp_tool_info_get_standard (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->standard_tool_info;
}
