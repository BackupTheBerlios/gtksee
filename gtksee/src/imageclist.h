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

#ifndef __IMAGECLIST_H__
#define __IMAGECLIST_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#include "gtypes.h"

#define IMAGE_CLIST(obj)      GTK_CHECK_CAST (obj, image_clist_get_type (), ImageCList)
#define IMAGE_CLIST_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, image_clist_get_type (), ImageCListClass)
#define IS_IMAGE_CLIST(obj)      GTK_CHECK_TYPE (obj, image_clist_get_type ())

/* Maximum titles */
#define MAX_TITLES 4

typedef struct _ImageCList ImageCList;
typedef struct _ImageCListClass  ImageCListClass;

struct _ImageCListClass
{
   GtkCListClass parent_class;
};

struct _ImageCList
{
   GtkCList       clist;
   guchar         *titles[MAX_TITLES];
   guchar         dir[256];
   guchar         selected_item[64];
   ImageSortType  sort_type;
   ImageInfo      *info;

   /* the two field should be set when a dir is selected */
   gint nfiles;
   glong total_size;
};

guint       image_clist_get_type     ();
GtkWidget*  image_clist_new          ();
void        image_clist_set_dir      (ImageCList *il, guchar *dir);
void        image_clist_clear        (ImageCList *il);
void        image_clist_refresh      (ImageCList *il);
void        image_clist_update_info  (ImageCList *il, ImageInfo *info);
ImageInfo*  image_clist_get_first    (ImageCList *il);
ImageInfo*  image_clist_get_last     (ImageCList *il);
ImageInfo*  image_clist_get_next     (ImageCList *il, ImageInfo *info);
ImageInfo*  image_clist_get_previous (ImageCList *il, ImageInfo *info);
void        image_clist_remove_cache (ImageCList *il);
void        image_clist_set_sort_type(ImageCList *il, ImageSortType type);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
