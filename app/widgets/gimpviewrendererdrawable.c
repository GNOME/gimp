/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreviewrendererdrawable.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "gimppreviewrendererdrawable.h"


static void   gimp_preview_renderer_drawable_class_init (GimpPreviewRendererDrawableClass *klass);
static void   gimp_preview_renderer_drawable_init       (GimpPreviewRendererDrawable      *renderer);

static void   gimp_preview_renderer_drawable_render     (GimpPreviewRenderer *renderer,
                                                         GtkWidget           *widget);


static GimpPreviewRendererClass *parent_class = NULL;


GType
gimp_preview_renderer_drawable_get_type (void)
{
  static GType renderer_type = 0;

  if (! renderer_type)
    {
      static const GTypeInfo renderer_info =
      {
        sizeof (GimpPreviewRendererDrawableClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_preview_renderer_drawable_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPreviewRendererDrawable),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_preview_renderer_drawable_init,
      };

      renderer_type = g_type_register_static (GIMP_TYPE_PREVIEW_RENDERER,
                                              "GimpPreviewRendererDrawable",
                                              &renderer_info, 0);
    }

  return renderer_type;
}

static void
gimp_preview_renderer_drawable_class_init (GimpPreviewRendererDrawableClass *klass)
{
  GimpPreviewRendererClass *renderer_class;

  renderer_class = GIMP_PREVIEW_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  renderer_class->render = gimp_preview_renderer_drawable_render;
}

static void
gimp_preview_renderer_drawable_init (GimpPreviewRendererDrawable *renderer)
{
}

static void
gimp_preview_renderer_drawable_render (GimpPreviewRenderer *renderer,
                                       GtkWidget           *widget)
{
  GimpDrawable *drawable;
  GimpItem     *item;
  GimpImage    *gimage;
  gint          width;
  gint          height;
  gint          preview_width;
  gint          preview_height;
  gboolean      scaling_up;
  TempBuf      *render_buf = NULL;

  drawable = GIMP_DRAWABLE (renderer->viewable);
  item     = GIMP_ITEM (drawable);
  gimage   = gimp_item_get_image (item);

  width  = renderer->width;
  height = renderer->height;

  if (gimage && ! renderer->is_popup)
    {
      width  = MAX (1, ROUND ((((gdouble) width / (gdouble) gimage->width) *
			       (gdouble) item->width)));
      height = MAX (1, ROUND ((((gdouble) height / (gdouble) gimage->height) *
			      (gdouble) item->height)));

      gimp_viewable_calc_preview_size (item->width,
                                       item->height,
                                       width,
                                       height,
                                       renderer->dot_for_dot,
                                       gimage->xresolution,
                                       gimage->yresolution,
                                       &preview_width,
                                       &preview_height,
                                       &scaling_up);
    }
  else
    {
      gimp_viewable_calc_preview_size (item->width,
                                       item->height,
                                       width,
                                       height,
                                       renderer->dot_for_dot,
                                       gimage ? gimage->xresolution : 1.0,
                                       gimage ? gimage->yresolution : 1.0,
                                       &preview_width,
                                       &preview_height,
                                       &scaling_up);
    }

  if (scaling_up)
    {
      TempBuf *temp_buf;

      temp_buf = gimp_viewable_get_new_preview (renderer->viewable,
                                                item->width, 
						item->height);

      if (temp_buf)
        {
          render_buf = temp_buf_scale (temp_buf, preview_width, preview_height);

          temp_buf_free (temp_buf);
        }
    }
  else
    {
      render_buf = gimp_viewable_get_new_preview (renderer->viewable,
						  preview_width,
						  preview_height);
    }

  if (render_buf)
    {
      if (gimage && ! renderer->is_popup)
        {
          if (item->offset_x != 0)
            render_buf->x =
              ROUND ((((gdouble) renderer->width / (gdouble) gimage->width) *
                      (gdouble) item->offset_x));

          if (item->offset_y != 0)
            render_buf->y =
              ROUND ((((gdouble) renderer->height / (gdouble) gimage->height) *
                      (gdouble) item->offset_y));
        }
      else
        {
          if (preview_width < width)
            render_buf->x = (width - preview_width) / 2;

          if (preview_height < height)
            render_buf->y = (height - preview_height) / 2;
        }

      gimp_preview_renderer_render_buffer (renderer, render_buf, -1,
                                           GIMP_PREVIEW_BG_CHECKS,
                                           GIMP_PREVIEW_BG_CHECKS);

      temp_buf_free (render_buf);
    }
  else
    {
      const gchar *stock_id;

      stock_id = gimp_viewable_get_stock_id (renderer->viewable);

      gimp_preview_renderer_default_render_stock (renderer, widget, stock_id);
    }
}
