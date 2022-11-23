/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasearchpopup.h
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

#ifndef __LIGMA_SEARCH_POPUP_H__
#define __LIGMA_SEARCH_POPUP_H__

#include "ligmapopup.h"

#define LIGMA_TYPE_SEARCH_POPUP            (ligma_search_popup_get_type ())
#define LIGMA_SEARCH_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SEARCH_POPUP, LigmaSearchPopup))
#define LIGMA_SEARCH_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SEARCH_POPUP, LigmaSearchPopupClass))
#define LIGMA_IS_SEARCH_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SEARCH_POPUP))
#define LIGMA_IS_SEARCH_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SEARCH_POPUP))
#define LIGMA_SEARCH_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SEARCH_POPUP, LigmaSearchPopupClass))

/**
 * LigmaSearchPopupCallback:
 * @popup:  the #LigmaSearchPopup to operate on.
 * @search: the text searched.
 *
 * Callback used by @popup to fill in its result list.
 * It should make use of ligma_search_popup_add_result() to fill in
 * results.
 */
typedef struct _LigmaSearchPopup           LigmaSearchPopup;
typedef struct _LigmaSearchPopupClass      LigmaSearchPopupClass;
typedef struct _LigmaSearchPopupPrivate    LigmaSearchPopupPrivate;

typedef void   (*LigmaSearchPopupCallback) (LigmaSearchPopup  *popup,
                                           const gchar      *search,
                                           gpointer          data);

struct _LigmaSearchPopup
{
  LigmaPopup               parent_instance;

  LigmaSearchPopupPrivate *priv;
};

struct _LigmaSearchPopupClass
{
  LigmaPopupClass          parent_class;
};

GType       ligma_search_popup_get_type   (void);

GtkWidget * ligma_search_popup_new        (Ligma                    *ligma,
                                          const gchar             *role,
                                          const gchar             *title,
                                          LigmaSearchPopupCallback  callback,
                                          gpointer                 callback_data);

void        ligma_search_popup_add_result (LigmaSearchPopup *popup,
                                          LigmaAction      *action,
                                          gint             section);

#endif  /*  __LIGMA_SEARCH_POPUP_H__  */
