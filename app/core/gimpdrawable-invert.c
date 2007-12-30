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

#define ENABLE_GEGL
#ifdef ENABLE_GEGL

#include "config.h"

#include <glib-object.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpdrawable.h"
#include "gimpdrawable-invert.h"
#include "gimpimage.h"
#include "gimpprogress.h"
#include "gimp-intl.h"

typedef struct Data {
  GeglNode      *gegl;
  GeglNode      *output;
  GeglProcessor *processor;
  GeglRectangle  rect;
  GimpDrawable  *drawable;
  GimpProgress  *progress;
} Data;

static gboolean
idle_fun (gpointer fdata)
{
  Data *data = fdata;
  gboolean     more_work;
  gdouble      progress;

  more_work = gegl_processor_work (data->processor, &progress);
  if (more_work)
    {
      gimp_progress_set_text (data->progress, _("Invert"));
      gimp_progress_set_value (data->progress, progress);
      return TRUE;
    }

  g_object_unref (data->processor);
  gimp_drawable_update (data->drawable, data->rect.x,     data->rect.y,
                                        data->rect.width, data->rect.height);
  gimp_drawable_merge_shadow (data->drawable, TRUE, _("Invert"));
  {
    GimpImage *image = gimp_item_get_image (GIMP_ITEM (data->drawable));
    if (image)
      {
        /* FIXME: gimp image flush has already been called by the action,
         * it needs to still do so as long as only the ifdef is used to
         * toggle between the versions
         */
        gimp_image_flush (image);
      }
  }

  gimp_progress_end (data->progress);
  g_object_unref (data->gegl);
  g_free (data);
  return FALSE;
}

void
gimp_drawable_invert (GimpDrawable *drawable,
                      GimpProgress *progress)
{
  GeglNode      *input;
  GeglNode      *invert;

  Data *data;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  data = g_malloc0 (sizeof (Data));
  data->drawable = drawable;

  if (! gimp_drawable_mask_intersect (drawable,
                                      &data->rect.x,     &data->rect.y,
                                      &data->rect.width, &data->rect.height))
    return;

  data->gegl     = gegl_node_new ();
  data->progress = progress;
  input  = gegl_node_new_child (data->gegl,
                     "operation",    "gimp-tilemanager-source",
                     "tile-manager", gimp_drawable_get_tiles (drawable),
                     NULL);
  data->output = gegl_node_new_child (data->gegl,
                     "operation",    "gimp-tilemanager-sink",
                     "tile-manager", gimp_drawable_get_shadow_tiles (drawable),
                     NULL);
  invert = gegl_node_new_child (data->gegl,
                     "operation",    "invert",
                     NULL);

  gegl_node_link_many (input, invert, data->output, NULL);

  data->processor = gegl_node_new_processor (data->output, &data->rect);

  gimp_progress_start (data->progress, _("Invert"), FALSE);

  /* do the actual processing in an idle callback */
  g_idle_add (idle_fun, data);
}


#else /* ENABLE_GEGL is not defined */ 

#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "base/gimplut.h"
#include "base/lut-funcs.h"
#include "base/pixel-processor.h"
#include "base/pixel-region.h"

#include "gimpdrawable.h"
#include "gimpdrawable-invert.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


void
gimp_drawable_invert (GimpDrawable *drawable,
                      GimpProgress *progress)
{
  PixelRegion  srcPR, destPR;
  gint         x, y, width, height;
  GimpLut     *lut;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    return;

  lut = invert_lut_new (gimp_drawable_bytes (drawable));

  pixel_region_init (&srcPR, gimp_drawable_get_tiles (drawable),
                     x, y, width, height, FALSE);
  pixel_region_init (&destPR, gimp_drawable_get_shadow_tiles (drawable),
                     x, y, width, height, TRUE);

  pixel_regions_process_parallel ((PixelProcessorFunc) gimp_lut_process,
                                  lut, 2, &srcPR, &destPR);

  gimp_lut_free (lut);

  gimp_drawable_merge_shadow (drawable, TRUE, _("Invert"));

  gimp_drawable_update (drawable, x, y, width, height);
}

#endif
