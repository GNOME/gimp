/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprogress.h
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PROGRESS_H__
#define __GIMP_PROGRESS_H__

G_BEGIN_DECLS

/**
 * GimpProgressVtableStartFunc:
 * @message:
 * @cancelable:
 * @user_data: (closure):
 *
 * Starts the progress
 */
typedef void (* GimpProgressVtableStartFunc) (const gchar *message,
                                              gboolean     cancelable,
                                              gpointer     user_data);

/**
 * GimpProgressVtableEndFunc:
 * @user_data: (closure):
 *
 * Ends the progress
 */
typedef void (* GimpProgressVtableEndFunc) (gpointer user_data);

/**
 * GimpProgressVtableSetTextFunc:
 * @message:
 * @user_data: (closure):
 *
 * Sets a new text on the progress.
 */
typedef void (* GimpProgressVtableSetTextFunc) (const gchar *message,
                                                gpointer     user_data);

/**
 * GimpProgressVtableSetValueFunc:
 * @percentage:
 * @user_data: (closure):
 *
 * Sets a new percentage on the progress.
 */
typedef void (* GimpProgressVtableSetValueFunc) (gdouble  percentage,
                                                 gpointer user_data);

/**
 * GimpProgressVtablePulseFunc:
 * @user_data: (closure):
 *
 * Makes the progress pulse
 */
typedef void (* GimpProgressVtablePulseFunc) (gpointer user_data);

/**
 * GimpProgressVtableGetWindowFunc:
 * @user_data: (closure):
 *
 * Returns: the ID of the window where the progress is displayed.
 */
typedef guint32 (* GimpProgressVtableGetWindowFunc) (gpointer user_data);


typedef struct _GimpProgressVtable GimpProgressVtable;

/**
 * GimpProgressVtable:
 * @start:      starts the progress.
 * @end:        ends the progress.
 * @set_text:   sets a new text on the progress.
 * @set_value:  sets a new percentage on the progress.
 * @pulse:      makes the progress pulse.
 * @get_window: returns the ID of the window where the progress is displayed.
 * @_gimp_reserved1: reserved pointer for future expansion.
 * @_gimp_reserved2: reserved pointer for future expansion.
 * @_gimp_reserved3: reserved pointer for future expansion.
 * @_gimp_reserved4: reserved pointer for future expansion.
 * @_gimp_reserved5: reserved pointer for future expansion.
 * @_gimp_reserved6: reserved pointer for future expansion.
 * @_gimp_reserved7: reserved pointer for future expansion.
 * @_gimp_reserved8: reserved pointer for future expansion.
 **/
struct _GimpProgressVtable
{
  GimpProgressVtableStartFunc     start;
  GimpProgressVtableEndFunc       end;
  GimpProgressVtableSetTextFunc   set_text;
  GimpProgressVtableSetValueFunc  set_value;
  GimpProgressVtablePulseFunc     pulse;
  GimpProgressVtableGetWindowFunc get_window;

  /* Padding for future expansion. Must be initialized with NULL! */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


const gchar * gimp_progress_install_vtable  (const GimpProgressVtable *vtable,
                                             gpointer                  user_data,
                                             GDestroyNotify            user_data_destroy);
void          gimp_progress_uninstall       (const gchar              *progress_callback);

gboolean      gimp_progress_init            (const gchar              *message);
gboolean      gimp_progress_init_printf     (const gchar              *format,
                                             ...) G_GNUC_PRINTF (1, 2);

gboolean      gimp_progress_set_text_printf (const gchar              *format,
                                             ...) G_GNUC_PRINTF (1, 2);

gboolean      gimp_progress_update          (gdouble                   percentage);


G_END_DECLS

#endif /* __GIMP_PROGRESS_H__ */
