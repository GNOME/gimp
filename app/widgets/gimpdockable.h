/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadockable.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DOCKABLE_H__
#define __LIGMA_DOCKABLE_H__


#define LIGMA_DOCKABLE_DRAG_OFFSET (-6)


#define LIGMA_TYPE_DOCKABLE            (ligma_dockable_get_type ())
#define LIGMA_DOCKABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DOCKABLE, LigmaDockable))
#define LIGMA_DOCKABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DOCKABLE, LigmaDockableClass))
#define LIGMA_IS_DOCKABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DOCKABLE))
#define LIGMA_IS_DOCKABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DOCKABLE))
#define LIGMA_DOCKABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DOCKABLE, LigmaDockableClass))


typedef struct _LigmaDockablePrivate LigmaDockablePrivate;
typedef struct _LigmaDockableClass   LigmaDockableClass;

/**
 * LigmaDockable:
 *
 * A kind of adapter to make other widgets dockable. The widget to
 * dock is put inside the LigmaDockable, which is put in a
 * LigmaDockbook.
 */
struct _LigmaDockable
{
  GtkBin               parent_instance;

  LigmaDockablePrivate *p;
};

struct _LigmaDockableClass
{
  GtkBinClass  parent_class;
};


GType           ligma_dockable_get_type          (void) G_GNUC_CONST;

GtkWidget     * ligma_dockable_new               (const gchar   *name,
                                                 const gchar   *blurb,
                                                 const gchar   *icon_name,
                                                 const gchar   *help_id);

void            ligma_dockable_set_dockbook      (LigmaDockable  *dockable,
                                                 LigmaDockbook  *dockbook);
LigmaDockbook  * ligma_dockable_get_dockbook      (LigmaDockable  *dockable);

void            ligma_dockable_set_tab_style     (LigmaDockable  *dockable,
                                                 LigmaTabStyle   tab_style);
LigmaTabStyle    ligma_dockable_get_tab_style     (LigmaDockable  *dockable);

void            ligma_dockable_set_locked        (LigmaDockable  *dockable,
                                                 gboolean       lock);
gboolean        ligma_dockable_get_locked        (LigmaDockable  *dockable);

const gchar   * ligma_dockable_get_name          (LigmaDockable  *dockable);
const gchar   * ligma_dockable_get_blurb         (LigmaDockable  *dockable);
const gchar   * ligma_dockable_get_help_id       (LigmaDockable  *dockable);
const gchar   * ligma_dockable_get_icon_name     (LigmaDockable  *dockable);
GtkWidget     * ligma_dockable_get_icon          (LigmaDockable  *dockable,
                                                 GtkIconSize    size);

GtkWidget     * ligma_dockable_create_tab_widget (LigmaDockable  *dockable,
                                                 LigmaContext   *context,
                                                 LigmaTabStyle   tab_style,
                                                 GtkIconSize    size);
void            ligma_dockable_set_context       (LigmaDockable  *dockable,
                                                 LigmaContext   *context);
LigmaUIManager * ligma_dockable_get_menu          (LigmaDockable  *dockable,
                                                 const gchar  **ui_path,
                                                 gpointer      *popup_data);

void            ligma_dockable_detach            (LigmaDockable  *dockable);


#endif /* __LIGMA_DOCKABLE_H__ */
