#ifndef _g_gimp_colormap_dialog_prot
#define _g_gimp_colormap_dialog_prot
#include <colormap_dialog.h>
#include <gtk/gtkdialog.h>
#include <gimpimage.h>
#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkpreview.h>
#include <gtk/gtkoptionmenu.h>
#include <gimpset.h>
#include <gtk/gtkentry.h>
#include <color_notebook.h>


typedef struct _GimpColormapDialogClass GimpColormapDialogClass;
struct _GimpColormapDialogClass {
	GtkDialogClass parent_class;
	void (*selected) (GimpColormapDialog*);
};
struct _GimpColormapDialog {
	GtkDialog parent;
	GimpImage* image;
	gint col_index;
        gint dnd_col_index;
	GtkWidget* vbox;
	GtkPreview* palette;
	GtkWidget* image_menu;
	GtkOptionMenu* option_menu;
	GimpSet* context;
	guint event_handler;
	gint xn;
	gint yn;
	gint cellsize;
	GtkWidget* index_spinbutton;
	GtkAdjustment* index_adjustment;
	GtkEntry* color_entry;
	GimpSetHandlerId rename_handler;
	GimpSetHandlerId cmap_changed_handler;
	GtkWidget* add_item;
	ColorNotebookP color_notebook;
};
void gimp_colormap_dialog_selected (
	GimpColormapDialog* colormap_dialog);
#endif /* _g_gimp_colormap_dialog_prot */
