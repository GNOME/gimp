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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "paint-funcs/paint-funcs.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "core/gimpitem.h"
#include "core/gimppickable.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable.h"
#include "core/gimppattern.h"

#include "gimpheal.h"
#include "gimphealoptions.h"

#include "gimp-intl.h"

/* NOTES:
 *
 * I had the code working for healing from a pattern, but the results look
 * terrible and I can't see a use for it right now.
 *
 * The support for registered alignment has been removed because it doesn't make
 * sense for healing.
 */

enum
{
  PROP_0,
  PROP_SRC_DRAWABLE,
  PROP_SRC_X,
  PROP_SRC_Y
};

static void   gimp_heal_set_property     (GObject          *object,
                                          guint             property_id,
                                          const GValue     *value,
                                          GParamSpec       *pspec);

static void   gimp_heal_get_property     (GObject          *object,
                                          guint             property_id,
                                          GValue           *value,
                                          GParamSpec       *pspec);

static void   gimp_heal_paint            (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options,
                                          GimpPaintState    paint_state,
                                          guint32           time);

static void   gimp_heal_motion           (GimpPaintCore    *paint_core,
                                          GimpDrawable     *drawable,
                                          GimpPaintOptions *paint_options);

static void   gimp_heal_set_src_drawable (GimpHeal         *heal,
                                          GimpDrawable     *drawable);


G_DEFINE_TYPE (GimpHeal, gimp_heal, GIMP_TYPE_BRUSH_CORE)


void
gimp_heal_register (Gimp                      *gimp,
                    GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,                    /* gimp */
                GIMP_TYPE_HEAL,          /* paint type */
                GIMP_TYPE_HEAL_OPTIONS,  /* paint options type */
                "gimp-heal",             /* identifier */
                _("Heal"),               /* blurb */
                "gimp-tool-heal");       /* stock id */
}

static void
gimp_heal_class_init (GimpHealClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  object_class->set_property               = gimp_heal_set_property;
  object_class->get_property               = gimp_heal_get_property;

  paint_core_class->paint                  = gimp_heal_paint;

  brush_core_class->handles_changing_brush = TRUE;

  g_object_class_install_property (object_class, PROP_SRC_DRAWABLE,
                                   g_param_spec_object ("src-drawable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DRAWABLE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_X,
                                   g_param_spec_double ("src-x", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_Y,
                                   g_param_spec_double ("src-y", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_heal_init (GimpHeal *heal)
{
  heal->set_source   = FALSE;

  heal->src_drawable = NULL;
  heal->src_x        = 0.0;
  heal->src_y        = 0.0;

  heal->offset_x     = 0.0;
  heal->offset_y     = 0.0;
  heal->first_stroke = TRUE;
}

static void
gimp_heal_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpHeal *heal = GIMP_HEAL (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLE:
      gimp_heal_set_src_drawable (heal, g_value_get_object (value));
      break;
    case PROP_SRC_X:
      heal->src_x = g_value_get_double (value);
      break;
    case PROP_SRC_Y:
      heal->src_y = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_heal_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpHeal *heal = GIMP_HEAL (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLE:
      g_value_set_object (value, heal->src_drawable);
      break;
    case PROP_SRC_X:
      g_value_set_int (value, heal->src_x);
      break;
    case PROP_SRC_Y:
      g_value_set_int (value, heal->src_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_heal_paint (GimpPaintCore    *paint_core,
                 GimpDrawable     *drawable,
                 GimpPaintOptions *paint_options,
                 GimpPaintState    paint_state,
                 guint32           time)
{
  GimpHeal *heal = GIMP_HEAL (paint_core);

  /* gimp passes the current state of the painting system to the function */
  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      /* heal->set_source is set by gimphealtool when CTRL is pressed */
      if (heal->set_source)
        {
          /* defined later in this file, sets heal to the current drawable
           * and notifies heal that the drawable is ready */
          gimp_heal_set_src_drawable (heal, drawable);

          /* get the current source coordinates from the paint core */
          heal->src_x = paint_core->cur_coords.x;
          heal->src_y = paint_core->cur_coords.y;

          /* set first stroke to be true */
          heal->first_stroke = TRUE;
        }
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (heal->set_source)
        {
          /*  if the control key is down, move the src and return */
          heal->src_x = paint_core->cur_coords.x;
          heal->src_y = paint_core->cur_coords.y;

          heal->first_stroke = TRUE;
        }
      else
        {
          /*  otherwise, update the target  */

          /* get the current destination from the paint core */
          gint dest_x = paint_core->cur_coords.x;
          gint dest_y = paint_core->cur_coords.y;

          /* if this is the first stroke, record the offset of the destination
           * relative to the source */
          if (heal->first_stroke)
            {
              heal->offset_x = heal->src_x - dest_x;
              heal->offset_y = heal->src_y - dest_y;

              heal->first_stroke = FALSE;
            }

          /* if this is not the first stroke, set the source as
           * destination + offset */
          heal->src_x = dest_x + heal->offset_x;
          heal->src_y = dest_y + heal->offset_y;

          /* defined later, does the actual cloning */
          gimp_heal_motion (paint_core, drawable, paint_options);
        }
      break;

    default:
      break;
    }

  /* notify the brush that the src attributes have changed */
  g_object_notify (G_OBJECT (heal), "src-x");
  g_object_notify (G_OBJECT (heal), "src-y");
}

/*
 * Substitute any zeros in the input PixelRegion for ones.  This is needed by
 * the algorithm to avoid division by zero, and to get a more realistic image
 * since multiplying by zero is often incorrect (i.e., healing from a dark to
 * a light region will have incorrect spots of zero)
 */
static void
gimp_heal_substitute_0_for_1 (PixelRegion *pr)
{
  gint     i, j, k;

  gint     height = pr->h;
  gint     width  = pr->w;
  gint     depth  = pr->bytes;

  guchar  *pr_data = pr->data;

  guchar  *p;

  for (i = 0; i < height; i++)
    {
      p = pr_data;

      for (j = 0; j < width; j++)
        {
          for (k = 0; k < depth; k++)
            {
              if (p[k] == 0)
                p[k] = 1;
            }

          p += depth;
        }

      pr_data += pr->rowstride;
    }
}

/*
 * Divide topPR by bottomPR and store the result as a double
 */
static void
gimp_heal_divide (PixelRegion *topPR,
                  PixelRegion *bottomPR,
                  gdouble     *result)
{
  gint     i, j, k;

  gint     height = topPR->h;
  gint     width  = topPR->w;
  gint     depth  = topPR->bytes;

  guchar  *t_data = topPR->data;
  guchar  *b_data = bottomPR->data;

  guchar  *t;
  guchar  *b;
  gdouble *r      = result;

  for (i = 0; i < height; i++)
    {
      t = t_data;
      b = b_data;

      for (j = 0; j < width; j++)
        {
          for (k = 0; k < depth; k++)
            {
              r[k] = (gdouble) (t[k]) / (gdouble) (b[k]);
            }

          t += depth;
          b += depth;
          r += depth;
        }

      t_data += topPR->rowstride;
      b_data += bottomPR->rowstride;
    }
}

/*
 * multiply first by secondPR and store the result as a PixelRegion
 */
static void
gimp_heal_multiply (gdouble     *first,
                    PixelRegion *secondPR,
                    PixelRegion *resultPR)
{
  gint     i, j, k;

  gint     height = secondPR->h;
  gint     width  = secondPR->w;
  gint     depth  = secondPR->bytes;

  guchar  *s_data = secondPR->data;
  guchar  *r_data = resultPR->data;

  gdouble *f      = first;
  guchar  *s;
  guchar  *r;

  for (i = 0; i < height; i++)
    {
      s = s_data;
      r = r_data;

      for (j = 0; j < width; j++)
        {
          for (k = 0; k < depth; k++)
            {
              r[k] = (guchar) CLAMP0255 (ROUND (((gdouble) (f[k])) * ((gdouble) (s[k]))));
            }

          f += depth;
          s += depth;
          r += depth;
        }

      s_data += secondPR->rowstride;
      r_data += resultPR->rowstride;
    }
}

/*
 * Perform one iteration of the laplace solver for matrix.  Store the result in
 * solution and return the cummulative error of the solution.
 */
gdouble
gimp_heal_laplace_iteration (gdouble *matrix,
                             gint     height,
                             gint     depth,
                             gint     width,
                             gdouble *solution)
{
  gint     rowstride  = width * depth;
  gint     i, j, k;
  gdouble  err        = 0.0;
  gdouble  tmp, diff;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
        {
          if ((i == 0) || (i == (height - 1)) || (j == 0) || (j == (height - 1)))
            /* do nothing at the boundary */
            for (k = 0; k < depth; k++)
              *(solution + k) = *(matrix + k);
          else
            {
              /* calculate the mean of four neighbours for all color channels */
              /* v[i][j] = 0.45 * (v[i][j-1]+v[i][j+1]+v[i-1][j]+v[i+1][j]) */
              for (k = 0; k < depth; k++)
                {
                  tmp = *(solution + k);

                  *(solution + k) = 0.25 *
                                    ( *(matrix - depth + k)       /* west */
                                    + *(matrix + depth + k)       /* east */
                                    + *(matrix - rowstride + k)   /* north */
                                    + *(matrix + rowstride + k)); /* south */

                  diff = *(solution + k) - tmp;
                  err += diff*diff;
                }
            }

          /* advance pointers to data */
          matrix += depth; solution += depth;
        }
    }

  return sqrt (err);
}

/*
 * Solve the laplace equation for matrix and store the result in solution.
 */
void
gimp_heal_laplace_loop (gdouble *matrix,
                        gint     height,
                        gint     depth,
                        gint     width,
                        gdouble *solution)
{
#define EPSILON   0.0001
#define MAX_ITER  500

  gint num_iter = 0;
  gdouble err;

  /* do one iteration and store the amount of error */
  err = gimp_heal_laplace_iteration (matrix, height, depth, width, solution);

  /* copy solution to matrix */
  memcpy (matrix, solution, width * height * depth * sizeof(double));

  /* repeat until convergence or max iterations */
  while (err > EPSILON)
    {
      err = gimp_heal_laplace_iteration (matrix, height, depth, width, solution);
      memcpy (matrix, solution, width * height * depth * sizeof(double));

      num_iter++;

      if (num_iter >= MAX_ITER)
        break;
    }
}

/*
 * Algorithm Design:
 *
 * T. Georgiev, "Image Reconstruction Invariant to Relighting", EUROGRAPHICS
 * 2005, http://www.tgeorgiev.net/
 */
PixelRegion *
gimp_heal_region (PixelRegion *tempPR,
                  PixelRegion *srcPR)
{
  gdouble *i_1 = g_malloc (tempPR->h * tempPR->bytes * tempPR->w * sizeof (gdouble));
  gdouble *i_2 = g_malloc (tempPR->h * tempPR->bytes * tempPR->w * sizeof (gdouble));

  /* substitute 0's for 1's for the division and multiplication operations that
   * come later */
  gimp_heal_substitute_0_for_1 (srcPR);

  /* divide tempPR by srcPR and store the result as a double in i_1 */
  gimp_heal_divide (tempPR, srcPR, i_1);

  /* FIXME: is a faster implementation needed? */
  gimp_heal_laplace_loop (i_1, tempPR->h, tempPR->bytes, tempPR->w, i_2);

  /* multiply a double by srcPR and store in tempPR */
  gimp_heal_multiply (i_2, srcPR, tempPR);

  g_free (i_1);
  g_free (i_2);

  return tempPR;
}

static void
gimp_heal_motion (GimpPaintCore     *paint_core,
                  GimpDrawable      *drawable,
                  GimpPaintOptions  *paint_options)
{
  GimpHeal            *heal             = GIMP_HEAL (paint_core);
  GimpHealOptions     *options          = GIMP_HEAL_OPTIONS (paint_options);
  GimpContext         *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions *pressure_options = paint_options->pressure_options;
  GimpImage           *image;
  GimpImage           *src_image        = NULL;
  GimpPickable        *src_pickable     = NULL;
  TileManager         *src_tiles;
  TempBuf             *area;
  TempBuf             *temp;
  gdouble              opacity;
  PixelRegion          tempPR;
  PixelRegion          srcPR;
  PixelRegion          destPR;
  gint                 offset_x;
  gint                 offset_y;

  /* get the image */
  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /* display a warning about indexed images and return */
  if (GIMP_IMAGE_TYPE_IS_INDEXED (drawable->type))
    {
      g_message (_("Indexed images are not currently supported."));
      return;
    }

  opacity = gimp_paint_options_get_fade (paint_options, image, paint_core->pixel_dist);

  if (opacity == 0.0)
    return;

  if (! heal->src_drawable)
    return;

  /* prepare the regions to get data from */
  src_pickable = GIMP_PICKABLE (heal->src_drawable);
  src_image    = gimp_pickable_get_image (src_pickable);

  /* make local copies */
  offset_x = heal->offset_x;
  offset_y = heal->offset_y;

  /* adjust offsets and pickable if we are merging layers */
  if (options->sample_merged)
    {
      gint off_x, off_y;

      src_pickable = GIMP_PICKABLE (src_image->projection);

      gimp_item_offsets (GIMP_ITEM (heal->src_drawable), &off_x, &off_y);

      offset_x += off_x;
      offset_y += off_y;
    }

  /* get the canvas area */
  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (!area)
    return;

  /* clear the area where we want to paint */
  temp_buf_data_clear (area);

  /* get the source tiles */
  src_tiles = gimp_pickable_get_tiles (src_pickable);

  /* FIXME: the area under the cursor and the source area should be x% larger
   * than the brush size.  Otherwise the brush must be a lot bigger than the
   * area to heal to get good results.  Having the user pick such a large brush
   * is perhaps counter-intutitive? */

  /* Get the area underneath the cursor */
  {
    TempBuf  *orig;
    gint      x1, x2, y1, y2;

    x1 = CLAMP (area->x,
                0, tile_manager_width  (src_tiles));
    y1 = CLAMP (area->y,
                0, tile_manager_height (src_tiles));
    x2 = CLAMP (area->x + area->width,
                0, tile_manager_width  (src_tiles));
    y2 = CLAMP (area->y + area->height,
                0, tile_manager_height (src_tiles));

    if (! (x2 - x1) || (! (y2 - y1)))
      return;

    /*  get the original image data at the cursor location */
    if (options->sample_merged)
      orig = gimp_paint_core_get_orig_proj (paint_core,
                                            src_pickable,
                                            x1, y1, x2, y2);
    else
      orig = gimp_paint_core_get_orig_image (paint_core,
                                             GIMP_DRAWABLE (src_pickable),
                                             x1, y1, x2, y2);

    pixel_region_init_temp_buf (&srcPR, orig, 0, 0, x2 - x1, y2 - y1);
  }

  temp = temp_buf_new (srcPR.w, srcPR.h, srcPR.bytes, 0, 0, NULL);

  pixel_region_init_temp_buf (&tempPR, temp, 0, 0, srcPR.w, srcPR.h);

  copy_region (&srcPR, &tempPR);

  /* now tempPR holds the data under the cursor */

  /* get a copy of the location we will sample from */
  {
    TempBuf  *orig;
    gint      x1, x2, y1, y2;

    x1 = CLAMP (area->x + offset_x,
                0, tile_manager_width  (src_tiles));
    y1 = CLAMP (area->y + offset_y,
                0, tile_manager_height (src_tiles));
    x2 = CLAMP (area->x + offset_x + area->width,
                0, tile_manager_width  (src_tiles));
    y2 = CLAMP (area->y + offset_y + area->height,
                0, tile_manager_height (src_tiles));

    if (! (x2 - x1) || (! (y2 - y1)))
      return;

    /* get the original image data at the sample location */
    if (options->sample_merged)
      orig = gimp_paint_core_get_orig_proj (paint_core,
                                            src_pickable,
                                            x1, y1, x2, y2);
    else
      orig = gimp_paint_core_get_orig_image (paint_core,
                                             GIMP_DRAWABLE (src_pickable),
                                             x1, y1, x2, y2);

    pixel_region_init_temp_buf (&srcPR, orig, 0, 0, x2 - x1, y2 - y1);

    /* set the proper offset */
    offset_x = x1 - (area->x + offset_x);
    offset_y = y1 - (area->y + offset_y);
  }

  /* now srcPR holds the source area to sample from */

  /* get the destination to paint to */
  pixel_region_init_temp_buf (&destPR, area, offset_x, offset_y, srcPR.w, srcPR.h);

  /* FIXME: Can we ensure that this is true in the code above?
   * Is it already guaranteed to be true before we get here? */
  /* check that srcPR, tempPR, and destPR are the same size */
  if ((srcPR.w     != tempPR.w    ) || (srcPR.w     != destPR.w    ) ||
      (srcPR.h     != tempPR.h    ) || (srcPR.h     != destPR.h    ))
    {
      g_message (_("Source and destination regions are not the same size."));
      return;
    }

  /* heal tempPR using srcPR */
  gimp_heal_region (&tempPR, &srcPR);

  /* re-initialize tempPR so that it can be used within copy_region */
  pixel_region_init_data (&tempPR, tempPR.data, tempPR.bytes, tempPR.rowstride,
                          0, 0, tempPR.w, tempPR.h);

  /* add an alpha region to the area if necessary.
   * sample_merged doesn't need an alpha because its always 4 bpp */
  if ((! gimp_drawable_has_alpha (drawable)) && (! options->sample_merged))
    add_alpha_region (&tempPR, &destPR);
  else
    copy_region (&tempPR, &destPR);

  /* check the brush pressure */
  if (pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  /* replace the canvas with our healed data */
  gimp_brush_core_replace_canvas (GIMP_BRUSH_CORE (paint_core),
                                  drawable,
                                  MIN (opacity, GIMP_OPACITY_OPAQUE),
                                  gimp_context_get_opacity (context),
                                  gimp_paint_options_get_brush_mode (paint_options),
                                  GIMP_PAINT_CONSTANT);
}

static void
gimp_heal_src_drawable_removed (GimpDrawable *drawable,
                                GimpHeal     *heal)
{
  /* set the drawable to NULL */
  if (drawable == heal->src_drawable)
    {
      heal->src_drawable = NULL;
    }

  /* disconnect the signal handler for this function */
  g_signal_handlers_disconnect_by_func (drawable,
                                        gimp_heal_src_drawable_removed,
                                        heal);
}

static void
gimp_heal_set_src_drawable (GimpHeal     *heal,
                            GimpDrawable *drawable)
{
  /* check if we already have a drawable */
  if (heal->src_drawable == drawable)
    return;

  /* remove the current signal handler */
  if (heal->src_drawable)
    g_signal_handlers_disconnect_by_func (heal->src_drawable,
                                          gimp_heal_src_drawable_removed,
                                          heal);

  /* set the drawable */
  heal->src_drawable = drawable;

  /* connect the signal handler that handles the "remove" signal */
  if (heal->src_drawable)
    g_signal_connect (heal->src_drawable, "removed",
                      G_CALLBACK (gimp_heal_src_drawable_removed),
                      heal);

  /* notify heal that the source drawable is set */
  g_object_notify (G_OBJECT (heal), "src-drawable");
}
