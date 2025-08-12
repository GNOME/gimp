/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagpopup.h
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

#pragma once


#define GIMP_TYPE_TAG_POPUP            (gimp_tag_popup_get_type ())
#define GIMP_TAG_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TAG_POPUP, GimpTagPopup))
#define GIMP_TAG_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TAG_POPUP, GimpTagPopupClass))
#define GIMP_IS_TAG_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TAG_POPUP))
#define GIMP_IS_TAG_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TAG_POPUP))
#define GIMP_TAG_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TAG_POPUP, GimpTagPopupClass))


typedef struct _GimpTagPopupClass  GimpTagPopupClass;

typedef struct _PopupTagData PopupTagData;

struct _GimpTagPopup
{
  GtkWindow          parent_instance;

  GimpComboTagEntry *combo_entry;

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
  GtkStateFlags      upper_arrow_state;
  GtkStateFlags      lower_arrow_state;

  gboolean           smooth_scrolling;
};

struct _GimpTagPopupClass
{
  GtkWindowClass  parent_class;
};


GType       gimp_tag_popup_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_tag_popup_new      (GimpComboTagEntry *entry);

void        gimp_tag_popup_show     (GimpTagPopup      *popup,
                                     GdkEvent          *event);
