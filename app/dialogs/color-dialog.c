/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color_notebook module (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimpviewable.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpviewabledialog.h"

#include "color-history.h"
#include "color-notebook.h"

#include "gimp-intl.h"


#define RESPONSE_RESET   1
#define COLOR_AREA_SIZE 20


struct _ColorNotebook
{
  GtkWidget             *shell;
  GtkWidget             *selection;

  GtkWidget             *history[COLOR_HISTORY_SIZE];

  ColorNotebookCallback  callback;
  gpointer               client_data;

  gboolean               wants_updates;
};


static void   color_notebook_help_func     (const gchar           *help_id,
                                            gpointer               help_data);

static void   color_notebook_response      (GtkWidget             *widget,
                                            gint                   response_id,
                                            ColorNotebook         *cnp);

static void   color_notebook_color_changed (GimpColorSelection    *selection,
                                            ColorNotebook         *cnp);

static void   color_history_color_clicked  (GtkWidget             *widget,
                                            ColorNotebook         *cnp);
static void   color_history_color_changed  (GtkWidget             *widget,
                                            gpointer               data);
static void   color_history_add_clicked    (GtkWidget             *widget,
                                            ColorNotebook         *cnp);


static GList *color_notebooks = NULL;


/*  public functions  */

ColorNotebook *
color_notebook_new (GimpViewable          *viewable,
                    const gchar           *title,
                    const gchar           *stock_id,
                    const gchar           *desc,
                    GtkWidget             *parent,
                    GimpDialogFactory     *dialog_factory,
                    const gchar           *dialog_identifier,
                    const GimpRGB         *color,
                    ColorNotebookCallback  callback,
                    gpointer               client_data,
                    gboolean               wants_updates,
                    gboolean               show_alpha)
{
  ColorNotebook *cnp;
  GtkWidget     *table;
  GtkWidget     *button;
  GtkWidget     *arrow;
  const gchar   *role;
  gint           i;

  g_return_val_if_fail (viewable == NULL || GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (dialog_factory == NULL || dialog_identifier != NULL,
                        NULL);
  g_return_val_if_fail (color != NULL, NULL);

  cnp = g_new0 (ColorNotebook, 1);

  cnp->callback      = callback;
  cnp->client_data   = client_data;
  cnp->wants_updates = wants_updates;

  role = dialog_identifier ? dialog_identifier : "gimp-color-selector";

  if (desc)
    {
      cnp->shell = gimp_viewable_dialog_new (viewable, title, role,
                                             stock_id, desc,
                                             parent,
                                             color_notebook_help_func, NULL,
                                             NULL);

      gtk_window_set_resizable (GTK_WINDOW (cnp->shell), FALSE);
    }
  else
    {
      cnp->shell = gimp_dialog_new (title, role,
                                    parent, 0,
                                    color_notebook_help_func, NULL,
                                    NULL);
   }

  g_object_set_data (G_OBJECT (cnp->shell), "color-notebook", cnp);

  gtk_dialog_add_buttons (GTK_DIALOG (cnp->shell),
                          GIMP_STOCK_RESET, RESPONSE_RESET,
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK,     GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (cnp->shell), GTK_RESPONSE_OK);

  g_signal_connect (cnp->shell, "response",
                    G_CALLBACK (color_notebook_response),
                    cnp);

  g_object_add_weak_pointer (G_OBJECT (cnp->shell), (gpointer *) &cnp->shell);

  if (dialog_factory)
    gimp_dialog_factory_add_foreign (dialog_factory, dialog_identifier,
                                     cnp->shell);

  cnp->selection = gimp_color_selection_new ();
  gtk_container_set_border_width (GTK_CONTAINER (cnp->selection), 12);
  gimp_color_selection_set_show_alpha (GIMP_COLOR_SELECTION (cnp->selection),
                                       show_alpha);
  gimp_color_selection_set_color (GIMP_COLOR_SELECTION (cnp->selection), color);
  gimp_color_selection_set_old_color (GIMP_COLOR_SELECTION (cnp->selection),
                                      color);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (cnp->shell)->vbox),
                     cnp->selection);
  gtk_widget_show (cnp->selection);

  g_signal_connect (cnp->selection, "color_changed",
                    G_CALLBACK (color_notebook_color_changed),
                    cnp);

  /* The color history */
  table = gtk_table_new (2, 1 + COLOR_HISTORY_SIZE / 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_end (GTK_BOX (GIMP_COLOR_SELECTION (cnp->selection)->right_vbox),
                    table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  button = gtk_button_new ();
  gtk_widget_set_size_request (button, COLOR_AREA_SIZE, COLOR_AREA_SIZE);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 0, 1);
  gimp_help_set_help_data (button,
			   _("Add the current color to the color history"),
			   NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
		    G_CALLBACK (color_history_add_clicked),
		    cnp);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (button), arrow);
  gtk_widget_show (arrow);

  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    {
      GimpRGB history_color;
      gint    row, column;

      color_history_get (i, &history_color);

      button = gtk_button_new ();
      gtk_widget_set_size_request (button, COLOR_AREA_SIZE, COLOR_AREA_SIZE);

      column = i % (COLOR_HISTORY_SIZE / 2);
      row    = i / (COLOR_HISTORY_SIZE / 2);

      gtk_table_attach_defaults (GTK_TABLE (table), button,
                                 column + 1, column + 2, row, row + 1);

      cnp->history[i] = gimp_color_area_new (&history_color,
					     GIMP_COLOR_AREA_SMALL_CHECKS,
					     GDK_BUTTON2_MASK);
      gtk_container_add (GTK_CONTAINER (button), cnp->history[i]);
      gtk_widget_show (cnp->history[i]);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
			G_CALLBACK (color_history_color_clicked),
			cnp);

      g_signal_connect (cnp->history[i], "color_changed",
			G_CALLBACK (color_history_color_changed),
			GINT_TO_POINTER (i));
    }

  gtk_widget_show (cnp->shell);

  color_notebooks = g_list_prepend (color_notebooks, cnp);

  return cnp;
}

void
color_notebook_free (ColorNotebook *cnp)
{
  g_return_if_fail (cnp != NULL);

  color_notebooks = g_list_remove (color_notebooks, cnp);

  /*  may be already destroyed by dialog factory  */
  if (cnp->shell)
    {
      g_object_remove_weak_pointer (G_OBJECT (cnp->shell),
                                    (gpointer *) &cnp->shell);
      gtk_widget_destroy (cnp->shell);
    }

  g_free (cnp);
}

void
color_notebook_set_viewable (ColorNotebook *cnb,
                             GimpViewable  *viewable)
{
  g_return_if_fail (cnb != NULL);

  if (GIMP_IS_VIEWABLE_DIALOG (cnb->shell))
    gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (cnb->shell),
                                       viewable);
}

void
color_notebook_set_title (ColorNotebook *cnb,
                          const gchar   *title)
{
  g_return_if_fail (cnb != NULL);
  g_return_if_fail (title != NULL);

  gtk_window_set_title (GTK_WINDOW (cnb->shell), title);
}

void
color_notebook_set_color (ColorNotebook *cnp,
			  const GimpRGB *color)
{
  g_return_if_fail (cnp != NULL);
  g_return_if_fail (color != NULL);

  g_signal_handlers_block_by_func (cnp->selection,
                                   color_notebook_color_changed,
                                   cnp);

  gimp_color_selection_set_color (GIMP_COLOR_SELECTION (cnp->selection), color);
  gimp_color_selection_set_old_color (GIMP_COLOR_SELECTION (cnp->selection),
                                      color);

  g_signal_handlers_unblock_by_func (cnp->selection,
                                     color_notebook_color_changed,
                                     cnp);
}

void
color_notebook_get_color (ColorNotebook *cnp,
			  GimpRGB       *color)
{
  g_return_if_fail (cnp != NULL);
  g_return_if_fail (color != NULL);

  gimp_color_selection_get_color (GIMP_COLOR_SELECTION (cnp->selection), color);
}

void
color_notebook_show (ColorNotebook *cnp)
{
  g_return_if_fail (cnp != NULL);

  gtk_window_present (GTK_WINDOW (cnp->shell));
}

void
color_notebook_hide (ColorNotebook *cnp)
{
  g_return_if_fail (cnp != NULL);

  gtk_widget_hide (cnp->shell);
}


/*  private functions  */

static void
color_notebook_help_func (const gchar *help_id,
                          gpointer     help_data)
{
  ColorNotebook     *cnp;
  GimpColorNotebook *notebook;

  cnp = g_object_get_data (G_OBJECT (help_data), "color-notebook");

  notebook =
    GIMP_COLOR_NOTEBOOK (GIMP_COLOR_SELECTION (cnp->selection)->notebook);

  help_id = GIMP_COLOR_SELECTOR_GET_CLASS (notebook->cur_page)->help_id;

  gimp_standard_help_func (help_id, NULL);
}

static void
color_notebook_response (GtkWidget     *widget,
                         gint           response_id,
                         ColorNotebook *cnp)
{
  GimpRGB color;

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_color_selection_reset (GIMP_COLOR_SELECTION (cnp->selection));
      break;

    case GTK_RESPONSE_OK:
      color_history_add_clicked (NULL, cnp);

      gimp_color_selection_get_color (GIMP_COLOR_SELECTION (cnp->selection),
                                      &color);

      if (cnp->callback)
        cnp->callback (cnp, &color,
                       COLOR_NOTEBOOK_OK,
                       cnp->client_data);
      break;

    default:
      gimp_color_selection_get_old_color (GIMP_COLOR_SELECTION (cnp->selection),
                                          &color);

      if (cnp->callback)
        cnp->callback (cnp, &color,
                       COLOR_NOTEBOOK_CANCEL,
                       cnp->client_data);
      break;
    }
}

static void
color_notebook_color_changed (GimpColorSelection *selection,
                              ColorNotebook      *cnp)
{
  GimpRGB color;

  gimp_color_selection_get_color (selection, &color);

  if (cnp->wants_updates && cnp->callback)
    cnp->callback (cnp,
                   &color,
                   COLOR_NOTEBOOK_UPDATE,
                   cnp->client_data);
}


/*  color history callbacks  */

static void
color_history_color_clicked (GtkWidget     *widget,
			     ColorNotebook *cnp)
{
  GimpColorArea *color_area;
  GimpRGB        color;

  color_area = GIMP_COLOR_AREA (GTK_BIN (widget)->child);

  gimp_color_area_get_color (color_area, &color);
  gimp_color_selection_set_color (GIMP_COLOR_SELECTION (cnp->selection),
                                  &color);
}

static void
color_history_color_changed (GtkWidget *widget,
			     gpointer   data)
{
  GimpRGB  changed_color;
  gint     color_index;
  GList   *list;

  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &changed_color);

  color_index = GPOINTER_TO_INT (data);

  color_history_set (color_index, &changed_color);

  for (list = color_notebooks; list; list = g_list_next (list))
    {
      ColorNotebook *notebook = list->data;

      if (notebook->history[color_index] != widget)
        {
          g_signal_handlers_block_by_func (notebook->history[color_index],
                                           color_history_color_changed,
                                           data);

          gimp_color_area_set_color
            (GIMP_COLOR_AREA (notebook->history[color_index]), &changed_color);

          g_signal_handlers_unblock_by_func (notebook->history[color_index],
                                             color_history_color_changed,
                                             data);
        }
    }
}

static void
color_history_add_clicked (GtkWidget     *widget,
			   ColorNotebook *cnp)
{
  GimpRGB color;
  gint    shift_begin;
  gint    i;

  gimp_color_selection_get_color (GIMP_COLOR_SELECTION (cnp->selection),
                                  &color);

  shift_begin = color_history_add (&color);

  for (i = shift_begin; i >= 0; i--)
    {
      color_history_get (i, &color);

      gimp_color_area_set_color (GIMP_COLOR_AREA (cnp->history[i]), &color);
    }
}
