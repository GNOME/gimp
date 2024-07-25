/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimptemplate.h
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

#ifndef __GIMP_TEMPLATE_H__
#define __GIMP_TEMPLATE_H__


#include "gimpviewable.h"


#define GIMP_TEMPLATE_PARAM_COPY_FIRST (1 << (8 + G_PARAM_USER_SHIFT))

#ifdef GIMP_UNSTABLE
/* Uncommon ratio, with at least one odd value, to encourage testing
 * GIMP with unusual numbers.
 * It also has to be a higher resolution, to push GIMP a little further
 * in tests. */
#define GIMP_DEFAULT_IMAGE_WIDTH   2001
#define GIMP_DEFAULT_IMAGE_HEIGHT  1984
#else
/* 1366x768 is the most common screen resolution in 2016.
 * 1920x1080 is the second most common.
 * Since GIMP targets advanced graphics artists, let's go for the
 * highest common dimension.
 */
#define GIMP_DEFAULT_IMAGE_WIDTH   1920
#define GIMP_DEFAULT_IMAGE_HEIGHT  1080
#endif


#define GIMP_TYPE_TEMPLATE            (gimp_template_get_type ())
#define GIMP_TEMPLATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEMPLATE, GimpTemplate))
#define GIMP_TEMPLATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEMPLATE, GimpTemplateClass))
#define GIMP_IS_TEMPLATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEMPLATE))
#define GIMP_IS_TEMPLATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEMPLATE))
#define GIMP_TEMPLATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TEMPLATE, GimpTemplateClass))


typedef struct _GimpTemplateClass GimpTemplateClass;

struct _GimpTemplate
{
  GimpViewable  parent_instance;
};

struct _GimpTemplateClass
{
  GimpViewableClass  parent_instance;
};


GType               gimp_template_get_type            (void) G_GNUC_CONST;

GimpTemplate      * gimp_template_new                 (const gchar  *name);

void                gimp_template_set_from_image      (GimpTemplate *template,
                                                       GimpImage    *image);

gint                gimp_template_get_width           (GimpTemplate *template);
gint                gimp_template_get_height          (GimpTemplate *template);
GimpUnit          * gimp_template_get_unit            (GimpTemplate *template);

gdouble             gimp_template_get_resolution_x    (GimpTemplate *template);
gdouble             gimp_template_get_resolution_y    (GimpTemplate *template);
GimpUnit          * gimp_template_get_resolution_unit (GimpTemplate *template);

GimpImageBaseType   gimp_template_get_base_type       (GimpTemplate *template);
GimpPrecision       gimp_template_get_precision       (GimpTemplate *template);

GimpColorProfile  * gimp_template_get_color_profile   (GimpTemplate *template);
GimpColorProfile  * gimp_template_get_simulation_profile
                                                      (GimpTemplate *template);
GimpColorRenderingIntent gimp_template_get_simulation_intent
                                                      (GimpTemplate *template);
gboolean            gimp_template_get_simulation_bpc  (GimpTemplate *template);

GimpFillType        gimp_template_get_fill_type       (GimpTemplate *template);

const gchar       * gimp_template_get_comment         (GimpTemplate *template);

guint64             gimp_template_get_initial_size    (GimpTemplate *template);


#endif /* __GIMP_TEMPLATE__ */
