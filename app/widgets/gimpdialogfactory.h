/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdialogfactory.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpobject.h"


#define GIMP_DIALOG_VISIBILITY_KEY "gimp-dialog-visibility"

typedef enum
{
  GIMP_DIALOG_VISIBILITY_UNKNOWN = 0,
  GIMP_DIALOG_VISIBILITY_INVISIBLE,
  GIMP_DIALOG_VISIBILITY_VISIBLE,
  GIMP_DIALOG_VISIBILITY_HIDDEN
} GimpDialogVisibilityState;


/* In order to support constructors of various types, these functions
 * takes the union of the set of arguments required for each type of
 * widget constructor. If this set becomes too big we can consider
 * passing a struct or use varargs.
 */
typedef GtkWidget * (* GimpDialogNewFunc)     (GimpDialogFactory      *factory,
                                               GimpContext            *context,
                                               GimpUIManager          *ui_manager,
                                               gint                    view_size);


struct _GimpDialogFactoryEntry
{
  gchar                *identifier;
  gchar                *name;
  gchar                *blurb;
  gchar                *icon_name;
  gchar                *help_id;

  GimpDialogNewFunc     new_func;
  GimpDialogRestoreFunc restore_func;
  gint                  view_size;

  gboolean              singleton;
  gboolean              session_managed;
  gboolean              remember_size;
  gboolean              remember_if_open;

  /* If TRUE the visibility of the dialog is toggleable */
  gboolean              hideable;

  /* If TRUE the entry is for a GimpImageWindow, FALSE otherwise */
  gboolean              image_window;

  /* If TRUE the entry is for a dockable, FALSE otherwise */
  gboolean              dockable;
};


#define GIMP_TYPE_DIALOG_FACTORY            (gimp_dialog_factory_get_type ())
#define GIMP_DIALOG_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactory))
#define GIMP_DIALOG_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactoryClass))
#define GIMP_IS_DIALOG_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DIALOG_FACTORY))
#define GIMP_IS_DIALOG_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DIALOG_FACTORY))
#define GIMP_DIALOG_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DIALOG_FACTORY, GimpDialogFactoryClass))


typedef struct _GimpDialogFactoryPrivate  GimpDialogFactoryPrivate;
typedef struct _GimpDialogFactoryClass    GimpDialogFactoryClass;

/**
 * GimpDialogFactory:
 *
 * A factory with the main purpose of creating toplevel windows and
 * position them according to the session information kept within the
 * factory. Over time it has accumulated more functionality than this.
 */
struct _GimpDialogFactory
{
  GimpObject                parent_instance;

  GimpDialogFactoryPrivate *p;
};

struct _GimpDialogFactoryClass
{
  GimpObjectClass  parent_class;

  void (* dock_window_added)   (GimpDialogFactory *factory,
                                GimpDockWindow    *dock_window);
  void (* dock_window_removed) (GimpDialogFactory *factory,
                                GimpDockWindow    *dock_window);
};


GType               gimp_dialog_factory_get_type             (void) G_GNUC_CONST;
GimpDialogFactory * gimp_dialog_factory_new                  (const gchar             *name,
                                                              GimpContext             *context);

void                gimp_dialog_factory_register_entry       (GimpDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              const gchar             *name,
                                                              const gchar             *blurb,
                                                              const gchar             *icon_name,
                                                              const gchar             *help_id,
                                                              GimpDialogNewFunc        new_func,
                                                              GimpDialogRestoreFunc    restore_func,
                                                              gint                     view_size,
                                                              gboolean                 singleton,
                                                              gboolean                 session_managed,
                                                              gboolean                 remember_size,
                                                              gboolean                 remember_if_open,
                                                              gboolean                 hideable,
                                                              gboolean                 image_window,
                                                              gboolean                 dockable);

GimpDialogFactoryEntry *
                    gimp_dialog_factory_find_entry           (GimpDialogFactory       *factory,
                                                              const gchar             *identifier);
GimpSessionInfo *   gimp_dialog_factory_find_session_info    (GimpDialogFactory       *factory,
                                                              const gchar             *identifier);
GtkWidget *         gimp_dialog_factory_find_widget          (GimpDialogFactory       *factory,
                                                              const gchar             *identifiers);

GtkWidget *         gimp_dialog_factory_dialog_new           (GimpDialogFactory       *factory,
                                                              GdkMonitor              *monitor,
                                                              GimpUIManager           *ui_manager,
                                                              GtkWidget               *parent,
                                                              const gchar             *identifier,
                                                              gint                     view_size,
                                                              gboolean                 present);

GimpContext *       gimp_dialog_factory_get_context          (GimpDialogFactory       *factory);
GList *             gimp_dialog_factory_get_open_dialogs     (GimpDialogFactory       *factory);

GList *             gimp_dialog_factory_get_session_infos    (GimpDialogFactory       *factory);
void                gimp_dialog_factory_add_session_info     (GimpDialogFactory       *factory,
                                                              GimpSessionInfo         *info);

GtkWidget *         gimp_dialog_factory_dialog_raise         (GimpDialogFactory       *factory,
                                                              GdkMonitor              *monitor,
                                                              GtkWidget               *parent,
                                                              const gchar             *identifiers,
                                                              gint                     view_size);

GtkWidget *         gimp_dialog_factory_dockable_new         (GimpDialogFactory       *factory,
                                                              GimpDock                *dock,
                                                              const gchar             *identifier,
                                                              gint                     view_size);

void                gimp_dialog_factory_add_dialog           (GimpDialogFactory       *factory,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                gimp_dialog_factory_add_foreign          (GimpDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                gimp_dialog_factory_position_dialog      (GimpDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                gimp_dialog_factory_remove_dialog        (GimpDialogFactory       *factory,
                                                              GtkWidget               *dialog);

void                gimp_dialog_factory_hide_dialog          (GtkWidget               *dialog);

void                gimp_dialog_factory_save                 (GimpDialogFactory       *factory,
                                                              GimpConfigWriter        *writer);
void                gimp_dialog_factory_restore              (GimpDialogFactory       *factory,
                                                              GdkMonitor              *monitor);

void                gimp_dialog_factory_set_state            (GimpDialogFactory       *factory,
                                                              GimpDialogsState         state);
GimpDialogsState    gimp_dialog_factory_get_state            (GimpDialogFactory       *factory);

void                gimp_dialog_factory_show_with_display    (GimpDialogFactory       *factory);
void                gimp_dialog_factory_hide_with_display    (GimpDialogFactory       *factory);

void                gimp_dialog_factory_set_busy             (GimpDialogFactory       *factory);
void                gimp_dialog_factory_unset_busy           (GimpDialogFactory       *factory);

GimpDialogFactory * gimp_dialog_factory_from_widget          (GtkWidget               *dialog,
                                                              GimpDialogFactoryEntry **entry);

void                gimp_dialog_factory_set_has_min_size     (GtkWindow               *window,
                                                              gboolean                 has_min_size);
gboolean            gimp_dialog_factory_get_has_min_size     (GtkWindow               *window);

GimpDialogFactory * gimp_dialog_factory_get_singleton        (void);
void                gimp_dialog_factory_set_singleton        (GimpDialogFactory       *factory);
