/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PLUG_IN_H__
#define __GIMP_PLUG_IN_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PLUG_IN            (gimp_plug_in_get_type ())
#define GIMP_PLUG_IN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUG_IN, GimpPlugIn))
#define GIMP_PLUG_IN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUG_IN, GimpPlugInClass))
#define GIMP_IS_PLUG_IN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUG_IN))
#define GIMP_IS_PLUG_IN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PLUG_IN))
#define GIMP_PLUG_IN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PLUG_IN, GimpPlugInClass))


typedef struct _GimpPlugInClass   GimpPlugInClass;
typedef struct _GimpPlugInPrivate GimpPlugInPrivate;

struct _GimpPlugIn
{
  GObject            parent_instance;

  GimpPlugInPrivate *priv;
};

struct _GimpPlugInClass
{
  GObjectClass  parent_class;

  void             (* quit)             (GimpPlugIn  *plug_in);

  gchar         ** (* init_procedures)  (GimpPlugIn  *plug_in,
                                         gint        *n_procedures);
  gchar         ** (* query_procedures) (GimpPlugIn  *plug_in,
                                         gint        *n_procedures);

  GimpProcedure  * (* create_procedure) (GimpPlugIn  *plug_in,
                                         const gchar *name);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GType           gimp_plug_in_get_type               (void) G_GNUC_CONST;

void            gimp_plug_in_set_translation_domain (GimpPlugIn    *plug_in,
                                                     const gchar   *domain_name,
                                                     GFile         *domain_path);
void            gimp_plug_in_set_help_domain        (GimpPlugIn    *plug_in,
                                                     const gchar   *domain_name,
                                                     GFile         *domain_uri);

void            gimp_plug_in_add_menu_branch        (GimpPlugIn    *plug_in,
                                                     const gchar   *menu_path,
                                                     const gchar   *menu_label);

GimpProcedure * gimp_plug_in_create_procedure       (GimpPlugIn    *plug_in,
                                                     const gchar   *name);

void            gimp_plug_in_add_temp_procedure     (GimpPlugIn    *plug_in,
                                                     GimpProcedure *procedure);
void            gimp_plug_in_remove_temp_procedure  (GimpPlugIn    *plug_in,
                                                     const gchar   *name);

GList         * gimp_plug_in_get_temp_procedures    (GimpPlugIn    *plug_in);
GimpProcedure * gimp_plug_in_get_temp_procedure     (GimpPlugIn    *plug_in,
                                                     const gchar   *name);


G_END_DECLS

#endif /* __GIMP_PLUG_IN_H__ */
