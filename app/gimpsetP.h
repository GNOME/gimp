#ifndef __GIMPSETP_H__
#define __GIMPSETP_H__

#include "gimpobjectP.h"
#include "gimpset.h"

struct _GimpSet{
	GimpObject gobject;
	GtkType type;
	GSList* list;
	GArray* handlers;
	gboolean weak;
	gpointer active_element;
};

struct _GimpSetClass{
	GimpObjectClass parent_class;
};

typedef struct _GimpSetClass GimpSetClass;

#define GIMP_SET_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_set_get_type(), GimpSetClass)
     

#endif
