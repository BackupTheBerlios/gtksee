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

#ifndef __VIEWERSTATUS_H__
#define __VIEWERSTATUS_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

GtkWidget*	get_viewer_status_bar	(GdkPixmap *pixmap,
					 GdkBitmap *mask);
void		set_viewer_status_type	(GdkPixmap *pixmap, GdkBitmap *mask);
void		set_viewer_status_name	(gchar *text);
void		set_viewer_status_size	(glong size);
void		set_viewer_status_prop	(gchar *text);
void		set_viewer_status_zoom	(gint n);

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif

