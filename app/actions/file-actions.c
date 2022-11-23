/* LIGMA - The GNU Image Manipulation Program
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimagefile.h"
#include "core/ligmaviewable.h"

#include "file/ligma-file.h"

#include "plug-in/ligmapluginmanager-file.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmaactionimpl.h"
#include "widgets/ligmahelp-ids.h"

#include "display/ligmadisplay.h"

#include "actions.h"
#include "file-actions.h"
#include "file-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void    file_actions_last_opened_update  (LigmaContainer   *container,
                                                 LigmaImagefile   *unused,
                                                 LigmaActionGroup *group);
static void    file_actions_last_opened_reorder (LigmaContainer   *container,
                                                 LigmaImagefile   *unused1,
                                                 gint             unused2,
                                                 LigmaActionGroup *group);
static void    file_actions_close_all_update    (LigmaContainer   *images,
                                                 LigmaObject      *unused,
                                                 LigmaActionGroup *group);
static gchar * file_actions_create_label        (const gchar     *format,
                                                 GFile           *file);


static const LigmaActionEntry file_actions[] =
{
  { "file-menu",             NULL, NC_("file-action", "_File")        },
  { "file-create-menu",      NULL, NC_("file-action", "Crea_te")      },
  { "file-open-recent-menu", NULL, NC_("file-action", "Open _Recent") },

  { "file-open", LIGMA_ICON_IMAGE_OPEN,
    NC_("file-action", "_Open..."), "<primary>O",
    NC_("file-action", "Open an image file"),
    file_open_cmd_callback,
    LIGMA_HELP_FILE_OPEN },

  { "file-open-as-layers", LIGMA_ICON_LAYER,
    NC_("file-action", "Op_en as Layers..."), "<primary><alt>O",
    NC_("file-action", "Open an image file as layers"),
    file_open_as_layers_cmd_callback,
    LIGMA_HELP_FILE_OPEN_AS_LAYER },

  { "file-open-location", LIGMA_ICON_WEB,
    NC_("file-action", "Open _Location..."), NULL,
    NC_("file-action", "Open an image file from a specified location"),
    file_open_location_cmd_callback,
    LIGMA_HELP_FILE_OPEN_LOCATION },

  { "file-create-template", NULL,
    NC_("file-action", "Create _Template..."), NULL,
    NC_("file-action", "Create a new template from this image"),
    file_create_template_cmd_callback,
    LIGMA_HELP_FILE_CREATE_TEMPLATE },

  { "file-revert", LIGMA_ICON_IMAGE_RELOAD,
    NC_("file-action", "Re_vert"), NULL,
    NC_("file-action", "Reload the image file from disk"),
    file_revert_cmd_callback,
    LIGMA_HELP_FILE_REVERT },

  { "file-close-all", LIGMA_ICON_CLOSE_ALL,
    NC_("file-action", "C_lose All"), "<primary><shift>W",
    NC_("file-action", "Close all opened images"),
    file_close_all_cmd_callback,
    LIGMA_HELP_FILE_CLOSE_ALL },

  { "file-copy-location", LIGMA_ICON_EDIT_COPY,
    NC_("file-action", "Copy _Image Location"), NULL,
    NC_("file-action", "Copy image file location to clipboard"),
    file_copy_location_cmd_callback,
    LIGMA_HELP_FILE_COPY_LOCATION },

  { "file-show-in-file-manager", LIGMA_ICON_FILE_MANAGER,
    NC_("file-action", "Show in _File Manager"), "<primary><alt>F",
    NC_("file-action", "Show image file location in the file manager"),
    file_show_in_file_manager_cmd_callback,
    LIGMA_HELP_FILE_SHOW_IN_FILE_MANAGER },

  { "file-quit", LIGMA_ICON_APPLICATION_EXIT,
    NC_("file-action", "_Quit"), "<primary>Q",
    NC_("file-action", "Quit the GNU Image Manipulation Program"),
    file_quit_cmd_callback,
    LIGMA_HELP_FILE_QUIT }
};

static const LigmaEnumActionEntry file_save_actions[] =
{
  { "file-save", LIGMA_ICON_DOCUMENT_SAVE,
    NC_("file-action", "_Save"), "<primary>S",
    NC_("file-action", "Save this image"),
    LIGMA_SAVE_MODE_SAVE, FALSE,
    LIGMA_HELP_FILE_SAVE },

  { "file-save-as", LIGMA_ICON_DOCUMENT_SAVE_AS,
    NC_("file-action", "Save _As..."), "<primary><shift>S",
    NC_("file-action", "Save this image with a different name"),
    LIGMA_SAVE_MODE_SAVE_AS, FALSE,
    LIGMA_HELP_FILE_SAVE_AS },

  { "file-save-a-copy", NULL,
    NC_("file-action", "Save a Cop_y..."), NULL,
    NC_("file-action",
        "Save a copy of this image, without affecting the source file "
        "(if any) or the current state of the image"),
    LIGMA_SAVE_MODE_SAVE_A_COPY, FALSE,
    LIGMA_HELP_FILE_SAVE_A_COPY },

  { "file-save-and-close", NULL,
    NC_("file-action", "Save and Close..."), NULL,
    NC_("file-action", "Save this image and close its window"),
    LIGMA_SAVE_MODE_SAVE_AND_CLOSE, FALSE,
    LIGMA_HELP_FILE_SAVE },

  { "file-export", NULL,
    NC_("file-action", "E_xport..."), "<primary>E",
    NC_("file-action", "Export the image"),
    LIGMA_SAVE_MODE_EXPORT, FALSE,
    LIGMA_HELP_FILE_EXPORT },

  { "file-overwrite", NULL,
    NC_("file-action", "Over_write"), "",
    NC_("file-action", "Export the image back to the imported file in the import format"),
    LIGMA_SAVE_MODE_OVERWRITE, FALSE,
    LIGMA_HELP_FILE_OVERWRITE },

  { "file-export-as", NULL,
    NC_("file-action", "E_xport As..."), "<primary><shift>E",
    NC_("file-action", "Export the image to various file formats such as PNG or JPEG"),
    LIGMA_SAVE_MODE_EXPORT_AS, FALSE,
    LIGMA_HELP_FILE_EXPORT_AS }
};

void
file_actions_setup (LigmaActionGroup *group)
{
  LigmaEnumActionEntry *entries;
  gint                 n_entries;
  gint                 i;

  ligma_action_group_add_actions (group, "file-action",
                                 file_actions,
                                 G_N_ELEMENTS (file_actions));

  ligma_action_group_add_enum_actions (group, "file-action",
                                      file_save_actions,
                                      G_N_ELEMENTS (file_save_actions),
                                      file_save_cmd_callback);

  n_entries = LIGMA_GUI_CONFIG (group->ligma->config)->last_opened_size;

  entries = g_new0 (LigmaEnumActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      entries[i].name           = g_strdup_printf ("file-open-recent-%02d",
                                                   i + 1);
      entries[i].icon_name      = LIGMA_ICON_DOCUMENT_OPEN,
      entries[i].label          = entries[i].name;
      entries[i].tooltip        = NULL;
      entries[i].value          = i;
      entries[i].value_variable = FALSE;

      if (i < 9)
        entries[i].accelerator = g_strdup_printf ("<primary>%d", i + 1);
      else if (i == 9)
        entries[i].accelerator = g_strdup ("<primary>0");
      else
        entries[i].accelerator = NULL;
    }

  ligma_action_group_add_enum_actions (group, NULL, entries, n_entries,
                                      file_open_recent_cmd_callback);

  for (i = 0; i < n_entries; i++)
    {
      ligma_action_group_set_action_visible (group, entries[i].name, FALSE);
      ligma_action_group_set_action_context (group, entries[i].name,
                                            ligma_get_user_context (group->ligma));

      g_free ((gchar *) entries[i].name);
      if (entries[i].accelerator)
        g_free ((gchar *) entries[i].accelerator);
    }

  g_free (entries);

  g_signal_connect_object (group->ligma->documents, "add",
                           G_CALLBACK (file_actions_last_opened_update),
                           group, 0);
  g_signal_connect_object (group->ligma->documents, "remove",
                           G_CALLBACK (file_actions_last_opened_update),
                           group, 0);
  g_signal_connect_object (group->ligma->documents, "reorder",
                           G_CALLBACK (file_actions_last_opened_reorder),
                           group, 0);

  file_actions_last_opened_update (group->ligma->documents, NULL, group);

  /*  also listen to image adding/removal so we catch the case where
   *  the last image is closed but its display stays open.
   */
  g_signal_connect_object (group->ligma->images, "add",
                           G_CALLBACK (file_actions_close_all_update),
                           group, 0);
  g_signal_connect_object (group->ligma->images, "remove",
                           G_CALLBACK (file_actions_close_all_update),
                           group, 0);

  file_actions_close_all_update (group->ligma->displays, NULL, group);
}

void
file_actions_update (LigmaActionGroup *group,
                     gpointer         data)
{
  Ligma         *ligma           = action_data_get_ligma (data);
  LigmaImage    *image          = action_data_get_image (data);
  GList        *drawables      = NULL;
  GFile        *file           = NULL;
  GFile        *source         = NULL;
  GFile        *export         = NULL;
  gboolean      show_overwrite = FALSE;

  if (image)
    {
      drawables = ligma_image_get_selected_drawables (image);

      file   = ligma_image_get_file (image);
      source = ligma_image_get_imported_file (image);
      export = ligma_image_get_exported_file (image);
    }

  show_overwrite =
    (source &&
     ligma_plug_in_manager_file_procedure_find (ligma->plug_in_manager,
                                               LIGMA_FILE_PROCEDURE_GROUP_EXPORT,
                                               source, NULL));

#define SET_VISIBLE(action,condition) \
        ligma_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("file-save",                 drawables);
  SET_SENSITIVE ("file-save-as",              drawables);
  SET_SENSITIVE ("file-save-a-copy",          drawables);
  SET_SENSITIVE ("file-save-and-close",       drawables);
  SET_SENSITIVE ("file-revert",               file || source);
  SET_SENSITIVE ("file-export",               drawables);
  SET_VISIBLE   ("file-export",               ! show_overwrite);
  SET_SENSITIVE ("file-overwrite",            show_overwrite);
  SET_VISIBLE   ("file-overwrite",            show_overwrite);
  SET_SENSITIVE ("file-export-as",            drawables);
  SET_SENSITIVE ("file-create-template",      image);
  SET_SENSITIVE ("file-copy-location",        file || source || export);
  SET_SENSITIVE ("file-show-in-file-manager", file || source || export);

  if (file)
    {
      ligma_action_group_set_action_label (group,
                                          "file-save",
                                          C_("file-action", "_Save"));
    }
  else
    {
      ligma_action_group_set_action_label (group,
                                          "file-save",
                                          C_("file-action", "_Save..."));
    }

  if (export)
    {
      gchar *label = file_actions_create_label (_("Export to %s"), export);
      ligma_action_group_set_action_label (group, "file-export", label);
      g_free (label);
    }
  else if (show_overwrite)
    {
      gchar *label = file_actions_create_label (_("Over_write %s"), source);
      ligma_action_group_set_action_label (group, "file-overwrite", label);
      g_free (label);
    }
  else
    {
      ligma_action_group_set_action_label (group,
                                          "file-export",
                                          C_("file-action", "E_xport..."));
    }

  /*  needed for the empty display  */
  SET_SENSITIVE ("file-close-all", image);

#undef SET_SENSITIVE

  g_list_free (drawables);
}


/*  private functions  */

static void
file_actions_last_opened_update (LigmaContainer   *container,
                                 LigmaImagefile   *unused,
                                 LigmaActionGroup *group)
{
  gint num_documents;
  gint i;
  gint n = LIGMA_GUI_CONFIG (group->ligma->config)->last_opened_size;

  num_documents = ligma_container_get_n_children (container);

  for (i = 0; i < n; i++)
    {
      LigmaAction *action;
      gchar      *name = g_strdup_printf ("file-open-recent-%02d", i + 1);

      action = ligma_action_group_get_action (group, name);

      if (i < num_documents)
        {
          LigmaImagefile *imagefile = (LigmaImagefile *)
            ligma_container_get_child_by_index (container, i);

          if (LIGMA_ACTION_IMPL (action)->viewable != (LigmaViewable *) imagefile)
            {
              GFile       *file;
              const gchar *name;
              gchar       *basename;
              gchar       *escaped;

              file = ligma_imagefile_get_file (imagefile);

              name     = ligma_file_get_utf8_name (file);
              basename = g_path_get_basename (name);

              escaped = ligma_escape_uline (basename);

              g_free (basename);

              g_object_set (action,
                            "label",    escaped,
                            "tooltip",  name,
                            "visible",  TRUE,
                            "viewable", imagefile,
                            NULL);

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
file_actions_last_opened_reorder (LigmaContainer   *container,
                                  LigmaImagefile   *unused1,
                                  gint             unused2,
                                  LigmaActionGroup *group)
{
  file_actions_last_opened_update (container, unused1, group);
}

static void
file_actions_close_all_update (LigmaContainer   *images,
                               LigmaObject      *unused,
                               LigmaActionGroup *group)
{
  LigmaContainer *container  = group->ligma->displays;
  gint           n_displays = ligma_container_get_n_children (container);
  gboolean       sensitive  = (n_displays > 0);

  if (n_displays == 1)
    {
      LigmaDisplay *display;

      display = LIGMA_DISPLAY (ligma_container_get_first_child (container));

      if (! ligma_display_get_image (display))
        sensitive = FALSE;
    }

  ligma_action_group_set_action_sensitive (group, "file-close-all", sensitive, NULL);
}

static gchar *
file_actions_create_label (const gchar *format,
                           GFile       *file)
{
  gchar *basename         = g_path_get_basename (ligma_file_get_utf8_name (file));
  gchar *escaped_basename = ligma_escape_uline (basename);
  gchar *label            = g_strdup_printf (format, escaped_basename);

  g_free (escaped_basename);
  g_free (basename);

  return label;
}
