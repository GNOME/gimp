/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpviewable.h"

#include "file/file-utils.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "file-actions.h"
#include "file-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   file_actions_last_opened_update  (GimpContainer   *container,
                                                GimpImagefile   *unused,
                                                GimpActionGroup *group);
static void   file_actions_last_opened_reorder (GimpContainer   *container,
                                                GimpImagefile   *unused1,
                                                gint             unused2,
                                                GimpActionGroup *group);
static void   file_actions_close_all_update    (GimpContainer   *container,
                                                GimpObject      *unused,
                                                GimpActionGroup *group);


static const GimpActionEntry file_actions[] =
{
  { "file-menu",             NULL, N_("_File")        },
  { "file-open-recent-menu", NULL, N_("Open _Recent") },
  { "file-acquire-menu",     NULL, N_("Acq_uire")     },

  { "file-open", GTK_STOCK_OPEN,
    N_("_Open..."), NULL,
    N_("Open an image file"),
    G_CALLBACK (file_open_cmd_callback),
    GIMP_HELP_FILE_OPEN },

  { "file-open-as-layers", GIMP_STOCK_LAYER,
    N_("Op_en as Layers..."), "<control><alt>O",
    N_("Open an image file as layers"),
    G_CALLBACK (file_open_as_layers_cmd_callback),
    GIMP_HELP_FILE_OPEN_AS_LAYER },

  { "file-open-location", GIMP_STOCK_WEB,
    N_("Open _Location..."), NULL,
    N_("Open an image file from a specified location"),
    G_CALLBACK (file_open_location_cmd_callback),
    GIMP_HELP_FILE_OPEN_LOCATION },

  { "file-save-as-template", NULL,
    N_("Save as _Template..."), NULL,
    N_("Create a new template from this image"),
    G_CALLBACK (file_save_template_cmd_callback),
    GIMP_HELP_FILE_SAVE_AS_TEMPLATE },

  { "file-revert", GTK_STOCK_REVERT_TO_SAVED,
    N_("Re_vert"), NULL,
    N_("Reload the image file from disk"),
    G_CALLBACK (file_revert_cmd_callback),
    GIMP_HELP_FILE_REVERT },

  { "file-close-all", GTK_STOCK_CLOSE,
    N_("Close all"), "<shift><control>W",
    N_("Close all opened images"),
    G_CALLBACK (file_close_all_cmd_callback),
    GIMP_HELP_FILE_CLOSE_ALL },

  { "file-quit", GTK_STOCK_QUIT,
    N_("_Quit"), "<control>Q",
    N_("Quit the GNU Image Manipulation Program"),
    G_CALLBACK (file_quit_cmd_callback),
    GIMP_HELP_FILE_QUIT }
};

static const GimpEnumActionEntry file_save_actions[] =
{
  { "file-save", GTK_STOCK_SAVE,
    N_("_Save"), "<control>S",
    N_("Save this image"),
    GIMP_SAVE_MODE_SAVE, FALSE,
    GIMP_HELP_FILE_SAVE },

  { "file-save-as", GTK_STOCK_SAVE_AS,
    N_("Save _As..."), "<control><shift>S",
    N_("Save this image with a different name"),
    GIMP_SAVE_MODE_SAVE_AS, FALSE,
    GIMP_HELP_FILE_SAVE_AS },

  { "file-save-a-copy", NULL,
    N_("Save a Cop_y..."), NULL,
    N_("Save this image with a different name, but keep its current name"),
    GIMP_SAVE_MODE_SAVE_A_COPY, FALSE,
    GIMP_HELP_FILE_SAVE_A_COPY },

  { "file-save-and-close", NULL,
    N_("Save and Close..."), NULL,
    N_("Save this image and close its window"),
    GIMP_SAVE_MODE_SAVE_AND_CLOSE, FALSE,
    GIMP_HELP_FILE_SAVE }
};

void
file_actions_setup (GimpActionGroup *group)
{
  GimpEnumActionEntry *entries;
  gint                 n_entries;
  gint                 i;

  gimp_action_group_add_actions (group,
                                 file_actions,
                                 G_N_ELEMENTS (file_actions));

  gimp_action_group_add_enum_actions (group,
                                      file_save_actions,
                                      G_N_ELEMENTS (file_save_actions),
                                      G_CALLBACK (file_save_cmd_callback));

  n_entries = GIMP_GUI_CONFIG (group->gimp->config)->last_opened_size;

  entries = g_new0 (GimpEnumActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      entries[i].name           = g_strdup_printf ("file-open-recent-%02d",
                                                   i + 1);
      entries[i].stock_id       = GTK_STOCK_OPEN;
      entries[i].label          = entries[i].name;
      entries[i].tooltip        = NULL;
      entries[i].value          = i;
      entries[i].value_variable = FALSE;
      entries[i].help_id        = GIMP_HELP_FILE_OPEN_RECENT;

      if (i < 9)
        entries[i].accelerator = g_strdup_printf ("<control>%d", i + 1);
      else if (i == 9)
        entries[i].accelerator = "<control>0";
      else
        entries[i].accelerator = "";
    }

  gimp_action_group_add_enum_actions (group, entries, n_entries,
                                      G_CALLBACK (file_last_opened_cmd_callback));

  for (i = 0; i < n_entries; i++)
    {
      GtkAction *action;

      gimp_action_group_set_action_visible (group, entries[i].name, FALSE);

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                            entries[i].name);
      g_object_set (action,
                    "context", gimp_get_user_context (group->gimp),
                    NULL);

      g_free ((gchar *) entries[i].name);
      if (i < 9)
        g_free ((gchar *) entries[i].accelerator);
    }

  g_free (entries);

  g_signal_connect_object (group->gimp->documents, "add",
                           G_CALLBACK (file_actions_last_opened_update),
                           group, 0);
  g_signal_connect_object (group->gimp->documents, "remove",
                           G_CALLBACK (file_actions_last_opened_update),
                           group, 0);
  g_signal_connect_object (group->gimp->documents, "reorder",
                           G_CALLBACK (file_actions_last_opened_reorder),
                           group, 0);

  file_actions_last_opened_update (group->gimp->documents, NULL, group);

  g_signal_connect_object (group->gimp->displays, "add",
                           G_CALLBACK (file_actions_close_all_update),
                           group, 0);
  g_signal_connect_object (group->gimp->displays, "remove",
                           G_CALLBACK (file_actions_close_all_update),
                           group, 0);

  file_actions_close_all_update (group->gimp->displays, NULL, group);
}

void
file_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  GimpImage    *image    = action_data_get_image (data);
  GimpDrawable *drawable = NULL;

  if (image)
    drawable = gimp_image_get_active_drawable (image);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)

  SET_SENSITIVE ("file-open-as-layers",   image);
  SET_SENSITIVE ("file-save",             image && drawable);
  SET_SENSITIVE ("file-save-as",          image && drawable);
  SET_SENSITIVE ("file-save-a-copy",      image && drawable);
  SET_SENSITIVE ("file-save-as-template", image);
  SET_SENSITIVE ("file-revert",           image && GIMP_OBJECT (image)->name);

#undef SET_SENSITIVE
}


/*  private functions  */

static void
file_actions_last_opened_update (GimpContainer   *container,
                                 GimpImagefile   *unused,
                                 GimpActionGroup *group)
{
  gint num_documents;
  gint i;
  gint n = GIMP_GUI_CONFIG (group->gimp->config)->last_opened_size;

  num_documents = gimp_container_num_children (container);

  for (i = 0; i < n; i++)
    {
      GtkAction *action;
      gchar     *name = g_strdup_printf ("file-open-recent-%02d", i + 1);

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), name);

      if (i < num_documents)
        {
          GimpImagefile *imagefile = (GimpImagefile *)
            gimp_container_get_child_by_index (container, i);

          if (GIMP_ACTION (action)->viewable != (GimpViewable *) imagefile)
            {
              const gchar *uri;
              gchar       *filename;
              gchar       *basename;
              gchar       *escaped;

              uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

              filename = file_utils_uri_display_name (uri);
              basename = file_utils_uri_display_basename (uri);

              escaped = gimp_escape_uline (basename);

              g_free (basename);

              g_object_set (action,
                            "label",    escaped,
                            "tooltip",  filename,
                            "visible",  TRUE,
                            "viewable", imagefile,
                            NULL);

              g_free (filename);
              g_free (escaped);
            }
        }
      else
        {
          g_object_set (action,
                        "label",    name,
                        "tooltip",  NULL,
                        "visible",  FALSE,
                        "viewable", NULL,
                        NULL);
        }

      g_free (name);
    }
}

static void
file_actions_last_opened_reorder (GimpContainer   *container,
                                  GimpImagefile   *unused1,
                                  gint             unused2,
                                  GimpActionGroup *group)
{
  file_actions_last_opened_update (container, unused1, group);
}

static void
file_actions_close_all_update (GimpContainer   *container,
                               GimpObject      *unused,
                               GimpActionGroup *group)
{
  gint n_displays = gimp_container_num_children (container);

  gimp_action_group_set_action_sensitive (group, "file-close-all",
                                          n_displays > 0);
}
