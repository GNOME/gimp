/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#include <fontconfig/fontconfig.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "text-types.h"

#include "gegl/gimp-babl.h"

#include "core/gimperror.h"
#include "core/gimpimage.h"

#include "gimpfont.h"

#include "gimptext.h"
#include "gimptextlayout.h"

#include "gimp-intl.h"

struct _GimpTextLayout
{
  GObject         object;

  GimpText       *text;
  gdouble         xres;
  gdouble         yres;
  PangoLayout    *layout;
  PangoRectangle  extents;
  const Babl     *layout_space;
  GimpTRCType     layout_trc;
};


static void           gimp_text_layout_finalize   (GObject        *object);

static void           gimp_text_layout_position   (GimpTextLayout *layout);
static void           gimp_text_layout_set_markup (GimpTextLayout *layout,
                                                   GError        **error);

static PangoContext * gimp_text_get_pango_context (GimpText       *text,
                                                   gdouble         xres,
                                                   gdouble         yres);


G_DEFINE_TYPE (GimpTextLayout, gimp_text_layout, G_TYPE_OBJECT)

#define parent_class gimp_text_layout_parent_class


static void
gimp_text_layout_class_init (GimpTextLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_text_layout_finalize;
}

static void
gimp_text_layout_init (GimpTextLayout *layout)
{
  layout->text   = NULL;
  layout->layout = NULL;
}

static void
gimp_text_layout_finalize (GObject *object)
{
  GimpTextLayout *layout = GIMP_TEXT_LAYOUT (object);

  if (layout->text)
    {
      g_object_unref (layout->text);
      layout->text = NULL;
    }
  if (layout->layout)
    {
      g_object_unref (layout->layout);
      layout->layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


GimpTextLayout *
gimp_text_layout_new (GimpText     *text,
                      GimpImage    *target_image,
                      gdouble       xres,
                      gdouble       yres,
                      GError      **error)
{
  GimpTextLayout       *layout;
  const Babl           *target_space;
  PangoContext         *context;
  PangoFontDescription *font_desc;
  PangoAlignment        alignment = PANGO_ALIGN_LEFT;
  gint                  size;

  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (target_image), NULL);

  font_desc = pango_font_description_from_string (gimp_font_get_lookup_name (text->font));
  g_return_val_if_fail (font_desc != NULL, NULL);

  size = pango_units_from_double (gimp_units_to_points (text->font_size,
                                                        text->unit,
                                                        yres));

  pango_font_description_set_size (font_desc, MAX (1, size));

  context = gimp_text_get_pango_context (text, xres, yres);

  layout = g_object_new (GIMP_TYPE_TEXT_LAYOUT, NULL);

  layout->text   = g_object_ref (text);
  layout->layout = pango_layout_new (context);
  layout->xres   = xres;
  layout->yres   = yres;

  pango_layout_set_wrap (layout->layout, PANGO_WRAP_WORD_CHAR);

  pango_layout_set_font_description (layout->layout, font_desc);
  pango_font_description_free (font_desc);

  target_space = gimp_image_get_layer_space (target_image);
  if (babl_space_is_rgb (target_space) || babl_space_is_gray (target_space))
    layout->layout_space = target_space;
  else
    layout->layout_space = NULL;

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 2)
  layout->layout_trc = gimp_babl_trc (gimp_image_get_precision (target_image));
#else
  /* With older Cairo, we just use cairo-ARGB32 with no linear or perceptual
   * option.
   */
  layout->layout_trc = GIMP_TRC_NON_LINEAR;
#endif

  gimp_text_layout_set_markup (layout, error);

  switch (text->justify)
    {
    case GIMP_TEXT_JUSTIFY_LEFT:
      alignment = PANGO_ALIGN_LEFT;
      break;
    case GIMP_TEXT_JUSTIFY_RIGHT:
      alignment = PANGO_ALIGN_RIGHT;
      break;
    case GIMP_TEXT_JUSTIFY_CENTER:
      alignment = PANGO_ALIGN_CENTER;
      break;
    case GIMP_TEXT_JUSTIFY_FILL:
      alignment = PANGO_ALIGN_LEFT;
      pango_layout_set_justify (layout->layout, TRUE);
      break;
    }

  pango_layout_set_alignment (layout->layout, alignment);

  switch (text->box_mode)
    {
    case GIMP_TEXT_BOX_DYNAMIC:
      break;
    case GIMP_TEXT_BOX_FIXED:
      if (! PANGO_GRAVITY_IS_VERTICAL (pango_context_get_base_gravity (context)))
        pango_layout_set_width (layout->layout,
                                pango_units_from_double
                                (gimp_units_to_pixels (text->box_width,
                                                       text->box_unit,
                                                       xres)));
      else
        pango_layout_set_width (layout->layout,
                                pango_units_from_double
                                (gimp_units_to_pixels (text->box_height,
                                                       text->box_unit,
                                                       yres)));
      break;
    }

  pango_layout_set_indent (layout->layout,
                           pango_units_from_double
                           (gimp_units_to_pixels (text->indent,
                                                  text->unit,
                                                  xres)));
  pango_layout_set_spacing (layout->layout,
                            pango_units_from_double
                            (gimp_units_to_pixels (text->line_spacing,
                                                   text->unit,
                                                   yres)));

  gimp_text_layout_position (layout);

  switch (text->box_mode)
    {
    case GIMP_TEXT_BOX_DYNAMIC:
      break;
    case GIMP_TEXT_BOX_FIXED:
      layout->extents.width  = ceil (gimp_units_to_pixels (text->box_width,
                                                           text->box_unit,
                                                           xres));
      layout->extents.height = ceil (gimp_units_to_pixels (text->box_height,
                                                           text->box_unit,
                                                           yres));

/* #define VERBOSE */

#ifdef VERBOSE
      g_printerr ("extents set to %d x %d\n",
                  layout->extents.width, layout->extents.height);
#endif
      break;
    }

  g_object_unref (context);

  return layout;
}

const GimpTRCType
gimp_text_layout_get_trc (GimpTextLayout *layout)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), GIMP_TRC_NON_LINEAR);

  return layout->layout_trc;
}

const Babl *
gimp_text_layout_get_format (GimpTextLayout *layout,
                             const gchar    *babl_type)
{
  const Babl *format;
  gchar      *format_name;

  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), NULL);

  if (! babl_space_is_gray (layout->layout_space))
    {
      switch (layout->layout_trc)
        {
        case GIMP_TRC_LINEAR:
          format_name = g_strdup_printf ("RGB %s", babl_type);
          break;
        case GIMP_TRC_NON_LINEAR:
          format_name = g_strdup_printf ("R'G'B' %s", babl_type);
          break;
        case GIMP_TRC_PERCEPTUAL:
          format_name = g_strdup_printf ("R~G~B~ %s", babl_type);
          break;
        default:
          g_return_val_if_reached (NULL);
        }
    }
  else
    {
      switch (layout->layout_trc)
        {
        case GIMP_TRC_LINEAR:
          format_name = g_strdup_printf ("Y %s", babl_type);
          break;
        case GIMP_TRC_NON_LINEAR:
          format_name = g_strdup_printf ("Y' %s", babl_type);
          break;
        case GIMP_TRC_PERCEPTUAL:
          format_name = g_strdup_printf ("Y~ %s", babl_type);
          break;
        default:
          g_return_val_if_reached (NULL);
        }
    }

  format = babl_format_with_space (format_name, layout->layout_space);
  g_free (format_name);

  return format;
}

const Babl *
gimp_text_layout_get_space (GimpTextLayout *layout)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), NULL);

  return layout->layout_space;
}

gboolean
gimp_text_layout_get_size (GimpTextLayout *layout,
                           gint           *width,
                           gint           *height)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), FALSE);

  if (width)
    *width = layout->extents.width;

  if (height)
    *height = layout->extents.height;

  return (layout->extents.width > 0 && layout->extents.height > 0);
}

void
gimp_text_layout_get_offsets (GimpTextLayout *layout,
                              gint           *x,
                              gint           *y)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));

  if (x)
    *x = layout->extents.x;

  if (y)
    *y = layout->extents.y;
}

void
gimp_text_layout_get_resolution (GimpTextLayout *layout,
                                 gdouble        *xres,
                                 gdouble        *yres)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));

  if (xres)
    *xres = layout->xres;

  if (yres)
    *yres = layout->yres;
}

GimpText *
gimp_text_layout_get_text (GimpTextLayout *layout)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), NULL);

  return layout->text;
}

PangoLayout *
gimp_text_layout_get_pango_layout (GimpTextLayout *layout)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYOUT (layout), NULL);

  return layout->layout;
}

void
gimp_text_layout_get_transform (GimpTextLayout *layout,
                                cairo_matrix_t *matrix)
{
  GimpText *text;
  gdouble   xres = 0;
  gdouble   yres = 0;
  gdouble   norm;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (matrix != NULL);

  text = gimp_text_layout_get_text (layout);

  gimp_text_layout_get_resolution (layout, &xres, &yres);

  norm = 1.0 / yres * xres;

  matrix->xx = text->transformation.coeff[0][0] * norm;
  matrix->xy = text->transformation.coeff[0][1] * 1.0;
  matrix->yx = text->transformation.coeff[1][0] * norm;
  matrix->yy = text->transformation.coeff[1][1] * 1.0;
  matrix->x0 = 0;
  matrix->y0 = 0;
}

void
gimp_text_layout_transform_rect (GimpTextLayout *layout,
                                 PangoRectangle *rect)
{
  cairo_matrix_t matrix;
  gdouble        x, y;
  gdouble        width, height;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (rect != NULL);

  x      = rect->x;
  y      = rect->y;
  width  = rect->width;
  height = rect->height;

  gimp_text_layout_get_transform (layout, &matrix);

  cairo_matrix_transform_point (&matrix, &x, &y);
  cairo_matrix_transform_distance (&matrix, &width, &height);

  rect->x      = ROUND (x);
  rect->y      = ROUND (y);
  rect->width  = ROUND (width);
  rect->height = ROUND (height);
}

void
gimp_text_layout_transform_point (GimpTextLayout *layout,
                                  gdouble        *x,
                                  gdouble        *y)
{
  cairo_matrix_t matrix;
  gdouble        _x = 0.0;
  gdouble        _y = 0.0;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));

  if (x) _x = *x;
  if (y) _y = *y;

  gimp_text_layout_get_transform (layout, &matrix);

  cairo_matrix_transform_point (&matrix, &_x, &_y);

  if (x) *x = _x;
  if (y) *y = _y;
}

void
gimp_text_layout_transform_distance (GimpTextLayout *layout,
                                     gdouble        *x,
                                     gdouble        *y)
{
  cairo_matrix_t matrix;
  gdouble        _x = 0.0;
  gdouble        _y = 0.0;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));

  if (x) _x = *x;
  if (y) _y = *y;

  gimp_text_layout_get_transform (layout, &matrix);

  cairo_matrix_transform_distance (&matrix, &_x, &_y);

  if (x) *x = _x;
  if (y) *y = _y;
}

void
gimp_text_layout_untransform_rect (GimpTextLayout *layout,
                                   PangoRectangle *rect)
{
  cairo_matrix_t matrix;
  gdouble        x, y;
  gdouble        width, height;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (rect != NULL);

  x      = rect->x;
  y      = rect->y;
  width  = rect->width;
  height = rect->height;

  gimp_text_layout_get_transform (layout, &matrix);

  if (cairo_matrix_invert (&matrix) == CAIRO_STATUS_SUCCESS)
    {
      cairo_matrix_transform_point (&matrix, &x, &y);
      cairo_matrix_transform_distance (&matrix, &width, &height);

      rect->x      = ROUND (x);
      rect->y      = ROUND (y);
      rect->width  = ROUND (width);
      rect->height = ROUND (height);
    }
}

void
gimp_text_layout_untransform_point (GimpTextLayout *layout,
                                    gdouble        *x,
                                    gdouble        *y)
{
  cairo_matrix_t matrix;
  gdouble        _x = 0.0;
  gdouble        _y = 0.0;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));

  if (x) _x = *x;
  if (y) _y = *y;

  gimp_text_layout_get_transform (layout, &matrix);

  if (cairo_matrix_invert (&matrix) == CAIRO_STATUS_SUCCESS)
    {
      cairo_matrix_transform_point (&matrix, &_x, &_y);

      if (x) *x = _x;
      if (y) *y = _y;
    }
}

void
gimp_text_layout_untransform_distance (GimpTextLayout *layout,
                                       gdouble        *x,
                                       gdouble        *y)
{
  cairo_matrix_t matrix;
  gdouble        _x = 0.0;
  gdouble        _y = 0.0;

  g_return_if_fail (GIMP_IS_TEXT_LAYOUT (layout));

  if (x) _x = *x;
  if (y) _y = *y;

  gimp_text_layout_get_transform (layout, &matrix);

  if (cairo_matrix_invert (&matrix) == CAIRO_STATUS_SUCCESS)
    {
      cairo_matrix_transform_distance (&matrix, &_x, &_y);

      if (x) *x = _x;
      if (y) *y = _y;
    }
}

static gboolean
gimp_text_layout_split_markup (const gchar  *markup,
                               gchar       **open_tag,
                               gchar       **content,
                               gchar       **close_tag)
{
  gchar *p_open;
  gchar *p_close;

  p_open = strstr (markup, "<markup>");
  if (! p_open)
    return FALSE;

  *open_tag = g_strndup (markup, p_open - markup + strlen ("<markup>"));

  p_close = g_strrstr (markup, "</markup>");
  if (! p_close)
    {
      g_free (*open_tag);
      return FALSE;
    }

  *close_tag = g_strdup (p_close);

  if (p_open + strlen ("<markup>") < p_close)
    {
      *content = g_strndup (p_open + strlen ("<markup>"),
                            p_close - p_open - strlen ("<markup>"));
    }
  else
    {
      *content = g_strdup ("");
    }

  return TRUE;
}

static gchar *
gimp_text_layout_apply_tags (GimpTextLayout *layout,
                             const gchar    *markup)
{
  const Babl *format;
  GimpText   *text  = layout->text;
  gchar      *result;
  guchar      color[3];

  /* Unfortunately Pango markup are very limited, color-wise. Colors are
   * written in hexadecimal, so they are u8 as maximum precision, and
   * unbounded colors are not accessible.
   * At the very least, what we do is to write color values in the target
   * space in the PangoLayout, so that we don't end up stuck to sRGB text
   * colors even in images with wider gamut spaces.
   *
   * Moreover this is limited to RGB and Grayscale spaces. Therefore, images
   * with other backends will be limited to the sRGB gamut, for as long as Pango
   * do not evolve (or unless we changed our rendering backend).
   */
  format = gimp_text_layout_get_format (layout, "u8");
  gegl_color_get_pixel (text->color, format, color);
  if (! babl_space_is_gray (babl_format_get_space (format)))
    result = g_strdup_printf ("<span color=\"#%02x%02x%02x\">%s</span>",
                              color[0], color[1], color[2], markup);
  else
    result = g_strdup_printf ("<span color=\"#%02x%02x%02x\">%s</span>",
                              color[0], color[0], color[0], markup);

  /* Updating font 'locl' (if supported) with 'lang' feature tag */
  if (text->language)
    {
      gchar *tmp = g_strdup_printf ("<span lang=\"%s\">%s</span>",
                                    text->language,
                                    result);
      g_free (result);
      result = tmp;
    }

  if (fabs (text->letter_spacing) > 0.1)
    {
      gchar *tmp = g_strdup_printf ("<span letter_spacing=\"%d\">%s</span>",
                                    (gint) (text->letter_spacing * PANGO_SCALE),
                                    result);
      g_free (result);
      result = tmp;
    }

  return result;
}

static void
gimp_text_layout_set_markup (GimpTextLayout  *layout,
                             GError         **error)
{
  GimpText *text      = layout->text;
  gchar    *open_tag  = NULL;
  gchar    *content   = NULL;
  gchar    *close_tag = NULL;
  gchar    *tagged;
  gchar    *markup;

  if (text->markup)
    {
      if (! gimp_text_layout_split_markup (text->markup,
                                           &open_tag, &content, &close_tag))
        {
          open_tag  = g_strdup ("<markup>");
          content   = g_strdup ("");
          close_tag = g_strdup ("</markup>");
        }
    }
  else
    {
      open_tag  = g_strdup ("<markup>");
      close_tag = g_strdup ("</markup>");

      if (text->text)
        content = g_markup_escape_text (text->text, -1);
      else
        content = g_strdup ("");
    }

  tagged = gimp_text_layout_apply_tags (layout, content);

  g_free (content);

  markup = g_strconcat (open_tag, tagged, close_tag, NULL);

  g_free (open_tag);
  g_free (tagged);
  g_free (close_tag);

  if (pango_parse_markup (markup, -1, 0, NULL, NULL, NULL, error) == FALSE)
    {
      if (error && *error                       &&
          (*error)->domain == G_MARKUP_ERROR    &&
          (*error)->code == G_MARKUP_ERROR_INVALID_CONTENT)
        {
          /* Errors from pango lib are not accurate enough.
           * Other possible error codes are: G_MARKUP_ERROR_UNKNOWN_ELEMENT
           * and G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, which likely indicate a bug
           * in GIMP code or a pango library version issue.
           * G_MARKUP_ERROR_INVALID_CONTENT on the other hand likely indicates
           * size/color/style/weight/variant/etc. value issue. Font size is the
           * only free text in GIMP GUI so we assume that must be it.
           * Also we output a custom message because pango's error->message is
           * too technical (telling of <span> tags, not using user's font size
           * unit, and such). */
          g_error_free (*error);
          *error = NULL;
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("The new text layout cannot be generated. "
                                 "Most likely the font size is too big."));
        }
    }
  else
    {
      pango_layout_set_markup (layout->layout, markup, -1);
    }

  g_free (markup);
}

static void
gimp_text_layout_position (GimpTextLayout *layout)
{
  PangoRectangle  ink;
  PangoRectangle  logical;
  PangoContext   *context;
  gint            x1, y1;
  gint            x2, y2;
  gint            border;

  layout->extents.x      = 0;
  layout->extents.y      = 0;
  layout->extents.width  = 0;
  layout->extents.height = 0;

  pango_layout_get_pixel_extents (layout->layout, &ink, &logical);

  ink.width = ceil ((gdouble) ink.width * layout->xres / layout->yres);
  logical.width = ceil ((gdouble) logical.width * layout->xres / layout->yres);
  context = pango_layout_get_context (layout->layout);

#ifdef VERBOSE
  g_printerr ("ink rect: %d x %d @ %d, %d\n",
              ink.width, ink.height, ink.x, ink.y);
  g_printerr ("logical rect: %d x %d @ %d, %d\n",
              logical.width, logical.height, logical.x, logical.y);
#endif

  if (ink.width < 1 || ink.height < 1)
    {
      layout->extents.width  = 1;
      layout->extents.height = logical.height;
      return;
    }

  x1 = MIN (ink.x, logical.x);
  y1 = MIN (ink.y, logical.y);
  x2 = MAX (ink.x + ink.width,  logical.x + logical.width);
  y2 = MAX (ink.y + ink.height, logical.y + logical.height);

  layout->extents.x      = - x1;
  layout->extents.y      = - y1;
  layout->extents.width  = x2 - x1;
  layout->extents.height = y2 - y1;

  /* If the width of the layout is > 0, then the text-box is FIXED and
   * the layout position should be offset if the alignment is centered
   * or right-aligned, also adjust for RTL text direction.
   */
  if (pango_layout_get_width (layout->layout) > 0)
    {
      PangoAlignment    align    = pango_layout_get_alignment (layout->layout);
      GimpTextDirection base_dir = layout->text->base_dir;
      gint              width;

      pango_layout_get_pixel_size (layout->layout, &width, NULL);

      if ((base_dir == GIMP_TEXT_DIRECTION_LTR && align == PANGO_ALIGN_RIGHT) ||
          (base_dir == GIMP_TEXT_DIRECTION_RTL && align == PANGO_ALIGN_RIGHT) ||
          (base_dir == GIMP_TEXT_DIRECTION_TTB_RTL && align == PANGO_ALIGN_RIGHT) ||
          (base_dir == GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT && align == PANGO_ALIGN_RIGHT) ||
          (base_dir == GIMP_TEXT_DIRECTION_TTB_LTR && align == PANGO_ALIGN_LEFT) ||
          (base_dir == GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT && align == PANGO_ALIGN_LEFT))
        {
          layout->extents.x +=
            PANGO_PIXELS (pango_layout_get_width (layout->layout)) - width;
        }
      else if (align == PANGO_ALIGN_CENTER)
        {
          layout->extents.x +=
            (PANGO_PIXELS (pango_layout_get_width (layout->layout)) - width) / 2;
       }
    }

  border = (layout->text->border > 0) ? layout->text->border : 0;

  if (layout->text->outline != GIMP_TEXT_OUTLINE_NONE &&
      layout->text->outline_direction != GIMP_TEXT_OUTLINE_DIRECTION_INNER)
    border += layout->text->outline_width;

  if (border > 0)
    {
      layout->extents.x      += border;
      layout->extents.y      += border;
      layout->extents.width  += 2 * border;
      layout->extents.height += 2 * border;
    }

  if (PANGO_GRAVITY_IS_VERTICAL (pango_context_get_base_gravity (context)))
    {
      gint temp;

      temp = layout->extents.y;
      layout->extents.y = layout->extents.x;
      layout->extents.x = temp;

      temp = layout->extents.height;
      layout->extents.height = layout->extents.width;
      layout->extents.width = temp;
    }

#ifdef VERBOSE
  g_printerr ("layout extents: %d x %d @ %d, %d\n",
              layout->extents.width, layout->extents.height,
              layout->extents.x, layout->extents.y);
#endif
}

static cairo_font_options_t *
gimp_text_get_font_options (GimpText *text)
{
  cairo_font_options_t *options = cairo_font_options_create ();

  cairo_font_options_set_antialias (options, (text->antialias ?
                                              CAIRO_ANTIALIAS_GRAY :
                                              CAIRO_ANTIALIAS_NONE));

  switch (text->hint_style)
    {
    case GIMP_TEXT_HINT_STYLE_NONE:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
      break;

    case GIMP_TEXT_HINT_STYLE_SLIGHT:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_SLIGHT);
      break;

    case GIMP_TEXT_HINT_STYLE_MEDIUM:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_MEDIUM);
      break;

    case GIMP_TEXT_HINT_STYLE_FULL:
      cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_FULL);
      break;
    }

  return options;
}

static PangoContext *
gimp_text_get_pango_context (GimpText *text,
                             gdouble   xres,
                             gdouble   yres)
{
  PangoContext         *context;
  PangoFontMap         *fontmap;
  cairo_font_options_t *options;

  fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
  if (! fontmap)
    g_error ("You are using a Pango that has been built against a cairo "
             "that lacks the Freetype font backend");

  /* In case a font becomes missing mid-session and is chosen (or is already in use)
   * pango substitutes EVERY font for the default font, to avoid this
   * the FcConfig has to be set everytime a pango fontmap is created
   */
  pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (fontmap), FcConfigGetCurrent ());
  pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap), yres);

  context = pango_font_map_create_context (fontmap);
  g_object_unref (fontmap);

  options = gimp_text_get_font_options (text);
  pango_cairo_context_set_font_options (context, options);
  cairo_font_options_destroy (options);

  if (text->language)
    pango_context_set_language (context,
                                pango_language_from_string (text->language));

  switch (text->base_dir)
    {
    case GIMP_TEXT_DIRECTION_LTR:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_NATURAL);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_SOUTH);
      break;

    case GIMP_TEXT_DIRECTION_RTL:
      pango_context_set_base_dir (context, PANGO_DIRECTION_RTL);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_NATURAL);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_SOUTH);
      break;

    case GIMP_TEXT_DIRECTION_TTB_RTL:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_LINE);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_EAST);
      break;

    case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_STRONG);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_EAST);
      break;

    case GIMP_TEXT_DIRECTION_TTB_LTR:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_LINE);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_WEST);
      break;

    case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      pango_context_set_base_dir (context, PANGO_DIRECTION_LTR);
      pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_STRONG);
      pango_context_set_base_gravity (context, PANGO_GRAVITY_WEST);
      break;
    }

  return context;
}
