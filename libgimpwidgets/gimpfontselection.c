/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfontselection.c
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

#include <glib.h>
#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpfontselection.h"

#include "libgimp/libgimp-intl.h"


#define  FONT_LIST_HEIGHT       136
#define  FONT_LIST_WIDTH        190
#define  FONT_STYLE_LIST_WIDTH  170
#define  DEFAULT_FONT           "sans 24"


static void   gimp_font_selection_class_init   (GimpFontSelectionClass *klass);
static void   gimp_font_selection_init         (GimpFontSelection      *fontsel);
static void   gimp_font_selection_finalize     (GObject        *object);

static void   gimp_font_selection_select_font  (GtkWidget      *widget,
                                                gint	       row,
                                                gint	       column,
                                                GdkEventButton *bevent,
                                                gpointer        data);
static void   gimp_font_selection_select_style (GtkWidget      *widget,
                                                gint	        row,
                                                gint	        column,
                                                GdkEventButton *bevent,
                                                gpointer       data);

static void   gimp_font_selection_show_available_fonts  (GimpFontSelection *fontsel);
static void   gimp_font_selection_show_available_styles (GimpFontSelection *fontsel);


enum
{
  FONT_CHANGED,
  LAST_SIGNAL
};

static guint gimp_font_selection_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;


GType
gimp_font_selection_get_type (void)
{
  static GType fontsel_type = 0;

  if (!fontsel_type)
    {
      static const GTypeInfo fontsel_info =
      {
        sizeof (GimpFontSelectionClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_font_selection_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpFontSelection),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_font_selection_init,
      };

      fontsel_type = g_type_register_static (GTK_TYPE_VBOX, 
                                             "GimpFontSelection", 
                                             &fontsel_info, 0);
    }
  
  return fontsel_type;
}

static void
gimp_font_selection_class_init (GimpFontSelectionClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gimp_font_selection_signals[FONT_CHANGED] = 
    g_signal_new ("font_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpFontSelectionClass,
				   font_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize = gimp_font_selection_finalize;

  klass->font_changed = NULL;
}

static void
gimp_font_selection_init (GimpFontSelection *fontsel)
{
  GtkWidget *scrolled_win;
  GtkWidget *table;
  GtkWidget *label;

  fontsel->context   = NULL;
  fontsel->font_desc = pango_font_description_from_string (DEFAULT_FONT);

  /* Create the table of font & style. */
  table = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (fontsel), table, TRUE, TRUE, 0);
    
  /* Create the clists  */
  fontsel->font_clist = gtk_clist_new (1);
  gtk_clist_column_titles_hide (GTK_CLIST (fontsel->font_clist));
  gtk_clist_set_column_auto_resize (GTK_CLIST (fontsel->font_clist), 0, TRUE);
  GTK_WIDGET_SET_FLAGS (fontsel->font_clist, GTK_CAN_FOCUS);
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolled_win, FONT_LIST_WIDTH, FONT_LIST_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_win), fontsel->font_clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show (fontsel->font_clist);
  gtk_widget_show (scrolled_win);

  gtk_table_attach (GTK_TABLE (table), scrolled_win, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  
  fontsel->font_style_clist = gtk_clist_new (1);
  gtk_clist_column_titles_hide (GTK_CLIST (fontsel->font_style_clist));
  gtk_clist_set_column_auto_resize (GTK_CLIST (fontsel->font_style_clist),
				    0, TRUE);
  GTK_WIDGET_SET_FLAGS (fontsel->font_style_clist, GTK_CAN_FOCUS);
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (scrolled_win, FONT_STYLE_LIST_WIDTH, -1);
  gtk_container_add (GTK_CONTAINER (scrolled_win), fontsel->font_style_clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show (fontsel->font_style_clist);
  gtk_widget_show (scrolled_win);
  gtk_table_attach (GTK_TABLE (table), scrolled_win, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);

  label = gtk_label_new_with_mnemonic (_("_Family:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), fontsel->font_clist);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, 0, 0, 0);  
  label = gtk_label_new_with_mnemonic (_("_Style:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                 fontsel->font_style_clist);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);
 
  g_signal_connect (G_OBJECT (fontsel->font_clist), "select_row",
                    G_CALLBACK (gimp_font_selection_select_font),
                    fontsel);
  
  g_signal_connect (G_OBJECT (fontsel->font_style_clist), "select_row",
                    G_CALLBACK (gimp_font_selection_select_style),
                    fontsel);
}

static void
gimp_font_selection_finalize (GObject *object)
{
  GimpFontSelection *fontsel;

  g_return_if_fail (GIMP_IS_FONT_SELECTION (object));

  fontsel = GIMP_FONT_SELECTION (object);

  if (fontsel->context)
    g_object_unref (G_OBJECT (fontsel->context));

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_font_selection_new:
 * @context: the #PangoContext to select a font from.
 *
 * Creates a new #GimpFontSelection widget.
 *
 * Returns: A pointer to the new #GimpFontSelection widget.
 **/
GtkWidget *
gimp_font_selection_new (PangoContext *context)
{
  GimpFontSelection *fontsel;

  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  fontsel = g_object_new (GIMP_TYPE_FONT_SELECTION, NULL);

  fontsel->context = context;
  g_object_ref (G_OBJECT (fontsel->context));

  gimp_font_selection_show_available_fonts (fontsel);  
  gimp_font_selection_show_available_styles (fontsel);
  
  return GTK_WIDGET (fontsel);
}

gboolean
gimp_font_selection_set_fontname (GimpFontSelection *fontsel,
                                  const gchar       *fontname)
{
  PangoFontDescription *new_desc;
  PangoFontDescription **descs;
  gint n_descs, i;
  gboolean found = FALSE;

  g_return_val_if_fail (GIMP_IS_FONT_SELECTION (fontsel), FALSE);

  new_desc = pango_font_description_from_string (fontname);

  /* Check to make sure that this is in the list of allowed fonts */

  pango_context_list_fonts (fontsel->context,
			    new_desc->family_name, &descs, &n_descs);
  
  for (i=0; i<n_descs; i++)
    {
      if (descs[i]->weight  == new_desc->weight  &&
	  descs[i]->style   == new_desc->style   &&
	  descs[i]->stretch == new_desc->stretch &&
	  descs[i]->variant == new_desc->variant)
	{
	  found = TRUE;
	  break;
	}
    }

  pango_font_descriptions_free (descs, n_descs);

  if (!found)
    return FALSE;

  pango_font_description_free (fontsel->font_desc);
  fontsel->font_desc = new_desc;

  g_signal_emit (G_OBJECT (fontsel), 
                 gimp_font_selection_signals[FONT_CHANGED], 0);

  return TRUE;  
}

const gchar *
gimp_font_selection_get_fontname (GimpFontSelection *fontsel)
{
  return pango_font_description_to_string (fontsel->font_desc);
}

static void
gimp_font_selection_select_font (GtkWidget      *widget,
                                 gint	         row,
                                 gint	         column,
                                 GdkEventButton *bevent,
                                 gpointer        data)
{
  GimpFontSelection *fontsel;
  gchar             *family_name;
  gint               index;

  fontsel = GIMP_FONT_SELECTION (data);

  if (GTK_CLIST (fontsel->font_clist)->selection)
    {
      index = GPOINTER_TO_INT (GTK_CLIST (fontsel->font_clist)->selection->data);

      if (gtk_clist_get_text (GTK_CLIST (fontsel->font_clist), index, 0, &family_name) &&
	  strcasecmp (fontsel->font_desc->family_name, family_name) != 0)
	{
	  g_free (fontsel->font_desc->family_name);
	  fontsel->font_desc->family_name  = g_strdup (family_name);
	  
	  gimp_font_selection_show_available_styles (fontsel);

          gtk_clist_select_row (GTK_CLIST (fontsel->font_style_clist), 0, 0);
          if (gtk_clist_row_is_visible (GTK_CLIST (fontsel->font_style_clist), 0) != GTK_VISIBILITY_FULL)
            gtk_clist_moveto (GTK_CLIST (fontsel->font_style_clist), 0, -1,
                              0.5, 0);

	}
    }
} 

static void
gimp_font_selection_select_style (GtkWidget      *widget,
                                  gint	          row,
                                  gint	          column,
                                  GdkEventButton *bevent,
                                  gpointer        data)
{
  GimpFontSelection    *fontsel;
  PangoFontDescription *tmp_desc;
  gchar                *style_name;
  gint                  index;

  fontsel = GIMP_FONT_SELECTION (data);

  if (GTK_CLIST (fontsel->font_style_clist)->selection)
    {
      index = GPOINTER_TO_INT (GTK_CLIST (fontsel->font_style_clist)->selection->data);

      if (gtk_clist_get_text (GTK_CLIST (fontsel->font_style_clist), index, 0, &style_name))
	{
	  tmp_desc = pango_font_description_from_string (style_name);
	  
	  fontsel->font_desc->style = tmp_desc->style;
	  fontsel->font_desc->variant = tmp_desc->variant;
	  fontsel->font_desc->weight = tmp_desc->weight;
	  fontsel->font_desc->stretch = tmp_desc->stretch;
	  
	  pango_font_description_free (tmp_desc);
	}
    }
}     

static int
compare_strings (gconstpointer a, 
                 gconstpointer b)
{
  return strcasecmp ((const char *) a, (const char *) b);
}

static void
gimp_font_selection_show_available_fonts (GimpFontSelection *fontsel)
{
  gchar **families;
  gint    n_families, i;

  pango_context_list_families (fontsel->context,
			       &families, &n_families);
  qsort (families, n_families, sizeof (gchar *), 
         compare_strings);

  gtk_clist_freeze (GTK_CLIST (fontsel->font_clist));
  gtk_clist_clear (GTK_CLIST (fontsel->font_clist));

  for (i=0; i<n_families; i++)
    {
      gtk_clist_append (GTK_CLIST (fontsel->font_clist), &families[i]);

      if (!strcasecmp (families[i], fontsel->font_desc->family_name))
        gtk_clist_select_row (GTK_CLIST (fontsel->font_clist), i, 0);
    }
  
  gtk_clist_thaw (GTK_CLIST (fontsel->font_clist));

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
gimp_font_selection_show_available_styles (GimpFontSelection *fontsel)
{
  PangoFontDescription **descs;
  gint n_descs, i;
  gint match_row = -1;
  gchar *str;
  
  pango_context_list_fonts (fontsel->context,
                            fontsel->font_desc->family_name, 
                            &descs, &n_descs);
  qsort (descs, n_descs, sizeof (PangoFontDescription *), 
         compare_font_descriptions);

  gtk_clist_freeze (GTK_CLIST (fontsel->font_style_clist));
  gtk_clist_clear (GTK_CLIST (fontsel->font_style_clist));

  for (i=0; i<n_descs; i++)
    {
      PangoFontDescription tmp_desc;

      tmp_desc = *descs[i];
      tmp_desc.family_name = NULL;
      tmp_desc.size = 0;

      str = pango_font_description_to_string (&tmp_desc);
      gtk_clist_append (GTK_CLIST (fontsel->font_style_clist), &str);

      if (descs[i]->weight == fontsel->font_desc->weight &&
	  descs[i]->style == fontsel->font_desc->style &&
	  descs[i]->stretch == fontsel->font_desc->stretch &&
	  descs[i]->variant == fontsel->font_desc->variant)
	match_row = i;
      
      g_free (str);
    }

  gtk_clist_select_row (GTK_CLIST (fontsel->font_style_clist), match_row, 0);
  
  gtk_clist_thaw (GTK_CLIST (fontsel->font_style_clist));

  pango_font_descriptions_free (descs, n_descs);
}  
