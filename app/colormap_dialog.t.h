#ifndef _g_gimp_colormap_dialog_type
#define _g_gimp_colormap_dialog_type
#include <gtk/gtktypeutils.h>


#define GIMP_TYPE_COLORMAP_DIALOG \
 (_gimp_colormap_dialog_type ? (void)0 : _gimp_colormap_dialog_init_type (), _gimp_colormap_dialog_type)
extern GtkType _gimp_colormap_dialog_type;
void _gimp_colormap_dialog_init_type (void);
typedef struct _GimpColormapDialog GimpColormapDialog;
#define GIMP_IS_COLORMAP_DIALOG(o) GTK_CHECK_TYPE(o, GIMP_TYPE_COLORMAP_DIALOG)
#define GIMP_COLORMAP_DIALOG(o) GTK_CHECK_CAST(o, GIMP_TYPE_COLORMAP_DIALOG, GimpColormapDialog)
#endif /* _g_gimp_colormap_dialog_type */
