#ifndef __GIMP_OBJECT_H__
#define __GIMP_OBJECT_H__

#include <gtk/gtktypeutils.h>
#include "gimpobjectF.h"

#define GIMP_TYPE_OBJECT gimp_object_get_type()

#define GIMP_OBJECT(obj) GTK_CHECK_CAST (obj, GIMP_TYPE_OBJECT, GimpObject)

#define GIMP_IS_OBJECT(obj) GTK_CHECK_TYPE (obj, GIMP_TYPE_OBJECT)


guint gimp_object_get_type(void);

#endif




	
