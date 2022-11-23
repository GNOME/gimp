/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-gui.h"
#include "ligmacontainer.h"
#include "ligmacontext.h"
#include "ligmadisplay.h"
#include "ligmaimage.h"
#include "ligmaprogress.h"
#include "ligmawaitable.h"

#include "about.h"

#include "ligma-intl.h"


void
ligma_gui_init (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma->gui.ungrab                 = NULL;
  ligma->gui.set_busy               = NULL;
  ligma->gui.unset_busy             = NULL;
  ligma->gui.show_message           = NULL;
  ligma->gui.help                   = NULL;
  ligma->gui.get_program_class      = NULL;
  ligma->gui.get_display_name       = NULL;
  ligma->gui.get_user_time          = NULL;
  ligma->gui.get_theme_dir          = NULL;
  ligma->gui.get_icon_theme_dir     = NULL;
  ligma->gui.display_get_window_id  = NULL;
  ligma->gui.display_create         = NULL;
  ligma->gui.display_delete         = NULL;
  ligma->gui.displays_reconnect     = NULL;
  ligma->gui.progress_new           = NULL;
  ligma->gui.progress_free          = NULL;
  ligma->gui.pdb_dialog_set         = NULL;
  ligma->gui.pdb_dialog_close       = NULL;
  ligma->gui.recent_list_add_file   = NULL;
  ligma->gui.recent_list_load       = NULL;
  ligma->gui.get_mount_operation    = NULL;
  ligma->gui.query_profile_policy   = NULL;
  ligma->gui.query_rotation_policy  = NULL;
}

void
ligma_gui_ungrab (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (ligma->gui.ungrab)
    ligma->gui.ungrab (ligma);
}

void
ligma_set_busy (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /* FIXME: ligma_busy HACK */
  ligma->busy++;

  if (ligma->busy == 1)
    {
      if (ligma->gui.set_busy)
        ligma->gui.set_busy (ligma);
    }
}

static gboolean
ligma_idle_unset_busy (gpointer data)
{
  Ligma *ligma = data;

  ligma_unset_busy (ligma);

  ligma->busy_idle_id = 0;

  return FALSE;
}

void
ligma_set_busy_until_idle (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (! ligma->busy_idle_id)
    {
      ligma_set_busy (ligma);

      ligma->busy_idle_id = g_idle_add_full (G_PRIORITY_HIGH,
                                            ligma_idle_unset_busy, ligma,
                                            NULL);
    }
}

void
ligma_unset_busy (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (ligma->busy > 0);

  /* FIXME: ligma_busy HACK */
  ligma->busy--;

  if (ligma->busy == 0)
    {
      if (ligma->gui.unset_busy)
        ligma->gui.unset_busy (ligma);
    }
}

void
ligma_show_message (Ligma                *ligma,
                   GObject             *handler,
                   LigmaMessageSeverity  severity,
                   const gchar         *domain,
                   const gchar         *message)
{
  const gchar *desc = (severity == LIGMA_MESSAGE_ERROR) ? "Error" : "Message";

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (handler == NULL || G_IS_OBJECT (handler));
  g_return_if_fail (message != NULL);

  if (! domain)
    domain = LIGMA_ACRONYM;

  if (! ligma->console_messages)
    {
      if (ligma->gui.show_message)
        {
          ligma->gui.show_message (ligma, handler, severity,
                                  domain, message);
          return;
        }
      else if (LIGMA_IS_PROGRESS (handler) &&
               ligma_progress_message (LIGMA_PROGRESS (handler), ligma,
                                      severity, domain, message))
        {
          /* message has been handled by LigmaProgress */
          return;
        }
    }

  ligma_enum_get_value (LIGMA_TYPE_MESSAGE_SEVERITY, severity,
                       NULL, NULL, &desc, NULL);
  g_printerr ("%s-%s: %s\n\n", domain, desc, message);
}

void
ligma_wait (Ligma         *ligma,
           LigmaWaitable *waitable,
           const gchar  *format,
           ...)
{
  va_list  args;
  gchar   *message;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_WAITABLE (waitable));
  g_return_if_fail (format != NULL);

  if (ligma_waitable_wait_for (waitable, 0.5 * G_TIME_SPAN_SECOND))
    return;

  va_start (args, format);

  message = g_strdup_vprintf (format, args);

  va_end (args);

  if (! ligma->console_messages &&
      ligma->gui.wait           &&
      ligma->gui.wait (ligma, waitable, message))
    {
      return;
    }

  /* Translator:  This message is displayed while LIGMA is waiting for
   * some operation to finish.  The %s argument is a message describing
   * the operation.
   */
  g_printerr (_("Please wait: %s\n"), message);

  ligma_waitable_wait (waitable);

  g_free (message);
}

void
ligma_help (Ligma         *ligma,
           LigmaProgress *progress,
           const gchar  *help_domain,
           const gchar  *help_id)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  if (ligma->gui.help)
    ligma->gui.help (ligma, progress, help_domain, help_id);
}

const gchar *
ligma_get_program_class (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma->gui.get_program_class)
    return ligma->gui.get_program_class (ligma);

  return NULL;
}

gchar *
ligma_get_display_name (Ligma     *ligma,
                       gint      display_id,
                       GObject **monitor,
                       gint     *monitor_number)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (monitor != NULL, NULL);
  g_return_val_if_fail (monitor_number != NULL, NULL);

  if (ligma->gui.get_display_name)
    return ligma->gui.get_display_name (ligma, display_id,
                                       monitor, monitor_number);

  *monitor = NULL;

  return NULL;
}

/**
 * ligma_get_user_time:
 * @ligma:
 *
 * Returns the timestamp of the last user interaction. The timestamp is
 * taken from events caused by user interaction such as key presses or
 * pointer movements. See gdk_x11_display_get_user_time().
 *
 * Returns: the timestamp of the last user interaction
 */
guint32
ligma_get_user_time (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), 0);

  if (ligma->gui.get_user_time)
    return ligma->gui.get_user_time (ligma);

  return 0;
}

GFile *
ligma_get_theme_dir (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma->gui.get_theme_dir)
    return ligma->gui.get_theme_dir (ligma);

  return NULL;
}

GFile *
ligma_get_icon_theme_dir (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma->gui.get_icon_theme_dir)
    return ligma->gui.get_icon_theme_dir (ligma);

  return NULL;
}

LigmaObject *
ligma_get_window_strategy (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma->gui.get_window_strategy)
    return ligma->gui.get_window_strategy (ligma);

  return NULL;
}

LigmaDisplay *
ligma_get_empty_display (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma->gui.get_empty_display)
    return ligma->gui.get_empty_display (ligma);

  return NULL;
}

guint32
ligma_get_display_window_id (Ligma        *ligma,
                            LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), -1);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), -1);

  if (ligma->gui.display_get_window_id)
    return ligma->gui.display_get_window_id (display);

  return -1;
}

LigmaDisplay *
ligma_create_display (Ligma      *ligma,
                     LigmaImage *image,
                     LigmaUnit   unit,
                     gdouble    scale,
                     GObject   *monitor)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (monitor == NULL || G_IS_OBJECT (monitor), NULL);

  if (ligma->gui.display_create)
    return ligma->gui.display_create (ligma, image, unit, scale, monitor);

  return NULL;
}

void
ligma_delete_display (Ligma        *ligma,
                     LigmaDisplay *display)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  if (ligma->gui.display_delete)
    ligma->gui.display_delete (display);
}

void
ligma_reconnect_displays (Ligma      *ligma,
                         LigmaImage *old_image,
                         LigmaImage *new_image)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_IMAGE (old_image));
  g_return_if_fail (LIGMA_IS_IMAGE (new_image));

  if (ligma->gui.displays_reconnect)
    ligma->gui.displays_reconnect (ligma, old_image, new_image);
}

LigmaProgress *
ligma_new_progress (Ligma        *ligma,
                   LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (display == NULL || LIGMA_IS_DISPLAY (display), NULL);

  if (ligma->gui.progress_new)
    return ligma->gui.progress_new (ligma, display);

  return NULL;
}

void
ligma_free_progress (Ligma         *ligma,
                    LigmaProgress *progress)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_PROGRESS (progress));

  if (ligma->gui.progress_free)
    ligma->gui.progress_free (ligma, progress);
}

gboolean
ligma_pdb_dialog_new (Ligma          *ligma,
                     LigmaContext   *context,
                     LigmaProgress  *progress,
                     LigmaContainer *container,
                     const gchar   *title,
                     const gchar   *callback_name,
                     const gchar   *object_name,
                     ...)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (title != NULL, FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);

  if (ligma->gui.pdb_dialog_new)
    {
      va_list args;

      va_start (args, object_name);

      retval = ligma->gui.pdb_dialog_new (ligma, context, progress,
                                         container, title,
                                         callback_name, object_name,
                                         args);

      va_end (args);
    }

  return retval;
}

gboolean
ligma_pdb_dialog_set (Ligma          *ligma,
                     LigmaContainer *container,
                     const gchar   *callback_name,
                     const gchar   *object_name,
                     ...)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);
  g_return_val_if_fail (object_name != NULL, FALSE);

  if (ligma->gui.pdb_dialog_set)
    {
      va_list args;

      va_start (args, object_name);

      retval = ligma->gui.pdb_dialog_set (ligma, container, callback_name,
                                         object_name, args);

      va_end (args);
    }

  return retval;
}

gboolean
ligma_pdb_dialog_close (Ligma          *ligma,
                       LigmaContainer *container,
                       const gchar   *callback_name)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);
  g_return_val_if_fail (callback_name != NULL, FALSE);

  if (ligma->gui.pdb_dialog_close)
    return ligma->gui.pdb_dialog_close (ligma, container, callback_name);

  return FALSE;
}

gboolean
ligma_recent_list_add_file (Ligma        *ligma,
                           GFile       *file,
                           const gchar *mime_type)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  if (ligma->gui.recent_list_add_file)
    return ligma->gui.recent_list_add_file (ligma, file, mime_type);

  return FALSE;
}

void
ligma_recent_list_load (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (ligma->gui.recent_list_load)
    ligma->gui.recent_list_load (ligma);
}

GMountOperation *
ligma_get_mount_operation (Ligma         *ligma,
                          LigmaProgress *progress)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);

  if (ligma->gui.get_mount_operation)
    return ligma->gui.get_mount_operation (ligma, progress);

  return g_mount_operation_new ();
}

LigmaColorProfilePolicy
ligma_query_profile_policy (Ligma                      *ligma,
                           LigmaImage                 *image,
                           LigmaContext               *context,
                           LigmaColorProfile         **dest_profile,
                           LigmaColorRenderingIntent  *intent,
                           gboolean                  *bpc,
                           gboolean                  *dont_ask)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), LIGMA_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), LIGMA_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), LIGMA_COLOR_PROFILE_POLICY_KEEP);
  g_return_val_if_fail (dest_profile != NULL, LIGMA_COLOR_PROFILE_POLICY_KEEP);

  if (ligma->gui.query_profile_policy)
    return ligma->gui.query_profile_policy (ligma, image, context,
                                           dest_profile,
                                           intent, bpc,
                                           dont_ask);

  return LIGMA_COLOR_PROFILE_POLICY_KEEP;
}

LigmaMetadataRotationPolicy
ligma_query_rotation_policy (Ligma        *ligma,
                            LigmaImage   *image,
                            LigmaContext *context,
                            gboolean    *dont_ask)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), LIGMA_METADATA_ROTATION_POLICY_ROTATE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), LIGMA_METADATA_ROTATION_POLICY_ROTATE);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), LIGMA_METADATA_ROTATION_POLICY_ROTATE);

  if (ligma->gui.query_rotation_policy)
    return ligma->gui.query_rotation_policy (ligma, image, context, dont_ask);

  return LIGMA_METADATA_ROTATION_POLICY_ROTATE;
}
