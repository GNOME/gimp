/*
 * gimprational.h
 *
 *  Created on: 19.09.2014
 *      Author: kuhse
 */

#ifndef __GIMPRATIONAL_H__
#define __GIMPRATIONAL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GIMP_TYPE_RATIONAL             (rational_get_type  ())
#define GIMP_TYPE_SRATIONAL            (srational_get_type ())

typedef struct _RationalValue RationalValue;

struct _RationalValue
{
  guint nom;
  guint denom;
};

typedef struct _SRationalValue SRationalValue;

struct _SRationalValue
{
  gint  nom;
  guint denom;
};

typedef struct _Rational Rational;

struct _Rational
{
  GArray *rational_array;
  gint length;
};

typedef struct _SRational SRational;

struct _SRational
{
  GArray *srational_array;
  gint length;
};

GType                          rational_get_type                     (void) G_GNUC_CONST;
GType                          srational_get_type                    (void) G_GNUC_CONST;

void                           rational_free                         (gpointer             data);
void                           srational_free                        (gpointer             data);

void                           string_to_rational                    (gchar               *string,
                                                                      Rational           **rational);
void                           rational_to_int                       (Rational            *rational,
                                                                      gint               **nom,
                                                                      gint               **denom,
                                                                      gint                *length);
void                           string_to_srational                   (gchar               *string,
                                                                      SRational          **srational);


G_END_DECLS

#endif /* __GIMPRATIONAL_H__ */
