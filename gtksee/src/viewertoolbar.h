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

#ifndef __VIEWERTOOLBAR_H__
#define __VIEWERTOOLBAR_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#define LEFT      1
#define RIGHT     2
#define RIGHTLEFT 3
#define UPDOWN    4

void		viewer_toolbar_browse		(GtkWidget *widget, gpointer data);
void		viewer_toolbar_full_screen	(GtkWidget *widget, gpointer data);
void		viewer_toolbar_save_image	(GtkWidget *widget, gpointer data);
void		viewer_toolbar_next_image	(GtkWidget *widget, gpointer data);
void		viewer_toolbar_prev_image	(GtkWidget *widget, gpointer data);
void		viewer_toolbar_slideshow_toggled(GtkWidget *widget, gpointer data);
void		viewer_toolbar_fitscreen_toggled(GtkWidget *widget, gpointer data);
void		viewer_toolbar_rotate	(GtkWidget *widget, gpointer data);
void		viewer_toolbar_reflect	(GtkWidget *widget, gpointer data);
void		viewer_toolbar_refresh		(GtkWidget *widget, gpointer data);
void		viewer_save_enable		(gboolean e);
void		viewer_next_enable		(gboolean e);
void		viewer_prev_enable		(gboolean e);
void		viewer_slideshow_set_state	(gboolean e);

GtkWidget*	get_viewer_toolbar		(GtkWidget *parent);

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif
