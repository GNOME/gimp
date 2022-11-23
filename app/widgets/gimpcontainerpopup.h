/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainerpopup.h
 * Copyright (C) 2003-2014 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_POPUP_H__
#define __LIGMA_CONTAINER_POPUP_H__


#include "ligmapopup.h"


#define LIGMA_TYPE_CONTAINER_POPUP            (ligma_container_popup_get_type ())
#define LIGMA_CONTAINER_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTAINER_POPUP, LigmaContainerPopup))
#define LIGMA_CONTAINER_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTAINER_POPUP, LigmaContainerPopupClass))
#define LIGMA_IS_CONTAINER_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTAINER_POPUP))
#define LIGMA_IS_CONTAINER_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTAINER_POPUP))
#define LIGMA_CONTAINER_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTAINER_POPUP, LigmaContainerPopupClass))


typedef struct _LigmaContainerPopupClass  LigmaContainerPopupClass;

struct _LigmaContainerPopup
{
  LigmaPopup            parent_instance;

  LigmaContainer       *container;
  LigmaContext         *orig_context;
  LigmaContext         *context;

  LigmaViewType         view_type;
  gint                 default_view_size;
  gint                 view_size;
  gint                 view_border_width;

  GtkWidget           *frame;
  LigmaContainerEditor *editor;

  LigmaDialogFactory   *dialog_factory;
  gchar               *dialog_identifier;
  gchar               *dialog_icon_name;
  gchar               *dialog_tooltip;
};

struct _LigmaContainerPopupClass
{
  LigmaPopupClass  parent_instance;
};


GType       ligma_container_popup_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_container_popup_new      (LigmaContainer      *container,
                                           LigmaContext        *context,
                                           LigmaViewType        view_type,
                                           gint                default_view_size,
                                           gint                view_size,
                                           gint                view_border_width,
                                           LigmaDialogFactory  *dialog_factory,
                                           const gchar        *dialog_identifier,
                                           const gchar        *dialog_icon_name,
                                           const gchar        *dialog_tooltip);

LigmaViewType ligma_container_popup_get_view_type (LigmaContainerPopup *popup);
void         ligma_container_popup_set_view_type (LigmaContainerPopup *popup,
                                                 LigmaViewType        view_type);

gint         ligma_container_popup_get_view_size (LigmaContainerPopup *popup);
void         ligma_container_popup_set_view_size (LigmaContainerPopup *popup,
                                                 gint                view_size);


#endif  /*  __LIGMA_CONTAINER_POPUP_H__  */
