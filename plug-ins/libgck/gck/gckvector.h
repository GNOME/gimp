/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               */
/***************************************************************************/

#ifndef __GCKVECTOR_H__
#define __GCKVECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Two dimensional vector functions */
/* ================================ */

double     gck_vector2_inner_product (GckVector2 *a,GckVector2 *b);
GckVector3 gck_vector2_cross_product (GckVector2 *a,GckVector2 *b);
double     gck_vector2_length        (GckVector2 *a);
void       gck_vector2_normalize     (GckVector2 *a);
void       gck_vector2_mul           (GckVector2 *a,double b);
void       gck_vector2_sub           (GckVector2 *c,GckVector2 *a,GckVector2 *b);
void       gck_vector2_set           (GckVector2 *a, double x,double y);
void       gck_vector2_add           (GckVector2 *c,GckVector2 *a,GckVector2 *b);
void       gck_vector2_neg           (GckVector2 *a);
void       gck_vector2_rotate        (GckVector2 *v,double alpha);

/* Three dimensional vector functions */
/* ================================== */

double     gck_vector3_inner_product (GckVector3 *a,GckVector3 *b);
GckVector3 gck_vector3_cross_product (GckVector3 *a,GckVector3 *b);
double     gck_vector3_length        (GckVector3 *a);
void       gck_vector3_normalize     (GckVector3 *a);
void       gck_vector3_mul           (GckVector3 *a,double b);
void       gck_vector3_sub           (GckVector3 *c,GckVector3 *a,GckVector3 *b);
void       gck_vector3_set           (GckVector3 *a, double x,double y,double z);
void       gck_vector3_add           (GckVector3 *c,GckVector3 *a,GckVector3 *b);
void       gck_vector3_neg           (GckVector3 *a);
void       gck_vector3_rotate        (GckVector3 *v,double alpha,double beta,double gamma);

void gck_2d_to_3d(int sx,int sy,int w,int h,int x,int y,GckVector3 *vp,GckVector3 *p);
void gck_3d_to_2d(int sx,int sy,int w,int h,double *x,double *y,GckVector3 *vp,GckVector3 *p);

#ifdef __cplusplus
}
#endif

#endif
