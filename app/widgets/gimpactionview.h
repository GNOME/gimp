/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaactionview.h
 * Copyright (C) 2004-2005  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_ACTION_VIEW_H__
#define __LIGMA_ACTION_VIEW_H__


enum
{
  LIGMA_ACTION_VIEW_COLUMN_VISIBLE,
  LIGMA_ACTION_VIEW_COLUMN_ACTION,
  LIGMA_ACTION_VIEW_COLUMN_ICON_NAME,
  LIGMA_ACTION_VIEW_COLUMN_LABEL,
  LIGMA_ACTION_VIEW_COLUMN_LABEL_CASEFOLD,
  LIGMA_ACTION_VIEW_COLUMN_NAME,
  LIGMA_ACTION_VIEW_COLUMN_ACCEL_KEY,
  LIGMA_ACTION_VIEW_COLUMN_ACCEL_MASK,
  LIGMA_ACTION_VIEW_COLUMN_ACCEL_CLOSURE,
  LIGMA_ACTION_VIEW_N_COLUMNS
};


#define LIGMA_TYPE_ACTION_VIEW            (ligma_action_view_get_type ())
#define LIGMA_ACTION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ACTION_VIEW, LigmaActionView))
#define LIGMA_ACTION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ACTION_VIEW, LigmaActionViewClass))
#define LIGMA_IS_ACTION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ACTION_VIEW))
#define LIGMA_IS_ACTION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ACTION_VIEW))
#define LIGMA_ACTION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ACTION_VIEW, LigmaActionViewClass))


typedef struct _LigmaActionViewClass LigmaActionViewClass;

struct _LigmaActionView
{
  GtkTreeView    parent_instance;

  LigmaUIManager *manager;
  gboolean       show_shortcuts;

  gchar         *filter;
};

struct _LigmaActionViewClass
{
  GtkTreeViewClass  parent_class;
};


GType       ligma_action_view_get_type   (void) G_GNUC_CONST;

GtkWidget * ligma_action_view_new        (LigmaUIManager  *manager,
                                         const gchar    *select_action,
                                         gboolean        show_shortcuts);

void        ligma_action_view_set_filter (LigmaActionView *view,
                                         const gchar    *filter);


#endif  /*  __LIGMA_ACTION_VIEW_H__  */
