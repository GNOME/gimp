/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaprogress.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_PROGRESS_H__
#define __LIGMA_PROGRESS_H__

G_BEGIN_DECLS

/**
 * LigmaProgressVtableStartFunc:
 * @message: The message to show
 * @cancelable: Whether the procedure is cancelable
 * @user_data: (closure): User data
 *
 * Starts the progress
 */
typedef void (* LigmaProgressVtableStartFunc) (const gchar *message,
                                              gboolean     cancelable,
                                              gpointer     user_data);

/**
 * LigmaProgressVtableEndFunc:
 * @user_data: (closure): User data
 *
 * Ends the progress
 */
typedef void (* LigmaProgressVtableEndFunc) (gpointer user_data);

/**
 * LigmaProgressVtableSetTextFunc:
 * @message: The new text
 * @user_data: (closure): User data
 *
 * Sets a new text on the progress.
 */
typedef void (* LigmaProgressVtableSetTextFunc) (const gchar *message,
                                                gpointer     user_data);

/**
 * LigmaProgressVtableSetValueFunc:
 * @percentage: The progress in percent
 * @user_data: (closure): User data
 *
 * Sets a new percentage on the progress.
 */
typedef void (* LigmaProgressVtableSetValueFunc) (gdouble  percentage,
                                                 gpointer user_data);

/**
 * LigmaProgressVtablePulseFunc:
 * @user_data: (closure): User data
 *
 * Makes the progress pulse
 */
typedef void (* LigmaProgressVtablePulseFunc) (gpointer user_data);

/**
 * LigmaProgressVtableGetWindowFunc:
 * @user_data: (closure): User data
 *
 * Returns: the ID of the window where the progress is displayed.
 */
typedef guint64 (* LigmaProgressVtableGetWindowFunc) (gpointer user_data);


typedef struct _LigmaProgressVtable LigmaProgressVtable;

/**
 * LigmaProgressVtable:
 * @start:      starts the progress.
 * @end:        ends the progress.
 * @set_text:   sets a new text on the progress.
 * @set_value:  sets a new percentage on the progress.
 * @pulse:      makes the progress pulse.
 * @get_window: returns the ID of the window where the progress is displayed.
 * @_ligma_reserved1: reserved pointer for future expansion.
 * @_ligma_reserved2: reserved pointer for future expansion.
 * @_ligma_reserved3: reserved pointer for future expansion.
 * @_ligma_reserved4: reserved pointer for future expansion.
 * @_ligma_reserved5: reserved pointer for future expansion.
 * @_ligma_reserved6: reserved pointer for future expansion.
 * @_ligma_reserved7: reserved pointer for future expansion.
 * @_ligma_reserved8: reserved pointer for future expansion.
 **/
struct _LigmaProgressVtable
{
  LigmaProgressVtableStartFunc     start;
  LigmaProgressVtableEndFunc       end;
  LigmaProgressVtableSetTextFunc   set_text;
  LigmaProgressVtableSetValueFunc  set_value;
  LigmaProgressVtablePulseFunc     pulse;
  LigmaProgressVtableGetWindowFunc get_window;

  /* Padding for future expansion. Must be initialized with NULL! */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


const gchar * ligma_progress_install_vtable  (const LigmaProgressVtable *vtable,
                                             gpointer                  user_data,
                                             GDestroyNotify            user_data_destroy);
void          ligma_progress_uninstall       (const gchar              *progress_callback);

gboolean      ligma_progress_init            (const gchar              *message);
gboolean      ligma_progress_init_printf     (const gchar              *format,
                                             ...) G_GNUC_PRINTF (1, 2);

gboolean      ligma_progress_set_text_printf (const gchar              *format,
                                             ...) G_GNUC_PRINTF (1, 2);

gboolean      ligma_progress_update          (gdouble                   percentage);


G_END_DECLS

#endif /* __LIGMA_PROGRESS_H__ */
