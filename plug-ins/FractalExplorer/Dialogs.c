#include "FractalExplorer.h"
#include "Callbacks.h"
#include "Dialogs.h"
#include "Languages.h"
#include "Events.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "logo.h"

/**********************************************************************
 FUNCTION: explorer_dialog
 *********************************************************************/

/* void gtk_entry_adjust_scroll ( GtkEntry* ); */

 
gint
explorer_dialog(void)
{
    GtkWidget          *dialog,
                       *top_table,
                       *text,
                       *top_table2,
                       *frame,
                       *frame2,
                       *frame3,
                       *toggle,
                       *toggle_vbox,
                       *toggle_vbox2,
                       *toggle_vbox3,
		       *notebook,
                       *hbox,
                       *table,
                       *table6,
                       *button;
    gint                argc;
    gchar             **argv;
    guchar             *color_cube;
    GSList             *redmode_group = NULL,
                       *greenmode_group = NULL,
                       *bluemode_group = NULL,
                       *language_group = NULL,
                       *colormode_group = NULL,
                       *type_group = NULL;

    do_redsinus = (wvals.redmode == SINUS);
    do_redcosinus = (wvals.redmode == COSINUS);
    do_rednone = (wvals.redmode == NONE);
    do_greensinus = (wvals.greenmode == SINUS);
    do_greencosinus = (wvals.greenmode == COSINUS);
    do_greennone = (wvals.greenmode == NONE);
    do_bluesinus = (wvals.bluemode == SINUS);
    do_bluecosinus = (wvals.bluemode == COSINUS);
    do_bluenone = (wvals.bluemode == NONE);
    do_redinvert = (wvals.redinvert != FALSE);
    do_greeninvert = (wvals.greeninvert != FALSE);
    do_blueinvert = (wvals.blueinvert != FALSE);
    do_colormode1 = (wvals.colormode == 0);
    do_colormode2 = (wvals.colormode == 1);
    do_type0 = (wvals.fractaltype == 0);
    do_type1 = (wvals.fractaltype == 1);
    do_type2 = (wvals.fractaltype == 2);
    do_type3 = (wvals.fractaltype == 3);
    do_type4 = (wvals.fractaltype == 4);
    do_type5 = (wvals.fractaltype == 5);
    do_type6 = (wvals.fractaltype == 6);
    do_type7 = (wvals.fractaltype == 7);
    do_type8 = (wvals.fractaltype == 8);

    do_english = (wvals.language == 0);
    do_french = (wvals.language == 1);
    do_german = (wvals.language == 2);

    argc = 1;
    argv = g_new(gchar *, 1);
    argv[0] = g_strdup("fractalexplorer");

    gtk_init(&argc, &argv);

    plug_in_parse_fractalexplorer_path();
    
    gtk_preview_set_gamma(gimp_gamma());
    gtk_preview_set_install_cmap(gimp_install_cmap());
    color_cube = gimp_color_cube();
    gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2], color_cube[3]);

    gtk_widget_set_default_visual(gtk_preview_get_visual());
    gtk_widget_set_default_colormap(gtk_preview_get_cmap());

    wint.wimage = g_malloc(preview_width * preview_height * 3 * sizeof(guchar));
    elements = g_malloc(sizeof(DialogElements));

    explorer_load_dialog();
    
    dialog = maindlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Fractal Explorer <Daniel Cotting/cotting@multimania.com>");
    gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 0);
    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		       (GtkSignalFunc) dialog_close_callback,
		       NULL);

    top_table = gtk_table_new(4, 5, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(top_table), 4);
    gtk_table_set_row_spacings(GTK_TABLE(top_table), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), top_table, FALSE, FALSE, 0);
    gtk_widget_show(top_table);

  /* Tool-Tips */

  /* use black as foreground: */
    tips = gtk_tooltips_new();
    tips_fg.red = 0;
    tips_fg.green = 0;
    tips_fg.blue = 0;
  /* postit yellow (khaki) as background: */
    gdk_color_alloc(gtk_widget_get_colormap(top_table), &tips_fg);
    tips_bg.red = 61669;
    tips_bg.green = 59113;
    tips_bg.blue = 35979;
    gdk_color_alloc(gtk_widget_get_colormap(top_table), &tips_bg);
    gtk_tooltips_set_colors(tips, &tips_bg, &tips_fg);

  /* Preview */

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_table_attach(GTK_TABLE(top_table), frame, 0, 1, 0, 1, 0, 0, 0, 0);
    gtk_widget_show(frame);

    wint.preview = gtk_preview_new(GTK_PREVIEW_COLOR);
    gtk_preview_size(GTK_PREVIEW(wint.preview), preview_width, preview_height);
    gtk_container_add(GTK_CONTAINER(frame), wint.preview);
    gtk_signal_connect(GTK_OBJECT(wint.preview), "button_press_event",
		       (GtkSignalFunc) preview_button_press_event, NULL);
    gtk_signal_connect(GTK_OBJECT(wint.preview), "button_release_event",
		       (GtkSignalFunc) preview_button_release_event, NULL);
    gtk_signal_connect(GTK_OBJECT(wint.preview), "motion_notify_event",
		       (GtkSignalFunc) preview_motion_notify_event, NULL);
    gtk_signal_connect(GTK_OBJECT(wint.preview), "leave_notify_event",
		       (GtkSignalFunc) preview_leave_notify_event, NULL);
    gtk_signal_connect(GTK_OBJECT(wint.preview), "enter_notify_event",
		       (GtkSignalFunc) preview_enter_notify_event, NULL);
    gtk_widget_set_events(wint.preview, GDK_BUTTON_PRESS_MASK
			  | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK
			  | GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK);
    gtk_widget_show(wint.preview);

  /* Create notebook */
    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_table_attach(GTK_TABLE(top_table), notebook, 4, 5, 0, 4, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show (notebook);

    
  /* Controls */
    frame2 = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame2), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame2), 10);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame2, 
			    gtk_label_new (msg[lng][MSG_FRACTALOPTIONS]));
    gtk_widget_show (frame2);

    toggle_vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox2), 0);
    gtk_container_add(GTK_CONTAINER(frame2), toggle_vbox2);
    gtk_widget_show(toggle_vbox2);

    top_table2 = gtk_table_new(5, 5, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(top_table2), 10);
    gtk_table_set_row_spacings(GTK_TABLE(top_table2), 0);
    gtk_box_pack_start(GTK_BOX(toggle_vbox2), top_table2, FALSE, FALSE, 0);
    gtk_widget_show(top_table2);

    frame = gtk_frame_new(msg[lng][MSG_PARAMETERS]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
    gtk_table_attach(GTK_TABLE(top_table2), frame, 0, 4, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 0);
    gtk_container_add(GTK_CONTAINER(frame), toggle_vbox);

    table = gtk_table_new(9, 5, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 0);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), table, FALSE, FALSE, 0);
    gtk_widget_show(table);
    dialog_create_value("XMIN", GTK_TABLE(table), 0, &wvals.xmin, -3, 3, msg[lng][MSG_XMIN], &(elements->xmin));
    dialog_create_value("XMAX", GTK_TABLE(table), 1, &wvals.xmax, -3, 3, msg[lng][MSG_XMAX], &(elements->xmax));
    dialog_create_value("YMIN", GTK_TABLE(table), 2, &wvals.ymin, -3, 3, msg[lng][MSG_YMIN], &(elements->ymin));
    dialog_create_value("YMAX", GTK_TABLE(table), 3, &wvals.ymax, -3, 3, msg[lng][MSG_YMAX], &(elements->ymax));
    dialog_create_value("ITER", GTK_TABLE(table), 4, &wvals.iter, 0, 1000, msg[lng][MSG_ITER], &(elements->iter));
    dialog_create_value("CX", GTK_TABLE(table), 5, &wvals.cx, -2.5, 2.5, msg[lng][MSG_CX], &(elements->cx));
    dialog_create_value("CY", GTK_TABLE(table), 6, &wvals.cy, -2.5, 2.5, msg[lng][MSG_CY], &(elements->cy));


    button = gtk_button_new();
    gtk_table_attach(GTK_TABLE(table), button, 1, 2, 7, 8, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    text = gtk_label_new(msg[lng][MSG_RESET]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_widget_show(text);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_reset_callback,
		       dialog);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_RESET_PARAM_COMMENT]);
    
    button = gtk_button_new();
    text = gtk_label_new(msg[lng][MSG_LOAD]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_table_attach(GTK_TABLE(table), button, 0, 1, 7, 8, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show(text);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) load_button_press,
		       dialog);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_LOADCOMMENT]);
    
    button = gtk_button_new();
    gtk_table_attach(GTK_TABLE(table), button, 2, 3, 7, 8, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    text = gtk_label_new(msg[lng][MSG_SAVE]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_widget_show(text);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_save_callback,
		       dialog);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_SAVECOMMENT]);

    
    gtk_widget_show(table);
    gtk_widget_show(toggle_vbox);
    gtk_widget_show(frame);

/*  Fractal type toggle box  */
    frame = gtk_frame_new(msg[lng][MSG_FRACTALTYPE]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(top_table2), frame, 0, 4, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), toggle_vbox, FALSE, FALSE, 0);

    toggle = elements->type_mandelbrot = gtk_radio_button_new_with_label(type_group, "Mandelbrot");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type0);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type0);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Mandelbrot");

    toggle = elements->type_julia = gtk_radio_button_new_with_label(type_group, "Julia");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type1);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type1);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Julia");

    toggle = elements->type_barnsley1 = gtk_radio_button_new_with_label(type_group, "Barnsley 1");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type2);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type2);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Barnsley");

    gtk_widget_show(toggle_vbox);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), toggle_vbox, FALSE, FALSE, 0);

    toggle = elements->type_barnsley2 = gtk_radio_button_new_with_label(type_group, "Barnsley 2");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type3);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type3);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Barnsley 2");
    
    toggle = elements->type_barnsley3 = gtk_radio_button_new_with_label(type_group, "Barnsley 3");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type4);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type4);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Barnsley 3");

    toggle = elements->type_spider = gtk_radio_button_new_with_label(type_group, "Spider");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type5);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type5);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Spider");

    gtk_widget_show(toggle_vbox);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), toggle_vbox, FALSE, FALSE, 0);

    toggle = elements->type_manowar = gtk_radio_button_new_with_label(type_group, "Man'o'war");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type6);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type6);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Man'o'war");
    
    toggle = elements->type_lambda = gtk_radio_button_new_with_label(type_group, "Lambda");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type7);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type7);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Lambda");
    
    toggle = elements->type_sierpinski = gtk_radio_button_new_with_label(type_group, "Sierpinski");
    type_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_type8);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_type8);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Sierpinski");
    
    gtk_widget_show(toggle_vbox);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 5);

    gtk_box_pack_start(GTK_BOX(hbox), toggle_vbox, TRUE, TRUE, 0);


    gtk_widget_show(toggle_vbox);
    gtk_widget_show(hbox);
    gtk_widget_show(frame);

    frame = gtk_frame_new(msg[lng][MSG_PREVIEW]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(top_table), frame, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 5);
    gtk_container_add(GTK_CONTAINER(frame), toggle_vbox);
    
    toggle = gtk_check_button_new_with_label(msg[lng][MSG_REALTIMEPREVIEW]);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &wvals.alwayspreview);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), wvals.alwayspreview);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_REDRAWCOMMENT]);

    button = gtk_button_new();
    text = gtk_label_new(msg[lng][MSG_REDRAW]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_widget_show(text);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_redraw_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_REDRAWPREVIEW]);
    
    gtk_widget_show(toggle_vbox);
    gtk_widget_show(frame);

    frame = gtk_frame_new(msg[lng][MSG_ZOOMOPTS]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(top_table), frame, 0, 1, 3, 4, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 5);
    gtk_container_add(GTK_CONTAINER(frame), toggle_vbox);
    
    button = gtk_button_new();
    text = gtk_label_new(msg[lng][MSG_UNDOZOOM]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_widget_show(text);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_undo_zoom_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    set_tooltip(tips, button,msg[lng][MSG_UNDOCOMMENT]);
    
    button = gtk_button_new();
    text = gtk_label_new(msg[lng][MSG_REDOZOOM]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_widget_show(text);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_redo_zoom_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_REDOCOMMENT]);

    button = gtk_button_new();
    text = gtk_label_new(msg[lng][MSG_STEPIN]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_widget_show(text);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_step_in_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_STEPIN]);

    button = gtk_button_new();
    text = gtk_label_new(msg[lng][MSG_STEPOUT]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_widget_show(text);

    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_step_out_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_STEPOUT]);
    
    gtk_widget_show(toggle_vbox);
    gtk_widget_show(frame);

    frame2 = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame2), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame2), 10);
    gtk_widget_show(frame2);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame2, 
			    gtk_label_new (msg[lng][MSG_COLOROPTS]));

    toggle_vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox2), 0);
    gtk_container_add(GTK_CONTAINER(frame2), toggle_vbox2);
    gtk_widget_show(toggle_vbox2);

    top_table2 = gtk_table_new(5, 5, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(top_table2), 10);
    gtk_table_set_row_spacings(GTK_TABLE(top_table2), 0);
    gtk_box_pack_start(GTK_BOX(toggle_vbox2), top_table2, FALSE, FALSE, 0);
    gtk_widget_show(top_table2);

    frame = gtk_frame_new(msg[lng][MSG_NUMCOLORS]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(top_table2), frame, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show(frame);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 0);
    gtk_container_add(GTK_CONTAINER(frame), toggle_vbox);
    gtk_widget_show(toggle_vbox);

    table6 = gtk_table_new(3, 3, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table6), 0);
    gtk_table_set_row_spacings(GTK_TABLE(table6), 0);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), table6, FALSE, FALSE, 0);
    gtk_widget_show(table6);
    dialog_create_int_value(msg[lng][MSG_NUMCOLORS], GTK_TABLE(table6), 0, &wvals.ncolors, 2, MAXNCOLORS, msg[lng][MSG_CHGNUMCOLORS], &(elements->ncol));

    elements->useloglog = toggle = gtk_check_button_new_with_label(msg[lng][MSG_USELOGLOG]);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &wvals.useloglog);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), wvals.useloglog);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_USELOGLOGCOMMENT]);

    gtk_widget_show(toggle_vbox);
    gtk_widget_show(frame);

    frame = gtk_frame_new(msg[lng][MSG_COLORDENSITY]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(top_table2), frame, 0, 1, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show(frame);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 0);
    gtk_container_add(GTK_CONTAINER(frame), toggle_vbox);
    gtk_widget_show(toggle_vbox);

    table6 = gtk_table_new(3, 3, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table6), 0);
    gtk_table_set_row_spacings(GTK_TABLE(table6), 0);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), table6, FALSE, FALSE, 0);
    gtk_widget_show(table6);
    dialog_create_value(msg[lng][MSG_RED], GTK_TABLE(table6), 0, &wvals.redstretch, 0.0, 1.0, msg[lng][MSG_REDINTENSITY], &(elements->red));
    dialog_create_value(msg[lng][MSG_GREEN], GTK_TABLE(table6), 1, &wvals.greenstretch, 0.0, 1.0, msg[lng][MSG_GREENINTENSITY], &(elements->green));
    dialog_create_value(msg[lng][MSG_BLUE], GTK_TABLE(table6), 2, &wvals.bluestretch, 0.0, 1.0, msg[lng][MSG_BLUEINTENSITY], &(elements->blue));
    gtk_widget_show(toggle_vbox);
    gtk_widget_show(frame);

    frame3 = gtk_frame_new(msg[lng][MSG_COLORFUNCTION]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame3), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(top_table2), frame3, 0, 1, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show(frame3);

    toggle_vbox3 = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox3), 0);
    gtk_container_add(GTK_CONTAINER(frame3), toggle_vbox3);
    gtk_widget_show(toggle_vbox3);

    table6 = gtk_table_new(4, 4, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table6), 0);
    gtk_table_set_row_spacings(GTK_TABLE(table6), 0);
    gtk_box_pack_start(GTK_BOX(toggle_vbox3), table6, FALSE, FALSE, 0);
    gtk_widget_show(table6);

    frame = gtk_frame_new(msg[lng][MSG_RED]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(table6), frame, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show(frame);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 0);
    gtk_container_add(GTK_CONTAINER(frame), toggle_vbox);
    gtk_widget_show(toggle_vbox);

    toggle = elements->redmodesin = gtk_radio_button_new_with_label(redmode_group, msg[lng][MSG_SINE]);
    redmode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_redsinus);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_redsinus);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_SINECOMMENT]);

    toggle = elements->redmodecos = gtk_radio_button_new_with_label(redmode_group, msg[lng][MSG_COSINE]);
    redmode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_redcosinus);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_redcosinus);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_COSINECOMMENT]);

    toggle = elements->redmodenone = gtk_radio_button_new_with_label(redmode_group, msg[lng][MSG_NONE]);
    redmode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_rednone);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_rednone);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_NONECOMMENT]);

    elements->redinvert = toggle = gtk_check_button_new_with_label(msg[lng][MSG_INVERSION]);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &wvals.redinvert);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), wvals.redinvert);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_INVERSIONCOMMENT]);

    
    gtk_widget_show(toggle_vbox);
    gtk_widget_show(frame);

/*  Greenmode toggle box  */
    frame = gtk_frame_new(msg[lng][MSG_GREEN]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(table6), frame, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show(frame);
    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 0);
    gtk_container_add(GTK_CONTAINER(frame), toggle_vbox);
    gtk_widget_show(toggle_vbox);

    toggle = elements->greenmodesin = gtk_radio_button_new_with_label(greenmode_group, msg[lng][MSG_SINE]);
    greenmode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_greensinus);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_greensinus);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_SINECOMMENT]);

    toggle = elements->greenmodecos = gtk_radio_button_new_with_label(greenmode_group, msg[lng][MSG_COSINE]);
    greenmode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_greencosinus);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_greencosinus);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_COSINECOMMENT]);

    toggle = elements->greenmodenone = gtk_radio_button_new_with_label(greenmode_group, msg[lng][MSG_NONE]);
    greenmode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_greennone);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_greennone);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_NONECOMMENT]);

    elements->greeninvert =     toggle = gtk_check_button_new_with_label(msg[lng][MSG_INVERSION]);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &wvals.greeninvert);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), wvals.greeninvert);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_INVERSIONCOMMENT]);

    gtk_widget_show(toggle_vbox);
    gtk_widget_show(frame);

/*  Bluemode toggle box  */
    frame = gtk_frame_new(msg[lng][MSG_BLUE]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(table6), frame, 2, 3, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    gtk_widget_show(frame);
    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 0);
    gtk_container_add(GTK_CONTAINER(frame), toggle_vbox);
    gtk_widget_show(toggle_vbox);

    toggle = elements->bluemodesin = gtk_radio_button_new_with_label(bluemode_group, msg[lng][MSG_SINE]);
    bluemode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_bluesinus);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_bluesinus);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_SINECOMMENT]);

    toggle = elements->bluemodecos = gtk_radio_button_new_with_label(bluemode_group, msg[lng][MSG_COSINE]);
    bluemode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_bluecosinus);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_bluecosinus);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_COSINECOMMENT]);

    toggle = elements->bluemodenone = gtk_radio_button_new_with_label(bluemode_group, msg[lng][MSG_NONE]);
    bluemode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_bluenone);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_bluenone);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_NONECOMMENT]);

    
    elements->blueinvert = toggle = gtk_check_button_new_with_label(msg[lng][MSG_INVERSION]);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &wvals.blueinvert);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), wvals.blueinvert);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_INVERSIONCOMMENT]);

    gtk_widget_show(toggle_vbox);
    gtk_widget_show(frame);
    gtk_widget_show(toggle_vbox3);
    gtk_widget_show(frame3);

/*  Colormode toggle box  */

    frame = gtk_frame_new(msg[lng][MSG_COLORMODE]);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_table_attach(GTK_TABLE(top_table2), frame, 0, 1, 3, 4, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 3, 3);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    toggle_vbox = gtk_vbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), toggle_vbox, FALSE, FALSE, 10);
    toggle = elements->colormode0 = gtk_radio_button_new_with_label(colormode_group, msg[lng][MSG_ASSPECIFIED]);
    colormode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_colormode1);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_colormode1);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_ASSPECIFIEDCOMMENT]);
    
    
    toggle = elements->colormode1 = gtk_radio_button_new_with_label(colormode_group, msg[lng][MSG_APPLYGRADIENT]);
    colormode_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_colormode2);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_colormode2);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, msg[lng][MSG_APPLYGRADIENTCOMMENT]);
    gtk_widget_show(toggle_vbox);
    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), toggle_vbox, TRUE, TRUE, 10);
    cmap_preview = gtk_preview_new(GTK_PREVIEW_COLOR);
    gtk_preview_size(GTK_PREVIEW(cmap_preview), 32, 32);
    gtk_box_pack_start(GTK_BOX(toggle_vbox), cmap_preview, TRUE, TRUE, 10);
    gtk_widget_show(cmap_preview);
    gtk_widget_show(toggle_vbox);
    gtk_widget_show(hbox);
    gtk_widget_show(frame);
    
    frame= add_objects_list ();
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, 
 			    gtk_label_new (msg[lng][MSG_FRACTALPRESETS]));
    gtk_widget_show (frame);
    
    frame= add_gradients_list ();
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, 
 			    gtk_label_new (msg[lng][MSG_GRADIENTPRESETS]));
    gtk_widget_show (frame);

    frame2 = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame2), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame2), 10);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame2, 
			    gtk_label_new (msg[lng][MSG_GENERALOPTIONS]));
			    
    toggle_vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox2), 10);
    gtk_container_add(GTK_CONTAINER(frame2), toggle_vbox2);
    gtk_widget_show(toggle_vbox2);

    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 10);
    gtk_box_pack_start(GTK_BOX(toggle_vbox2), toggle_vbox, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0);
    gtk_widget_show(toggle_vbox);

    toggle = gtk_radio_button_new_with_label(language_group, "English");
    language_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_english);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_english);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "This sets the default language to English. Note that you will have to restart the plug-in!");

    toggle = gtk_radio_button_new_with_label(language_group, "Français");
    language_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_french);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_french);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Cette option active les messages en français. Il vous faudra redémarrer le programme pour que les changements prennent effet.");

    toggle = gtk_radio_button_new_with_label(language_group, "Deutsch");
    language_group = gtk_radio_button_group(GTK_RADIO_BUTTON(toggle));
    gtk_box_pack_start(GTK_BOX(toggle_vbox), toggle, FALSE, FALSE, 0);
    gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
		       (GtkSignalFunc) explorer_toggle_update,
		       &do_german);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), do_german);
    gtk_widget_show(toggle);
    set_tooltip(tips, toggle, "Diese Option stellt die deutschen Texte ein. Damit diese jedoch angezeigt werden, ist ein Neustart des Programms noetig.");
    gtk_widget_show(toggle_vbox);
    
    toggle_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(toggle_vbox), 10);
    gtk_box_pack_start(GTK_BOX(toggle_vbox2), toggle_vbox, FALSE, FALSE, 0);
    gtk_widget_show(toggle_vbox);
    
    button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(toggle_vbox), button, FALSE, FALSE, 0);
    text = gtk_label_new(msg[lng][MSG_SAVELANGUAGE]);
    gtk_misc_set_alignment(GTK_MISC(text), 0.5, 0.5);
    gtk_container_add(GTK_CONTAINER(button), text);
    gtk_widget_show(text);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_savelanguage_callback,
		       dialog);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_SAVELANGUAGE_COMMENT]);
    
    gtk_widget_show(toggle_vbox);
			    
			    
    gtk_widget_show (frame2);

  /* Buttons */

    gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area), 5);

    button = gtk_button_new_with_label(msg[lng][MSG_OK]);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_ok_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_STARTCALC]);

    button = gtk_button_new_with_label(msg[lng][MSG_CANCEL]);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) dialog_cancel_callback,
		       dialog);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_MAINDLGCANCEL]);

    button = gtk_button_new_with_label(msg[lng][MSG_ABOUT]);
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) explorer_about_callback, button);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		       button, TRUE, TRUE, 0);
    gtk_widget_show(button);
    set_tooltip(tips, button, msg[lng][MSG_ABOUTCOMMENT]);

    /* Done */

    /* Popup for list area: Not yet fully implemented
    
       fractalexplorer_op_menu_create(maindlg);
       
    */
    gtk_signal_disconnect_by_data (GTK_OBJECT (loaddlg), loaddlg);
    gtk_widget_destroy (loaddlg);
  
    gtk_widget_show(dialog);
    ready_now = TRUE;

    set_cmap_preview();

    dialog_update_preview();
    gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_NONE);
    gtk_main();
    gdk_flush();
    if (the_tile != NULL) {
	gimp_tile_unref(the_tile, FALSE);
	the_tile = NULL;
    }
    g_free(wint.wimage);

    return wint.run;
}				/* explorer_dialog */

/**********************************************************************
 FUNCTION: dialog_update_preview
 *********************************************************************/

void
dialog_update_preview()
{
    double              left,
                        right,
                        bottom,
                        top;
    double              dx,
                        dy,
                        cx,
                        cy;
    int                 px,
                        py;
    int                 xcoord,
                        ycoord;
    int                 iteration;
    guchar             *p_ul,
                       *p;
    double              a,
                        b,
                        x,
                        y,
			oldx,
			oldy,
			foldxinitx,
			foldxinity,
			tempsqrx,
			tempsqry,
			tmpx=0,
			tmpy=0,
			foldyinitx,
			foldyinity,
                        adjust,
                        xx=0;
    int                 zaehler,
                        color;
    int                 useloglog;
			
    if (NULL == wint.preview)
	return;

    if ((ready_now) && (wvals.alwayspreview)) {
/*        gtk_widget_set_sensitive(maindlg, FALSE); */
	left = sel_x1;
	right = sel_x2 - 1;
	bottom = sel_y2 - 1;
	top = sel_y1;
	dx = (right - left) / (preview_width - 1);
	dy = (bottom - top) / (preview_height - 1);

	xmin = wvals.xmin;
	xmax = wvals.xmax;
	ymin = wvals.ymin;
	ymax = wvals.ymax;
	cx = wvals.cx;
	cy = wvals.cy;
	xbild = preview_width;
	ybild = preview_height;
	xdiff = (xmax - xmin) / xbild;
	ydiff = (ymax - ymin) / ybild;

	py = 0;

	p_ul = wint.wimage;
	iteration = (int) wvals.iter;
	useloglog = wvals.useloglog;
	for (ycoord = 0; ycoord < preview_height; ycoord++) {
	    px = 0;

	    for (xcoord = 0; xcoord < preview_width; xcoord++) {
		a = (double) xmin + (double) xcoord *xdiff;
		b = (double) ymin + (double) ycoord *ydiff;

		if (wvals.fractaltype!=0) {
		    tmpx=x = a;
		    tmpy=y = b;
		} else {
		    x = 0;
		    y = 0;
		}
		for (zaehler = 0; (zaehler < iteration) && ((x * x + y * y) < 4); zaehler++) {
		    oldx=x; oldy=y;
		    if (wvals.fractaltype==1) {
		        /* Julia */
			xx = x * x - y * y + cx;
			y = 2.0 * x * y + cy; 
		    } else if (wvals.fractaltype==0) {
		        /*Mandelbrot*/
		        xx = x * x - y * y + a;
			y = 2.0 * x * y + b;
		    } else if (wvals.fractaltype==2) {
/* Barnsley M1 */
		    	foldxinitx = oldx * cx;
                        foldyinity = oldy * cy;
                        foldxinity = oldx * cy;
                        foldyinitx = oldy * cx;
                        /* orbit calculation */
                       if(oldx >= 0)
		       {
		          xx = (foldxinitx - cx - foldyinity);
                          y = (foldyinitx - cy + foldxinity);
		       }
		       else
		       {
		          xx = (foldxinitx + cx - foldyinity);
		          y = (foldyinitx + cy + foldxinity);
		       }
		    } else if (wvals.fractaltype==3) {
/* Barnsley Unnamed */
		       
			foldxinitx = oldx * cx;
                        foldyinity = oldy * cy;
                        foldxinity = oldx * cy;
                        foldyinitx = oldy * cx;
                        /* orbit calculation */
                        if(foldxinity + foldyinitx >= 0)
                        {
                           xx = foldxinitx - cx - foldyinity;
                           y = foldyinitx - cy + foldxinity;
                        }
		        else
		        {
		           xx = foldxinitx + cx - foldyinity;
                           y = foldyinitx + cy + foldxinity;
                        }
		    } else if (wvals.fractaltype==4) {
		        /*Barnsley 1*/
                        foldxinitx  = oldx * oldx;
                        foldyinity  = oldy * oldy;
                        foldxinity  = oldx * oldy;
                        /* orbit calculation */
                        if(oldx > 0)
                         {
                           xx = foldxinitx - foldyinity - 1.0;
                           y = foldxinity * 2;
                         }
                        else
                         {
                           xx = foldxinitx - foldyinity -1.0 + cx * oldx;
                           y = foldxinity * 2;
                           y += cy * oldx;
                         }
		    } else if (wvals.fractaltype==5) {
			 /* Spider(XAXIS) { c=z=pixel: z=z*z+c; c=c/2+z, |z|<=4 } */
                        xx = x*x - y*y + tmpx + cx;
                        y = 2 * oldx * oldy + tmpy +cy;
                        tmpx = tmpx/2 + xx;
                        tmpy = tmpy/2 + y;
		    } else if (wvals.fractaltype==6) {
/*			ManOWarfpFractal() */
                        xx = x*x - y*y + tmpx + cx;
                        y = 2.0 * x * y + tmpy + cy;
                        tmpx = oldx;
			tmpy = oldy;
		    } else if (wvals.fractaltype==7) {
/* Lambda */
            		tempsqrx=x*x;
	           	tempsqry=y*y;
			tempsqrx = oldx - tempsqrx + tempsqry;
                        tempsqry = -(oldy * oldx);
                        tempsqry += tempsqry + oldy;
                        xx = cx * tempsqrx - cy * tempsqry;
                        y = cx * tempsqry + cy * tempsqrx;
		    } else if (wvals.fractaltype==8) {
/* Sierpinski */
			xx = oldx + oldx;
                        y = oldy + oldy;
                        if(oldy > .5)
                           y = y - 1;
                        else if (oldx > .5)
                           xx = xx - 1;
		    }
		    x = xx;
		}
		if (useloglog) {
		  adjust = log(log(x*x+y*y)/2)/log(2);
		} else {
		  adjust = 0.0;
		}
		color = (int) (((zaehler - adjust) * (wvals.ncolors -1))/ iteration);
		p_ul[0] = colormap[color][0];
		p_ul[1] = colormap[color][1];
		p_ul[2] = colormap[color][2];
		p_ul += 3;
		px += 1;
	    }			/* for */
	    py += 1;
	}			/* for */
	p = wint.wimage;

	for (y = 0; y < preview_height; y++) {
	    gtk_preview_draw_row(GTK_PREVIEW(wint.preview), p, 0, y, preview_width);
	    p += preview_width * 3;
	}			/* for */
	gtk_widget_draw(wint.preview, NULL);
	gdk_flush();
/*	gtk_widget_set_sensitive(maindlg, TRUE); */
    }
}				/* dialog_update_preview */

/**********************************************************************
 FUNCTION: dialog_create_value
 *********************************************************************/

void
dialog_create_value(char *title, GtkTable * table, int row, gdouble * value,
	       int left, int right, const char *desc, scaledata * scalevalues)
{
    GtkWidget          *label;
    GtkWidget          *scale;
    GtkWidget          *entry;
    GtkObject          *scale_data;
    char                buf[MAXSTRLEN];
    scaledata          *pppp;

    pppp = scalevalues;

    label = gtk_label_new(title);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
    gtk_widget_show(label);

    scale_data = gtk_adjustment_new(*value, left, right,
				    (right - left) / 1000,
				    (right - left) / 1000,
				    0);
    pppp->data = GTK_ADJUSTMENT(scale_data);
    gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		       (GtkSignalFunc) dialog_scale_update,
		       value);

    scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
    gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
    gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_scale_set_digits(GTK_SCALE(scale), 3);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
    gtk_widget_show(scale);
    set_tooltip(tips, scale, desc);

    entry = gtk_entry_new();
    pppp->text = entry;
    gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
    gtk_object_set_user_data(scale_data, entry);
     gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
     sprintf(buf, "%0.15f", *value);
     gtk_entry_set_text(GTK_ENTRY(entry), buf);
     gtk_entry_set_position (GTK_ENTRY (entry), 0);
     /* gtk_entry_adjust_scroll (GTK_ENTRY (entry)); */
     gtk_signal_connect(GTK_OBJECT(entry), "changed",
                       (GtkSignalFunc) dialog_entry_update,
                       value);
    gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, 0, 0, 4, 0);
    gtk_widget_show(entry);
    set_tooltip(tips, entry, desc);

}				/* dialog_create_value */

/**********************************************************************
 FUNCTION: dialog_create_int_value
 *********************************************************************/

void
dialog_create_int_value(char *title, GtkTable * table, int row, gint * value,
	       int left, int right, const char *desc, scaledata * scalevalues)
{
    GtkWidget          *label;
    GtkWidget          *scale;
    GtkWidget          *entry;
    GtkObject          *scale_data;
    char                buf[MAXSTRLEN];
    scaledata          *pppp;

    pppp = scalevalues;

    label = gtk_label_new(title);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
    gtk_widget_show(label);

    scale_data = gtk_adjustment_new(1.0 *(*value), left, right,
				    (right - left) / 200,
				    (right - left) / 200,
				    0);
    pppp->data = GTK_ADJUSTMENT(scale_data);
    gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		       (GtkSignalFunc) dialog_scale_int_update,
		       value);

    scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
    gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
    gtk_table_attach(table, scale, 1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_scale_set_digits(GTK_SCALE(scale), 3);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
    gtk_widget_show(scale);
    set_tooltip(tips, scale, desc);

    entry = gtk_entry_new();
    pppp->text = entry;
    gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
    gtk_object_set_user_data(scale_data, entry);
    gtk_widget_set_usize(entry, ENTRY_WIDTH - 20, 0);
    sprintf(buf, "%i", *value);
    gtk_entry_set_text(GTK_ENTRY(entry), buf);
    gtk_signal_connect(GTK_OBJECT(entry), "changed",
		       (GtkSignalFunc) dialog_entry_int_update,
		       value);
    gtk_table_attach(GTK_TABLE(table), entry, 2, 3, row, row + 1, 0, 0, 4, 0);
    gtk_widget_show(entry);
    set_tooltip(tips, entry, desc);

}				/* dialog_create_int_value */

/**********************************************************************
 FUNCTION: set_cmap_preview()
 *********************************************************************/

void
set_cmap_preview()
{
    int                 i,
                        x,
                        y,
                        j;
    guchar             *b;
    guchar              c[GR_WIDTH*3];
    int xsize, ysize;

    if (NULL == cmap_preview)
	return;

    if (NULL == cmap_preview_long)
	return;

    if (NULL == cmap_preview_long2)
	return;
		
    make_color_map();

    for (ysize = 1; ysize * ysize * ysize < wvals.ncolors; ysize++) /**/;
    xsize = wvals.ncolors / ysize;
    while (xsize * ysize < wvals.ncolors) xsize++;
    b = g_new(guchar, xsize * 3);

    gtk_preview_size     (GTK_PREVIEW(cmap_preview), xsize, ysize*4);
    gtk_widget_set_usize (GTK_WIDGET(cmap_preview),  xsize, ysize*4);
    
    for (y = 0; y < ysize*4; y += 4) {
	for (x = 0; x < xsize; x++) {
	    i = x + (y / 4) * xsize;
	    if (i > wvals.ncolors) {
	      for (j = 0; j < 3; j++) 
		b[x * 3 + j] = 0;
	    } else {
	      for (j = 0; j < 3; j++)
		b[x * 3 + j] = colormap[i][j];
	    }
	}
	gtk_preview_draw_row(GTK_PREVIEW(cmap_preview), b, 0, y, xsize);
	gtk_preview_draw_row(GTK_PREVIEW(cmap_preview), b, 0, y + 1, xsize);
	gtk_preview_draw_row(GTK_PREVIEW(cmap_preview), b, 0, y + 2, xsize);
	gtk_preview_draw_row(GTK_PREVIEW(cmap_preview), b, 0, y + 3, xsize);
    }

	for (x = 0; x < GR_WIDTH; x++) {
	    for (j = 0; j < 3; j++)
		c[x * 3 + j] = colormap[(int)((float)x/(float)GR_WIDTH*256.0)][j];
	}
	for (i=0; i<32; i++)
	{
	   gtk_preview_draw_row(GTK_PREVIEW(cmap_preview_long), c, 0, i, GR_WIDTH);
	}     
	for (i=0; i<32; i++)
	{
	   gtk_preview_draw_row(GTK_PREVIEW(cmap_preview_long2), c, 0, i, GR_WIDTH);
	}     
    gtk_widget_draw(cmap_preview, NULL);
    gtk_widget_draw(cmap_preview_long, NULL);
    gtk_widget_draw(cmap_preview_long2, NULL);
    g_free(b);
}

/**********************************************************************
 FUNCTION: make_color_map()
 *********************************************************************/

void
make_color_map()
{
    int                 i,
                        j;
    int                 r,
                        gr,
                        bl;
    double             *g = NULL;
    double              redstretch,
                        greenstretch,
                        bluestretch,
                        pi = atan(1) * 4;

    if (wvals.colormode) {
	g = gimp_gradients_sample_uniform(wvals.ncolors);
    }
    redstretch = wvals.redstretch * 127.5;
    greenstretch = wvals.greenstretch * 127.5;
    bluestretch = wvals.bluestretch * 127.5;
    for (i = 0; i < wvals.ncolors; i++)
	if (wvals.colormode) {
	    for (j = 0; j < 3; j++)
		colormap[i][j] = (int) (g[i * 4 + j] * 255.0);
	} else {
	  double x = (i*2.0) / wvals.ncolors;
	    r = gr = bl = 0;

	    switch (wvals.redmode) {
	    case SINUS:
		r = (int) redstretch *(1.0 + sin((x - 1) * pi));
		break;
	    case COSINUS:
	        r = (int) redstretch *(1.0 + cos((x - 1) * pi));
		break;
	    case NONE:
	        r = (int)(redstretch *(x));
		break;
	    default:
		break;
	    }

	    switch (wvals.greenmode) {
	    case SINUS:
		gr = (int) greenstretch *(1.0 + sin((x - 1) * pi));
		break;
	    case COSINUS:
		gr = (int) greenstretch *(1.0 + cos((x - 1) * pi));
		break;
	    case NONE:
	        gr = (int)(greenstretch *(x));
		break;
	    default:
		break;
	    }
	    switch (wvals.bluemode) {
	    case SINUS:
		bl = (int) bluestretch *(1.0 + sin((x - 1) * pi));
		break;
	    case COSINUS:
		bl = (int) bluestretch *(1.0 + cos((x - 1) * pi));
		break;
	    case NONE:
	        bl = (int)(bluestretch *(x));
		break;
	    default:
		break;
	    }
	    if (r > 255) {
		r = 255;
	    }
	    if (gr > 255) {
		gr = 255;
	    }
	    if (bl > 255) {
		bl = 255;
	    }
	    if (wvals.redinvert) {
		r = 255-r;
	    }
	    if (wvals.greeninvert) {
		gr = 255-gr;
	    }
	    if (wvals.blueinvert) {
		bl = 255-bl;
	    }
	    colormap[i][0] = r;
	    colormap[i][1] = gr;
	    colormap[i][2] = bl;
	}
    g_free(g);
}

/**********************************************************************
 FUNCTION: explorer_logo_dialog
 *********************************************************************/

GtkWidget          *
explorer_logo_dialog()
{
    GtkWidget          *xdlg;
    GtkWidget          *xbutton;
    GtkWidget 	       *xlabel=NULL;
    GtkWidget          *xlogo_box;
    GtkWidget          *xpreview;
    GtkWidget          *xframe,
                       *xframe2,
		       *xframe3;
    GtkWidget          *xvbox;
    GtkWidget          *xhbox;
    GtkWidget	       *vpaned;
    #if 0
    GtkWidget 	       *table;
    GtkWidget 	       *text;
    GtkWidget 	       *hscrollbar;
    GtkWidget 	       *vscrollbar;
    #endif
    guchar             *temp,
                       *temp2;
    unsigned char      *datapointer;
    gint                y,
                        x;

    xdlg = logodlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(xdlg), msg[lng][MSG_ABOUT]);
    gtk_window_position(GTK_WINDOW(xdlg), GTK_WIN_POS_NONE);
    gtk_signal_connect(GTK_OBJECT(xdlg), "destroy",
		       (GtkSignalFunc) dialog_close_callback,
		       NULL);

    xbutton = gtk_button_new_with_label(msg[lng][MSG_OK]);
    GTK_WIDGET_SET_FLAGS(xbutton, GTK_CAN_DEFAULT);
    gtk_signal_connect(GTK_OBJECT(xbutton), "clicked",
		       (GtkSignalFunc) explorer_logo_ok_callback,
		       xdlg);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(xdlg)->action_area),
		       xbutton, TRUE, TRUE, 0);
    gtk_widget_grab_default(xbutton);
    gtk_widget_show(xbutton);
    set_tooltip(tips, xbutton, msg[lng][MSG_ABOUTBOXOKCOMMENT]);

    xframe = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(xframe), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width(GTK_CONTAINER(xframe), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(xdlg)->vbox), xframe, TRUE, TRUE, 0);
    xvbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(xvbox), 10);
    gtk_container_add(GTK_CONTAINER(xframe), xvbox);

  /*  The logo frame & drawing area  */
    xhbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(xvbox), xhbox, FALSE, TRUE, 0);

    xlogo_box = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(xhbox), xlogo_box, FALSE, FALSE, 0);

    xframe2 = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(xframe2), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(xlogo_box), xframe2, FALSE, FALSE, 0);

    xpreview = gtk_preview_new(GTK_PREVIEW_COLOR);
    gtk_preview_size(GTK_PREVIEW(xpreview), logo_width, logo_height);
    temp = g_malloc((logo_width + 10) * 3);
    datapointer = header_data+logo_width*logo_height-1;
    for (y = 0; y < logo_height; y++) {
	temp2 = temp;
	for (x = 0; x < logo_width; x++) {
	    HEADER_PIXEL(datapointer, temp2);
	    temp2 += 3;
	}
	gtk_preview_draw_row(GTK_PREVIEW(xpreview),
			     temp,
			     0, y, logo_width);
    }
    g_free(temp);
    gtk_container_add(GTK_CONTAINER(xframe2), xpreview);
    gtk_widget_show(xpreview);
    gtk_widget_show(xframe2);
    gtk_widget_show(xlogo_box);
    gtk_widget_show(xhbox);

    xhbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(xvbox), xhbox, TRUE, TRUE, 0);

    vpaned = gtk_vpaned_new ();
    gtk_box_pack_start(GTK_BOX(xhbox), vpaned, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER(vpaned), 0);
    gtk_widget_show (vpaned);

    xframe3 = gtk_frame_new (NULL);
/*    gtk_frame_set_shadow_type (GTK_FRAME(xframe3), GTK_SHADOW_IN); */
/*    gtk_widget_set_usize (xframe3, 20, 20); */
    gtk_paned_add1 (GTK_PANED (vpaned), xframe3);
    gtk_widget_show (xframe3);
#if 0    
    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacing (GTK_TABLE (table), 0, 0);
    gtk_table_set_col_spacing (GTK_TABLE (table), 0, 0);
    gtk_container_add(GTK_CONTAINER(xframe3), table);
    gtk_widget_show (table);

    text = gtk_text_new (NULL, NULL);
    gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
    			GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
    gtk_widget_show (text);

    hscrollbar = gtk_hscrollbar_new (GTK_TEXT (text)->hadj);
    gtk_table_attach (GTK_TABLE (table), hscrollbar, 0, 1, 1, 2,
			GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_EXPAND| GTK_FILL | GTK_SHRINK, 0, 0);
    gtk_widget_show (hscrollbar);

    vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
    gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
			GTK_EXPAND| GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
    gtk_widget_show (vscrollbar);

    gtk_text_freeze (GTK_TEXT (text));

    gtk_widget_realize (text);

    gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL, 
                         "\nCotting Software Productions\n"
			 "Quellenstrasse 10\n"
			 "CH-8005 Zuerich (Switzerland)\n\n"
			 "cotting@multimania.com\n"
			 "http://www.multimania.com/cotting\n\n"
			 "Fractal Chaos Explorer\nPlug-In for the GIMP\n"
			 "Version 2.00 Beta 2 (Multilingual)\n"
	  , -1);
 
    gtk_text_thaw (GTK_TEXT (text));
#endif

    xlabel = gtk_label_new("\nCotting Software Productions\n"
			 "Quellenstrasse 10\n"
			 "CH-8005 Zuerich (Switzerland)\n\n"
			 "cotting@multimania.com\n"
			 "http://www.multimania.com/cotting\n\n"
			 "Fractal Chaos Explorer\nPlug-In for the GIMP\n"
			 "Version 2.00 Beta 2 (Multilingual)\n");
    gtk_container_add(GTK_CONTAINER(xframe3),  xlabel);
    gtk_widget_show(xlabel);

    xframe3 = gtk_frame_new (NULL);
/*    gtk_frame_set_shadow_type (GTK_FRAME(xframe3), GTK_SHADOW_IN); */
/*    gtk_widget_set_usize (xframe3, 20, 20); */
    gtk_paned_add2 (GTK_PANED (vpaned), xframe3);
    gtk_widget_show (xframe3);

#if 0    
    table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_row_spacing (GTK_TABLE (table), 0, 0);
    gtk_table_set_col_spacing (GTK_TABLE (table), 0, 0);
    gtk_container_add(GTK_CONTAINER(xframe3), table);
    gtk_widget_show (table);

    text = gtk_text_new (NULL, NULL);
    gtk_table_attach (GTK_TABLE (table), text, 0, 1, 0, 1,
    			GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
    gtk_widget_show (text);

    hscrollbar = gtk_hscrollbar_new (GTK_TEXT (text)->hadj);
    gtk_table_attach (GTK_TABLE (table), hscrollbar, 0, 1, 1, 2,
			GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
    gtk_widget_show (hscrollbar);

    vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
    gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
			GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
    gtk_widget_show (vscrollbar);

    gtk_text_freeze (GTK_TEXT (text));

    gtk_widget_realize (text);

    gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL, 
                         "\nContains code from:\n\n"
			 "Daniel Cotting\n<cotting@mygale.org>\n"
			 "Peter Kirchgessner\n<Pkirchg@aol.com>\n"
			 "Scott Draves\n<spot@cs.cmu.edu>\n"
			 "Andy Thomas\n<alt@picnic.demon.co.uk>\n"
			 "and the GIMP distribution.\n"
	  , -1);
 
    gtk_text_thaw (GTK_TEXT (text));
#endif

    xlabel = gtk_label_new("\nContains code from:\n\n"
			 "Daniel Cotting\n<cotting@mygale.org>\n"
			 "Peter Kirchgessner\n<Pkirchg@aol.com>\n"
			 "Scott Draves\n<spot@cs.cmu.edu>\n"
			 "Andy Thomas\n<alt@picnic.demon.co.uk>\n"
			 "and the GIMP distribution.\n");
    gtk_container_add(GTK_CONTAINER(xframe3),  xlabel);
    gtk_widget_show(xlabel);

    gtk_widget_show(xhbox);

    gtk_widget_show(xvbox);
    gtk_widget_show(xframe);
    gtk_widget_show(xdlg);

    gtk_main();
    gdk_flush();
    return xdlg;
}

/**********************************************************************
 FUNCTION: explorer_load_dialog
 *********************************************************************/

GtkWidget          *
explorer_load_dialog()
{
    GtkWidget          *xdlg;
    
    GtkWidget          *xlabel;
    GtkWidget          *xlogo_box;
    GtkWidget          *xpreview;
    GtkWidget          *xframe,
                       *xframe2;
    GtkWidget          *xvbox;
    GtkWidget          *xhbox;
    guchar             *temp,
                       *temp2;
    unsigned char      *datapointer;
    gint                y,
                        x;

    xdlg = loaddlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(xdlg), "Loading...");
    gtk_window_position(GTK_WINDOW(xdlg), GTK_WIN_POS_NONE);
    gtk_signal_connect(GTK_OBJECT(xdlg), "destroy",
		       (GtkSignalFunc) dialog_close_callback,
		       loaddlg);

    xframe = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(xframe), GTK_SHADOW_ETCHED_IN);
    gtk_container_set_border_width(GTK_CONTAINER(xframe), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(xdlg)->vbox), xframe, TRUE, TRUE, 0);
    xvbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(xvbox), 10);
    gtk_container_add(GTK_CONTAINER(xframe), xvbox);

  /*  The logo frame & drawing area  */
    xhbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(xvbox), xhbox, FALSE, TRUE, 0);

    xlogo_box = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(xhbox), xlogo_box, FALSE, FALSE, 0);

    xframe2 = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(xframe2), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(xlogo_box), xframe2, FALSE, FALSE, 0);

    xpreview = gtk_preview_new(GTK_PREVIEW_COLOR);
    gtk_preview_size(GTK_PREVIEW(xpreview), logo_width, logo_height);
    temp = g_malloc((logo_width + 10) * 3);
    datapointer = header_data+logo_width*logo_height-1;
    for (y = 0; y < logo_height; y++) {
	temp2 = temp;
	for (x = 0; x < logo_width; x++) {
	    HEADER_PIXEL(datapointer, temp2);
	    temp2 += 3;
	}
	gtk_preview_draw_row(GTK_PREVIEW(xpreview),
			     temp,
			     0, y, logo_width);
    }
    g_free(temp);
    gtk_container_add(GTK_CONTAINER(xframe2), xpreview);
    gtk_widget_show(xpreview);
    gtk_widget_show(xframe2);
    gtk_widget_show(xlogo_box);
    gtk_widget_show(xhbox);

    xhbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(xvbox), xhbox, TRUE, TRUE, 0);
    xlabel = gtk_label_new("... Loading FractalExplorer ...\n"
                           "... Lade FractalExplorer ...\n"
			   "... Chargement de FractalExplorer ...");
    gtk_box_pack_start(GTK_BOX(xhbox), xlabel, TRUE, FALSE, 0);
    gtk_widget_show(xlabel);

    gtk_widget_show(xhbox);

    gtk_widget_show(xvbox);
    gtk_widget_show(xframe);
    gtk_widget_show(xdlg);
    gtk_widget_realize(xdlg);
    gdk_flush();
    return xdlg;
}

/**********************************************************************
 FUNCTION: set_tooltip
 *********************************************************************/

void
set_tooltip(GtkTooltips * tooltips, GtkWidget * widget, const char *desc)
{
    if (desc && desc[0])
	gtk_tooltips_set_tip(tooltips, widget, (char *) desc, NULL);
}

/**********************************************************************
 FUNCTION: dialog_change_scale
 *********************************************************************/

void
dialog_change_scale()
{
    char                text[MAXSTRLEN];
    ready_now = FALSE;
    
    do_redsinus = (wvals.redmode == SINUS);
    do_redcosinus = (wvals.redmode == COSINUS);
    do_rednone = (wvals.redmode == NONE);
    do_greensinus = (wvals.greenmode == SINUS);
    do_greencosinus = (wvals.greenmode == COSINUS);
    do_greennone = (wvals.greenmode == NONE);
    do_bluesinus = (wvals.bluemode == SINUS);
    do_bluecosinus = (wvals.bluemode == COSINUS);
    do_bluenone = (wvals.bluemode == NONE);
    do_redinvert = (wvals.redinvert != FALSE);
    do_greeninvert = (wvals.greeninvert != FALSE);
    do_blueinvert = (wvals.blueinvert != FALSE);
    do_colormode1 = (wvals.colormode == 0);
    do_colormode2 = (wvals.colormode == 1);
    do_type0 = (wvals.fractaltype == 0);
    do_type1 = (wvals.fractaltype == 1);
    do_type2 = (wvals.fractaltype == 2);
    do_type3 = (wvals.fractaltype == 3);
    do_type4 = (wvals.fractaltype == 4);
    do_type5 = (wvals.fractaltype == 5);
    do_type6 = (wvals.fractaltype == 6);
    do_type7 = (wvals.fractaltype == 7);
    do_type8 = (wvals.fractaltype == 8);

    do_english = (wvals.language == 0);
    do_german = (wvals.language == 2);
    do_french = (wvals.language == 1);
    
    elements->xmin.data->value = wvals.xmin;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->xmin.data), "value_changed");
    elements->xmax.data->value = wvals.xmax;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->xmax.data), "value_changed");
    elements->ymin.data->value = wvals.ymin;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->ymin.data), "value_changed");
    elements->ymax.data->value = wvals.ymax;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->ymax.data), "value_changed");
    elements->iter.data->value = wvals.iter;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->iter.data), "value_changed");
    elements->cx.data->value = wvals.cx;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->cx.data), "value_changed");
    elements->cy.data->value = wvals.cy;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->cy.data), "value_changed");
    
     sprintf(text, "%0.15f", wvals.xmin);
     gtk_entry_set_text(GTK_ENTRY(elements->xmin.text), text);
     gtk_entry_set_position (GTK_ENTRY (elements->xmin.text), 0);
     /* gtk_entry_adjust_scroll (GTK_ENTRY (elements->xmin.text)); */
     sprintf(text, "%0.15f", wvals.xmax);
     gtk_entry_set_text(GTK_ENTRY(elements->xmax.text), text);
     gtk_entry_set_position (GTK_ENTRY (elements->xmax.text), 0);
     /* gtk_entry_adjust_scroll (GTK_ENTRY (elements->xmax.text)); */
     sprintf(text, "%0.15f", wvals.ymin);
     gtk_entry_set_text(GTK_ENTRY(elements->ymin.text), text);
     gtk_entry_set_position (GTK_ENTRY (elements->ymin.text), 0);
     /* gtk_entry_adjust_scroll (GTK_ENTRY (elements->ymin.text)); */
     sprintf(text, "%0.15f", wvals.ymax);
     gtk_entry_set_text(GTK_ENTRY(elements->ymax.text), text);
     gtk_entry_set_position (GTK_ENTRY (elements->ymax.text), 0);
     /* gtk_entry_adjust_scroll (GTK_ENTRY (elements->ymax.text)); */
     sprintf(text, "%0.15f", wvals.iter);
     gtk_entry_set_text(GTK_ENTRY(elements->iter.text), text);
     gtk_entry_set_position (GTK_ENTRY (elements->iter.text), 0);
     /* gtk_entry_adjust_scroll (GTK_ENTRY (elements->iter.text)); */
     sprintf(text, "%0.15f", wvals.cx);
     gtk_entry_set_text(GTK_ENTRY(elements->cx.text), text);
     gtk_entry_set_position (GTK_ENTRY (elements->cx.text), 0);
     /* gtk_entry_adjust_scroll (GTK_ENTRY (elements->cx.text)); */
     sprintf(text, "%0.15f", wvals.cy);
     gtk_entry_set_text(GTK_ENTRY(elements->cy.text), text);
     gtk_entry_set_position (GTK_ENTRY (elements->cy.text), 0);
     /* gtk_entry_adjust_scroll (GTK_ENTRY (elements->cy.text)); */

    elements->red.data->value = wvals.redstretch;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->red.data), "value_changed");
    sprintf(text, "%0.0f", wvals.redstretch);
    gtk_entry_set_text(GTK_ENTRY(elements->red.text), text);
    elements->green.data->value = wvals.greenstretch;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->green.data), "value_changed");
    sprintf(text, "%0.0f", wvals.greenstretch);
    gtk_entry_set_text(GTK_ENTRY(elements->green.text), text);
    elements->blue.data->value = wvals.bluestretch;
    gtk_signal_emit_by_name(GTK_OBJECT(elements->blue.data), "value_changed");
    sprintf(text, "%0.0f", wvals.bluestretch);
    gtk_entry_set_text(GTK_ENTRY(elements->blue.text), text);
    

    if (wvals.fractaltype==0) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_mandelbrot), wvals.fractaltype == 0);
    if (wvals.fractaltype==1) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_julia), wvals.fractaltype == 1);
    if (wvals.fractaltype==2) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_barnsley1), wvals.fractaltype == 2);
    if (wvals.fractaltype==3) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_barnsley2), wvals.fractaltype == 3);
    if (wvals.fractaltype==4) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_barnsley3), wvals.fractaltype == 4);
    if (wvals.fractaltype==5) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_spider), wvals.fractaltype == 5);
    if (wvals.fractaltype==6) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_manowar), wvals.fractaltype == 6);
    if (wvals.fractaltype==7) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_lambda), wvals.fractaltype == 7);
    if (wvals.fractaltype==8) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->type_sierpinski), wvals.fractaltype == 8);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->redmodesin), wvals.redmode == SINUS);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->redmodecos), wvals.redmode == COSINUS);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->redmodenone), wvals.redmode == NONE);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->greenmodesin), wvals.greenmode == SINUS);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->greenmodecos), wvals.greenmode == COSINUS);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->greenmodenone), wvals.greenmode == NONE);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->bluemodesin), wvals.bluemode == SINUS);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->bluemodecos), wvals.bluemode == COSINUS);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->bluemodenone), wvals.bluemode == NONE);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->redinvert), wvals.redinvert != FALSE);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->greeninvert), wvals.greeninvert != FALSE);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->blueinvert), wvals.blueinvert != FALSE);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->colormode0), wvals.colormode == 0);
    gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(elements->colormode1), wvals.colormode == 1);
    ready_now = TRUE;
}

/**********************************************************************
 FUNCTION: ok_warn_window
 *********************************************************************/

/* From testgtk */
static void
ok_warn_window(GtkWidget * widget,
	       gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}

/**********************************************************************
 FUNCTION: create_warn_dialog
 *********************************************************************/

void
create_warn_dialog(gchar * msg)
{
    GtkWidget          *window = NULL;
    GtkWidget          *label;
    GtkWidget          *button;

    window = gtk_dialog_new();

    gtk_window_set_title(GTK_WINDOW(window), "Warning");
    gtk_container_set_border_width(GTK_CONTAINER(window), 0);

    button = gtk_button_new_with_label("OK");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
		       (GtkSignalFunc) ok_warn_window,
		       window);

    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area), button, TRUE, TRUE, 0);
    gtk_widget_grab_default(button);
    gtk_widget_show(button);
    label = gtk_label_new(msg);
    gtk_misc_set_padding(GTK_MISC(label), 10, 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), label, TRUE, TRUE, 0);
    gtk_widget_show(label);
    gtk_widget_show(window);
    gtk_main();
    gdk_flush();

}

/**********************************************************************
 FUNCTION: save_options
 *********************************************************************/

void
save_options(FILE * fp)
{
  /* Save options */
  
    fprintf(fp, "fractaltype: %i\n", wvals.fractaltype);
    fprintf(fp, "xmin: %0.15f\n", wvals.xmin);
    fprintf(fp, "xmax: %0.15f\n", wvals.xmax);
    fprintf(fp, "ymin: %0.15f\n", wvals.ymin);
    fprintf(fp, "ymax: %0.15f\n", wvals.ymax);
    fprintf(fp, "iter: %0.15f\n", wvals.iter);
    fprintf(fp, "cx: %0.15f\n", wvals.cx);
    fprintf(fp, "cy: %0.15f\n", wvals.cy);
    fprintf(fp, "redstretch: %0.15f\n", wvals.redstretch);
    fprintf(fp, "greenstretch: %0.15f\n", wvals.greenstretch);
    fprintf(fp, "bluestretch: %0.15f\n", wvals.bluestretch);
    fprintf(fp, "redmode: %i\n", wvals.redmode);
    fprintf(fp, "greenmode: %i\n", wvals.greenmode);
    fprintf(fp, "bluemode: %i\n", wvals.bluemode);
    fprintf(fp, "redinvert: %i\n", wvals.redinvert);
    fprintf(fp, "greeninvert: %i\n", wvals.greeninvert);
    fprintf(fp, "blueinvert: %i\n", wvals.blueinvert);
    fprintf(fp, "colormode: %i\n", wvals.colormode);
    fputs("#**********************************************************************\n", fp);
    fprintf(fp, "<EOF>\n");
    fputs("#**********************************************************************\n", fp);
}

/**********************************************************************
 FUNCTION: save_callback
 *********************************************************************/

void
save_callback()
{
    FILE               *fp;
    gchar              *savename;

    savename = filename;

    fp = fopen(savename, "w+");

    if (!fp) {
	gchar               errbuf[MAXSTRLEN];

	sprintf(errbuf, msg[lng][MSG_SAVEERROR], savename);
	create_warn_dialog(errbuf);
	g_warning(errbuf);
	return;
    }
  /* Write header out */
    fputs(FRACTAL_HEADER, fp);
    fputs("#**********************************************************************\n", fp);
    fputs("# This is a data file for the Fractal Explorer plug-in for the GIMP   *\n", fp);
    fputs("# Get the plug-in at              http://www.multimania.com/cotting   *\n", fp);
    fputs("#**********************************************************************\n", fp);

    save_options(fp);

    if (ferror(fp))
	create_warn_dialog(msg[lng][MSG_WRITEFAILURE]);
    fclose(fp);
}

/**********************************************************************
 FUNCTION: file_selection_ok
 *********************************************************************/

void
file_selection_ok(GtkWidget * w,
		  GtkFileSelection * fs,
		  gpointer data)
{
    gchar              *filenamebuf;
    struct stat         filestat;
    gint                err;
    filenamebuf = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));

  /* Get the name */
    if (strlen(filenamebuf) == 0) {
	create_warn_dialog(msg[lng][MSG_NOFILENAME]);
	return;
    }
  /* Check if directory exists */
    err = stat(filenamebuf, &filestat);

    if (!err && S_ISDIR(filestat.st_mode)) {
      /* Can't save to directory */
	create_warn_dialog(msg[lng][MSG_NOSAVETODIR]);
	return;
    }
    filename = g_strdup(filenamebuf);
    save_callback();
    gtk_widget_destroy(GTK_WIDGET(fs));
}

/**********************************************************************
 FUNCTION: destroy_window
 *********************************************************************/

void
destroy_window(GtkWidget * widget,
	       GtkWidget ** window)
{
    *window = NULL;
}

/**********************************************************************
 FUNCTION: load_file_selection_ok
**********************************************************************/

void
load_file_selection_ok(GtkWidget * w,
		       GtkFileSelection * fs,
		       gpointer data)
{
    struct stat         filestat;
    gint                err;

    filename = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));

    err = stat(filename, &filestat);

    if (!err && S_ISREG(filestat.st_mode)) {
	explorer_load();
    }
    gtk_widget_destroy(GTK_WIDGET(fs));
    gtk_widget_show(maindlg);
    dialog_change_scale();
    set_cmap_preview();
    dialog_update_preview();
}


/**********************************************************************
 FUNCTION: create_load_file_selection
 *********************************************************************/


void
create_load_file_selection()
{
    GtkWidget   *window = NULL;

  /* Load a single object */
    window = gtk_file_selection_new(msg[lng][MSG_LOADWINTITLE]);
    gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);

    gtk_signal_connect(GTK_OBJECT(window), "destroy",
		       (GtkSignalFunc) destroy_window,
		       &window);

    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
		       "clicked", (GtkSignalFunc) load_file_selection_ok,
		       (gpointer) window);
    set_tooltip(tips, GTK_FILE_SELECTION(window)->ok_button, msg[lng][MSG_LOADBUTTONCOMMENT]);

    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(window)->cancel_button),
			      "clicked", (GtkSignalFunc) gtk_widget_destroy,
			      GTK_OBJECT(window));
    set_tooltip(tips, GTK_FILE_SELECTION(window)->cancel_button, msg[lng][MSG_CANCELLOAD]);
    if (!GTK_WIDGET_VISIBLE(window))
	gtk_widget_show(window);
}


/**********************************************************************
 FUNCTION: create_file_selection
 *********************************************************************/

void
create_file_selection()
{
    GtkWidget   *window = NULL;

    if (!window) {
	window = gtk_file_selection_new(msg[lng][MSG_SAVEWINTITLE]);
	gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);

	gtk_signal_connect(GTK_OBJECT(window), "destroy",
			   (GtkSignalFunc) destroy_window,
			   &window);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
			   "clicked", (GtkSignalFunc) file_selection_ok,
			   (gpointer) window);
	set_tooltip(tips, GTK_FILE_SELECTION(window)->ok_button, msg[lng][MSG_SAVEBUTTONCOMMENT]);

	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(window)->cancel_button),
				"clicked", (GtkSignalFunc) gtk_widget_destroy,
				  GTK_OBJECT(window));
	set_tooltip(tips, GTK_FILE_SELECTION(window)->cancel_button, msg[lng][MSG_CANCELSAVE]);
    }
    if(tpath)
      {
         gtk_file_selection_set_filename(GTK_FILE_SELECTION (window),tpath);
      }
    else
      /* Last path is where usually saved to */
      if(fractalexplorer_path_list)
      {
	gtk_file_selection_set_filename(GTK_FILE_SELECTION (window),
					g_list_nth(fractalexplorer_path_list,
						   g_list_length(fractalexplorer_path_list)-1)->data);
      }
    else
      gtk_file_selection_set_filename(GTK_FILE_SELECTION (window),"/tmp");

    if (!GTK_WIDGET_VISIBLE(window))
	gtk_widget_show(window);

}

/**********************************************************************
 FUNCTION: get_line
 *********************************************************************/

char *
get_line(gchar * buf, gint s, FILE * from, gint init)
{
    gint                slen;
    char               *ret;

    if (init)
	line_no = 1;
    else
	line_no++;

    do {
	ret = fgets(buf, s, from);
    }
    while (!ferror(from) && buf[0] == '#');

    slen = strlen(buf);

  /* The last newline is a pain */
    if (slen > 0)
	buf[slen - 1] = '\0';

    if (ferror(from)) {
	g_warning("Error reading file");
	return (0);
    }
    return (ret);
}

/**********************************************************************
 FUNCTION: load_options
 *********************************************************************/

gint
load_options(fractalexplorerOBJ * xxx, FILE * fp)
{
    gchar               load_buf[MAX_LOAD_LINE];
    gchar               str_buf[MAX_LOAD_LINE];
    gchar               opt_buf[MAX_LOAD_LINE];
    
    xxx->opts.fractaltype=0;
    xxx->opts.xmin=-2.0;
    xxx->opts.xmax=2.0;
    xxx->opts.ymin=-1.5;
    xxx->opts.ymax=1.5;
    xxx->opts.iter=50.0;
    xxx->opts.cx=-0.75;
    xxx->opts.cy=-0.2;
    xxx->opts.colormode=0;
    xxx->opts.redstretch=1.0;
    xxx->opts.greenstretch=1.0;
    xxx->opts.bluestretch=1.0;
    xxx->opts.redmode=1;
    xxx->opts.greenmode=1;
    xxx->opts.bluemode=1;
    xxx->opts.redinvert=0;
    xxx->opts.greeninvert=0;
    xxx->opts.blueinvert=0;
    xxx->opts.alwayspreview=1;
    
    get_line(load_buf, MAX_LOAD_LINE, fp, 0);

    while (!feof(fp) && strcmp(load_buf, "<EOF>")) {
      /* Get option name */

	sscanf(load_buf, "%s %s", str_buf, opt_buf);

	if (!strcmp(str_buf, "fractaltype:")) {
	  /* Value is decimal */
	    int                 sp = 0;

	    sp = atoi(opt_buf);
	    if (sp < 0)
		return (-1);
	    xxx->opts.fractaltype = sp;
	} else if (!strcmp(str_buf, "xmin:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.xmin = sp;
	} else if (!strcmp(str_buf, "xmax:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.xmax = sp;
	} else if (!strcmp(str_buf, "ymin:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.ymin = sp;
	} else if (!strcmp(str_buf, "ymax:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.ymax = sp;
	} else if (!strcmp(str_buf, "redstretch:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.redstretch = sp;
	} else if (!strcmp(str_buf, "greenstretch:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.greenstretch = sp;
	} else if (!strcmp(str_buf, "bluestretch:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.bluestretch = sp;
	} else if (!strcmp(str_buf, "iter:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.iter = sp;
	} else if (!strcmp(str_buf, "cx:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.cx = sp;
	} else if (!strcmp(str_buf, "cy:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    xxx->opts.cy = sp;
	} else if (!strcmp(str_buf, "redmode:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    xxx->opts.redmode = sp;
	} else if (!strcmp(str_buf, "greenmode:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    xxx->opts.greenmode = sp;
	} else if (!strcmp(str_buf, "bluemode:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    xxx->opts.bluemode = sp;
	} else if (!strcmp(str_buf, "redinvert:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    xxx->opts.redinvert = sp;
	} else if (!strcmp(str_buf, "greeninvert:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    xxx->opts.greeninvert = sp;
	} else if (!strcmp(str_buf, "blueinvert:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    xxx->opts.blueinvert = sp;
	} else if (!strcmp(str_buf, "colormode:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    xxx->opts.colormode = sp;
	}
	get_line(load_buf, MAX_LOAD_LINE, fp, 0);
    }
    return (0);
}


/**********************************************************************
 FUNCTION: gradient_load_options
 *********************************************************************/

gint
gradient_load_options(gradientOBJ * xxx, FILE * fp)
{
    gchar               load_buf[MAX_LOAD_LINE];
    gchar               str_buf[MAX_LOAD_LINE];
    gchar               opt_buf[MAX_LOAD_LINE];
    
    get_line(load_buf, MAX_LOAD_LINE, fp, 0);

    while (!feof(fp) && strcmp(load_buf, "<EOF>")) {
      /* Get option name */

	sscanf(load_buf, "%s %s", str_buf, opt_buf);

	if (!strcmp(str_buf, "redstretch:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    wvals.redstretch = sp;
	} else if (!strcmp(str_buf, "greenstretch:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    wvals.greenstretch = sp;
	} else if (!strcmp(str_buf, "bluestretch:")) {
	  /* Value is double */
	    double              sp = 0;

	    sp = atof(opt_buf);
	    wvals.bluestretch = sp;
	} else if (!strcmp(str_buf, "redmode:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    wvals.redmode = sp;
	} else if (!strcmp(str_buf, "greenmode:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    wvals.greenmode = sp;
	} else if (!strcmp(str_buf, "bluemode:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    wvals.bluemode = sp;
	} else if (!strcmp(str_buf, "redinvert:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    wvals.redinvert = sp;
	} else if (!strcmp(str_buf, "greeninvert:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    wvals.greeninvert = sp;
	} else if (!strcmp(str_buf, "blueinvert:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    wvals.blueinvert = sp;
	} else if (!strcmp(str_buf, "colormode:")) {
	    int              sp = 0;

	    sp = atoi(opt_buf);
	    wvals.colormode = sp;
	}
	get_line(load_buf, MAX_LOAD_LINE, fp, 0);
    }
    return (0);
}

/**********************************************************************
 FUNCTION: explorer_load
 *********************************************************************/

void
explorer_load()
{
    FILE               *fp;
    gchar               load_buf[MAX_LOAD_LINE];

    g_assert(filename != NULL); 
    fp = fopen(filename, "r");
    if (!fp) {
	g_warning(msg[lng][MSG_OPENERROR], filename);
	return;
    }
    get_line(load_buf, MAX_LOAD_LINE, fp, 1);

    if (strncmp(FRACTAL_HEADER, load_buf, strlen(load_buf))) {
	gchar               err[MAXSTRLEN];

	sprintf(err, msg[lng][MSG_WRONGFILETYPE], filename);
	create_warn_dialog(err);
	return;
    }
    if (load_options(current_obj,fp)) {
      /* waste some mem */
	gchar               err[MAXSTRLEN];

	sprintf(err,
		msg[lng][MSG_CORRUPTFILE],
		filename,
		line_no);
	create_warn_dialog(err);
	return;
    }
    wvals=current_obj->opts;
    fclose(fp);
}
