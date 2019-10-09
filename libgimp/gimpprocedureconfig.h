/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpprocedureconfig.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_PROCEDURE_CONFIG_H__
#define __GIMP_PROCEDURE_CONFIG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PROCEDURE_CONFIG            (gimp_procedure_config_get_type ())
#define GIMP_PROCEDURE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROCEDURE_CONFIG, GimpProcedureConfig))
#define GIMP_PROCEDURE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROCEDURE_CONFIG, GimpProcedureConfigClass))
#define GIMP_IS_PROCEDURE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROCEDURE_CONFIG))
#define GIMP_IS_PROCEDURE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROCEDURE_CONFIG))
#define GIMP_PROCEDURE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROCEDURE_CONFIG, GimpProcedureConfigClass))


typedef struct _GimpProcedureConfigClass   GimpProcedureConfigClass;
typedef struct _GimpProcedureConfigPrivate GimpProcedureConfigPrivate;

struct _GimpProcedureConfig
{
  GObject                     parent_instance;

  GimpProcedureConfigPrivate *priv;
};

struct _GimpProcedureConfigClass
{
  GObjectClass  parent_class;

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


GType   gimp_procedure_config_get_type      (void) G_GNUC_CONST;

GimpProcedure *
        gimp_procedure_config_get_procedure (GimpProcedureConfig *config);

void    gimp_procedure_config_set_values    (GimpProcedureConfig  *config,
                                             const GimpValueArray *values);
void    gimp_procedure_config_get_values    (GimpProcedureConfig  *config,
                                             GimpValueArray       *values);

void    gimp_procedure_config_begin_run     (GimpProcedureConfig  *config,
                                             GimpImage            *image,
                                             GimpRunMode           run_mode,
                                             const GimpValueArray *args);
void    gimp_procedure_config_end_run       (GimpProcedureConfig  *config,
                                             GimpPDBStatusType     status);

GimpMetadata *
        gimp_procedure_config_begin_export  (GimpProcedureConfig  *config,
                                             GimpImage            *original_image,
                                             GimpRunMode           run_mode,
                                             const GimpValueArray *args,
                                             const gchar          *mime_type);
void    gimp_procedure_config_end_export    (GimpProcedureConfig  *config,
                                             GimpImage            *exported_image,
                                             GFile                *file,
                                             GimpPDBStatusType     status);

G_END_DECLS

#endif /* __GIMP_PROCEDURE_CONFIG_H__ */
