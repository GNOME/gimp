/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatagpopup.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#ifndef __LIGMA_TAG_POPUP_H__
#define __LIGMA_TAG_POPUP_H__


#define LIGMA_TYPE_TAG_POPUP            (ligma_tag_popup_get_type ())
#define LIGMA_TAG_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TAG_POPUP, LigmaTagPopup))
#define LIGMA_TAG_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TAG_POPUP, LigmaTagPopupClass))
#define LIGMA_IS_TAG_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TAG_POPUP))
#define LIGMA_IS_TAG_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TAG_POPUP))
#define LIGMA_TAG_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TAG_POPUP, LigmaTagPopupClass))


typedef struct _LigmaTagPopupClass  LigmaTagPopupClass;

typedef struct _PopupTagData PopupTagData;

struct _LigmaTagPopup
{
  GtkWindow          parent_instance;

  LigmaComboTagEntry *combo_entry;

  GtkWidget         *frame;
  GtkWidget         *border_area;
  GtkWidget         *tag_area;

  PangoLayout       *layout;

  PopupTagData      *tag_data;
  gint               tag_count;

  PopupTagData      *prelight;

  gboolean           single_select_disabled;

  guint              scroll_timeout_id;
  gint               scroll_height;
  gint               scroll_y;
  gint               scroll_step;
  gint               scroll_arrow_height;
  gboolean           scroll_fast;
  gboolean           arrows_visible;
  gboolean           upper_arrow_prelight;
  gboolean           lower_arrow_prelight;
  GtkStateType       upper_arrow_state;
  GtkStateType       lower_arrow_state;

  gboolean           smooth_scrolling;
};

struct _LigmaTagPopupClass
{
  GtkWindowClass  parent_class;
};


GType       ligma_tag_popup_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_tag_popup_new      (LigmaComboTagEntry *entry);

void        ligma_tag_popup_show     (LigmaTagPopup      *popup,
                                     GdkEvent          *event);


#endif  /*  __LIGMA_TAG_POPUP_H__  */
