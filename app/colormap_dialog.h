#ifndef _g_gimp_colormap_dialog_funcs
#define _g_gimp_colormap_dialog_funcs
#include <gtk/gtkdialog.h>
#include <colormap_dialog.t.h>
#include <gimpset.h>
#include <gimpimage.h>
#include <glib.h>
GimpColormapDialog* gimp_colormap_dialog_create (
	GimpSet* context);
GimpImage* gimp_colormap_dialog_image (
	const GimpColormapDialog* colormap_dialog);
gint gimp_colormap_dialog_col_index (
	const GimpColormapDialog* colormap_dialog);
typedef void (*GimpColormapDialogHandler_selected)(GimpColormapDialog*,
	gpointer);
void gimp_colormap_dialog_connect_selected (
	GimpColormapDialogHandler_selected handler,
	gpointer user_data,
	GimpColormapDialog* colormap_dialog);
#endif /* _g_gimp_colormap_dialog_funcs */
