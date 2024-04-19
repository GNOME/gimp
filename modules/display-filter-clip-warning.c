/* GIMP - The GNU Image Manipulation Program
 *
 * Copyright (C) 2017 Ell
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
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define DEFAULT_SHADOWS_COLOR    ((gdouble[]) {0.25, 0.25, 1.00, 1.00})
#define DEFAULT_HIGHLIGHTS_COLOR ((gdouble[]) {1.00, 0.25, 0.25, 1.00})
#define DEFAULT_BOGUS_COLOR      ((gdouble[]) {1.00, 1.00, 0.25, 1.00})


#define CDISPLAY_TYPE_CLIP_WARNING            (cdisplay_clip_warning_get_type ())
#define CDISPLAY_CLIP_WARNING(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_CLIP_WARNING, CdisplayClipWarning))
#define CDISPLAY_CLIP_WARNING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_CLIP_WARNING, CdisplayClipWarningClass))
#define CDISPLAY_IS_CLIP_WARNING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_CLIP_WARNING))
#define CDISPLAY_IS_CLIP_WARNING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_CLIP_WARNING))


typedef enum
{
  WARNING_SHADOW    = 1 << 0,
  WARNING_HIGHLIGHT = 1 << 1,
  WARNING_BOGUS     = 1 << 2
} Warning;


typedef struct _CdisplayClipWarning      CdisplayClipWarning;
typedef struct _CdisplayClipWarningClass CdisplayClipWarningClass;

struct _CdisplayClipWarning
{
  GimpColorDisplay  parent_instance;

  gboolean          show_shadows;
  GeglColor        *shadows_color;
  gboolean          show_highlights;
  GeglColor        *highlights_color;
  gboolean          show_bogus;
  GeglColor        *bogus_color;
  gboolean          include_alpha;
  gboolean          include_transparent;

  gfloat            colors[8][2][4];
};

struct _CdisplayClipWarningClass
{
  GimpColorDisplayClass  parent_instance;
};


enum
{
  PROP_0,
  PROP_SHOW_SHADOWS,
  PROP_SHADOWS_COLOR,
  PROP_SHOW_HIGHLIGHTS,
  PROP_HIGHLIGHTS_COLOR,
  PROP_SHOW_BOGUS,
  PROP_BOGUS_COLOR,
  PROP_INCLUDE_ALPHA,
  PROP_INCLUDE_TRANSPARENT
};


GType          cdisplay_clip_warning_get_type       (void);

static void    cdisplay_clip_warning_finalize       (GObject             *object);
static void    cdisplay_clip_warning_set_property   (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void    cdisplay_clip_warning_get_property   (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static void    cdisplay_clip_warning_convert_buffer (GimpColorDisplay    *display,
                                                     GeglBuffer          *buffer,
                                                     GeglRectangle       *area);

static void    cdisplay_clip_warning_set_member     (CdisplayClipWarning *clip_warning,
                                                     const gchar         *property_name,
                                                     gpointer             member,
                                                     gconstpointer        value,
                                                     gsize                size);
static void    cdisplay_clip_warning_update_colors  (CdisplayClipWarning *clip_warning);


static const GimpModuleInfo cdisplay_clip_warning_info =
{
  GIMP_MODULE_ABI_VERSION,
  /* Translators: "Clip Warning" is the name of a (color) display filter
   * that highlights pixels outside of the color space range.
   * Shown as a label description. */
  N_("Clip warning color display filter"),
  "Ell",
  "v1.0",
  "(c) 2017, released under the GPL",
  "2017"
};


G_DEFINE_DYNAMIC_TYPE (CdisplayClipWarning, cdisplay_clip_warning,
                       GIMP_TYPE_COLOR_DISPLAY)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_clip_warning_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_clip_warning_register_type (module);

  return TRUE;
}

static void
cdisplay_clip_warning_class_init (CdisplayClipWarningClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);
  GeglColor             *color         = gegl_color_new (NULL);

  object_class->finalize     = cdisplay_clip_warning_finalize;
  object_class->get_property = cdisplay_clip_warning_get_property;
  object_class->set_property = cdisplay_clip_warning_set_property;

  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"),
                        DEFAULT_SHADOWS_COLOR);
  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SHADOWS,
                            "show-shadows",
                            _("Show shadows"),
                            _("Show warning for pixels with a negative component"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

   GIMP_CONFIG_PROP_COLOR (object_class, PROP_SHADOWS_COLOR,
                           "shadows-color",
                           _("Shadows color"),
                           _("Shadows warning color"),
                           FALSE, color,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_DEFAULTS);

  gegl_param_spec_set_property_key (
    g_object_class_find_property (G_OBJECT_CLASS (klass), "shadows-color"),
    "sensitive", "show-shadows");

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_HIGHLIGHTS,
                            "show-highlights",
                            _("Show highlights"),
                            _("Show warning for pixels with a component greater than one"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"),
                        DEFAULT_HIGHLIGHTS_COLOR);
  GIMP_CONFIG_PROP_COLOR (object_class, PROP_HIGHLIGHTS_COLOR,
                          "highlights-color",
                          _("Highlights color"),
                          _("Highlights warning color"),
                          FALSE, color,
                          GIMP_PARAM_STATIC_STRINGS |
                          GIMP_CONFIG_PARAM_DEFAULTS);

  gegl_param_spec_set_property_key (
    g_object_class_find_property (G_OBJECT_CLASS (klass), "highlights-color"),
    "sensitive", "show-highlights");

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_BOGUS,
                            "show-bogus",
                            _("Show bogus"),
                            _("Show warning for pixels with an infinite or NaN component"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"),
                        DEFAULT_BOGUS_COLOR);
  GIMP_CONFIG_PROP_COLOR (object_class, PROP_BOGUS_COLOR,
                          "bogus-color",
                          _("Bogus color"),
                          _("Bogus warning color"),
                          FALSE, color,
                          GIMP_PARAM_STATIC_STRINGS |
                          GIMP_CONFIG_PARAM_DEFAULTS);

  gegl_param_spec_set_property_key (
    g_object_class_find_property (G_OBJECT_CLASS (klass), "bogus-color"),
    "sensitive", "show-bogus");

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_INCLUDE_ALPHA,
                            "include-alpha",
                            _("Include alpha component"),
                            _("Include alpha component in the warning"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_INCLUDE_TRANSPARENT,
                            "include-transparent",
                            _("Include transparent pixels"),
                            _("Include fully transparent pixels in the warning"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  display_class->name            = _("Clip Warning");
  display_class->help_id         = "gimp-colordisplay-clip-warning";
  display_class->icon_name       = GIMP_ICON_DISPLAY_FILTER_CLIP_WARNING;

  display_class->convert_buffer  = cdisplay_clip_warning_convert_buffer;

  g_object_unref (color);
}

static void
cdisplay_clip_warning_class_finalize (CdisplayClipWarningClass *klass)
{
}

static void
cdisplay_clip_warning_init (CdisplayClipWarning *clip_warning)
{
  GeglColor *color;

  color = gegl_color_new (NULL);
  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"),
                        DEFAULT_SHADOWS_COLOR);
  clip_warning->shadows_color = color;

  color = gegl_color_new (NULL);
  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"),
                        DEFAULT_HIGHLIGHTS_COLOR);
  clip_warning->highlights_color = color;

  color = gegl_color_new (NULL);
  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"),
                        DEFAULT_BOGUS_COLOR);
  clip_warning->bogus_color = color;
}

static void
cdisplay_clip_warning_finalize (GObject *object)
{
  CdisplayClipWarning *clip_warning = CDISPLAY_CLIP_WARNING (object);

  g_clear_object (&clip_warning->shadows_color);
  g_clear_object (&clip_warning->highlights_color);
  g_clear_object (&clip_warning->bogus_color);
}

static void
cdisplay_clip_warning_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  CdisplayClipWarning *clip_warning = CDISPLAY_CLIP_WARNING (object);

  switch (property_id)
    {
    case PROP_SHOW_SHADOWS:
      g_value_set_boolean (value, clip_warning->show_shadows);
      break;
    case PROP_SHADOWS_COLOR:
      g_value_set_object (value, clip_warning->shadows_color);
      break;

    case PROP_SHOW_HIGHLIGHTS:
      g_value_set_boolean (value, clip_warning->show_highlights);
      break;
    case PROP_HIGHLIGHTS_COLOR:
      g_value_set_object (value, clip_warning->highlights_color);
      break;

    case PROP_SHOW_BOGUS:
      g_value_set_boolean (value, clip_warning->show_bogus);
      break;
    case PROP_BOGUS_COLOR:
      g_value_set_object (value, clip_warning->bogus_color);
      break;

    case PROP_INCLUDE_ALPHA:
      g_value_set_boolean (value, clip_warning->include_alpha);
      break;
    case PROP_INCLUDE_TRANSPARENT:
      g_value_set_boolean (value, clip_warning->include_transparent);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_clip_warning_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  CdisplayClipWarning *clip_warning = CDISPLAY_CLIP_WARNING (object);

#define SET_MEMBER_PTR(member, value)                                          \
  cdisplay_clip_warning_set_member (clip_warning,                              \
                                    pspec->name,                               \
                                    &clip_warning->member,                     \
                                    (value),                                   \
                                    sizeof (clip_warning->member))
#define SET_MEMBER_VAL(member, type, value)                                    \
  SET_MEMBER_PTR (member, &(type) {value})

  switch (property_id)
    {
    case PROP_SHOW_SHADOWS:
      SET_MEMBER_VAL (show_shadows, gboolean, g_value_get_boolean (value));
      break;
    case PROP_SHADOWS_COLOR:
      g_clear_object (&clip_warning->shadows_color);
      clip_warning->shadows_color = gegl_color_duplicate (g_value_get_object (value));
      break;

    case PROP_SHOW_HIGHLIGHTS:
      SET_MEMBER_VAL (show_highlights, gboolean, g_value_get_boolean (value));
      break;
    case PROP_HIGHLIGHTS_COLOR:
      g_clear_object (&clip_warning->highlights_color);
      clip_warning->highlights_color = gegl_color_duplicate (g_value_get_object (value));
      break;

    case PROP_SHOW_BOGUS:
      SET_MEMBER_VAL (show_bogus, gboolean, g_value_get_boolean (value));
      break;
    case PROP_BOGUS_COLOR:
      g_clear_object (&clip_warning->bogus_color);
      clip_warning->bogus_color = gegl_color_duplicate (g_value_get_object (value));
      break;

    case PROP_INCLUDE_ALPHA:
      SET_MEMBER_VAL (include_alpha, gboolean, g_value_get_boolean (value));
      break;
    case PROP_INCLUDE_TRANSPARENT:
      SET_MEMBER_VAL (include_transparent, gboolean, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

    if (property_id == PROP_SHADOWS_COLOR    ||
        property_id == PROP_HIGHLIGHTS_COLOR ||
        property_id == PROP_BOGUS_COLOR)
      {
        cdisplay_clip_warning_update_colors (clip_warning);
        g_object_notify (G_OBJECT (clip_warning), pspec->name);
        gimp_color_display_changed (GIMP_COLOR_DISPLAY (clip_warning));
      }

#undef SET_MEMBER_PTR
#undef SET_MEMBER_VAL
}

static void
cdisplay_clip_warning_convert_buffer (GimpColorDisplay *display,
                                      GeglBuffer       *buffer,
                                      GeglRectangle    *area)
{
  CdisplayClipWarning *clip_warning = CDISPLAY_CLIP_WARNING (display);
  GeglBufferIterator  *iter;

  iter = gegl_buffer_iterator_new (buffer, area, 0,
                                   babl_format ("R'G'B'A float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data  = iter->items[0].data;
      gint    count = iter->length;
      gint    x     = iter->items[0].roi.x;
      gint    y     = iter->items[0].roi.y;

      while (count--)
        {
          gint warning = 0;

          if (clip_warning->include_transparent ||
              ! (data[3] <= 0.0f) /* include nan */)
            {
              if (clip_warning->show_bogus                                              &&
                  (! isfinite (data[0]) || ! isfinite (data[1]) || ! isfinite (data[2]) ||
                   (clip_warning->include_alpha && ! isfinite (data[3]))))
                {
                  /* don't combine warning color of pixels with a bogus
                   * component with other warnings
                   */
                  warning = WARNING_BOGUS;
                }
              else
                {
                  if (clip_warning->show_shadows                          &&
                      (data[0] < 0.0f || data[1] < 0.0f || data[2] < 0.0f ||
                       (clip_warning->include_alpha && data[3] < 0.0f)))
                    {
                      warning |= WARNING_SHADOW;
                    }

                  if (clip_warning->show_highlights                       &&
                      (data[0] > 1.0f || data[1] > 1.0f || data[2] > 1.0f ||
                       (clip_warning->include_alpha && data[3] > 1.0f)))
                    {
                      warning |= WARNING_HIGHLIGHT;
                    }
                }
            }

          if (warning)
            {
              gboolean alt = ((x + y) >> 3) & 1;

              memcpy (data, clip_warning->colors[warning][alt],
                      4 * sizeof (gfloat));
            }

          data += 4;

          if (++x == iter->items[0].roi.x + iter->items[0].roi.width)
            {
              x = iter->items[0].roi.x;
              y++;
            }
        }
    }
}

static void
cdisplay_clip_warning_set_member (CdisplayClipWarning *clip_warning,
                                  const gchar         *property_name,
                                  gpointer             member,
                                  gconstpointer        value,
                                  gsize                size)
{
  if (memcmp (member, value, size))
    {
      memcpy (member, value, size);

      cdisplay_clip_warning_update_colors (clip_warning);

      g_object_notify (G_OBJECT (clip_warning), property_name);
      gimp_color_display_changed (GIMP_COLOR_DISPLAY (clip_warning));
    }
}

static void
cdisplay_clip_warning_update_colors (CdisplayClipWarning *clip_warning)
{
  gint i;
  gint j;

  for (i = 0; i < 8; i++)
    {
      gfloat *color     = clip_warning->colors[i][0];
      gfloat *alt_color = clip_warning->colors[i][1];
      gfloat  alt_value;
      gint    n         = 0;
      gfloat  rgb[3];

      memset (color, 0, 3 * sizeof (gfloat));

      if (i & WARNING_SHADOW)
        {
          gegl_color_get_pixel (clip_warning->shadows_color,
                                babl_format ("R'G'B' float"),
                                rgb);
          color[0] += rgb[0];
          color[1] += rgb[1];
          color[2] += rgb[2];

          n++;
        }

      if (i & WARNING_HIGHLIGHT)
        {
          gegl_color_get_pixel (clip_warning->highlights_color,
                                babl_format ("R'G'B' float"),
                                rgb);
          color[0] += rgb[0];
          color[1] += rgb[1];
          color[2] += rgb[2];

          n++;
        }

      if (i & WARNING_BOGUS)
        {
          gegl_color_get_pixel (clip_warning->bogus_color,
                                babl_format ("R'G'B' float"),
                                rgb);
          color[0] += rgb[0];
          color[1] += rgb[1];
          color[2] += rgb[2];

          n++;
        }

      if (n)
        {
          for (j = 0; j < 3; j++)
            color[j] /= n;
        }
      color[3] = 1.0;

      if (MAX (color[0], MAX (color[1], color[2])) <= 0.5)
        alt_value = 1.0;
      else
        alt_value = 0.0;

      for (j = 0; j < 3; j++)
        alt_color[j] = 0.75 * color[j] + 0.25 * alt_value;
      alt_color[3] = 1.0;
    }
}
