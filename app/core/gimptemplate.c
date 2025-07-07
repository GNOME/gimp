/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimptemplate.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

/* This file contains the definition of the image template objects.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"

#include "gimpimage.h"
#include "gimpprojection.h"
#include "gimptemplate.h"

#include "gimp-intl.h"


#define DEFAULT_RESOLUTION 300.0

enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_UNIT,
  PROP_XRESOLUTION,
  PROP_YRESOLUTION,
  PROP_RESOLUTION_UNIT,
  PROP_BASE_TYPE,
  PROP_PRECISION,
  PROP_COMPONENT_TYPE,
  PROP_LINEAR,
  PROP_TRC,
  PROP_COLOR_PROFILE,
  PROP_SIMULATION_PROFILE,
  PROP_SIMULATION_BPC,
  PROP_SIMULATION_INTENT,
  PROP_FILL_TYPE,
  PROP_COMMENT,
  PROP_FILENAME,

  /* compat cruft */
  PROP_COLOR_MANAGED
};


typedef struct _GimpTemplatePrivate GimpTemplatePrivate;

struct _GimpTemplatePrivate
{
  gint                     width;
  gint                     height;
  GimpUnit                *unit;

  gdouble                  xresolution;
  gdouble                  yresolution;
  GimpUnit                *resolution_unit;

  GimpImageBaseType        base_type;
  GimpPrecision            precision;

  GFile                   *color_profile;
  GFile                   *simulation_profile;
  GimpColorRenderingIntent simulation_intent;
  gboolean                 simulation_bpc;

  GimpFillType             fill_type;

  gchar                   *comment;
  gchar                   *filename;

  guint64            initial_size;
};

#define GET_PRIVATE(template) ((GimpTemplatePrivate *) gimp_template_get_instance_private ((GimpTemplate *) (template)))


static void      gimp_template_finalize     (GObject      *object);
static void      gimp_template_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void      gimp_template_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);
static void      gimp_template_notify       (GObject      *object,
                                             GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpTemplate, gimp_template, GIMP_TYPE_VIEWABLE,
                         G_ADD_PRIVATE (GimpTemplate)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_template_parent_class


static void
gimp_template_class_init (GimpTemplateClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize     = gimp_template_finalize;

  object_class->set_property = gimp_template_set_property;
  object_class->get_property = gimp_template_get_property;
  object_class->notify       = gimp_template_notify;

  viewable_class->default_icon_name = "gimp-template";
  viewable_class->name_editable     = TRUE;

  GIMP_CONFIG_PROP_INT (object_class, PROP_WIDTH,
                        "width",
                        _("Width"),
                        NULL,
                        GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                        GIMP_DEFAULT_IMAGE_WIDTH,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_HEIGHT,
                        "height",
                        _("Height"),
                        NULL,
                        GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                        GIMP_DEFAULT_IMAGE_HEIGHT,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_UNIT (object_class, PROP_UNIT,
                         "unit",
                         _("Unit"),
                         _("The unit used for coordinate display "
                           "when not in dot-for-dot mode."),
                         TRUE, FALSE, gimp_unit_pixel (),
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_RESOLUTION (object_class, PROP_XRESOLUTION,
                               "xresolution",
                               _("Resolution X"),
                               _("The horizontal image resolution."),
                               DEFAULT_RESOLUTION,
                               GIMP_PARAM_STATIC_STRINGS |
                               GIMP_TEMPLATE_PARAM_COPY_FIRST);

  GIMP_CONFIG_PROP_RESOLUTION (object_class, PROP_YRESOLUTION,
                               "yresolution",
                               _("Resolution X"),
                               _("The vertical image resolution."),
                               DEFAULT_RESOLUTION,
                               GIMP_PARAM_STATIC_STRINGS |
                               GIMP_TEMPLATE_PARAM_COPY_FIRST);

  GIMP_CONFIG_PROP_UNIT (object_class, PROP_RESOLUTION_UNIT,
                         "resolution-unit",
                         _("Resolution unit"),
                         NULL,
                         FALSE, FALSE, gimp_unit_inch (),
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_BASE_TYPE,
                         "image-type", /* serialized name */
                         _("Image type"),
                         NULL,
                         GIMP_TYPE_IMAGE_BASE_TYPE, GIMP_RGB,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_PRECISION,
                         "precision",
                         _("Precision"),
                         NULL,
                         GIMP_TYPE_PRECISION, GIMP_PRECISION_U8_NON_LINEAR,
                         GIMP_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_COMPONENT_TYPE,
                                   g_param_spec_enum ("component-type",
                                                      _("Precision"),
                                                      NULL,
                                                      GIMP_TYPE_COMPONENT_TYPE,
                                                      GIMP_COMPONENT_TYPE_U8,
                                                      G_PARAM_READWRITE |
                                                      GIMP_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_TRC,
                                   g_param_spec_enum ("trc",
                                                      _("Linear/Perceptual"),
                                                      NULL,
                                                      GIMP_TYPE_TRC_TYPE,
                                                      GIMP_TRC_NON_LINEAR,
                                                      G_PARAM_READWRITE |
                                                      GIMP_PARAM_STATIC_STRINGS));

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_COLOR_PROFILE,
                           "color-profile",
                           _("Color profile"),
                           NULL,
                           G_TYPE_FILE,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_SIMULATION_PROFILE,
                           "simulation-profile",
                           _("Simulation profile"),
                           NULL,
                           G_TYPE_FILE,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_SIMULATION_INTENT,
                         "simulation-intent",
                         _("Simulation Rendering Intent"),
                         NULL,
                         GIMP_TYPE_COLOR_RENDERING_INTENT,
                         GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SIMULATION_BPC,
                            "simulation-bpc",
                            _("Use Black Point Compensation for Simulation"),
                            NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_FILL_TYPE,
                         "fill-type",
                         _("Fill type"),
                         NULL,
                         GIMP_TYPE_FILL_TYPE, GIMP_FILL_BACKGROUND,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_COMMENT,
                           "comment",
                           _("Comment"),
                           NULL,
                           NULL,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_FILENAME,
                           "filename",
                           _("Filename"),
                           NULL,
                           NULL,
                           GIMP_PARAM_STATIC_STRINGS);

  /* compat cruft */
  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_COLOR_MANAGED,
                            "color-managed",
                            NULL, NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_CONFIG_PARAM_IGNORE);
}

static void
gimp_template_init (GimpTemplate *template)
{
}

static void
gimp_template_finalize (GObject *object)
{
  GimpTemplatePrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->color_profile);
  g_clear_object (&private->simulation_profile);
  g_clear_pointer (&private->comment,  g_free);
  g_clear_pointer (&private->filename, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_template_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpTemplatePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_WIDTH:
      private->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_int (value);
      break;
    case PROP_UNIT:
      private->unit = g_value_get_object (value);
      break;
    case PROP_XRESOLUTION:
      private->xresolution = g_value_get_double (value);
      break;
    case PROP_YRESOLUTION:
      private->yresolution = g_value_get_double (value);
      break;
    case PROP_RESOLUTION_UNIT:
      private->resolution_unit = g_value_get_object (value);
      break;
    case PROP_BASE_TYPE:
      private->base_type = g_value_get_enum (value);
      break;
    case PROP_PRECISION:
      private->precision = g_value_get_enum (value);
      g_object_notify (object, "component-type");
      g_object_notify (object, "trc");
      break;
    case PROP_COMPONENT_TYPE:
      private->precision =
        gimp_babl_precision (g_value_get_enum (value),
                             gimp_babl_trc (private->precision));
      g_object_notify (object, "precision");
      break;
    case PROP_TRC:
      private->precision =
        gimp_babl_precision (gimp_babl_component_type (private->precision),
                             g_value_get_enum (value));
      g_object_notify (object, "precision");
      break;
    case PROP_COLOR_PROFILE:
      g_set_object (&private->color_profile, g_value_get_object (value));
      break;
    case PROP_SIMULATION_PROFILE:
      g_set_object (&private->simulation_profile, g_value_get_object (value));
      break;
    case PROP_SIMULATION_INTENT:
      private->simulation_intent = g_value_get_enum (value);
      break;
    case PROP_SIMULATION_BPC:
      private->simulation_bpc = g_value_get_boolean (value);
      break;
    case PROP_FILL_TYPE:
      private->fill_type = g_value_get_enum (value);
      break;
    case PROP_COMMENT:
      g_set_str (&private->comment, g_value_get_string (value));
      break;
    case PROP_FILENAME:
      g_set_str (&private->filename, g_value_get_string (value));
      break;

    case PROP_COLOR_MANAGED:
      /* ignored */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_template_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpTemplatePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_WIDTH:
      g_value_set_int (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, private->height);
      break;
    case PROP_UNIT:
      g_value_set_object (value, private->unit);
      break;
    case PROP_XRESOLUTION:
      g_value_set_double (value, private->xresolution);
      break;
    case PROP_YRESOLUTION:
      g_value_set_double (value, private->yresolution);
      break;
    case PROP_RESOLUTION_UNIT:
      g_value_set_object (value, private->resolution_unit);
      break;
    case PROP_BASE_TYPE:
      g_value_set_enum (value, private->base_type);
      break;
    case PROP_PRECISION:
      g_value_set_enum (value, private->precision);
      break;
    case PROP_COMPONENT_TYPE:
      g_value_set_enum (value, gimp_babl_component_type (private->precision));
      break;
    case PROP_TRC:
      g_value_set_enum (value, gimp_babl_trc (private->precision));
      break;
    case PROP_COLOR_PROFILE:
      g_value_set_object (value, private->color_profile);
      break;
    case PROP_SIMULATION_PROFILE:
      g_value_set_object (value, private->simulation_profile);
      break;
    case PROP_SIMULATION_INTENT:
      g_value_set_enum (value, private->simulation_intent);
      break;
    case PROP_SIMULATION_BPC:
      g_value_set_boolean (value, private->simulation_bpc);
      break;
    case PROP_FILL_TYPE:
      g_value_set_enum (value, private->fill_type);
      break;
    case PROP_COMMENT:
      g_value_set_string (value, private->comment);
      break;
    case PROP_FILENAME:
      g_value_set_string (value, private->filename);
      break;

    case PROP_COLOR_MANAGED:
      /* ignored */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_template_notify (GObject    *object,
                      GParamSpec *pspec)
{
  GimpTemplatePrivate *private = GET_PRIVATE (object);
  const Babl          *format;
  gint                 bytes;

  if (G_OBJECT_CLASS (parent_class)->notify)
    G_OBJECT_CLASS (parent_class)->notify (object, pspec);

  /* the initial layer */
  format = gimp_babl_format (private->base_type,
                             private->precision,
                             private->fill_type == GIMP_FILL_TRANSPARENT,
                             NULL);
  bytes = babl_format_get_bytes_per_pixel (format);

  /* the selection */
  format = gimp_babl_mask_format (private->precision);
  bytes += babl_format_get_bytes_per_pixel (format);

  private->initial_size = ((guint64) bytes          *
                           (guint64) private->width *
                           (guint64) private->height);

  private->initial_size +=
    gimp_projection_estimate_memsize (private->base_type,
                                      gimp_babl_component_type (private->precision),
                                      private->width, private->height);
}


/*  public functions  */

GimpTemplate *
gimp_template_new (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (GIMP_TYPE_TEMPLATE,
                       "name", name,
                       NULL);
}

void
gimp_template_set_from_image (GimpTemplate *template,
                              GimpImage    *image)
{
  gdouble             xresolution;
  gdouble             yresolution;
  GimpImageBaseType   base_type;
  const GimpParasite *parasite;
  gchar              *comment = NULL;

  g_return_if_fail (GIMP_IS_TEMPLATE (template));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  gimp_image_get_resolution (image, &xresolution, &yresolution);

  base_type = gimp_image_get_base_type (image);

  if (base_type == GIMP_INDEXED)
    base_type = GIMP_RGB;

  parasite =  gimp_image_parasite_find (image, "gimp-comment");
  if (parasite)
    {
      guint32 parasite_size;

      comment = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
      comment = g_strndup (comment, parasite_size);
    }

  g_object_set (template,
                "width",           gimp_image_get_width (image),
                "height",          gimp_image_get_height (image),
                "xresolution",     xresolution,
                "yresolution",     yresolution,
                "resolution-unit", gimp_image_get_unit (image),
                "image-type",      base_type,
                "precision",       gimp_image_get_precision (image),
                "comment",         comment,
                NULL);

  if (comment)
    g_free (comment);
}

gint
gimp_template_get_width (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), 0);

  return GET_PRIVATE (template)->width;
}

gint
gimp_template_get_height (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), 0);

  return GET_PRIVATE (template)->height;
}

GimpUnit *
gimp_template_get_unit (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), gimp_unit_inch ());

  return GET_PRIVATE (template)->unit;
}

gdouble
gimp_template_get_resolution_x (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), 1.0);

  return GET_PRIVATE (template)->xresolution;
}

gdouble
gimp_template_get_resolution_y (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), 1.0);

  return GET_PRIVATE (template)->yresolution;
}

GimpUnit *
gimp_template_get_resolution_unit (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), gimp_unit_inch ());

  return GET_PRIVATE (template)->resolution_unit;
}

GimpImageBaseType
gimp_template_get_base_type (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), GIMP_RGB);

  return GET_PRIVATE (template)->base_type;
}

GimpPrecision
gimp_template_get_precision (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template),
                        GIMP_PRECISION_U8_NON_LINEAR);

  return GET_PRIVATE (template)->precision;
}

GimpColorProfile *
gimp_template_get_color_profile (GimpTemplate *template)
{
  GimpTemplatePrivate *private;

  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), FALSE);

  private = GET_PRIVATE (template);

  if (private->color_profile)
    return gimp_color_profile_new_from_file (private->color_profile, NULL);

  return NULL;
}

GimpColorProfile *
gimp_template_get_simulation_profile (GimpTemplate *template)
{
  GimpTemplatePrivate *private;

  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), FALSE);

  private = GET_PRIVATE (template);

  if (private->simulation_profile)
    return gimp_color_profile_new_from_file (private->simulation_profile,
                                             NULL);

  return NULL;
}

GimpColorRenderingIntent
gimp_template_get_simulation_intent (GimpTemplate *template)
{
  GimpTemplatePrivate *private;

  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), FALSE);

  private = GET_PRIVATE (template);

  return private->simulation_intent;
}

gboolean
gimp_template_get_simulation_bpc (GimpTemplate *template)
{
  GimpTemplatePrivate *private;

  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), FALSE);

  private = GET_PRIVATE (template);

  return private->simulation_bpc;
}

GimpFillType
gimp_template_get_fill_type (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), GIMP_FILL_BACKGROUND);

  return GET_PRIVATE (template)->fill_type;
}

const gchar *
gimp_template_get_comment (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), NULL);

  return GET_PRIVATE (template)->comment;
}

guint64
gimp_template_get_initial_size (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), 0);

  return GET_PRIVATE (template)->initial_size;
}
