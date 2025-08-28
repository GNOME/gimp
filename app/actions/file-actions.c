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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimpviewable.h"

#include "file/gimp-file.h"

#include "plug-in/gimppluginmanager-file.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimpactionimpl.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "actions.h"
#include "file-actions.h"
#include "file-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void    file_actions_last_opened_update  (GimpContainer   *container,
                                                 GimpImagefile   *unused,
                                                 GimpActionGroup *group);
static void    file_actions_last_opened_reorder (GimpContainer   *container,
                                                 GimpImagefile   *unused1,
                                                 gint             unused2,
                                                 gint             unused3,
                                                 GimpActionGroup *group);
static void    file_actions_close_all_update    (GimpContainer   *images,
                                                 GimpObject      *unused,
                                                 GimpActionGroup *group);
static gchar * file_actions_create_label        (const gchar     *format,
                                                 GFile           *file);


static const GimpActionEntry file_actions[] =
{
  { "file-open", GIMP_ICON_IMAGE_OPEN,
    NC_("file-action", "_Open..."), NULL, { "<primary>O", NULL },
    NC_("file-action", "Open an image file"),
    file_open_cmd_callback,
    GIMP_HELP_FILE_OPEN },

  { "file-open-as-layers", GIMP_ICON_LAYER,
    NC_("file-action", "Op_en as Layers..."), NULL, { "<primary><alt>O", NULL },
    NC_("file-action", "Open an image file as layers"),
    file_open_as_layers_cmd_callback,
    GIMP_HELP_FILE_OPEN_AS_LAYER },

  { "file-open-as-link-layers", GIMP_ICON_LAYER,
    NC_("file-action", "Op_en as Link Layer..."), NULL, { "<primary><alt><shift>O", NULL },
    NC_("file-action", "Open an image file as Link layer"),
    file_open_as_link_layer_cmd_callback,
    GIMP_HELP_FILE_OPEN_AS_LINK_LAYER },

  { "file-open-location", GIMP_ICON_WEB,
    NC_("file-action", "Open _Location..."), NULL, { NULL },
    NC_("file-action", "Open an image file from a specified location"),
    file_open_location_cmd_callback,
    GIMP_HELP_FILE_OPEN_LOCATION },

  { "file-create-template", NULL,
    NC_("file-action", "Create _Template..."), NULL, { NULL },
    NC_("file-action", "Create a new template from this image"),
    file_create_template_cmd_callback,
    GIMP_HELP_FILE_CREATE_TEMPLATE },

  { "file-revert", GIMP_ICON_IMAGE_RELOAD,
    NC_("file-action", "Re_vert"), NULL, { NULL },
    NC_("file-action", "Reload the image file from disk"),
    file_revert_cmd_callback,
    GIMP_HELP_FILE_REVERT },

  { "file-close-all", GIMP_ICON_CLOSE_ALL,
    NC_("file-action", "C_lose All"), NULL, { "<primary><shift>W", NULL },
    NC_("file-action", "Close all opened images"),
    file_close_all_cmd_callback,
    GIMP_HELP_FILE_CLOSE_ALL },

  { "file-copy-location", GIMP_ICON_EDIT_COPY,
    NC_("file-action", "Copy _Image Location"), NULL, { NULL },
    NC_("file-action", "Copy image file location to clipboard"),
    file_copy_location_cmd_callback,
    GIMP_HELP_FILE_COPY_LOCATION },

  { "file-show-in-file-manager", GIMP_ICON_FILE_MANAGER,
    NC_("file-action", "Show in _File Manager"), NULL, { "<primary><alt>F", NULL },
    NC_("file-action", "Show image file location in the file manager"),
    file_show_in_file_manager_cmd_callback,
    GIMP_HELP_FILE_SHOW_IN_FILE_MANAGER },

  { "file-quit", GIMP_ICON_APPLICATION_EXIT,
    NC_("file-action", "_Quit"), NULL, { "<primary>Q", NULL },
    NC_("file-action", "Quit the GNU Image Manipulation Program"),
    file_quit_cmd_callback,
    GIMP_HELP_FILE_QUIT }
};

static const GimpEnumActionEntry file_save_actions[] =
{
  { "file-save", GIMP_ICON_DOCUMENT_SAVE,
    NC_("file-action", "_Save"), NULL, { "<primary>S", NULL },
    NC_("file-action", "Save this image"),
    GIMP_SAVE_MODE_SAVE, FALSE,
    GIMP_HELP_FILE_SAVE },

  { "file-save-as", GIMP_ICON_DOCUMENT_SAVE_AS,
    NC_("file-action", "Save _As..."), NULL, { "<primary><shift>S", NULL },
    NC_("file-action", "Save this image with a different name"),
    GIMP_SAVE_MODE_SAVE_AS, FALSE,
    GIMP_HELP_FILE_SAVE_AS },

  { "file-save-a-copy", NULL,
    NC_("file-action", "Save a Cop_y..."), NULL, { NULL },
    NC_("file-action",
        "Save a copy of this image, without affecting the source file "
        "(if any) or the current state of the image"),
    GIMP_SAVE_MODE_SAVE_A_COPY, FALSE,
    GIMP_HELP_FILE_SAVE_A_COPY },

  { "file-save-and-close", NULL,
    NC_("file-action", "Save and Close..."), NULL, { NULL },
    NC_("file-action", "Save this image and close its window"),
    GIMP_SAVE_MODE_SAVE_AND_CLOSE, FALSE,
    GIMP_HELP_FILE_SAVE },

  { "file-export", NULL,
    NC_("file-action", "E_xport..."), NULL, { "<primary>E", NULL },
    NC_("file-action", "Export the image"),
    GIMP_SAVE_MODE_EXPORT, FALSE,
    GIMP_HELP_FILE_EXPORT },

  { "file-overwrite", NULL,
    NC_("file-action", "Over_write"), NULL, { NULL },
    NC_("file-action", "Export the image back to the imported file in the import format"),
    GIMP_SAVE_MODE_OVERWRITE, FALSE,
    GIMP_HELP_FILE_OVERWRITE },

  { "file-export-as", NULL,
    NC_("file-action", "E_xport As..."), NULL, { "<primary><shift>E", NULL },
    NC_("file-action", "Export the image to various file formats such as PNG or JPEG"),
    GIMP_SAVE_MODE_EXPORT_AS, FALSE,
    GIMP_HELP_FILE_EXPORT_AS }
};

void
file_actions_setup (GimpActionGroup *group)
{
  GimpEnumActionEntry *entries;
  gint                 n_entries;
  gint                 i;

  gimp_action_group_add_actions (group, "file-action",
                                 file_actions,
                                 G_N_ELEMENTS (file_actions));

  gimp_action_group_add_enum_actions (group, "file-action",
                                      file_save_actions,
                                      G_N_ELEMENTS (file_save_actions),
                                      file_save_cmd_callback);

  n_entries = GIMP_GUI_CONFIG (group->gimp->config)->last_opened_size;

  entries = g_new0 (GimpEnumActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      entries[i].name           = g_strdup_printf ("file-open-recent-%02d",
                                                   i + 1);
      entries[i].icon_name      = GIMP_ICON_DOCUMENT_OPEN,
      entries[i].label          = entries[i].name;
      entries[i].tooltip        = NULL;
      entries[i].value          = i;
      entries[i].value_variable = FALSE;

      if (i < 9)
        entries[i].accelerator[0] = g_strdup_printf ("<primary>%d", i + 1);
      else if (i == 9)
        entries[i].accelerator[0] = g_strdup ("<primary>0");
      else
        entries[i].accelerator[0] = NULL;
    }

  gimp_action_group_add_enum_actions (group, NULL, entries, n_entries,
                                      file_open_recent_cmd_callback);

  for (i = 0; i < n_entries; i++)
    {
      gimp_action_group_set_action_visible (group, entries[i].name, FALSE);

      g_free ((gchar *) entries[i].name);
      if (entries[i].accelerator[0])
        g_free ((gchar *) entries[i].accelerator[0]);
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

  /*  also listen to image adding/removal so we catch the case where
   *  the last image is closed but its display stays open.
   */
  g_signal_connect_object (group->gimp->images, "add",
                           G_CALLBACK (file_actions_close_all_update),
                           group, 0);
  g_signal_connect_object (group->gimp->images, "remove",
                           G_CALLBACK (file_actions_close_all_update),
                           group, 0);

  file_actions_close_all_update (group->gimp->displays, NULL, group);
}

void
file_actions_update (GimpActionGroup *group,
                     gpointer         data)
{
  Gimp         *gimp           = action_data_get_gimp (data);
  GimpImage    *image          = action_data_get_image (data);
  GList        *drawables      = NULL;
  GFile        *file           = NULL;
  GFile        *source         = NULL;
  GFile        *export         = NULL;
  gboolean      show_overwrite = FALSE;

  if (image)
    {
      drawables = gimp_image_get_selected_drawables (image);

      file   = gimp_image_get_file (image);
      source = gimp_image_get_imported_file (image);
      export = gimp_image_get_exported_file (image);
    }

  show_overwrite =
    (source &&
     gimp_plug_in_manager_file_procedure_find (gimp->plug_in_manager,
                                               GIMP_FILE_PROCEDURE_GROUP_EXPORT,
                                               source, NULL));

#define SET_VISIBLE(action,condition) \
        gimp_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

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
      gimp_action_group_set_action_label (group,
                                          "file-save",
                                          C_("file-action", "_Save"));
    }
  else
    {
      gimp_action_group_set_action_label (group,
                                          "file-save",
                                          C_("file-action", "_Save..."));
    }

  if (export)
    {
      GimpAction *action;
      gchar      *label = file_actions_create_label (_("Export to %s"),
                                                     export);

      gimp_action_group_set_action_label (group, "file-export", label);

      action = gimp_action_group_get_action (group, "file-export");
      g_object_set (action,
                    "ellipsize",       PANGO_ELLIPSIZE_MIDDLE,
                    "max-width-chars", 40,
                    NULL);

      g_free (label);
    }
  else if (show_overwrite)
    {
      GimpAction *action;
      gchar      *label = file_actions_create_label (_("Over_write %s"),
                                                     source);

      gimp_action_group_set_action_label (group, "file-overwrite", label);

      action = gimp_action_group_get_action (group, "file-overwrite");
      g_object_set (action,
                    "ellipsize",       PANGO_ELLIPSIZE_MIDDLE,
                    "max-width-chars", 40,
                    NULL);

      g_free (label);
    }
  else
    {
      gimp_action_group_set_action_label (group,
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
file_actions_last_opened_update (GimpContainer   *container,
                                 GimpImagefile   *unused,
                                 GimpActionGroup *group)
{
  gint num_documents;
  gint i;
  gint n = GIMP_GUI_CONFIG (group->gimp->config)->last_opened_size;

  num_documents = gimp_container_get_n_children (container);

  for (i = 0; i < n; i++)
    {
      GimpAction *action;
      gchar      *name = g_strdup_printf ("file-open-recent-%02d", i + 1);

      action = gimp_action_group_get_action (group, name);

      if (i < num_documents)
        {
          GimpImagefile *imagefile = (GimpImagefile *)
            gimp_container_get_child_by_index (container, i);

          if (gimp_action_get_viewable (action) != (GimpViewable *) imagefile)
            {
              GFile       *file;
              const gchar *name;
              gchar       *basename;
              gchar       *escaped;
              gchar       *label;

              file = gimp_imagefile_get_file (imagefile);

              name     = gimp_file_get_utf8_name (file);
              basename = g_path_get_basename (name);

              escaped = gimp_escape_uline (basename);
              g_free (basename);
              /* TRANSLATORS: the %s will be replaced by the file name of a
               * recently opened image.
               */
              label   = g_strdup_printf (_("Open \"%s\""), escaped);

              g_object_set (action,
                            "label",           label,
                            "short-label",     escaped,
                            "tooltip",         name,
                            "visible",         TRUE,
                            "viewable",        imagefile,
                            "ellipsize",       PANGO_ELLIPSIZE_MIDDLE,
                            "max-width-chars", 40,
                            NULL);

              g_free (label);
              g_free (escaped);
            }
        }
      else
        {
          g_object_set (action,
                        "label",           name,
                        "tooltip",         NULL,
                        "visible",         FALSE,
                        "viewable",        NULL,
                        "ellipsize",       PANGO_ELLIPSIZE_MIDDLE,
                        "max-width-chars", 40,
                        NULL);
        }

      g_free (name);
    }
}

static void
file_actions_last_opened_reorder (GimpContainer   *container,
                                  GimpImagefile   *unused1,
                                  gint             unused2,
                                  gint             unused3,
                                  GimpActionGroup *group)
{
  file_actions_last_opened_update (container, unused1, group);
}

static void
file_actions_close_all_update (GimpContainer   *images,
                               GimpObject      *unused,
                               GimpActionGroup *group)
{
  GimpContainer *container  = group->gimp->displays;
  gint           n_displays = gimp_container_get_n_children (container);
  gboolean       sensitive  = (n_displays > 0);

  if (n_displays == 1)
    {
      GimpDisplay *display;

      display = GIMP_DISPLAY (gimp_container_get_first_child (container));

      if (! gimp_display_get_image (display))
        sensitive = FALSE;
    }

  gimp_action_group_set_action_sensitive (group, "file-close-all", sensitive, NULL);
}

static gchar *
file_actions_create_label (const gchar *format,
                           GFile       *file)
{
  gchar *basename         = g_path_get_basename (gimp_file_get_utf8_name (file));
  gchar *escaped_basename = gimp_escape_uline (basename);
  gchar *label            = g_strdup_printf (format, escaped_basename);

  g_free (escaped_basename);
  g_free (basename);

  return label;
}
