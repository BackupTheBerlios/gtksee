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
 
#ifndef __DNDVIEWPORT_H__
#define __DNDVIEWPORT_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#define DND_VIEWPORT(obj)		GTK_CHECK_CAST (obj, dnd_viewport_get_type (), DndViewport)
#define DND_VIEWPORT_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, dnd_viewport_get_type (), DndViewportClass)
#define IS_DND_VIEWPORT(obj)		GTK_CHECK_TYPE (obj, dnd_viewport_get_type ())

typedef struct _DndViewport		DndViewport;
typedef struct _DndViewportClass	DndViewportClass;

struct _DndViewportClass
{
	GtkViewportClass parent_class;
	void (*parent_realize) (GtkWidget *widget);
	void (*popup) (GtkWidget *widget);
	void (*double_clicked) (GtkWidget *widget);
};

struct _DndViewport
{
        GtkViewport viewport;
	gboolean dragging;
	gint old_x;
	gint old_y;
	gint move_x;
	gint move_y;
};

guint		dnd_viewport_get_type	();
GtkWidget*	dnd_viewport_new	();
void		dnd_viewport_move	(DndViewport *viewport, gint x, gint y);
void		dnd_viewport_reset	(DndViewport *viewport);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
