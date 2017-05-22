/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-dialog-export.c
 * Copyright (C) 2017 Jehan <jehan@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#undef GDK_DISABLE_DEPRECATED
#include <libgimp/gimpui.h>
#include "libgimp/stdplugins-intl.h"

#include "core/animation.h"
#include "core/animation-playback.h"

#include "animation-dialog.h"
#include "animation-dialog-export.h"

static void animation_dialog_export_video (AnimationPlayback *playback,
                                           gchar             *filename);

void
animation_dialog_export (GtkWindow         *main_dialog,
                         AnimationPlayback *playback)
{
  GtkWidget     *dialog;
  GtkFileFilter *all;
  GtkFileFilter *videos;
  gchar         *filename = NULL;

  dialog = gtk_file_chooser_dialog_new (_("Export animation"),
                                        main_dialog,
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Export"), GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  /*gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                         default_folder_for_saving);*/
  /*gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                       "Untitled document");*/

  /* Add filters. */
  all = gtk_file_filter_new ();
  gtk_file_filter_set_name (all, _("All files"));
  gtk_file_filter_add_pattern (all, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all);

  videos = gtk_file_filter_new ();
  gtk_file_filter_set_name (videos, _("Video Files"));
  gtk_file_filter_add_pattern (videos, "*.[oO][gG][vV]");
  gtk_file_filter_add_mime_type (videos, "video/x-theora+ogg");
  gtk_file_filter_add_mime_type (videos, "video/ogg");
  gtk_file_filter_add_pattern (videos, "*.[aA][vV][iI]");
  gtk_file_filter_add_pattern (videos, "*.[mM][oO][vV]");
  gtk_file_filter_add_pattern (videos, "*.[mM][pP][gG]");
  gtk_file_filter_add_pattern (videos, "*.[mM][pP]4");
  gtk_file_filter_add_mime_type (videos, "video/x-msvideo");
  gtk_file_filter_add_mime_type (videos, "video/quicktime");
  gtk_file_filter_add_mime_type (videos, "video/mpeg");
  gtk_file_filter_add_mime_type (videos, "video/mp4");
  gtk_file_filter_add_mime_type (videos, "video/x-matroska");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), videos);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), videos);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    }
  gtk_widget_destroy (dialog);
  if (filename)
    {
      animation_dialog_export_video (playback, filename);
      g_free (filename);
    }
}

static void
animation_dialog_export_video (AnimationPlayback *playback,
                               gchar             *filename)
{
  Animation *animation;
  GeglNode  *graph;
  GeglNode  *export;
  GeglNode  *input;
  gint       duration;
  gint       i;

  animation = animation_playback_get_animation (playback);
  duration = animation_get_duration (animation);
  graph  = gegl_node_new ();
  export = gegl_node_new_child (graph,
                                "operation", "gegl:ff-save",
                                "path", filename,
                                NULL);
  input = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-source",
                               NULL);
  gegl_node_set (export, "frame-rate", 24.0, NULL);
  gegl_node_set (export, "video-bufsize", 0, NULL);
  gegl_node_set (export, "video-bit-rate", 0, NULL);
  gegl_node_link_many (input, export, NULL);

  for (i = 0; i < duration; i++)
    {
      GeglBuffer *buffer;

      g_signal_emit_by_name (animation, "loading",
                             (gdouble) i / ((gdouble) duration - 0.999));
      buffer = animation_playback_get_buffer (playback, i);
      gegl_node_set (input, "buffer", buffer, NULL);
      gegl_node_process (export);
      g_object_unref (buffer);
    }
  g_object_unref (graph);
  g_signal_emit_by_name (animation, "loaded");
}
