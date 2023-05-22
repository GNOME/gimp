/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2018 Øyvind Kolås <pippin@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"

#define DEFAULT_EXPOSURE 0.0

#define CDISPLAY_TYPE_ACES_RRT            (cdisplay_aces_rrt_get_type ())
#define CDISPLAY_ACES_RRT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_ACES_RRT, CdisplayAcesRRT))
#define CDISPLAY_ACES_RRT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_ACES_RRT, CdisplayAcesRRTClass))
#define CDISPLAY_IS_ACES_RRT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_ACES_RRT))
#define CDISPLAY_IS_ACES_RRT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_ACES_RRT))


typedef struct _CdisplayAcesRRT      CdisplayAcesRRT;
typedef struct _CdisplayAcesRRTClass CdisplayAcesRRTClass;

struct _CdisplayAcesRRT
{
  GimpColorDisplay  parent_instance;

  gdouble           exposure;
};

struct _CdisplayAcesRRTClass
{
  GimpColorDisplayClass  parent_instance;
};


enum
{
  PROP_0,
  PROP_EXPOSURE
};


GType              cdisplay_aces_rrt_get_type        (void);

static void        cdisplay_aces_rrt_set_property    (GObject            *object,
                                                      guint               property_id,
                                                      const GValue       *value,
                                                      GParamSpec         *pspec);
static void        cdisplay_aces_rrt_get_property    (GObject            *object,
                                                      guint               property_id,
                                                      GValue             *value,
                                                      GParamSpec         *pspec);

static void        cdisplay_aces_rrt_convert_buffer  (GimpColorDisplay   *display,
                                                      GeglBuffer         *buffer,
                                                      GeglRectangle      *area);
static void        cdisplay_aces_rrt_set_exposure    (CdisplayAcesRRT      *aces_rrt,
                                                      gdouble             value);


static const GimpModuleInfo cdisplay_aces_rrt_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("ACES RRT (RRT = Reference Rendering Transform). An HDR to SDR proof color display filter, using a luminance-only approximation of the ACES RRT, a pre-defined filmic look to be used before ODT (display or output space ICC profile)"),
  "Øyvind Kolås <pippin@gimp.org>",
  "v0.1",
  "(c) 2018, released under the LGPLv2+",
  "July 17, 2018"
};


G_DEFINE_DYNAMIC_TYPE (CdisplayAcesRRT, cdisplay_aces_rrt,
                       GIMP_TYPE_COLOR_DISPLAY)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_aces_rrt_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_aces_rrt_register_type (module);

  return TRUE;
}

static void
cdisplay_aces_rrt_class_init (CdisplayAcesRRTClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  object_class->get_property     = cdisplay_aces_rrt_get_property;
  object_class->set_property     = cdisplay_aces_rrt_set_property;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_EXPOSURE,
                           "exposure",
                           _("Pre-transform change in stops"),
                           NULL,
                           -10.0, 10.0, DEFAULT_EXPOSURE,
                           1);

  display_class->name            = _("Aces RRT");
  display_class->help_id         = "gimp-colordisplay-aces-rrt";
  display_class->icon_name       = GIMP_ICON_DISPLAY_FILTER_GAMMA;

  display_class->convert_buffer  = cdisplay_aces_rrt_convert_buffer;
}

static void
cdisplay_aces_rrt_class_finalize (CdisplayAcesRRTClass *klass)
{
}

static void
cdisplay_aces_rrt_init (CdisplayAcesRRT *aces_rrt)
{
}

static void
cdisplay_aces_rrt_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  CdisplayAcesRRT *aces_rrt = CDISPLAY_ACES_RRT (object);

  switch (property_id)
    {
    case PROP_EXPOSURE:
      g_value_set_double (value, aces_rrt->exposure);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_aces_rrt_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  CdisplayAcesRRT *aces_rrt = CDISPLAY_ACES_RRT (object);

  switch (property_id)
    {
    case PROP_EXPOSURE:
      cdisplay_aces_rrt_set_exposure (aces_rrt, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static inline float aces_aces_rrt (float x)
{
 /*
    Approximation of the ACES aces_rrt HDR to SDR mapping from:
    https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
  */
 float a = x * (x + 0.0245786f) - 0.000090537f;
 float b = x * (0.983729f * x + 0.4329510f) + 0.238081f;
 return a / b;
}

static void
cdisplay_aces_rrt_convert_buffer (GimpColorDisplay *display,
                                  GeglBuffer       *buffer,
                                  GeglRectangle    *area)
{
  CdisplayAcesRRT     *filter = CDISPLAY_ACES_RRT (display);
  GeglBufferIterator *iter;
  float gain = 1.0f / exp2f(-filter->exposure);

  iter = gegl_buffer_iterator_new (buffer, area, 0,
                                   babl_format ("RGBA float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data  = iter->items[0].data;
      gint    count = iter->length;

      while (count--)
        {
          *data = aces_aces_rrt (*data * gain); data++;
          *data = aces_aces_rrt (*data * gain); data++;
          *data = aces_aces_rrt (*data * gain); data++;
          data++;
        }
    }
}

static void
cdisplay_aces_rrt_set_exposure (CdisplayAcesRRT *aces_rrt,
                                gdouble          value)
{
  if (value != aces_rrt->exposure)
    {
      aces_rrt->exposure = value;

      g_object_notify (G_OBJECT (aces_rrt), "exposure");
      gimp_color_display_changed (GIMP_COLOR_DISPLAY (aces_rrt));
    }
}
