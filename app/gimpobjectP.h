#ifndef __GIMP_OBJECT_P_H__
#define __GIMP_OBJECT_P_H__

#include <gtk/gtkobject.h>
#include "gimpobjectF.h"

struct _GimpObject
{
	GtkObject object;
};

typedef struct
{
	GtkObjectClass parent_class;
}GimpObjectClass;

guint gimp_object_get_type(void);
#endif
