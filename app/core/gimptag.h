
#ifndef __GIMP_TAG_H__
#define __GIMP_TAG_H__

GimpTag         gimp_tag_from_string   (const gchar      *string);
#define         gimp_tag_to_string      g_quark_to_string

int             gimp_tag_compare_func  (const void       *p1,
                                        const void       *p2);

#endif // __GIMP_TAG_H__

