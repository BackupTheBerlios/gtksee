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

#ifndef __GTKSEETOOLBAR_H__
#define __GTKSEETOOLBAR_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>
#include "gtypes.h"

void		toolbar_cdup		(GtkWidget *widget, gpointer data);
void		toolbar_refresh		(GtkWidget *widget, gpointer data);
void		toolbar_view		(GtkWidget *widget, gpointer data);
void		toolbar_about		(GtkWidget *widget, gpointer data);
void		toolbar_thumbnails	(GtkWidget *widget, gpointer data);
void		toolbar_details		(GtkWidget *widget, gpointer data);
void		toolbar_small_icons	(GtkWidget *widget, gpointer data);

GtkWidget*	get_main_toolbar	(GtkWidget *parent);
void		toolbar_view_enable	(gboolean e);
void		toolbar_remove_enable	(gboolean e);
void		toolbar_rename_enable	(gboolean e);
void		toolbar_set_list_type	(ImageListType type);

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif
