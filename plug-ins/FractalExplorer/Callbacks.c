#include "FractalExplorer.h"
#include "Callbacks.h"
#include "Dialogs.h"
#include <stdlib.h>

/**********************************************************************
 FUNCTION: dialog_close_callback
 *********************************************************************/

void
dialog_close_callback(GtkWidget * widget, gpointer data)
{
    gtk_main_quit();
}				/* dialog_close_callback */

/**********************************************************************
 FUNCTION: dialog_save_callback
 *********************************************************************/

void
dialog_save_callback(GtkWidget * widget, gpointer data)
{
    create_file_selection();
}				/* dialog_save_callback */
/**********************************************************************
 FUNCTION: load_button_press
 *********************************************************************/

void
load_button_press(GtkWidget * widget,
		  gpointer data)
{
   create_load_file_selection();
}


/**********************************************************************
 FUNCTION: dialog_ok_callback
 *********************************************************************/

void
dialog_ok_callback(GtkWidget * widget, gpointer data)
{
    wint.run = TRUE;
    gtk_widget_destroy(GTK_WIDGET(data));
}				/* dialog_ok_callback */

/**********************************************************************
 FUNCTION: dialog_reset_callback
 *********************************************************************/

void
dialog_reset_callback(GtkWidget * widget, gpointer data)
{
    wvals.xmin = standardvals.xmin;
    wvals.xmax = standardvals.xmax;
    wvals.ymin = standardvals.ymin;
    wvals.ymax = standardvals.ymax;
    wvals.iter = standardvals.iter;
    wvals.cx = standardvals.cx;
    wvals.cy = standardvals.cy;
    dialog_change_scale();
    set_cmap_preview();
    dialog_update_preview();
}

/**********************************************************************
 FUNCTION: dialog_redraw_callback
 *********************************************************************/

void
dialog_redraw_callback(GtkWidget * widget, gpointer data)
{
    int                 alwaysprev = wvals.alwayspreview;
    wvals.alwayspreview = TRUE;
    set_cmap_preview();
    dialog_update_preview();
    wvals.alwayspreview = alwaysprev;
}

/**********************************************************************
 FUNCTION: dialog_cancel_callback
 *********************************************************************/

void
dialog_cancel_callback(GtkWidget * widget, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}				/* dialog_cancel_callback */

/**********************************************************************
 FUNCTION: dialog_undo_zoom_callback
 *********************************************************************/

void
dialog_undo_zoom_callback(GtkWidget * widget, gpointer data)
{
    if (zoomindex > 1) {
	zooms[zoomindex] = wvals;
	zoomindex--;
        wvals = zooms[zoomindex];
        dialog_change_scale();
        set_cmap_preview();
        dialog_update_preview();
    }
}				/* dialog_undo_zoom_callback */

/**********************************************************************
 FUNCTION: dialog_redo_zoom_callback
 *********************************************************************/

void
dialog_redo_zoom_callback(GtkWidget * widget, gpointer data)
{
    if (zoomindex < zoommax) {
	zoomindex++;
        wvals = zooms[zoomindex];
        dialog_change_scale();
        set_cmap_preview();
        dialog_update_preview();
    }
}				/* dialog_redo_zoom_callback */

/**********************************************************************
 FUNCTION: dialog_step_in_callback
 *********************************************************************/

void
dialog_step_in_callback(GtkWidget * widget, gpointer data)
{
    double xdifferenz;
    double ydifferenz;
    if (zoomindex < zoommax) {
        zooms[zoomindex]=wvals;
	zoomindex++;
    }
    xdifferenz=wvals.xmax-wvals.xmin;
    ydifferenz=wvals.ymax-wvals.ymin;
    wvals.xmin+=1.0/6.0*xdifferenz;
    wvals.ymin+=1.0/6.0*ydifferenz;
    wvals.xmax-=1.0/6.0*xdifferenz;
    wvals.ymax-=1.0/6.0*ydifferenz;
    zooms[zoomindex]=wvals;
    dialog_change_scale();
    set_cmap_preview();
    dialog_update_preview();
}				/* dialog_step_in_callback */

/**********************************************************************
 FUNCTION: dialog_step_out_callback
 *********************************************************************/

void
dialog_step_out_callback(GtkWidget * widget, gpointer data)
{
    double xdifferenz;
    double ydifferenz;
    if (zoomindex < zoommax) {
        zooms[zoomindex]=wvals;
	zoomindex++;
    }
    xdifferenz=wvals.xmax-wvals.xmin;
    ydifferenz=wvals.ymax-wvals.ymin;
    wvals.xmin-=1.0/4.0*xdifferenz;
    wvals.ymin-=1.0/4.0*ydifferenz;
    wvals.xmax+=1.0/4.0*xdifferenz;
    wvals.ymax+=1.0/4.0*ydifferenz;
    zooms[zoomindex]=wvals;
    dialog_change_scale();
    set_cmap_preview();
    dialog_update_preview();
}				/* dialog_step_out_callback */

/**********************************************************************
 FUNCTION: explorer_logo_ok_callback
 *********************************************************************/

void
explorer_logo_ok_callback(GtkWidget * widget, gpointer data)
{
    gtk_widget_set_sensitive(maindlg, TRUE);
    gtk_widget_destroy(logodlg);
}

/**********************************************************************
 FUNCTION: explorer_about_callback
 *********************************************************************/

void
explorer_about_callback(GtkWidget * widget, gpointer data)
{
    gtk_widget_set_sensitive(maindlg, FALSE);
    explorer_logo_dialog();
}

/**********************************************************************
 FUNCTION: explorer_toggle_update
 *********************************************************************/

void
explorer_toggle_update(GtkWidget * widget,
		       gpointer data)
{
    int                *toggle_val;

    toggle_val = (int *) data;

    if (GTK_TOGGLE_BUTTON(widget)->active)
	*toggle_val = TRUE;
    else
	*toggle_val = FALSE;

    if (do_redsinus)
	wvals.redmode = SINUS;
    else if (do_redcosinus)
	wvals.redmode = COSINUS;
    else if (do_rednone)
	wvals.redmode = NONE;

    if (do_greensinus)
	wvals.greenmode = SINUS;
    else if (do_greencosinus)
	wvals.greenmode = COSINUS;
    else if (do_greennone)
	wvals.greenmode = NONE;

    if (do_bluesinus)
	wvals.bluemode = SINUS;
    else if (do_bluecosinus)
	wvals.bluemode = COSINUS;
    else if (do_bluenone)
	wvals.bluemode = NONE;

    if (do_colormode1)
	wvals.colormode = 0;
    else if (do_colormode2)
	wvals.colormode = 1;
	
    if (do_type0)
	wvals.fractaltype = 0;
    else if (do_type1)
	wvals.fractaltype = 1;
    else if (do_type2)
	wvals.fractaltype = 2;
    else if (do_type3)
	wvals.fractaltype = 3;
    else if (do_type4)
	wvals.fractaltype = 4;
    else if (do_type5)
	wvals.fractaltype = 5;
    else if (do_type6)
	wvals.fractaltype = 6;
    else if (do_type7)
	wvals.fractaltype = 7;
    else if (do_type8)
	wvals.fractaltype = 8;

    set_cmap_preview();
    dialog_update_preview();
}

/**********************************************************************
 FUNCTION: dialog_scale_update
 *********************************************************************/

void
dialog_scale_update(GtkAdjustment * adjustment, gdouble * value)
{
    GtkWidget          *entry;
    char                buf[MAXSTRLEN];

    if (*value != adjustment->value) {
	*value = adjustment->value;

	entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
	sprintf(buf, "%0.15f", *value);

	gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
	set_cmap_preview();
	dialog_update_preview();
    }
}				/* dialog_scale_update */

/**********************************************************************
 FUNCTION: dialog_scale_int_update
 *********************************************************************/

void
dialog_scale_int_update(GtkAdjustment * adjustment, gint * value)
{
    GtkWidget          *entry;
    char                buf[MAXSTRLEN];

    if (*value != adjustment->value) {
	*value = (int) adjustment->value;

	entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
	sprintf(buf, "%i", *value);

	gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
	set_cmap_preview();
	dialog_update_preview();
    }
}				/* dialog_scale_int_update */

/**********************************************************************
 FUNCTION: dialog_entry_update
 *********************************************************************/

void
dialog_entry_update(GtkWidget * widget, gdouble * value)
{
    GtkAdjustment      *adjustment;
    gdouble             new_value;

    new_value = atof(gtk_entry_get_text(GTK_ENTRY(widget)));

    if (*value != new_value) {
	adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

	if ((new_value >= adjustment->lower) &&
	    (new_value <= adjustment->upper)) {
	    *value = new_value;
	    adjustment->value = new_value;

	    gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");

	    dialog_update_preview();
	}
    }
}				/* dialog_entry_update */

/**********************************************************************
 FUNCTION: dialog_entry_int_update
 *********************************************************************/

void
dialog_entry_int_update(GtkWidget * widget, gint * value)
{
    GtkAdjustment      *adjustment;
    gint             new_value;

    new_value = atoi(gtk_entry_get_text(GTK_ENTRY(widget)));

    if (*value != new_value) {
	adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

	if ((1.0*new_value >= adjustment->lower) &&
	    (1.0*new_value <= adjustment->upper)) {
	    *value = new_value;
	    adjustment->value = 1.0*new_value;

	    gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");

	    dialog_update_preview();
	}
    }
}				/* dialog_entry_update */
