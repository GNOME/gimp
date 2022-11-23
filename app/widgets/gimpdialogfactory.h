/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadialogfactory.h
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DIALOG_FACTORY_H__
#define __LIGMA_DIALOG_FACTORY_H__


#include "core/ligmaobject.h"

#define LIGMA_DIALOG_VISIBILITY_KEY "ligma-dialog-visibility"

typedef enum
{
  LIGMA_DIALOG_VISIBILITY_UNKNOWN = 0,
  LIGMA_DIALOG_VISIBILITY_INVISIBLE,
  LIGMA_DIALOG_VISIBILITY_VISIBLE,
  LIGMA_DIALOG_VISIBILITY_HIDDEN
} LigmaDialogVisibilityState;


/* In order to support constructors of various types, these functions
 * takes the union of the set of arguments required for each type of
 * widget constructor. If this set becomes too big we can consider
 * passing a struct or use varargs.
 */
typedef GtkWidget * (* LigmaDialogNewFunc)     (LigmaDialogFactory      *factory,
                                               LigmaContext            *context,
                                               LigmaUIManager          *ui_manager,
                                               gint                    view_size);


struct _LigmaDialogFactoryEntry
{
  gchar                *identifier;
  gchar                *name;
  gchar                *blurb;
  gchar                *icon_name;
  gchar                *help_id;

  LigmaDialogNewFunc     new_func;
  LigmaDialogRestoreFunc restore_func;
  gint                  view_size;

  gboolean              singleton;
  gboolean              session_managed;
  gboolean              remember_size;
  gboolean              remember_if_open;

  /* If TRUE the visibility of the dialog is toggleable */
  gboolean              hideable;

  /* If TRUE the entry is for a LigmaImageWindow, FALSE otherwise */
  gboolean              image_window;

  /* If TRUE the entry is for a dockable, FALSE otherwise */
  gboolean              dockable;
};


#define LIGMA_TYPE_DIALOG_FACTORY            (ligma_dialog_factory_get_type ())
#define LIGMA_DIALOG_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DIALOG_FACTORY, LigmaDialogFactory))
#define LIGMA_DIALOG_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DIALOG_FACTORY, LigmaDialogFactoryClass))
#define LIGMA_IS_DIALOG_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DIALOG_FACTORY))
#define LIGMA_IS_DIALOG_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DIALOG_FACTORY))
#define LIGMA_DIALOG_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DIALOG_FACTORY, LigmaDialogFactoryClass))


typedef struct _LigmaDialogFactoryPrivate  LigmaDialogFactoryPrivate;
typedef struct _LigmaDialogFactoryClass    LigmaDialogFactoryClass;

/**
 * LigmaDialogFactory:
 *
 * A factory with the main purpose of creating toplevel windows and
 * position them according to the session information kept within the
 * factory. Over time it has accumulated more functionality than this.
 */
struct _LigmaDialogFactory
{
  LigmaObject                parent_instance;

  LigmaDialogFactoryPrivate *p;
};

struct _LigmaDialogFactoryClass
{
  LigmaObjectClass  parent_class;

  void (* dock_window_added)   (LigmaDialogFactory *factory,
                                LigmaDockWindow    *dock_window);
  void (* dock_window_removed) (LigmaDialogFactory *factory,
                                LigmaDockWindow    *dock_window);
};


GType               ligma_dialog_factory_get_type             (void) G_GNUC_CONST;
LigmaDialogFactory * ligma_dialog_factory_new                  (const gchar             *name,
                                                              LigmaContext             *context,
                                                              LigmaMenuFactory         *menu_factory);

void                ligma_dialog_factory_register_entry       (LigmaDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              const gchar             *name,
                                                              const gchar             *blurb,
                                                              const gchar             *icon_name,
                                                              const gchar             *help_id,
                                                              LigmaDialogNewFunc        new_func,
                                                              LigmaDialogRestoreFunc    restore_func,
                                                              gint                     view_size,
                                                              gboolean                 singleton,
                                                              gboolean                 session_managed,
                                                              gboolean                 remember_size,
                                                              gboolean                 remember_if_open,
                                                              gboolean                 hideable,
                                                              gboolean                 image_window,
                                                              gboolean                 dockable);

LigmaDialogFactoryEntry *
                    ligma_dialog_factory_find_entry           (LigmaDialogFactory       *factory,
                                                              const gchar             *identifier);
LigmaSessionInfo *   ligma_dialog_factory_find_session_info    (LigmaDialogFactory       *factory,
                                                              const gchar             *identifier);
GtkWidget *         ligma_dialog_factory_find_widget          (LigmaDialogFactory       *factory,
                                                              const gchar             *identifiers);

GtkWidget *         ligma_dialog_factory_dialog_new           (LigmaDialogFactory       *factory,
                                                              GdkMonitor              *monitor,
                                                              LigmaUIManager           *ui_manager,
                                                              GtkWidget               *parent,
                                                              const gchar             *identifier,
                                                              gint                     view_size,
                                                              gboolean                 present);

LigmaContext *       ligma_dialog_factory_get_context          (LigmaDialogFactory       *factory);
LigmaMenuFactory *   ligma_dialog_factory_get_menu_factory     (LigmaDialogFactory       *factory);
GList *             ligma_dialog_factory_get_open_dialogs     (LigmaDialogFactory       *factory);

GList *             ligma_dialog_factory_get_session_infos    (LigmaDialogFactory       *factory);
void                ligma_dialog_factory_add_session_info     (LigmaDialogFactory       *factory,
                                                              LigmaSessionInfo         *info);

GtkWidget *         ligma_dialog_factory_dialog_raise         (LigmaDialogFactory       *factory,
                                                              GdkMonitor              *monitor,
                                                              GtkWidget               *parent,
                                                              const gchar             *identifiers,
                                                              gint                     view_size);

GtkWidget *         ligma_dialog_factory_dockable_new         (LigmaDialogFactory       *factory,
                                                              LigmaDock                *dock,
                                                              const gchar             *identifier,
                                                              gint                     view_size);

void                ligma_dialog_factory_add_dialog           (LigmaDialogFactory       *factory,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                ligma_dialog_factory_add_foreign          (LigmaDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                ligma_dialog_factory_position_dialog      (LigmaDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                ligma_dialog_factory_remove_dialog        (LigmaDialogFactory       *factory,
                                                              GtkWidget               *dialog);

void                ligma_dialog_factory_hide_dialog          (GtkWidget               *dialog);

void                ligma_dialog_factory_save                 (LigmaDialogFactory       *factory,
                                                              LigmaConfigWriter        *writer);
void                ligma_dialog_factory_restore              (LigmaDialogFactory       *factory,
                                                              GdkMonitor              *monitor);

void                ligma_dialog_factory_set_state            (LigmaDialogFactory       *factory,
                                                              LigmaDialogsState         state);
LigmaDialogsState    ligma_dialog_factory_get_state            (LigmaDialogFactory       *factory);

void                ligma_dialog_factory_show_with_display    (LigmaDialogFactory       *factory);
void                ligma_dialog_factory_hide_with_display    (LigmaDialogFactory       *factory);

void                ligma_dialog_factory_set_busy             (LigmaDialogFactory       *factory);
void                ligma_dialog_factory_unset_busy           (LigmaDialogFactory       *factory);

LigmaDialogFactory * ligma_dialog_factory_from_widget          (GtkWidget               *dialog,
                                                              LigmaDialogFactoryEntry **entry);

void                ligma_dialog_factory_set_has_min_size     (GtkWindow               *window,
                                                              gboolean                 has_min_size);
gboolean            ligma_dialog_factory_get_has_min_size     (GtkWindow               *window);

LigmaDialogFactory * ligma_dialog_factory_get_singleton        (void);
void                ligma_dialog_factory_set_singleton        (LigmaDialogFactory       *factory);


#endif  /*  __LIGMA_DIALOG_FACTORY_H__  */
