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

#include "config.h"

#define SCROLLBAR_HEIGHT 30

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/types.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#include <sys/stat.h>

#include "gtypes.h"
#include "util.h"
#include "detect.h"
#include "rc.h"

#include "imagelist.h"
#include "imagetnitem.h"
#include "imagetnlist.h"

#define SUFFIX(str, suf) (strstr(str,suf)>str)

enum
{
   SELECT_IMAGE_SIGNAL,
   MAX_SIGNALS
};

static gint image_tnlist_signals[MAX_SIGNALS];

static void image_tnlist_class_init    (ImageTnListClass *class);
static void image_tnlist_init          (ImageTnList *il);
void        image_tnlist_selected      (GtkWidget *widget, GdkEvent *event,
                                          ImageTnList *il);
GtkWidget*  image_tnlist_create_item   (ImageTnList *il, ImageInfo *info);
GtkWidget*  image_tnlist_get_table     (ImageTnList *il);
void        menu_file_view             (GtkWidget *widget, gpointer data);
static void image_tnlist_resort        (ImageTnList *il);

static void
image_tnlist_class_init(ImageTnListClass *class)
{
   GtkObjectClass *object_class;
   object_class = (GtkObjectClass*) class;
   image_tnlist_signals[SELECT_IMAGE_SIGNAL] = gtk_signal_new ("select_image",
      GTK_RUN_FIRST,
      object_class->type,
      GTK_SIGNAL_OFFSET (ImageTnListClass, select_image),
      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
   gtk_object_class_add_signals (object_class, image_tnlist_signals, MAX_SIGNALS);
   class->select_image = NULL;
}

static void
image_tnlist_init(ImageTnList *il)
{
   int t;

   il -> dir [0] = '\0';
   il -> selected_item [0] = '\0';
   il -> selected_widget = NULL;
   t = rc_get_int("image_sort_type");
   il -> sort_type = (t == RC_NOT_EXISTS ? IMAGE_SORT_ASCEND_BY_NAME : t);
   il -> max_width = 1;
   il -> info = NULL;
   il -> lock = FALSE;
   il -> nfiles = 0;
   il -> total_size = 0;
}

void
image_tnlist_selected(GtkWidget *widget, GdkEvent *event, ImageTnList *il)
{
   ImageInfo *info;

   if (event->type == GDK_2BUTTON_PRESS)
   {
      menu_file_view(widget, NULL);
      return;
   }

   if (il->lock || widget == il->selected_widget) return;
   if (il->selected_widget != NULL)
      gtk_widget_set_state(il->selected_widget, GTK_STATE_NORMAL);
   il->selected_widget = widget;

   info = gtk_object_get_user_data(GTK_OBJECT(widget));
   strcpy(il->selected_item, info->name);
   il->info = info;
   gtk_widget_set_state(widget, GTK_STATE_SELECTED);
   gtk_signal_emit(GTK_OBJECT(il), image_tnlist_signals[SELECT_IMAGE_SIGNAL]);
}

guint
image_tnlist_get_type()
{
   static guint il_type = 0;

   if (!il_type)
   {
      GtkTypeInfo il_info =
      {
         "ImageTnList",
         sizeof (ImageTnList),
         sizeof (ImageTnListClass),
         (GtkClassInitFunc) image_tnlist_class_init,
         (GtkObjectInitFunc) image_tnlist_init,
         (GtkArgSetFunc) NULL,
         (GtkArgGetFunc) NULL
      };
      il_type = gtk_type_unique (gtk_hbox_get_type (), &il_info);
   }
   return il_type;
}

GtkWidget*
image_tnlist_new()
{
   ImageTnList *il;
   GtkWidget *scrolled_win;

   il = IMAGE_TNLIST(gtk_type_new(image_tnlist_get_type()));
   scrolled_win = gtk_scrolled_window_new(NULL, NULL);
   gtk_widget_show(GTK_WIDGET(il));
#ifdef GTK_HAVE_FEATURES_1_1_5
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), GTK_WIDGET(il));
#else
   gtk_container_add(GTK_CONTAINER(scrolled_win), GTK_WIDGET(il));
#endif
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
      GTK_POLICY_AUTOMATIC,
      GTK_POLICY_ALWAYS);

   return GTK_WIDGET(scrolled_win);
}

void
image_tnlist_set_width(ImageTnList *il, guint width)
{
   il->max_width = width;
}

void
image_tnlist_set_dir(ImageTnList *il, guchar *dir)
{
   if (il->lock) return;
   strcpy(il -> dir, dir);
   image_tnlist_refresh(il);
}

void
image_tnlist_clear(ImageTnList *il)
{
   GtkWidget *table;
   ImageTnItem *item;
   GList *children;
   ImageInfo *info;

   if (il->lock) return;

   il->lock = TRUE;
   table = image_tnlist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table))
   {
      il->lock = FALSE;
      return;
   }
   children = gtk_container_children(GTK_CONTAINER(table));
   while (children != NULL)
   {
      item = IMAGE_TNITEM(children->data);
      info = gtk_object_get_user_data(GTK_OBJECT(item));
      if (info->loaded && info->cache.buffer != NULL)
         g_free(info->cache.buffer);
      if (info->has_desc) g_free(info->desc);
      g_free(info);
      image_tnitem_destroy(IMAGE_TNITEM(item));
      children = g_list_remove_link(children, children);
   }
   gtk_container_remove(GTK_CONTAINER(il), table);

   il -> selected_item [0] = '\0';
   il -> selected_widget = NULL;
   il -> info = NULL;
   il -> nfiles = 0;
   il -> total_size = 0;

   il->lock = FALSE;
}

void
image_tnlist_refresh(ImageTnList *il)
{
   DIR *dp;
   struct dirent *item;
   guchar fullname[256];
   struct stat st;
   GList *list, *t_pos;
   ImageInfo *info;
   gint type, rows, columns, serial;
   GtkWidget *table, *img;

   if (il->lock || strlen(il->dir) < 1) return;

   image_tnlist_clear(il);

   il->lock = TRUE;
   if ((dp = opendir(il->dir)) == NULL) return;
   list = NULL;

   il -> nfiles = 0;
   il -> total_size = 0;
   while ((item = readdir(dp)) != NULL)
   {
      if (item -> d_ino == 0) continue;
      if (strcmp(item -> d_name, ".") == 0) continue;
      if (strcmp(item -> d_name, "..") == 0) continue;
      strcpy(fullname, il->dir);
      if (fullname[strlen(fullname) - 1] != '/') strcat(fullname,"/");
      strcat(fullname, item -> d_name);
      if (stat(fullname, &st) != 0) continue;
      if ((st.st_mode & S_IFMT) != S_IFREG) continue;
      if (SUFFIX(item -> d_name, ".xpm") ||
          SUFFIX(item -> d_name, ".XPM")) type = XPM;
      else if (SUFFIX(item -> d_name, ".gif") ||
          SUFFIX(item -> d_name, ".GIF")) type = GIF;
      else if (SUFFIX(item -> d_name, ".jpg") ||
          SUFFIX(item -> d_name, ".JPG") ||
          SUFFIX(item -> d_name, ".jpeg") ||
          SUFFIX(item -> d_name, ".JPEG")) type = JPG;
      else if (SUFFIX(item -> d_name, ".bmp") ||
          SUFFIX(item -> d_name, ".BMP")) type = BMP;
      else if (SUFFIX(item -> d_name, ".ico") ||
          SUFFIX(item -> d_name, ".ICO")) type = ICO;
      else if (SUFFIX(item -> d_name, ".pcx") ||
          SUFFIX(item -> d_name, ".PCX")) type = PCX;
      else if (SUFFIX(item -> d_name, ".tif") ||
          SUFFIX(item -> d_name, ".TIF") ||
          SUFFIX(item -> d_name, ".tiff") ||
          SUFFIX(item -> d_name, ".TIFF")) type = TIF;
      else if (SUFFIX(item -> d_name, ".png") ||
          SUFFIX(item -> d_name, ".PNG")) type = PNG;
      else if (SUFFIX(item -> d_name, ".ppm") ||
          SUFFIX(item -> d_name, ".PPM") ||
          SUFFIX(item -> d_name, ".pnm") ||
          SUFFIX(item -> d_name, ".PNM") ||
          SUFFIX(item -> d_name, ".pgm") ||
          SUFFIX(item -> d_name, ".PGM") ||
          SUFFIX(item -> d_name, ".pbm") ||
          SUFFIX(item -> d_name, ".PBM")) type = PNM;
      else if (SUFFIX(item -> d_name, ".psd") ||
          SUFFIX(item -> d_name, ".PSD")) type = PSD;
      else if (SUFFIX(item -> d_name, ".xbm") ||
          SUFFIX(item -> d_name, ".XBM") ||
          SUFFIX(item -> d_name, ".icon") ||
          SUFFIX(item -> d_name, ".ICON") ||
          SUFFIX(item -> d_name, ".bitmap") ||
          SUFFIX(item -> d_name, ".BITMAP")) type = XBM;
      else if (SUFFIX(item -> d_name, ".xcf") ||
          SUFFIX(item -> d_name, ".XCF")) type = XCF;
      else type = UNKNOWN;
      info = g_malloc(sizeof(ImageInfo));
      info -> type = type;
      strcpy(info -> name, item -> d_name);
      info -> size = st.st_size;
      info -> time = st.st_mtime;
      info -> width = -1;
      info -> height = -1;
      info -> has_desc = FALSE;
      info -> ncolors = -1;
      info -> real_ncolors = TRUE;
      info -> alpha = 0;
      info -> bpp = 0;
      info -> loaded = FALSE;
      info -> cache.buffer = NULL;
      info -> valid = TRUE;
      /*
       * We should detect image type and setup header imformations
       */
      detect_image_type(fullname, info);
      if (info->width < 0 || info->type == UNKNOWN)
      {
         /* Do not show this image in the list */
         g_free(info);
         continue;
      }
      info -> type_pixmap = image_type_get_pixmap(info->type);
      info -> type_mask = image_type_get_mask(info->type);
      list = g_list_insert_sorted(list, info, image_list_get_sort_func(il->sort_type));
      il -> nfiles ++;
      il -> total_size += info -> size;
   }
   g_free(item);
   closedir(dp);

   columns = max(1, (il->max_width-SCROLLBAR_HEIGHT) / (THUMBNAIL_WIDTH+4));
   if (il->nfiles == 0)
      rows = 0;
   else
      rows = (il->nfiles - 1) / columns + 1;
   table = gtk_table_new(rows, columns, TRUE);
   gtk_widget_show(table);
   gtk_container_add(GTK_CONTAINER(il), table);

   serial = 0;
   t_pos = g_list_first(list);
   while (t_pos != NULL)
   {
      info = t_pos -> data;
      info->serial = serial;
      img = image_tnlist_create_item(il, info);
      image_set_tooltips(img, info);
      gtk_table_attach(
         GTK_TABLE(table), img,
         serial % columns, serial % columns + 1,
         serial / columns, serial / columns + 1,
         0, 0, 2, 2);
      t_pos->data = img;
      t_pos = t_pos -> next;
      serial++;
   }

   t_pos = g_list_first(list);
   while (t_pos != NULL)
   {
      while (gtk_events_pending()) gtk_main_iteration();
      img = GTK_WIDGET(t_pos->data);
      info = gtk_object_get_user_data(GTK_OBJECT(img));
      strcpy(fullname, il->dir);
      if (fullname[strlen(fullname) - 1] != '/') strcat(fullname,"/");
      strcat(fullname, info->name);
      image_tnitem_set_image(IMAGE_TNITEM(img), fullname);
      t_pos = g_list_remove_link(t_pos, t_pos);
   }

   il->lock = FALSE;
}

void
image_tnlist_update_info(ImageTnList *il, ImageInfo *info)
{
   /* nothing to do */
}

ImageInfo*
image_tnlist_get_first(ImageTnList *il)
{
   return image_tnlist_get_by_serial(il, 0);
}

ImageInfo*
image_tnlist_get_last(ImageTnList *il)
{
   GtkWidget *table;
   ImageTnItem *item;
   GList *children;
   ImageInfo *info;

   table = image_tnlist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table)) return NULL;
   children = gtk_container_children(GTK_CONTAINER(table));
   item = IMAGE_TNITEM(children->data);
   if (item == NULL) return NULL;
   info = gtk_object_get_user_data(GTK_OBJECT(item));
   if (info != NULL) return info;
   return NULL;
}

ImageInfo*
image_tnlist_get_previous(ImageTnList *il, ImageInfo *info)
{
   return image_tnlist_get_by_serial(il, info->serial - 1);
}

ImageInfo*
image_tnlist_get_next(ImageTnList *il, ImageInfo *info)
{
   return image_tnlist_get_by_serial(il, info->serial + 1);
}

GtkWidget*
image_tnlist_create_item(ImageTnList *il, ImageInfo *info)
{
   GtkWidget *item;
   GdkColor color;

   item = image_tnitem_new(info->name);
   if (info->type < UNKNOWN)
   {
      image_type_get_color(info->type, &color);
      image_tnitem_set_color(IMAGE_TNITEM(item), &color);
   }
   gtk_signal_connect(
      GTK_OBJECT(item),
      "button_press_event",
      GTK_SIGNAL_FUNC(image_tnlist_selected),
      il);
   gtk_widget_show(item);

   gtk_object_set_user_data(GTK_OBJECT(item), info);

   return item;
}

void
image_tnlist_select_by_serial(ImageTnList *il, gint serial)
{
   GtkWidget *table;
   ImageTnItem *item;
   GList *children;
   ImageInfo *info;

   table = image_tnlist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table)) return;
   children = gtk_container_children(GTK_CONTAINER(table));
   info = NULL;
   while (children != NULL)
   {
      item = IMAGE_TNITEM(children->data);
      if (item == NULL) break;
      info = gtk_object_get_user_data(GTK_OBJECT(item));
      if (info != NULL && info->serial == serial)
      {
         image_tnlist_selected(GTK_WIDGET(item), NULL, il);
         break;
      }
      children = children->next;
   }
}

ImageInfo*
image_tnlist_get_by_serial(ImageTnList *il, gint serial)
{
   GtkWidget *table;
   ImageTnItem *item;
   GList *children;
   ImageInfo *info;

   table = image_tnlist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table)) return NULL;
   children = gtk_container_children(GTK_CONTAINER(table));
   info = NULL;
   while (children != NULL)
   {
      item = IMAGE_TNITEM(children->data);
      if (item == NULL) return NULL;
      info = gtk_object_get_user_data(GTK_OBJECT(item));
      if (info != NULL && info->serial == serial) return info;
      children = children->next;
   }
   return NULL;
}

void
image_tnlist_remove_cache(ImageTnList *il)
{
   GtkWidget *table;
   ImageTnItem *item;
   GList *children;
   ImageInfo *info;

   table = image_tnlist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table)) return;
   children = gtk_container_children(GTK_CONTAINER(table));
   while (children != NULL)
   {
      item = IMAGE_TNITEM(children->data);
      if (item == NULL) return;
      info = gtk_object_get_user_data(GTK_OBJECT(item));
      if (info->loaded && info->cache.buffer != NULL)
         g_free(info->cache.buffer);
      info->loaded = FALSE;
      children = children->next;
   }
}

GtkWidget*
image_tnlist_get_table(ImageTnList *il)
{
   GList *list;

   list = gtk_container_children(GTK_CONTAINER(il));
   if (list == NULL || list->data == NULL) return NULL;
   return GTK_WIDGET(list->data);
}

void
image_tnlist_set_sort_type(ImageTnList *il, ImageSortType type)
{
   il->sort_type = type;
   image_tnlist_resort(il);
}

static void
image_tnlist_resort(ImageTnList *il)
{
   image_tnlist_refresh(il);
}
