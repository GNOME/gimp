#ifndef __GIMPBRUSHLISTP_H__
#define __GIMPBRUSHLISTP_H__

#include "gimplistP.h"
#include "gimpbrushlist.h"

struct _GimpBrushList
{
  GimpList gimplist;

  gint     num_brushes;
};

typedef struct _GimpBrushListClass GimpBrushListClass;

struct _GimpBrushListClass
{
  GimpListClass parent_class;
};

#define BRUSH_LIST_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gimp_brush_list_get_type(), GimpBrushListClass)

#endif /* __GIMPBRUSHLISTP_H__ */
