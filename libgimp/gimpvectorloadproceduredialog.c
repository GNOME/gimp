/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectorloadproceduredialog.c
 * Copyright (C) 2024 Jehan
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"
#include "gimpui.h"
#include "gimpvectorloadproceduredialog.h"
#include "gimpresolutionentry-private.h"

#include "libgimp-intl.h"


struct _GimpVectorLoadProcedureDialogPrivate
{
  /* TODO: add thumbnail support from the file. */
  GFile    *file;
};


static void gimp_vector_load_procedure_dialog_fill_list  (GimpProcedureDialog *dialog,
                                                          GimpProcedure       *procedure,
                                                          GimpProcedureConfig *config,
                                                          GList               *properties);
static void gimp_vector_load_procedure_dialog_fill_start (GimpProcedureDialog *dialog,
                                                          GimpProcedure       *procedure,
                                                          GimpProcedureConfig *config);


G_DEFINE_TYPE_WITH_PRIVATE (GimpVectorLoadProcedureDialog, gimp_vector_load_procedure_dialog, GIMP_TYPE_PROCEDURE_DIALOG)

#define parent_class gimp_vector_load_procedure_dialog_parent_class

static void
gimp_vector_load_procedure_dialog_class_init (GimpVectorLoadProcedureDialogClass *klass)
{
  GimpProcedureDialogClass *proc_dialog_class;

  proc_dialog_class = GIMP_PROCEDURE_DIALOG_CLASS (klass);

  proc_dialog_class->fill_start = gimp_vector_load_procedure_dialog_fill_start;
  proc_dialog_class->fill_list  = gimp_vector_load_procedure_dialog_fill_list;
}

static void
gimp_vector_load_procedure_dialog_init (GimpVectorLoadProcedureDialog *dialog)
{
  dialog->priv = gimp_vector_load_procedure_dialog_get_instance_private (dialog);

  dialog->priv->file        = NULL;
}

static void
gimp_vector_load_procedure_dialog_fill_start (GimpProcedureDialog *dialog,
                                              GimpProcedure       *procedure,
                                              GimpProcedureConfig *config)
{
  GtkWidget *content_area;
  GtkWidget *res_entry;

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  /* Resolution */
  res_entry = gimp_prop_resolution_entry_new (G_OBJECT (config),
                                              "width", "height", "pixel-density",
                                              "physical-unit");

  gtk_box_pack_start (GTK_BOX (content_area), res_entry, FALSE, FALSE, 0);
  gtk_widget_show (res_entry);

  GIMP_PROCEDURE_DIALOG_CLASS (parent_class)->fill_start (dialog, procedure, config);
}

static void
gimp_vector_load_procedure_dialog_fill_list (GimpProcedureDialog *dialog,
                                             GimpProcedure       *procedure,
                                             GimpProcedureConfig *config,
                                             GList               *properties)
{
  GList *properties2 = NULL;
  GList *iter;

  for (iter = properties; iter; iter = iter->next)
    {
      gchar *propname = iter->data;

      if (g_strcmp0 (propname, "width") == 0                    ||
          g_strcmp0 (propname, "height") == 0                   ||
          g_strcmp0 (propname, "keep-ratio") == 0               ||
          g_strcmp0 (propname, "prefer-native-dimensions") == 0 ||
          g_strcmp0 (propname, "pixel-density") == 0            ||
          g_strcmp0 (propname, "physical-unit") == 0)
        /* Ignoring the standards args which are being handled by fill_start(). */
        continue;

      properties2 = g_list_prepend (properties2, propname);
    }
  properties2 = g_list_reverse (properties2);
  GIMP_PROCEDURE_DIALOG_CLASS (parent_class)->fill_list (dialog, procedure, config, properties2);
  g_list_free (properties2);
}


/* Public Functions */


GtkWidget *
gimp_vector_load_procedure_dialog_new (GimpVectorLoadProcedure *procedure,
                                       GimpProcedureConfig     *config,
                                       GFile                   *file)
{
  GtkWidget   *dialog;
  gchar       *title;
  const gchar *format_name;
  const gchar *help_id;
  gboolean     use_header_bar;

  g_return_val_if_fail (GIMP_IS_VECTOR_LOAD_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (gimp_procedure_config_get_procedure (config) ==
                        GIMP_PROCEDURE (procedure), NULL);
  /*g_return_val_if_fail (G_IS_FILE (file), NULL);*/

  format_name = gimp_file_procedure_get_format_name (GIMP_FILE_PROCEDURE (procedure));
  if (! format_name)
    {
      g_critical ("%s: no format name set on file procedure '%s'. "
                  "Set one with gimp_file_procedure_set_format_name()",
                  G_STRFUNC,
                  gimp_procedure_get_name (GIMP_PROCEDURE (procedure)));
      return NULL;
    }
  /* TRANSLATORS: %s will be a format name, e.g. "PNG" or "JPEG". */
  title = g_strdup_printf (_("Load %s Image"), format_name);

  help_id = gimp_procedure_get_help_id (GIMP_PROCEDURE (procedure));

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (GIMP_TYPE_VECTOR_LOAD_PROCEDURE_DIALOG,
                         "procedure",      procedure,
                         "config",         config,
                         "title",          title,
                         "help-func",      gimp_standard_help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                         NULL);
  GIMP_VECTOR_LOAD_PROCEDURE_DIALOG (dialog)->priv->file = file;
  g_free (title);

  return dialog;
}
