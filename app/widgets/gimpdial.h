/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadial.h
 * Copyright (C) 2014 Michael Natterer <mitch@ligma.org>
 *
 * Based on code from the color-rotate plug-in
 * Copyright (C) 1997-1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                         Based on code from Pavel Grinfeld (pavel@ml.com)
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

#ifndef __LIGMA_DIAL_H__
#define __LIGMA_DIAL_H__


#include "ligmacircle.h"


#define LIGMA_TYPE_DIAL            (ligma_dial_get_type ())
#define LIGMA_DIAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DIAL, LigmaDial))
#define LIGMA_DIAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DIAL, LigmaDialClass))
#define LIGMA_IS_DIAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_DIAL))
#define LIGMA_IS_DIAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DIAL))
#define LIGMA_DIAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DIAL, LigmaDialClass))


typedef struct _LigmaDialPrivate LigmaDialPrivate;
typedef struct _LigmaDialClass   LigmaDialClass;

struct _LigmaDial
{
  LigmaCircle       parent_instance;

  LigmaDialPrivate *priv;
};

struct _LigmaDialClass
{
  LigmaCircleClass  parent_class;
};


GType          ligma_dial_get_type          (void) G_GNUC_CONST;

GtkWidget    * ligma_dial_new               (void);


#endif /* __LIGMA_DIAL_H__ */
