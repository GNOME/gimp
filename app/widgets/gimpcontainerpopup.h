/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerpopup.h
 * Copyright (C) 2003-2014 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_CONTAINER_POPUP_H__
#define __GIMP_CONTAINER_POPUP_H__


#include "gimppopup.h"


#define GIMP_TYPE_CONTAINER_POPUP            (gimp_container_popup_get_type ())
#define GIMP_CONTAINER_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_POPUP, GimpContainerPopup))
#define GIMP_CONTAINER_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_POPUP, GimpContainerPopupClass))
#define GIMP_IS_CONTAINER_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_POPUP))
#define GIMP_IS_CONTAINER_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_POPUP))
#define GIMP_CONTAINER_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_POPUP, GimpContainerPopupClass))


typedef struct _GimpContainerPopupClass  GimpContainerPopupClass;

struct _GimpContainerPopup
{
  GimpPopup            parent_instance;

  GimpContainer       *container;
  GimpContext         *orig_context;
  GimpContext         *context;

  GimpViewType         view_type;
  gint                 default_view_size;
  gint                 view_size;
  gint                 view_border_width;

  GtkWidget           *frame;
  GimpContainerEditor *editor;

  GimpDialogFactory   *dialog_factory;
  gchar               *dialog_identifier;
  gchar               *dialog_icon_name;
  gchar               *dialog_tooltip;
};

struct _GimpContainerPopupClass
{
  GimpPopupClass  parent_instance;
};


GType       gimp_container_popup_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_container_popup_new      (GimpContainer      *container,
                                           GimpContext        *context,
                                           GimpViewType        view_type,
                                           gint                default_view_size,
                                           gint                view_size,
                                           gint                view_border_width,
                                           GimpDialogFactory  *dialog_factory,
                                           const gchar        *dialog_identifier,
                                           const gchar        *dialog_icon_name,
                                           const gchar        *dialog_tooltip);

GimpViewType gimp_container_popup_get_view_type (GimpContainerPopup *popup);
void         gimp_container_popup_set_view_type (GimpContainerPopup *popup,
                                                 GimpViewType        view_type);

gint         gimp_container_popup_get_view_size (GimpContainerPopup *popup);
void         gimp_container_popup_set_view_size (GimpContainerPopup *popup,
                                                 gint                view_size);


#endif  /*  __GIMP_CONTAINER_POPUP_H__  */
