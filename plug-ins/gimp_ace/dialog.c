/* dialog.c: user interface for GIMP Adaptive Contrast Enhancment plug-in */
/* Copyright (C) 1999 Kevin M. Turner <kevint@poboxes.com> */
/* $Id$ */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.  See the file COPYING for details.
 *
 */

#include <config.h> /* autostuff */

#include <stdio.h> /* for asprintf(wintitle,...) */

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include <libintl.h> /* i18n - gettext */
#define _(String) gettext (String)
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
# define N_(String) (String)
#endif

#include "gimp_ace.h" /* AceValues */

static void ok_callback  (GtkWidget *widget, gpointer data);
static void close_callback  (GtkWidget *widget, gpointer data);
static GtkWidget *ace_dialog_new(GtkSignalFunc ok_callback, 
				GtkSignalFunc close_callback,
				gboolean *go_ahead);
static void adjust_double_changed (GtkAdjustment *adj, 
				   gpointer *unused);
static void adjust_uint_changed   (GtkAdjustment *adj, 
				   gpointer *unused);

static void omenu_callback (GtkWidget *menuitem, 
			    gpointer data);

static void toggled_cb (GtkToggleButton *toggle, 
			gboolean *data);

static void strength_changed   (GtkAdjustment *strength_adj, 
				GtkAdjustment *bradj_adj);
static void bradj_changed   (GtkAdjustment *bradj_adj, 
			     gpointer unused);

#define SPACING 4
#define HBOX_SPACING SPACING
#define DIALOG_BORDER_WIDTH 0
#define OUTER_BORDER_WIDTH 6
#define ACTION_AREA_BORDER_WIDTH OUTER_BORDER_WIDTH
#define TOP_AREA_BORDER_WIDTH OUTER_BORDER_WIDTH
#define TABLE_ROW_SPACING SPACING
#define TABLE_COL_SPACING SPACING

#define SPIN_ATTOPT_X GTK_EXPAND | GTK_FILL
#define SPIN_ATTOPT_Y 0

#define SCALE_ATTOPT_X GTK_EXPAND | GTK_FILL
#define SCALE_ATTOPT_Y 0

#define LABEL_ATTOPT 0

gboolean ace_dialog(guint32 drawable_id, AceValues *acevals)
{
	GtkWidget *dlg;

	GtkWidget *vbox1;
	GtkWidget *table1;
	GtkObject *bradj_scale_adj;
	GtkWidget *bradj_scale;
	GtkObject *strength_scale_adj;
	GtkWidget *strength_scale;
	GtkWidget *link_toggle;
	GtkWidget *label;
	GtkWidget *hseparator;
	GtkWidget *table2;
	GtkObject *iter_spin_adj;
	GtkWidget *iter_spin;
	GtkObject *size_spin_adj;
	GtkWidget *size_spin;
	GtkObject *smooth_spin_adj;
	GtkWidget *smooth_spin;
	GtkObject *foobarum_spin_adj;
	GtkWidget *foobarum_spin;

	GtkTooltips *tooltips;

	GtkWidget *menuitem, *color_menu, *color_omenu;
	GtkWidget *frame;

	gboolean go_ahead=FALSE;

#if 0
        printf("Waiting... (pid %d)\n", getpid());
        kill(getpid(), 19);
#endif
	
	/* Standard gimp_dialog creation. */
	dlg = ace_dialog_new((GtkSignalFunc) ok_callback,
			     (GtkSignalFunc) close_callback,
			     &go_ahead);

	tooltips=gtk_tooltips_new();

	/* Neato Box */
	vbox1 = gtk_vbox_new (FALSE, SPACING);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox),vbox1, 
			    TRUE, TRUE, 0);
	gtk_container_border_width (GTK_CONTAINER (vbox1), 
				    TOP_AREA_BORDER_WIDTH);

	/* Table which holds sliders */
	table1 = gtk_table_new (2, 3, FALSE);
	gtk_box_pack_start (GTK_BOX (vbox1), table1, TRUE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table1), SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table1), SPACING);

	/* slider: strength */
	strength_scale_adj=gtk_adjustment_new (acevals->strength, 
					       0, 1, 0.01, 0.1, 0);
	strength_scale = gtk_hscale_new (GTK_ADJUSTMENT (strength_scale_adj));
	gtk_widget_set_name (strength_scale, "strength_scale");
	gtk_table_attach (GTK_TABLE (table1), strength_scale, 1, 2, 0, 1,
			  SCALE_ATTOPT_X, SCALE_ATTOPT_Y, 
			  0, 0);
	gtk_scale_set_value_pos (GTK_SCALE (strength_scale), GTK_POS_LEFT);
	gtk_scale_set_digits (GTK_SCALE (strength_scale), 2);
	gtk_range_set_update_policy (GTK_RANGE (strength_scale), 
				     GTK_UPDATE_DELAYED);

	/* slider: Brightness adjust */
	bradj_scale_adj=gtk_adjustment_new (acevals->bradj, 
					    0, 1, 0.01, 0.1, 0);
	bradj_scale = gtk_hscale_new (GTK_ADJUSTMENT (bradj_scale_adj));
	gtk_widget_set_name (bradj_scale, "bradj_scale");
	gtk_table_attach (GTK_TABLE (table1), bradj_scale, 1, 2, 1, 2,
			  SCALE_ATTOPT_X, SCALE_ATTOPT_Y, 
			  0, 0);
	gtk_scale_set_value_pos (GTK_SCALE (bradj_scale), GTK_POS_LEFT);
	gtk_scale_set_digits (GTK_SCALE (bradj_scale), 2);
	gtk_range_set_update_policy (GTK_RANGE (bradj_scale), 
				     GTK_UPDATE_DELAYED);

	/* Link toggle */
	link_toggle = gtk_toggle_button_new_with_label(_("Link"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(link_toggle),
				     acevals->link);
	gtk_signal_connect (GTK_OBJECT(link_toggle),"toggled",
			    GTK_SIGNAL_FUNC(toggled_cb),
			    &(acevals->link));
	gtk_table_attach (GTK_TABLE (table1), link_toggle, 2, 3, 0, 2,
			  LABEL_ATTOPT, LABEL_ATTOPT, 
			  0, 0);

	/* Callbacks for above adjustments */
	gtk_object_set_user_data(GTK_OBJECT(strength_scale_adj),
				 &(acevals->strength));
	gtk_signal_connect (GTK_OBJECT (strength_scale_adj), "value_changed",
                            GTK_SIGNAL_FUNC (strength_changed), 
			    bradj_scale_adj);

	gtk_object_set_user_data(GTK_OBJECT(bradj_scale_adj),
				 &(acevals->bradj));
	gtk_object_set_data(GTK_OBJECT(bradj_scale_adj),
			    "link", link_toggle);
	gtk_signal_connect (GTK_OBJECT (bradj_scale_adj), "value_changed",
                            GTK_SIGNAL_FUNC (bradj_changed), 
			    NULL);
	
	/* Labels for brightness adjust and strength */
	label = gtk_label_new (_("Strength:"));
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 0, 1,
			  LABEL_ATTOPT, LABEL_ATTOPT,
			  0, 0);

	label = gtk_label_new (_("Brightness Adjust:"));
	gtk_table_attach (GTK_TABLE (table1), label, 0, 1, 1, 2,
			  LABEL_ATTOPT, LABEL_ATTOPT,
			  0, 0);	

	/* Seperator */
	hseparator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (vbox1), hseparator, FALSE, FALSE, 0);

	/* Table which holds spinbuttons */
	/* (iterations, size, threshold) */

	table2 = gtk_table_new (2, 4, FALSE);
	gtk_box_pack_start (GTK_BOX (vbox1), table2, TRUE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table2), SPACING);
	gtk_table_set_col_spacings (GTK_TABLE (table2), SPACING);

	/* Iteration spin */
	iter_spin_adj = gtk_adjustment_new (acevals->iterations, 
					    1, 200, 1, 10, 0);
	iter_spin = gtk_spin_button_new (GTK_ADJUSTMENT (iter_spin_adj), 
					 1, 0);
	gtk_widget_set_name (iter_spin, "iteration_spin");
	gtk_table_attach (GTK_TABLE (table2), iter_spin, 0, 1, 1, 2,
			  SPIN_ATTOPT_X, SPIN_ATTOPT_Y,
			  0, 0);
	gtk_object_set_user_data(GTK_OBJECT(iter_spin_adj),
				 &(acevals->iterations));
	gtk_signal_connect (GTK_OBJECT (iter_spin_adj), "value_changed",
                            GTK_SIGNAL_FUNC (adjust_uint_changed), NULL);

	/* Iteration label */
	label = gtk_label_new (_("Iterations:"));
	gtk_table_attach (GTK_TABLE (table2), label, 0, 1, 0, 1,
			  LABEL_ATTOPT, LABEL_ATTOPT,
			  0, 0);
		
	/* size spin */
	size_spin_adj = gtk_adjustment_new (acevals->wsize, 
					    4, 128, 1, 10, 0);
	size_spin = gtk_spin_button_new (GTK_ADJUSTMENT (size_spin_adj), 1, 0);
	gtk_widget_set_name (size_spin, "size_spin");
	gtk_table_attach (GTK_TABLE (table2), size_spin, 1, 2, 1, 2,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 
			  0, 0);
	gtk_object_set_user_data(GTK_OBJECT(size_spin_adj),
				 &(acevals->wsize));
	gtk_signal_connect (GTK_OBJECT (size_spin_adj), "value_changed",
                            GTK_SIGNAL_FUNC (adjust_uint_changed), NULL);

	/* size label */
	label = gtk_label_new (_("Detail Size:"));
	gtk_table_attach (GTK_TABLE (table2), label, 1, 2, 0, 1,
			  LABEL_ATTOPT, LABEL_ATTOPT,
			  0, 0);

	/* smoothing spin */
	smooth_spin_adj = gtk_adjustment_new (acevals->smoothing,
						0, 100, 5, 10, 0);
	smooth_spin = gtk_spin_button_new (GTK_ADJUSTMENT (smooth_spin_adj),
					     1, 0);
	gtk_widget_set_name (smooth_spin, "smooth_spin");
	gtk_table_attach (GTK_TABLE (table2), smooth_spin, 2, 3, 1, 2,
			  SPIN_ATTOPT_X, SPIN_ATTOPT_Y,
			  0, 0);
	gtk_object_set_user_data(GTK_OBJECT(smooth_spin_adj),
				 &(acevals->smoothing));
	gtk_signal_connect (GTK_OBJECT (smooth_spin_adj), "value_changed",
                            GTK_SIGNAL_FUNC (adjust_double_changed), NULL);

	/* smoothing label */
	label = gtk_label_new (_("Smoothing:"));
	gtk_table_attach (GTK_TABLE (table2), label, 2, 3, 0, 1,
			  LABEL_ATTOPT, LABEL_ATTOPT,
			  0, 0);

	/* foobarum spin */
	foobarum_spin_adj = gtk_adjustment_new (acevals->coefftol,
						0, 1, .001, .01, 0);
	foobarum_spin = gtk_spin_button_new (GTK_ADJUSTMENT (foobarum_spin_adj),
					     1, 3);
	gtk_widget_set_name (foobarum_spin, "foobarum_spin");
	gtk_table_attach (GTK_TABLE (table2), foobarum_spin, 3, 4, 1, 2,
			  SPIN_ATTOPT_X, SPIN_ATTOPT_Y,
			  0, 0);
	gtk_object_set_user_data(GTK_OBJECT(foobarum_spin_adj),
				 &(acevals->coefftol));
	gtk_signal_connect (GTK_OBJECT (foobarum_spin_adj), "value_changed",
                            GTK_SIGNAL_FUNC (adjust_double_changed), NULL);

	/* foobarum label */
	label = gtk_label_new (_("foobarm:"));
	gtk_table_attach (GTK_TABLE (table2), label, 3, 4, 0, 1,
			  LABEL_ATTOPT, LABEL_ATTOPT,
			  0, 0);

	/** Color method **/
	frame=gtk_frame_new(_("Color Overflow"));

	color_menu=gtk_menu_new();

	menuitem = gtk_menu_item_new_with_label(_("limit"));
	gtk_menu_append(GTK_MENU(color_menu), menuitem);
	gtk_object_set_user_data(GTK_OBJECT(menuitem),
				 GUINT_TO_POINTER(GLACE_COLOR_Yxy));
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   GTK_SIGNAL_FUNC(omenu_callback),
			   &(acevals->color_method));	
	
	menuitem = gtk_menu_item_new_with_label(_("add white"));
	gtk_menu_append(GTK_MENU(color_menu), menuitem);
	gtk_object_set_user_data(GTK_OBJECT(menuitem),
				 GUINT_TO_POINTER(GLACE_COLOR_LUMIN));
	gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
			   GTK_SIGNAL_FUNC(omenu_callback),
			   &(acevals->color_method));	

	gtk_widget_show_all(color_menu);

	color_omenu = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(color_omenu),
				 color_menu);

	switch(acevals->color_method) {
	case GLACE_COLOR_Yxy:
		gtk_option_menu_set_history(GTK_OPTION_MENU(color_omenu), 0);
		break;
	case GLACE_COLOR_LUMIN:
		gtk_option_menu_set_history(GTK_OPTION_MENU(color_omenu), 1);
		break;
	}

	gtk_container_add(GTK_CONTAINER(frame), color_omenu);
	gtk_box_pack_start (GTK_BOX(vbox1), frame, FALSE, TRUE, 0);

	/* done */
	
        gtk_widget_show_all(dlg);

        gtk_main();

	return(go_ahead);
}

/* ???_dialog_new: */
/* This should probably be generalized into a libgimpui function,
   but there are many different strings for argv0, wmclass,
   and window title, and I find it easier to hardcode them now. */
static GtkWidget *ace_dialog_new(GtkSignalFunc ok_callback, 
			       GtkSignalFunc close_callback,
			       gboolean *go_ahead)
{
	GtkWidget *dlg;
	GtkWidget *button;

	guchar *color_cube;

	gchar *wintitle;

	gchar **argv;
	gint  argc;

	/* It's desirable to have an argv0 to pass to gtk_init,
	   so that GTK+ knows what the program's name is... */
	argc = 1;
	argv = g_new (gchar *, 1);
	argv[0] = g_strdup ("gimp_ace");
       
	gtk_init (&argc, &argv);
	
	gtk_rc_parse (gimp_gtkrc ());

	/* FIXME: I don't understand any of this colormapcube stuff. */
        gtk_preview_set_gamma(gimp_gamma());
        gtk_preview_set_install_cmap(gimp_install_cmap());
        color_cube = gimp_color_cube();
	gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);  

        gtk_widget_set_default_visual(gtk_preview_get_visual());
        gtk_widget_set_default_colormap(gtk_preview_get_cmap());

	dlg = gtk_dialog_new ();
	
	gtk_window_set_wmclass(GTK_WINDOW(dlg), 
			       "plug-in-ace", 
			       "Gimp");

	gtk_widget_set_name(dlg,"plug-in-ace");
	
	/* This window title includes the version string. */
	wintitle = g_strdup_printf(_("Adaptive Contrast Enhancement v %s"), 
				   VERSION);
	gtk_window_set_title (GTK_WINDOW (dlg), 
			      wintitle);
	g_free(wintitle);
	gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
	gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
			    close_callback,
			    NULL);

	/* Various border settings... */
	gtk_container_border_width(GTK_CONTAINER(dlg), 
				   DIALOG_BORDER_WIDTH);
	gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dlg)->action_area),
				   ACTION_AREA_BORDER_WIDTH);
	/* Action Area */

	gtk_widget_set_name(GTK_DIALOG(dlg)->action_area,"action_area");

	button = gtk_button_new_with_label(_("OK"));
	gtk_widget_set_name(button,"ok_button");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
                           GTK_SIGNAL_FUNC(ok_callback),
                           go_ahead);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), 
			   button, TRUE, TRUE, 0);
        gtk_widget_grab_default(button);

        button = gtk_button_new_with_label(_("Cancel"));
	gtk_widget_set_name(button,"cancel_button");
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_signal_connect(GTK_OBJECT(button), "clicked",
                           close_callback,
			   NULL);

        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), 
			   button, TRUE, TRUE, 0);

	/* FIXME: I'm missing a HELP button. */

	return dlg;
}

static void 
adjust_uint_changed (GtkAdjustment *adj, gpointer *unused)
{
	guint *variable;
	variable=gtk_object_get_user_data(GTK_OBJECT(adj));
	*variable=(guint)(adj->value);

/*	dialog_update_preview(pre_info); */
}

static void 
adjust_double_changed (GtkAdjustment *adj, gpointer *unused)
{
	gdouble *variable;
	variable=gtk_object_get_user_data(GTK_OBJECT(adj));
	*variable=(gdouble)(adj->value);

/*	dialog_update_preview(pre_info); */
}

static void
toggled_cb (GtkToggleButton *toggle, gboolean *data)
{
	*data=toggle->active;
}

static void 
strength_changed (GtkAdjustment *strength_adj, GtkAdjustment *bradj_adj)
{
	GtkToggleButton *link_toggle = NULL;
	gdouble *strength;
	strength=gtk_object_get_user_data (GTK_OBJECT(strength_adj));
	*strength=(gdouble)(strength_adj->value);

	link_toggle=
		GTK_TOGGLE_BUTTON(gtk_object_get_data(GTK_OBJECT(bradj_adj),
						      "link"));

	if(link_toggle->active)
		gtk_adjustment_set_value (bradj_adj, *strength);
}

static void 
bradj_changed (GtkAdjustment *bradj_adj, gpointer unused)
{
	GtkToggleButton *link_toggle = NULL;
	gdouble *bradj_p=NULL;
	gdouble bradj=(gdouble)(bradj_adj->value);
	
	if(TRUE) {
		bradj_p = gtk_object_get_user_data (GTK_OBJECT(bradj_adj));
		*bradj_p = bradj;
	}

	if(FALSE) {
	link_toggle=
		GTK_TOGGLE_BUTTON(gtk_object_get_data(GTK_OBJECT(bradj_adj),
						      "link"));
	gtk_toggle_button_set_active(link_toggle, FALSE);
	}

}

static void
omenu_callback (GtkWidget *menuitem, gpointer data)
{
	*((Glace_ColorMethods*)data) =
		(Glace_ColorMethods)GPOINTER_TO_UINT(
			gtk_object_get_user_data(GTK_OBJECT(menuitem)));
	printf("option menu set data to %d\n",
	       *((Glace_ColorMethods*)data));
}


static void 
ok_callback  (GtkWidget *unused, gpointer data)
{
	gboolean *go_ahead=(gboolean*)data;

	*go_ahead=TRUE;

	gtk_main_quit();
}

static void 
close_callback  (GtkWidget *unused_widget, gpointer unused_data)
{
	gtk_main_quit();
}
