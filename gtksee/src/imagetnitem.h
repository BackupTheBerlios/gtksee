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
 
#ifndef __IMAGETNITEM_H__
#define __IMAGETNITEM_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#define IMAGE_TNITEM(obj)		GTK_CHECK_CAST (obj, image_tnitem_get_type (), ImageTnItem)
#define IMAGE_TNITEM_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, image_tnitem_get_type (), ImageTnItemClass)
#define IS_IMAGE_TNITEM(obj)		GTK_CHECK_TYPE (obj, image_tnitem_get_type ())

typedef struct _ImageTnItem		ImageTnItem;
typedef struct _ImageTnItemClass	ImageTnItemClass;

struct _ImageTnItemClass
{
        GtkWidgetClass parent_class;
};

struct _ImageTnItem
{
        GtkWidget widget;
	
	guchar label[80];
	guchar filename[256];
	GtkPreview *image;
	gboolean color_set;
	GdkGC *bg_gc;
	GdkColor color;
};

guint		image_tnitem_get_type		();
GtkWidget*	image_tnitem_new		(guchar *label);
void		image_tnitem_set_color		(ImageTnItem *ii, GdkColor *color);
void		image_tnitem_set_image		(ImageTnItem *ii, guchar *filename);
void		image_tnitem_destroy		(ImageTnItem *ii);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
