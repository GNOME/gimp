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

  GtkWidget            *dialog;

  GtkWidget            *font_clist;
  GtkWidget            *font_style_clist;

  PangoFontFamily     **families;
  PangoFontFamily      *family;	 /* Current family */
  gint                  n_families;
  
  PangoFontFace       **faces;
  PangoFontFace        *face;	 /* Current face   */

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

  dialog->layout = pango_layout_new (fontsel->context);
  pango_layout_set_text (dialog->layout, 
                         "my mind is going ...", -1);

  dialog->families   = NULL;
  dialog->family     = NULL;
  dialog->n_families = 0;
 
  dialog->faces      = NULL;
  dialog->face       = NULL;

  gimp_font_selection_dialog_show_available_fonts (dialog);

  gimp_font_selection_dialog_set_font_desc (dialog, fontsel->font_desc);

  return dialog;
}

void 
gimp_font_selection_dialog_destroy (GimpFontSelectionDialog *dialog)
{
  gint i;

  g_return_if_fail (dialog != NULL);

  gtk_widget_destroy (dialog->dialog);
  g_free (dialog->bitmap.buffer);

  for (i = 0; i < dialog->n_families; i++)
    g_object_unref (dialog->families[i]);

  g_free (dialog->families);
  g_free (dialog->faces);

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
                                          PangoFontDescription    *desc)
{
  const gchar *name;
  gint i;
  gint row = -1;

  g_return_if_fail (dialog != NULL);

  if (desc)
    {
      name = pango_font_description_get_family (desc);

      for (i = 0; i < dialog->n_families; i++)
        {
          if (!g_strcasecmp (name, 
                             pango_font_family_get_name (dialog->families[i])))
            {
              row = i;
              break;
            }
        }
    }
  
  if (row >= 0)
    {
      dialog->family = dialog->families[row];
      gtk_clist_select_row (GTK_CLIST (dialog->font_clist), row, 0);
      if (gtk_clist_row_is_visible (GTK_CLIST (dialog->font_clist), row) != GTK_VISIBILITY_FULL)
        gtk_clist_moveto (GTK_CLIST (dialog->font_clist), row, -1,
                          0.5, 0);

      gimp_font_selection_dialog_show_available_styles (dialog);
    }
  else
    {
      dialog->family = NULL;
    }
}

static void
gimp_font_selection_dialog_ok (GtkWidget *widget,
                               gpointer   data)
{
  GimpFontSelectionDialog *dialog = (GimpFontSelectionDialog *) data;

  gimp_font_selection_dialog_apply (widget, data);
  gtk_widget_hide (dialog->dialog);
}

static void
gimp_font_selection_dialog_apply (GtkWidget *widget,
                                  gpointer   data)
{
  PangoFontDescription *font_desc;
  GimpFontSelectionDialog *dialog = (GimpFontSelectionDialog *) data;
  
  if (dialog->face)
    {
      font_desc = pango_font_face_describe (dialog->face);

      gimp_font_selection_set_font_desc (dialog->fontsel, font_desc);
      pango_font_description_free (font_desc);
    }
}

static void
gimp_font_selection_dialog_select_font (GtkWidget      *widget,
                                        gint	        row,
                                        gint	        column,
                                        GdkEventButton *bevent,
                                        gpointer        data)
{
  GimpFontSelectionDialog *dialog;
  gint   index;

  dialog = (GimpFontSelectionDialog *) data;

  if (GTK_CLIST (dialog->font_clist)->selection)
    {
      index = GPOINTER_TO_INT (GTK_CLIST (dialog->font_clist)->selection->data);

      if (dialog->family != dialog->families[index])
	{
	  dialog->family = dialog->families[index];
	  	  
	  gimp_font_selection_dialog_show_available_styles (dialog);

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
  gint   index;

  dialog = (GimpFontSelectionDialog *) data;

  if (bevent && !GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);
  
  if (GTK_CLIST (dialog->font_style_clist)->selection)
    {
      index = GPOINTER_TO_INT (GTK_CLIST (dialog->font_style_clist)->selection->data);
      dialog->face = dialog->faces[index];
    }

  gimp_font_selection_dialog_preview (dialog);
}     

static int
cmp_families (const void *a, const void *b)
{
  const gchar *a_name = pango_font_family_get_name (*(PangoFontFamily **)a);
  const gchar *b_name = pango_font_family_get_name (*(PangoFontFamily **)b);
  
  return strcmp (a_name, b_name);
}

static void
gimp_font_selection_dialog_show_available_fonts (GimpFontSelectionDialog *dialog)
{
  const gchar *name;
  gint i;

  pango_context_list_families (dialog->fontsel->context,
			       &dialog->families, &dialog->n_families);
  qsort (dialog->families, dialog->n_families, sizeof (PangoFontFamily *), 
         cmp_families);

  gtk_clist_freeze (GTK_CLIST (dialog->font_clist));
  gtk_clist_clear (GTK_CLIST (dialog->font_clist));

  for (i = 0; i < dialog->n_families; i++)
    {
      g_object_ref (dialog->families[i]);
      name = pango_font_family_get_name (dialog->families[i]);
      gtk_clist_append (GTK_CLIST (dialog->font_clist), (gchar **)&name);
    }

  gtk_clist_thaw (GTK_CLIST (dialog->font_clist));
}

static int
compare_font_descriptions (const PangoFontDescription *a, 
                           const PangoFontDescription *b)
{
  gint val;

  val = strcmp (pango_font_description_get_family (a), 
                pango_font_description_get_family (b));

  if (val != 0)
    return val;

  if (pango_font_description_get_weight (a) != pango_font_description_get_weight (b))
    return pango_font_description_get_weight (a) - pango_font_description_get_weight (b);

  if (pango_font_description_get_style (a) != pango_font_description_get_style (b))
    return pango_font_description_get_style (a) - pango_font_description_get_style (b);
  
  if (pango_font_description_get_stretch (a) != pango_font_description_get_stretch (b))
    return pango_font_description_get_stretch (a) - pango_font_description_get_stretch (b);

  if (pango_font_description_get_variant (a) != pango_font_description_get_variant (b))
    return pango_font_description_get_variant (a) - pango_font_description_get_variant (b);

  return 0;
}

static gint
faces_sort_func (const void *a, const void *b)
{
  PangoFontDescription *desc_a;
  PangoFontDescription *desc_b;
  gint ord;

  desc_a = pango_font_face_describe (*(PangoFontFace **)a);
  desc_b = pango_font_face_describe (*(PangoFontFace **)b);

  /* FIXME: shouldn't need to check this, but there's a PangoFT2 bug */
  if (desc_a && desc_b)
    ord = compare_font_descriptions (desc_a, desc_b);
  else
    ord = 0;
  
  pango_font_description_free (desc_a);
  pango_font_description_free (desc_b);

  return ord;
}

static gboolean
font_description_style_equal (const PangoFontDescription *a,
			      const PangoFontDescription *b)
{
  return (pango_font_description_get_weight (a) == pango_font_description_get_weight (b) &&
	  pango_font_description_get_style (a) == pango_font_description_get_style (b) &&
	  pango_font_description_get_stretch (a) == pango_font_description_get_stretch (b) &&
	  pango_font_description_get_variant (a) == pango_font_description_get_variant (b));
}

static void
gimp_font_selection_dialog_show_available_styles (GimpFontSelectionDialog *dialog)
{
  PangoFontDescription *old_desc;
  const gchar *str;
  gint row     = -1;
  gint n_faces = 0;
  gint i;

  if (dialog->face)
    old_desc = pango_font_face_describe (dialog->face);
  else
    old_desc= NULL;

  if (dialog->faces)
    {
      g_free (dialog->faces);
      dialog->faces = NULL;
    }

  if (dialog->family)
    {
      pango_font_family_list_faces (dialog->family, &dialog->faces, &n_faces);
      
      if (n_faces > 0)
        qsort (dialog->faces, n_faces, sizeof (PangoFontFace *), 
               faces_sort_func);
    }

  gtk_clist_freeze (GTK_CLIST (dialog->font_style_clist));
  gtk_clist_clear (GTK_CLIST (dialog->font_style_clist));

  for (i = 0; i < n_faces && row < 0; i++)
    {
      str = pango_font_face_get_face_name (dialog->faces[i]);
      gtk_clist_append (GTK_CLIST (dialog->font_style_clist), (gchar **)&str);

      if (old_desc)
	{
	  PangoFontDescription *tmp_desc;

          tmp_desc = pango_font_face_describe (dialog->faces[i]);
	  
	  if (font_description_style_equal (tmp_desc, old_desc))
            row = i;
         
          pango_font_description_free (tmp_desc);
	}
    }

  if (old_desc)
    pango_font_description_free (old_desc);

  if (row < 0 && n_faces)
    row = 0;
  
  if (row >= 0)
    {
      dialog->face = dialog->faces[row];
      gtk_clist_select_row (GTK_CLIST (dialog->font_style_clist), row, 0);
      if (gtk_clist_row_is_visible (GTK_CLIST (dialog->font_style_clist), row) != GTK_VISIBILITY_FULL)
        gtk_clist_moveto (GTK_CLIST (dialog->font_style_clist), row, -1,
                          0.5, 0);    
    }
  else
    {
      dialog->face = NULL;
    }

  gtk_clist_thaw (GTK_CLIST (dialog->font_style_clist));
}

static void
gimp_font_selection_dialog_preview (GimpFontSelectionDialog *dialog)
{
  PangoRectangle rect;

  if (dialog->face)
    {
      PangoFontDescription *font_desc;

      font_desc = pango_font_face_describe (dialog->face);
      pango_font_description_set_size (font_desc, PANGO_SCALE * 24);
      pango_layout_set_font_description (dialog->layout, font_desc);
      pango_font_description_free (font_desc);
      
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
  
  if (dialog->face && dialog->bitmap.buffer)
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
