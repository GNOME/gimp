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

#include <pango/pangoft2.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpfontselection.h"
#include "gimpfontselection-dialog.h"

#include "gimp-intl.h"


#define  FONT_SIZE              18

#define  FONT_LIST_HEIGHT       136
#define  FONT_LIST_WIDTH        190
#define  FONT_STYLE_LIST_WIDTH  170

enum
{
  FAMILY_COLUMN,
  FAMILY_NAME_COLUMN
};

enum
{
  FACE_COLUMN,
  FACE_NAME_COLUMN
};


struct _GimpFontSelectionDialog
{
  GimpFontSelection    *fontsel;

  GtkWidget            *dialog;

  GtkWidget            *family_list;
  GtkWidget            *face_list;

  PangoFontFamily      *family;
  PangoFontFace        *face;

  GtkWidget            *preview;
  FT_Bitmap             bitmap;
  PangoLayout          *layout;
};


static void  gimp_font_selection_dialog_font_changed (GimpFontSelection       *fontsel,
                                                      GimpFontSelectionDialog *dialog);

static void  gimp_font_selection_dialog_ok            (GtkWidget      *widget,
                                                       gpointer        data);
static void  gimp_font_selection_dialog_apply         (GtkWidget      *widget,
                                                       gpointer        data);
static void  gimp_font_selection_dialog_preview       (GimpFontSelectionDialog *dialog);
static void  gimp_font_selection_dialog_select_family (GtkTreeSelection *selection,
                                                       gpointer          data);
static void  gimp_font_selection_dialog_select_style  (GtkTreeSelection *selection,
                                                       gpointer          data);
static void  gimp_font_selection_dialog_show_available_fonts  (GimpFontSelectionDialog *dialog);
static void  gimp_font_selection_dialog_show_available_styles (GimpFontSelectionDialog *dialog);
static gboolean  gimp_font_selection_dialog_preview_expose (GtkWidget      *widget,
                                                            GdkEventExpose *event,
                                                            gpointer        data);
static void  gimp_font_selection_dialog_preview_resize (GtkWidget      *widget,
                                                        GtkAllocation  *allocation,
                                                        gpointer        data);
static void  gimp_font_selection_dialog_preview        (GimpFontSelectionDialog *dialog);


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
  GtkListStore            *model;
  GtkTreeSelection        *select;
  GtkTreeViewColumn       *column;
  GtkWidget               *scrolled_win;
  GtkWidget               *table;
  GtkWidget               *label;
  GtkWidget               *frame;
  GClosure                *closure;

  g_return_val_if_fail (GIMP_IS_FONT_SELECTION (fontsel), NULL);

  dialog = g_new0 (GimpFontSelectionDialog, 1);

  dialog->fontsel = fontsel;

  dialog->dialog = 
    gimp_dialog_new (_("GIMP Font Selection"), "font_selection",
                     gimp_standard_help_func, "dialogs/font_selection.html",
                     GTK_WIN_POS_MOUSE,
                     FALSE, TRUE, TRUE,
                     
                     GTK_STOCK_APPLY, gimp_font_selection_dialog_apply,
                     dialog, NULL, NULL, TRUE, FALSE,
                     
                     GTK_STOCK_CANCEL, gtk_widget_hide,
                     NULL, (gpointer) 1, NULL, FALSE, TRUE,
                     
                     GTK_STOCK_OK, gimp_font_selection_dialog_ok,
                     dialog, NULL, NULL, TRUE, FALSE,
                     
                     NULL);

  gtk_container_set_border_width (GTK_CONTAINER (dialog->dialog), 1);

  /* Create the font preview */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox), frame, 
                      FALSE, FALSE, 0);
  gtk_widget_show (frame);
 
  dialog->preview = gtk_drawing_area_new ();
  gtk_widget_set_size_request (dialog->preview, -1, 30);
  gtk_container_add (GTK_CONTAINER (frame), dialog->preview);
  gtk_widget_show (dialog->preview);
  g_signal_connect (dialog->preview, "size_allocate",
                    G_CALLBACK (gimp_font_selection_dialog_preview_resize),
                    dialog);
  g_signal_connect (dialog->preview, "expose_event",
                    G_CALLBACK (gimp_font_selection_dialog_preview_expose),
                    dialog);

  /* Create the table of font & style. */
  table = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    
  /* Create the lists  */
  model = gtk_list_store_new (2,
			      G_TYPE_OBJECT,  /* FAMILY_COLUMN */
			      G_TYPE_STRING); /* FAMILY_NAME_COLUMN */
  dialog->family_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  g_object_unref (model);

  column = 
    gtk_tree_view_column_new_with_attributes ("Family",
                                              gtk_cell_renderer_text_new (),
                                              "text", FAMILY_NAME_COLUMN,
                                              NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->family_list), column);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->family_list),
				     FALSE);
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->family_list));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), 
                                       GTK_SHADOW_IN);
  gtk_widget_set_size_request (scrolled_win, 
                               FONT_LIST_WIDTH, FONT_LIST_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_win), dialog->family_list);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  gtk_widget_show (dialog->family_list);
  gtk_widget_show (scrolled_win);

  gtk_table_attach (GTK_TABLE (table), scrolled_win, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);
  
  
  model = gtk_list_store_new (2,
			      G_TYPE_OBJECT,  /* FACE_COLUMN */
			      G_TYPE_STRING); /* FACE_NAME_COLUMN */
  dialog->face_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  g_object_unref (model);

  column = 
    gtk_tree_view_column_new_with_attributes ("Face",
                                              gtk_cell_renderer_text_new (),
                                              "text", FACE_NAME_COLUMN,
                                              NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->face_list), column);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->face_list), FALSE);
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->face_list));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), 
                                       GTK_SHADOW_IN);
  gtk_widget_set_size_request (scrolled_win, 
                               FONT_STYLE_LIST_WIDTH, FONT_LIST_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_win), dialog->face_list);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  gtk_widget_show (dialog->face_list);
  gtk_widget_show (scrolled_win);
  gtk_table_attach (GTK_TABLE (table), scrolled_win, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL, 0, 0);

  label = gtk_label_new_with_mnemonic (_("_Family:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->family_list);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_FILL, 0, 0, 0);  
  label = gtk_label_new_with_mnemonic (_("_Style:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->face_list);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox), table, 
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Insert the fonts. */
  gimp_font_selection_dialog_show_available_fonts (dialog);
  
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->family_list));
  g_signal_connect (select, "changed",
		    G_CALLBACK (gimp_font_selection_dialog_select_family), 
                    dialog);

  gimp_font_selection_dialog_show_available_styles (dialog);
  
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->face_list));
  g_signal_connect (select, "changed",
		    G_CALLBACK (gimp_font_selection_dialog_select_style), 
                    dialog);

  dialog->layout = pango_layout_new (fontsel->context);
  pango_layout_set_text (dialog->layout,
                         /* This is a so-called pangram; it's supposed to
                            contain all characters found in the alphabet. */
                         _("Pack my box with five dozen liquor jugs."), -1);

  gimp_font_selection_dialog_set_font_desc (dialog, fontsel->font_desc);

  closure = g_cclosure_new (G_CALLBACK (gimp_font_selection_dialog_font_changed),
                            dialog, NULL);
  g_object_watch_closure (G_OBJECT (dialog->dialog), closure);
  g_signal_connect_closure (fontsel, "font_changed", closure, FALSE);

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
  
  gtk_window_present (GTK_WINDOW (dialog->dialog));
}

static void
gimp_font_selection_dialog_font_changed (GimpFontSelection       *fontsel,
                                         GimpFontSelectionDialog *dialog)
{
  gimp_font_selection_dialog_set_font_desc (dialog,
                                            gimp_font_selection_get_font_desc (fontsel));
}

static void
scroll_to_selection (GtkTreeView *tree_view)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
      gtk_tree_view_scroll_to_cell (tree_view, path, NULL, TRUE, 0.5, 0.5);
      gtk_tree_path_free (path);
    }
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

void
gimp_font_selection_dialog_set_font_desc (GimpFontSelectionDialog *dialog,
                                          PangoFontDescription    *desc)
{
  PangoFontFamily  *new_family = NULL;
  GtkTreeModel     *model;
  GtkTreeSelection *select;
  GtkTreeIter       iter;
  gboolean          valid;
  const gchar      *name;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (desc != NULL);

  name = pango_font_description_get_family (desc);
  if (!name)
    return;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->family_list));
  for (valid = gtk_tree_model_get_iter_root (model, &iter);
       valid && !new_family;
       valid = gtk_tree_model_iter_next (model, &iter))
    {
      PangoFontFamily *family;
      
      gtk_tree_model_get (model, &iter, FAMILY_COLUMN, &family, -1);
      
      if (g_ascii_strcasecmp (pango_font_family_get_name (family), name) == 0)
        new_family = family;
      
      g_object_unref (family);
    }

  if (!new_family && !gtk_tree_model_get_iter_root (model, &iter))
     return;

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->family_list));
  gtk_tree_selection_select_iter (select, &iter);

  gimp_font_selection_dialog_show_available_styles (dialog);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->face_list));
  for (valid = gtk_tree_model_get_iter_root (model, &iter);
       valid;
       valid = gtk_tree_model_iter_next (model, &iter))
    {
      PangoFontFace        *face;
      PangoFontDescription *tmp_desc;
      
      gtk_tree_model_get (model, &iter, FACE_COLUMN, &face, -1);
      tmp_desc = pango_font_face_describe (face);
      
      if (font_description_style_equal (tmp_desc, desc))
        {
	  select =
	    gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->face_list));
          gtk_tree_selection_select_iter (select, &iter);

          pango_font_description_free (tmp_desc);
          g_object_unref (face);
          break;
        }
      
      pango_font_description_free (tmp_desc);
      g_object_unref (face);
    }

  scroll_to_selection (GTK_TREE_VIEW (dialog->family_list));
  scroll_to_selection (GTK_TREE_VIEW (dialog->face_list));
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

      g_signal_handlers_block_by_func (dialog->fontsel,
                                       G_CALLBACK (gimp_font_selection_dialog_font_changed),
                                       dialog);

      gimp_font_selection_set_font_desc (dialog->fontsel, font_desc);

      g_signal_handlers_unblock_by_func (dialog->fontsel,
                                         G_CALLBACK (gimp_font_selection_dialog_font_changed),
                                         dialog);

      pango_font_description_free (font_desc);
    }
}

static void
gimp_font_selection_dialog_select_family (GtkTreeSelection *selection,
                                          gpointer          data)
{
  GimpFontSelectionDialog *dialog;
  GtkTreeModel            *model;
  GtkTreeIter              iter;

  dialog = (GimpFontSelectionDialog *) data;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      PangoFontFamily *family;
      
      gtk_tree_model_get (model, &iter, FAMILY_COLUMN, &family, -1);
      if (dialog->family != family)
	{
	  dialog->family = family;
	  gimp_font_selection_dialog_show_available_styles (dialog);
          gimp_font_selection_dialog_preview (dialog);
	}

      g_object_unref (family);
    }
}

static void
gimp_font_selection_dialog_select_style (GtkTreeSelection *selection,
                                         gpointer          data)
{
  GimpFontSelectionDialog *dialog;
  GtkTreeModel            *model;
  GtkTreeIter              iter;
  
  dialog = (GimpFontSelectionDialog *) data;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      PangoFontFace *face;
      
      gtk_tree_model_get (model, &iter, FACE_COLUMN, &face, -1);

      if (dialog->face != face)
        {
          dialog->face = face;
          gimp_font_selection_dialog_preview (dialog);
        }

      g_object_unref (face);
    }
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
  GtkListStore     *model;
  PangoFontFamily **families;
  PangoFontFamily  *match_family = NULL;
  const gchar      *current_name;
  gint              n_families, i;
  
  model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->family_list)));
  
  pango_context_list_families (dialog->fontsel->context,
			       &families, &n_families);
  qsort (families, n_families, sizeof (PangoFontFamily *), cmp_families);

  gtk_list_store_clear (model);

  if (dialog->family)
    current_name = pango_font_family_get_name (dialog->family);
  else
    current_name = "Sans";

  for (i = 0; i < n_families; i++)
    {
      const gchar *name = pango_font_family_get_name (families[i]);
      GtkTreeIter  iter;

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
			  FAMILY_COLUMN, families[i],
			  FAMILY_NAME_COLUMN, name,
			  -1);
      
      if (i == 0 || !g_ascii_strcasecmp (name, current_name))
        match_family = families[i];
    }

  dialog->family = match_family;

  g_free (families);
}

static gint
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

  ord = compare_font_descriptions (desc_a, desc_b);
  
  pango_font_description_free (desc_a);
  pango_font_description_free (desc_b);

  return ord;
}

static void
gimp_font_selection_dialog_show_available_styles (GimpFontSelectionDialog *dialog)
{
  PangoFontFace        **faces;
  PangoFontDescription  *old_desc;
  GtkListStore          *model;
  GtkTreeSelection      *select;
  GtkTreeIter            match_row;
  PangoFontFace         *match_face = NULL;
  gint                   n_faces, i;
  
  model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->face_list)));
  
  if (dialog->face)
    old_desc = pango_font_face_describe (dialog->face);
  else
    old_desc= NULL;

  pango_font_family_list_faces (dialog->family, &faces, &n_faces);
  qsort (faces, n_faces, sizeof (PangoFontFace *), faces_sort_func);

  gtk_list_store_clear (model);

  if (n_faces < 0)
    return;

  for (i = 0; i < n_faces; i++)
    {
      GtkTreeIter  iter;
      const gchar *str = pango_font_face_get_face_name (faces[i]);

      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
			  FACE_COLUMN, faces[i],
			  FACE_NAME_COLUMN, str, -1);

      if (i == 0)
	{
	  match_row  = iter;
	  match_face = faces[i];
	}
      else if (old_desc)
	{
	  PangoFontDescription *tmp_desc = pango_font_face_describe (faces[i]);
	  
	  if (font_description_style_equal (tmp_desc, old_desc))
            {
	      match_row  = iter;
              match_face = faces[i];
            }

	  pango_font_description_free (tmp_desc);
	}
    }

  if (old_desc)
    pango_font_description_free (old_desc);

  dialog->face = match_face;

  g_free (faces);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->face_list));
  gtk_tree_selection_select_iter (select, &match_row);
}

static void
gimp_font_selection_dialog_preview (GimpFontSelectionDialog *dialog)
{
  PangoRectangle rect;

  if (dialog->face)
    {
      PangoFontDescription *font_desc;

      font_desc = pango_font_face_describe (dialog->face);
      pango_font_description_set_size (font_desc, PANGO_SCALE * FONT_SIZE);
      pango_layout_set_font_description (dialog->layout, font_desc);
      pango_font_description_free (font_desc);
      
      pango_layout_get_pixel_extents (dialog->layout, NULL, &rect);

      if (rect.height + 4 > dialog->preview->allocation.height)
        gtk_widget_set_size_request (dialog->preview, -1, rect.height + 4);

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
