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
 
#ifndef __IMAGETNLIST_H__
#define __IMAGETNLIST_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#include "gtypes.h"

#define IMAGE_TNLIST(obj)		GTK_CHECK_CAST (obj, image_tnlist_get_type (), ImageTnList)
#define IMAGE_TNLIST_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, image_tnlist_get_type (), ImageTnListClass)
#define IS_IMAGE_TNLIST(obj)		GTK_CHECK_TYPE (obj, image_tnlist_get_type ())

typedef struct _ImageTnList		ImageTnList;
typedef struct _ImageTnListClass	ImageTnListClass;

struct _ImageTnListClass
{
        GtkHBoxClass parent_class;
	void (*select_image) (ImageTnList *il);
};

struct _ImageTnList
{
        GtkHBox hbox;
	guint max_width;
        guchar dir[256];
	guchar selected_item[64];
	ImageSortType sort_type;
	GtkWidget *selected_widget;
	ImageInfo *info;
	gboolean lock;
	
	/* the two field should be set when a dir is selected */
	gint nfiles;
	glong total_size;
};

guint		image_tnlist_get_type		();
GtkWidget*	image_tnlist_new		();
void		image_tnlist_set_width		(ImageTnList *il, guint width);
void		image_tnlist_set_dir		(ImageTnList *il, guchar *dir);
void		image_tnlist_clear		(ImageTnList *il);
void		image_tnlist_refresh		(ImageTnList *il);
void		image_tnlist_update_info	(ImageTnList *il, ImageInfo *info);
ImageInfo*	image_tnlist_get_first		(ImageTnList *il);
ImageInfo*	image_tnlist_get_last		(ImageTnList *il);
ImageInfo*	image_tnlist_get_next		(ImageTnList *il, ImageInfo *info);
ImageInfo*	image_tnlist_get_previous	(ImageTnList *il, ImageInfo *info);
void		image_tnlist_select_by_serial	(ImageTnList *il, gint serial);
ImageInfo*	image_tnlist_get_by_serial	(ImageTnList *il, gint serial);
void		image_tnlist_remove_cache	(ImageTnList *il);
void		image_tnlist_set_sort_type	(ImageTnList *il,
						 ImageSortType type);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
