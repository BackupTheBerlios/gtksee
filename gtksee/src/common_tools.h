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
 
#ifndef __COMMON_TOOLS_H__
#define __COMMON_TOOLS_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#define FONT_BIG 0
#define FONT_SMALL 1

typedef	gboolean	(*GetFileFunc)		(guchar *buffer);
typedef	void		(*FinishCallbackFunc)	();

typedef struct
{
	/* a widget(dialog) which should be closed */
	GtkWidget *widget;
	/* function: get the file name */
	GetFileFunc file_func;
	/* function: called when finished */
	FinishCallbackFunc fin_func;
	/* additional parameters... */
	GtkWidget *entry;
} tool_parameters;

void		remove_file	(GtkWidget *widget, tool_parameters *param);
void		rename_file	(GtkWidget *widget, tool_parameters *param);
GtkWidget*	info_dialog_new	(guchar *title, guint *sizes, guchar **text);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
