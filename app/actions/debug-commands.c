/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpprojectable.h"
#include "core/gimpprojection.h"

#include "gegl/gimp-gegl-utils.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpimagewindow.h"

#include "menus/menus.h"

#include "actions.h"
#include "debug-commands.h"


/*  local function prototypes  */

static gboolean  debug_benchmark_projection    (GimpDisplay *display);
static gboolean  debug_show_image_graph        (GimpImage   *source_image);

static void      debug_print_qdata             (GimpObject  *object);
static void      debug_print_qdata_foreach     (GQuark       key_id,
                                                gpointer     data,
                                                gpointer     user_data);


/*  public functions  */

void
debug_gtk_inspector_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  gtk_window_set_interactive_debugging (TRUE);
}

void
debug_mem_profile_cmd_callback (GimpAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  extern gboolean  gimp_debug_memsize;
  Gimp            *gimp;
  return_if_no_gimp (gimp, data);

  gimp_debug_memsize = TRUE;

  gimp_object_get_memsize (GIMP_OBJECT (gimp), NULL);

  gimp_debug_memsize = FALSE;
}

void
debug_benchmark_projection_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpDisplay *display;
  return_if_no_display (display, data);

  g_idle_add ((GSourceFunc) debug_benchmark_projection, g_object_ref (display));
}

void
debug_show_image_graph_cmd_callback (GimpAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  GimpImage *source_image = NULL;
  return_if_no_image (source_image, data);

  g_idle_add ((GSourceFunc) debug_show_image_graph, g_object_ref (source_image));
}

void
debug_dump_keyboard_shortcuts_cmd_callback (GimpAction *action,
                                            GVariant   *value,
                                            gpointer    data)
{
  GimpDisplay   *display;
  GimpUIManager *manager;
  GList         *group_it;
  GList         *strings = NULL;

  return_if_no_display (display, data);

  manager = menus_get_image_manager_singleton (display->gimp);

  /* Gather formatted strings of keyboard shortcuts */
  for (group_it = gimp_ui_manager_get_action_groups (manager);
       group_it;
       group_it = g_list_next (group_it))
    {
      GimpActionGroup *group     = group_it->data;
      GList           *actions   = NULL;
      GList           *action_it = NULL;

      actions = gimp_action_group_list_actions (group);
      actions = g_list_sort (actions, (GCompareFunc) gimp_action_name_compare);

      for (action_it = actions; action_it; action_it = g_list_next (action_it))
        {
          gchar       **accels;
          GimpAction   *action = action_it->data;
          const gchar  *name   = gimp_action_get_name (action);

          if (name[0] == '<')
            continue;

          accels = gimp_action_get_display_accels (action);

          if (accels && accels[0])
            {
              const gchar *label_tmp;
              gchar       *label;

              label_tmp  = gimp_action_get_label (action);
              label      = gimp_strip_uline (label_tmp);

              strings = g_list_prepend (strings,
                                        g_strdup_printf ("%-20s %s",
                                                         accels[0], label));

              g_free (label);

              for (gint i = 1; accels[i] != NULL; i++)
                strings = g_list_prepend (strings, g_strdup (accels[i]));
            }

          g_strfreev (accels);
        }

      g_list_free (actions);
    }

  /* Sort and prints the strings */
  {
    GList *string_it = NULL;

    strings = g_list_sort (strings, (GCompareFunc) strcmp);

    for (string_it = strings; string_it; string_it = g_list_next (string_it))
      {
        g_print ("%s\n", (gchar *) string_it->data);
        g_free (string_it->data);
      }

    g_list_free (strings);
  }
}

void
debug_dump_attached_data_cmd_callback (GimpAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  Gimp        *gimp         = action_data_get_gimp (data);
  GimpContext *user_context = gimp_get_user_context (gimp);

  debug_print_qdata (GIMP_OBJECT (gimp));
  debug_print_qdata (GIMP_OBJECT (user_context));
}


/*  private functions  */

static gboolean
debug_benchmark_projection (GimpDisplay *display)
{
  GimpImage *image = gimp_display_get_image (display);

  if (image)
    {
      GimpProjection *projection = gimp_image_get_projection (image);

      gimp_projection_stop_rendering (projection);

      GIMP_TIMER_START ();

      gimp_image_invalidate_all (image);
      gimp_projection_flush_now (projection, TRUE);

      GIMP_TIMER_END ("Validation of the entire projection");

      g_object_unref (display);
    }

  return FALSE;
}

static gboolean
debug_show_image_graph (GimpImage *source_image)
{
  GeglNode   *image_graph;
  GeglNode   *output_node;
  GimpImage  *new_image;
  GeglNode   *introspect;
  GeglNode   *sink;
  GeglBuffer *buffer = NULL;

  image_graph = gimp_projectable_get_graph (GIMP_PROJECTABLE (source_image));

  output_node = gegl_node_get_output_proxy (image_graph, "output");

  introspect = gegl_node_new_child (NULL,
                                    "operation", "gegl:introspect",
                                    "node",      output_node,
                                    NULL);
  sink = gegl_node_new_child (NULL,
                              "operation", "gegl:buffer-sink",
                              "buffer",    &buffer,
                              NULL);

  gegl_node_link_many (introspect, sink, NULL);
  gegl_node_process (sink);

  if (buffer)
    {
      gchar *new_name;

      /* This should not happen but "gegl:introspect" is a bit fickle as
       * it uses an external binary `dot`. Prevent useless crashes.
       * I don't output a warning when buffer is NULL because anyway
       * GEGL will output one itself.
       */
      new_name = g_strdup_printf ("%s GEGL graph",
                                  gimp_image_get_display_name (source_image));

      new_image = gimp_create_image_from_buffer (source_image->gimp,
                                                 buffer, new_name);
      gimp_image_set_file (new_image, g_file_new_for_uri (new_name));

      g_free (new_name);
      g_object_unref (buffer);
    }

  g_object_unref (sink);
  g_object_unref (introspect);

  g_object_unref (source_image);

  return FALSE;
}

static void
debug_print_qdata (GimpObject *object)
{
  g_print ("\nData attached to '%s':\n\n", gimp_object_get_name (object));
  g_datalist_foreach (&G_OBJECT (object)->qdata,
                      debug_print_qdata_foreach,
                      NULL);
  g_print ("\n");
}

static void
debug_print_qdata_foreach (GQuark   key_id,
                           gpointer data,
                           gpointer user_data)
{
  g_print ("%s: %p\n", g_quark_to_string (key_id), data);
}
