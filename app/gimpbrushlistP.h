#ifndef __GIMPBRUSHLISTP_H__
#define __GIMPBRUSHLISTP_H__

#include "gimplistP.h"
#include "gimpbrushlist.h"

struct _GimpBrushList
{
  GimpList gimplist;
  int num_brushes;
};

struct _GimpBrushListClass
{
  GimpListClass parent_class;
};

typedef struct _GimpBrushListClass GimpBrushListClass;


#define BRUSH_LIST_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gimp_brush_list_get_type(), GimpBrushListClass)

#endif /* __GIMPBRUSHLISTP_H__ */
