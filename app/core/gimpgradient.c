/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpcontext.h"
#include "gimpgradient.h"
#include "gimpgradient-load.h"
#include "gimpgradient-save.h"
#include "gimptagged.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


#define EPSILON 1e-10


static void          gimp_gradient_tagged_iface_init (GimpTaggedInterface *iface);
static void          gimp_gradient_finalize          (GObject             *object);

static gint64        gimp_gradient_get_memsize       (GimpObject          *object,
                                                      gint64              *gui_size);

static void          gimp_gradient_get_preview_size  (GimpViewable        *viewable,
                                                      gint                 size,
                                                      gboolean             popup,
                                                      gboolean             dot_for_dot,
                                                      gint                *width,
                                                      gint                *height);
static gboolean      gimp_gradient_get_popup_size    (GimpViewable        *viewable,
                                                      gint                 width,
                                                      gint                 height,
                                                      gboolean             dot_for_dot,
                                                      gint                *popup_width,
                                                      gint                *popup_height);
static GimpTempBuf * gimp_gradient_get_new_preview   (GimpViewable        *viewable,
                                                      GimpContext         *context,
                                                      gint                 width,
                                                      gint                 height,
                                                      GeglColor           *fg_color);

static const gchar * gimp_gradient_get_extension     (GimpData            *data);
static void          gimp_gradient_copy              (GimpData            *data,
                                                      GimpData            *src_data);
static gint          gimp_gradient_compare           (GimpData            *data1,
                                                      GimpData            *data2);

static gchar       * gimp_gradient_get_checksum      (GimpTagged          *tagged);

static inline GimpGradientSegment *
              gimp_gradient_get_segment_at_internal  (GimpGradient        *gradient,
                                                      GimpGradientSegment *seg,
                                                      gdouble              pos);
static GeglColor   * gimp_gradient_get_flat_color    (GimpContext         *context,
                                                      GeglColor           *color,
                                                      GimpGradientColor    color_type);


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
static inline gdouble  gimp_gradient_calc_step_factor              (gdouble  middle,
                                                                    gdouble  pos);


G_DEFINE_TYPE_WITH_CODE (GimpGradient, gimp_gradient, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_TAGGED,
                                                gimp_gradient_tagged_iface_init))

#define parent_class gimp_gradient_parent_class

static const Babl *fish_srgb_to_linear_rgb = NULL;
static const Babl *fish_linear_rgb_to_srgb = NULL;
static const Babl *fish_srgb_to_cie_lab    = NULL;
static const Babl *fish_cie_lab_to_srgb    = NULL;


static void
gimp_gradient_class_init (GimpGradientClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  object_class->finalize            = gimp_gradient_finalize;

  gimp_object_class->get_memsize    = gimp_gradient_get_memsize;

  viewable_class->default_icon_name = "gimp-tool-gradient";
  viewable_class->default_name      = _("Gradient");
  viewable_class->get_preview_size  = gimp_gradient_get_preview_size;
  viewable_class->get_popup_size    = gimp_gradient_get_popup_size;
  viewable_class->get_new_preview   = gimp_gradient_get_new_preview;

  data_class->save                  = gimp_gradient_save;
  data_class->get_extension         = gimp_gradient_get_extension;
  data_class->copy                  = gimp_gradient_copy;
  data_class->compare               = gimp_gradient_compare;

  fish_srgb_to_linear_rgb = babl_fish (babl_format ("R'G'B' double"),
                                       babl_format ("RGB double"));
  fish_linear_rgb_to_srgb = babl_fish (babl_format ("RGB double"),
                                       babl_format ("R'G'B' double"));
  fish_srgb_to_cie_lab    = babl_fish (babl_format ("R'G'B' double"),
                                       babl_format ("CIE Lab float"));
  fish_cie_lab_to_srgb    = babl_fish (babl_format ("CIE Lab float"),
                                       babl_format ("R'G'B' double"));
}

static void
gimp_gradient_tagged_iface_init (GimpTaggedInterface *iface)
{
  iface->get_checksum = gimp_gradient_get_checksum;
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
  *height = 1 + size / 2;
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

static GimpTempBuf *
gimp_gradient_get_new_preview (GimpViewable *viewable,
                               GimpContext  *context,
                               gint          width,
                               gint          height,
                               GeglColor    *fg_color G_GNUC_UNUSED)
{
  GimpGradient        *gradient = GIMP_GRADIENT (viewable);
  GimpGradientSegment *seg      = NULL;
  GimpTempBuf         *temp_buf;
  guchar              *buf;
  guchar              *p;
  guchar              *row;
  gint                 x, y;
  gdouble              dx, cur_x;

  dx    = 1.0 / (width - 1);
  cur_x = 0.0;
  p     = row = g_malloc (width * 4);

  /* Create lines to fill the image */

  for (x = 0; x < width; x++)
    {
      GeglColor *color = NULL;

      seg = gimp_gradient_get_color_at (gradient, context, seg, cur_x,
                                        FALSE,
                                        GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL,
                                        &color);

      gegl_color_get_pixel (color, babl_format ("R'G'B'A u8"), p);

      p     += 4;
      cur_x += dx;

      g_object_unref (color);
    }

  temp_buf = gimp_temp_buf_new (width, height, babl_format ("R'G'B'A u8"));

  buf = gimp_temp_buf_get_data (temp_buf);

  for (y = 0; y < height; y++)
    memcpy (buf + (width * y * 4), row, width * 4);

  g_free (row);

  return temp_buf;
}

static void
gimp_gradient_copy (GimpData *data,
                    GimpData *src_data)
{
  GimpGradient        *gradient     = GIMP_GRADIENT (data);
  GimpGradient        *src_gradient = GIMP_GRADIENT (src_data);
  GimpGradientSegment *head, *prev, *cur, *orig;

  if (gradient->segments)
    {
      gimp_gradient_segments_free (gradient->segments);
      gradient->segments = NULL;
    }

  prev = NULL;
  orig = src_gradient->segments;
  head = NULL;

  while (orig)
    {
      cur = gimp_gradient_segment_new ();

      g_clear_object (&cur->left_color);
      g_clear_object (&cur->right_color);

      *cur = *orig;  /* Copy everything */

      cur->prev = prev;
      cur->next = NULL;
      cur->left_color  = gegl_color_duplicate (orig->left_color);
      cur->right_color = gegl_color_duplicate (orig->right_color);

      if (prev)
        prev->next = cur;
      else
        head = cur;  /* Remember head */

      prev = cur;
      orig = orig->next;
    }

  gradient->segments = head;

  gimp_data_dirty (GIMP_DATA (gradient));
}

static gint
gimp_gradient_compare (GimpData *data1,
                       GimpData *data2)
{
  gboolean is_custom1;
  gboolean is_custom2;

  /* check whether data1 and data2 are the custom gradient, which is the only
   * writable internal gradient.
   */
  is_custom1 = gimp_data_is_internal (data1) && gimp_data_is_writable (data1);
  is_custom2 = gimp_data_is_internal (data2) && gimp_data_is_writable (data2);

  /* order the custom gradient before all the other gradients; use the default
   * ordering for the rest.
   */
  if (is_custom1)
    {
      if (is_custom2)
        return 0;
      else
        return -1;
    }
  else if (is_custom2)
    {
      return +1;
    }
  else
    return GIMP_DATA_CLASS (parent_class)->compare (data1, data2);
}

static gchar *
gimp_gradient_get_checksum (GimpTagged *tagged)
{
  GimpGradient *gradient        = GIMP_GRADIENT (tagged);
  gchar        *checksum_string = NULL;

  if (gradient->segments)
    {
      GChecksum           *checksum = g_checksum_new (G_CHECKSUM_MD5);
      GimpGradientSegment *segment  = gradient->segments;
      guchar               data[40];

      while (segment)
        {
          const Babl *format;
          gsize       length;

          g_checksum_update (checksum,
                             (const guchar *) &segment->left,
                             sizeof (segment->left));
          g_checksum_update (checksum,
                             (const guchar *) &segment->middle,
                             sizeof (segment->middle));
          g_checksum_update (checksum,
                             (const guchar *) &segment->right,
                             sizeof (segment->right));
          g_checksum_update (checksum,
                             (const guchar *) &segment->left_color_type,
                             sizeof (segment->left_color_type));

          format = gegl_color_get_format (segment->left_color);
          gegl_color_get_pixel (segment->left_color, format, data);
          length = babl_format_get_bytes_per_pixel (format);
          g_checksum_update (checksum, data, (gssize) length);
          g_checksum_update (checksum, (const guchar *) babl_get_name (format),
                             strlen (babl_get_name (format)));

          g_checksum_update (checksum,
                             (const guchar *) &segment->right_color_type,
                             sizeof (segment->right_color_type));

          format = gegl_color_get_format (segment->right_color);
          gegl_color_get_pixel (segment->right_color, format, data);
          length = babl_format_get_bytes_per_pixel (format);
          g_checksum_update (checksum, data, (gssize) length);
          g_checksum_update (checksum, (const guchar *) babl_get_name (format),
                             strlen (babl_get_name (format)));

          g_checksum_update (checksum,
                             (const guchar *) &segment->type,
                             sizeof (segment->type));
          g_checksum_update (checksum,
                             (const guchar *) &segment->color,
                             sizeof (segment->color));

          segment = segment->next;
        }

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}


/*  public functions  */

GimpData *
gimp_gradient_new (GimpContext *context,
                   const gchar *name)
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
gimp_gradient_get_standard (GimpContext *context)
{
  static GimpData *standard_gradient = NULL;

  if (! standard_gradient)
    {
      g_set_weak_pointer (&standard_gradient,
                          gimp_gradient_new (context, "Standard"));

      gimp_data_clean (standard_gradient);
      gimp_data_make_internal (standard_gradient, "gimp-gradient-standard");
    }

  return standard_gradient;
}

static const gchar *
gimp_gradient_get_extension (GimpData *data)
{
  return GIMP_GRADIENT_FILE_EXTENSION;
}

/**
 * gimp_gradient_get_color_at:
 * @gradient:          a gradient
 * @context:           a context
 * @seg: (nullable):   a segment to seed the search with (or %NULL)
 * @pos:               position in the gradient (between 0.0 and 1.0)
 * @reverse:           when %TRUE, use the reversed gradient
 * @blend_color_space: color space to use for blending RGB segments
 * @color:             returns a newly allocated color
 *
 * If you are iterating over an gradient, you should pass the return
 * value from the last call for @seg.
 *
 * Returns: the gradient segment the color is from
 **/
GimpGradientSegment *
gimp_gradient_get_color_at (GimpGradient                 *gradient,
                            GimpContext                  *context,
                            GimpGradientSegment          *seg,
                            gdouble                       pos,
                            gboolean                      reverse,
                            GimpGradientBlendColorSpace   blend_color_space,
                            GeglColor                   **color)
{
  gdouble    factor = 0.0;
  gdouble    seg_len;
  gdouble    middle;
  GeglColor *left_color;
  GeglColor *right_color;
  gdouble    left_alpha;
  gdouble    right_alpha;

  /* type-check disabled to improve speed */
  /* g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL); */
  g_return_val_if_fail (color != NULL && *color == NULL, NULL);

  *color = gegl_color_new (NULL);

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

    case GIMP_GRADIENT_SEGMENT_STEP:
      factor = gimp_gradient_calc_step_factor (middle, pos);
      break;

    default:
      g_warning ("%s: Unknown gradient type %d.", G_STRFUNC, seg->type);
      break;
    }

  /* Get left/right colors */

  if (context)
    {
      left_color  = gimp_gradient_segment_get_left_flat_color (gradient, context, seg);
      right_color = gimp_gradient_segment_get_right_flat_color (gradient, context, seg);
    }
  else
    {
      left_color  = g_object_ref (seg->left_color);
      right_color = g_object_ref (seg->right_color);
    }

  /* Calculate color components */

  if (seg->color == GIMP_GRADIENT_SEGMENT_RGB)
    {
      gfloat  float_components[3];
      gdouble left_components[3];
      gdouble right_components[3];
      gdouble ret_components[3];

      switch (blend_color_space)
        {
        case GIMP_GRADIENT_BLEND_CIE_LAB:
          gegl_color_get_pixel (left_color, babl_format ("CIE Lab float"), float_components);
          for (gint i = 0; i < 3; i++)
            left_components[i] = float_components[i];

          gegl_color_get_pixel (right_color, babl_format ("CIE Lab float"), float_components);
          for (gint i = 0; i < 3; i++)
            right_components[i] = float_components[i];
          break;

        case GIMP_GRADIENT_BLEND_RGB_LINEAR:
          gegl_color_get_pixel (left_color, babl_format ("RGB double"), left_components);
          gegl_color_get_pixel (right_color, babl_format ("RGB double"), right_components);

        case GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL:
          /* TODO: shouldn't this format use the color space of the target
           * image? Will it change computation? */
          gegl_color_get_pixel (left_color, babl_format ("R'G'B' double"), left_components);
          gegl_color_get_pixel (right_color, babl_format ("R'G'B' double"), right_components);
          break;
        }

      ret_components[0] = left_components[0] + (right_components[0] - left_components[0]) * factor;
      ret_components[1] = left_components[1] + (right_components[1] - left_components[1]) * factor;
      ret_components[2] = left_components[2] + (right_components[2] - left_components[2]) * factor;

      switch (blend_color_space)
        {
        case GIMP_GRADIENT_BLEND_CIE_LAB:
          for (gint i = 0; i < 3; i++)
            float_components[i] = (gfloat) ret_components[i];
          gegl_color_set_pixel (*color, babl_format ("CIE Lab float"), float_components);
          break;

        case GIMP_GRADIENT_BLEND_RGB_LINEAR:
          gegl_color_set_pixel (*color, babl_format ("RGB double"), ret_components);
          break;

        case GIMP_GRADIENT_BLEND_RGB_PERCEPTUAL:
          gegl_color_set_pixel (*color, babl_format ("R'G'B' double"), ret_components);
          break;
        }
    }
  else
    {
      gfloat left_hsv[3];
      gfloat right_hsv[3];

      gegl_color_get_pixel (left_color, babl_format ("HSV float"), left_hsv);
      gegl_color_get_pixel (right_color, babl_format ("HSV float"), right_hsv);

      left_hsv[1] = left_hsv[1] + (right_hsv[1] - left_hsv[1]) * factor;
      left_hsv[2] = left_hsv[2] + (right_hsv[2] - left_hsv[2]) * factor;

      switch (seg->color)
        {
        case GIMP_GRADIENT_SEGMENT_HSV_CCW:
          if (left_hsv[0] < right_hsv[0])
            {
              left_hsv[0] += (right_hsv[0] - left_hsv[0]) * factor;
            }
          else
            {
              left_hsv[0] += (1.0 - (left_hsv[0] - right_hsv[0])) * factor;

              if (left_hsv[0] > 1.0)
                left_hsv[0] -= 1.0;
            }
          break;

        case GIMP_GRADIENT_SEGMENT_HSV_CW:
          if (right_hsv[0] < left_hsv[0])
            {
              left_hsv[0] -= (left_hsv[0] - right_hsv[0]) * factor;
            }
          else
            {
              left_hsv[0] -= (1.0 - (right_hsv[0] - left_hsv[0])) * factor;

              if (left_hsv[0] < 0.0)
                left_hsv[0] += 1.0;
            }
          break;

        default:
          g_warning ("%s: Unknown coloring mode %d",
                     G_STRFUNC, (gint) seg->color);
          break;
        }

      gegl_color_set_pixel (*color, babl_format ("HSV float"), left_hsv);
    }

  /* Calculate alpha */

  gegl_color_get_rgba (left_color, NULL, NULL, NULL, &left_alpha);
  gegl_color_get_rgba (right_color, NULL, NULL, NULL, &right_alpha);
  gimp_color_set_alpha (*color, left_alpha + (right_alpha - left_alpha) * factor);

  g_object_unref (left_color);
  g_object_unref (right_color);

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

void
gimp_gradient_split_at (GimpGradient                 *gradient,
                        GimpContext                  *context,
                        GimpGradientSegment          *seg,
                        gdouble                       pos,
                        GimpGradientBlendColorSpace   blend_color_space,
                        GimpGradientSegment         **newl,
                        GimpGradientSegment         **newr)
{
  GeglColor           *color = NULL;
  GimpGradientSegment *newseg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  gimp_data_freeze (GIMP_DATA (gradient));

  pos = CLAMP (pos, 0.0, 1.0);
  seg = gimp_gradient_get_segment_at_internal (gradient, seg, pos);

  /* Get color at pos */
  gimp_gradient_get_color_at (gradient, context, seg, pos,
                              FALSE, blend_color_space, &color);

  /* Create a new segment and insert it in the list */

  newseg = gimp_gradient_segment_new ();

  newseg->prev = seg;
  newseg->next = seg->next;

  seg->next = newseg;

  if (newseg->next)
    newseg->next->prev = newseg;

  /* Set coordinates of new segment */

  newseg->left   = pos;
  newseg->right  = seg->right;
  newseg->middle = (newseg->left + newseg->right) / 2.0;

  /* Set coordinates of original segment */

  seg->right  = newseg->left;
  seg->middle = (seg->left + seg->right) / 2.0;

  /* Set colors of both segments */

  g_clear_object (&newseg->right_color);
  g_clear_object (&newseg->left_color);

  newseg->right_color_type = seg->right_color_type;
  newseg->right_color      = seg->right_color;

  seg->right_color_type = newseg->left_color_type = GIMP_GRADIENT_COLOR_FIXED;
  seg->right_color      = color;
  newseg->left_color    = gegl_color_duplicate (color);

  /* Set parameters of new segment */

  newseg->type  = seg->type;
  newseg->color = seg->color;

  /* Done */

  if (newl) *newl = seg;
  if (newr) *newr = newseg;

  gimp_data_thaw (GIMP_DATA (gradient));
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
      GeglColor *color;

      color = gimp_gradient_segment_get_left_flat_color (gradient, context, seg);
      g_clear_object (&seg->left_color);
      seg->left_color = color;

      seg->left_color_type = GIMP_GRADIENT_COLOR_FIXED;

      color = gimp_gradient_segment_get_right_flat_color (gradient, context, seg);
      g_clear_object (&seg->right_color);
      seg->right_color = color;

      seg->right_color_type = GIMP_GRADIENT_COLOR_FIXED;
    }

  return flat;
}


/*  gradient segment functions  */

GimpGradientSegment *
gimp_gradient_segment_new (void)
{
  GimpGradientSegment *seg;

  seg = g_slice_new0 (GimpGradientSegment);

  seg->left   = 0.0;
  seg->middle = 0.5;
  seg->right  = 1.0;

  seg->left_color_type = GIMP_GRADIENT_COLOR_FIXED;
  seg->left_color = gegl_color_new ("black");

  seg->right_color_type = GIMP_GRADIENT_COLOR_FIXED;
  seg->right_color = gegl_color_new ("white");

  seg->type  = GIMP_GRADIENT_SEGMENT_LINEAR;
  seg->color = GIMP_GRADIENT_SEGMENT_RGB;

  seg->prev = seg->next = NULL;

  return seg;
}


void
gimp_gradient_segment_free (GimpGradientSegment *seg)
{
  g_return_if_fail (seg != NULL);

  g_clear_object (&seg->right_color);
  g_clear_object (&seg->left_color);
  g_slice_free (GimpGradientSegment, seg);
}

void
gimp_gradient_segments_free (GimpGradientSegment *seg)
{
  GimpGradientSegment *current = seg;
  GimpGradientSegment *next;

  g_return_if_fail (seg != NULL);

  if (seg->prev)
    seg->prev->next = NULL;

  do
    {
      next = current->next;
      gimp_gradient_segment_free (current);
      current = next;
    }
  while (next);
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
gimp_gradient_segment_split_midpoint (GimpGradient                 *gradient,
                                      GimpContext                  *context,
                                      GimpGradientSegment          *lseg,
                                      GimpGradientBlendColorSpace   blend_color_space,
                                      GimpGradientSegment         **newl,
                                      GimpGradientSegment         **newr)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (lseg != NULL);
  g_return_if_fail (newl != NULL);
  g_return_if_fail (newr != NULL);

  gimp_gradient_split_at (gradient, context, lseg, lseg->middle,
                          blend_color_space, newl, newr);
}

void
gimp_gradient_segment_split_uniform (GimpGradient                 *gradient,
                                     GimpContext                  *context,
                                     GimpGradientSegment          *lseg,
                                     gint                          parts,
                                     GimpGradientBlendColorSpace   blend_color_space,
                                     GimpGradientSegment         **newl,
                                     GimpGradientSegment         **newr)
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
      GeglColor *color = NULL;

      seg = gimp_gradient_segment_new ();

      if (i == 0)
        tmp = seg; /* Remember first segment */

      seg->left   = lseg->left + i * seg_len;
      seg->right  = lseg->left + (i + 1) * seg_len;
      seg->middle = (seg->left + seg->right) / 2.0;

      seg->left_color_type  = GIMP_GRADIENT_COLOR_FIXED;
      seg->right_color_type = GIMP_GRADIENT_COLOR_FIXED;

      gimp_gradient_get_color_at (gradient, context, lseg,
                                  seg->left,  FALSE, blend_color_space,
                                  &color);
      g_clear_object (&seg->left_color);
      seg->left_color = color;

      color = NULL;
      gimp_gradient_get_color_at (gradient, context, lseg,
                                  seg->right, FALSE, blend_color_space,
                                  &color);
      g_clear_object (&seg->right_color);
      seg->right_color = color;

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
  g_clear_object (&tmp->left_color);
  tmp->left_color      = gegl_color_duplicate (lseg->left_color);

  seg->right_color_type = lseg->right_color_type;
  g_clear_object (&seg->right_color);
  seg->right_color      = gegl_color_duplicate (lseg->right_color);

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

GeglColor *
gimp_gradient_segment_get_left_color (GimpGradient        *gradient,
                                      GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (seg != NULL, NULL);

  return seg->left_color;
}

void
gimp_gradient_segment_set_left_color (GimpGradient        *gradient,
                                      GimpGradientSegment *seg,
                                      GeglColor           *color)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (seg != NULL);
  g_return_if_fail (GEGL_IS_COLOR (color));

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_range_blend (gradient, seg, seg,
                                     color, seg->right_color,
                                     TRUE, TRUE);

  gimp_data_thaw (GIMP_DATA (gradient));
}

GeglColor *
gimp_gradient_segment_get_right_color (GimpGradient        *gradient,
                                       GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (seg != NULL, NULL);

  return seg->right_color;
}

void
gimp_gradient_segment_set_right_color (GimpGradient        *gradient,
                                       GimpGradientSegment *seg,
                                       GeglColor           *color)
{
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (seg != NULL);
  g_return_if_fail (GEGL_IS_COLOR (color));

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_range_blend (gradient, seg, seg,
                                     seg->left_color, color,
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

GeglColor *
gimp_gradient_segment_get_left_flat_color (GimpGradient        *gradient,
                                           GimpContext         *context,
                                           GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (seg != NULL, NULL);

  return gimp_gradient_get_flat_color (context,
                                       seg->left_color, seg->left_color_type);
}

GeglColor *
gimp_gradient_segment_get_right_flat_color (GimpGradient        *gradient,
                                            GimpContext         *context,
                                            GimpGradientSegment *seg)
{
  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (seg != NULL, NULL);

  return gimp_gradient_get_flat_color (context,
                                       seg->right_color, seg->right_color_type);
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

gint
gimp_gradient_segment_range_get_n_segments (GimpGradient        *gradient,
                                            GimpGradientSegment *range_l,
                                            GimpGradientSegment *range_r)
{
  gint n_segments = 0;

  g_return_val_if_fail (GIMP_IS_GRADIENT (gradient), 0);
  g_return_val_if_fail (range_l != NULL, 0);

  for (; range_l != range_r; range_l = range_l->next)
    n_segments++;

  if (range_r != NULL)
    n_segments++;

  return n_segments;
}

void
gimp_gradient_segment_range_compress (GimpGradient        *gradient,
                                      GimpGradientSegment *range_l,
                                      GimpGradientSegment *range_r,
                                      gdouble              new_l,
                                      gdouble              new_r)
{
  gdouble              orig_l, orig_r;
  GimpGradientSegment *seg, *aseg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (range_l != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! range_r)
    range_r = gimp_gradient_segment_get_last (range_l);

  orig_l = range_l->left;
  orig_r = range_r->right;

  if (orig_r - orig_l > EPSILON)
    {
      gdouble scale;

      scale = (new_r - new_l) / (orig_r - orig_l);

      seg = range_l;

      do
        {
          if (seg->prev)
            seg->left  = new_l + (seg->left - orig_l) * scale;
          seg->middle  = new_l + (seg->middle - orig_l) * scale;
          if (seg->next)
            seg->right = new_l + (seg->right - orig_l) * scale;

          /* Next */

          aseg = seg;
          seg  = seg->next;
        }
      while (aseg != range_r);
    }
  else
    {
      gint n;
      gint i;

      n = gimp_gradient_segment_range_get_n_segments (gradient,
                                                      range_l, range_r);

      for (i = 0, seg = range_l; i < n; i++, seg = seg->next)
        {
          if (seg->prev)
            seg->left  = new_l + (new_r - new_l) * (i + 0.0) / n;
          seg->middle  = new_l + (new_r - new_l) * (i + 0.5) / n;;
          if (seg->next)
            seg->right = new_l + (new_r - new_l) * (i + 1.0) / n;
        }
    }

  /* Make sure that the left and right endpoints of the range are *exactly*
   * equal to new_l and new_r; the above computations can introduce
   * numerical inaccuracies.
   */

  range_l->left  = new_l;
  range_l->right = new_r;

  gimp_data_thaw (GIMP_DATA (gradient));
}

void
gimp_gradient_segment_range_blend (GimpGradient        *gradient,
                                   GimpGradientSegment *lseg,
                                   GimpGradientSegment *rseg,
                                   GeglColor           *color1,
                                   GeglColor           *color2,
                                   gboolean             blend_colors,
                                   gboolean             blend_opacity)
{
  gdouble              left, len;
  GimpGradientSegment *seg;
  GimpGradientSegment *aseg;
  gdouble              red1;
  gdouble              green1;
  gdouble              blue1;
  gdouble              alpha1;
  gdouble              red2;
  gdouble              green2;
  gdouble              blue2;
  gdouble              alpha2;
  gdouble              dred;
  gdouble              dgreen;
  gdouble              dblue;
  gdouble              dalpha;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));
  g_return_if_fail (lseg != NULL);

  gimp_data_freeze (GIMP_DATA (gradient));

  if (! rseg)
    rseg = gimp_gradient_segment_get_last (lseg);

  /* TODO: currently doing this computation with sRGB value, because that's what
   * was done historically. Is it right though?
   */
  gegl_color_get_rgba_with_space (color1, &red1, &green1, &blue1, &alpha1, NULL);
  gegl_color_get_rgba_with_space (color2, &red2, &green2, &blue2, &alpha2, NULL);
  dred   = red2 - red1;
  dgreen = green2 - green1;
  dblue  = blue2 - blue1;
  dalpha = alpha2 - alpha1;

  left  = lseg->left;
  len   = rseg->right - left;

  seg = lseg;

  do
    {
      gdouble seg_left_red;
      gdouble seg_left_green;
      gdouble seg_left_blue;
      gdouble seg_left_alpha;
      gdouble seg_right_red;
      gdouble seg_right_green;
      gdouble seg_right_blue;
      gdouble seg_right_alpha;

      if (blend_colors)
        {
          seg_left_red    = red1 + (seg->left - left) / len * dred;
          seg_left_green  = green1 + (seg->left - left) / len * dgreen;
          seg_left_blue   = blue1 + (seg->left - left) / len * dblue;

          seg_right_red   = red1 + (seg->right - left) / len * dred;
          seg_right_green = green1 + (seg->right - left) / len * dgreen;
          seg_right_blue  = blue1 + (seg->right - left) / len * dblue;
        }
      else if (blend_opacity)
        {
          gegl_color_get_rgba_with_space (seg->left_color, &seg_left_red, &seg_left_green,
                                          &seg_left_blue, NULL, NULL);
          gegl_color_get_rgba_with_space (seg->right_color, &seg_right_red, &seg_right_green,
                                          &seg_right_blue, NULL, NULL);
        }

      if (blend_opacity)
        {
          seg_left_alpha  = alpha1 + (seg->left - left) / len * dalpha;
          seg_right_alpha = alpha1 + (seg->right - left) / len * dalpha;
        }
      else if (blend_colors)
        {
          gegl_color_get_rgba_with_space (seg->left_color, NULL, NULL, NULL, &seg_left_alpha, NULL);
          gegl_color_get_rgba_with_space (seg->right_color, NULL, NULL, NULL, &seg_right_alpha, NULL);
        }

      if (blend_colors || blend_opacity)
        {
          gegl_color_set_rgba_with_space (seg->left_color, seg_left_red, seg_left_green,
                                          seg_left_blue, seg_left_alpha, NULL);
          gegl_color_set_rgba_with_space (seg->right_color, seg_right_red, seg_right_green,
                                          seg_right_blue, seg_right_alpha, NULL);
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
        {
          seg->left = left + right - oseg->right;
        }

      seg->middle = left + right - oseg->middle;
      seg->right  = left + right - oseg->left;

      g_clear_object (&seg->left_color);
      g_clear_object (&seg->right_color);

      seg->left_color_type = oseg->right_color_type;
      seg->left_color      = gegl_color_duplicate (oseg->right_color);

      seg->right_color_type = oseg->left_color_type;
      seg->right_color      = gegl_color_duplicate (oseg->left_color);

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

          g_clear_object (&seg->left_color);
          g_clear_object (&seg->right_color);

          seg->left_color_type = oseg->left_color_type;
          seg->left_color      = gegl_color_duplicate (oseg->left_color);

          seg->right_color_type = oseg->right_color_type;
          seg->right_color      = gegl_color_duplicate (oseg->right_color);

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
gimp_gradient_segment_range_split_midpoint (GimpGradient                 *gradient,
                                            GimpContext                  *context,
                                            GimpGradientSegment          *start_seg,
                                            GimpGradientSegment          *end_seg,
                                            GimpGradientBlendColorSpace   blend_color_space,
                                            GimpGradientSegment         **final_start_seg,
                                            GimpGradientSegment         **final_end_seg)
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
                                            seg, blend_color_space,
                                            &lseg, &rseg);
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
gimp_gradient_segment_range_split_uniform (GimpGradient                 *gradient,
                                           GimpContext                  *context,
                                           GimpGradientSegment          *start_seg,
                                           GimpGradientSegment          *end_seg,
                                           gint                          parts,
                                           GimpGradientBlendColorSpace   blend_color_space,
                                           GimpGradientSegment         **final_start_seg,
                                           GimpGradientSegment         **final_end_seg)
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
                                           parts, blend_color_space,
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
  if (final_start_seg)
    *final_start_seg = start_seg;
  if (final_end_seg)
    *final_end_seg = end_seg;
}

void
gimp_gradient_segment_range_merge (GimpGradient         *gradient,
                                   GimpGradientSegment  *start_seg,
                                   GimpGradientSegment  *end_seg,
                                   GimpGradientSegment **final_start_seg,
                                   GimpGradientSegment **final_end_seg)
{
  GimpGradientSegment *seg;

  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  if (! end_seg)
    end_seg = gimp_gradient_segment_get_last (start_seg);

  gimp_data_freeze (GIMP_DATA (gradient));

  if (start_seg == end_seg)
    goto out;

  /* Copy the end segment's right position and color to the start segment */

  start_seg->right            = end_seg->right;
  start_seg->right_color_type = end_seg->right_color_type;
  g_clear_object (&start_seg->right_color);
  start_seg->right_color      = gegl_color_duplicate (end_seg->right_color);

  /* Center the start segment's midpoint */

  start_seg->middle = (start_seg->left + start_seg->right) / 2.0;

  /* Remove range segments past the start segment from the segment list */

  start_seg->next = end_seg->next;

  if (start_seg->next)
    start_seg->next->prev = start_seg;

  /* Merge the range's blend function and coloring type, and free the rest of
   * the segments.
   */

  seg = end_seg;

  while (seg != start_seg)
    {
      GimpGradientSegment *prev = seg->prev;

      /* If the blend function and/or coloring type aren't uniform, reset them. */

      if (seg->type != start_seg->type)
        start_seg->type = GIMP_GRADIENT_SEGMENT_LINEAR;

      if (seg->color != start_seg->color)
        start_seg->color = GIMP_GRADIENT_SEGMENT_RGB;

      gimp_gradient_segment_free (seg);

      seg = prev;
    }

out:

  if (final_start_seg)
    *final_start_seg = start_seg;
  if (final_end_seg)
    *final_end_seg = start_seg;

  gimp_data_thaw (GIMP_DATA (gradient));
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

  if (! range_r)
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

static inline GimpGradientSegment *
gimp_gradient_get_segment_at_internal (GimpGradient        *gradient,
                                       GimpGradientSegment *seg,
                                       gdouble              pos)
{
  /* handle FP imprecision at the edges of the gradient */
  pos = CLAMP (pos, 0.0, 1.0);

  if (! seg)
    seg = gradient->segments;

  if (pos >= seg->left)
    {
      while (seg->next && pos >= seg->right)
        seg = seg->next;
    }
  else
    {
      do
        seg = seg->prev;
      while (pos < seg->left);
    }

  return seg;
}

static GeglColor *
gimp_gradient_get_flat_color (GimpContext       *context,
                              GeglColor         *color,
                              GimpGradientColor  color_type)
{
  GeglColor *flat_color = NULL;

  switch (color_type)
    {
    case GIMP_GRADIENT_COLOR_FIXED:
      flat_color = gegl_color_duplicate (color);
      break;

    case GIMP_GRADIENT_COLOR_FOREGROUND:
    case GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT:
      flat_color = gegl_color_duplicate (gimp_context_get_foreground (context));

      if (color_type == GIMP_GRADIENT_COLOR_FOREGROUND_TRANSPARENT)
        gimp_color_set_alpha (flat_color, 0.0);
      break;

    case GIMP_GRADIENT_COLOR_BACKGROUND:
    case GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT:
      flat_color = gegl_color_duplicate (gimp_context_get_background (context));

      if (color_type == GIMP_GRADIENT_COLOR_BACKGROUND_TRANSPARENT)
        gimp_color_set_alpha (flat_color, 0.0);
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  return flat_color;
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
    return 1.0;
  else if (1.0 - middle < EPSILON)
    return 0.0;

  return exp (-G_LN2 * log (pos) / log (middle));
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

static inline gdouble
gimp_gradient_calc_step_factor (gdouble middle,
                                gdouble pos)
{
  return pos >= middle;
}
