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

#include "libgimpcolor/gimpcolor.h"

#include "paint-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"

#include "gimppaintoptions.h"
#include "gimppencil.h"


static void   gimp_pencil_class_init (GimpPencilClass    *klass);
static void   gimp_pencil_init       (GimpPencil         *pencil);

static void   gimp_pencil_paint      (GimpPaintCore      *paint_core,
                                      GimpDrawable       *drawable,
                                      GimpPaintOptions   *paint_options,
                                      GimpPaintCoreState  paint_state);

static void   gimp_pencil_motion     (GimpPaintCore      *paint_core,
                                      GimpDrawable       *drawable,
                                      GimpPaintOptions   *paint_options);


static GimpPaintCoreClass *parent_class = NULL;


GType
gimp_pencil_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpPencilClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_pencil_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPencil),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_pencil_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_CORE,
                                     "GimpPencilCore", 
                                     &info, 0);
    }

  return type;
}

static void 
gimp_pencil_class_init (GimpPencilClass *klass)
{  
  GimpPaintCoreClass *paint_core_class;

  paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  paint_core_class->paint = gimp_pencil_paint;
}

static void
gimp_pencil_init (GimpPencil *pencil)
{
  GimpPaintCore *paint_core;

  paint_core = GIMP_PAINT_CORE (pencil);

  paint_core->flags |= CORE_HANDLES_CHANGING_BRUSH;
}
             
static void
gimp_pencil_paint (GimpPaintCore      *paint_core,
                   GimpDrawable       *drawable,
                   GimpPaintOptions   *paint_options,
                   GimpPaintCoreState  paint_state)
{
  switch (paint_state)
    {
    case INIT_PAINT:
      break;

    case MOTION_PAINT:
      gimp_pencil_motion (paint_core, drawable, paint_options);
      break;

    case FINISH_PAINT:
      break;

    default:
      break;
    }
}

static void
gimp_pencil_motion (GimpPaintCore    *paint_core,
                    GimpDrawable     *drawable,
                    GimpPaintOptions *paint_options)
{
  GimpImage            *gimage;
  GimpContext          *context;
  TempBuf              *area;
  guchar                col[MAX_CHANNELS];
  gint                  opacity;
  gdouble               scale;
  PaintApplicationMode  paint_appl_mode;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  context = gimp_get_current_context (gimage->gimp);

  paint_appl_mode = paint_options->incremental ? INCREMENTAL : CONSTANT;

  if (paint_options->pressure_options->size)
    scale = paint_core->cur_coords.pressure;
  else
    scale = 1.0;

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  /*  color the pixels  */
  if (paint_options->pressure_options->color)
    {
      GimpRGB color;

      gimp_gradient_get_color_at (gimp_context_get_gradient (context),
				  paint_core->cur_coords.pressure,
                                  &color);

      gimp_rgba_get_uchar (&color,
			   &col[RED_PIX],
			   &col[GREEN_PIX],
			   &col[BLUE_PIX],
			   &col[ALPHA_PIX]);

      paint_appl_mode = INCREMENTAL;
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height,
                    area->bytes);
    }
  else if (paint_core->brush && paint_core->brush->pixmap)
    {
      /* if its a pixmap, do pixmap stuff */      
      gimp_paint_core_color_area_with_pixmap (paint_core,
                                              gimage, drawable, 
                                              area, scale, HARD);
      paint_appl_mode = INCREMENTAL;
    }
  else
    {
      gimp_image_get_foreground (gimage, drawable, col);
      col[area->bytes - 1] = OPAQUE_OPACITY;
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height,
                    area->bytes);
    }

  opacity = 255 * gimp_context_get_opacity (context);

  if (paint_options->pressure_options->opacity)
    opacity = opacity * 2.0 * paint_core->cur_coords.pressure;

  /*  paste the newly painted canvas to the gimage which is being worked on  */
  gimp_paint_core_paste_canvas (paint_core, drawable, 
                                MIN (opacity, 255),
                                gimp_context_get_opacity (context) * 255,
                                gimp_context_get_paint_mode (context),
                                HARD, scale, paint_appl_mode);
}
