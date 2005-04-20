#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include <gtk/gtk.h>


typedef enum
{
  SMALL,
  MEDIUM,
  LARGE,
  ASIS
} WidgetSize;

typedef struct WidgetInfo
{
  GtkWidget *window;
  gchar *name;
  gboolean no_focus;
  gboolean include_decorations;
  WidgetSize size;
} WidgetInfo;

GList *get_all_widgets (void);


#endif /* __WIDGETS_H__ */
