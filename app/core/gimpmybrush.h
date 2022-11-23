/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamybrush.h
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

#ifndef __LIGMA_MYBRUSH_H__
#define __LIGMA_MYBRUSH_H__


#include "ligmadata.h"


#define LIGMA_TYPE_MYBRUSH            (ligma_mybrush_get_type ())
#define LIGMA_MYBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MYBRUSH, LigmaMybrush))
#define LIGMA_MYBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MYBRUSH, LigmaMybrushClass))
#define LIGMA_IS_MYBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MYBRUSH))
#define LIGMA_IS_MYBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MYBRUSH))
#define LIGMA_MYBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MYBRUSH, LigmaMybrushClass))


typedef struct _LigmaMybrushPrivate LigmaMybrushPrivate;
typedef struct _LigmaMybrushClass   LigmaMybrushClass;

struct _LigmaMybrush
{
  LigmaData            parent_instance;

  LigmaMybrushPrivate *priv;
};

struct _LigmaMybrushClass
{
  LigmaDataClass  parent_class;
};


GType         ligma_mybrush_get_type             (void) G_GNUC_CONST;

LigmaData    * ligma_mybrush_new                  (LigmaContext *context,
                                                 const gchar *name);
LigmaData    * ligma_mybrush_get_standard         (LigmaContext *context);

const gchar * ligma_mybrush_get_brush_json       (LigmaMybrush *brush);

gdouble       ligma_mybrush_get_radius           (LigmaMybrush *brush);
gdouble       ligma_mybrush_get_opaque           (LigmaMybrush *brush);
gdouble       ligma_mybrush_get_hardness         (LigmaMybrush *brush);
gdouble       ligma_mybrush_get_offset_by_random (LigmaMybrush *brush);
gboolean      ligma_mybrush_get_is_eraser        (LigmaMybrush *brush);


#endif /* __LIGMA_MYBRUSH_H__ */
