/*
 * GTK See -- a image viewer based on GTK+
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

#ifndef __GTKSEEMENU_H__
#define __GTKSEEMENU_H__

#ifdef __cplusplus
	extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#include "gtypes.h"

void		menu_file_view		(GtkWidget *widget, gpointer data);
void		menu_file_full_view	(GtkWidget *widget, gpointer data);
void		menu_file_quit		(GtkWidget *widget, gpointer data);
void		menu_edit_cut		(GtkWidget *widget, gpointer data);
void		menu_edit_copy		(GtkWidget *widget, gpointer data);
void		menu_edit_paste		(GtkWidget *widget, gpointer data);
void		menu_edit_paste_link	(GtkWidget *widget, gpointer data);
void		menu_edit_select_all	(GtkWidget *widget, gpointer data);
void		menu_edit_delete	(GtkWidget *widget, gpointer data);
void		menu_view_hide		(GtkWidget *widget, gpointer data);
void		menu_view_show_hidden	(GtkWidget *widget, gpointer data);
void		menu_view_thumbnails	(GtkWidget *widget, gpointer data);
void		menu_view_small_icons	(GtkWidget *widget, gpointer data);
void		menu_view_details	(GtkWidget *widget, gpointer data);
void		menu_view_preview	(GtkWidget *widget, gpointer data);
void		menu_view_sort_by_name	(GtkWidget *widget, gpointer data);
void		menu_view_sort_by_size	(GtkWidget *widget, gpointer data);
void		menu_view_sort_by_property
					(GtkWidget *widget, gpointer data);
void		menu_view_sort_by_date	(GtkWidget *widget, gpointer data);
void		menu_view_sort_ascend	(GtkWidget *widget, gpointer data);
void		menu_view_sort_descend	(GtkWidget *widget, gpointer data);
void		menu_view_refresh	(GtkWidget *widget, gpointer data);
void		menu_view_quick_refresh	(GtkWidget *widget, gpointer data);
void		menu_view_auto_refresh	(GtkWidget *widget, gpointer data);
void		menu_tools_slide_show	(GtkWidget *widget, gpointer data);
void		menu_help_legal		(GtkWidget *widget, gpointer data);
void		menu_help_usage		(GtkWidget *widget, gpointer data);
void		menu_help_known_bugs	(GtkWidget *widget, gpointer data);
void		menu_help_feedback	(GtkWidget *widget, gpointer data);
void		menu_help_about		(GtkWidget *widget, gpointer data);
void		menu_send_gimp		(GtkWidget *widget, gpointer data);
GtkWidget*	get_main_menu		(GtkWidget *window);

void		menu_set_list_type	(ImageListType type);

#ifdef __cplusplus
	}
#endif /* __cplusplus */

#endif /* __GTKSEEMENU_H__ */

