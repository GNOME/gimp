/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppopup.h
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

#pragma once


#define GIMP_TYPE_POPUP            (gimp_popup_get_type ())
#define GIMP_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_POPUP, GimpPopup))
#define GIMP_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_POPUP, GimpPopupClass))
#define GIMP_IS_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_POPUP))
#define GIMP_IS_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_POPUP))
#define GIMP_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_POPUP, GimpPopupClass))


typedef struct _GimpPopupClass  GimpPopupClass;

struct _GimpPopup
{
  GtkWindow  parent_instance;
};

struct _GimpPopupClass
{
  GtkWindowClass  parent_instance;

  void (* cancel)  (GimpPopup *popup);
  void (* confirm) (GimpPopup *popup);
};


GType   gimp_popup_get_type (void) G_GNUC_CONST;

void    gimp_popup_show     (GimpPopup *popup,
                             GtkWidget *widget);
