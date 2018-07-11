/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfilter.h
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

#ifndef __GIMP_FILTER_H__
#define __GIMP_FILTER_H__


#include "gimpviewable.h"


#define GIMP_TYPE_FILTER            (gimp_filter_get_type ())
#define GIMP_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILTER, GimpFilter))
#define GIMP_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILTER, GimpFilterClass))
#define GIMP_IS_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILTER))
#define GIMP_IS_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILTER))
#define GIMP_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILTER, GimpFilterClass))


typedef struct _GimpFilterClass GimpFilterClass;

struct _GimpFilter
{
  GimpViewable  parent_instance;
};

struct _GimpFilterClass
{
  GimpViewableClass  parent_class;

  /*  signals  */
  void       (* active_changed) (GimpFilter *filter);

  /*  virtual functions  */
  GeglNode * (* get_node)       (GimpFilter *filter);
};


GType            gimp_filter_get_type         (void) G_GNUC_CONST;
GimpFilter     * gimp_filter_new              (const gchar    *name);

GeglNode       * gimp_filter_get_node         (GimpFilter     *filter);
GeglNode       * gimp_filter_peek_node        (GimpFilter     *filter);

void             gimp_filter_set_active       (GimpFilter     *filter,
                                               gboolean        active);
gboolean         gimp_filter_get_active       (GimpFilter     *filter);

void             gimp_filter_set_is_last_node (GimpFilter     *filter,
                                               gboolean        is_last_node);
gboolean         gimp_filter_get_is_last_node (GimpFilter     *filter);

void             gimp_filter_set_applicator   (GimpFilter     *filter,
                                               GimpApplicator *applicator);
GimpApplicator * gimp_filter_get_applicator   (GimpFilter     *filter);


#endif /* __GIMP_FILTER_H__ */
