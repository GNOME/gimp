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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"
#include "gimpparamspecs-desc.h"

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

static gint        gimp_proc_view_add_label   (GtkGrid       *grid,
                                               gint           row,
                                               const gchar   *left_label,
                                               const gchar   *right_label);
static GtkWidget * gimp_proc_view_create_args (GimpProcedure *procedure,
                                               gboolean       return_values,
                                               GtkSizeGroup  *name_group,
                                               GtkSizeGroup  *type_group,
                                               GtkSizeGroup  *desc_group);


/*  public functions  */


/**
 * gimp_proc_view_new:
 * @procedure_name: The name of a procedure.
 *
 * Returns: (transfer full): a new widget providing a view on a
 *          GIMP procedure
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_proc_view_new (const gchar *procedure_name)
{
  GimpProcedure   *procedure;
  GtkWidget       *main_vbox;
  GtkWidget       *frame;
  GtkWidget       *vbox;
  GtkWidget       *grid;
  GtkWidget       *label;
  GtkSizeGroup    *name_group;
  GtkSizeGroup    *type_group;
  GtkSizeGroup    *desc_group;
  const gchar     *blurb;
  const gchar     *help;
  const gchar     *help_id;
  const gchar     *authors;
  const gchar     *copyright;
  const gchar     *date;
  GimpPDBProcType  type;
  const gchar     *type_str;
  gint             row;

  g_return_val_if_fail (gimp_is_canonical_identifier (procedure_name), NULL);

  procedure = gimp_pdb_lookup_procedure (gimp_get_pdb (),
                                         procedure_name);

  type      = gimp_procedure_get_proc_type (procedure);
  blurb     = gimp_procedure_get_blurb     (procedure);
  help      = gimp_procedure_get_help      (procedure);
  help_id   = gimp_procedure_get_help_id   (procedure);
  authors   = gimp_procedure_get_authors   (procedure);
  copyright = gimp_procedure_get_copyright (procedure);
  date      = gimp_procedure_get_date      (procedure);

  if (blurb     && strlen (blurb) < 2)     blurb     = NULL;
  if (help      && strlen (help) < 2)      help      = NULL;
  if (help_id   && strlen (help_id) < 2)   help_id   = NULL;
  if (authors   && strlen (authors) < 2)   authors   = NULL;
  if (copyright && strlen (copyright) < 2) copyright = NULL;
  if (date      && strlen (date) < 2)      date      = NULL;

  if (blurb && help && ! strcmp (blurb, help))
    help = NULL;

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

  if (blurb)
    {
      label = gtk_label_new (blurb);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
    }

  if (type != GIMP_PDB_PROC_TYPE_INTERNAL)
    {
      GList *list;

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
      gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
      gtk_widget_show (grid);

      row = 0;

      row = gimp_proc_view_add_label (GTK_GRID (grid), row,
                                      _("Image types:"),
                                      gimp_procedure_get_image_types (procedure));
      row = gimp_proc_view_add_label (GTK_GRID (grid), row,
                                      _("Menu label:"),
                                      gimp_procedure_get_menu_label (procedure));

      for (list = gimp_procedure_get_menu_paths (procedure);
           list;
           list = g_list_next (list))
        {
          row = gimp_proc_view_add_label (GTK_GRID (grid), row,
                                          _("Menu path:"),
                                          list->data);
        }
    }

  name_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  type_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  desc_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* in parameters */
  grid = gimp_proc_view_create_args (procedure, FALSE,
                                     name_group, type_group, desc_group);

  if (grid)
    {
      frame = gimp_frame_new (_("Parameters"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);
    }

  /* out parameters */
  grid = gimp_proc_view_create_args (procedure, TRUE,
                                     name_group, type_group, desc_group);

  if (grid)
    {
      frame = gimp_frame_new (_("Return Values"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);
    }

  g_object_unref (name_group);
  g_object_unref (type_group);
  g_object_unref (desc_group);

  if (! help && ! authors && ! date && ! copyright)
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

  /* show the authors & the copyright */

  if (authors || date || copyright)
    {
      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
      gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
      gtk_widget_show (grid);

      row = 0;

      row = gimp_proc_view_add_label (GTK_GRID (grid), row,
                                      _("Authors:"),
                                      authors);
      row = gimp_proc_view_add_label (GTK_GRID (grid), row,
                                      _("Date:"),
                                      date);
      row = gimp_proc_view_add_label (GTK_GRID (grid), row,
                                      _("Copyright:"),
                                      copyright);
    }

  return main_vbox;
}


/*  private functions  */

static gint
gimp_proc_view_add_label (GtkGrid       *grid,
                          gint           row,
                          const gchar   *left_label,
                          const gchar   *right_label)
{
  if (right_label && strlen (right_label))
    {
      GtkWidget *label;

      label = gtk_label_new (right_label);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

      gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                left_label, 0.0, 0.0,
                                label, 3);
    }

  return row;
}

static GtkWidget *
gimp_proc_view_create_args (GimpProcedure *procedure,
                            gboolean       return_values,
                            GtkSizeGroup  *name_group,
                            GtkSizeGroup  *type_group,
                            GtkSizeGroup  *desc_group)
{
  GtkWidget   *grid;
  GParamSpec **pspecs;
  gint         n_pspecs;
  gint         i;

  if (return_values)
    pspecs = gimp_procedure_get_return_values (procedure, &n_pspecs);
  else
    pspecs = gimp_procedure_get_arguments (procedure, &n_pspecs);

  if (! pspecs)
    return NULL;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GtkWidget  *label;
      gchar      *desc;
      gchar      *blurb;

      desc = gimp_param_spec_get_desc (pspec);

      if (desc)
        {
          blurb = g_strconcat (g_param_spec_get_blurb (pspec), " ", desc, NULL);
          g_free (desc);
        }
      else
        {
          blurb = g_strdup (g_param_spec_get_blurb (pspec));
        }

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
      label = gtk_label_new (blurb);
      gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
      gtk_label_set_selectable (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_size_group_add_widget (desc_group, label);
      gtk_grid_attach (GTK_GRID (grid), label, 2, i, 1, 1);
      gtk_widget_show (label);

      g_free (blurb);
    }

  return grid;
}
