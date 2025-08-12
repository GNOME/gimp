/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginview.h
 * Copyright (C) 2017  Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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


#define GIMP_TYPE_PLUG_IN_VIEW            (gimp_plug_in_view_get_type ())
#define GIMP_PLUG_IN_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUG_IN_VIEW, GimpPlugInView))
#define GIMP_PLUG_IN_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUG_IN_VIEW, GimpPlugInViewClass))
#define GIMP_IS_PLUG_IN_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUG_IN_VIEW))
#define GIMP_IS_PLUG_IN_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PLUG_IN_VIEW))
#define GIMP_PLUG_IN_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PLUG_IN_VIEW, GimpPlugInViewClass))


typedef struct _GimpPlugInViewClass GimpPlugInViewClass;

struct _GimpPlugInView
{
  GtkTreeView  parent_instance;

  GHashTable  *plug_in_hash;
};

struct _GimpPlugInViewClass
{
  GtkTreeViewClass   parent_class;

  void (* changed) (GimpPlugInView *view);
};


GType       gimp_plug_in_view_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_plug_in_view_new         (GSList         *procedures);

gchar     * gimp_plug_in_view_get_plug_in (GimpPlugInView *view);
gboolean    gimp_plug_in_view_set_plug_in (GimpPlugInView *view,
                                           const gchar    *path);
