#ifndef __GIMP_OBJECT_P_H__
#define __GIMP_OBJECT_P_H__

#include <gtk/gtkobject.h>
#include "gimpobject.h"

struct _GimpObject
{
	GtkObject object;
};

typedef struct
{
	GtkObjectClass parent_class;
}GimpObjectClass;

#define GIMP_OBJECT_CLASS(klass) \
GTK_CHECK_CLASS_CAST (klass, GIMP_TYPE_OBJECT, GimpObjectClass)


#define GIMP_TYPE_INIT(typevar, obtype, classtype, obinit, classinit, parent) \
if(!typevar){ \
	GtkTypeInfo _info={#obtype, \
			   sizeof(obtype), \
			   sizeof(classtype), \
			   (GtkClassInitFunc)classinit, \
			   (GtkObjectInitFunc)obinit, \
			   NULL, NULL, NULL}; \
	typevar=gtk_type_unique(parent, &_info); \
}
#endif
