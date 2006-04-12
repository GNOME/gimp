/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "pdb/gimppluginprocedure.h"

#include "gimp.h"
#include "gimp-gui.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpprogress.h"

#include "about.h"

#include "gimp-intl.h"


void
gimp_gui_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->gui.threads_enter       = NULL;
  gimp->gui.threads_leave       = NULL;
  gimp->gui.set_busy            = NULL;
  gimp->gui.unset_busy          = NULL;
  gimp->gui.message             = NULL;
  gimp->gui.help                = NULL;
  gimp->gui.get_program_class   = NULL;
  gimp->gui.get_display_name    = NULL;
  gimp->gui.get_theme_dir       = NULL;
  gimp->gui.display_get_by_id   = NULL;
  gimp->gui.display_get_id      = NULL;
  gimp->gui.display_get_window  = NULL;
  gimp->gui.display_create      = NULL;
  gimp->gui.display_delete      = NULL;
  gimp->gui.displays_reconnect  = NULL;
  gimp->gui.menus_init          = NULL;
  gimp->gui.menus_create_item   = NULL;
  gimp->gui.menus_delete_item   = NULL;
  gimp->gui.menus_create_branch = NULL;
  gimp->gui.progress_new        = NULL;
  gimp->gui.progress_free       = NULL;
  gimp->gui.pdb_dialog_set      = NULL;
  gimp->gui.pdb_dialog_close    = NULL;
  gimp->gui.pdb_dialogs_check   = NULL;
}

void
gimp_threads_enter (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->gui.threads_enter)
    gimp->gui.threads_enter (gimp);
}

void
gimp_threads_leave (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->gui.threads_leave)
    gimp->gui.threads_leave (gimp);
}

void
gimp_set_busy (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* FIXME: gimp_busy HACK */
  gimp->busy++;

  if (gimp->busy == 1)
    {
      if (gimp->gui.set_busy)
        gimp->gui.set_busy (gimp);
    }
}

static gboolean
gimp_idle_unset_busy (gpointer data)
{
  Gimp *gimp = data;

  gimp_unset_busy (gimp);

  gimp->busy_idle_id = 0;

  return FALSE;
}

void
gimp_set_busy_until_idle (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! gimp->busy_idle_id)
    {
      gimp_set_busy (gimp);

      gimp->busy_idle_id = g_idle_add_full (G_PRIORITY_HIGH,
                                            gimp_idle_unset_busy, gimp,
                                            NULL);
    }
}

void
gimp_unset_busy (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimp->busy > 0);

  /* FIXME: gimp_busy HACK */
  gimp->busy--;

  if (gimp->busy == 0)
    {
      if (gimp->gui.unset_busy)
        gimp->gui.unset_busy (gimp);
    }
}

void
gimp_message (Gimp        *gimp,
              const gchar *domain,
              const gchar *message)
{
  gchar *message2 = gimp_any_to_utf8 (message, -1,
                                      "Cannot convert message to utf8.");

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! domain)
    domain = GIMP_ACRONYM;

  if (! gimp->console_messages && gimp->gui.message)
    gimp->gui.message (gimp, domain, message2);
  else
    g_printerr ("%s: %s\n\n", domain, message2);

  g_free (message2);
}

void
gimp_help (Gimp        *gimp,
           const gchar *help_domain,
           const gchar *help_id)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->gui.help)
    gimp->gui.help (gimp, help_domain, help_id);
}

const gchar *
gimp_get_program_class (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->gui.get_program_class)
    return gimp->gui.get_program_class (gimp);

  return NULL;
}

gchar *
gimp_get_display_name (Gimp *gimp,
                       gint  display_ID,
                       gint *monitor_number)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (monitor_number != NULL, NULL);

  if (gimp->gui.get_display_name)
    return gimp->gui.get_display_name (gimp, display_ID, monitor_number);

  *monitor_number = 0;

  return NULL;
}

const gchar *
gimp_get_theme_dir (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->gui.get_theme_dir)
    return gimp->gui.get_theme_dir (gimp);

  return NULL;
}

GimpObject *
gimp_get_display_by_ID (Gimp *gimp,
                        gint  ID)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->gui.display_get_by_id)
    return gimp->gui.display_get_by_id (gimp, ID);

  return NULL;
}

gint
gimp_get_display_ID (Gimp       *gimp,
                     GimpObject *display)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), -1);
  g_return_val_if_fail (GIMP_IS_OBJECT (display), -1);

  if (gimp->gui.display_get_id)
    return gimp->gui.display_get_id (display);

  return -1;
}

guint32
gimp_get_display_window (Gimp       *gimp,
                         GimpObject *display)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), -1);
  g_return_val_if_fail (GIMP_IS_OBJECT (display), -1);

  if (gimp->gui.display_get_window)
    return gimp->gui.display_get_window (display);

  return -1;
}

GimpObject *
gimp_create_display (Gimp      *gimp,
                     GimpImage *image,
                     GimpUnit   unit,
                     gdouble    scale)
{
  GimpObject *display = NULL;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  if (gimp->gui.display_create)
    {
      display = gimp->gui.display_create (image, unit, scale);

      gimp_container_add (gimp->displays, display);
    }

  return display;
}

void
gimp_delete_display (Gimp       *gimp,
                     GimpObject *display)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_OBJECT (display));

  if (gimp->gui.display_delete)
    gimp->gui.display_delete (display);
}

void
gimp_reconnect_displays (Gimp      *gimp,
                         GimpImage *old_image,
                         GimpImage *new_image)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_IMAGE (old_image));
  g_return_if_fail (GIMP_IS_IMAGE (new_image));

  if (gimp->gui.displays_reconnect)
    gimp->gui.displays_reconnect (gimp, old_image, new_image);
}

void
gimp_menus_init (Gimp        *gimp,
                 GSList      *plug_in_defs,
                 const gchar *std_plugins_domain)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (std_plugins_domain != NULL);

  if (gimp->gui.menus_init)
    gimp->gui.menus_init (gimp, plug_in_defs, std_plugins_domain);
}

void
gimp_menus_create_item (Gimp                *gimp,
                        GimpPlugInProcedure *proc,
                        const gchar         *menu_path)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  if (gimp->gui.menus_create_item)
    gimp->gui.menus_create_item (gimp, proc, menu_path);
}

void
gimp_menus_delete_item (Gimp                *gimp,
                        GimpPlugInProcedure *proc)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  if (gimp->gui.menus_delete_item)
    gimp->gui.menus_delete_item (gimp, proc);
}

void
gimp_menus_create_branch (Gimp        *gimp,
                          const gchar *progname,
                          const gchar *menu_path,
                          const gchar *menu_label)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (menu_label != NULL);

  if (gimp->gui.menus_create_branch)
    gimp->gui.menus_create_branch (gimp, progname, menu_path, menu_label);
}

GimpProgress *
gimp_new_progress (Gimp       *gimp,
                   GimpObject *display)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (display == NULL || GIMP_IS_OBJECT (display), NULL);

  if (gimp->gui.progress_new)
    return gimp->gui.progress_new (gimp, display);

  return NULL;
}

void
gimp_free_progress (Gimp         *gimp,
                    GimpProgress *progress)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  if (gimp->gui.progress_free)
    gimp->gui.progress_free (gimp, progress);
}

gboolean
gimp_pdb_dialog_new (Gimp          *gimp,
                     GimpContext   *context,
                     GimpContainer *container,
                     const gchar   *title,
                     const gchar   *callback_name,
                     const gchar   *object_name,
                     ...)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (title != NULL, FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);

  if (gimp->gui.pdb_dialog_new)
    {
      va_list  args;

      va_start (args, object_name);

      retval = gimp->gui.pdb_dialog_new (gimp, context, container, title,
                                         callback_name, object_name,
                                         args);

      va_end (args);
    }

  return retval;
}

gboolean
gimp_pdb_dialog_set (Gimp          *gimp,
                     GimpContainer *container,
                     const gchar   *callback_name,
                     const gchar   *object_name,
                     ...)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);
  g_return_val_if_fail (object_name != NULL, FALSE);

  if (gimp->gui.pdb_dialog_set)
    {
      va_list  args;

      va_start (args, object_name);

      retval = gimp->gui.pdb_dialog_set (gimp, container, callback_name,
                                         object_name, args);

      va_end (args);
    }

  return retval;
}

gboolean
gimp_pdb_dialog_close (Gimp          *gimp,
                       GimpContainer *container,
                       const gchar   *callback_name)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);

  if (gimp->gui.pdb_dialog_close)
    return gimp->gui.pdb_dialog_close (gimp, container, callback_name);

  return FALSE;
}

void
gimp_pdb_dialogs_check (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->gui.pdb_dialogs_check)
    gimp->gui.pdb_dialogs_check (gimp);
}
