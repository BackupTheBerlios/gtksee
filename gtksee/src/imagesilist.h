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
 
#ifndef __IMAGESILIST_H__
#define __IMAGESILIST_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#include "gtypes.h"

#define IMAGE_SILIST(obj)		GTK_CHECK_CAST (obj, image_silist_get_type (), ImageSiList)
#define IMAGE_SILIST_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, image_silist_get_type (), ImageSiListClass)
#define IS_IMAGE_SILIST(obj)		GTK_CHECK_TYPE (obj, image_silist_get_type ())

typedef struct _ImageSiList		ImageSiList;
typedef struct _ImageSiListClass	ImageSiListClass;

struct _ImageSiListClass
{
        GtkHBoxClass parent_class;
	void (*select_image) (ImageSiList *il);
};

struct _ImageSiList
{
        GtkHBox hbox;
	guint max_height;
        guchar dir[256];
	guchar selected_item[64];
	ImageSortType sort_type;
	GtkWidget *selected_widget;
	ImageInfo *info;
	
	/* the two field should be set when a dir is selected */
	gint nfiles;
	glong total_size;
};

guint		image_silist_get_type		();
GtkWidget*	image_silist_new		();
void		image_silist_set_height		(ImageSiList *il, guint height);
void		image_silist_set_dir		(ImageSiList *il, guchar *dir);
void		image_silist_clear		(ImageSiList *il);
void		image_silist_refresh		(ImageSiList *il);
void		image_silist_update_info	(ImageSiList *il, ImageInfo *info);
ImageInfo*	image_silist_get_first		(ImageSiList *il);
ImageInfo*	image_silist_get_last		(ImageSiList *il);
ImageInfo*	image_silist_get_next		(ImageSiList *il, ImageInfo *info);
ImageInfo*	image_silist_get_previous	(ImageSiList *il, ImageInfo *info);
void		image_silist_select_by_serial	(ImageSiList *il, gint serial);
ImageInfo*	image_silist_get_by_serial	(ImageSiList *il, gint serial);
void		image_silist_remove_cache	(ImageSiList *il);
void		image_silist_set_sort_type	(ImageSiList *il,
						 ImageSortType type);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
