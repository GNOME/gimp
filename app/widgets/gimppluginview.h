/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginview.h
 * Copyright (C) 2017  Michael Natterer <mitch@ligma.org>
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_PLUG_IN_VIEW_H__
#define __LIGMA_PLUG_IN_VIEW_H__


#define LIGMA_TYPE_PLUG_IN_VIEW            (ligma_plug_in_view_get_type ())
#define LIGMA_PLUG_IN_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PLUG_IN_VIEW, LigmaPlugInView))
#define LIGMA_PLUG_IN_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PLUG_IN_VIEW, LigmaPlugInViewClass))
#define LIGMA_IS_PLUG_IN_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PLUG_IN_VIEW))
#define LIGMA_IS_PLUG_IN_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PLUG_IN_VIEW))
#define LIGMA_PLUG_IN_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PLUG_IN_VIEW, LigmaPlugInViewClass))


typedef struct _LigmaPlugInViewClass LigmaPlugInViewClass;

struct _LigmaPlugInView
{
  GtkTreeView  parent_instance;

  GHashTable  *plug_in_hash;
};

struct _LigmaPlugInViewClass
{
  GtkTreeViewClass   parent_class;

  void (* changed) (LigmaPlugInView *view);
};


GType       ligma_plug_in_view_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_plug_in_view_new         (GSList         *procedures);

gchar     * ligma_plug_in_view_get_plug_in (LigmaPlugInView *view);
gboolean    ligma_plug_in_view_set_plug_in (LigmaPlugInView *view,
                                           const gchar    *path);


#endif  /*  __LIGMA_PLUG_IN_VIEW_H__  */
