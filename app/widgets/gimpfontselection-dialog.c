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
#include <pango/pangoft2.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpfontselection.h"
#include "gimpfontselection-dialog.h"

#include "libgimp/gimpintl.h"


#define  FONT_LIST_HEIGHT       136
#define  FONT_LIST_WIDTH        190
#define  FONT_STYLE_LIST_WIDTH  170


struct _GimpFontSelectionDialog
{
  GimpFontSelection    *fontsel;
  PangoFontDescription *font_desc;

  GtkWidget            *dialog;

  GtkWidget            *font_clist;
  GtkWidget            *font_style_clist;

  GtkWidget            *preview;
  FT_Bitmap             bitmap;
  PangoLayout          *layout;
};


static void   gimp_font_selection_dialog_ok           (GtkWidget      *widget,
                                                       gpointer        data);
static void   gimp_font_selection_dialog_apply        (GtkWidget      *widget,
                                                       gpointer        data);
static void   gimp_font_selection_dialog_preview      (GimpFontSelectionDialog *dialog);
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

static void   gimp_font_selection_dialog_show_available_fonts  (GimpFontSelectionDialog *dialog);
static void   gimp_font_selection_dialog_show_available_styles (GimpFontSelectionDialog *dialog);
static gboolean  gimp_font_selection_dialog_preview_expose (GtkWidget      *widget,
                                                            GdkEventExpose *event,
                                                            gpointer        data);
static void      gimp_font_selection_dialog_preview_resize (GtkWidget      *widget,
                                                            GtkAllocation  *allocation,
                                                            gpointer        data);
static void      gimp_font_selection_dialog_preview        (GimpFontSelectionDialog *dialog);


/**
 * gimp_font_selection_dialog_new:
 * @fontsel: the GimpFontSelection that creates and owns this dialog.
 *
 * Creates a new #GimpFontSelectionDialog.
 *
 * Returns: A pointer to the new #GimpFontSelectionDialog.
 **/
GimpFontSelectionDialog *
gimp_font_selection_dialog_new (GimpFontSelection *fontsel)
{
  GimpFontSelectionDialog *dialog;

  GtkWidget *scrolled_win;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *frame;

  g_return_val_if_fail (GIMP_IS_FONT_SELECTION (fontsel), NULL);

  dialog = g_new0 (GimpFontSelectionDialog, 1);

  dialog->fontsel = fontsel;

  if (fontsel->font_desc)
    dialog->font_desc = pango_font_description_copy (fontsel->font_desc);
  else
    dialog->font_desc = pango_font_description_from_string ("");

  dialog->layout = pango_layout_new (fontsel->context);
  pango_layout_set_text (dialog->layout, 
                         "my mind is going ...", -1);

  dialog->dialog = 
    gimp_dialog_new (_("GIMP Font Selection"), "font_selection",
                     gimp_standard_help_func, "dialogs/font_selection.html",
                     GTK_WIN_POS_MOUSE,
                     FALSE, TRUE, TRUE,
                     
                     GTK_STOCK_OK, gimp_font_selection_dialog_ok,
                     dialog, NULL, NULL, TRUE, FALSE,
                     
                     GTK_STOCK_APPLY, gimp_font_selection_dialog_apply,
                     dialog, NULL, NULL, TRUE, FALSE,
                     
                     GTK_STOCK_CANCEL, gtk_widget_hide,
                     NULL, (gpointer) 1, NULL, FALSE, TRUE,
                     
                     NULL);

  gtk_container_set_border_width (GTK_CONTAINER (dialog->dialog), 1);

  /* Create the font preview */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox), frame, 
                      FALSE, FALSE, 0);
  gtk_widget_show (frame);
 
  dialog->preview = gtk_drawing_area_new ();
  gtk_widget_set_usize (dialog->preview, -1, 30);
  gtk_container_add (GTK_CONTAINER (frame), dialog->preview);
  gtk_widget_show (dialog->preview);
  g_signal_connect (G_OBJECT (dialog->preview), "size_allocate",
                    G_CALLBACK (gimp_font_selection_dialog_preview_resize),
                    dialog);
  g_signal_connect (G_OBJECT (dialog->preview), "expose_event",
                    G_CALLBACK (gimp_font_selection_dialog_preview_expose),
                    dialog);

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

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox), table, 
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  g_signal_connect (G_OBJECT (dialog->font_clist), "select_row",
                    G_CALLBACK (gimp_font_selection_dialog_select_font),
                    dialog);
  
  g_signal_connect (G_OBJECT (dialog->font_style_clist), "select_row",
                    G_CALLBACK (gimp_font_selection_dialog_select_style),
                    dialog);

  gimp_font_selection_dialog_show_available_fonts (dialog);  
  gimp_font_selection_dialog_show_available_styles (dialog);

  gimp_font_selection_dialog_preview (dialog);

  return dialog;
}

void 
gimp_font_selection_dialog_destroy (GimpFontSelectionDialog *dialog)
{
  g_return_if_fail (dialog != NULL);

  gtk_widget_destroy (dialog->dialog);
  g_free (dialog->bitmap.buffer);
  g_free (dialog);
}

void 
gimp_font_selection_dialog_show (GimpFontSelectionDialog *dialog)
{
  g_return_if_fail (dialog != NULL);
  
  gtk_widget_show (dialog->dialog);
}

void
gimp_font_selection_dialog_set_font_desc (GimpFontSelectionDialog *dialog,
                                          PangoFontDescription    *new_desc)
{
  g_return_if_fail (dialog != NULL);
 
  if (dialog->font_desc)
    pango_font_description_free (dialog->font_desc);
  
  dialog->font_desc = new_desc ? pango_font_description_copy (new_desc) : NULL;
  
  gimp_font_selection_dialog_show_available_fonts (dialog);  
  gimp_font_selection_dialog_show_available_styles (dialog);
}

static void
gimp_font_selection_dialog_ok (GtkWidget *widget,
                               gpointer   data)
{
  GimpFontSelectionDialog *dialog = (GimpFontSelectionDialog *) data;

  gimp_font_selection_set_font_desc (dialog->fontsel, dialog->font_desc);
  gtk_widget_hide (dialog->dialog);
}

static void
gimp_font_selection_dialog_apply (GtkWidget *widget,
                                  gpointer   data)
{
  GimpFontSelectionDialog *dialog = (GimpFontSelectionDialog *) data;

  gimp_font_selection_set_font_desc (dialog->fontsel, dialog->font_desc);
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

  dialog = (GimpFontSelectionDialog *) data;

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

          gimp_font_selection_dialog_preview (dialog);
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

  dialog = (GimpFontSelectionDialog *) data;

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

      gimp_font_selection_dialog_preview (dialog);
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

static void
gimp_font_selection_dialog_preview (GimpFontSelectionDialog *dialog)
{
  PangoRectangle rect;

  if (dialog->font_desc)
    {
      dialog->font_desc->size = PANGO_SCALE * 24;
      pango_layout_set_font_description (dialog->layout, dialog->font_desc);
      dialog->font_desc->size = 0;

      pango_layout_get_pixel_extents (dialog->layout, NULL, &rect);

      if (rect.height + 4 > dialog->preview->allocation.height)
        gtk_widget_set_usize (dialog->preview, -1, rect.height + 4);

      /* FIXME: align on baseline */
      
      memset (dialog->bitmap.buffer, 0,
              dialog->bitmap.rows * dialog->bitmap.pitch);
      pango_ft2_render_layout (&dialog->bitmap, dialog->layout, 2, 2);
    }

  gtk_widget_queue_draw (dialog->preview);
}

static gboolean
gimp_font_selection_dialog_preview_expose (GtkWidget      *widget,
                                           GdkEventExpose *event,
                                           gpointer        data)
{
  GimpFontSelectionDialog *dialog = (GimpFontSelectionDialog *) data;
  GdkRectangle            *area;
  
  if (!GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  area = &event->area;
  gdk_draw_rectangle (widget->window,
                      widget->style->white_gc,
                      TRUE, area->x, area->y, area->width, area->height);      
  
  if (dialog->font_desc && dialog->bitmap.buffer)
    {
      guchar *src;
      GdkGC  *gc;

      src = dialog->bitmap.buffer + area->y * dialog->bitmap.pitch + area->x;

      gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
      gdk_gc_set_function (gc, GDK_COPY_INVERT);

      gdk_draw_gray_image (widget->window,
                           widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                           area->x, area->y, 
                           area->width, area->height,
                           GDK_RGB_DITHER_NORMAL, 
                           src, dialog->bitmap.pitch);

      gdk_gc_set_function (gc, GDK_COPY);
    }

  return FALSE;
}

static void
gimp_font_selection_dialog_preview_resize (GtkWidget     *widget,
                                           GtkAllocation *allocation,
                                           gpointer       data)
{
  GimpFontSelectionDialog *dialog = (GimpFontSelectionDialog *) data;
  FT_Bitmap *bitmap;
  
  bitmap = &dialog->bitmap;
  if (bitmap->buffer)
    g_free (bitmap->buffer);

  bitmap->rows  = allocation->height;
  bitmap->width = allocation->width;
  bitmap->pitch = allocation->width;
  if (bitmap->pitch & 3)
    bitmap->pitch += 4 - (bitmap->pitch & 3);
  bitmap->buffer = g_malloc (bitmap->rows * bitmap->pitch);

  gimp_font_selection_dialog_preview (dialog);
}
