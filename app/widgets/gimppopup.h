/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapopup.h
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

#ifndef __LIGMA_POPUP_H__
#define __LIGMA_POPUP_H__


#define LIGMA_TYPE_POPUP            (ligma_popup_get_type ())
#define LIGMA_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_POPUP, LigmaPopup))
#define LIGMA_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_POPUP, LigmaPopupClass))
#define LIGMA_IS_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_POPUP))
#define LIGMA_IS_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_POPUP))
#define LIGMA_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_POPUP, LigmaPopupClass))


typedef struct _LigmaPopupClass  LigmaPopupClass;

struct _LigmaPopup
{
  GtkWindow  parent_instance;
};

struct _LigmaPopupClass
{
  GtkWindowClass  parent_instance;

  void (* cancel)  (LigmaPopup *popup);
  void (* confirm) (LigmaPopup *popup);
};


GType   ligma_popup_get_type (void) G_GNUC_CONST;

void    ligma_popup_show     (LigmaPopup *popup,
                             GtkWidget *widget);


#endif  /*  __LIGMA_POPUP_H__  */
