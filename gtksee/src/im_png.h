/*
 * GTK See -- an image viewer based on GTK+
 * Copyright (C) 1998 Hotaru Lee <jkhotaru@mail.sti.com.cn> <hotaru@163.net>
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
 */

#ifndef __IM_PNG_H__
#define __IM_PNG_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#ifdef HAVE_LIBPNG

#include <gtk/gtk.h>
#include <png.h>

typedef	gboolean	(*PngLoadFunc)		(guchar *buffer, gint width, gint left, gint scanline, gint components, gint pass, gint mode);

gboolean	png_get_header		(gchar *filename, png_info *info);
gboolean	png_load		(gchar *filename, PngLoadFunc func);

#endif /* HAVE_LIBPNG */

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif

