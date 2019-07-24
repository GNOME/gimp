#ifndef __WIDGETS_H__
#define __WIDGETS_H__


typedef enum
{
  SMALL,
  MEDIUM,
  LARGE,
  ASIS
} WidgetSize;

typedef struct WidgetInfo
{
  GtkWidget  *widget;
  gchar      *name;
  gboolean    no_focus;
  gboolean    show_all;
  WidgetSize  size;
} WidgetInfo;

GList * get_all_widgets (void);


#endif /* __WIDGETS_H__ */
