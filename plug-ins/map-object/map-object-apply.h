#ifndef __MAPOBJECT_APPLY_H__
#define __MAPOBJECT_APPLY_H__

extern gdouble imat[4][4];
extern gfloat  rotmat[16];

void init_compute     (void);
void compute_image    (void);
void copy_from_config (GimpProcedureConfig  *config);

#endif  /* __MAPOBJECT_APPLY_H__ */
