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

#ifndef __IMAGELIST_H__
#define __IMAGELIST_H__

#ifdef __cplusplus
        extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>

#include "gtypes.h"

#define IMAGE_LIST(obj)          GTK_CHECK_CAST (obj, image_list_get_type (), ImageList)
#define IMAGE_LIST_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, image_list_get_type (), ImageListClass)
#define IS_IMAGE_LIST(obj)       GTK_CHECK_TYPE (obj, image_list_get_type ())

typedef struct _ImageList        ImageList;
typedef struct _ImageListClass   ImageListClass;

typedef enum
{
   IS_CLIST,
   IS_THUMB,
   IS_SILIST
} TypeList;

struct _ImageListClass
{
   GtkHBoxClass            parent_class;
   void (*select_image)    (ImageList *il);
   void (*unselect_image)  (ImageList *il);
};

struct _ImageList
{
   GtkHBox        hbox;
   GtkWidget      *parent;
   guchar         dir[256];
   ImageListType  list_type;
   GtkWidget      *child_list;
   GtkWidget      *child_scrolled_win;

   /* the two field should be set when a dir is selected */
   gint           nfiles;
   glong          total_size;
};

guint       image_list_get_type     ();
GtkWidget*  image_list_new          (GtkWidget *parent);
GList*      image_list_get_selection(ImageList *il);
GdkPixmap*  image_type_get_pixmap   (ImageType type);
GdkBitmap*  image_type_get_mask     (ImageType type);
GdkColor    image_type_get_color    (ImageType type);

ImageInfo*  image_list_get_by_serial(ImageList *il, guint serial);
ImageInfo*  image_list_get_selected (ImageList *il);
ImageInfo*  image_list_get_first    (ImageList *il);
ImageInfo*  image_list_get_last     (ImageList *il);
ImageInfo*  image_list_get_next     (ImageList *il, ImageInfo *info);
ImageInfo*  image_list_get_previous (ImageList *il, ImageInfo *info);

guchar*  image_list_get_dir      (ImageList *il);
guchar*  image_type_get_type     (ImageType type);

void  image_list_load_pixmaps    (GtkWidget *parent);
void  image_list_set_dir         (ImageList *il, guchar *dir);
void  image_list_clear           (ImageList *il);
void  image_list_refresh         (ImageList *il);
void  image_list_update_info     (ImageList *il, ImageInfo *info);
void  image_convert_info         (ImageInfo *info, guchar *buffer);
void  image_list_select_by_serial(ImageList *il, gint serial);
void  image_list_select_all      (ImageList *il);
void  image_list_set_list_type   (ImageList *il, ImageListType type);
void  image_list_set_sort_type   (ImageList *il, ImageSortType type);
void  image_list_remove_cache    (ImageList *il);
void  image_set_tooltips         (GtkWidget *widget, ImageInfo *info);

gint  image_cmp_default             (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_ascend_by_name      (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_descend_by_name     (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_ascend_by_size      (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_descend_by_size     (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_ascend_by_property  (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_descend_by_property (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_ascend_by_date      (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_descend_by_date     (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_ascend_by_type      (ImageInfo *i1, ImageInfo *i2);
gint  image_cmp_descend_by_type     (ImageInfo *i1, ImageInfo *i2);

GList* image_get_load_file(guchar *dir, gint *nfiles, glong *total_size,
                              ImageSortType sort_type, TypeList type_list);

GCompareFunc image_list_get_sort_func   (ImageSortType type);

#ifdef __cplusplus
        }
#endif /* __cplusplus */

#endif
