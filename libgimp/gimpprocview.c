/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpprocview.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

/*
 * dbbrowser_utils.c
 * 0.08  26th sept 97  by Thomas NOEL <thomas@minet.net>
 *
 * 98/12/13  Sven Neumann <sven@gimp.org> : added help display
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"

#include "gimpuitypes.h"
#include "gimpprocview.h"

#include "libgimp-intl.h"


/**
 * SECTION: gimpprocview
 * @title: GimpProcView
 * @short_description: A widget showing information about a PDB procedure.
 *
 * A widget showing information about a PDB procedure, mainly for the
 * procedure and plug-in browsers.
 **/


/*  local function prototypes  */

static GtkWidget * gimp_proc_view_create_params (const GimpParamDef *params,
                                                 gint                n_params,
                                                 GtkSizeGroup       *name_group,
                                                 GtkSizeGroup       *type_group,
                                                 GtkSizeGroup       *desc_group);
static GtkWidget * gimp_proc_view_create_args   (const gchar        *procedure_name,
                                                 gint                n_args,
                                                 gboolean            return_values,
                                                 GtkSizeGroup       *name_group,
                                                 GtkSizeGroup       *type_group,
                                                 GtkSizeGroup       *desc_group);


/*  public functions  */


/**
 * gimp_proc_view_new:
 * @procedure_name: The name of a procedure.
 * @menu_path:      (nullable):  The procedure's menu path, or %NULL.
 *
 * Returns: (transfer full): a new widget providing a view on a
 *          GIMP procedure
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_proc_view_new (const gchar *procedure_name,
                    const gchar *menu_path)
{
  GtkWidget       *main_vbox;
  GtkWidget       *frame;
  GtkWidget       *vbox;
  GtkWidget       *grid;
  GtkWidget       *label;
  GtkWidget       *notebook;
  GtkSizeGroup    *name_group;
  GtkSizeGroup    *type_group;
  GtkSizeGroup    *desc_group;
  gchar           *blurb;
  gchar           *help;
  gchar           *author;
  gchar           *copyright;
  gchar           *date;
  GimpPDBProcType  type;
  gint             n_params;
  gint             n_return_vals;
  GimpParamDef    *params;
  GimpParamDef    *return_vals;
  const gchar     *type_str;
  gint             row;

  g_return_val_if_fail (procedure_name != NULL, NULL);

  gimp_pdb_proc_info (procedure_name,
                      &blurb,
                      &help,
                      &author,
                      &copyright,
                      &date,
                      &type,
                      &n_params,
                      &n_return_vals,
                      &params,
                      &return_vals);

  if (blurb     && strlen (blurb) < 2)     g_clear_pointer (&blurb,     g_free);
  if (help      && strlen (help) < 2)      g_clear_pointer (&help,      g_free);
  if (author    && strlen (author) < 2)    g_clear_pointer (&author,    g_free);
  if (date      && strlen (date) < 2)      g_clear_pointer (&date,      g_free);
  if (copyright && strlen (copyright) < 2) g_clear_pointer (&copyright, g_free);

  if (blurb && help && ! strcmp (blurb, help))
    g_clear_pointer (&help, g_free);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  /* show the name */

  frame = gimp_frame_new (procedure_name);
  label = gtk_frame_get_label_widget (GTK_FRAME (frame));
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  if (! gimp_enum_get_value (GIMP_TYPE_PDB_PROC_TYPE, type,
                             NULL, NULL, &type_str, NULL))
    type_str = "UNKNOWN";

  label = gtk_label_new (type_str);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (menu_path)
    {
      label = gtk_label_new_with_mnemonic (menu_path);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
    }

  if (blurb)
    {
      label = gtk_label_new (blurb);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
    }

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("GValue-based API")));
  gtk_widget_show (vbox);

  name_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  type_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  desc_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* in parameters */
  if (n_params)
    {
      frame = gimp_frame_new (_("Parameters"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gimp_proc_view_create_args (procedure_name, n_params, FALSE,
                                         name_group, type_group, desc_group);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);
    }

  /* out parameters */
  if (n_return_vals)
    {
      frame = gimp_frame_new (_("Return Values"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gimp_proc_view_create_args (procedure_name, n_return_vals, TRUE,
                                         name_group, type_group, desc_group);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);
    }

  g_object_unref (name_group);
  g_object_unref (type_group);
  g_object_unref (desc_group);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Legacy API")));
  gtk_widget_show (vbox);

  name_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  type_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  desc_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* in parameters */
  if (n_params)
    {
      frame = gimp_frame_new (_("Parameters"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gimp_proc_view_create_params (params, n_params,
                                           name_group, type_group, desc_group);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);
    }

  /* out parameters */
  if (n_return_vals)
    {
      frame = gimp_frame_new (_("Return Values"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gimp_proc_view_create_params (return_vals, n_return_vals,
                                           name_group, type_group, desc_group);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);
    }

  g_object_unref (name_group);
  g_object_unref (type_group);
  g_object_unref (desc_group);

  if (! help && ! author && ! date && ! copyright)
    return main_vbox;

  frame = gimp_frame_new (_("Additional Information"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* show the help */
  if (help)
    {
      label = gtk_label_new (help);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
    }

  /* show the author & the copyright */

  if (! author && ! date && ! copyright)
    goto out;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  row = 0;

  if (author)
    {
      label = gtk_label_new (author);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                _("Author:"), 0.0, 0.0,
                                label, 3);
    }

  if (date)
    {
      label = gtk_label_new (date);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                _("Date:"), 0.0, 0.0,
                                label, 3);
    }

  if (copyright)
    {
      label = gtk_label_new (copyright);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                _("Copyright:"), 0.0, 0.0,
                                label, 3);
    }

 out:

  g_free (blurb);
  g_free (help);
  g_free (author);
  g_free (copyright);
  g_free (date);

  while (n_params--)
    {
      g_free (params[n_params].name);
      g_free (params[n_params].description);
    }

  g_free (params);

  while (n_return_vals--)
    {
      g_free (return_vals[n_return_vals].name);
      g_free (return_vals[n_return_vals].description);
    }

  g_free (return_vals);

  return main_vbox;
}


/*  private functions  */

static GtkWidget *
gimp_proc_view_create_params (const GimpParamDef *params,
                              gint                n_params,
                              GtkSizeGroup       *name_group,
                              GtkSizeGroup       *type_group,
                              GtkSizeGroup       *desc_group)
{
  GtkWidget *grid;
  gint       i;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);

  for (i = 0; i < n_params; i++)
    {
      GtkWidget   *label;
      const gchar *type;
      gchar       *upper;

      /* name */
      label = gtk_label_new (params[i].name);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_size_group_add_widget (name_group, label);
      gtk_grid_attach (GTK_GRID (grid), label, 0, i, 1, 1);
      gtk_widget_show (label);

      /* type */
      if (! gimp_enum_get_value (GIMP_TYPE_PDB_ARG_TYPE, params[i].type,
                                 NULL, &type, NULL, NULL))
        upper = g_strdup ("UNKNOWN");
      else
        upper = g_ascii_strup (type, -1);

      label = gtk_label_new (upper);
      g_free (upper);

      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_FAMILY, "monospace",
                                 PANGO_ATTR_STYLE,  PANGO_STYLE_ITALIC,
                                 -1);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_size_group_add_widget (type_group, label);
      gtk_grid_attach (GTK_GRID (grid), label, 1, i, 1, 1);
      gtk_widget_show (label);

      /* description */
      label = gtk_label_new (params[i].description);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_size_group_add_widget (desc_group, label);
      gtk_grid_attach (GTK_GRID (grid), label, 2, i, 1, 1);
      gtk_widget_show (label);
    }

  return grid;
}

static GtkWidget *
gimp_proc_view_create_args (const gchar  *procedure_name,
                            gint          n_args,
                            gboolean      return_values,
                            GtkSizeGroup *name_group,
                            GtkSizeGroup *type_group,
                            GtkSizeGroup *desc_group)
{
  GtkWidget *grid;
  gint       i;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);

  for (i = 0; i < n_args; i++)
    {
      GParamSpec *pspec;
      GtkWidget  *label;

      if (return_values)
        pspec = gimp_pdb_proc_return_value (procedure_name, i);
      else
        pspec = gimp_pdb_proc_argument (procedure_name, i);

      /* name */
      label = gtk_label_new (g_param_spec_get_name (pspec));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_size_group_add_widget (name_group, label);
      gtk_grid_attach (GTK_GRID (grid), label, 0, i, 1, 1);
      gtk_widget_show (label);

      /* type */
      label = gtk_label_new (g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_FAMILY, "monospace",
                                 PANGO_ATTR_STYLE,  PANGO_STYLE_ITALIC,
                                 -1);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_size_group_add_widget (type_group, label);
      gtk_grid_attach (GTK_GRID (grid), label, 1, i, 1, 1);
      gtk_widget_show (label);

      /* description */
      label = gtk_label_new (g_param_spec_get_blurb (pspec));
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_size_group_add_widget (desc_group, label);
      gtk_grid_attach (GTK_GRID (grid), label, 2, i, 1, 1);
      gtk_widget_show (label);

      g_param_spec_unref (pspec);
    }

  return grid;
}
