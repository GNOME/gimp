/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfontselection-dialog.c
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * Partially based on code from GtkFontSelection.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpfontselection.h"
#include "gimpfontselection-dialog.h"

#include "libgimp/gimpintl.h"


#define  FONT_LIST_HEIGHT       136
#define  FONT_LIST_WIDTH        190
#define  FONT_STYLE_LIST_WIDTH  170


static void   gimp_font_selection_dialog_class_init (GimpFontSelectionDialogClass *klass);
static void   gimp_font_selection_dialog_init       (GimpFontSelectionDialog      *dialog);
static void   gimp_font_selection_dialog_finalize   (GObject                      *object);

static void   gimp_font_selection_dialog_select_font  (GtkWidget      *widget,
                                                       gint            row,
                                                       gint            column,
                                                       GdkEventButton *bevent,
                                                       gpointer        data);
static void   gimp_font_selection_dialog_select_style (GtkWidget      *widget,
                                                       gint            row,
                                                       gint            column,
                                                       GdkEventButton *bevent,
                                                       gpointer        data);
static void   gimp_font_selection_dialog_response     (GimpFontSelectionDialog      *dialog,
                                                       gint                          response_id,
                                                       gpointer                      data);

static void   gimp_font_selection_dialog_show_available_fonts  (GimpFontSelectionDialog *dialog);
static void   gimp_font_selection_dialog_show_available_styles (GimpFontSelectionDialog *dialog);


static GtkDialogClass *parent_class = NULL;


GType
gimp_font_selection_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (!dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpFontSelectionDialogClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_font_selection_dialog_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpFontSelectionDialog),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_font_selection_dialog_init,
      };

      dialog_type = g_type_register_static (GTK_TYPE_DIALOG, 
                                            "GimpFontSelectionDialog", 
                                             &dialog_info, 0);
    }
  
  return dialog_type;
}

static void
gimp_font_selection_dialog_class_init (GimpFontSelectionDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gimp_font_selection_dialog_finalize;
}

static void
gimp_font_selection_dialog_init (GimpFontSelectionDialog *dialog)
{
  GtkWidget *scrolled_win;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *button;

  /* Create the table of font & style. */
  table = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    
  /* Create the clists  */
  dialog->font_clist = gtk_clist_new (1);
  gtk_clist_column_titles_hide (GTK_CLIST (dialog->font_clist));
  gtk_clist_set_column_auto_resize (GTK_CLIST (dialog->font_clist), 0, TRUE);
  GTK_WIDGET_SET_FLAGS (dialog->font_clist, GTK_CAN_FOCUS);
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolled_win, FONT_LIST_WIDTH, FONT_LIST_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_win), dialog->font_clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show (dialog->font_clist);
  gtk_widget_show (scrolled_win);

  gtk_table_attach (GTK_TABLE (table), scrolled_win, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  
  dialog->font_style_clist = gtk_clist_new (1);
  gtk_clist_column_titles_hide (GTK_CLIST (dialog->font_style_clist));
  gtk_clist_set_column_auto_resize (GTK_CLIST (dialog->font_style_clist),
				    0, TRUE);
  GTK_WIDGET_SET_FLAGS (dialog->font_style_clist, GTK_CAN_FOCUS);
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolled_win, FONT_STYLE_LIST_WIDTH, -1);
  gtk_container_add (GTK_CONTAINER (scrolled_win), dialog->font_style_clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show (dialog->font_style_clist);
  gtk_widget_show (scrolled_win);
  gtk_table_attach (GTK_TABLE (table), scrolled_win, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);

  label = gtk_label_new_with_mnemonic (_("_Family:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->font_clist);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, 0, 0, 0);  
  label = gtk_label_new_with_mnemonic (_("_Style:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                 dialog->font_style_clist);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, 
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  g_signal_connect (G_OBJECT (dialog->font_clist), "select_row",
                    G_CALLBACK (gimp_font_selection_dialog_select_font),
                    dialog);
  
  g_signal_connect (G_OBJECT (dialog->font_style_clist), "select_row",
                    G_CALLBACK (gimp_font_selection_dialog_select_style),
                    dialog);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), 
                                  GTK_STOCK_OK, GTK_RESPONSE_OK);
  gtk_widget_grab_default (button);
  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  GTK_STOCK_APPLY, GTK_RESPONSE_APPLY);
  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  g_signal_connect (GTK_DIALOG (dialog), "response",
                    G_CALLBACK (gimp_font_selection_dialog_response),
                    NULL);
                    
}

static void
gimp_font_selection_dialog_finalize (GObject *object)
{
  GimpFontSelectionDialog *dialog = GIMP_FONT_SELECTION_DIALOG (object);

  if (dialog->font_desc)
    pango_font_description_free (dialog->font_desc);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}


/**
 * gimp_font_selection_dialog_new:
 * @fontsel: the GimpFontSelection that creates and owns this dialog.
 *
 * Creates a new #GimpFontSelectionDialog.
 *
 * Returns: A pointer to the new #GimpFontSelectionDialog.
 **/
GtkWidget *
gimp_font_selection_dialog_new (GimpFontSelection *fontsel)
{
  GimpFontSelectionDialog *dialog;

  g_return_val_if_fail (GIMP_IS_FONT_SELECTION (fontsel), NULL);

  dialog= g_object_new (GIMP_TYPE_FONT_SELECTION_DIALOG, NULL);
  dialog->fontsel = fontsel;

  if (fontsel->font_desc)
    dialog->font_desc = pango_font_description_copy (fontsel->font_desc);
  else
    dialog->font_desc = pango_font_description_from_string ("");

  gtk_window_set_title (GTK_WINDOW (dialog), _("GIMP Font Selection"));  

  gimp_font_selection_dialog_show_available_fonts (dialog);  
  gimp_font_selection_dialog_show_available_styles (dialog);

  return GTK_WIDGET (dialog);
}

void
gimp_font_selection_dialog_set_font_desc (GimpFontSelectionDialog *dialog,
                                          PangoFontDescription    *new_desc)
{
  g_return_if_fail (GIMP_IS_FONT_SELECTION_DIALOG (dialog));
 
  if (dialog->font_desc)
    pango_font_description_free (dialog->font_desc);
  
  dialog->font_desc = new_desc ? pango_font_description_copy (new_desc) : NULL;
  
  gimp_font_selection_dialog_show_available_fonts (dialog);  
  gimp_font_selection_dialog_show_available_styles (dialog);
}

static void
gimp_font_selection_dialog_select_font (GtkWidget      *widget,
                                        gint	         row,
                                        gint	         column,
                                        GdkEventButton *bevent,
                                        gpointer        data)
{
  GimpFontSelectionDialog *dialog;
  gchar *family_name;
  gint   index;

  dialog = GIMP_FONT_SELECTION_DIALOG (data);

  if (GTK_CLIST (dialog->font_clist)->selection)
    {
      index = GPOINTER_TO_INT (GTK_CLIST (dialog->font_clist)->selection->data);

      if (gtk_clist_get_text (GTK_CLIST (dialog->font_clist), index, 0, &family_name) &&
	  (!dialog->font_desc->family_name || 
           strcasecmp (dialog->font_desc->family_name, family_name) != 0))
	{
          g_free (dialog->font_desc->family_name);
          dialog->font_desc->family_name = g_strdup (family_name);

	  gimp_font_selection_dialog_show_available_styles (dialog);

          gtk_clist_select_row (GTK_CLIST (dialog->font_style_clist), 0, 0);
          if (gtk_clist_row_is_visible (GTK_CLIST (dialog->font_style_clist), 0) != GTK_VISIBILITY_FULL)
            gtk_clist_moveto (GTK_CLIST (dialog->font_style_clist), 0, -1,
                              0.5, 0);
	}
    }
} 

static void
gimp_font_selection_dialog_select_style (GtkWidget      *widget,
                                         gint	         row,
                                         gint	         column,
                                         GdkEventButton *bevent,
                                         gpointer        data)
{
  GimpFontSelectionDialog *dialog;
  PangoFontDescription    *tmp_desc;
  gchar *style_name;
  gint   index;

  dialog    = GIMP_FONT_SELECTION_DIALOG (data);

  if (GTK_CLIST (dialog->font_style_clist)->selection)
    {
      index = GPOINTER_TO_INT (GTK_CLIST (dialog->font_style_clist)->selection->data);

      if (gtk_clist_get_text (GTK_CLIST (dialog->font_style_clist), index, 0, &style_name))
	{
          tmp_desc = pango_font_description_from_string (style_name);
  
          dialog->font_desc->style   = tmp_desc->style;
          dialog->font_desc->variant = tmp_desc->variant;
          dialog->font_desc->weight  = tmp_desc->weight;
          dialog->font_desc->stretch = tmp_desc->stretch;
          
          pango_font_description_free (tmp_desc);
	}
    }
}     

static void
gimp_font_selection_dialog_response (GimpFontSelectionDialog *dialog,
                                     gint                     response_id,
                                     gpointer                 data)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      gtk_widget_hide (GTK_WIDGET (dialog));      
      /* fallthrough */
    case GTK_RESPONSE_APPLY:
      gimp_font_selection_set_font_desc (dialog->fontsel, dialog->font_desc);
      break;
    case GTK_RESPONSE_CLOSE:
      gtk_widget_hide (GTK_WIDGET (dialog));
      break;
    default:
      break;
    }
}

static int
compare_strings (gconstpointer a, 
                 gconstpointer b)
{
  return strcasecmp (*(const char **)a, *(const char **)b);
}

static void
gimp_font_selection_dialog_show_available_fonts (GimpFontSelectionDialog *dialog)
{
  gchar **families;
  gint    n_families, i;
  gint    selected = -1;

  pango_context_list_families (dialog->fontsel->context,
			       &families, &n_families);
  qsort (families, n_families, sizeof (gchar *), compare_strings);

  gtk_clist_freeze (GTK_CLIST (dialog->font_clist));
  gtk_clist_clear (GTK_CLIST (dialog->font_clist));

  for (i=0; i<n_families; i++)
    {
      gtk_clist_append (GTK_CLIST (dialog->font_clist), &families[i]);

      if (dialog->font_desc->family_name &&
          strcasecmp (families[i], dialog->font_desc->family_name) == 0)
        {
          selected = i;
          gtk_clist_select_row (GTK_CLIST (dialog->font_clist), i, 0);
        }
    }
  
  gtk_clist_thaw (GTK_CLIST (dialog->font_clist));

  if (selected != -1 &&
      gtk_clist_row_is_visible (GTK_CLIST (dialog->font_clist), selected) != GTK_VISIBILITY_FULL)
    gtk_clist_moveto (GTK_CLIST (dialog->font_clist), selected, -1, 0.5, 0);

  pango_font_map_free_families (families, n_families);  
}
  
static int
compare_font_descriptions (gconstpointer a_ptr, 
                           gconstpointer b_ptr)
{
  PangoFontDescription *a = (PangoFontDescription *) a_ptr;
  PangoFontDescription *b = (PangoFontDescription *) b_ptr;
  gint val;

  val = strcasecmp (a->family_name, b->family_name);

  if (val != 0)
    return val;

  if (a->weight != b->weight)
    return a->weight - b->weight;

  if (a->style != b->style)
    return a->style - b->style;
  
  if (a->stretch != b->stretch)
    return a->stretch - b->stretch;

  if (a->variant != b->variant)
    return a->variant - b->variant;

  return 0;
}

static void
gimp_font_selection_dialog_show_available_styles (GimpFontSelectionDialog *dialog)
{
  PangoFontDescription **descs;
  gint n_descs, i;
  gint match_row = -1;
  gchar *str;
  
  pango_context_list_fonts (dialog->fontsel->context,
                            dialog->font_desc->family_name, 
                            &descs, &n_descs);
  qsort (descs, n_descs, sizeof (PangoFontDescription *), 
         compare_font_descriptions);

  gtk_clist_freeze (GTK_CLIST (dialog->font_style_clist));
  gtk_clist_clear (GTK_CLIST (dialog->font_style_clist));

  for (i=0; i<n_descs; i++)
    {
      PangoFontDescription tmp_desc;

      tmp_desc = *descs[i];
      tmp_desc.family_name = NULL;
      tmp_desc.size = 0;

      str = pango_font_description_to_string (&tmp_desc);
      gtk_clist_append (GTK_CLIST (dialog->font_style_clist), &str);

      if (descs[i]->weight  == dialog->font_desc->weight  &&
	  descs[i]->style   == dialog->font_desc->style   &&
	  descs[i]->stretch == dialog->font_desc->stretch &&
	  descs[i]->variant == dialog->font_desc->variant)
	match_row = i;
      
      g_free (str);
    }

  gtk_clist_select_row (GTK_CLIST (dialog->font_style_clist), match_row, 0);
  gtk_clist_thaw (GTK_CLIST (dialog->font_style_clist));

  pango_font_descriptions_free (descs, n_descs);
}  
