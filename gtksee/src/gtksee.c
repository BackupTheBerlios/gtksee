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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "common_tools.h"
#include "intl.h"
#include "timestamp.h"
#include "util.h"
#include "gtypes.h"
#include "detect.h"
#include "scanline.h"
#include "dirtree.h"
#include "imagelist.h"
#include "rc.h"
#include "gtkseemenu.h"
#include "gtkseestatus.h"
#include "gtkseeabout.h"
#include "gtkseetoolbar.h"
#include "viewer.h"
#include "gtksee.h"

#include "pixmaps/blank.xpm"
#include "../icons/gtksee.xpm"

#define REFRESH_INTERVAL   3000
#define PREVIEW_WIDTH      (min(gdk_screen_width()-440,420))
#define PREVIEW_HEIGHT     (min(gdk_screen_height()-300,360))

typedef enum
{
   PASTE_MOVING,
   PASTE_COPYING,
   PASTE_LINKING
} PasteType;

GtkWidget *entry;
GtkWidget *imagelist;
GtkWidget *preview_box;
GtkWidget *preview_image;
GtkWidget *tree, *treewin;

static GtkWidget *browser_window;
static GdkCursor *watch_cursor;

static gchar current_selected_dir[256];
static guint old_preview_width, old_preview_height;
static gboolean enable_auto_refresh = FALSE;
static guint auto_refresh_func;
static time_t list_last_modified = 0;
static gchar root[256];
static gint busy_level;

static guint clipboard_size = 0;
static gboolean clipboard_moving = FALSE;
static char **file_clipboard = NULL;

static void set_busy_cursor   ();
static void set_normal_cursor ();

static void dir_selected            (GtkTree *dt, GtkWidget *child);
static void dir_selected_internal   (GtkTree *dt, GtkWidget *child);
static void file_selected           (ImageList *il);
static void file_selected_internal  (ImageList *il);
static void file_unselected         (ImageList *il);
static void dir_entry_enter         (GtkWidget *widget, GtkWidget *entry);
static void refresh_status          ();
static void preview_size_changed    (GtkWidget *box);
static void clear_preview_box       ();
static void auto_refresh_list       ();
static void show_viewer             ();
static gint imagelist_doubleselect  (GtkWidget * widget, GdkEventButton * event);

static void file_clipboard_clear    ();
static void file_clipboard_alloc    (guint num);
static void file_clipboard_set      (guint num, guchar *file);
static void file_paste              (guchar *src_file, guchar *dest_path,
                                       PasteType type);
static void menu_edit_paste_internal(gboolean pastelink);

gboolean
get_current_image(gchar *buffer)
{
   ImageInfo *info;

   if ((info = image_list_get_selected(IMAGE_LIST(imagelist))) == NULL)
   {
      buffer[0] = '\0';
      return FALSE;
   } else
   {
      strncpy(buffer, current_selected_dir, 256);
      if (buffer[strlen(buffer) - 1] != '/')
         strcat(buffer,"/");
      strcat(buffer, info->name);
      return TRUE;
   }
}

void
refresh_list()
{
   struct stat st;

   if (busy_level > 0) return;
   if (stat(image_list_get_dir(IMAGE_LIST(imagelist)), &st) != 0) return;
   list_last_modified = st.st_mtime;

   set_busy_cursor();
   clear_preview_box();
   image_list_refresh(IMAGE_LIST(imagelist));
   refresh_status();
   set_normal_cursor();
}

static void
auto_refresh_list()
{
   struct stat st;

   if (busy_level > 0) return;
   if (stat(image_list_get_dir(IMAGE_LIST(imagelist)), &st) != 0) return;
   if (st.st_mtime == list_last_modified) return;
   refresh_list();
}

static void
clear_preview_box()
{
   if (GTK_WIDGET_REALIZED(preview_image))
   {
      gdk_window_clear_area(
         preview_image->window,
         0, 0,
         preview_image->allocation.width,
         preview_image->allocation.height);
   }
}

static void
preview_size_changed(GtkWidget *box)
{
   guint width, height;

   if (busy_level > 0) return;

   width = box->allocation.width;
   height = box->allocation.height;

   if (old_preview_width == width && old_preview_height == height)
      return;
   old_preview_width = width;
   old_preview_height = height;

   /* remove the cache, because all image should be reload
    * after the size has been changed */
   image_list_remove_cache(IMAGE_LIST(imagelist));
   if (image_list_get_selected(IMAGE_LIST(imagelist)) == NULL) return;

   /* call this to reload the image */
   file_selected_internal(IMAGE_LIST(imagelist));
}

static void
dir_entry_enter(GtkWidget *widget, GtkWidget *entry)
{
   gchar entry_text[256];
   GtkAdjustment *vadj;
   gint pos;
   struct stat st;

   if (busy_level > 0) return;

   strncpy(entry_text, gtk_entry_get_text(GTK_ENTRY(entry)), sizeof(entry_text));
   if (strlen(entry_text) <= 0)
      return;
   if (entry_text[0] != '/')
   {
      strncpy(entry_text, current_selected_dir, sizeof(entry_text));
      if (entry_text[strlen(entry_text) - 1] != '/')
         strcat(entry_text, "/");
      strcat(entry_text, gtk_entry_get_text(GTK_ENTRY(entry)));
   }
   pos = dirtree_select_dir(DIRTREE(tree), entry_text);

   if ((pos < 0) && (stat(entry_text, &st) == 0))
   {  /* for automounting -lc- */
      dirtree_refresh(DIRTREE(tree));
      pos = dirtree_select_dir(DIRTREE(tree), entry_text);
   }

   if (pos >= 0)
   {
      vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(treewin));
      gtk_adjustment_set_value(
         vadj,
         min(vadj->upper - vadj->page_size,
         DIRTREE(tree)->item_height * pos));
   }
}

static void
set_busy_cursor()
{
   if (busy_level == 0)
   {
      gdk_window_set_cursor(browser_window->window, watch_cursor);
      while (gtk_events_pending()) gtk_main_iteration();
      gtk_grab_add(imagelist);
   }
   busy_level++;
}

static void
set_normal_cursor()
{
   busy_level--;
   if (busy_level == 0)
   {
      gdk_window_set_cursor(browser_window->window, NULL);
      gtk_grab_remove(imagelist);
   }
}

static void
dir_selected(GtkTree *dt, GtkWidget *child)
{
   if (busy_level > 0) return;
   dir_selected_internal(dt, child);
}

static void
dir_selected_internal(GtkTree *dt, GtkWidget *child)
{
   struct stat st;

   set_busy_cursor();
   clear_preview_box();

   gtk_entry_set_text(GTK_ENTRY(entry), DIRTREE(dt)->selected_dir);
   strcpy(current_selected_dir, DIRTREE(dt)->selected_dir);
   image_list_set_dir(IMAGE_LIST(imagelist), DIRTREE(dt)->selected_dir);

   refresh_status();
   set_normal_cursor();
   if (stat(image_list_get_dir(IMAGE_LIST(imagelist)), &st) != 0) return;
   list_last_modified = st.st_mtime;
}

static void
file_selected(ImageList *il)
{
   if (busy_level > 0) return;
   file_selected_internal(il);
}

static void
file_selected_internal(ImageList *il)
{
   gchar       *text;
   gchar       fullname[256], buffer[256];
   ImageInfo   *info = image_list_get_selected(il);
   struct tm   *time;
   gint        hour, i;
   gint        max_width, max_height;
   gint        old_width, t;
   gboolean    fast_preview;
   GtkPreview  *preview = GTK_PREVIEW(preview_image);
   ImageCache  *loaded_cache;
   guchar      *ptr;

   if (info == NULL) return;
   clear_preview_box();

   toolbar_remove_enable(TRUE);
   toolbar_rename_enable(TRUE);
   toolbar_timestamp_enable(TRUE);

   max_width   = preview_image->allocation.width - 4;
   max_height  = preview_image->allocation.height - 4;

   /* setting status_file(size, date/time) */
   text = fsize(info->size);
   time = localtime(&info->time);
   if (time->tm_hour <=12)
   {
      hour = time->tm_hour;
   } else
   {
      hour = time->tm_hour - 12;
   }
   if (hour == 0)
   {
      sprintf(buffer, "%s, %04i/%02i/%02i 12:%02i AM",
         text, time->tm_year + 1900, time->tm_mon+1, time->tm_mday,
         time->tm_min);
   } else
   {
      sprintf(buffer, "%s, %04i/%02i/%02i %02i:%02i %s",
         text, time->tm_year + 1900, time->tm_mon+1, time->tm_mday,
         hour, time->tm_min, (time->tm_hour<12)?"AM":"PM");
   }
   g_free(text);
   set_status_file(buffer);

   /* setting status_type(pixmap) */
   set_status_type(info->type_pixmap, info->type_mask);

   /* setting status_name(filename) */
   if (info->has_desc)
   {
      sprintf(buffer, "%s (%s)", info->name, info->desc);
      set_status_name(buffer);
   } else
   {
      set_status_name(info->name);
   }

   /* setting status_prop(image properties) */
   text = g_malloc(30);
   image_convert_info(info, text);
   set_status_prop(text);

   fast_preview = rc_get_boolean("fast_preview");

   set_busy_cursor();
   if (info -> valid)
   {
      /* load the image if necessary */
      if (!info->loaded)
      {
         info->loaded = TRUE;
         strcpy(fullname, image_list_get_dir(il));
         if (fullname[strlen(fullname) - 1] != '/') strcat(fullname,"/");
         strcat(fullname, info->name);
         old_width = info->width;
         load_scaled_image(fullname, info,
            max_width, max_height,
            il->parent,
            preview_image,
            SCANLINE_DISPLAY |
            (fast_preview?SCANLINE_PREVIEW:0));

         loaded_cache = scanline_get_cache();
         info->cache.buffer_width = loaded_cache->buffer_width;
         info->cache.buffer_height = loaded_cache->buffer_height;
         t = loaded_cache->buffer_width * loaded_cache->buffer_height * 3;
         info->cache.buffer = g_malloc(t);
         memcpy(info->cache.buffer, loaded_cache->buffer, t);

         if (info->width != old_width)
         {
            /* we should refresh the image info */
            image_list_update_info(il, info);
            image_convert_info(info, buffer);
            set_status_prop(buffer);
            set_status_type(info->type_pixmap, info->type_mask);
            toolbar_view_enable(image_list_get_first(IMAGE_LIST(imagelist)) != NULL);
         }
         if (info->has_desc && info->desc != NULL)
         {
            sprintf(buffer, "%s (%s)", info->name, info->desc);
            set_status_name(buffer);
            info->has_desc = TRUE;
         }

         if(info->valid == FALSE)
            return;

      } else
      {
         /* The image has been load before. Use cache. */
         gtk_preview_size(preview, info->cache.buffer_width,
            info->cache.buffer_height);
         gtk_widget_queue_resize(preview_image);
         /*
         ptr = info->cache.buffer;
         for (i = 0; i < info->cache.buffer_height; i++)
         {
             gtk_preview_draw_row(preview, ptr,
                0, i, info->cache.buffer_width);
             ptr += info->cache.buffer_width * 3;
         }
         gtk_widget_draw(preview_image, NULL);
         */
      }

      ptr = info->cache.buffer;
      for (i = 0; i < info->cache.buffer_height; i++)
      {
          gtk_preview_draw_row(preview, ptr,
             0, i, info->cache.buffer_width);
          ptr += info->cache.buffer_width * 3;
      }
      gtk_widget_draw(preview_image, NULL);

      set_normal_cursor();
   }
}

static void
file_unselected(ImageList *il)
{
   if (busy_level > 0) return;

   toolbar_remove_enable(FALSE);
   toolbar_rename_enable(FALSE);
   toolbar_timestamp_enable(FALSE);
}

/* This method is called from image viewer */
void
show_browser()
{
   gtk_widget_show(browser_window);
   if (enable_auto_refresh)
   {
      auto_refresh_func = gtk_timeout_add(
         REFRESH_INTERVAL,
         (GtkFunction)auto_refresh_list,
         NULL);
   }
}

void
menu_file_full_view(GtkWidget *widget, gpointer data)
{
   rc_set_boolean("full_screen", TRUE);

   show_viewer();
}

void
menu_file_view(GtkWidget *widget, gpointer data)
{
   viewer_set_slide_show(FALSE);
   show_viewer();
}

static void
show_viewer()
{
   if (busy_level > 0) return;

   if (image_list_get_first(IMAGE_LIST(imagelist)) == NULL) return;
   if (enable_auto_refresh)
   {
      gtk_timeout_remove(auto_refresh_func);
   }
   gtk_widget_hide(browser_window);
   get_viewer_window(imagelist);
}

void
menu_view_refresh(GtkWidget *widget, gpointer data)
{
   if (busy_level > 0) return;
   set_busy_cursor();

   refresh_all();
   set_normal_cursor();
}

void
refresh_all()
{
   gint           serial, pos;
   ImageInfo      *info;
   GtkAdjustment  *vadj;

   info = image_list_get_selected(IMAGE_LIST(imagelist));
   serial = (info==NULL) ? -1 : info->serial;

   dirtree_refresh(DIRTREE(tree));
   pos = dirtree_select_dir(DIRTREE(tree), DIRTREE(tree)->selected_dir);
   if (pos >=0) {
      vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(treewin));
      gtk_adjustment_set_value(
         vadj,
         min(vadj->upper - vadj->page_size,
         DIRTREE(tree)->item_height * pos));
   }
   dir_selected_internal(GTK_TREE(tree), NULL);

   if (serial >= 0)
   {
      image_list_select_by_serial(IMAGE_LIST(imagelist), serial);
      file_selected_internal(IMAGE_LIST(imagelist));
   }
}

void
gtksee_set_root(guchar *new_root)
{
   dirtree_set_root(DIRTREE(tree), new_root);
}

void
menu_file_quit(GtkWidget *widget, gpointer data)
{
   close_gtksee();
}

void
menu_edit_copy(GtkWidget *widget, gpointer data)
{
   GList *selection;
   ImageInfo *info;
   guint counter;
   char buf[256];

   selection = image_list_get_selection(IMAGE_LIST(imagelist));
   if (selection == NULL) return;
   file_clipboard_clear();
   file_clipboard_alloc(g_list_length(selection));
   counter = 0;
   selection = g_list_first(selection);
   while (selection != NULL)
   {
      info = image_list_get_by_serial(IMAGE_LIST(imagelist),
               (guint)selection->data);
      sprintf(buf, "%s/%s", IMAGE_LIST(imagelist)->dir, info->name);
      file_clipboard_set(counter++, buf);
      selection = g_list_next(selection);
   }
   clipboard_moving = FALSE;
}

void
menu_edit_cut(GtkWidget *widget, gpointer data)
{
   /* same as copy; except the flag clipboard_moving */
   menu_edit_copy(widget, data);
   clipboard_moving = TRUE;
}

void
menu_edit_paste(GtkWidget *widget, gpointer data)
{
   menu_edit_paste_internal(FALSE);
}

void
menu_edit_paste_link(GtkWidget *widget, gpointer data)
{
   menu_edit_paste_internal(TRUE);
}

static void
menu_edit_paste_internal(gboolean pastelink)
{
   gint i;
   guchar *dir = IMAGE_LIST(imagelist)->dir;
   guchar *file, buf[256];

   if (file_clipboard == NULL || clipboard_size < 1) return;

   /* cannot paste to the same directory! */
   if (strncmp(file_clipboard[0], dir, strlen(dir)) == 0 &&
       file_clipboard[0][strlen(dir)] == '/' &&
       strchr(file_clipboard[0] + strlen(dir) + 1, '/') == NULL)
   {
      return;
   }

   for (i = 0; i < clipboard_size; i++)
   {
      file_paste(file_clipboard[i], dir,
         clipboard_moving ? PASTE_MOVING :
         (pastelink ? PASTE_LINKING : PASTE_COPYING));
      /*
       * If we are moving files, we should fix the clipboard
       * so that we can paste again; and next time should be
       * copy, not move. clipboard_moving flag should be fixed
       * after the loop.
       */
      if (clipboard_moving)
      {
         file = strrchr(file_clipboard[i], '/') + 1;
         sprintf(buf, "%s/%s", dir, file);
         file_clipboard_set(i, buf);
      }
   }
   clipboard_moving = FALSE;
   refresh_list();
}

void
menu_edit_rename(GtkWidget *widget, gpointer data)
{
   GList *selection;

   selection = image_list_get_selection(IMAGE_LIST(imagelist));
   if (selection == NULL) return;
   rename_file(imagelist, selection);
   file_clipboard_clear();
}

void
menu_edit_timestamp(GtkWidget *widget, gpointer data)
{
   GList *selection;

   selection = image_list_get_selection(IMAGE_LIST(imagelist));
   if (selection == NULL) return;
   timestamp_file(imagelist, selection);
   file_clipboard_clear();
}

void
menu_edit_delete(GtkWidget *widget, gpointer data)
{
   GList *selection;

   selection = image_list_get_selection(IMAGE_LIST(imagelist));
   if (selection == NULL) return;
   remove_file(imagelist, selection);
   file_clipboard_clear();
}

void
menu_edit_select_all(GtkWidget *widget, gpointer data)
{
   image_list_select_all(IMAGE_LIST(imagelist));
}

void
menu_view_show_hidden(GtkWidget *widget, gpointer data)
{
   if (busy_level > 0) return;

   rc_set_boolean("show_hidden", GTK_CHECK_MENU_ITEM(widget)->active);

   menu_view_refresh(widget, data);
}

void
menu_view_hide(GtkWidget *widget, gpointer data)
{
   if (busy_level > 0) return;

   rc_set_boolean("hide_non_images", GTK_CHECK_MENU_ITEM(widget)->active);

   refresh_list();
}

void
menu_view_thumbnails(GtkWidget *widget, gpointer data)
{
   if (GTK_CHECK_MENU_ITEM(widget)->active)
   {
      toolbar_set_list_type(IMAGE_LIST_THUMBNAILS);
   }
}

void
menu_view_small_icons(GtkWidget *widget, gpointer data)
{
   if (GTK_CHECK_MENU_ITEM(widget)->active)
   {
      toolbar_set_list_type(IMAGE_LIST_SMALL_ICONS);
   }
}

void
menu_view_details(GtkWidget *widget, gpointer data)
{
   if (GTK_CHECK_MENU_ITEM(widget)->active)
   {
      toolbar_set_list_type(IMAGE_LIST_DETAILS);
   }
}

void
menu_view_preview(GtkWidget *widget, gpointer data)
{
   rc_set_boolean("fast_preview", GTK_CHECK_MENU_ITEM(widget)->active);

}

void
menu_view_sort_by_name(GtkWidget *widget, gpointer data)
{
   ImageSortType type;

   type = rc_get_int("image_sort_type");
   if (type == RC_NOT_EXISTS)
   {
      type = IMAGE_SORT_ASCEND_BY_NAME;
   } else
   {
      type = type % 2;
   }
   image_list_set_sort_type(IMAGE_LIST(imagelist), type);
}

void
menu_view_sort_by_size(GtkWidget *widget, gpointer data)
{
   ImageSortType type;

   type = rc_get_int("image_sort_type");
   if (type == RC_NOT_EXISTS)
   {
      type = IMAGE_SORT_ASCEND_BY_SIZE;
   } else
   {
      type = type % 2 + 2;
   }
   image_list_set_sort_type(IMAGE_LIST(imagelist), type);
}

void
menu_view_sort_by_property(GtkWidget *widget, gpointer data)
{
   ImageSortType type;

   type = rc_get_int("image_sort_type");
   if (type == RC_NOT_EXISTS)
   {
      type = IMAGE_SORT_ASCEND_BY_PROPERTY;
   } else
   {
      type = type % 2 + 4;
   }
   image_list_set_sort_type(IMAGE_LIST(imagelist), type);
}

void
menu_view_sort_by_date(GtkWidget *widget, gpointer data)
{
   ImageSortType type;

   type = rc_get_int("image_sort_type");
   if (type == RC_NOT_EXISTS)
   {
      type = IMAGE_SORT_ASCEND_BY_DATE;
   } else
   {
      type = type % 2 + 6;
   }
   image_list_set_sort_type(IMAGE_LIST(imagelist), type);
}

void
menu_view_sort_by_type(GtkWidget *widget, gpointer data)
{
   ImageSortType type;

   type = rc_get_int("image_sort_type");
   if (type == RC_NOT_EXISTS)
   {
      type = IMAGE_SORT_ASCEND_BY_TYPE;
   } else
   {
      type = type % 2 + 8;
   }
   image_list_set_sort_type(IMAGE_LIST(imagelist), type);
}

void
menu_view_sort_ascend(GtkWidget *widget, gpointer data)
{
   ImageSortType type;

   type = rc_get_int("image_sort_type");
   if (type == RC_NOT_EXISTS)
   {
      type = IMAGE_SORT_ASCEND_BY_NAME;
   } else
   {
      type = type - type % 2;
   }
   image_list_set_sort_type(IMAGE_LIST(imagelist), type);
}

void
menu_view_sort_descend(GtkWidget *widget, gpointer data)
{
   ImageSortType type;

   type = rc_get_int("image_sort_type");
   if (type == RC_NOT_EXISTS)
   {
      type = IMAGE_SORT_DESCEND_BY_NAME;
   } else
   {
      type = type - type % 2 + 1;
   }
   image_list_set_sort_type(IMAGE_LIST(imagelist), type);
}

void
menu_view_quick_refresh(GtkWidget *widget, gpointer data)
{
   refresh_list();
}

void
menu_view_auto_refresh(GtkWidget *widget, gpointer data)
{
   enable_auto_refresh = GTK_CHECK_MENU_ITEM(widget)->active;
   if (enable_auto_refresh)
   {
      auto_refresh_func = gtk_timeout_add(
         REFRESH_INTERVAL,
         (GtkFunction)auto_refresh_list,
         NULL);
   } else
   {
      gtk_timeout_remove(auto_refresh_func);
   }
}

void
menu_tools_slide_show(GtkWidget *widget, gpointer data)
{
   viewer_set_slide_show(TRUE);
   show_viewer();
}

void
menu_help_contents(GtkWidget *widget, gpointer data)
{
   generate_dialog(CONTENTS);
}

void
menu_help_about(GtkWidget *widget, gpointer data)
{
   generate_dialog(ABOUT);
}

void
toolbar_cdup(GtkWidget *widget, gpointer data)
{
   GtkAdjustment *vadj;
   gint n;

   if (busy_level > 0) return;

   vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(treewin));
   n = dirtree_cdup(DIRTREE(tree));
   if (n >=0) {
      gtk_adjustment_set_value(
         vadj,
         min(vadj->upper - vadj->page_size,
         DIRTREE(tree)->item_height * n));
   }

}

void
toolbar_refresh(GtkWidget *widget, gpointer data)
{
   menu_view_refresh(widget, data);
}

void
toolbar_view(GtkWidget *widget, gpointer data)
{
   show_viewer();
}

void
toolbar_about(GtkWidget *widget, gpointer data)
{
   menu_help_about(widget, data);
}

void
toolbar_thumbnails(GtkWidget *widget, gpointer data)
{
   if (busy_level > 0) return;

   clear_preview_box();
   rc_set_int("image_list_type", IMAGE_LIST_THUMBNAILS);

   if (GTK_TOGGLE_BUTTON(widget)->active)
   {
      set_busy_cursor();
      image_list_set_list_type(IMAGE_LIST(imagelist), IMAGE_LIST_THUMBNAILS);
      refresh_status();
      menu_set_list_type(IMAGE_LIST_THUMBNAILS);
      set_normal_cursor();
   }
}

void
toolbar_small_icons(GtkWidget *widget, gpointer data)
{
   if (busy_level > 0) return;

   clear_preview_box();
   rc_set_int("image_list_type", IMAGE_LIST_SMALL_ICONS);

   if (GTK_TOGGLE_BUTTON(widget)->active)
   {
      set_busy_cursor();
      image_list_set_list_type(IMAGE_LIST(imagelist), IMAGE_LIST_SMALL_ICONS);
      refresh_status();
      menu_set_list_type(IMAGE_LIST_SMALL_ICONS);
      set_normal_cursor();
   }
}

void
toolbar_details(GtkWidget *widget, gpointer data)
{
   if (busy_level > 0) return;

   clear_preview_box();
   rc_set_int("image_list_type", IMAGE_LIST_DETAILS);

   if (GTK_TOGGLE_BUTTON(widget)->active)
   {
      set_busy_cursor();
      image_list_set_list_type(IMAGE_LIST(imagelist), IMAGE_LIST_DETAILS);
      refresh_status();
      menu_set_list_type(IMAGE_LIST_DETAILS);
      set_normal_cursor();
   }
}

static void
refresh_status()
{
   gchar buffer[64];

   /* set status_main(total files and size) */
   sprintf(buffer, _("Total %i file(s) (%s)"),
      IMAGE_LIST(imagelist)->nfiles,
      fsize(IMAGE_LIST(imagelist)->total_size));
   set_status_main(buffer);

   /*
    * When a new dir is selected, no file item are being selected.
    * so, clear the status bar.
    */
   set_status_file("");
   set_status_type(NULL, NULL);
   set_status_name("");
   set_status_prop("");
   toolbar_remove_enable(FALSE);
   toolbar_rename_enable(FALSE);
   toolbar_timestamp_enable(FALSE);
   toolbar_view_enable(image_list_get_first(IMAGE_LIST(imagelist)) != NULL);
}

gint
imagelist_doubleselect (GtkWidget * widget, GdkEventButton * event)
{
   if (event->type == GDK_2BUTTON_PRESS)
   {
      menu_file_view(widget, NULL);
   }
   return FALSE;
}

int
main (int argc, char *argv[])
{
   struct stat st;
   GList       *infos = NULL;
   ImageInfo   info,
               *pinfo;
   guchar      *r;
   int         option;

#ifdef ENABLE_NLS
   bindtextdomain(PACKAGE, LOCALEDIR);
   textdomain(PACKAGE);
#endif

   rc_init();

   /* Initial root directory */
   r = rc_get("root_directory");
   if (r == NULL)
   {
      if (getenv("HOME"))
      {
         strncpy(root, getenv("HOME"), sizeof(root));
      }
      else
      {
         strcpy(root, "/");
      }
   } else
   {
      strncpy(root, r, sizeof(root));
   }

   /* Search command line options */
   while( (option = getopt(argc, argv, "R:fish?vr")) != -1 )
   {
      switch(option)
      {
         case 'R':
            strncpy(root, optarg, sizeof(root));

            if (stat(root, &st) != 0)
            {
               printf("Error: cannot stat directory.\n");
               _exit(0);
            }

            if ((st.st_mode & S_IFMT) != S_IFDIR)
            {
               printf("Error: not a directory.");
               _exit(0);
            }

            break;

         case 'r':

#ifdef HAVE_GETCWD
            getcwd(root, sizeof(root));
#else
            if (getenv("HOME"))
            {
               strcnpy(root, getenv("HOME"), sizeof(root));
            }
            else
            {
               strcpy(root, "/");
            }
#endif
            break;

         case 'f':
            rc_set_boolean("full_screen", TRUE);

            break;

         case 'i':
            rc_set_boolean("fit_screen", TRUE);

            break;

         case 's':
            viewer_set_slide_show(TRUE);
            break;

         case 'v':
            printf("GTK See v%s", VERSION);
#if defined(__DATE__) && defined(__TIME__)
            printf(", %s %s", __DATE__ , __TIME__);
#endif
            printf("\nThis is free software; see the file named COPYING in the source for details.\n");
            _exit(0);
            break;

         case '?':
         case 'h':
         default:
            print_help_message();
      }
   }

   while(optind < argc)
   {
      if (stat(argv[optind], &st) != 0)
         continue;

      strncpy(info.name, argv[optind], sizeof(info.name));
      info.type = UNKNOWN;
      info.size = st.st_size;
      if (detect_image_type(argv[optind], &info))
      {
         pinfo = g_malloc(sizeof(ImageInfo));
         memcpy(pinfo, &info, sizeof(ImageInfo));
         infos = g_list_append(infos, pinfo);
      }

      optind++;
   }

   gtk_set_locale();

   gtk_init(&argc, &argv);

   if (infos == NULL)
   {
      gtksee_main();
   } else
   {
      get_viewer_window_with_files(infos);
   }

   gtk_main();
   return 0;
}

void
gtksee_main()
{
   GtkWidget      *main_vbox;
   GtkWidget      *menubar, *toolbar;
   GtkWidget      *statusbar;
   GtkWidget      *window;
   GtkWidget      *hpaned, *vpaned, *vbox;
   GtkAdjustment  *vadj;

   GtkStyle       *style;
   GdkPixmap      *pixmap, *blank_pixmap;
   GdkBitmap      *mask, *blank_mask;

   gchar          wd[256];
   gint           pos;

   /* it's safe to disable all events first */
   busy_level = 1;

   /* creating main window */
   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_signal_connect (GTK_OBJECT(window), "delete_event",
      GTK_SIGNAL_FUNC(close_gtksee), NULL);
   gtk_container_border_width(GTK_CONTAINER(window), 2);
   gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, TRUE);

   gtk_window_set_title(GTK_WINDOW(window), "GTK See");
   gtk_window_set_wmclass(GTK_WINDOW(window), "browser", "GTKSee");
   gtk_widget_set_uposition(window, 10, 10);

   browser_window = window;
   gtk_widget_realize(window);
   style = gtk_widget_get_style(window);

   /* creating dirtree */
#ifdef HAVE_GETCWD
   getcwd(wd, sizeof(wd));
#else
   strcpy(wd, "/");
#endif
   tree = dirtree_new_by_dir(window, root, wd);
   gtk_signal_connect(GTK_OBJECT(tree), "selection_changed",
                        GTK_SIGNAL_FUNC(dir_selected),
                        NULL);
   treewin = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(treewin),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
#ifndef GTK_HAVE_FEATURES_1_1_8
   gtk_widget_set_usize(treewin, PREVIEW_WIDTH, 170);
#endif

#ifdef GTK_HAVE_FEATURES_1_1_5
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(treewin), tree);
#else
   gtk_container_add(GTK_CONTAINER(treewin), tree);
#endif
   gtk_widget_show (tree);
   gtk_widget_show(treewin);

   /* creating pixmap */
   blank_pixmap = gdk_pixmap_create_from_xpm_d(
      window->window,
      &blank_mask, &style->bg[GTK_STATE_NORMAL],
      (gchar **)blank_xpm);

   /* creating preview area */
   preview_box = gtk_frame_new(NULL);
#ifndef GTK_HAVE_FEATURES_1_1_8
   gtk_widget_set_usize(preview_box, PREVIEW_WIDTH, PREVIEW_HEIGHT);
#endif
   old_preview_width = PREVIEW_WIDTH;
   old_preview_height = PREVIEW_HEIGHT;
   gtk_signal_connect_object(GTK_OBJECT(preview_box),
                              "size_allocate",
                              GTK_SIGNAL_FUNC(preview_size_changed),
                              GTK_OBJECT(preview_box));
   gtk_widget_show(preview_box);

   preview_image = gtk_preview_new(GTK_PREVIEW_COLOR);
   gtk_container_add(GTK_CONTAINER(preview_box), preview_image);
   gtk_widget_show(preview_image);

   /* adding tree & pixmap into vpaned */
   vpaned = gtk_vpaned_new();
#ifdef GTK_HAVE_FEATURES_1_1_8
   gtk_paned_pack1(GTK_PANED(vpaned), treewin, FALSE, TRUE);
   gtk_paned_pack2(GTK_PANED(vpaned), preview_box, TRUE, TRUE);
   gtk_paned_compute_position(GTK_PANED(vpaned), 170+PREVIEW_HEIGHT, 170, PREVIEW_HEIGHT);
   gtk_paned_set_position(GTK_PANED(vpaned), 170);
#else
   gtk_paned_add1(GTK_PANED(vpaned), treewin);
   gtk_paned_add2(GTK_PANED(vpaned), preview_box);
#endif
   gtk_widget_show(vpaned);

   /* creating entry */
   entry = gtk_entry_new();
   gtk_signal_connect(
      GTK_OBJECT(entry),
      "activate",
      (GtkSignalFunc)dir_entry_enter,
      entry);
   gtk_widget_show(entry);

   /* creating imagelist */
   imagelist = image_list_new(window);
   gtk_signal_connect_object(GTK_OBJECT(imagelist),
      "select_image",
      GTK_SIGNAL_FUNC(file_selected),
      GTK_OBJECT(imagelist));
   gtk_signal_connect_object(GTK_OBJECT(imagelist),
      "unselect_image",
      GTK_SIGNAL_FUNC(file_unselected),
      GTK_OBJECT(imagelist));
   gtk_signal_connect (GTK_OBJECT (imagelist),
      "button_press_event",
      GTK_SIGNAL_FUNC (imagelist_doubleselect),
      NULL);

#ifndef GTK_HAVE_FEATURES_1_1_8
   gtk_widget_set_usize(imagelist, 390, 370);
#endif

   gtk_widget_show(imagelist);

   /* adding entry & imagelist into vbox */
   vbox = gtk_vbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox), imagelist, TRUE, TRUE, 0);
   gtk_widget_show(vbox);

   /* adding vpaned * vbox into hpaned */
   hpaned = gtk_hpaned_new();
#ifdef GTK_HAVE_FEATURES_1_1_8
   gtk_paned_pack1(GTK_PANED(hpaned), vpaned, TRUE, TRUE);
   gtk_paned_pack2(GTK_PANED(hpaned), vbox, FALSE, TRUE);
   gtk_paned_compute_position(GTK_PANED(hpaned), 390+PREVIEW_WIDTH, PREVIEW_WIDTH, 390);
   gtk_paned_set_position(GTK_PANED(hpaned), PREVIEW_WIDTH);
#else
   gtk_paned_add1(GTK_PANED(hpaned), vpaned);
   gtk_paned_add2(GTK_PANED(hpaned), vbox);
#endif
   gtk_widget_show(hpaned);

   /* creating menubar */
   menubar = get_main_menu(window);
   gtk_widget_show(menubar);

   /* creating toolbar */
   toolbar = get_main_toolbar(window);
   gtk_widget_show(toolbar);

   /* creating statusbar */
   statusbar = get_main_status_bar(blank_pixmap, blank_mask);
   gtk_widget_show(statusbar);

   /* adding menubar, toolbar, hpaned & statusbar into main_vbox */
   main_vbox = gtk_vbox_new(FALSE, 2);
   gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(main_vbox), hpaned, TRUE, TRUE, 0);
   gtk_box_pack_end(GTK_BOX(main_vbox), statusbar, FALSE, FALSE, 0);
   gtk_widget_show(main_vbox);

   gtk_container_add(GTK_CONTAINER(window), main_vbox);

   /* other values */

   scanline_init(gdk_color_context_new(
      gdk_window_get_visual(window->window),
      gdk_window_get_colormap(window->window)),
      style);
   watch_cursor = gdk_cursor_new(GDK_WATCH);
   busy_level = 0;
   enable_auto_refresh = FALSE;

   gtk_widget_unrealize(window);
#ifdef GTK_HAVE_FEATURES_1_1_8
   gtk_window_set_default_size(GTK_WINDOW(window), PREVIEW_WIDTH + 390, 240 + PREVIEW_HEIGHT);
#endif

   gtk_widget_show(window);

   /* Icon of GTK See ... */
   pixmap = gdk_pixmap_create_from_xpm_d(window->window,
               &mask, &style->bg[GTK_STATE_NORMAL],
               (gchar **)gtksee_xpm);
   gdk_window_set_icon(window->window, NULL, pixmap, mask);
   gdk_window_set_icon_name(window->window, "GTK See Icon");
   g_free(pixmap);
   g_free(mask);

   /* selecting current working directory */

   strncpy(wd, root, sizeof(wd));
   pos = dirtree_select_dir(DIRTREE(tree), wd);

   if (pos >=0)
   {
      vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(treewin));
      gtk_adjustment_set_value(vadj,
         min(vadj->upper - vadj->page_size,
         DIRTREE(tree)->item_height * pos));
   }

}

static void
file_clipboard_clear()
{
   gint i;

   if (file_clipboard == NULL) return;
   for (i = 0; i < clipboard_size; i++)
   {
      if (file_clipboard[i] != NULL) free(file_clipboard[i]);
   }
   free(file_clipboard);
   file_clipboard = NULL;
   clipboard_size = 0;
}

static void
file_clipboard_alloc(guint num)
{
   gint i;

   clipboard_size = num;
   if (num < 1) return;
   file_clipboard = g_malloc(sizeof(guchar*) * num);
   for (i = 0; i < num; i++)
   {
      file_clipboard[i] = NULL;
   }
}

static void
file_clipboard_set(guint num, guchar *file)
{
   if (file_clipboard[num] != NULL) free(file_clipboard[num]);
   file_clipboard[num] = g_malloc(sizeof(guchar) * strlen(file) + 1);
   strcpy(file_clipboard[num], file);
}

static void
file_paste(guchar *src_file, guchar *dest_path, PasteType type)
{
   char buf[512];

   switch(type)
   {
     case PASTE_MOVING:
      sprintf(buf, "mv -f %s %s", src_file, dest_path);
      system(buf);
      break;
     case PASTE_COPYING:
      sprintf(buf, "cp -rf %s %s", src_file, dest_path);
      system(buf);
      break;
     case PASTE_LINKING:
      sprintf(buf, "%s/%s", dest_path, strrchr(src_file, '/') + 1);
      link(src_file, buf);
      break;
     default:
      break;
   }
}
