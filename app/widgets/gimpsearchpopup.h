/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsearchpopup.h
 * Copyright (C) 2015 Jehan <jehan at girinstud.io>
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


#define GIMP_TYPE_SEARCH_POPUP            (gimp_search_popup_get_type ())
#define GIMP_SEARCH_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SEARCH_POPUP, GimpSearchPopup))
#define GIMP_SEARCH_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SEARCH_POPUP, GimpSearchPopupClass))
#define GIMP_IS_SEARCH_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SEARCH_POPUP))
#define GIMP_IS_SEARCH_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SEARCH_POPUP))
#define GIMP_SEARCH_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SEARCH_POPUP, GimpSearchPopupClass))

/**
 * GimpSearchPopupCallback:
 * @popup:  the #GimpSearchPopup to operate on.
 * @search: the text searched.
 *
 * Callback used by @popup to fill in its result list.
 * It should make use of gimp_search_popup_add_result() to fill in
 * results.
 */
typedef struct _GimpSearchPopup           GimpSearchPopup;
typedef struct _GimpSearchPopupClass      GimpSearchPopupClass;
typedef struct _GimpSearchPopupPrivate    GimpSearchPopupPrivate;

typedef void   (*GimpSearchPopupCallback) (GimpSearchPopup  *popup,
                                           const gchar      *search,
                                           gpointer          data);

struct _GimpSearchPopup
{
  GimpPopup               parent_instance;

  GimpSearchPopupPrivate *priv;
};

struct _GimpSearchPopupClass
{
  GimpPopupClass          parent_class;
};

GType       gimp_search_popup_get_type   (void);

GtkWidget * gimp_search_popup_new        (Gimp                    *gimp,
                                          const gchar             *role,
                                          const gchar             *title,
                                          GimpSearchPopupCallback  callback,
                                          gpointer                 callback_data);

void        gimp_search_popup_add_result (GimpSearchPopup *popup,
                                          GimpAction      *action,
                                          gint             section);
