/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1999 Manish Singh <yosh@gimp.org>
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


#define DEFAULT_CONTRAST 1.0


#define CDISPLAY_TYPE_CONTRAST            (cdisplay_contrast_get_type ())
#define CDISPLAY_CONTRAST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_CONTRAST, CdisplayContrast))
#define CDISPLAY_CONTRAST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_CONTRAST, CdisplayContrastClass))
#define CDISPLAY_IS_CONTRAST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_CONTRAST))
#define CDISPLAY_IS_CONTRAST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_CONTRAST))


typedef struct _CdisplayContrast      CdisplayContrast;
typedef struct _CdisplayContrastClass CdisplayContrastClass;

struct _CdisplayContrast
{
  GimpColorDisplay  parent_instance;

  gdouble           contrast;
};

struct _CdisplayContrastClass
{
  GimpColorDisplayClass  parent_instance;
};


enum
{
  PROP_0,
  PROP_CONTRAST
};


GType              cdisplay_contrast_get_type        (void);

static void        cdisplay_contrast_set_property    (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void        cdisplay_contrast_get_property    (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void        cdisplay_contrast_convert_buffer  (GimpColorDisplay *display,
                                                      GeglBuffer       *buffer,
                                                      GeglRectangle    *area);
static void        cdisplay_contrast_set_contrast    (CdisplayContrast *contrast,
                                                      gdouble           value);


static const GimpModuleInfo cdisplay_contrast_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("High Contrast color display filter"),
  "Jay Cox <jaycox@gimp.org>",
  "v0.2",
  "(c) 2000, released under the GPL",
  "October 14, 2000"
};


G_DEFINE_DYNAMIC_TYPE (CdisplayContrast, cdisplay_contrast,
                       GIMP_TYPE_COLOR_DISPLAY)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_contrast_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_contrast_register_type (module);

  return TRUE;
}

static void
cdisplay_contrast_class_init (CdisplayContrastClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  object_class->get_property     = cdisplay_contrast_get_property;
  object_class->set_property     = cdisplay_contrast_set_property;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_CONTRAST,
                           "contrast",
                           _("Contrast cycles"),
                           NULL,
                           0.01, 10.0, DEFAULT_CONTRAST,
                           0);

  display_class->name            = _("Contrast");
  display_class->help_id         = "gimp-colordisplay-contrast";
  display_class->icon_name       = GIMP_ICON_DISPLAY_FILTER_CONTRAST;

  display_class->convert_buffer  = cdisplay_contrast_convert_buffer;
}

static void
cdisplay_contrast_class_finalize (CdisplayContrastClass *klass)
{
}

static void
cdisplay_contrast_init (CdisplayContrast *contrast)
{
}

static void
cdisplay_contrast_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  CdisplayContrast *contrast = CDISPLAY_CONTRAST (object);

  switch (property_id)
    {
    case PROP_CONTRAST:
      g_value_set_double (value, contrast->contrast);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_contrast_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  CdisplayContrast *contrast = CDISPLAY_CONTRAST (object);

  switch (property_id)
    {
    case PROP_CONTRAST:
      cdisplay_contrast_set_contrast (contrast, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_contrast_convert_buffer (GimpColorDisplay *display,
                                  GeglBuffer       *buffer,
                                  GeglRectangle    *area)
{
  CdisplayContrast   *contrast = CDISPLAY_CONTRAST (display);
  GeglBufferIterator *iter;
  gfloat              c;

  c = contrast->contrast * 2 * G_PI;

  iter = gegl_buffer_iterator_new (buffer, area, 0,
                                   babl_format ("R'G'B'A float"),
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *data  = iter->items[0].data;
      gint    count = iter->length;

      while (count--)
        {
          *data = 0.5 * (1.0 + sin (c * *data)); data++;
          *data = 0.5 * (1.0 + sin (c * *data)); data++;
          *data = 0.5 * (1.0 + sin (c * *data)); data++;

          data++;
        }
    }
}

static void
cdisplay_contrast_set_contrast (CdisplayContrast *contrast,
                                gdouble           value)
{
  if (value <= 0.0)
    value = 1.0;

  if (value != contrast->contrast)
    {
      contrast->contrast = value;

      g_object_notify (G_OBJECT (contrast), "contrast");
      gimp_color_display_changed (GIMP_COLOR_DISPLAY (contrast));
    }
}
