/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafilter.h
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

#ifndef __LIGMA_FILTER_H__
#define __LIGMA_FILTER_H__


#include "ligmaviewable.h"


#define LIGMA_TYPE_FILTER            (ligma_filter_get_type ())
#define LIGMA_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILTER, LigmaFilter))
#define LIGMA_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILTER, LigmaFilterClass))
#define LIGMA_IS_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILTER))
#define LIGMA_IS_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILTER))
#define LIGMA_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILTER, LigmaFilterClass))


typedef struct _LigmaFilterClass LigmaFilterClass;

struct _LigmaFilter
{
  LigmaViewable  parent_instance;
};

struct _LigmaFilterClass
{
  LigmaViewableClass  parent_class;

  /*  signals  */
  void       (* active_changed) (LigmaFilter *filter);

  /*  virtual functions  */
  GeglNode * (* get_node)       (LigmaFilter *filter);
};


GType            ligma_filter_get_type         (void) G_GNUC_CONST;
LigmaFilter     * ligma_filter_new              (const gchar    *name);

GeglNode       * ligma_filter_get_node         (LigmaFilter     *filter);
GeglNode       * ligma_filter_peek_node        (LigmaFilter     *filter);

void             ligma_filter_set_active       (LigmaFilter     *filter,
                                               gboolean        active);
gboolean         ligma_filter_get_active       (LigmaFilter     *filter);

void             ligma_filter_set_is_last_node (LigmaFilter     *filter,
                                               gboolean        is_last_node);
gboolean         ligma_filter_get_is_last_node (LigmaFilter     *filter);

void             ligma_filter_set_applicator   (LigmaFilter     *filter,
                                               LigmaApplicator *applicator);
LigmaApplicator * ligma_filter_get_applicator   (LigmaFilter     *filter);


#endif /* __LIGMA_FILTER_H__ */
