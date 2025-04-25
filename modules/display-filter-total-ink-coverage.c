/* GIMP - The GNU Image Manipulation Program
 *
 * Copyright (C) 2025 Alx Sa
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


#define DEFAULT_INK_COLOR    ((gdouble[]) {0.0, 1.00, 0.00, 1.00})


#define CDISPLAY_TYPE_TOTAL_INK_COVERAGE            (cdisplay_total_ink_coverage_get_type ())
#define CDISPLAY_TOTAL_INK_COVERAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_TOTAL_INK_COVERAGE, CdisplayTotalInkCoverage))
#define CDISPLAY_TOTAL_INK_COVERAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_TOTAL_INK_COVERAGE, CdisplayTotalInkCoverageClass))
#define CDISPLAY_IS_TOTAL_INK_COVERAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_TOTAL_INK_COVERAGE))
#define CDISPLAY_IS_TOTAL_INK_COVERAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_TOTAL_INK_COVERAGE))


typedef struct _CdisplayTotalInkCoverage      CdisplayTotalInkCoverage;
typedef struct _CdisplayTotalInkCoverageClass CdisplayTotalInkCoverageClass;

struct _CdisplayTotalInkCoverage
{
  GimpColorDisplay  parent_instance;

  gint              ink_limit;
  GeglColor        *ink_color;

  gfloat            colors[5];
};

struct _CdisplayTotalInkCoverageClass
{
  GimpColorDisplayClass  parent_instance;
};


enum
{
  PROP_0,
  PROP_INK_LIMIT,
  PROP_INK_COLOR,
};


GType          cdisplay_total_ink_coverage_get_type       (void);

static void    cdisplay_total_ink_coverage_finalize       (GObject                  *object);
static void    cdisplay_total_ink_coverage_set_property   (GObject                  *object,
                                                           guint                     property_id,
                                                           const GValue             *value,
                                                           GParamSpec               *pspec);
static void    cdisplay_total_ink_coverage_get_property   (GObject                  *object,
                                                           guint                     property_id,
                                                           GValue                   *value,
                                                           GParamSpec               *pspec);

static void    cdisplay_total_ink_coverage_convert_buffer (GimpColorDisplay         *display,
                                                           GeglBuffer               *buffer,
                                                           GeglRectangle            *area);

static void    cdisplay_total_ink_coverage_update_colors  (CdisplayTotalInkCoverage *total_ink_coverage);


static const GimpModuleInfo cdisplay_total_ink_coverage_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Total Ink Coverage color display filter"),
  "Alx Sa",
  "v1.0",
  "(c) 2025, released under the GPL",
  "2017"
};


G_DEFINE_DYNAMIC_TYPE (CdisplayTotalInkCoverage, cdisplay_total_ink_coverage,
                       GIMP_TYPE_COLOR_DISPLAY)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_total_ink_coverage_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_total_ink_coverage_register_type (module);

  return TRUE;
}

static void
cdisplay_total_ink_coverage_class_init (CdisplayTotalInkCoverageClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);
  GeglColor             *color         = gegl_color_new (NULL);

  object_class->finalize     = cdisplay_total_ink_coverage_finalize;
  object_class->get_property = cdisplay_total_ink_coverage_get_property;
  object_class->set_property = cdisplay_total_ink_coverage_set_property;

  GIMP_CONFIG_PROP_INT (object_class, PROP_INK_LIMIT,
                        "ink-limit",
                        _("Limit (%)"),
                        _("Show warning if combined CMYK values are over "
                          "this limit."),
                        0, 400, 240,
                        GIMP_PARAM_STATIC_STRINGS);

  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"),
                        DEFAULT_INK_COLOR);
  GIMP_CONFIG_PROP_COLOR (object_class, PROP_INK_COLOR,
                          "ink-color",
                          _("Color"),
                          _("Total Ink Coverage warning color"),
                          FALSE, color,
                          GIMP_PARAM_STATIC_STRINGS |
                          GIMP_CONFIG_PARAM_DEFAULTS);

  display_class->name            = _("Total Ink Coverage");
  display_class->help_id         = "gimp-colordisplay-total-ink-coverage";
  display_class->icon_name       = GIMP_ICON_COLOR_SELECTOR_CMYK;

  display_class->convert_buffer  = cdisplay_total_ink_coverage_convert_buffer;

  g_object_unref (color);
}

static void
cdisplay_total_ink_coverage_class_finalize (CdisplayTotalInkCoverageClass *klass)
{
}

static void
cdisplay_total_ink_coverage_init (CdisplayTotalInkCoverage *total_ink_coverage)
{
  GeglColor *color;

  color = gegl_color_new (NULL);
  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"),
                        DEFAULT_INK_COLOR);
  total_ink_coverage->ink_color = color;

  cdisplay_total_ink_coverage_update_colors (total_ink_coverage);
}

static void
cdisplay_total_ink_coverage_finalize (GObject *object)
{
  CdisplayTotalInkCoverage *total_ink_coverage = CDISPLAY_TOTAL_INK_COVERAGE (object);

  g_clear_object (&total_ink_coverage->ink_color);
}

static void
cdisplay_total_ink_coverage_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  CdisplayTotalInkCoverage *total_ink_coverage = CDISPLAY_TOTAL_INK_COVERAGE (object);

  switch (property_id)
    {
    case PROP_INK_LIMIT:
      g_value_set_int (value, total_ink_coverage->ink_limit);
      break;
    case PROP_INK_COLOR:
      g_value_set_object (value, total_ink_coverage->ink_color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_total_ink_coverage_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  CdisplayTotalInkCoverage *total_ink_coverage = CDISPLAY_TOTAL_INK_COVERAGE (object);

  switch (property_id)
    {
    case PROP_INK_LIMIT:
      total_ink_coverage->ink_limit = g_value_get_int (value);
      break;
    case PROP_INK_COLOR:
      g_clear_object (&total_ink_coverage->ink_color);
      total_ink_coverage->ink_color = gegl_color_duplicate (g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

    if (property_id == PROP_INK_LIMIT ||
        property_id == PROP_INK_COLOR)
      {
        if (property_id == PROP_INK_COLOR)
          cdisplay_total_ink_coverage_update_colors (total_ink_coverage);

        g_object_notify (G_OBJECT (total_ink_coverage), pspec->name);
        gimp_color_display_changed (GIMP_COLOR_DISPLAY (total_ink_coverage));
      }
}

static void
cdisplay_total_ink_coverage_convert_buffer (GimpColorDisplay *display,
                                            GeglBuffer       *buffer,
                                            GeglRectangle    *area)
{
  CdisplayTotalInkCoverage *total_ink_coverage;
  GeglBufferIterator       *iter;
  GimpColorConfig          *config       = NULL;
  GimpColorManaged         *managed      = NULL;
  GimpColorProfile         *cmyk_profile = NULL;
  GimpColorRenderingIntent  intent;
  const Babl               *space        = NULL;

  total_ink_coverage = CDISPLAY_TOTAL_INK_COVERAGE (display);

  intent  = GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  config  = gimp_color_display_get_config (display);
  managed = gimp_color_display_get_managed (display);

  /* Try Image soft-proofing profile first, then default CMYK profile */
  if (managed)
    {
      cmyk_profile = gimp_color_managed_get_simulation_profile (managed);
      intent       = gimp_color_managed_get_simulation_intent (managed);
    }

  if (! cmyk_profile && config)
    {
      cmyk_profile = gimp_color_config_get_cmyk_color_profile (config, NULL);
      intent       = gimp_color_config_get_simulation_intent (config);
    }

  if (cmyk_profile && gimp_color_profile_is_cmyk (cmyk_profile))
    space = gimp_color_profile_get_space (cmyk_profile, intent, NULL);

  iter = gegl_buffer_iterator_new (buffer, area, 0,
                                   babl_format_with_space ("CMYKA float", space),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data  = iter->items[0].data;
      gint    count = iter->length;

      while (count--)
        {
          gboolean warning = FALSE;

          if (! (data[4] <= 0.0f))
            {
              gfloat total = (data[0] + data[1] + data[2] + data[3]) * 100.0;

              if (total > total_ink_coverage->ink_limit)
                warning = TRUE;
            }

          if (warning)
            memcpy (data, total_ink_coverage->colors,
                    5 * sizeof (gfloat));

          data += 5;
        }
    }
}

static void
cdisplay_total_ink_coverage_update_colors (CdisplayTotalInkCoverage *total_ink_coverage)
{
  gfloat cmyka[5];
  GimpColorConfig          *config       = NULL;
  GimpColorManaged         *managed      = NULL;
  GimpColorProfile         *cmyk_profile = NULL;
  GimpColorRenderingIntent  intent;
  const Babl               *space        = NULL;

  config =
    gimp_color_display_get_config (GIMP_COLOR_DISPLAY (total_ink_coverage));
  managed =
    gimp_color_display_get_managed (GIMP_COLOR_DISPLAY (total_ink_coverage));

  intent = GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  /* Try Image Soft-proofing profile first, then default CMYK profile */
  if (managed)
    {
      cmyk_profile = gimp_color_managed_get_simulation_profile (managed);
      intent       = gimp_color_managed_get_simulation_intent (managed);
    }

  if (! cmyk_profile && config)
    {
      cmyk_profile = gimp_color_config_get_cmyk_color_profile (config, NULL);
      intent       = gimp_color_config_get_simulation_intent (config);
    }

  if (cmyk_profile && gimp_color_profile_is_cmyk (cmyk_profile))
    space = gimp_color_profile_get_space (cmyk_profile, intent, NULL);

  gegl_color_get_pixel (total_ink_coverage->ink_color,
                        babl_format_with_space ("CMYKA float", space),
                        cmyka);

  memcpy (total_ink_coverage->colors, cmyka,
          5 * sizeof (gfloat));
}
