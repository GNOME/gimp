/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapickablepopup.h
 * Copyright (C) 2014 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PICKABLE_POPUP_H__
#define __LIGMA_PICKABLE_POPUP_H__


#include "ligmapopup.h"


#define LIGMA_TYPE_PICKABLE_POPUP            (ligma_pickable_popup_get_type ())
#define LIGMA_PICKABLE_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PICKABLE_POPUP, LigmaPickablePopup))
#define LIGMA_PICKABLE_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PICKABLE_POPUP, LigmaPickablePopupClass))
#define LIGMA_IS_PICKABLE_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PICKABLE_POPUP))
#define LIGMA_IS_PICKABLE_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PICKABLE_POPUP))
#define LIGMA_PICKABLE_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PICKABLE_POPUP, LigmaPickablePopupClass))


typedef struct _LigmaPickablePopupPrivate LigmaPickablePopupPrivate;
typedef struct _LigmaPickablePopupClass   LigmaPickablePopupClass;

struct _LigmaPickablePopup
{
  LigmaPopup                 parent_instance;

  LigmaPickablePopupPrivate *priv;
};

struct _LigmaPickablePopupClass
{
  LigmaPopupClass  parent_instance;
};


GType          ligma_pickable_popup_get_type     (void) G_GNUC_CONST;

GtkWidget    * ligma_pickable_popup_new          (LigmaContext       *context,
                                                 gint               view_size,
                                                 gint               view_border_width);

LigmaPickable * ligma_pickable_popup_get_pickable (LigmaPickablePopup *popup);


#endif  /*  __LIGMA_PICKABLE_POPUP_H__  */
