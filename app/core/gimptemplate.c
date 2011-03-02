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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file contains the definition of the image template objects.
 */

#include "config.h"

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpimage.h"
#include "gimpprojection.h"
#include "gimptemplate.h"

#include "gimp-intl.h"


#define DEFAULT_RESOLUTION    72.0


enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_UNIT,
  PROP_XRESOLUTION,
  PROP_YRESOLUTION,
  PROP_RESOLUTION_UNIT,
  PROP_IMAGE_TYPE,
  PROP_FILL_TYPE,
  PROP_COMMENT,
  PROP_FILENAME
};


typedef struct _GimpTemplatePrivate GimpTemplatePrivate;

struct _GimpTemplatePrivate
{
  gint               width;
  gint               height;
  GimpUnit           unit;

  gdouble            xresolution;
  gdouble            yresolution;
  GimpUnit           resolution_unit;

  GimpImageBaseType  image_type;
  GimpFillType       fill_type;

  gchar             *comment;
  gchar             *filename;

  guint64            initial_size;
};

#define GET_PRIVATE(template) G_TYPE_INSTANCE_GET_PRIVATE (template, \
                                                           GIMP_TYPE_TEMPLATE, \
                                                           GimpTemplatePrivate)


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

  viewable_class->default_stock_id = "gimp-template";

  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_WIDTH, "width",
                                NULL,
                                GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                                GIMP_DEFAULT_IMAGE_WIDTH,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_HEIGHT, "height",
                                NULL,
                                GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                                GIMP_DEFAULT_IMAGE_HEIGHT,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_UNIT, "unit",
                                 N_("The unit used for coordinate display "
                                    "when not in dot-for-dot mode."),
                                 TRUE, FALSE, GIMP_UNIT_PIXEL,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_RESOLUTION (object_class, PROP_XRESOLUTION,
                                       "xresolution",
                                       N_("The horizontal image resolution."),
                                       DEFAULT_RESOLUTION,
                                       GIMP_PARAM_STATIC_STRINGS |
                                       GIMP_TEMPLATE_PARAM_COPY_FIRST);
  GIMP_CONFIG_INSTALL_PROP_RESOLUTION (object_class, PROP_YRESOLUTION,
                                       "yresolution",
                                       N_("The vertical image resolution."),
                                       DEFAULT_RESOLUTION,
                                       GIMP_PARAM_STATIC_STRINGS |
                                       GIMP_TEMPLATE_PARAM_COPY_FIRST);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_RESOLUTION_UNIT,
                                 "resolution-unit",
                                 NULL,
                                 FALSE, FALSE, GIMP_UNIT_INCH,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_IMAGE_TYPE,
                                 "image-type",
                                 NULL,
                                 GIMP_TYPE_IMAGE_BASE_TYPE, GIMP_RGB,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_FILL_TYPE,
                                 "fill-type",
                                 NULL,
                                 GIMP_TYPE_FILL_TYPE, GIMP_BACKGROUND_FILL,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_COMMENT,
                                   "comment",
                                   NULL,
                                   NULL,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_FILENAME,
                                   "filename",
                                   NULL,
                                   NULL,
                                   GIMP_PARAM_STATIC_STRINGS);

  g_type_class_add_private (klass, sizeof (GimpTemplatePrivate));
}

static void
gimp_template_init (GimpTemplate *template)
{
}

static void
gimp_template_finalize (GObject *object)
{
  GimpTemplatePrivate *private = GET_PRIVATE (object);

  if (private->comment)
    {
      g_free (private->comment);
      private->comment = NULL;
    }

  if (private->filename)
    {
      g_free (private->filename);
      private->filename = NULL;
    }

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
      private->unit = g_value_get_int (value);
      break;
    case PROP_XRESOLUTION:
      private->xresolution = g_value_get_double (value);
      break;
    case PROP_YRESOLUTION:
      private->yresolution = g_value_get_double (value);
      break;
    case PROP_RESOLUTION_UNIT:
      private->resolution_unit = g_value_get_int (value);
      break;
    case PROP_IMAGE_TYPE:
      private->image_type = g_value_get_enum (value);
      break;
    case PROP_FILL_TYPE:
      private->fill_type = g_value_get_enum (value);
      break;
    case PROP_COMMENT:
      if (private->comment)
        g_free (private->comment);
      private->comment = g_value_dup_string (value);
      break;
    case PROP_FILENAME:
      if (private->filename)
        g_free (private->filename);
      private->filename = g_value_dup_string (value);
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
      g_value_set_int (value, private->unit);
      break;
    case PROP_XRESOLUTION:
      g_value_set_double (value, private->xresolution);
      break;
    case PROP_YRESOLUTION:
      g_value_set_double (value, private->yresolution);
      break;
    case PROP_RESOLUTION_UNIT:
      g_value_set_int (value, private->resolution_unit);
      break;
    case PROP_IMAGE_TYPE:
      g_value_set_enum (value, private->image_type);
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
  gint                 channels;

  if (G_OBJECT_CLASS (parent_class)->notify)
    G_OBJECT_CLASS (parent_class)->notify (object, pspec);

  channels = ((private->image_type == GIMP_RGB ? 3 : 1)     /* color      */ +
              (private->fill_type == GIMP_TRANSPARENT_FILL) /* alpha      */ +
              1                                             /* selection  */);

  private->initial_size = ((guint64) channels        *
                           (guint64) private->width *
                           (guint64) private->height);

  private->initial_size +=
    gimp_projection_estimate_memsize (private->image_type,
                                      private->width, private->height);

  if (! strcmp (pspec->name, "stock-id"))
    gimp_viewable_invalidate_preview (GIMP_VIEWABLE (object));
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
  GimpImageBaseType   image_type;
  const GimpParasite *parasite;
  gchar              *comment = NULL;

  g_return_if_fail (GIMP_IS_TEMPLATE (template));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  gimp_image_get_resolution (image, &xresolution, &yresolution);

  image_type = gimp_image_base_type (image);

  if (image_type == GIMP_INDEXED)
    image_type = GIMP_RGB;

  parasite =  gimp_image_parasite_find (image, "gimp-comment");
  if (parasite)
    comment = g_strndup (gimp_parasite_data (parasite),
                         gimp_parasite_data_size (parasite));

  g_object_set (template,
                "width",           gimp_image_get_width (image),
                "height",          gimp_image_get_height (image),
                "xresolution",     xresolution,
                "yresolution",     yresolution,
                "resolution-unit", gimp_image_get_unit (image),
                "image-type",      image_type,
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

GimpUnit
gimp_template_get_unit (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), GIMP_UNIT_INCH);

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

GimpUnit
gimp_template_get_resolution_unit (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), GIMP_UNIT_INCH);

  return GET_PRIVATE (template)->resolution_unit;
}

GimpImageBaseType
gimp_template_get_image_type (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), GIMP_RGB_IMAGE);

  return GET_PRIVATE (template)->image_type;
}

GimpFillType
gimp_template_get_fill_type (GimpTemplate *template)
{
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), GIMP_NO_FILL);

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
