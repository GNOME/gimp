
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <libgimpmath/gimpmath.h>

#include <gtk/gtklist.h>
#include <gtk/gtkpreview.h>

#include "gimpressionist.h"
#include "ppmtool.h"

#include <libgimp/stdplugins-intl.h>


GtkWidget *brushlist            = NULL;
GtkObject *brushreliefadjust    = NULL;
GtkObject *brushaspectadjust    = NULL;
GtkObject *brushgammaadjust     = NULL;

gint  brushfile = 2;
ppm_t brushppm  = {0, 0, NULL};

static GtkWidget    *brushprev  = NULL;
static GtkListStore *brushstore = NULL;

static void  updatebrushprev (const char *fn);

static gboolean colorfile (const char *fn)
{
  return fn && strstr(fn, ".ppm");
}

static void
brushdmenuselect (GtkWidget *widget,
                  gpointer   data)
{
  GimpPixelRgn src_rgn;
  guchar *src_row;
  guchar *src;
  gint id;
  gint alpha, has_alpha, bpp;
  gint x, y;
  ppm_t *p;
  gint x1, y1, x2, y2;
  gint row, col;
  GimpDrawable *drawable;
  gint rowstride;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &id);

  if (id == -1)
    return;

  if (brushfile == 2)
    return; /* Not finished GUI-building yet */

  if (brushfile)
    {
      /* unselectall(brushlist); */
      if (GTK_IS_WIDGET (presetsavebutton))
        gtk_widget_set_sensitive (GTK_WIDGET (presetsavebutton), FALSE);
    }

  gtk_adjustment_set_value (GTK_ADJUSTMENT (brushgammaadjust), 1.0);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brushaspectadjust), 0.0);

  drawable = gimp_drawable_get (id);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  bpp = gimp_drawable_bpp (drawable->drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  alpha = (has_alpha) ? bpp - 1 : bpp;

  if(brushppm.col)
    killppm(&brushppm);
  newppm(&brushppm, x2-x1, y2-y1);
  p = &brushppm;

  rowstride = p->width * 3;

  src_row = g_new (guchar, (x2 - x1) * bpp);

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, x2-x1, y2-y1, FALSE, FALSE);

  if(bpp == 3) { /* RGB */
    int bpr = (x2-x1) * 3;
    for(row = 0, y = y1; y < y2; row++, y++) {
      gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
      memcpy(p->col + row*rowstride, src_row, bpr);
    }
  } else if(bpp > 3) { /* RGBA */
    for(row = 0, y = y1; y < y2; row++, y++) {
      guchar *tmprow = p->col + row * rowstride;
      gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
      src = src_row;
      for (col = 0, x = x1; x < x2; col++, x++) {
	int k = col * 3;
	tmprow[k+0] = src[0];
	tmprow[k+1] = src[1];
	tmprow[k+2] = src[2];
	src += src_rgn.bpp;
      }
    }
  } else if(bpp == 2) { /* GrayA */
    for(row = 0, y = y1; y < y2; row++, y++) {
	guchar *tmprow = p->col + row * rowstride;
      gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
      src = src_row;
      for (col = 0, x = x1; x < x2; col++, x++) {
	int k = col * 3;
	tmprow[k+0] = src[0];
	tmprow[k+1] = src[0];
	tmprow[k+2] = src[0];
	src += src_rgn.bpp;
      }
    }
  } else { /* Gray */
    for(row = 0, y = y1; y < y2; row++, y++) {
      guchar *tmprow = p->col + row * rowstride;
      gimp_pixel_rgn_get_row (&src_rgn, src_row, x1, y, (x2 - x1));
      src = src_row;
      for (col = 0, x = x1; x < x2; col++, x++) {
	int k = col * 3;
	tmprow[k+0] = src[0];
	tmprow[k+1] = src[0];
	tmprow[k+2] = src[0];
	src += src_rgn.bpp;
      }
    }
  }
  g_free (src_row);

  if(bpp >= 3) pcvals.colorbrushes = 1;
  else pcvals.colorbrushes = 0;

  brushfile = 0;
  updatebrushprev(NULL);
}

void dummybrushdmenuselect(GtkWidget *w, gpointer data)
{
  if(brushppm.col)
    killppm(&brushppm);
  newppm(&brushppm, 10,10);
  brushfile = 0;
  updatebrushprev(NULL);
}

static void
brushlistrefresh (void)
{
  gtk_list_store_clear (brushstore);
  readdirintolist("Brushes", brushlist, NULL);
}

static void
savebrush_response (GtkWidget *dialog,
                    gint       response_id,
                    gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      saveppm (&brushppm, name);
      brushlistrefresh ();

      g_free (name);
    }

  gtk_widget_destroy (dialog);
}

static void
savebrush (GtkWidget *wg,
           gpointer   data)
{
  static GtkWidget *dialog   = NULL;
  GList            *thispath = parsepath ();
  gchar            *path;

  if (! brushppm.col)
    {
      g_message( _("Can only save drawables!"));
      return;
    }

  dialog =
    gtk_file_chooser_dialog_new (_("Save Brush"),
                                 GTK_WINDOW (gtk_widget_get_toplevel (wg)),
                                 GTK_FILE_CHOOSER_ACTION_SAVE,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

                                 NULL);

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

void
reloadbrush (const gchar *fn,
             ppm_t       *p)
{
  static char lastfn[256] = "";
  static ppm_t cache = {0,0,NULL};

  if (strcmp(fn, lastfn))
    {
      g_strlcpy (lastfn, fn, sizeof (lastfn));
      loadppm (fn, &cache);
    }
  copyppm(&cache, p);
  pcvals.colorbrushes = colorfile(fn);
}

static void
padbrush (ppm_t *p,
          gint   width,
          gint   height)
{
  guchar black[3] = {0,0,0};
  int left = (width - p->width) / 2;
  int right = (width - p->width) - left;
  int top = (height - p->height) / 2;
  int bottom = (height - p->height) - top;
  pad(p, left, right, top, bottom, black);
}

static void
updatebrushprev (const gchar *fn)
{
  gint   i, j;
  guchar buf[100];

  if(fn)
    brushfile = 1;

  if (!fn && brushfile) {
    memset(buf, 0, 100);
    for(i = 0; i < 100; i++) {
      gtk_preview_draw_row (GTK_PREVIEW (brushprev), buf, 0, i, 100);
    }
  } else {
    double sc;
    ppm_t p = {0,0,NULL};
    guchar gammatable[256];
    int newheight;

    if(brushfile)
      reloadbrush(fn, &p);
    else if(brushppm.col)
      copyppm(&brushppm, &p);

    pcvals.colorbrushes = colorfile(fn);

    sc = GTK_ADJUSTMENT(brushgammaadjust)->value;
    if(sc != 1.0)
      for(i = 0; i < 256; i++)
	gammatable[i] = pow(i / 255.0, sc) * 255;
    else
      for(i = 0; i < 256; i++)
	gammatable[i] = i;

    newheight = p.height*pow(10,GTK_ADJUSTMENT(brushaspectadjust)->value);
    sc = p.width > newheight ? p.width : newheight;
    sc = 100.0 / sc;
    resize_fast(&p, p.width*sc,newheight*sc);
    padbrush(&p, 100, 100);
    for(i = 0; i < 100; i++) {
      int k = i * p.width * 3;
      memset(buf,0,100);
      if(i < p.height)
	for(j = 0; j < p.width; j++)
	  buf[j] = gammatable[p.col[k + j * 3]];
      gtk_preview_draw_row (GTK_PREVIEW (brushprev), buf, 0, i, 100);
    }
    killppm(&p);
  }
  gtk_widget_queue_draw (brushprev);
}

static gboolean brushdontupdate = FALSE;

static void
selectbrush (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;

  if (brushdontupdate)
    return;

  if (brushfile == 0)
    {
      updatebrushprev (NULL);
      return;
    }

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *brush;

      gtk_tree_model_get (model, &iter, 0, &brush, -1);

      brushdontupdate = TRUE;
      gtk_adjustment_set_value(GTK_ADJUSTMENT(brushgammaadjust), 1.0);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(brushaspectadjust), 0.0);
      brushdontupdate = FALSE;

      if (brush)
        {
          gchar *fname = g_build_filename ("Brushes", brush, NULL);

          g_strlcpy (pcvals.selectedbrush,
                     fname, sizeof (pcvals.selectedbrush));

          updatebrushprev (fname);

          g_free (fname);
          g_free (brush);
        }
    }
}

static void
selectbrushfile (GtkTreeSelection *selection, gpointer data)
{
  brushfile = 1;
  gtk_widget_set_sensitive (presetsavebutton, TRUE);
  selectbrush (selection, NULL);
}

static void
brushaspectadjust_cb (GtkWidget *w, gpointer data)
{
  gimp_double_adjustment_update (GTK_ADJUSTMENT(w), data);
  updatebrushprev (pcvals.selectedbrush);
}

void
create_brushpage(GtkNotebook *notebook)
{
  GtkWidget *box1, *box2, *box3, *thispage;
  GtkWidget *view;
  GtkWidget *tmpw, *table;
  GtkWidget *frame;
  GtkWidget *combo;
  GtkWidget *label;
  GtkSizeGroup     *group;
  GtkTreeSelection *selection;

  label = gtk_label_new_with_mnemonic (_("_Brush"));

  thispage = gtk_vbox_new(FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show(thispage);

  box1 = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start(GTK_BOX(thispage), box1, TRUE,TRUE,0);
  gtk_widget_show (box1);

  view = createonecolumnlist (box1, selectbrushfile);
  brushlist = view;
  brushstore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  box2 = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX (box2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  brushprev = tmpw = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (tmpw), 100, 100);
  gtk_container_add (GTK_CONTAINER (frame), tmpw);
  gtk_widget_show (tmpw);

  box3 = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_end (GTK_BOX (box2), box3, FALSE, FALSE,0);
  gtk_widget_show (box3);

  tmpw = gtk_label_new( _("Gamma:"));
  gtk_misc_set_alignment (GTK_MISC (tmpw), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX (box3), tmpw, FALSE, FALSE,0);
  gtk_widget_show (tmpw);

  brushgammaadjust = gtk_adjustment_new(pcvals.brushgamma, 0.5, 3.0, 0.1,
					0.1, 1.0);
  tmpw = gtk_hscale_new(GTK_ADJUSTMENT(brushgammaadjust));
  gtk_widget_set_size_request (GTK_WIDGET(tmpw), 100, 30);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), FALSE);
  gtk_scale_set_digits(GTK_SCALE (tmpw), 2);
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect_swapped (brushgammaadjust, "value_changed",
			    G_CALLBACK(updatebrushprev), pcvals.selectedbrush);

  gimp_help_set_help_data
    (tmpw, _("Changes the gamma (brightness) of the selected brush"), NULL);

  box3 = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start(GTK_BOX (thispage), box3, FALSE, FALSE,0);
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
  gtk_box_pack_start(GTK_BOX(box3),tmpw, FALSE, FALSE, 0);
  g_signal_connect (tmpw, "clicked", G_CALLBACK(savebrush), NULL);
  gtk_widget_show(tmpw);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE(table), 6);
  gtk_table_set_row_spacings (GTK_TABLE(table), 6);
  gtk_box_pack_start(GTK_BOX (thispage), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  brushaspectadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 0,
			  _("Aspect ratio:"),
			  150, -1, pcvals.brushaspect,
			  -1.0, 1.0, 0.1, 0.1, 2,
			  TRUE, 0, 0,
			  _("Specifies the aspect ratio of the brush"),
			  NULL);
  gtk_size_group_add_widget (group,
                             GIMP_SCALE_ENTRY_LABEL (brushaspectadjust));
  g_signal_connect (brushaspectadjust, "value_changed",
                    G_CALLBACK (brushaspectadjust_cb), &pcvals.brushaspect);

  brushreliefadjust =
    gimp_scale_entry_new (GTK_TABLE(table), 0, 1,
			  _("Relief:"),
			  150, -1, pcvals.brushrelief,
			  0.0, 100.0, 1.0, 10.0, 1,
			  TRUE, 0, 0,
			  _("Specifies the amount of embossing to apply to the image (in percent)"),
			  NULL);
  gtk_size_group_add_widget (group,
                             GIMP_SCALE_ENTRY_LABEL (brushreliefadjust));
  g_signal_connect (brushreliefadjust, "value_changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.brushrelief);

  selectbrush(selection, NULL);
  readdirintolist("Brushes", view, pcvals.selectedbrush);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
