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

#define CDISPLAY_TYPE_FILMIC            (cdisplay_filmic_get_type ())
#define CDISPLAY_FILMIC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_FILMIC, CdisplayFilmic))
#define CDISPLAY_FILMIC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_FILMIC, CdisplayFilmicClass))
#define CDISPLAY_IS_FILMIC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_FILMIC))
#define CDISPLAY_IS_FILMIC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_FILMIC))


typedef struct _CdisplayFilmic      CdisplayFilmic;
typedef struct _CdisplayFilmicClass CdisplayFilmicClass;

struct _CdisplayFilmic
{
  GimpColorDisplay  parent_instance;

  gdouble           exposure;
};

struct _CdisplayFilmicClass
{
  GimpColorDisplayClass  parent_instance;
};


enum
{
  PROP_0,
  PROP_EXPOSURE
};


static GType       cdisplay_filmic_get_type        (void);

static void        cdisplay_filmic_set_property    (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void        cdisplay_filmic_get_property    (GObject            *object,
                                                   guint               property_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);

static void        cdisplay_filmic_convert_buffer  (GimpColorDisplay   *display,
                                                   GeglBuffer         *buffer,
                                                   GeglRectangle      *area);
static void        cdisplay_filmic_set_exposure    (CdisplayFilmic      *filmic,
                                                   gdouble             value);


static const GimpModuleInfo cdisplay_filmic_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Filmic HDR to SDR proof color display filter, usin an approximation of the ACES filmic transform."),
  "Øyvind Kolås <pippin@gimp.org>",
  "v0.2",
  "(c) 2018, released under the GPL",
  "July 17, 2018"
};


G_DEFINE_DYNAMIC_TYPE (CdisplayFilmic, cdisplay_filmic,
                       GIMP_TYPE_COLOR_DISPLAY)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_filmic_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_filmic_register_type (module);

  return TRUE;
}

static void
cdisplay_filmic_class_init (CdisplayFilmicClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  object_class->get_property     = cdisplay_filmic_get_property;
  object_class->set_property     = cdisplay_filmic_set_property;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_EXPOSURE,
                           "exposure",
                           _("pre-transform change in stops"),
                           NULL,
                           -10.0, 10.0, DEFAULT_EXPOSURE,
                           1);

  display_class->name            = _("Filmic");
  display_class->help_id         = "gimp-colordisplay-filmic";
  display_class->icon_name       = GIMP_ICON_DISPLAY_FILTER_GAMMA;

  display_class->convert_buffer  = cdisplay_filmic_convert_buffer;
}

static void
cdisplay_filmic_class_finalize (CdisplayFilmicClass *klass)
{
}

static void
cdisplay_filmic_init (CdisplayFilmic *filmic)
{
}

static void
cdisplay_filmic_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CdisplayFilmic *filmic = CDISPLAY_FILMIC (object);

  switch (property_id)
    {
    case PROP_EXPOSURE:
      g_value_set_double (value, filmic->exposure);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_filmic_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CdisplayFilmic *filmic = CDISPLAY_FILMIC (object);

  switch (property_id)
    {
    case PROP_EXPOSURE:
      cdisplay_filmic_set_exposure (filmic, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static inline float aces_filmic (float x)
{
 /*
    Approximation of the ACES filmic HDR to SDR mapping from:
    https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
  */
 float a = x * (x + 0.0245786f) - 0.000090537f;
 float b = x * (0.983729f * x + 0.4329510f) + 0.238081f;
 return a / b;
}

static void
cdisplay_filmic_convert_buffer (GimpColorDisplay *display,
                               GeglBuffer       *buffer,
                               GeglRectangle    *area)
{
  CdisplayFilmic     *filter = CDISPLAY_FILMIC (display);
  GeglBufferIterator *iter;
  float gain = 1.0f / exp2f(-filter->exposure);

  iter = gegl_buffer_iterator_new (buffer, area, 0,
                                   babl_format ("RGBA float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data  = iter->data[0];
      gint    count = iter->length;

      while (count--)
        {
          *data = aces_filmic (*data * gain); data++;
          *data = aces_filmic (*data * gain); data++;
          *data = aces_filmic (*data * gain); data++;
          data++;
        }
    }
}

static void
cdisplay_filmic_set_exposure (CdisplayFilmic *filmic,
                              gdouble        value)
{
  if (value != filmic->exposure)
    {
      filmic->exposure = value;

      g_object_notify (G_OBJECT (filmic), "exposure");
      gimp_color_display_changed (GIMP_COLOR_DISPLAY (filmic));
    }
}
