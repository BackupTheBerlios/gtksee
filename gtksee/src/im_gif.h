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

#ifndef __IM_GIF_H__
#define __IM_GIF_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

typedef guchar *GifColormap[3];

typedef struct gif_screen_struct
{
	unsigned int Width;
	unsigned int Height;
	GifColormap ColorMap;
	unsigned int BitPixel;
	unsigned int ColorResolution;
	unsigned int Background;
	unsigned int AspectRatio;
} gif_screen;

typedef	gboolean	(*GifLoadFunc)	(guint* buffer, GifColormap cmap, gint transparent, gint width, gint scanline, gint frame);

gboolean	gif_get_header	(gchar *filename, gif_screen *screen);
gchar*		gif_load	(gchar *filename, GifLoadFunc func);

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif

