/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_DISPLAY_H__
#define __LIGMA_DISPLAY_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_DISPLAY            (ligma_display_get_type ())
#define LIGMA_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DISPLAY, LigmaDisplay))
#define LIGMA_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DISPLAY, LigmaDisplayClass))
#define LIGMA_IS_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DISPLAY))
#define LIGMA_IS_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DISPLAY))
#define LIGMA_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DISPLAY, LigmaDisplayClass))


typedef struct _LigmaDisplayClass   LigmaDisplayClass;
typedef struct _LigmaDisplayPrivate LigmaDisplayPrivate;

struct _LigmaDisplay
{
  LigmaObject          parent_instance;

  Ligma               *ligma;
  LigmaDisplayConfig  *config;

  LigmaDisplayPrivate *priv;
};

struct _LigmaDisplayClass
{
  LigmaObjectClass     parent_class;

  gboolean         (* present)    (LigmaDisplay *display);
  gboolean         (* grab_focus) (LigmaDisplay *display);
};


GType         ligma_display_get_type  (void) G_GNUC_CONST;

gint          ligma_display_get_id    (LigmaDisplay *display);
LigmaDisplay * ligma_display_get_by_id (Ligma        *ligma,
                                      gint         id);

gboolean      ligma_display_present    (LigmaDisplay *display);
gboolean      ligma_display_grab_focus (LigmaDisplay *display);

Ligma        * ligma_display_get_ligma  (LigmaDisplay *display);


#endif /*  __LIGMA_DISPLAY_H__  */
