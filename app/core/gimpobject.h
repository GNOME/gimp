#ifndef __GIMP_OBJECT_H__
#define __GIMP_OBJECT_H__

#include "gimpobjectF.h"

#define GIMP_OBJECT(obj) \
GTK_CHECK_CAST (obj, gimp_object_get_type (), GimpObject)
#define GIMP_OBJECT_CLASS(klass) \
GTK_CHECK_CLASS_CAST (klass, gimp_object_get_type(), GimpObjectClass)
#define GIMP_IS_OBJECT(obj) \
GTK_CHECK_TYPE (obj, gimp_object_get_type())

guint gimp_object_get_type(void);

#endif




	
