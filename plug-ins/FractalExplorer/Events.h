#ifndef __FRACTALEXPLORER_EVENTS_H__
#define __FRACTALEXPLORER_EVENTS_H__


/* Preview events */
gint                preview_button_press_event(GtkWidget * widget, GdkEventButton * event);
gint                preview_button_release_event(GtkWidget * widget, GdkEventButton * event);
gint                preview_motion_notify_event(GtkWidget * widget, GdkEventButton * event);
gint                preview_leave_notify_event(GtkWidget * widget, GdkEventButton * event);
gint                preview_enter_notify_event(GtkWidget * widget, GdkEventButton * event);

#endif

