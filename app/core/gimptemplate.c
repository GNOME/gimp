/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimptemplate.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimptemplate.h"

#include "gimp-intl.h"


/*  The default image aspect ratio is the golden mean. We use
 *  two adjacent fibonacci numbers for the unstable series and
 *  some less odd values for the stable version.
 */

#ifdef GIMP_UNSTABLE
#define DEFAULT_IMAGE_WIDTH   377
#define DEFAULT_IMAGE_HEIGHT  233
#else
#define DEFAULT_IMAGE_WIDTH   420
#define DEFAULT_IMAGE_HEIGHT  300
#endif

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
                                DEFAULT_IMAGE_WIDTH,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_HEIGHT, "height",
                                NULL,
                                GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE,
                                DEFAULT_IMAGE_HEIGHT,
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
                                       GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_RESOLUTION (object_class, PROP_YRESOLUTION,
                                       "yresolution",
                                       N_("The vertical image resolution."),
                                       DEFAULT_RESOLUTION,
                                       GIMP_PARAM_STATIC_STRINGS);
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
}

static void
gimp_template_init (GimpTemplate *template)
{
}

static void
gimp_template_finalize (GObject *object)
{
  GimpTemplate *template = GIMP_TEMPLATE (object);

  if (template->comment)
    {
      g_free (template->comment);
      template->comment = NULL;
    }

  if (template->filename)
    {
      g_free (template->filename);
      template->filename = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_template_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpTemplate *template = GIMP_TEMPLATE (object);

  switch (property_id)
    {
    case PROP_WIDTH:
      template->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      template->height = g_value_get_int (value);
      break;
    case PROP_UNIT:
      template->unit = g_value_get_int (value);
      break;
    case PROP_XRESOLUTION:
      template->xresolution = g_value_get_double (value);
      break;
    case PROP_YRESOLUTION:
      template->yresolution = g_value_get_double (value);
      break;
    case PROP_RESOLUTION_UNIT:
      template->resolution_unit = g_value_get_int (value);
      break;
    case PROP_IMAGE_TYPE:
      template->image_type = g_value_get_enum (value);
      break;
    case PROP_FILL_TYPE:
      template->fill_type = g_value_get_enum (value);
      break;
    case PROP_COMMENT:
      if (template->comment)
        g_free (template->comment);
      template->comment = g_value_dup_string (value);
      break;
    case PROP_FILENAME:
      if (template->filename)
        g_free (template->filename);
      template->filename = g_value_dup_string (value);
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
  GimpTemplate *template = GIMP_TEMPLATE (object);

  switch (property_id)
    {
    case PROP_WIDTH:
      g_value_set_int (value, template->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, template->height);
      break;
    case PROP_UNIT:
      g_value_set_int (value, template->unit);
      break;
    case PROP_XRESOLUTION:
      g_value_set_double (value, template->xresolution);
      break;
    case PROP_YRESOLUTION:
      g_value_set_double (value, template->yresolution);
      break;
    case PROP_RESOLUTION_UNIT:
      g_value_set_int (value, template->resolution_unit);
      break;
    case PROP_IMAGE_TYPE:
      g_value_set_enum (value, template->image_type);
      break;
    case PROP_FILL_TYPE:
      g_value_set_enum (value, template->fill_type);
      break;
    case PROP_COMMENT:
      g_value_set_string (value, template->comment);
      break;
    case PROP_FILENAME:
      g_value_set_string (value, template->filename);
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
  GimpTemplate *template = GIMP_TEMPLATE (object);
  gint          channels;

  if (G_OBJECT_CLASS (parent_class)->notify)
    G_OBJECT_CLASS (parent_class)->notify (object, pspec);

  channels = ((template->image_type == GIMP_RGB ? 3 : 1)     /* color      */ +
              (template->fill_type == GIMP_TRANSPARENT_FILL) /* alpha      */ +
              1                                              /* selection  */);

  template->initial_size = ((guint64) channels        *
                            (guint64) template->width *
                            (guint64) template->height);

  template->initial_size +=
    gimp_projection_estimate_memsize (template->image_type,
                                      template->width, template->height);

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

GimpImage *
gimp_template_create_image (Gimp         *gimp,
                            GimpTemplate *template,
                            GimpContext  *context)
{
  GimpImage     *image;
  GimpLayer     *layer;
  GimpImageType  type;
  gint           width, height;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_TEMPLATE (template), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  image = gimp_create_image (gimp,
                             template->width, template->height,
                             template->image_type,
                             FALSE);

  gimp_image_undo_disable (image);

  if (template->comment)
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-comment",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (template->comment) + 1,
                                    template->comment);
      gimp_image_parasite_attach (image, parasite);
      gimp_parasite_free (parasite);
    }

  gimp_image_set_resolution (image,
                             template->xresolution, template->yresolution);
  gimp_image_set_unit (image, template->resolution_unit);

  width  = gimp_image_get_width (image);
  height = gimp_image_get_height (image);

  switch (template->fill_type)
    {
    case GIMP_TRANSPARENT_FILL:
      type = ((template->image_type == GIMP_RGB) ?
              GIMP_RGBA_IMAGE : GIMP_GRAYA_IMAGE);
      break;
    default:
      type = ((template->image_type == GIMP_RGB) ?
              GIMP_RGB_IMAGE : GIMP_GRAY_IMAGE);
      break;
    }

  layer = gimp_layer_new (image, width, height, type,
                          _("Background"),
                          GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  gimp_drawable_fill_by_type (GIMP_DRAWABLE (layer),
                              context, template->fill_type);

  gimp_image_add_layer (image, layer, 0);

  gimp_image_undo_enable (image);
  gimp_image_clean_all (image);

  gimp_create_display (gimp, image, template->unit, 1.0);

  g_object_unref (image);

  return image;
}
