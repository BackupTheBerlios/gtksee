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
 
#ifndef __IMAGESIITEM_H__
#define __IMAGESIITEM_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#define IMAGE_SIITEM(obj)		GTK_CHECK_CAST (obj, image_siitem_get_type (), ImageSiItem)
#define IMAGE_SIITEM_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, image_siitem_get_type (), ImageSiItemClass)
#define IS_IMAGE_SIITEM(obj)		GTK_CHECK_TYPE (obj, image_siitem_get_type ())

typedef struct _ImageSiItem		ImageSiItem;
typedef struct _ImageSiItemClass	ImageSiItemClass;

struct _ImageSiItemClass
{
        GtkWidgetClass parent_class;
};

struct _ImageSiItem
{
        GtkWidget widget;
	
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	guchar label[80];
	gboolean color_set;
	GdkGC *bg_gc;
	GdkColor color;
};

guint		image_siitem_get_type	();
GtkWidget*	image_siitem_new	(GdkPixmap *pixmap,
					 GdkBitmap *mask,
					 guchar *label);
void		image_siitem_set_color	(ImageSiItem *ii,
					 GdkColor *color);
void		image_siitem_set_pixmap	(ImageSiItem *ii,
					 GdkPixmap *pixmap,
					 GdkBitmap *mask);
void		image_siitem_destroy	(ImageSiItem *ii);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
