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

#define SIITEM_HEIGHT 18
#define SCROLLBAR_HEIGHT 25

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
#include <gtk/gtk.h>

#include "gtypes.h"
#include "util.h"
#include "detect.h"
#include "rc.h"

#include "imagelist.h"
#include "imagesiitem.h"
#include "imagesilist.h"

#define SUFFIX(str, suf) (strstr(str,suf)>str)

enum
{
   SELECT_IMAGE_SIGNAL,
   MAX_SIGNALS
};

static gint image_silist_signals[MAX_SIGNALS];

static void image_silist_class_init    (ImageSiListClass *klass);
static void image_silist_init          (ImageSiList *il);
void        image_silist_selected      (GtkWidget *widget, GdkEvent *event,
                                          ImageSiList *il);
GtkWidget*  image_silist_create_item   (ImageSiList *il, ImageInfo *info);
GtkWidget*  image_silist_get_child     (ImageSiList *il, gint serial);
GtkWidget*  image_silist_get_table     (ImageSiList *il);
void        menu_file_view             (GtkWidget *widget, gpointer data);
static void image_silist_resort        (ImageSiList *il);

static void
image_silist_class_init(ImageSiListClass *klass)
{
   GtkObjectClass *object_class;

   object_class = (GtkObjectClass*) klass;
   image_silist_signals[SELECT_IMAGE_SIGNAL] = gtk_signal_new ("select_image",
      GTK_RUN_FIRST,
      object_class->type,
      GTK_SIGNAL_OFFSET (ImageSiListClass, select_image),
      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
   gtk_object_class_add_signals (object_class, image_silist_signals, MAX_SIGNALS);
   klass->select_image = NULL;
}

static void
image_silist_init(ImageSiList *il)
{
   int t;

   il -> dir [0] = '\0';
   il -> selected_item [0] = '\0';
   il -> selected_widget = NULL;
   t = rc_get_int("image_sort_type");
   il -> sort_type = (t == RC_NOT_EXISTS ? IMAGE_SORT_ASCEND_BY_NAME : t);
   il -> max_height = 1;
   il -> info = NULL;
   il -> nfiles = 0;
   il -> total_size = 0;
}

void
image_silist_selected(GtkWidget *widget, GdkEvent *event, ImageSiList *il)
{
   ImageInfo *info;

   if (event->type == GDK_2BUTTON_PRESS)
   {
      menu_file_view(widget, NULL);
      return;
   }

   if (widget == il->selected_widget) return;
   if (il->selected_widget != NULL)
      gtk_widget_set_state(il->selected_widget, GTK_STATE_NORMAL);
   il->selected_widget = widget;

   info = gtk_object_get_user_data(GTK_OBJECT(widget));
   strcpy(il->selected_item, info->name);
   il->info = info;
   gtk_widget_set_state(widget, GTK_STATE_SELECTED);
   gtk_signal_emit(GTK_OBJECT(il), image_silist_signals[SELECT_IMAGE_SIGNAL]);
}

guint
image_silist_get_type()
{
   static guint il_type = 0;

   if (!il_type)
   {
      GtkTypeInfo il_info =
      {
         "ImageSiList",
         sizeof (ImageSiList),
         sizeof (ImageSiListClass),
         (GtkClassInitFunc) image_silist_class_init,
         (GtkObjectInitFunc) image_silist_init,
         (GtkArgSetFunc) NULL,
         (GtkArgGetFunc) NULL
      };
      il_type = gtk_type_unique (gtk_hbox_get_type (), &il_info);
   }
   return il_type;
}

GtkWidget*
image_silist_new()
{
   ImageSiList *il;
   GtkWidget *scrolled_win;

   il = IMAGE_SILIST(gtk_type_new(image_silist_get_type()));
   scrolled_win = gtk_scrolled_window_new(NULL, NULL);
   gtk_widget_show(GTK_WIDGET(il));
#ifdef GTK_HAVE_FEATURES_1_1_5
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), GTK_WIDGET(il));
#else
   gtk_container_add(GTK_CONTAINER(scrolled_win), GTK_WIDGET(il));
#endif
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
      GTK_POLICY_ALWAYS,
#ifdef GTK_HAVE_FEATURES_1_1_6
      GTK_POLICY_NEVER);
#else
      GTK_POLICY_AUTOMATIC);
#endif

   return GTK_WIDGET(scrolled_win);
}

void
image_silist_set_height(ImageSiList *il, guint height)
{
   il->max_height = height;
}

void
image_silist_set_dir(ImageSiList *il, guchar *dir)
{
   strcpy(il -> dir, dir);
   image_silist_refresh(il);
}

void
image_silist_clear(ImageSiList *il)
{
   GtkWidget *table;
   GtkWidget *item;
   GList *children;
   ImageInfo *info;

   table = image_silist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table)) return;
   children = gtk_container_children(GTK_CONTAINER(table));
   while (children != NULL)
   {
      item = GTK_WIDGET(children->data);
      info = gtk_object_get_user_data(GTK_OBJECT(item));
      if (info->loaded && info->cache.buffer != NULL)
         g_free(info->cache.buffer);
      if (info->has_desc) g_free(info->desc);
      g_free(info);
      children = g_list_remove_link(children, children);
   }
   gtk_container_remove(GTK_CONTAINER(il), table);

   il -> selected_item [0] = '\0';
   il -> selected_widget = NULL;
   il -> info = NULL;
   il -> nfiles = 0;
   il -> total_size = 0;
}

void
image_silist_refresh(ImageSiList *il)
{
   DIR *dp;
   struct dirent *item;
   gchar fullname[256];
   struct stat st;
   GList *list, *t_pos;
   ImageInfo *info;
   gint type, rows, columns, serial;
   GtkWidget *table, *img;
   gboolean show_hidden, hide_non_images;

   if (strlen(il->dir) < 1) return;

   image_silist_clear(il);

   if ((dp = opendir(il->dir)) == NULL) return;
   list = NULL;

   il -> nfiles = 0;
   il -> total_size = 0;
   show_hidden = rc_get_boolean("show_hidden");
   hide_non_images = rc_get_boolean("hide_non_images");

   while ((item = readdir(dp)) != NULL)
   {
      if (item -> d_ino == 0) continue;
      if (strcmp(item -> d_name, ".") == 0) continue;
      if (strcmp(item -> d_name, "..") == 0) continue;
      if (!show_hidden && item->d_name[0] == '.') continue;
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
      if (hide_non_images && (info->width < 0 || info->type == UNKNOWN))
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

   rows = max(1, (il->max_height-SCROLLBAR_HEIGHT) / SIITEM_HEIGHT);
   if (il->nfiles == 0)
      columns = 0;
   else
      columns = (il->nfiles - 1) / rows + 1;
   table = gtk_table_new(rows, columns, TRUE);
   gtk_widget_show(table);
   gtk_container_add(GTK_CONTAINER(il), table);

   serial = 0;

   t_pos = g_list_first(list);
   while (t_pos != NULL)
   {
      info = t_pos -> data;
      info->serial = serial;
      img = image_silist_create_item(il, info);
      image_set_tooltips(img, info);
      gtk_table_attach(
         GTK_TABLE(table), img,
         serial / rows, serial / rows + 1,
         serial % rows, serial % rows + 1,
         GTK_FILL, GTK_FILL, 1, 1);
      t_pos = g_list_remove_link(t_pos, t_pos);
      serial++;
   }
}

void
image_silist_update_info(ImageSiList *il, ImageInfo *info)
{
   GdkColor color;

   GtkWidget *child = image_silist_get_child(il, info->serial);
   if (child == NULL || !IS_IMAGE_SIITEM(child)) return;
   if (info->type < UNKNOWN)
   {
      image_type_get_color(info->type, &color);
      image_siitem_set_color(IMAGE_SIITEM(child), &color);
      image_set_tooltips(child, info);
   }
   image_siitem_set_pixmap(IMAGE_SIITEM(child), info->type_pixmap, info->type_mask);
}

ImageInfo*
image_silist_get_first(ImageSiList *il)
{
   gint i;
   ImageInfo *info;

   info = NULL;
   for (i = 0; i < il->nfiles; i++)
   {
      info = image_silist_get_by_serial(il, i);
      if (info == NULL) break;
      if (info->valid && info->width>0) return info;
   }
   return NULL;
}

ImageInfo*
image_silist_get_last(ImageSiList *il)
{
   gint i;
   ImageInfo *info;

   info = NULL;
   for (i = il->nfiles - 1; i >= 0; i--)
   {
      info = image_silist_get_by_serial(il, i);
      if (info == NULL) break;
      if (info->valid && info->width>0) return info;
   }
   return NULL;
}

ImageInfo*
image_silist_get_previous(ImageSiList *il, ImageInfo *info)
{
   gint i;
   ImageInfo *tinfo;

   tinfo = NULL;
   for (i = info->serial-1; i >= 0; i--)
   {
      tinfo = image_silist_get_by_serial(il, i);
      if (tinfo == NULL) break;
      if (tinfo->valid && tinfo->width>0) return tinfo;
   }
   return NULL;
}

ImageInfo*
image_silist_get_next(ImageSiList *il, ImageInfo *info)
{
   gint i;
   ImageInfo *tinfo;

   tinfo = NULL;
   for (i = info->serial+1; i < il->nfiles; i++)
   {
      tinfo = image_silist_get_by_serial(il, i);
      if (tinfo == NULL) break;
      if (tinfo->valid && tinfo->width>0) return tinfo;
   }
   return NULL;
}

GtkWidget*
image_silist_create_item(ImageSiList *il, ImageInfo *info)
{
   GtkWidget *item;
   GdkColor color;

   if (info->valid && info->width>0)
   {
      item = image_siitem_new(
         image_type_get_pixmap(info->type),
         image_type_get_mask(info->type),
         info->name);
      image_type_get_color(info->type, &color);
      image_siitem_set_color(IMAGE_SIITEM(item), &color);
   } else
   {
      item = image_siitem_new(
         image_type_get_pixmap(UNKNOWN),
         image_type_get_mask(UNKNOWN),
         info->name);
   }
   gtk_signal_connect(
      GTK_OBJECT(item),
      "button_press_event",
      GTK_SIGNAL_FUNC(image_silist_selected),
      il);
   gtk_widget_show(item);

   gtk_object_set_user_data(GTK_OBJECT(item), info);

   return item;
}

void
image_silist_select_by_serial(ImageSiList *il, gint serial)
{
   GtkWidget *table;
   GtkWidget *item;
   GList *children;
   ImageInfo *info;

   table = image_silist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table)) return;
   children = gtk_container_children(GTK_CONTAINER(table));
   info = NULL;
   while (children != NULL)
   {
      item = GTK_WIDGET(children->data);
      if (item == NULL) break;
      info = gtk_object_get_user_data(GTK_OBJECT(item));
      if (info != NULL && info->serial == serial)
      {
         image_silist_selected(item, NULL, il);
         break;
      }
      children = children->next;
   }
}

ImageInfo*
image_silist_get_by_serial(ImageSiList *il, gint serial)
{
   GtkWidget *child;

   child = image_silist_get_child(il, serial);
   if (child == NULL) return NULL;
   return (ImageInfo*)gtk_object_get_user_data(GTK_OBJECT(child));
}

GtkWidget*
image_silist_get_child(ImageSiList *il, gint serial)
{
   GtkWidget *table;
   GtkWidget *item;
   GList *children;
   ImageInfo *info;

   table = image_silist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table)) return NULL;
   children = gtk_container_children(GTK_CONTAINER(table));
   while (children != NULL)
   {
      item = GTK_WIDGET(children->data);
      if (item == NULL) return NULL;
      info = gtk_object_get_user_data(GTK_OBJECT(item));
      if (info != NULL && info->serial == serial) return item;
      children = children->next;
   }
   return NULL;
}

void
image_silist_remove_cache(ImageSiList *il)
{
   GtkWidget *table;
   ImageSiItem *item;
   GList *children;
   ImageInfo *info;

   table = image_silist_get_table(il);
   if (table == NULL || !GTK_IS_TABLE(table)) return;
   children = gtk_container_children(GTK_CONTAINER(table));
   while (children != NULL)
   {
      item = IMAGE_SIITEM(children->data);
      if (item == NULL) return;
      info = gtk_object_get_user_data(GTK_OBJECT(item));
      if (info->loaded && info->cache.buffer != NULL)
         g_free(info->cache.buffer);
      info->loaded = FALSE;
      children = children->next;
   }
}

GtkWidget*
image_silist_get_table(ImageSiList *il)
{
   GList *list;

   list = gtk_container_children(GTK_CONTAINER(il));
   if (list == NULL || list->data == NULL) return NULL;
   return GTK_WIDGET(list->data);
}

void
image_silist_set_sort_type(ImageSiList *il, ImageSortType type)
{
   il->sort_type = type;
   image_silist_resort(il);
}

static void
image_silist_resort(ImageSiList *il)
{
   image_silist_refresh(il);
}
