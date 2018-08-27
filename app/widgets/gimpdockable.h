/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockable.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_DOCKABLE_H__
#define __GIMP_DOCKABLE_H__


#define GIMP_DOCKABLE_DRAG_OFFSET (-6)


#define GIMP_TYPE_DOCKABLE            (gimp_dockable_get_type ())
#define GIMP_DOCKABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCKABLE, GimpDockable))
#define GIMP_DOCKABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCKABLE, GimpDockableClass))
#define GIMP_IS_DOCKABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCKABLE))
#define GIMP_IS_DOCKABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCKABLE))
#define GIMP_DOCKABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DOCKABLE, GimpDockableClass))


typedef struct _GimpDockablePrivate GimpDockablePrivate;
typedef struct _GimpDockableClass   GimpDockableClass;

/**
 * GimpDockable:
 *
 * A kind of adapter to make other widgets dockable. The widget to
 * dock is put inside the GimpDockable, which is put in a
 * GimpDockbook.
 */
struct _GimpDockable
{
  GtkBin               parent_instance;

  GimpDockablePrivate *p;
};

struct _GimpDockableClass
{
  GtkBinClass  parent_class;
};


GType           gimp_dockable_get_type          (void) G_GNUC_CONST;

GtkWidget     * gimp_dockable_new               (const gchar   *name,
                                                 const gchar   *blurb,
                                                 const gchar   *icon_name,
                                                 const gchar   *help_id);

void            gimp_dockable_set_dockbook      (GimpDockable  *dockable,
                                                 GimpDockbook  *dockbook);
GimpDockbook  * gimp_dockable_get_dockbook      (GimpDockable  *dockable);

void            gimp_dockable_set_tab_style     (GimpDockable  *dockable,
                                                 GimpTabStyle   tab_style);
GimpTabStyle    gimp_dockable_get_tab_style     (GimpDockable  *dockable);

void            gimp_dockable_set_locked        (GimpDockable  *dockable,
                                                 gboolean       lock);
gboolean        gimp_dockable_get_locked        (GimpDockable  *dockable);

const gchar   * gimp_dockable_get_name          (GimpDockable  *dockable);
const gchar   * gimp_dockable_get_blurb         (GimpDockable  *dockable);
const gchar   * gimp_dockable_get_help_id       (GimpDockable  *dockable);
const gchar   * gimp_dockable_get_icon_name     (GimpDockable  *dockable);
GtkWidget     * gimp_dockable_get_icon          (GimpDockable  *dockable,
                                                 GtkIconSize    size);

GtkWidget     * gimp_dockable_create_tab_widget (GimpDockable  *dockable,
                                                 GimpContext   *context,
                                                 GimpTabStyle   tab_style,
                                                 GtkIconSize    size);
void            gimp_dockable_set_context       (GimpDockable  *dockable,
                                                 GimpContext   *context);
GimpUIManager * gimp_dockable_get_menu          (GimpDockable  *dockable,
                                                 const gchar  **ui_path,
                                                 gpointer      *popup_data);

void            gimp_dockable_detach            (GimpDockable  *dockable);


#endif /* __GIMP_DOCKABLE_H__ */
