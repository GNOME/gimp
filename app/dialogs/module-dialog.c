/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * module-dialog.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
 * (C) 2008 Sven Neumann <sven@gimp.org>
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
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimp-modules.h"

#include "widgets/gimphelp-ids.h"

#include "module-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_REFRESH  1

enum
{
  INFO_AUTHOR,
  INFO_VERSION,
  INFO_DATE,
  INFO_COPYRIGHT,
  INFO_LOCATION,
  N_INFOS
};

typedef struct _ModuleDialog ModuleDialog;

struct _ModuleDialog
{
  Gimp         *gimp;

  GimpModule   *selected;
  GtkListStore *list;
  GtkWidget    *listbox;

  GtkWidget    *hint;
  GtkWidget    *grid;
  GtkWidget    *label[N_INFOS];
  GtkWidget    *error_box;
  GtkWidget    *error_label;
};


/*  local function prototypes  */

static GtkWidget *   create_widget_for_module   (gpointer          item,
                                                 gpointer          user_data);
static void          dialog_response            (GtkWidget        *widget,
                                                 gint              response_id,
                                                 ModuleDialog     *private);
static void          dialog_destroy_callback    (GtkWidget        *widget,
                                                 ModuleDialog     *private);
static void          dialog_select_callback     (GtkListBox       *listbox,
                                                 GtkListBoxRow    *row,
                                                 ModuleDialog     *private);
static void          dialog_enabled_toggled     (GtkToggleButton  *checkbox,
                                                 ModuleDialog     *private);
static void          dialog_info_init            (ModuleDialog     *private,
                                                 GtkWidget         *grid);


/*  public functions  */

GtkWidget *
module_dialog_new (Gimp *gimp)
{
  ModuleDialog      *private;
  GtkWidget         *dialog;
  GtkWidget         *vbox;
  GtkWidget         *sw;
  GtkWidget         *image;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  private = g_slice_new0 (ModuleDialog);

  private->gimp = gimp;

  dialog = gimp_dialog_new (_("Module Manager"),
                            "gimp-modules", NULL, 0,
                            gimp_standard_help_func, GIMP_HELP_MODULE_DIALOG,

                            _("_Refresh"), RESPONSE_REFRESH,
                            _("_Close"),   GTK_RESPONSE_CLOSE,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CLOSE,
                                           RESPONSE_REFRESH,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (dialog_response),
                    private);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  private->hint = gimp_hint_box_new (_("You will have to restart GIMP "
                                       "for the changes to take effect."));
  gtk_box_pack_start (GTK_BOX (vbox), private->hint, FALSE, FALSE, 0);

  if (gimp->write_modulerc)
    gtk_widget_show (private->hint);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_widget_set_size_request (sw, 124, 100);
  gtk_widget_show (sw);

  private->listbox = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (private->listbox),
                                   GTK_SELECTION_BROWSE);
  gtk_list_box_bind_model (GTK_LIST_BOX (private->listbox),
                           G_LIST_MODEL (gimp->module_db),
                           create_widget_for_module,
                           private,
                           NULL);
  g_signal_connect (private->listbox, "row-selected",
                    G_CALLBACK (dialog_select_callback),
                    private);

  gtk_container_add (GTK_CONTAINER (sw), private->listbox);
  gtk_widget_show (private->listbox);

  private->grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (private->grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), private->grid, FALSE, FALSE, 0);
  gtk_widget_show (private->grid);

  private->error_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), private->error_box, FALSE, FALSE, 0);

  image = gtk_image_new_from_icon_name (GIMP_ICON_DIALOG_WARNING,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (private->error_box), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  private->error_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (private->error_label), 0.0);
  gtk_box_pack_start (GTK_BOX (private->error_box),
                      private->error_label, TRUE, TRUE, 0);
  gtk_widget_show (private->error_label);

  dialog_info_init (private, private->grid);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (dialog_destroy_callback),
                    private);

  return dialog;
}


/*  private functions  */

static GtkWidget *
create_widget_for_module (gpointer item,
                          gpointer user_data)
{
  GimpModule           *module  = GIMP_MODULE (item);
  ModuleDialog         *private = user_data;
  const GimpModuleInfo *info    = gimp_module_get_info (module);
  GFile                *file    = gimp_module_get_file (module);
  GtkWidget            *row;
  GtkWidget            *grid;
  GtkWidget            *label;
  GtkWidget            *checkbox;

  row = gtk_list_box_row_new ();
  g_object_set_data (G_OBJECT (row), "module", module);
  gtk_widget_show (row);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  g_object_set (grid, "margin", 3, NULL);
  gtk_container_add (GTK_CONTAINER (row), grid);
  gtk_widget_show (grid);

  checkbox = gtk_check_button_new ();
  g_object_bind_property (module, "auto-load", checkbox, "active",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  g_signal_connect (checkbox, "toggled",
                    G_CALLBACK (dialog_enabled_toggled),
                    private);
  gtk_widget_show (checkbox);
  gtk_grid_attach (GTK_GRID (grid), checkbox, 0, 0, 1, 1);

  label = gtk_label_new (info ? dgettext (GETTEXT_PACKAGE "-libgimp", info->purpose) :
                                gimp_file_get_utf8_name (file));
  gtk_widget_show (label);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 0, 1, 1);

  return row;
}

static void
dialog_response (GtkWidget    *widget,
                 gint          response_id,
                 ModuleDialog *private)
{
  if (response_id == RESPONSE_REFRESH)
    gimp_modules_refresh (private->gimp);
  else
    gtk_widget_destroy (widget);
}

static void
dialog_destroy_callback (GtkWidget    *widget,
                         ModuleDialog *private)
{
  g_slice_free (ModuleDialog, private);
}

static void
dialog_select_callback (GtkListBox    *listbox,
                        GtkListBoxRow *row,
                        ModuleDialog  *private)
{
  guint                 i;
  GimpModule           *module;
  const GimpModuleInfo *info;
  const gchar          *location      = NULL;
  const gchar          *text[N_INFOS] = { NULL, };
  gboolean              show_error;

  if (row == NULL)
    {
      for (i = 0; i < N_INFOS; i++)
        gtk_label_set_text (GTK_LABEL (private->label[i]), NULL);
      gtk_label_set_text (GTK_LABEL (private->error_label), NULL);
      gtk_widget_hide (private->error_box);
      return;
    }

  module = g_object_get_data (G_OBJECT (row), "module");
  if (private->selected == module)
    return;

  private->selected = module;

  if (gimp_module_is_on_disk (module))
    location = gimp_file_get_utf8_name (gimp_module_get_file (module));

  info = gimp_module_get_info (module);

  if (info)
    {
      text[INFO_AUTHOR]    = info->author;
      text[INFO_VERSION]   = info->version;
      text[INFO_DATE]      = info->date;
      text[INFO_COPYRIGHT] = info->copyright;
      text[INFO_LOCATION]  = location ? location : _("Only in memory");
    }
  else
    {
      text[INFO_LOCATION]  = location ? location : _("No longer available");
    }

  for (i = 0; i < N_INFOS; i++)
    gtk_label_set_text (GTK_LABEL (private->label[i]),
                        text[i] ? text[i] : "--");

  /* Show errors */
  show_error = (gimp_module_get_state (module) == GIMP_MODULE_STATE_ERROR &&
                gimp_module_get_last_error (module));
  gtk_label_set_text (GTK_LABEL (private->error_label),
                      show_error ? gimp_module_get_last_error (module) : NULL);
  gtk_widget_set_visible (private->error_box, show_error);
}

static void
dialog_enabled_toggled (GtkToggleButton *checkbox,
                        ModuleDialog    *private)
{
  private->gimp->write_modulerc = TRUE;
  gtk_widget_show (private->hint);
}

static void
dialog_info_init (ModuleDialog *private,
                  GtkWidget    *grid)
{
  GtkWidget *label;
  gint       i;

  const gchar * const text[] =
  {
    N_("Author:"),
    N_("Version:"),
    N_("Date:"),
    N_("Copyright:"),
    N_("Location:")
  };

  for (i = 0; i < G_N_ELEMENTS (text); i++)
    {
      label = gtk_label_new (gettext (text[i]));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, i, 1, 1);
      gtk_widget_show (label);

      private->label[i] = gtk_label_new ("");
      gtk_label_set_xalign (GTK_LABEL (private->label[i]), 0.0);
      gtk_label_set_ellipsize (GTK_LABEL (private->label[i]),
                               PANGO_ELLIPSIZE_END);
      gtk_grid_attach (GTK_GRID (grid), private->label[i], 1, i, 1, 1);
      gtk_widget_show (private->label[i]);
    }
}
