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

#include <string.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpcontext.h"
#include "gimpgradient.h"
#include "gimpgradient-load.h"
#include "gimpgradient-save.h"


#define EPSILON 1e-10


static void       gimp_gradient_finalize         (GObject           *object);

static gint64     gimp_gradient_get_memsize      (GimpObject        *object,
                                                  gint64            *gui_size);

static void       gimp_gradient_get_preview_size (GimpViewable      *viewable,
                                                  gint               size,
                                                  gboolean           popup,
                                                  gboolean           dot_for_dot,
                                                  gint              *width,
                                                  gint              *height);
static gboolean   gimp_gradient_get_popup_size   (GimpViewable      *viewable,
                                                  gint               width,
                                                  gint               height,
                                                  gboolean           dot_for_dot,
                                                  gint              *popup_width,
                                                  gint              *popup_height);
static TempBuf  * gimp_gradient_get_new_preview  (GimpViewable      *viewable,
                                                  GimpContext       *context,
                                                  gint               width,
                                                  gint               height);
static gchar    * gimp_gradient_get_extension    (GimpData          *data);
static GimpData * gimp_gradient_duplicate        (GimpData          *data);

static GimpGradientSegment *
           gimp_gradient_get_segment_at_internal (GimpGradient        *gradient,
                                                  GimpGradientSegment *seg,
                                                  gdouble              pos);


static inline gdouble  gimp_gradient_calc_linear_factor            (gdouble  middle,
                                                                    gdouble  pos);
static inline gdouble  gimp_gradient_calc_curved_factor            (gdouble  middle,
                                                                    gdouble  pos);
static inline gdouble  gimp_gradient_calc_sine_factor              (gdouble  middle,
                                                                    gdouble  pos);
static inline gdouble  gimp_gradient_calc_sphere_increasing_factor (gdouble  middle,
                                                                    gdouble  pos);
static inline gdouble  gimp_gradient_calc_sphere_decreasing_factor (gdouble  middle,
                                                                    gdouble  pos);


G_DEFINE_TYPE (GimpGradient, gimp_gradient, GIMP_TYPE_DATA)

#define parent_class gimp_gradient_parent_class


static void
gimp_gradient_class_init (GimpGradientClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  object_class->finalize           = gimp_gradient_finalize;

  gimp_object_class->get_memsize   = gimp_gradient_get_memsize;

  viewable_class->default_stock_id = "gimp-gradient";
  viewable_class->get_preview_size = gimp_gradient_get_preview_size;
  viewable_class->get_popup_size   = gimp_gradient_get_popup_size;
  viewable_class->get_new_preview  = gimp_gradient_get_new_preview;

  data_class->save                 = gimp_gradient_save;
  data_class->get_extension        = gimp_gradient_get_extension;
  data_class->duplicate            = gimp_gradient_duplicate;
}

static void
gimp_gradient_init (GimpGradient *gradient)
{
  gradient->segments = NULL;
}

static void
gimp_gradient_finalize (GObject *object)
{
  GimpGradient *gradient = GIMP_GRADIENT (object);

  if (gradient->segments)
    {
      gimp_gradient_segments_free (gradient->segments);
      gradient->segments = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_gradient_get_memsize (GimpObject *object,
                           gint64     *gui_size)
{
  GimpGradient        *gradient = GIMP_GRADIENT (object);
  GimpGradientSegment *segment;
  gint64               memsize  = 0;

  for (segment = gradient->segments; segment; segment = segment->next)
    memsize += sizeof (GimpGradientSegment);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_gradient_get_preview_size (GimpViewable *viewable,
                                gint          size,
                                gboolean      popup,
                                gboolean      dot_for_dot,
                                gint         *width,
                                gint         *height)
{
  *width  = size;
  *height = size / 2;
}

static gboolean
gimp_gradient_get_popup_size (GimpViewable *viewable,
                              gint          width,
                              gint          height,
                              gboolean      dot_for_dot,
                              gint         *popup_width,
                              gint         *popup_height)
{
  if (width < 128 || height < 32)
    {
      *popup_width  = 128;
      *popup_height =  32;

      return TRUE;
    }

  return FALSE;
}

static TempBuf *
gimp_gradient_get_new_preview (GimpViewable *viewable,
                               GimpContext  *context,
                               gint          width,
                               gint          height)
{
  GimpGradient        *gradient = GIMP_GRADIENT (viewable);
  GimpGradientSegment *seg      = NULL;
  TempBuf             *temp_buf;
  guchar              *buf;
  guchar              *p;
  guchar              *row;
  gint                 x, y;
  gdouble              dx, cur_x;
  GimpRGB              color;

  dx    = 1.0 / (width - 1);
  cur_x = 0.0;
  p     = row = g_malloc (width * 4);

  /* Create lines to fill the image */

  for (x = 0; x < width; x++)
    {
      seg = gimp_gradient_get_color_at (gradient, context, seg, cur_x,
                                        FALSE, &color);

      *p++ = color.r * 255.0;
      *p++ = color.g * 255.0;
      *p++ = color.b * 255.0;
      *p++ = color.a * 255.0;

      cur_x += dx;
    }

  temp_buf = temp_buf_new (width, height, 4, 0, 0, NULL);

  buf = temp_buf_data (temp_buf);

  for (y = 0; y < height; y++)
    memcpy (buf + (width * y * 4), row, width * 4);

  g_free (row);

  return temp_buf;
}

static GimpData *
gimp_gradient_duplicate (GimpData *data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *head, *prev, *cur, *orig;

  gradient = g_object_new (GIMP_TYPE_GRADIENT, NULL);

  prev = NULL;
  orig = GIMP_GRADIENT (data)->segments;
  head = NULL;

  while (orig)
    {
      cur = gimp_gradient_segment_new ();

      *cur = *orig;  /* Copy everything */

      cur->prev = prev;
      cur->next = NULL;

      if (prev)
        prev->next = cur;
      else
        head = cur;  /* Remember head */

      prev = cur;
      orig = orig->next;
    }

  gradient->segments = head;

  return GIMP_DATA (gradient);
}


/*  public functions  */

GimpData *
gimp_gradient_new (const gchar *name)
{
  GimpGradient *gradient;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  gradient = g_object_new (GIMP_TYPE_GRADIENT,
                           "name", name,
                           NULL);

  gradient->segments = gimp_gradient_segment_new ();

  return GIMP_DATA (gradient);
}

GimpData *
gimp_gradient_get_standard (void)
{
  static GimpData *standard_gradient = NULL;

  if (! standard_gradient)
    {
      standard_gradient = gimp_gradient_new ("Standard");

      standard_gradient->dirty = FALSE;
      gimp_data_make_internal (standard_gradient);

      g_object_ref (standard_gradient);
    }

  return standard_gradient;
}

static gchar *
gimp_gradient_get_extension (GimpData *data)
{
  return GIMP_GRADIENT_FILE_EXTENSION;
}

/**
 * gimp_gradient_get_color_at:
 * @gradient: a gradient
 * @context:  a context
 * @seg:      a segment to seed the search with (or %NULL)
 * @pos:      position in the gradient (between 0.0 and 1.0)
 * @reverse:  when %TRUE, use the reversed gradient
 * @color:    returns the color
 *
 * If you are iterating over an gradient, you should pass the the
 * return value from the last call for @seg.
 *
 * Return value: the gradient segment the color is from
 **/
GimpGradientSegment *
gimp_gradient_get_color_at (GimpGradient        *gradient,
                            GimpContext         *context,
                            GimpGradientSegment *seg,
                            gdouble              pos,
                            gboolean             reverse,
                            GimpRGB             *color)
{
  gdouble  factor = 0.0;
  gdouble  seg_len;
  gdouble  middle;
  GimpRGB  left_color;
  GimpRGB  right_color;
  GimpRGB  rgb;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  pos = CLAMP (pos, 0.0, 1.0);

  if (reverse)
    pos = 1.0 - pos;

  seg = gimp_gradient_get_segment_at_internal (gradient, seg, pos);

  seg_len = seg->right - seg->left;

  if (seg_len < EPSILON)
    {
      middle = 0.5;
      pos    = 0.5;
    }
  else
    {
      middle = (seg->middle - seg->left) / seg_len;
      pos    = (pos - seg->left) / seg_len;
    }

  switch (seg->type)
    {
    case GIMP_GRADIENT_SEGMENT_LINEAR:
      factor = gimp_gradient_calc_linear_factor (middle, pos);
      break;

    case GIMP_GRADIENT_SEGMENT_CURVED:
      factor = gimp_gradient_calc_curved_factor (middle, pos);
      break;

    case GIMP_GRADIENT_SEGMENT_SINE:
      factor = gimp_gradient_calc_sine_factor (middle, pos);
      break;

    case GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING:
      factor = gimp_gradient_calc_sphere_increasing_factor (middle, pos);
      break;

    case GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING:
      factor = gimp_gradient_calc_sphere_decreasing_factor (middle, pos);
      break;

    default:
      g_warning ("%s: Unknown gradient type %d.", G_STRFUNC, seg->type);
      break;
    }

  /* Get left/right colors */

  switch (seg->left_color_type)
    {
    case GIMP_GRADIENT_COLOR_FIXED:
      left_color = seg->left_color;
      break;

    case GIMP_GRADIENT_COLOR_FOREGROUND:
    case GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT:
      gimp_context_get_foreground (context, &left_color);

      if (seg->left_color_type == GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT)
        gimp_rgb_set_alpha (&left_color, 0.0);
      break;

    case GIMP_GRADIENT_COLOR_BACKGROUND:
    case GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT:
      gimp_context_get_background (context, &left_color);

      if (seg->left_color_type == GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT)
        gimp_rgb_set_alpha (&left_color, 0.0);
      break;
    }

  switch (seg->right_color_type)
    {
    case GIMP_GRADIENT_COLOR_FIXED:
      right_color = seg->right_color;
      break;

    case GIMP_GRADIENT_COLOR_FOREGROUND:
    case GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT:
      gimp_context_get_foreground (context, &right_color);

      if (seg->right_color_type == GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT)
        gimp_rgb_set_alpha (&right_color, 0.0);
      break;

    case GIMP_GRADIENT_COLOR_BACKGROUND:
    case GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT:
      gimp_context_get_background (context, &right_color);

      if (seg->right_color_type == GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT)
        gimp_rgb_set_alpha (&right_color, 0.0);
      break;
    }

  /* Calculate color components */

  if (seg->color == GIMP_GRADIENT_SEGMENT_RGB)
    {
      rgb.r = left_color.r + (right_color.r - left_color.r) * factor;
      rgb.g = left_color.g + (right_color.g - left_color.g) * factor;
      rgb.b = left_color.b + (right_color.b - left_color.b) * factor;
    }
  else
    {
      GimpHSV left_hsv;
      GimpHSV right_hsv;

      gimp_rgb_to_hsv (&left_color,  &left_hsv);
      gimp_rgb_to_hsv (&right_color, &right_hsv);

      left_hsv.s = left_hsv.s + (right_hsv.s - left_hsv.s) * factor;
      left_hsv.v = left_hsv.v + (right_hsv.v - left_hsv.v) * factor;

      switch (seg->color)
        {
        case GIMP_GRADIENT_SEGMENT_HSV_CCW:
          if (left_hsv.h < right_hsv.h)
            {
              left_hsv.h += (right_hsv.h - left_hsv.h) * factor;
            }
          else
            {
              left_hsv.h += (1.0 - (left_hsv.h - right_hsv.h)) * factor;

              if (left_hsv.h > 1.0)
                left_hsv.h -= 1.0;
            }
          break;

        case GIMP_GRADIENT_SEGMENT_HSV_CW:
          if (right_hsv.h < left_hsv.h)
            {
              left_hsv.h -= (left_hsv.h - right_hsv.h) * factor;
            }
          else
            {
              left_hsv.h -= (1.0 - (right_hsv.h - left_hsv.h)) * factor;

              if (left_hsv.h < 0.0)
                left_hsv.h += 1.0;
            }
          break;

        default:
          g_warning ("%s: Unknown coloring mode %d",
                     G_STRFUNC, (gint) seg->color);
          break;
        }

      gimp_hsv_to_rgb (&left_hsv, &rgb);
    }

  /* Calculate alpha */

  rgb.a = left_color.a + (right_color.a - left_color.a) * factor;

  *color = rgb;

  return seg;
}

GimpGradientSegment *
gimp_gradient_get_segment_at (GimpGradient *gradient,
                              gdouble       pos)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);

  return gimp_gradient_get_segment_at_internal (gradient, NULL, pos);
}

gboolean
gimp_gradient_has_fg_bg_segments (GimpGradient *gradient)
{
  GimpGradientSegment *segment;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), FALSE);

  for (segment = gradient->segments; segment; segment = segment->next)
    if (segment->left_color_type  != GIMP_GRADIENT_COLOR_FIXED ||
        segment->right_color_type != GIMP_GRADIENT_COLOR_FIXED)
      return TRUE;

  return FALSE;
}

GimpGradient *
gimp_gradient_flatten (GimpGradient *gradient,
                       GimpContext  *context)
{
  GimpGradient        *flat;
  GimpGradientSegment *seg;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  flat = GIMP_GRADIENT (gimp_data_duplicate (GIMP_DATA (gradient)));

  for (seg = flat->segments; seg; seg = seg->next)
    {
      switch (seg->left_color_type)
        {
        case GIMP_GRADIENT_COLOR_FIXED:
          break;

        case GIMP_GRADIENT_COLOR_FOREGROUND:
        case GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT:
          gimp_context_get_foreground (context, &seg->left_color);

          if (seg->left_color_type == GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT)
            gimp_rgb_set_alpha (&seg->left_color, 0.0);
          break;

        case GIMP_GRADIENT_COLOR_BACKGROUND:
        case GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT:
          gimp_context_get_background (context, &seg->left_color);

          if (seg->left_color_type == GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT)
            gimp_rgb_set_alpha (&seg->left_color, 0.0);
          break;
        }

      seg->left_color_type = GIMP_GRADIENT_COLOR_FIXED;

      switch (seg->right_color_type)
        {
        case GIMP_GRADIENT_COLOR_FIXED:
          break;

        case GIMP_GRADIENT_COLOR_FOREGROUND:
        case GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT:
          gimp_context_get_foreground (context, &seg->right_color);

          if (seg->right_color_type == GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT)
            gimp_rgb_set_alpha (&seg->right_color, 0.0);
          break;

        case GIMP_GRADIENT_COLOR_BACKGROUND:
        case GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT:
          gimp_context_get_background (context, &seg->right_color);

          if (seg->right_color_type == GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT)
            gimp_rgb_set_alpha (&seg->right_color, 0.0);
          break;
        }

      seg->right_color_type = GIMP_GRADIENT_COLOR_FIXED;
    }

  return flat;
}


/*  gradient segment functions  */

GimpGradientSegment *
gimp_gradient_segment_new (void)
{
  GimpGradientSegment *seg;

  seg = g_slice_new (GimpGradientSegment);

  seg->left   = 0.0;
  seg->middle = 0.5;
  seg->right  = 1.0;

  seg->left_color_type = GIMP_GRADIENT_COLOR_FIXED;
  gimp_rgba_set (&seg->left_color,  0.0, 0.0, 0.0, 1.0);

  seg->right_color_type = GIMP_GRADIENT_COLOR_FIXED;
  gimp_rgba_set (&seg->right_color, 1.0, 1.0, 1.0, 1.0);

  seg->type  = GIMP_GRADIENT_SEGMENT_LINEAR;
  seg->color = GIMP_GRADIENT_SEGMENT_RGB;

  seg->prev = seg->next = NULL;

  return seg;
}


void
gimp_gradient_segment_free (GimpGradientSegment *seg)
{
  g_return_if_fail (seg != NULL);

  g_slice_free (GimpGradientSegment, seg);
}

void
gimp_gradient_segments_free (GimpGradientSegment *seg)
{
  g_return_if_fail (seg != NULL);

  g_slice_free_chain (GimpGradientSegment, seg, next);
}

GimpGradientSegment *
gimp_gradient_segment_get_last (GimpGradientSegment *seg)
{
  if (! seg)
    return NULL;

  while (seg->next)
    seg = seg->next;

  return seg;
}

GimpGradientSegment *
gimp_gradient_segment_get_first (GimpGradientSegment *seg)
{
  if (! seg)
    return NULL;

  while (seg->prev)
    seg = seg->prev;

  return seg;
}

GimpGradientSegment *
gimp_gradient_segment_get_nth (GimpGradientSegment *seg,
                               gint                 index)
{
  gint i = 0;

  g_return_val_if_fail (index >= 0, NULL);

  if (! seg)
    return NULL;

  while (seg && (i < index))
    {
      seg = seg->next;
      i++;
    }

  if (i == index)
    return seg;

  return NULL;
}

void
gimp_gradient_segment_split_midpoint (GimpGradient         *gradient,
                                      GimpContext          *context,
                                      GimpGradientSegment  *lseg,
                                      GimpGradientSegment **newl,
                                      GimpGradientSegment **newr)
{
  GimpRGB              color;
  GimpGradientSegment *newseg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (lseg != NULL);
  g_return_if_fail (newl != NULL);
  g_return_if_fail (newr != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  /* Get color at original segment's midpoint */
  gimp_gradient_get_color_at (gradient, context, lseg, lseg->middle,
                              FALSE, &color);

  /* Create a new segment and insert it in the list */

  newseg = gimp_gradient_segment_new ();

  newseg->prev = lseg;
  newseg->next = lseg->next;

  lseg->next = newseg;

  if (newseg->next)
    newseg->next->prev = newseg;

  /* Set coordinates of new segment */

  newseg->left   = lseg->middle;
  newseg->right  = lseg->right;
  newseg->middle = (newseg->left + newseg->right) / 2.0;

  /* Set coordinates of original segment */

  lseg->right  = newseg->left;
  lseg->middle = (lseg->left + lseg->right) / 2.0;

  /* Set colors of both segments */

  newseg->right_color_type = lseg->right_color_type;
  newseg->right_color      = lseg->right_color;

  lseg->right_color_type = newseg->left_color_type = GIMP_GRADIENT_COLOR_FIXED;
  lseg->right_color      = newseg->left_color      = color;

  /* Set parameters of new segment */

  newseg->type  = lseg->type;
  newseg->color = lseg->color;

  /* Done */

  *newl = lseg;
  *newr = newseg;

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_split_uniform (GimpGradient         *gradient,
                                     GimpContext          *context,
                                     GimpGradientSegment  *lseg,
                                     gint                  parts,
                                     GimpGradientSegment **newl,
                                     GimpGradientSegment **newr)
{
  GimpGradientSegment *seg, *prev, *tmp;
  gdouble              seg_len;
  gint                 i;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (lseg != NULL);
  g_return_if_fail (newl != NULL);
  g_return_if_fail (newr != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  seg_len = (lseg->right - lseg->left) / parts; /* Length of divisions */

  seg  = NULL;
  prev = NULL;
  tmp  = NULL;

  for (i = 0; i < parts; i++)
    {
      seg = gimp_gradient_segment_new ();

      if (i == 0)
        tmp = seg; /* Remember first segment */

      seg->left   = lseg->left + i * seg_len;
      seg->right  = lseg->left + (i + 1) * seg_len;
      seg->middle = (seg->left + seg->right) / 2.0;

      seg->left_color_type  = GIMP_GRADIENT_COLOR_FIXED;
      seg->right_color_type = GIMP_GRADIENT_COLOR_FIXED;

      gimp_gradient_get_color_at (gradient, context, lseg,
                                  seg->left,  FALSE, &seg->left_color);
      gimp_gradient_get_color_at (gradient, context, lseg,
                                  seg->right, FALSE, &seg->right_color);

      seg->type  = lseg->type;
      seg->color = lseg->color;

      seg->prev = prev;
      seg->next = NULL;

      if (prev)
        prev->next = seg;

      prev = seg;
    }

  /* Fix edges */

  tmp->left_color_type = lseg->left_color_type;
  tmp->left_color      = lseg->left_color;

  seg->right_color_type = lseg->right_color_type;
  seg->right_color      = lseg->right_color;

  tmp->left  = lseg->left;
  seg->right = lseg->right; /* To squish accumulative error */

  /* Link in list */

  tmp->prev = lseg->prev;
  seg->next = lseg->next;

  if (lseg->prev)
    lseg->prev->next = tmp;
  else
    gradient->segments = tmp; /* We are on leftmost segment */

  if (lseg->next)
    lseg->next->prev = seg;

  /* Done */
  *newl = tmp;
  *newr = seg;

  /* Delete old segment */
  gimp_gradient_segment_free (lseg);

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_get_left_color (GimpGradient        *gradient,
                                      GimpGradientSegment *seg,
                                      GimpRGB             *color)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (seg != NULL);
  g_return_if_fail (color != NULL);

  *color = seg->left_color;
}

void
gimp_gradient_segment_set_left_color (GimpGradient        *gradient,
                                      GimpGradientSegment *seg,
                                      const GimpRGB       *color)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (seg != NULL);
  g_return_if_fail (color != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_range_blend (gradient, seg, seg,
                                     color, &seg->right_color,
                                     TRUE, TRUE);

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_get_right_color (GimpGradient        *gradient,
                                       GimpGradientSegment *seg,
                                       GimpRGB             *color)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (seg != NULL);
  g_return_if_fail (color != NULL);

  *color = seg->right_color;
}

void
gimp_gradient_segment_set_right_color (GimpGradient        *gradient,
                                       GimpGradientSegment *seg,
                                       const GimpRGB       *color)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (seg != NULL);
  g_return_if_fail (color != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_range_blend (gradient, seg, seg,
                                     &seg->left_color, color,
                                     TRUE, TRUE);

  gimp_data_thaw (GIMP_DATA (gradient));
}

GimpGradientColor
gimp_gradient_segment_get_left_color_type (GimpGradient        *gradient,
                                           GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0);
  g_return_val_if_fail (seg != NULL, 0);

  return seg->left_color_type;
}

void
gimp_gradient_segment_set_left_color_type (GimpGradient        *gradient,
                                           GimpGradientSegment *seg,
                                           GimpGradientColor    color_type)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (seg != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  seg->left_color_type = color_type;

  gimp_data_thaw (GIMP_DATA (gradient));
}

GimpGradientColor
gimp_gradient_segment_get_right_color_type (GimpGradient        *gradient,
                                            GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0);
  g_return_val_if_fail (seg != NULL, 0);

  return seg->right_color_type;
}

void
gimp_gradient_segment_set_right_color_type (GimpGradient        *gradient,
                                            GimpGradientSegment *seg,
                                            GimpGradientColor    color_type)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (seg != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  seg->right_color_type = color_type;

  gimp_data_thaw (GIMP_DATA (gradient));
}

gdouble
gimp_gradient_segment_get_left_pos (GimpGradient        *gradient,
                                    GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0.0);
  g_return_val_if_fail (seg != NULL, 0.0);

  return seg->left;
}

gdouble
gimp_gradient_segment_set_left_pos (GimpGradient        *gradient,
                                    GimpGradientSegment *seg,
                                    gdouble              pos)
{
  gdouble final_pos;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0.0);
  g_return_val_if_fail (seg != NULL, 0.0);

  if (seg->prev == NULL)
    {
      final_pos = 0;
    }
  else
    {
      gimp_data_freeze (GIMP_DATA (gradient));

      final_pos = seg->prev->right = seg->left =
          CLAMP (pos,
                 seg->prev->middle + EPSILON,
                 seg->middle - EPSILON);

      gimp_data_thaw (GIMP_DATA (gradient));
    }

  return final_pos;
}

gdouble
gimp_gradient_segment_get_right_pos (GimpGradient        *gradient,
                                     GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0.0);
  g_return_val_if_fail (seg != NULL, 0.0);

  return seg->right;
}

gdouble
gimp_gradient_segment_set_right_pos (GimpGradient        *gradient,
                                     GimpGradientSegment *seg,
                                     gdouble              pos)
{
  gdouble final_pos;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0.0);
  g_return_val_if_fail (seg != NULL, 0.0);

  if (seg->next == NULL)
    {
      final_pos = 1.0;
    }
  else
    {
      gimp_data_freeze (GIMP_DATA (gradient));

      final_pos = seg->next->left = seg->right =
          CLAMP (pos,
                 seg->middle + EPSILON,
                 seg->next->middle - EPSILON);

      gimp_data_thaw (GIMP_DATA (gradient));
    }

  return final_pos;
}

gdouble
gimp_gradient_segment_get_middle_pos (GimpGradient        *gradient,
                                      GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0.0);
  g_return_val_if_fail (seg != NULL, 0.0);

  return seg->middle;
}

gdouble
gimp_gradient_segment_set_middle_pos (GimpGradient        *gradient,
                                      GimpGradientSegment *seg,
                                      gdouble              pos)
{
  gdouble final_pos;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0.0);
  g_return_val_if_fail (seg != NULL, 0.0);

  gimp_data_freeze (GIMP_DATA (gradient));

  final_pos = seg->middle =
      CLAMP (pos,
             seg->left + EPSILON,
             seg->right - EPSILON);

  gimp_data_thaw (GIMP_DATA (gradient));

  return final_pos;
}

GimpGradientSegmentType
gimp_gradient_segment_get_blending_function (GimpGradient        *gradient,
                                             GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0);

  return seg->type;
}

GimpGradientSegmentColor
gimp_gradient_segment_get_coloring_type (GimpGradient        *gradient,
                                         GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0);

  return seg->color;
}

void
gimp_gradient_segment_range_compress (GimpGradient        *gradient,
                                      GimpGradientSegment *range_l,
                                      GimpGradientSegment *range_r,
                                      gdouble              new_l,
                                      gdouble              new_r)
{
  gdouble              orig_l, orig_r;
  gdouble              scale;
  GimpGradientSegment *seg, *aseg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (range_l != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! range_r)
    range_r = gimp_gradient_segment_get_last (range_l);

  orig_l = range_l->left;
  orig_r = range_r->right;

  scale = (new_r - new_l) / (orig_r - orig_l);

  seg = range_l;

  do
    {
      if (seg->prev)
        seg->left   = new_l + (seg->left - orig_l) * scale;
      seg->middle = new_l + (seg->middle - orig_l) * scale;
      if (seg->next)
        seg->right  = new_l + (seg->right - orig_l) * scale;

      /* Next */

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != range_r);

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_blend (GimpGradient        *gradient,
                                   GimpGradientSegment *lseg,
                                   GimpGradientSegment *rseg,
                                   const GimpRGB       *rgb1,
                                   const GimpRGB       *rgb2,
                                   gboolean             blend_colors,
                                   gboolean             blend_opacity)
{
  GimpRGB              d;
  gdouble              left, len;
  GimpGradientSegment *seg;
  GimpGradientSegment *aseg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (lseg != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! rseg)
    rseg = gimp_gradient_segment_get_last (lseg);

  d.r = rgb2->r - rgb1->r;
  d.g = rgb2->g - rgb1->g;
  d.b = rgb2->b - rgb1->b;
  d.a = rgb2->a - rgb1->a;

  left  = lseg->left;
  len   = rseg->right - left;

  seg = lseg;

  do
    {
      if (blend_colors)
        {
          seg->left_color.r  = rgb1->r + (seg->left - left) / len * d.r;
          seg->left_color.g  = rgb1->g + (seg->left - left) / len * d.g;
          seg->left_color.b  = rgb1->b + (seg->left - left) / len * d.b;

          seg->right_color.r = rgb1->r + (seg->right - left) / len * d.r;
          seg->right_color.g = rgb1->g + (seg->right - left) / len * d.g;
          seg->right_color.b = rgb1->b + (seg->right - left) / len * d.b;
        }

      if (blend_opacity)
        {
          seg->left_color.a  = rgb1->a + (seg->left - left) / len * d.a;
          seg->right_color.a = rgb1->a + (seg->right - left) / len * d.a;
        }

      aseg = seg;
      seg = seg->next;
    }
  while (aseg != rseg);
  gimp_data_thaw (GIMP_DATA (gradient));

}

void
gimp_gradient_segment_range_set_blending_function (GimpGradient            *gradient,
                                                   GimpGradientSegment     *start_seg,
                                                   GimpGradientSegment     *end_seg,
                                                   GimpGradientSegmentType  new_type)
{
  GimpGradientSegment *seg;
  gboolean             reached_last_segment = FALSE;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  gimp_data_freeze (GIMP_DATA (gradient));

  seg = start_seg;
  while (seg && ! reached_last_segment)
    {
      if (seg == end_seg)
        reached_last_segment = TRUE;

      seg->type = new_type;
      seg = seg->next;
    }

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_set_coloring_type (GimpGradient             *gradient,
                                               GimpGradientSegment      *start_seg,
                                               GimpGradientSegment      *end_seg,
                                               GimpGradientSegmentColor  new_color)
{
  GimpGradientSegment *seg;
  gboolean             reached_last_segment = FALSE;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  gimp_data_freeze (GIMP_DATA (gradient));

  seg = start_seg;
  while (seg && ! reached_last_segment)
    {
      if (seg == end_seg)
        reached_last_segment = TRUE;

      seg->color = new_color;
      seg = seg->next;
    }

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_flip (GimpGradient         *gradient,
                                  GimpGradientSegment  *start_seg,
                                  GimpGradientSegment  *end_seg,
                                  GimpGradientSegment **final_start_seg,
                                  GimpGradientSegment **final_end_seg)
{
  GimpGradientSegment *oseg, *oaseg;
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *lseg, *rseg;
  gdouble              left, right;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! end_seg)
    end_seg = gimp_gradient_segment_get_last (start_seg);

  left  = start_seg->left;
  right = end_seg->right;

  /* Build flipped segments */

  prev = NULL;
  oseg = end_seg;
  tmp  = NULL;

  do
    {
      seg = gimp_gradient_segment_new ();

      if (prev == NULL)
        {
          seg->left = left;
          tmp = seg; /* Remember first segment */
        }
      else
        seg->left = left + right - oseg->right;

      seg->middle = left + right - oseg->middle;
      seg->right  = left + right - oseg->left;

      seg->left_color_type = oseg->right_color_type;
      seg->left_color      = oseg->right_color;

      seg->right_color_type = oseg->left_color_type;
      seg->right_color      = oseg->left_color;

      switch (oseg->type)
        {
        case GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING:
          seg->type = GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING;
          break;

        case GIMP_GRADIENT_SEGMENT_SPHERE_DECREASING:
          seg->type = GIMP_GRADIENT_SEGMENT_SPHERE_INCREASING;
          break;

        default:
          seg->type = oseg->type;
        }

      switch (oseg->color)
        {
        case GIMP_GRADIENT_SEGMENT_HSV_CCW:
          seg->color = GIMP_GRADIENT_SEGMENT_HSV_CW;
          break;

        case GIMP_GRADIENT_SEGMENT_HSV_CW:
          seg->color = GIMP_GRADIENT_SEGMENT_HSV_CCW;
          break;

        default:
          seg->color = oseg->color;
        }

      seg->prev = prev;
      seg->next = NULL;

      if (prev)
        prev->next = seg;

      prev = seg;

      oaseg = oseg;
      oseg  = oseg->prev; /* Move backwards! */
    }
  while (oaseg != start_seg);

  seg->right = right; /* Squish accumulative error */

  /* Free old segments */

  lseg = start_seg->prev;
  rseg = end_seg->next;

  oseg = start_seg;

  do
    {
      oaseg = oseg->next;
      gimp_gradient_segment_free (oseg);
      oseg = oaseg;
    }
  while (oaseg != rseg);

  /* Link in new segments */

  if (lseg)
    lseg->next = tmp;
  else
    gradient->segments = tmp;

  tmp->prev = lseg;

  seg->next = rseg;

  if (rseg)
    rseg->prev = seg;

  /* Reset selection */

  if (final_start_seg)
    *final_start_seg = tmp;

  if (final_end_seg)
    *final_end_seg = seg;

  /* Done */

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_replicate (GimpGradient         *gradient,
                                       GimpGradientSegment  *start_seg,
                                       GimpGradientSegment  *end_seg,
                                       gint                  replicate_times,
                                       GimpGradientSegment **final_start_seg,
                                       GimpGradientSegment **final_end_seg)
{
  gdouble              sel_left, sel_right, sel_len;
  gdouble              new_left;
  gdouble              factor;
  GimpGradientSegment *prev, *seg, *tmp;
  GimpGradientSegment *oseg, *oaseg;
  GimpGradientSegment *lseg, *rseg;
  gint                 i;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  if (! end_seg)
    end_seg = gimp_gradient_segment_get_last (start_seg);

  if (replicate_times < 2)
    {
      *final_start_seg = start_seg;
      *final_end_seg   = end_seg;
      return;
    }

  gimp_data_freeze (GIMP_DATA (gradient));

  /* Remember original parameters */
  sel_left  = start_seg->left;
  sel_right = end_seg->right;
  sel_len   = sel_right - sel_left;

  factor = 1.0 / replicate_times;

  /* Build replicated segments */

  prev = NULL;
  seg  = NULL;
  tmp  = NULL;

  for (i = 0; i < replicate_times; i++)
    {
      /* Build one cycle */

      new_left  = sel_left + i * factor * sel_len;

      oseg = start_seg;

      do
        {
          seg = gimp_gradient_segment_new ();

          if (prev == NULL)
            {
              seg->left = sel_left;
              tmp = seg; /* Remember first segment */
            }
          else
            {
              seg->left = new_left + factor * (oseg->left - sel_left);
            }

          seg->middle = new_left + factor * (oseg->middle - sel_left);
          seg->right  = new_left + factor * (oseg->right - sel_left);

          seg->left_color_type = oseg->left_color_type;
          seg->left_color      = oseg->left_color;

          seg->right_color_type = oseg->right_color_type;
          seg->right_color      = oseg->right_color;

          seg->type  = oseg->type;
          seg->color = oseg->color;

          seg->prev = prev;
          seg->next = NULL;

          if (prev)
            prev->next = seg;

          prev = seg;

          oaseg = oseg;
          oseg  = oseg->next;
        }
      while (oaseg != end_seg);
    }

  seg->right = sel_right; /* Squish accumulative error */

  /* Free old segments */

  lseg = start_seg->prev;
  rseg = end_seg->next;

  oseg = start_seg;

  do
    {
      oaseg = oseg->next;
      gimp_gradient_segment_free (oseg);
      oseg = oaseg;
    }
  while (oaseg != rseg);

  /* Link in new segments */

  if (lseg)
    lseg->next = tmp;
  else
    gradient->segments = tmp;

  tmp->prev = lseg;

  seg->next = rseg;

  if (rseg)
    rseg->prev = seg;

  /* Reset selection */

  if (final_start_seg)
    *final_start_seg = tmp;

  if (final_end_seg)
    *final_end_seg = seg;

  /* Done */

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_split_midpoint (GimpGradient         *gradient,
                                            GimpContext          *context,
                                            GimpGradientSegment  *start_seg,
                                            GimpGradientSegment  *end_seg,
                                            GimpGradientSegment **final_start_seg,
                                            GimpGradientSegment **final_end_seg)
{
  GimpGradientSegment *seg, *lseg, *rseg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! end_seg)
    end_seg = gimp_gradient_segment_get_last (start_seg);

  seg = start_seg;

  do
    {
      gimp_gradient_segment_split_midpoint (gradient, context,
                                            seg, &lseg, &rseg);
      seg = rseg->next;
    }
  while (lseg != end_seg);

  if (final_start_seg)
    *final_start_seg = start_seg;

  if (final_end_seg)
    *final_end_seg = rseg;

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_split_uniform (GimpGradient         *gradient,
                                           GimpContext          *context,
                                           GimpGradientSegment  *start_seg,
                                           GimpGradientSegment  *end_seg,
                                           gint                  parts,
                                           GimpGradientSegment **final_start_seg,
                                           GimpGradientSegment **final_end_seg)
{
  GimpGradientSegment *seg, *aseg, *lseg, *rseg, *lsel;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (! end_seg)
    end_seg = gimp_gradient_segment_get_last (start_seg);

  if (parts < 2)
    {
      *final_start_seg = start_seg;
      *final_end_seg   = end_seg;
      return;
    }

  gimp_data_freeze (GIMP_DATA (gradient));

  seg = start_seg;
  lsel = NULL;

  do
    {
      aseg = seg;

      gimp_gradient_segment_split_uniform (gradient, context, seg,
                                           parts,
                                           &lseg, &rseg);

      if (seg == start_seg)
        lsel = lseg;

      seg = rseg->next;
    }
  while (aseg != end_seg);

  if (final_start_seg)
    *final_start_seg = lsel;

  if (final_end_seg)
    *final_end_seg = rseg;

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_delete (GimpGradient         *gradient,
                                    GimpGradientSegment  *start_seg,
                                    GimpGradientSegment  *end_seg,
                                    GimpGradientSegment **final_start_seg,
                                    GimpGradientSegment **final_end_seg)
{
  GimpGradientSegment *lseg, *rseg, *seg, *aseg, *next;
  gdouble              join;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  if (! end_seg)
    end_seg = gimp_gradient_segment_get_last (start_seg);

  /* Remember segments to the left and to the right of the selection */

  lseg = start_seg->prev;
  rseg = end_seg->next;

  /* Cannot delete all the segments in the gradient */

  if ((lseg == NULL) && (rseg == NULL))
    goto premature_return;

  gimp_data_freeze (GIMP_DATA (gradient));

  /* Calculate join point */

  join = (start_seg->left +
          end_seg->right) / 2.0;

  if (lseg == NULL)
    join = 0.0;
  else if (rseg == NULL)
    join = 1.0;

  /* Move segments */

  if (lseg != NULL)
    gimp_gradient_segment_range_compress (gradient, lseg, lseg,
                                          lseg->left, join);

  if (rseg != NULL)
    gimp_gradient_segment_range_compress (gradient, rseg, rseg,
                                          join, rseg->right);

  /* Link */

  if (lseg)
    lseg->next = rseg;

  if (rseg)
    rseg->prev = lseg;

  /* Delete old segments */

  seg = start_seg;

  do
    {
      next = seg->next;
      aseg = seg;

      gimp_gradient_segment_free (seg);

      seg = next;
    }
  while (aseg != end_seg);

  /* Change selection */

  if (rseg)
    {
      if (final_start_seg)
        *final_start_seg = rseg;

      if (final_end_seg)
        *final_end_seg = rseg;
    }
  else
    {
      if (final_start_seg)
        *final_start_seg = lseg;

      if (final_end_seg)
        *final_end_seg = lseg;
    }

  if (lseg == NULL)
    gradient->segments = rseg;

  gimp_data_thaw (GIMP_DATA (gradient));

  return;

 premature_return:
  *final_start_seg = start_seg;
  *final_end_seg   = end_seg;
}

void
gimp_gradient_segment_range_recenter_handles (GimpGradient        *gradient,
                                              GimpGradientSegment *start_seg,
                                              GimpGradientSegment *end_seg)
{
  GimpGradientSegment *seg, *aseg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! end_seg)
    end_seg = gimp_gradient_segment_get_last (start_seg);

  seg = start_seg;

  do
    {
      seg->middle = (seg->left + seg->right) / 2.0;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != end_seg);

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_redistribute_handles (GimpGradient        *gradient,
                                                  GimpGradientSegment *start_seg,
                                                  GimpGradientSegment *end_seg)
{
  GimpGradientSegment *seg, *aseg;
  gdouble              left, right, seg_len;
  gint                 num_segs;
  gint                 i;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! end_seg)
    end_seg = gimp_gradient_segment_get_last (start_seg);

  /* Count number of segments in selection */

  num_segs = 0;
  seg      = start_seg;

  do
    {
      num_segs++;
      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != end_seg);

  /* Calculate new segment length */

  left    = start_seg->left;
  right   = end_seg->right;
  seg_len = (right - left) / num_segs;

  /* Redistribute */

  seg = start_seg;

  for (i = 0; i < num_segs; i++)
    {
      seg->left   = left + i * seg_len;
      seg->right  = left + (i + 1) * seg_len;
      seg->middle = (seg->left + seg->right) / 2.0;

      seg = seg->next;
    }

  /* Fix endpoints to squish accumulative error */

  start_seg->left  = left;
  end_seg->right = right;

  gimp_data_thaw (GIMP_DATA (gradient));
}

gdouble
gimp_gradient_segment_range_move (GimpGradient        *gradient,
                                  GimpGradientSegment *range_l,
                                  GimpGradientSegment *range_r,
                                  gdouble              delta,
                                  gboolean             control_compress)
{
  gdouble              lbound, rbound;
  gint                 is_first, is_last;
  GimpGradientSegment *seg, *aseg;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0.0);

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! range_l)
    range_r = gimp_gradient_segment_get_last (range_l);

  /* First or last segments in gradient? */

  is_first = (range_l->prev == NULL);
  is_last  = (range_r->next == NULL);

  /* Calculate drag bounds */

  if (! control_compress)
    {
      if (!is_first)
        lbound = range_l->prev->middle + EPSILON;
      else
        lbound = range_l->left + EPSILON;

      if (!is_last)
        rbound = range_r->next->middle - EPSILON;
      else
        rbound = range_r->right - EPSILON;
    }
  else
    {
      if (!is_first)
        lbound = range_l->prev->left + 2.0 * EPSILON;
      else
        lbound = range_l->left + EPSILON;

      if (!is_last)
        rbound = range_r->next->right - 2.0 * EPSILON;
      else
        rbound = range_r->right - EPSILON;
    }

  /* Fix the delta if necessary */

  if (delta < 0.0)
    {
      if (!is_first)
        {
          if (range_l->left + delta < lbound)
            delta = lbound - range_l->left;
        }
      else
        if (range_l->middle + delta < lbound)
          delta = lbound - range_l->middle;
    }
  else
    {
      if (!is_last)
        {
          if (range_r->right + delta > rbound)
            delta = rbound - range_r->right;
        }
      else
        if (range_r->middle + delta > rbound)
          delta = rbound - range_r->middle;
    }

  /* Move all the segments inside the range */

  seg = range_l;

  do
    {
      if (!((seg == range_l) && is_first))
        seg->left  += delta;

      seg->middle  += delta;

      if (!((seg == range_r) && is_last))
        seg->right += delta;

      /* Next */

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != range_r);

  /* Fix the segments that surround the range */

  if (!is_first)
    {
      if (! control_compress)
        range_l->prev->right = range_l->left;
      else
        gimp_gradient_segment_range_compress (gradient,
                                              range_l->prev, range_l->prev,
                                              range_l->prev->left, range_l->left);
    }

  if (!is_last)
    {
      if (! control_compress)
        range_r->next->left = range_r->right;
      else
        gimp_gradient_segment_range_compress (gradient,
                                              range_r->next, range_r->next,
                                              range_r->right, range_r->next->right);
    }

  gimp_data_thaw (GIMP_DATA (gradient));

  return delta;
}


/*  private functions  */

static GimpGradientSegment *
gimp_gradient_get_segment_at_internal (GimpGradient        *gradient,
                                       GimpGradientSegment *seg,
                                       gdouble              pos)
{
  /* handle FP imprecision at the edges of the gradient */
  pos = CLAMP (pos, 0.0, 1.0);

  if (! seg)
    seg = gradient->segments;

  while (seg)
    {
      if (pos >= seg->left)
        {
          if (pos <= seg->right)
            {
              return seg;
            }
          else
            {
              seg = seg->next;
            }
        }
      else
        {
          seg = seg->prev;
        }
    }

  /* Oops: we should have found a segment, but we didn't */
  g_warning ("%s: no matching segment for position %0.15f", G_STRFUNC, pos);

  return NULL;
}

static inline gdouble
gimp_gradient_calc_linear_factor (gdouble middle,
                                  gdouble pos)
{
  if (pos <= middle)
    {
      if (middle < EPSILON)
        return 0.0;
      else
        return 0.5 * pos / middle;
    }
  else
    {
      pos -= middle;
      middle = 1.0 - middle;

      if (middle < EPSILON)
        return 1.0;
      else
        return 0.5 + 0.5 * pos / middle;
    }
}

static inline gdouble
gimp_gradient_calc_curved_factor (gdouble middle,
                                  gdouble pos)
{
  if (middle < EPSILON)
    middle = EPSILON;

  return pow (pos, log (0.5) / log (middle));
}

static inline gdouble
gimp_gradient_calc_sine_factor (gdouble middle,
                                gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos);

  return (sin ((-G_PI / 2.0) + G_PI * pos) + 1.0) / 2.0;
}

static inline gdouble
gimp_gradient_calc_sphere_increasing_factor (gdouble middle,
                                             gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos) - 1.0;

  /* Works for convex increasing and concave decreasing */
  return sqrt (1.0 - pos * pos);
}

static inline gdouble
gimp_gradient_calc_sphere_decreasing_factor (gdouble middle,
                                             gdouble pos)
{
  pos = gimp_gradient_calc_linear_factor (middle, pos);

  /* Works for convex decreasing and concave increasing */
  return 1.0 - sqrt(1.0 - pos * pos);
}
