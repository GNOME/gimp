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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
/*
 * The Page Specific Imports
 * */
#include "brush.h"
#include "color.h"
#include "general.h"
#include "orientation.h"
#include "orientmap.h"
#include "placement.h"
#include "preview.h"
#include "size.h"
#include "paper.h"
#include "presets.h"

#include "ppmtool.h"

#include "libgimp/stdplugins-intl.h"


static GtkWidget *dialog = NULL;

void
store_values (void)
{
  paper_store ();
  brush_store ();
  general_store ();
}

void
restore_values (void)
{
  brush_restore ();
  paper_restore ();
  orientation_restore ();
  size_restore ();
  place_restore ();
  general_restore ();
  color_restore ();

  update_orientmap_dialog ();
}

GtkWidget *
create_one_column_list (GtkWidget *parent,
                        void (*changed_cb) (GtkTreeSelection *selection,
                                            gpointer data))
{
  GtkListStore      *store;
  GtkTreeSelection  *selection;
  GtkCellRenderer   *renderer;
  GtkTreeViewColumn *column;
  GtkWidget         *swin, *view;

  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (parent), swin, FALSE, FALSE, 0);
  gtk_widget_show (swin);
  gtk_widget_set_size_request (swin, 150,-1);

  store = gtk_list_store_new (1, G_TYPE_STRING);
  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
  g_object_unref (store);
  gtk_widget_show (view);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Preset", renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  gtk_container_add (GTK_CONTAINER (swin), view);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed", G_CALLBACK (changed_cb), NULL);

  return view;
}

static gboolean
create_dialog (GimpProcedure       *procedure,
               GimpProcedureConfig *config)
{
  GtkWidget *notebook;
  GtkWidget *hbox;
  GtkWidget *preview_box;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("GIMPressionist"));

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  preview_box = create_preview ();
  gtk_box_pack_start (GTK_BOX (hbox), preview_box, FALSE, FALSE, 0);
  gtk_widget_show (preview_box);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (hbox), notebook, TRUE, TRUE, 5);
  gtk_widget_show (notebook);

  create_presetpage (GTK_NOTEBOOK (notebook));
  create_paperpage (GTK_NOTEBOOK (notebook));
  create_brushpage (GTK_NOTEBOOK (notebook));
  create_orientationpage (GTK_NOTEBOOK (notebook));
  create_sizepage (GTK_NOTEBOOK (notebook));
  create_placementpage (GTK_NOTEBOOK (notebook));
  create_colorpage (GTK_NOTEBOOK (notebook));
  create_generalpage (GTK_NOTEBOOK (notebook));

  updatepreview (NULL, NULL);

  /*
   * This is to make sure the values from the pcvals will be reflected
   * in the GUI here. Otherwise they will be set to the defaults.
   * */
  restore_values ();

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));
  if (run)
    store_values ();

  gtk_widget_destroy (dialog);

  return run;
}

gint
create_gimpressionist (GimpProcedure       *procedure,
                       GimpProcedureConfig *config)
{
  pcvals.run = FALSE;

  pcvals.run = create_dialog (procedure, config);

  return pcvals.run;
}
