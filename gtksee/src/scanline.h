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

#ifndef __SCANLINE_H__
#define __SCANLINE_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>
#include "gtypes.h"

#define SCANLINE_INTERACT 1
#define SCANLINE_DISPLAY 2
#define SCANLINE_PREVIEW 4

void		scanline_init	(GdkColorContext *gcc, GtkStyle *style);
void		scanline_start	();
void		scanline_cancel	();
void		load_scaled_image
				(gchar *orig_filename,
				 ImageInfo *info,
				 gint max_width,
				 gint max_height,
				 GtkWidget *window,
				 GtkWidget *container,
				 gint scanflags);
ImageCache*	scanline_get_cache
				(void);

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif
