/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <libgimpmath/gimpmath.h>

#include <gtk/gtklist.h>
#include <gtk/gtkpreview.h>

#include "gimpressionist.h"
#include "ppmtool.h"
#include "brush.h"
#include "presets.h"

#include <libgimp/stdplugins-intl.h>


static void  update_brush_preview (const char *fn);


static GtkWidget    *brush_preview    = NULL;
static GtkListStore *brush_list_store = NULL;

static GtkWidget *brush_list          = NULL;
static GtkObject *brush_relief_adjust = NULL;
static GtkObject *brush_aspect_adjust = NULL;
static GtkObject *brush_gamma_adjust  = NULL;
static gboolean   brush_dont_update   = FALSE;

static gchar *last_selected_brush = NULL;

static gint brush_from_file = 2;

static ppm_t brushppm  = {0, 0, NULL};

void
brush_restore (void)
{
  reselect (brush_list, pcvals.selected_brush);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_gamma_adjust), pcvals.brushgamma);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_relief_adjust), pcvals.brush_relief);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_aspect_adjust), pcvals.brush_aspect);
}

void
brush_store (void)
{
  pcvals.brushgamma = GTK_ADJUSTMENT (brush_gamma_adjust)->value;
}

void
brush_free (void)
{
  g_free (last_selected_brush);
}

void brush_get_selected (ppm_t *p)
{
  if (brush_from_file)
    brush_reload (pcvals.selected_brush, p);
  else
    ppm_copy (&brushppm, p);
}


static gboolean
file_is_color (const char *fn)
{
  return fn && strstr (fn, ".ppm");
}

void
set_colorbrushes (const gchar *fn)
{
  pcvals.color_brushes = file_is_color (fn);
}

static void
brushdmenuselect (GtkWidget *widget,
                  gpointer   data)
{
  GimpPixelRgn  src_rgn;
  guchar       *src_row;
  guchar       *src;
  gint          id;
  gint          alpha, bpp;
  gboolean      has_alpha;
  gint          x, y;
  ppm_t        *p;
  gint          x1, y1, x2, y2;
  gint          row;
  GimpDrawable *drawable;
  gint          rowstride;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &id);

  if (id == -1)
    return;

  if (brush_from_file == 2)
    return; /* Not finished GUI-building yet */

  if (brush_from_file)
    {
#if 0
      unselectall (brush_list);
#endif
      preset_save_button_set_sensitive (FALSE);
    }

  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_gamma_adjust), 1.0);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_aspect_adjust), 0.0);

  drawable = gimp_drawable_get (id);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  bpp = gimp_drawable_bpp (drawable->drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  alpha = (has_alpha) ? bpp - 1 : bpp;

  ppm_kill (&brushppm);
  ppm_new (&brushppm, x2 - x1, y2 - y1);
  p = &brushppm;

  rowstride = p->width * 3;

  src_row = g_new (guchar, (x2 - x1) * bpp);

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       0, 0, x2 - x1, y2 - y1,
                       FALSE, FALSE);

  if (bpp == 3)
    { /* RGB */
      int bpr = (x2 - x1) * 3;

      for (row = 0, y = y1; y < y2; row++, y++)
        {
          gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
          memcpy (p->col + row*rowstride, src_row, bpr);
        }
    }
  else
    { /* RGBA (bpp > 3) GrayA (bpp == 2) or Gray */
      gboolean is_gray = ((bpp > 3) ? TRUE : FALSE);

      for (row = 0, y = y1; y < y2; row++, y++)
        {
          guchar *tmprow = p->col + row * rowstride;
          guchar *tmprow_ptr;

          gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
          src = src_row;
          tmprow_ptr = tmprow;
          /* Possible micro-optimization here:
           * src_end = src + src_rgn.bpp * (x2-x1);
           * for ( ; src < src_end ; src += src_rgn.bpp)
           */
          for (x = x1; x < x2; x++)
            {
              *(tmprow_ptr++) = src[0];
              *(tmprow_ptr++) = src[is_gray ? 1 : 0];
              *(tmprow_ptr++) = src[is_gray ? 2 : 0];
              src += src_rgn.bpp;
            }
        }
    }
  g_free (src_row);

  if (bpp >= 3)
    pcvals.color_brushes = 1;
  else
    pcvals.color_brushes = 0;

  brush_from_file = 0;
  update_brush_preview (NULL);
}

#if 0
void
dummybrushdmenuselect (GtkWidget *w, gpointer data)
{
  ppm_kill (&brushppm);
  ppm_new (&brushppm, 10,10);
  brush_from_file = 0;
  update_brush_preview (NULL);
}
#endif

static void
brushlistrefresh (void)
{
  gtk_list_store_clear (brush_list_store);
  readdirintolist ("Brushes", brush_list, NULL);
}

static void
savebrush_response (GtkWidget *dialog,
                    gint       response_id,
                    gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      ppm_save (&brushppm, name);
      brushlistrefresh ();

      g_free (name);
    }

  gtk_widget_destroy (dialog);
}

static void
savebrush (GtkWidget *wg,
           gpointer   data)
{
  GtkWidget *dialog   = NULL;
  GList     *thispath = parsepath ();
  gchar     *path;

  if (! PPM_IS_INITED (&brushppm))
    {
      g_message ( _("Can only save drawables!"));
      return;
    }

  dialog =
    gtk_file_chooser_dialog_new (_("Save Brush"),
                                 GTK_WINDOW (gtk_widget_get_toplevel (wg)),
                                 GTK_FILE_CHOOSER_ACTION_SAVE,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                 NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                  TRUE);

  path = g_build_filename ((gchar *)thispath->data, "Brushes", NULL);

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), path);

  g_free (path);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &dialog);
  g_signal_connect (dialog, "response",
                    G_CALLBACK (savebrush_response),
                    NULL);

  gtk_widget_show (dialog);
}

static gboolean
validdrawable (gint32    imageid,
               gint32    drawableid,
               gpointer  data)
{
  return (gimp_drawable_is_rgb (drawableid) ||
          gimp_drawable_is_gray (drawableid));
}

/*
 * This function caches the last result. Call it with fn as NULL, to
 * free the arguments.
 * */
void
brush_reload (const gchar *fn,
              ppm_t       *p)
{
  static char  lastfn[256] = "";
  static ppm_t cache       = {0, 0, NULL};

  if (fn == NULL)
    {
      ppm_kill (&cache);
      lastfn[0] = '\0';
      return;
    }

  if (strcmp (fn, lastfn))
    {
      g_strlcpy (lastfn, fn, sizeof (lastfn));
      ppm_kill (&cache);
      ppm_load (fn, &cache);
    }
  ppm_copy (&cache, p);
  set_colorbrushes (fn);
}

static void
padbrush (ppm_t *p,
          gint   width,
          gint   height)
{
  guchar black[3] = {0, 0, 0};

  int left   = (width - p->width) / 2;
  int right  = (width - p->width) - left;
  int top    = (height - p->height) / 2;
  int bottom = (height - p->height) - top;

  ppm_pad (p, left, right, top, bottom, black);
}

static void
update_brush_preview (const gchar *fn)
{
  gint   i, j;
  guchar *preview_image;

  if (fn)
    brush_from_file = 1;

  preview_image = g_new0 (guchar, 100*100);

  if (!fn && brush_from_file)
    {
      /* preview_image is already initialized to our liking. */
    }
  else
    {
      double sc;
      ppm_t  p = {0, 0, NULL};
      guchar gammatable[256];
      int    newheight;

      if (brush_from_file)
        brush_reload (fn, &p);
      else if (PPM_IS_INITED (&brushppm))
        ppm_copy (&brushppm, &p);

      set_colorbrushes (fn);

      sc = GTK_ADJUSTMENT (brush_gamma_adjust)->value;
      if (sc != 1.0)
        for (i = 0; i < 256; i++)
          gammatable[i] = pow (i / 255.0, sc) * 255;
      else
        for (i = 0; i < 256; i++)
          gammatable[i] = i;

      newheight = p.height *
                    pow (10, GTK_ADJUSTMENT (brush_aspect_adjust)->value);

      sc = p.width > newheight ? p.width : newheight;
      sc = 100.0 / sc;
      resize_fast (&p, p.width*sc,newheight*sc);
      padbrush (&p, 100, 100);
      for (i = 0; i < 100; i++)
        {
          int k = i * p.width * 3;
          if (i < p.height)
            for (j = 0; j < p.width; j++)
              preview_image[i*100+j] = gammatable[p.col[k + j * 3]];
        }
      ppm_kill (&p);
    }
  gimp_preview_area_draw (GIMP_PREVIEW_AREA (brush_preview),
                          0, 0, 100, 100,
                          GIMP_GRAY_IMAGE,
                          preview_image,
                          100);

  g_free (preview_image);
}


/*
 * "force" implies here to change the brush even if it was the same.
 * It is used for the initialization of the preview.
 * */
static void
brush_select (GtkTreeSelection *selection, gboolean force)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;
  gchar        *fname = NULL;
  gchar        *brush = NULL;

  if (brush_dont_update)
    goto cleanup;

  if (brush_from_file == 0)
    {
      update_brush_preview (NULL);
      goto cleanup;
    }

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 0, &brush, -1);

      /* Check if the same brush was selected twice, and if so
       * break. Otherwise, the brush gamma and stuff would have been
       * reset.
       * */
      if (last_selected_brush == NULL)
        {
          last_selected_brush = g_strdup (brush);
        }
      else
        {
          if (!strcmp (last_selected_brush, brush))
            {
              if (!force)
                {
                  goto cleanup;
                }
            }
          else
            {
              g_free (last_selected_brush);
              last_selected_brush = g_strdup (brush);
            }
        }

      brush_dont_update = TRUE;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_gamma_adjust), 1.0);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_aspect_adjust), 0.0);
      brush_dont_update = FALSE;

      if (brush)
        {
          fname = g_build_filename ("Brushes", brush, NULL);

          g_strlcpy (pcvals.selected_brush,
                     fname, sizeof (pcvals.selected_brush));

          update_brush_preview (fname);

        }
    }
cleanup:
  g_free (fname);
  g_free (brush);
}

static void
brush_select_file (GtkTreeSelection *selection, gpointer data)
{
  brush_from_file = 1;
  preset_save_button_set_sensitive (TRUE);
  brush_select (selection, FALSE);
}

static void
brush_preview_size_allocate (GtkWidget *preview)
{
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (brush_list));
  brush_select (selection, TRUE);
}

static void
brush_asepct_adjust_cb (GtkWidget *w, gpointer data)
{
  gimp_double_adjustment_update (GTK_ADJUSTMENT (w), data);
  update_brush_preview (pcvals.selected_brush);
}

void
create_brushpage (GtkNotebook *notebook)
{
  GtkWidget        *box1, *box2, *box3, *thispage;
  GtkWidget        *view;
  GtkWidget        *tmpw, *table;
  GtkWidget        *frame;
  GtkWidget        *combo;
  GtkWidget        *label;
  GtkSizeGroup     *group;
  GtkTreeSelection *selection;

  label = gtk_label_new_with_mnemonic (_("_Brush"));

  thispage = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show (thispage);

  box1 = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (thispage), box1, TRUE,TRUE,0);
  gtk_widget_show (box1);

  view = create_one_column_list (box1, brush_select_file);
  brush_list = view;
  brush_list_store =
      GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  box2 = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  brush_preview = tmpw = gimp_preview_area_new ();
  gtk_widget_set_size_request (brush_preview, 100, 100);
  gtk_container_add (GTK_CONTAINER (frame), tmpw);
  gtk_widget_show (tmpw);
  g_signal_connect (brush_preview, "size-allocate",
                    G_CALLBACK (brush_preview_size_allocate), NULL);

  box3 = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_end (GTK_BOX (box2), box3, FALSE, FALSE,0);
  gtk_widget_show (box3);

  tmpw = gtk_label_new (_("Gamma:"));
  gtk_misc_set_alignment (GTK_MISC (tmpw), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);

  brush_gamma_adjust = gtk_adjustment_new (pcvals.brushgamma,
                                           0.5, 3.0, 0.1, 0.1, 1.0);
  tmpw = gtk_hscale_new (GTK_ADJUSTMENT (brush_gamma_adjust));
  gtk_widget_set_size_request (GTK_WIDGET (tmpw), 100, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), FALSE);
  gtk_scale_set_digits (GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect_swapped (brush_gamma_adjust, "value-changed",
                            G_CALLBACK (update_brush_preview),
                            pcvals.selected_brush);

  gimp_help_set_help_data
    (tmpw, _("Changes the gamma (brightness) of the selected brush"), NULL);

  box3 = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (thispage), box3, FALSE, FALSE,0);
  gtk_widget_show (box3);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  tmpw = gtk_label_new (_("Select:"));
  gtk_misc_set_alignment (GTK_MISC (tmpw), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);

  gtk_size_group_add_widget (group, tmpw);
  g_object_unref (group);

  combo = gimp_drawable_combo_box_new (validdrawable, NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), -1,
                              G_CALLBACK (brushdmenuselect),
                              NULL);

  gtk_box_pack_start (GTK_BOX (box3), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  tmpw = gtk_button_new_from_stock (GTK_STOCK_SAVE_AS);
  gtk_box_pack_start (GTK_BOX (box3),tmpw, FALSE, FALSE, 0);
  g_signal_connect (tmpw, "clicked", G_CALLBACK (savebrush), NULL);
  gtk_widget_show (tmpw);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (thispage), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  brush_aspect_adjust =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                          _("Aspect ratio:"),
                          150, -1, pcvals.brush_aspect,
                          -1.0, 1.0, 0.1, 0.1, 2,
                          TRUE, 0, 0,
                          _("Specifies the aspect ratio of the brush"),
                          NULL);
  gtk_size_group_add_widget (group,
                             GIMP_SCALE_ENTRY_LABEL (brush_aspect_adjust));
  g_signal_connect (brush_aspect_adjust, "value-changed",
                    G_CALLBACK (brush_asepct_adjust_cb), &pcvals.brush_aspect);

  brush_relief_adjust =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                          _("Relief:"),
                          150, -1, pcvals.brush_relief,
                          0.0, 100.0, 1.0, 10.0, 1,
                          TRUE, 0, 0,
                          _("Specifies the amount of embossing to apply to the image (in percent)"),
                          NULL);
  gtk_size_group_add_widget (group,
                             GIMP_SCALE_ENTRY_LABEL (brush_relief_adjust));
  g_signal_connect (brush_relief_adjust, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.brush_relief);

  brush_select (selection, FALSE);
  readdirintolist ("Brushes", view, pcvals.selected_brush);

  /*
   * This is so the "changed signal won't get sent to the brushes' list
   * and reset the gamma and stuff.
   * */
  gtk_widget_grab_focus (brush_list);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}

