/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1999 Manish Singh <yosh@ligma.org>
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

#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmamath/ligmamath.h"
#include "libligmamodule/ligmamodule.h"
#include "libligmawidgets/ligmawidgets.h"

#include "libligma/libligma-intl.h"


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
  LigmaColorDisplay  parent_instance;

  gdouble           contrast;
};

struct _CdisplayContrastClass
{
  LigmaColorDisplayClass  parent_instance;
};


enum
{
  PROP_0,
  PROP_CONTRAST
};


static GType       cdisplay_contrast_get_type        (void);

static void        cdisplay_contrast_set_property    (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void        cdisplay_contrast_get_property    (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void        cdisplay_contrast_convert_buffer  (LigmaColorDisplay *display,
                                                      GeglBuffer       *buffer,
                                                      GeglRectangle    *area);
static void        cdisplay_contrast_set_contrast    (CdisplayContrast *contrast,
                                                      gdouble           value);


static const LigmaModuleInfo cdisplay_contrast_info =
{
  LIGMA_MODULE_ABI_VERSION,
  N_("High Contrast color display filter"),
  "Jay Cox <jaycox@ligma.org>",
  "v0.2",
  "(c) 2000, released under the GPL",
  "October 14, 2000"
};


G_DEFINE_DYNAMIC_TYPE (CdisplayContrast, cdisplay_contrast,
                       LIGMA_TYPE_COLOR_DISPLAY)


G_MODULE_EXPORT const LigmaModuleInfo *
ligma_module_query (GTypeModule *module)
{
  return &cdisplay_contrast_info;
}

G_MODULE_EXPORT gboolean
ligma_module_register (GTypeModule *module)
{
  cdisplay_contrast_register_type (module);

  return TRUE;
}

static void
cdisplay_contrast_class_init (CdisplayContrastClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  LigmaColorDisplayClass *display_class = LIGMA_COLOR_DISPLAY_CLASS (klass);

  object_class->get_property     = cdisplay_contrast_get_property;
  object_class->set_property     = cdisplay_contrast_set_property;

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_CONTRAST,
                           "contrast",
                           _("Contrast cycles"),
                           NULL,
                           0.01, 10.0, DEFAULT_CONTRAST,
                           0);

  display_class->name            = _("Contrast");
  display_class->help_id         = "ligma-colordisplay-contrast";
  display_class->icon_name       = LIGMA_ICON_DISPLAY_FILTER_CONTRAST;

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
cdisplay_contrast_convert_buffer (LigmaColorDisplay *display,
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
      ligma_color_display_changed (LIGMA_COLOR_DISPLAY (contrast));
    }
}
