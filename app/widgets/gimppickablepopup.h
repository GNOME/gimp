/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppickablepopup.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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

#pragma once

#include "gimppopup.h"


#define GIMP_TYPE_PICKABLE_POPUP            (gimp_pickable_popup_get_type ())
#define GIMP_PICKABLE_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PICKABLE_POPUP, GimpPickablePopup))
#define GIMP_PICKABLE_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PICKABLE_POPUP, GimpPickablePopupClass))
#define GIMP_IS_PICKABLE_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PICKABLE_POPUP))
#define GIMP_IS_PICKABLE_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PICKABLE_POPUP))
#define GIMP_PICKABLE_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PICKABLE_POPUP, GimpPickablePopupClass))


typedef struct _GimpPickablePopupPrivate GimpPickablePopupPrivate;
typedef struct _GimpPickablePopupClass   GimpPickablePopupClass;

struct _GimpPickablePopup
{
  GimpPopup                 parent_instance;

  GimpPickablePopupPrivate *priv;
};

struct _GimpPickablePopupClass
{
  GimpPopupClass  parent_instance;
};


GType          gimp_pickable_popup_get_type     (void) G_GNUC_CONST;

GtkWidget    * gimp_pickable_popup_new          (GimpContext       *context,
                                                 gint               view_size,
                                                 gint               view_border_width);

GimpPickable * gimp_pickable_popup_get_pickable (GimpPickablePopup *popup);
