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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "actions-types.h"

#include "base/tile-manager.h"
#include "base/tile.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimppickable.h"
#include "core/gimpprojectable.h"
#include "core/gimpprojection.h"

#include "file/file-utils.h"

#include "gegl/gimp-gegl-utils.h"

#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"

#include "menus/menus.h"

#include "actions.h"
#include "debug-commands.h"


#ifdef ENABLE_DEBUG_MENU

/*  local function prototypes  */

static gboolean  debug_benchmark_projection    (GimpImage  *image);
static gboolean  debug_show_image_graph        (GimpImage  *source_image);

static void      debug_dump_menus_recurse_menu (GtkWidget  *menu,
                                                gint        depth,
                                                gchar      *path);

static void      debug_print_qdata             (GimpObject *object);
static void      debug_print_qdata_foreach     (GQuark      key_id,
                                                gpointer    data,
                                                gpointer    user_data);


/*  public functions  */

void
debug_mem_profile_cmd_callback (GtkAction *action,
                                gpointer   data)
{
  extern gboolean  gimp_debug_memsize;
  Gimp            *gimp;
  return_if_no_gimp (gimp, data);

  gimp_debug_memsize = TRUE;

  gimp_object_get_memsize (GIMP_OBJECT (gimp), NULL);

  gimp_debug_memsize = FALSE;
}

void
debug_benchmark_projection_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpImage *image;
  return_if_no_image (image, data);

  g_idle_add ((GSourceFunc) debug_benchmark_projection, g_object_ref (image));
}

void
debug_show_image_graph_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpImage *source_image = NULL;
  return_if_no_image (source_image, data);

  g_idle_add ((GSourceFunc) debug_show_image_graph, g_object_ref (source_image));
}

void
debug_dump_menus_cmd_callback (GtkAction *action,
                               gpointer   data)
{
  GList *list;

  for (list = global_menu_factory->registered_menus;
       list;
       list = g_list_next (list))
    {
      GimpMenuFactoryEntry *entry = list->data;
      GList                *managers;

      managers = gimp_ui_managers_from_name (entry->identifier);

      if (managers)
        {
          GimpUIManager *manager = managers->data;
          GList         *list;

          for (list = manager->registered_uis; list; list = g_list_next (list))
            {
              GimpUIManagerUIEntry *ui_entry = list->data;

              if (GTK_IS_MENU_SHELL (ui_entry->widget))
                {
                  g_print ("\n\n"
                           "========================================\n"
                           "Menu: %s%s\n"
                           "========================================\n\n",
                           entry->identifier, ui_entry->ui_path);

                  debug_dump_menus_recurse_menu (ui_entry->widget, 1,
                                                 entry->identifier);
                  g_print ("\n");
                }
            }
        }
    }
}

void
debug_dump_managers_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GList *list;

  for (list = global_menu_factory->registered_menus;
       list;
       list = g_list_next (list))
    {
      GimpMenuFactoryEntry *entry = list->data;
      GList                *managers;

      managers = gimp_ui_managers_from_name (entry->identifier);

      if (managers)
        {
          g_print ("\n\n"
		   "========================================\n"
                   "UI Manager: %s\n"
                   "========================================\n\n",
                   entry->identifier);

          g_print ("%s\n", gtk_ui_manager_get_ui (managers->data));
        }
    }
}

void
debug_dump_attached_data_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  Gimp        *gimp         = action_data_get_gimp (data);
  GimpContext *user_context = gimp_get_user_context (gimp);

  debug_print_qdata (GIMP_OBJECT (gimp));
  debug_print_qdata (GIMP_OBJECT (user_context));
}


/*  private functions  */

static gboolean
debug_benchmark_projection (GimpImage *image)
{
  GimpProjection *projection = gimp_image_get_projection (image);
  TileManager    *tiles;
  GTimer         *timer;
  gint            x, y;

  gimp_image_update (image,
                     0, 0,
                     gimp_image_get_width  (image),
                     gimp_image_get_height (image));
  gimp_projection_flush_now (projection);

  tiles = gimp_pickable_get_tiles (GIMP_PICKABLE (projection));

  timer = g_timer_new ();

  for (x = 0; x < tile_manager_width (tiles); x += TILE_WIDTH)
    {
      for (y = 0; y < tile_manager_height (tiles); y += TILE_HEIGHT)
        {
          Tile *tile = tile_manager_get_tile (tiles, x, y, TRUE, FALSE);

          tile_release (tile, FALSE);
        }
    }

  g_print ("Validation of the entire projection took %.0f ms\n",
           1000 * g_timer_elapsed (timer, NULL));

  g_timer_destroy (timer);

  g_object_unref (image);

  return FALSE;
}

static gboolean
debug_show_image_graph (GimpImage *source_image)
{
  Gimp            *gimp        = source_image->gimp;
  GimpProjectable *projectable = GIMP_PROJECTABLE (source_image);
  GeglNode        *image_graph = gimp_projectable_get_graph (projectable);
  GeglNode        *output_node = gegl_node_get_output_proxy (image_graph, "output");
  GimpImage       *new_image   = NULL;
  TileManager     *tiles       = NULL;
  GimpLayer       *layer       = NULL;
  GeglNode        *introspect  = NULL;
  GeglNode        *sink        = NULL;
  GeglBuffer      *buffer      = NULL;
  gchar           *new_name    = NULL;

  /* Setup and process the introspection graph */
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

  /* Create a new image of the result */
  tiles = gimp_buffer_to_tiles (buffer);
  new_name = g_strdup_printf ("%s GEGL graph",
                              file_utils_uri_display_name (gimp_image_get_uri (source_image)));
  new_image = gimp_create_image (gimp,
                                 tile_manager_width (tiles),
                                 tile_manager_height (tiles),
                                 GIMP_RGB,
                                 FALSE);
  gimp_object_set_name (GIMP_OBJECT (new_image),
                        new_name);
  layer = gimp_layer_new_from_tiles (tiles,
                                     new_image,
                                     GIMP_RGBA_IMAGE,
                                     new_name,
                                     1.0,
                                     GIMP_NORMAL_MODE);
  gimp_image_add_layer (new_image, layer, 0, FALSE);
  gimp_create_display (gimp, new_image, GIMP_UNIT_PIXEL, 1.0);

  /* Cleanup */
  g_object_unref (new_image);
  g_free (new_name);
  tile_manager_unref (tiles);
  g_object_unref (buffer);
  g_object_unref (sink);
  g_object_unref (introspect);
  g_object_unref (source_image);

  return FALSE;
}

static void
debug_dump_menus_recurse_menu (GtkWidget *menu,
                               gint       depth,
                               gchar     *path)
{
  GList *list;

  for (list = GTK_MENU_SHELL (menu)->children; list; list = g_list_next (list))
    {
      GtkWidget *menu_item = GTK_WIDGET (list->data);
      GtkWidget *child     = gtk_bin_get_child (GTK_BIN (menu_item));

      if (GTK_IS_LABEL (child))
        {
          const gchar *label;
          gchar       *full_path;
          gchar       *help_page;
          gchar       *format_str;

          label = gtk_label_get_text (GTK_LABEL (child));
          full_path = g_strconcat (path, "/", label, NULL);

          help_page = g_object_get_data (G_OBJECT (menu_item), "gimp-help-id");
          help_page = g_strdup (help_page);

          format_str = g_strdup_printf ("%%%ds%%%ds %%-20s %%s\n",
                                        depth * 2, depth * 2 - 40);
          g_print (format_str,
                   "", label, "", help_page ? help_page : "");
          g_free (format_str);
          g_free (help_page);

          if (GTK_MENU_ITEM (menu_item)->submenu)
            debug_dump_menus_recurse_menu (GTK_MENU_ITEM (menu_item)->submenu,
                                           depth + 1, full_path);

          g_free (full_path);
        }
    }
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

#endif /* ENABLE_DEBUG_MENU */
