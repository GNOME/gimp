/*
 * GIMP Dynamic Text -- This is a plug-in for The GIMP 1.0
 * Copyright (C) 1998,1999,2000 Marco Lamberto <lm@geocities.com>
 * Web page: http://www.geocities.com/Tokyo/1474/gimp/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gdyntext.h"
#include "font_selection.h"


gboolean gdt_compat_load(GdtVals *data)
{
	gchar *gdtparams = NULL;
	gchar *gdtparams0 = NULL;
	gchar **params = NULL;
	gchar font_family[1024];
	gchar font_style[1024];
	gint	font_size;
	gint	font_metric;
	GimpParasite *parasite = NULL;


	if ((parasite = gimp_drawable_parasite_find(data->drawable_id,
		GDYNTEXT_PARASITE_144)) != NULL) {
		/* GDynText 1.4.4 - xxxxx uses one parasite */
		gdtparams = strdup(gimp_parasite_data(parasite));
		gimp_parasite_free(parasite);
	} else if ((parasite = gimp_drawable_parasite_find(data->drawable_id,
		GDYNTEXT_PARASITE_131)) != NULL) {
		/* GDynText 1.3.1 - 1.4.3 uses one parasite */
		gdtparams = strdup(gimp_parasite_data(parasite));
		gimp_parasite_free(parasite);
	} else if ((parasite = gimp_drawable_parasite_find(data->drawable_id,
		GDYNTEXT_PARASITE_130_MAGIC)) != NULL) {
		/* GDynText 1.3.0 uses too parasites and no serialization!! */
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_TEXT);
		strncpy(data->text, gimp_parasite_data(parasite), gimp_parasite_data_size(parasite));
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_FONT_FAMILY);
		strncpy(font_family, gimp_parasite_data(parasite), gimp_parasite_data_size(parasite));
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_FONT_STYLE);
		strncpy(font_style, gimp_parasite_data(parasite), gimp_parasite_data_size(parasite));
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_FONT_SIZE);
		font_size = *(gint32*)gimp_parasite_data(parasite);
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_FONT_METRIC);
		font_metric = *(gint*)gimp_parasite_data(parasite);
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_FONT_COLOR);
		data->color = *(gint32*)gimp_parasite_data(parasite);
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_ANTIALIAS);
		data->antialias = *(gboolean*)gimp_parasite_data(parasite);
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_ALIGNMENT);
		data->alignment = *(GdtAlign*)gimp_parasite_data(parasite);
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_ROTATION);
		data->rotation = *(gint*)gimp_parasite_data(parasite);
		gimp_parasite_free(parasite);
		parasite = gimp_drawable_parasite_find(data->drawable_id,
			GDYNTEXT_PARASITE_130_PREVIEW);
		data->preview = *(gboolean*)gimp_parasite_data(parasite);
		gimp_parasite_free(parasite);

		/* FIXME: don't exit here!! */
		return TRUE;
	}

	if (gdtparams == NULL)
		gdtparams = gimp_layer_get_name(data->layer_id);

	if (!gimp_drawable_has_alpha(data->drawable_id) ||
		strncmp(gdtparams, "GDT", 3) != 0 ||
		GDT_MAGIC_REV(gdtparams) == GDT_MAGIC_REV(GDYNTEXT_MAGIC))
	{
		return FALSE;
	} else if (GDT_MAGIC_REV(gdtparams) > GDT_MAGIC_REV(GDYNTEXT_MAGIC)) {
	        static gchar *message = NULL;
		if (!message) 
		  message = g_strdup_printf (_(" WARNING: GDynText is too old!"
					       " A newer version is required to handle this layer."
					       " Get it from %s"), GDYNTEXT_WEB_PAGE);
		
	        data->messages = g_list_append (data->messages, message);
		return TRUE;
	}

	gdtparams0 = g_strndup(gdtparams + 6, strlen(gdtparams) - 7);
	params = g_strsplit(gdtparams0, "}{", -1);
	g_free(gdtparams0);

	data->new_layer		= FALSE;
	data->color				= strtol(params[C_FONT_COLOR], (char **)NULL, 16);
	data->antialias		= atoi(params[C_ANTIALIAS]);
	font_size					= atoi(params[C_FONT_SIZE]);
	font_metric				= atoi(params[C_FONT_SIZE_T]);
	
	/* older GDT < 0.6 formats don't have alignment */
	data->alignment = GDT_MAGIC_REV(gdtparams) < 6 ? LEFT : atoi(params[C_ALIGNMENT]);
	
	/* older GDT < 0.7 formats don't have rotation */
	data->rotation = GDT_MAGIC_REV(gdtparams) < 7 ? 0 : atoi(params[C_ROTATION]);
	
	strncpy(data->text, params[C_TEXT], sizeof(data->text));
	{
		gchar *text = strunescape(data->text);
		g_snprintf(data->text, sizeof(data->text), "%s", text);
		g_free(text);
	}
	strncpy(font_family, params[C_FONT_FAMILY], sizeof(font_family));
	
	/* older GDT < 0.8 formats don't have font style */
	strncpy(font_style, (GDT_MAGIC_REV(gdtparams) < 8 ?
		"" : params[C_FONT_STYLE]), sizeof(font_style));
	
	/* older GDT < 0.9 formats don't have line spacing */
	data->line_spacing = GDT_MAGIC_REV(gdtparams) < 9 ?
		0 : atoi(params[C_SPACING]);

	/* GDT <= 0.9 doesn't have layer alignment */
	data->layer_alignment	= LA_NONE;

	g_snprintf(data->xlfd, sizeof(data->xlfd),
		font_metric == FONT_METRIC_PIXELS ?
		"-*-%s-%s-*-%d-*-*-*-*-*-*-*" :
		"-*-%s-%s-*-*-%d-*-*-*-*-*-*",
		font_family, font_style,
		font_metric == FONT_METRIC_PIXELS ? font_size : font_size * 10);

	if (GDT_MAGIC_REV(gdtparams) < GDT_MAGIC_REV(GDYNTEXT_MAGIC)) {
	        static gchar *message = NULL;
		if (!message) 
		  message = g_strdup_printf (_(" Upgrading old GDynText layer to %s."), GDYNTEXT_MAGIC);
		
	        data->messages = g_list_append (data->messages, message);
	}
	
#ifdef DEBUG
	printf("gdt_compat_load:\n  '%s'\n  '%s'\n", gdtparams, data->xlfd);
#endif
	
	g_free(gdtparams);

	return TRUE;
}

/* vim: set ts=2 sw=2 tw=79 ai nowrap: */
