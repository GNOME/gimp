/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportdialog.c
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "config/gimpcoreconfig.h"

#include "display/display-types.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "file/file-utils.h"
#include "file/gimp-file.h"

#include "pdb/gimppdb.h"

#include "plug-in/gimppluginmanager.h"

#include "gimpexportdialog.h"
#include "gimpfiledialog.h"
#include "gimphelp-ids.h"
#include "gimpsizebox.h"

#include "gimp-intl.h"


static void     gimp_export_dialog_constructed        (GObject *object);
static GFile  * gimp_export_dialog_get_default_folder (Gimp    *gimp);

G_DEFINE_TYPE (GimpExportDialog, gimp_export_dialog,
               GIMP_TYPE_FILE_DIALOG)

#define parent_class gimp_export_dialog_parent_class

static void
gimp_export_dialog_class_init (GimpExportDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gimp_export_dialog_constructed;
}

static void
gimp_export_dialog_init (GimpExportDialog *dialog)
{
}

static void
gimp_export_dialog_constructed (GObject *object)
{
  GimpExportDialog *dialog = GIMP_EXPORT_DIALOG (object);
  GtkWidget        *frame;
  GtkWidget        *hbox;
  GtkWidget        *label;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->size_expander = gtk_expander_new_with_mnemonic (NULL);
  gtk_expander_set_label (GTK_EXPANDER (dialog->size_expander),
                          _("Scale Image for Export"));

  dialog->resize_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (dialog->size_expander), dialog->resize_vbox);

  frame = gimp_frame_new (_("Quality"));
  gtk_box_pack_end (GTK_BOX (dialog->resize_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("I_nterpolation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  dialog->interpolation_combo = gimp_enum_combo_box_new (GIMP_TYPE_INTERPOLATION_TYPE);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->interpolation_combo);
  gtk_box_pack_start (GTK_BOX (hbox), dialog->interpolation_combo, FALSE, FALSE, 0);
  gtk_widget_show (dialog->interpolation_combo);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (dialog->interpolation_combo),
                                 GIMP_FILE_DIALOG (dialog)->gimp->config->interpolation_type);

  gimp_file_dialog_add_extra_widget (GIMP_FILE_DIALOG (dialog),
                                     dialog->size_expander,
                                     FALSE, FALSE, 0);
  gtk_widget_show (dialog->size_expander);
}

/*  public functions  */

GtkWidget *
gimp_export_dialog_new (Gimp *gimp)
{
  GimpExportDialog *dialog;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = g_object_new (GIMP_TYPE_EXPORT_DIALOG,
                         "gimp",                      gimp,
                         "title",                     _("Export Image"),
                         "role",                      "gimp-file-export",
                         "help-id",                   GIMP_HELP_FILE_EXPORT_AS,
                         "stock-id",                  _("_Export"),

                         "automatic-label",           _("By Extension"),
                         "automatic-help-id",         GIMP_HELP_FILE_SAVE_BY_EXTENSION,

                         "action",                    GTK_FILE_CHOOSER_ACTION_SAVE,
                         "file-procs",                gimp->plug_in_manager->export_procs,
                         "file-procs-all-images",     gimp->plug_in_manager->save_procs,
                         "file-filter-label",         _("All export images"),
                         "local-only",                FALSE,
                         "do-overwrite-confirmation", TRUE,
                         NULL);

  return GTK_WIDGET (dialog);
}

void
gimp_export_dialog_set_image (GimpExportDialog *dialog,
                              Gimp             *gimp,
                              GimpImage        *image)
{
  GimpContext           *context = gimp_get_user_context (gimp);
  GimpDisplay           *display = gimp_context_get_display (context);
  GFile                 *dir_file  = NULL;
  GFile                 *name_file = NULL;
  GFile                 *ext_file  = NULL;
  gchar                 *basename;
  gdouble                xres, yres;
  gint                   export_width;
  gint                   export_height;
  GimpUnit               export_unit;
  gdouble                export_xres;
  gdouble                export_yres;
  GimpUnit               export_res_unit;
  GimpInterpolationType  export_interpolation;

  g_return_if_fail (GIMP_IS_EXPORT_DIALOG (dialog));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  GIMP_FILE_DIALOG (dialog)->image = image;

  gimp_file_dialog_set_file_proc (GIMP_FILE_DIALOG (dialog), NULL);

  /*
   * Priority of default paths for Export:
   *
   *   1. Last Export path
   *   2. Path of import source
   *   3. Path of XCF source
   *   4. Last path of any save to XCF
   *   5. Last Export path of any document
   *   6. The default path (usually the OS 'Documents' path)
   */

  dir_file = gimp_image_get_exported_file (image);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (image),
                                  "gimp-image-source-file");

  if (! dir_file)
    dir_file = gimp_image_get_imported_file (image);

  if (! dir_file)
    dir_file = gimp_image_get_file (image);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (gimp),
                                  GIMP_FILE_SAVE_LAST_FILE_KEY);

  if (! dir_file)
    dir_file = g_object_get_data (G_OBJECT (gimp),
                                  GIMP_FILE_EXPORT_LAST_FILE_KEY);

  if (! dir_file)
    dir_file = gimp_export_dialog_get_default_folder (gimp);

  /* Priority of default basenames for Export:
   *
   *   1. Last Export name
   *   3. Save URI
   *   2. Source file name
   *   3. 'Untitled'
   */

  name_file = gimp_image_get_exported_file (image);

  if (! name_file)
    name_file = gimp_image_get_file (image);

  if (! name_file)
    name_file = gimp_image_get_imported_file (image);

  if (! name_file)
    name_file = gimp_image_get_untitled_file (image);


  /* Priority of default type/extension for Export:
   *
   *   1. Type of last Export
   *   2. Type of the image Import
   *   3. Type of latest Export of any document
   *   4. .png
   */
  ext_file = gimp_image_get_exported_file (image);

  if (! ext_file)
    ext_file = gimp_image_get_imported_file (image);

  if (! ext_file)
    ext_file = g_object_get_data (G_OBJECT (gimp),
                                  GIMP_FILE_EXPORT_LAST_FILE_KEY);

  if (ext_file)
    g_object_ref (ext_file);
  else
    ext_file = g_file_new_for_uri ("file:///we/only/care/about/extension.png");

  if (ext_file)
    {
      GFile *tmp_file = file_utils_file_with_new_ext (name_file, ext_file);
      basename = g_path_get_basename (gimp_file_get_utf8_name (tmp_file));
      g_object_unref (tmp_file);
      g_object_unref (ext_file);
    }
  else
    {
      basename = g_path_get_basename (gimp_file_get_utf8_name (name_file));
    }

  if (g_file_query_file_type (dir_file, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                dir_file, NULL);
    }
  else
    {
      GFile *parent_file = g_file_get_parent (dir_file);
      gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog),
                                                parent_file, NULL);
      g_object_unref (parent_file);
    }

  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);

  /* Extra feature for resize-on-export:
   * I have to destroy and recreate the size box because current code does not
   * behave well if I just reset properties to other values. */
  if (dialog->size_box)
    gtk_widget_destroy (dialog->size_box);

  gimp_image_get_resolution (image, &xres, &yres);

  dialog->size_box = g_object_new (GIMP_TYPE_SIZE_BOX,
                                   "width",           gimp_image_get_width (image),
                                   "height",          gimp_image_get_height (image),
                                   "unit",            (GimpUnit) gimp_display_get_shell (display)->unit,
                                   "xresolution",     xres,
                                   "yresolution",     yres,
                                   "resolution-unit", gimp_image_get_unit (image),
                                   "keep-aspect",     TRUE,
                                   "edit-resolution", TRUE,
                                   NULL);
  gimp_image_get_export_dimensions (image,
                                    &export_width,
                                    &export_height,
                                    &export_unit,
                                    &export_xres,
                                    &export_yres,
                                    &export_res_unit,
                                    &export_interpolation);

  if  (export_width != 0 && export_height != 0 &&
       export_xres  != 0 && export_yres  != 0  &&
       (export_width != gimp_image_get_width (image)   ||
        export_height != gimp_image_get_height (image) ||
        export_xres   != xres || export_yres != yres   ||
        export_res_unit != gimp_image_get_unit (image)))
    {
      /* Some values have been previously set. Keep them! */
      gimp_size_box_set_size (GIMP_SIZE_BOX (dialog->size_box),
                              export_width,
                              export_height,
                              export_unit);
      gimp_size_box_set_resolution (GIMP_SIZE_BOX (dialog->size_box),
                                    export_xres,
                                    export_yres,
                                    export_res_unit);
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (dialog->interpolation_combo),
                                     export_interpolation);
      gtk_expander_set_expanded (GTK_EXPANDER (dialog->size_expander),
                                 TRUE);
    }
  else
    {
      gtk_expander_set_expanded (GTK_EXPANDER (dialog->size_expander),
                                 FALSE);
    }
  gtk_box_pack_start (GTK_BOX (dialog->resize_vbox), dialog->size_box,
                      FALSE, FALSE, 0);
  gtk_widget_show_all (dialog->resize_vbox);
}

void
gimp_export_dialog_get_dimensions (GimpExportDialog      *dialog,
                                   gint                  *width,
                                   gint                  *height,
                                   GimpUnit              *unit,
                                   gdouble               *xresolution,
                                   gdouble               *yresolution,
                                   GimpUnit              *resolution_unit,
                                   GimpInterpolationType *interpolation)
{
  g_object_get (dialog->size_box,
                "width",           width,
                "height",          height,
                "unit",            unit,
                "xresolution",     xresolution,
                "yresolution",     yresolution,
                "resolution-unit", resolution_unit,
                NULL);
  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (dialog->interpolation_combo),
                                 (gint *) interpolation);
}

/*  private functions  */

static GFile *
gimp_export_dialog_get_default_folder (Gimp *gimp)
{
  if (gimp->default_folder)
    {
      return gimp->default_folder;
    }
  else
    {
      GFile *file = g_object_get_data (G_OBJECT (gimp),
                                       "gimp-documents-folder");

      if (! file)
        {
          gchar *path;

          /* Make sure it ends in '/' */
          path = g_build_path (G_DIR_SEPARATOR_S,
                               g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS),
                               G_DIR_SEPARATOR_S,
                               NULL);

          /* Paranoia fallback, see bug #722400 */
          if (! path)
            path = g_build_path (G_DIR_SEPARATOR_S,
                                 g_get_home_dir (),
                                 G_DIR_SEPARATOR_S,
                                 NULL);

          file = g_file_new_for_path (path);
          g_free (path);

          g_object_set_data_full (G_OBJECT (gimp), "gimp-documents-folder",
                                  file, (GDestroyNotify) g_object_unref);
        }

      return file;
    }
}
