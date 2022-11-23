/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaprefsbox.h
 * Copyright (C) 2013-2016 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PREFS_BOX_H__
#define __LIGMA_PREFS_BOX_H__


#define LIGMA_TYPE_PREFS_BOX            (ligma_prefs_box_get_type ())
#define LIGMA_PREFS_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PREFS_BOX, LigmaPrefsBox))
#define LIGMA_PREFS_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PREFS_BOX, LigmaPrefsBoxClass))
#define LIGMA_IS_PREFS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PREFS_BOX))
#define LIGMA_IS_PREFS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PREFS_BOX))
#define LIGMA_PREFS_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PREFS_BOX, LigmaPrefsBoxClass))


typedef struct _LigmaPrefsBoxPrivate LigmaPrefsBoxPrivate;
typedef struct _LigmaPrefsBoxClass   LigmaPrefsBoxClass;

struct _LigmaPrefsBox
{
  GtkBox               parent_instance;

  LigmaPrefsBoxPrivate *priv;
};

struct _LigmaPrefsBoxClass
{
  GtkBoxClass  parent_class;
};


GType         ligma_prefs_box_get_type              (void) G_GNUC_CONST;

GtkWidget   * ligma_prefs_box_new                   (void);

GtkWidget   * ligma_prefs_box_add_page              (LigmaPrefsBox *box,
                                                    const gchar  *icon_name,
                                                    const gchar  *page_title,
                                                    const gchar  *tree_label,
                                                    const gchar  *help_id,
                                                    GtkTreeIter  *parent,
                                                    GtkTreeIter  *iter);

const gchar * ligma_prefs_box_get_current_icon_name (LigmaPrefsBox *box);
const gchar * ligma_prefs_box_get_current_help_id   (LigmaPrefsBox *box);

void          ligma_prefs_box_set_page_scrollable   (LigmaPrefsBox *box,
                                                    GtkWidget    *page,
                                                    gboolean      scrollable);
GtkWidget   * ligma_prefs_box_set_page_resettable   (LigmaPrefsBox *box,
                                                    GtkWidget    *page,
                                                    const gchar  *label);

GtkWidget   * ligma_prefs_box_get_tree_view         (LigmaPrefsBox *box);


#endif  /*  __LIGMA_PREFS_BOX_H__  */
