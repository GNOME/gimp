/**************************************************
 * file: megawidget/megawidget.c
 *
 * Copyright (c) 1997 Eric L. Hernes (erich@rrnet.com)
 * All rights reserved.
 *
 * !!
 * !! This file has been hacked to work with the flame plug-in
 * !!
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$ */

/* Functions added by Xavier Bouchoux (Xavier.Bouchoux@ensimag.imag.fr)
 *  mw_value_radio_group_new()
 *     -->  it modifies a user variable to a value associated with the current
 *          button 
 *  mw_ientry_button_new()
 *  mw_fentry_button_new()
 *     --->  Guess...
 *  mw_color_select_button_create()
 *     --> Creates a colored button, wich creates a colorselection window 
 *         (just like ifs compose)
 */


#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include "megawidget.h"


static void mw_scale_entry_new(GtkWidget *table, gchar *name,
                               gfloat defval, gfloat lorange,
                               gfloat hirange, gfloat st_inc,
                               gfloat pg_inc, gfloat pgsiz,
                               gpointer variablep, gchar *fmtvar,
                               gint left_a, gint right_a, gint top_a,
                               gint bottom_a, GtkCallback scale_cb,
                               GtkCallback entry_cb);
static void ui_ok_callback(GtkWidget *widget, gpointer data);
static void ui_close_callback(GtkWidget *widget, gpointer data);
static void ui_fscale_callback(GtkAdjustment *adj, gpointer data);
static void ui_fentry_callback(GtkWidget *widget, gpointer data);
static void ui_iscale_callback(GtkAdjustment *adj, gpointer data);
static void ui_ientry_callback(GtkWidget *widget, gpointer data);


mw_callback mw_update_cb;

#ifndef NO_PREVIEW
static mw_preview_t *mw_do_preview = NULL;
static gint do_preview = 1;
#endif

GtkWidget *
mw_app_new(gchar *resname, gchar *appname, gint *runpp){
   gchar **argv;
   gint argc;
   GtkWidget *dlg;
   GtkWidget *button;
   
   argc = 1;
   argv = g_new(gchar *, 1);
   *runpp = 0;

   argv[0] = g_strdup(resname);
   gtk_init(&argc, &argv);

   dlg = gtk_dialog_new();
   gtk_object_set_data(GTK_OBJECT(dlg), "runp", runpp);

   gtk_window_set_title(GTK_WINDOW(dlg), appname);
   gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
   gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
                      (GtkSignalFunc) ui_close_callback,
                      NULL);

   button = gtk_button_new_with_label("OK");
   GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
   gtk_signal_connect(GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) ui_ok_callback,
                      dlg);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button,
                      TRUE, TRUE, 0);
   gtk_widget_grab_default(button);
   gtk_widget_show(button);

   button = gtk_button_new_with_label("Cancel");
   GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
   gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT(dlg));
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button,
                      TRUE, TRUE, 0);
   gtk_widget_show(button);

   return dlg;
}

void
mw_fscale_entry_new(GtkWidget *table, gchar *name,
                    gfloat lorange, gfloat hirange,
                    gfloat st_inc, gfloat pg_inc, gfloat pgsiz,
                    gint left_a, gint right_a, gint top_a, gint bottom_a,
                    gdouble *var) {
   gchar buffer[40];

   sprintf(buffer, "%0.3f", *var);
   mw_scale_entry_new(table, name, *var, lorange, hirange,
                      st_inc, pg_inc, pgsiz, var, buffer,
                      left_a,  right_a,  top_a,  bottom_a,
                      (GtkCallback)ui_fscale_callback,
                      (GtkCallback)ui_fentry_callback);
}

void
mw_iscale_entry_new(GtkWidget *table, gchar *name,
                    gint lorange, gint hirange,
                    gint st_inc, gint pg_inc, gint pgsiz,
                    gint left_a, gint right_a, gint top_a, gint bottom_a,
                    gint *varp) {
   gchar buffer[40];

   sprintf(buffer, "%d", *varp);
   
   mw_scale_entry_new(table, name, (gfloat)*varp, (gfloat)lorange,
                      (gfloat)hirange, (gfloat)st_inc, (gfloat)pg_inc,
                      (gfloat)pgsiz, varp, buffer,
                      left_a,  right_a,  top_a,  bottom_a,
                      (GtkCallback)ui_iscale_callback,
                      (GtkCallback)ui_ientry_callback);
}

#ifndef NO_PREVIEW
struct mwPreview *
mw_preview_build_virgin(GDrawable *drw) {
   struct mwPreview *mwp;

   mwp = (struct mwPreview *)malloc(sizeof(struct mwPreview));
   if (drw->width > drw->height) {
      mwp->scale = (gdouble)drw->width/(gdouble)PREVIEW_SIZE;
      mwp->width = PREVIEW_SIZE;
      mwp->height = (drw->height)/(mwp->scale);
   } else {
      mwp->scale = (gdouble)drw->height/(gdouble)PREVIEW_SIZE;
      mwp->height = PREVIEW_SIZE;
      mwp->width = (drw->width)/(mwp->scale);
   }

   mwp->bpp = 3;
   mwp->bits = NULL;
   return(mwp);
}

struct mwPreview *
mw_preview_build(GDrawable *drw) {
   struct mwPreview *mwp;
   gint x, y, b;
   guchar *bc, *drwBits;
   GPixelRgn pr;

   mwp= mw_preview_build_virgin(drw);

   gimp_pixel_rgn_init(&pr, drw, 0, 0, drw->width, drw->height, FALSE, FALSE);
   drwBits = (guchar *)malloc(drw->width * drw->bpp);

   mwp->bpp = 3;
   bc = mwp->bits = (guchar *)malloc(mwp->width*mwp->height*mwp->bpp);
   for(y=0;y<mwp->height;y++) {
      gimp_pixel_rgn_get_row(&pr, drwBits, 0, (int)(y*mwp->scale), drw->width);
      for(x=0;x<mwp->width;x++) {
         for(b=0;b<mwp->bpp;b++)
            *bc++=*(drwBits+((gint)(x*mwp->scale)*drw->bpp)+b);
      }
   }
   free(drwBits);
   return(mwp);
}


GtkWidget *
mw_preview_new(GtkWidget *parent, struct mwPreview *mwp, mw_preview_t *fcn){
   GtkWidget *preview;
   GtkWidget *frame;
   GtkWidget *pframe;
   GtkWidget *vbox;
   GtkWidget *button;
   guchar *color_cube;
   
   gtk_preview_set_gamma (gimp_gamma ());
   gtk_preview_set_install_cmap (gimp_install_cmap ());
   color_cube = gimp_color_cube ();
   gtk_preview_set_color_cube (color_cube[0], color_cube[1],
                               color_cube[2], color_cube[3]);

   gtk_widget_set_default_visual (gtk_preview_get_visual ());
   gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

   frame = gtk_frame_new("Preview");
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   gtk_container_border_width(GTK_CONTAINER(frame), 10);
   gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, FALSE, 0);
   gtk_widget_show(frame);

   vbox = gtk_vbox_new(FALSE, 2);
   gtk_container_border_width(GTK_CONTAINER(vbox), 2);
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   gtk_widget_show(vbox);

   pframe = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(pframe), GTK_SHADOW_IN);
   gtk_container_border_width(GTK_CONTAINER(pframe), 3);
   gtk_box_pack_start(GTK_BOX(vbox), pframe, FALSE, FALSE, 0);
   gtk_widget_show(pframe);

   preview = gtk_preview_new(GTK_PREVIEW_COLOR);
   gtk_preview_size(GTK_PREVIEW(preview), mwp->width, mwp->height);
   gtk_container_add(GTK_CONTAINER(pframe), preview);
   gtk_widget_show(preview);
   mw_do_preview = fcn;

   button=gtk_check_button_new_with_label("Do Preview");
   gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(button), do_preview);
   gtk_signal_connect(GTK_OBJECT(button), "toggled",
                      (GtkSignalFunc) ui_toggle_callback,
                      &do_preview);
   gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
   gtk_widget_show(button);

   return preview;
}
#endif /* NO_PREVIEW */

/* internals */
static void
mw_scale_entry_new(GtkWidget *table, gchar *name,
                   gfloat defval, gfloat lorange, gfloat hirange,
                   gfloat st_inc, gfloat pg_inc, gfloat pgsiz,
                   gpointer variablep, gchar *fmtvar,
                   gint left_a, gint right_a, gint top_a, gint bottom_a,
                   GtkCallback scale_cb, GtkCallback entry_cb) {
   GtkWidget *label;
   GtkWidget *hbox;
   GtkWidget *scale;
   GtkWidget *entry;
   GtkObject *adjustment;

   label = gtk_label_new(name);
   gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

   gtk_table_attach(GTK_TABLE(table), label,
                    left_a, right_a, top_a, bottom_a,
                    GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(label);

   hbox = gtk_hbox_new(FALSE, 5);
   gtk_table_attach(GTK_TABLE(table), hbox,
                    left_a+1, right_a+1, top_a, bottom_a, 
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(hbox);
   
   adjustment = gtk_adjustment_new(defval, lorange, hirange, st_inc,
                                   pg_inc, pgsiz);
   gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
                      (GtkSignalFunc) scale_cb,
                      variablep);

   scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
   gtk_widget_set_usize(GTK_WIDGET(scale), 140, 0);
   gtk_scale_set_digits(GTK_SCALE(scale), 2);
   gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
   gtk_box_pack_start(GTK_BOX(hbox), scale, TRUE, TRUE, 0);
   gtk_widget_show(scale);

   entry = gtk_entry_new();
   gtk_object_set_user_data(GTK_OBJECT(entry), adjustment);
   gtk_object_set_user_data(adjustment, entry);
   gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
   gtk_widget_set_usize(entry, 75, 0);

   gtk_entry_set_text(GTK_ENTRY(entry), fmtvar);
   gtk_signal_connect(GTK_OBJECT(entry), "changed",
                      (GtkSignalFunc) entry_cb,
                      variablep);
   gtk_widget_show(entry);

}



static void
ui_close_callback(GtkWidget *widget, gpointer data){
   gtk_main_quit();
}

static void
ui_ok_callback(GtkWidget *widget, gpointer data){
   gint *rp;
   rp = gtk_object_get_data(GTK_OBJECT(data), "runp");
   *rp=1;
   gtk_widget_destroy(GTK_WIDGET(data));
}

static void
ui_fscale_callback(GtkAdjustment *adj, gpointer data){
  GtkWidget *ent;
  double *nval;
  gchar buf[40];

  nval = (double*)data;

  if (*nval != adj->value) {
    *nval = adj->value;
    ent = gtk_object_get_user_data(GTK_OBJECT(adj));
    sprintf(buf, "%0.2f", adj->value);

    gtk_signal_handler_block_by_data(GTK_OBJECT(ent), data);
    gtk_entry_set_text(GTK_ENTRY(ent), buf);
    gtk_signal_handler_unblock_by_data(GTK_OBJECT(ent), data);

    if (mw_update_cb) (*mw_update_cb)(data);

#ifndef NO_PREVIEW
    if (do_preview && mw_do_preview!=NULL) (*mw_do_preview)(NULL);
#endif
  }
}

static void
ui_fentry_callback(GtkWidget *widget, gpointer data){
  GtkAdjustment *adjustment;
  double new_val;
  double *val;

  val = (double*)data;
  new_val = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

  if (*val != new_val) {
    adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

    if ((new_val >= adjustment->lower) &&
	(new_val <= adjustment->upper)) {
      *val = new_val;
      adjustment->value = new_val;
      gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");
    }
    if (mw_update_cb) (*mw_update_cb)(data);
#ifndef NO_PREVIEW
    if (do_preview && mw_do_preview!=NULL) (*mw_do_preview)(NULL);
#endif
  }
}

static void
ui_iscale_callback(GtkAdjustment *adj, gpointer data){
  GtkWidget *ent;
  gint *nval;
  gchar buf[40];

  nval = (gint*)data;

  if (*nval != (gint)adj->value) {
    *nval = (gint)adj->value;
    ent = gtk_object_get_user_data(GTK_OBJECT(adj));
    sprintf(buf, "%d", (gint)adj->value);
    gtk_entry_set_text(GTK_ENTRY(ent), buf);
    if (mw_update_cb) (*mw_update_cb)(data);
#ifndef NO_PREVIEW
    if (do_preview && mw_do_preview!=NULL) (*mw_do_preview)(NULL);
#endif
  }
}

static void
ui_ientry_callback(GtkWidget *widget, gpointer data){
  GtkAdjustment *adj;
  gint new_val;
  gint *val;

  val = (gint *)data;
  new_val = strtol(gtk_entry_get_text(GTK_ENTRY(widget)), NULL, 0);

  if (*val != new_val) {
    adj = gtk_object_get_user_data(GTK_OBJECT(widget));

    if ((new_val >= adj->lower) &&
	(new_val <= adj->upper)) {
      *val = new_val;
      adj->value = new_val;
      gtk_signal_emit_by_name(GTK_OBJECT(adj), "value_changed");
    }
    if (mw_update_cb) (*mw_update_cb)(data);
#ifndef NO_PREVIEW
    if (do_preview && mw_do_preview!=NULL) (*mw_do_preview)(NULL);
#endif
  }
}



/* end of megawidget/megawidget.c */


