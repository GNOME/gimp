/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprefsbox.h
 * Copyright (C) 2013-2016 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_PREFS_BOX            (gimp_prefs_box_get_type ())
#define GIMP_PREFS_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PREFS_BOX, GimpPrefsBox))
#define GIMP_PREFS_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PREFS_BOX, GimpPrefsBoxClass))
#define GIMP_IS_PREFS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PREFS_BOX))
#define GIMP_IS_PREFS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PREFS_BOX))
#define GIMP_PREFS_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PREFS_BOX, GimpPrefsBoxClass))


typedef struct _GimpPrefsBoxPrivate GimpPrefsBoxPrivate;
typedef struct _GimpPrefsBoxClass   GimpPrefsBoxClass;

struct _GimpPrefsBox
{
  GtkBox               parent_instance;

  GimpPrefsBoxPrivate *priv;
};

struct _GimpPrefsBoxClass
{
  GtkBoxClass  parent_class;
};


GType         gimp_prefs_box_get_type              (void) G_GNUC_CONST;

GtkWidget   * gimp_prefs_box_new                   (void);

GtkWidget   * gimp_prefs_box_add_page              (GimpPrefsBox *box,
                                                    const gchar  *icon_name,
                                                    const gchar  *page_title,
                                                    const gchar  *tree_label,
                                                    const gchar  *help_id,
                                                    GtkTreeIter  *parent,
                                                    GtkTreeIter  *iter);

const gchar * gimp_prefs_box_get_current_icon_name (GimpPrefsBox *box);
const gchar * gimp_prefs_box_get_current_help_id   (GimpPrefsBox *box);

void          gimp_prefs_box_set_header_visible    (GimpPrefsBox *box,
                                                    gboolean      header_visible);
void          gimp_prefs_box_set_page_scrollable   (GimpPrefsBox *box,
                                                    GtkWidget    *page,
                                                    gboolean      scrollable);
GtkWidget   * gimp_prefs_box_set_page_resettable   (GimpPrefsBox *box,
                                                    GtkWidget    *page,
                                                    const gchar  *label);

GtkWidget   * gimp_prefs_box_get_tree_view         (GimpPrefsBox *box);
GtkWidget   * gimp_prefs_box_get_stack             (GimpPrefsBox *box);
