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
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "gtypes.h"
#include "scanline.h"
#include "imagelist.h"
#include "detect.h"
#include "dndviewport.h"
#include "rc.h"
#include "gtksee.h"
#include "viewertoolbar.h"
#include "viewerstatus.h"
#include "viewer.h"

#include "../icons/gtksee.xpm"
#include "pixmaps/blank.xpm"

#define MAX_WIDTH (gdk_screen_width()-10)
#define MAX_HEIGHT (gdk_screen_height()-80)
#define MAX_FULL_WIDTH (gdk_screen_width())
#define MAX_FULL_HEIGHT (gdk_screen_height())
#define MOVEMENT_INCREASE 200

static GtkWidget *window, *imagelist;
static GList *image_infos;
static ImageInfo *current;
static GtkWidget *frame;
static gchar viewing_image[256];
static gboolean viewer_busy;
static GdkCursor *viewer_watch_cursor = NULL;
static GtkWidget *viewport, *viewport_image;

static gboolean slide_show = FALSE;
static guint timeout_callback_tag = 0;
static gboolean have_xgrab;

void     view_current_image      ();
void     viewer_set_busy_cursor     ();
void     viewer_set_normal_cursor   ();
void     viewer_key_release      (GtkWidget *widget,
                   GdkEvent *event,
                   gpointer data);
void     viewer_menu_popup    (GtkWidget *widget,
                   gpointer data);
void     viewer_popup_view    (GtkWidget *widget,
                   gpointer data);
void     viewer_popup_slideshow     (GtkWidget *widget,
                   gpointer data);
void     viewer_popup_fitscreen     (GtkWidget *widget,
                   gpointer data);
void     viewer_slideshow_next      (gpointer data);
void     viewer_slideshow_toggled_internal
                  (gboolean active);
GtkWidget*  get_viewer_window_internal ();
GtkWidget*  get_full_view_window    ();

void
viewer_set_busy_cursor()
{
   if (!viewer_busy)
   {
      viewer_busy = TRUE;
      gdk_window_set_cursor(window->window, viewer_watch_cursor);
      while (gtk_events_pending()) gtk_main_iteration();
   }
}

void
viewer_set_normal_cursor()
{
   if (viewer_busy)
   {
      viewer_busy = FALSE;
      gdk_window_set_cursor(window->window, NULL);
   }
}

void
viewer_set_slide_show(gboolean s)
{
   slide_show = s;
}

void
view_current_image()
{
   gboolean    too_big, need_scale;
   gchar       buffer[256], *ptr;
   gint        width, height, max_width, max_height, tmp;
   gint        value, i;
   gboolean    full_screen, fit_screen;
   ImageCache  *loaded_cache;
   GtkPreview  *preview = GTK_PREVIEW(viewport_image);

   if (viewer_busy) return;

   viewer_set_busy_cursor();

   /* setting window title */
   if (imagelist == NULL)
   {
      ptr = strrchr(current->name, '/');
      if (ptr == NULL)
         ptr = current->name;
      else
         ptr++;
   } else
      ptr = current->name;

   sprintf(buffer, "%s - GTK See", ptr);
   gtk_window_set_title(GTK_WINDOW(window), buffer);

   full_screen = rc_get_boolean("full_screen");
   fit_screen = rc_get_boolean("fit_screen");

   if (!current -> valid)
   {
      if (!full_screen)
      {
         if (imagelist == NULL)
         {
            set_viewer_status_type(image_type_get_pixmap(current->type), image_type_get_mask(current->type));
         } else
         {
            set_viewer_status_type(current->type_pixmap, current->type_mask);
         }
         set_viewer_status_name(ptr);
         set_viewer_status_size(current->size);
         set_viewer_status_prop(_("Invalid"));
         set_viewer_status_zoom(100);
      }
      viewer_set_normal_cursor();
      return;
   }

   /* setting status bar */
   if (!full_screen)
   {
      if (imagelist == NULL)
      {
         set_viewer_status_type(image_type_get_pixmap(current->type), image_type_get_mask(current->type));
      } else
      {
         set_viewer_status_type(current->type_pixmap, current->type_mask);
      }
      set_viewer_status_name(ptr);
      set_viewer_status_size(current->size);
      image_convert_info(current, buffer);
      set_viewer_status_prop(buffer);
   }

   if (imagelist != NULL)
   {
      strcpy(viewing_image, image_list_get_dir(IMAGE_LIST(imagelist)));
      if (viewing_image[strlen(viewing_image) - 1] != '/') strcat(viewing_image,"/");
      strcat(viewing_image, current->name);
   } else
   {
      strcpy(viewing_image, current->name);
   }

   if (current -> width < 0)
   {
      if (detect_image_type(viewing_image, current))
      {
         /* we should refresh the image info */
         if (!full_screen)
         {
            image_convert_info(current, buffer);
            set_viewer_status_prop(buffer);
         }
         if (imagelist != NULL)
         {
            image_list_update_info(IMAGE_LIST(imagelist), current);
         }
      }
   }

   max_width = (full_screen?MAX_FULL_WIDTH:MAX_WIDTH);
   max_height = (full_screen?MAX_FULL_HEIGHT:MAX_HEIGHT);

   width = current->width;
   height = current->height;
   too_big = (width > max_width || height > max_height);

   need_scale = (fit_screen && too_big);

/* PARCHE
   need_scale = fit_screen;
*/
   if (need_scale)
   {
      if (max_width * height < max_height * width)
      {
         if (!full_screen)
         {
            set_viewer_status_zoom(max_width * 100 / width);
         }
         tmp = width;
         width = max_width;
         height = width * height / tmp;
      } else
      {
         if (!full_screen)
         {
            set_viewer_status_zoom(max_height * 100 / height);
         }
         tmp = height;
         height = max_height;
         width = height * width / tmp;
      }
      gtk_widget_set_usize(
         viewport,
         width,
         height);
   } else
   {
      if (!full_screen)
      {
         set_viewer_status_zoom(100);
      }
      gtk_widget_set_usize(
         viewport,
         min(width,max_width),
         min(height,max_height));
   }

   /* clear last image */
   gdk_window_clear_area(
      viewport_image->window,
      0, 0,
      viewport_image->allocation.width,
      viewport_image->allocation.height);

   /* allow the user to drag the viewport while displaying */
   gtk_grab_add(viewport);

   /* reset the viewport position to (0,0) */
   dnd_viewport_reset(DND_VIEWPORT(viewport));

   /* for slideshow: disable timeout temporarily before loading */
   if (slide_show && timeout_callback_tag != 0)
   {
      gtk_timeout_remove(timeout_callback_tag);
   }

   if (need_scale)
   {
      load_scaled_image(viewing_image, current,
         max_width, max_height,
         window,
         viewport_image,
         (slide_show ? 0 : (SCANLINE_INTERACT | SCANLINE_DISPLAY)));
   } else
   {
      load_scaled_image(viewing_image, current,
         -1, -1,
         window,
         viewport_image,
         (slide_show ? 0 : (SCANLINE_INTERACT | SCANLINE_DISPLAY)));
   }
/*Agregado por DANIEL OME */
loaded_cache = scanline_get_cache();
ptr = loaded_cache->buffer;
for (i = 0; i < loaded_cache->buffer_height; i++)
{
   gtk_preview_draw_row(preview, ptr,
      0, i, loaded_cache->buffer_width);
   ptr += loaded_cache->buffer_width * 3;
}
gtk_widget_draw(window, NULL);
/* FINAL DEL AGREGADO DE DANIEL OME */

   gtk_grab_remove(viewport);
   if (slide_show)
   {
      value = rc_get_int("slideshow_delay");
      timeout_callback_tag = gtk_timeout_add(
         ((value == RC_NOT_EXISTS) ? SLIDESHOW_DELAY : value),
         (GtkFunction)viewer_slideshow_next, NULL);
   }
   viewer_set_normal_cursor();
}

void
viewer_toolbar_browse(GtkWidget *widget, gpointer data)
{
   if (viewer_busy) return;

   if (slide_show) gtk_timeout_remove(timeout_callback_tag);
   if (have_xgrab)
   {
      gdk_pointer_ungrab(0);
      gdk_keyboard_ungrab(0);
   }
   gtk_widget_destroy(window);
   if (imagelist == NULL)
   {
      g_list_free(image_infos);
      gtksee_main();
   } else
   {
      show_browser();
   }
}

void
viewer_toolbar_full_screen(GtkWidget *widget, gpointer data)
{
   if (viewer_busy) return;

   rc_set_boolean("full_screen", TRUE);
   rc_save_gtkseerc();
   gtk_widget_destroy(window);
   get_full_view_window();
}

void
viewer_toolbar_next_image(GtkWidget *widget, gpointer data)
{
   GList *tmplist;
   ImageInfo *tmp;
   gboolean full_screen;

   if (viewer_busy) return;

   if (imagelist == NULL)
   {
      tmplist = g_list_next(image_infos);
      if (tmplist == NULL) return;
      image_infos = tmplist;
      tmp = (ImageInfo*) image_infos->data;
   } else
   {
      tmp = image_list_get_next(IMAGE_LIST(imagelist), current);
   }
   if (tmp == NULL) return;
   current = tmp;
   full_screen = rc_get_boolean("full_screen");

   if (!full_screen)
   {
      if (imagelist == NULL)
      {
         viewer_next_enable(g_list_next(image_infos) != NULL);
         viewer_prev_enable(g_list_previous(image_infos) != NULL);
      } else
      {
         viewer_next_enable(image_list_get_next(IMAGE_LIST(imagelist), current) != NULL);
         viewer_prev_enable(image_list_get_previous(IMAGE_LIST(imagelist), current) != NULL);
      }
   }
   view_current_image();
}

void
viewer_toolbar_prev_image(GtkWidget *widget, gpointer data)
{
   GList *tmplist;
   ImageInfo *tmp;
   gboolean full_screen;

   if (viewer_busy) return;

   if (imagelist == NULL)
   {
      tmplist = g_list_previous(image_infos);
      if (tmplist == NULL) return;
      image_infos = tmplist;
      tmp = (ImageInfo*) image_infos->data;
   } else
   {
      tmp = image_list_get_previous(IMAGE_LIST(imagelist), current);
   }
   if (tmp == NULL) return;
   current = tmp;
   full_screen = rc_get_boolean("full_screen");

   if (!full_screen)
   {
      if (imagelist == NULL)
      {
         viewer_next_enable(g_list_next(image_infos) != NULL);
         viewer_prev_enable(g_list_previous(image_infos) != NULL);
      } else
      {
         viewer_next_enable(image_list_get_next(IMAGE_LIST(imagelist), current) != NULL);
         viewer_prev_enable(image_list_get_previous(IMAGE_LIST(imagelist), current) != NULL);
      }
   }
   view_current_image();
}

void
viewer_toolbar_fitscreen_toggled(GtkWidget *widget, gpointer data)
{
   if (viewer_busy) return;

   rc_set_boolean("fit_screen", GTK_TOGGLE_BUTTON(widget)->active);
   rc_save_gtkseerc();
   view_current_image();
}

void
viewer_toolbar_slideshow_toggled(GtkWidget *widget, gpointer data)
{
   if (viewer_busy) return;

   viewer_slideshow_toggled_internal(GTK_TOGGLE_BUTTON(widget)->active);
}

void
viewer_slideshow_toggled_internal(gboolean active)
{
   gint value;

   slide_show = active;
   if (slide_show)
   {
      value = rc_get_int("slideshow_delay");
      timeout_callback_tag = gtk_timeout_add(
         ((value == RC_NOT_EXISTS) ? SLIDESHOW_DELAY : value),
         (GtkFunction)viewer_slideshow_next, NULL);
   } else
   {
      gtk_timeout_remove(timeout_callback_tag);
   }
}

void
viewer_slideshow_next(gpointer data)
{
   GList *tmplist;
   ImageInfo *tmp;
   gboolean full_screen;

   if (viewer_busy) return;

   if (imagelist == NULL)
   {
      tmplist = g_list_next(image_infos);
      if (tmplist == NULL)
      {
         image_infos = g_list_first(image_infos);
      } else
      {
         image_infos = tmplist;
      }
      tmp = (ImageInfo*) image_infos->data;
      if (tmp == NULL)
      {
         image_infos = g_list_first(image_infos);
         current = (ImageInfo*) image_infos->data;
      } else current = tmp;
   } else
   {
      tmp = image_list_get_next(IMAGE_LIST(imagelist), current);
      if (tmp == NULL) current = image_list_get_first(IMAGE_LIST(imagelist));
      else current = tmp;
   }

   full_screen = rc_get_boolean("full_screen");
   if (!full_screen)
   {
      if (imagelist == NULL)
      {
         viewer_next_enable(g_list_next(image_infos) != NULL);
         viewer_prev_enable(g_list_previous(image_infos) != NULL);
      } else
      {
         viewer_next_enable(image_list_get_next(IMAGE_LIST(imagelist), current) != NULL);
         viewer_prev_enable(image_list_get_previous(IMAGE_LIST(imagelist), current) != NULL);
      }
   }
   view_current_image();
}

void
viewer_toolbar_rotate_left(GtkWidget *widget, gpointer data)
{
   ImageCache *cache;
   gint i, j, bufsize, t;
   guchar *newbuf, *ptr1, *ptr2;

   if (viewer_busy) return;

   cache = scanline_get_cache();
   bufsize = cache->buffer_width * cache->buffer_height * 3;
   newbuf = g_malloc(bufsize);
   for (i = 0; i < cache->buffer_height; i++)
   {
      for (j = 0; j < cache->buffer_width; j++)
      {
         ptr1 = newbuf + ((cache->buffer_width - j - 1)
            * cache->buffer_height + i) * 3;
         ptr2 = cache->buffer +
            (i * cache->buffer_width + j) * 3;
         *ptr1++ = *ptr2++;
         *ptr1++ = *ptr2++;
         *ptr1++ = *ptr2++;
      }
   }
   g_free(cache->buffer);
   cache->buffer = newbuf;
   t = cache->buffer_width;
   cache->buffer_width = cache->buffer_height;
   cache->buffer_height = t;

   /* Display the rotated image */
   gtk_preview_size(GTK_PREVIEW(viewport_image),
      cache->buffer_width, cache->buffer_height);
   gtk_widget_queue_resize(viewport_image);
   for (i = 0; i < cache->buffer_height; i++)
   {
      gtk_preview_draw_row(GTK_PREVIEW(viewport_image),
         cache->buffer + i * cache->buffer_width * 3,
         0, i, cache->buffer_width);
   }
   gtk_widget_set_usize(viewport,
      min(cache->buffer_width, MAX_WIDTH),
      min(cache->buffer_height, MAX_HEIGHT));
   gtk_widget_draw(viewport_image, NULL);
}

void
viewer_toolbar_rotate_right(GtkWidget *widget, gpointer data)
{
   ImageCache *cache;
   gint i, j, bufsize, t;
   guchar *newbuf, *ptr1, *ptr2;

   if (viewer_busy) return;

   cache = scanline_get_cache();
   bufsize = cache->buffer_width * cache->buffer_height * 3;
   newbuf = g_malloc(bufsize);
   for (i = 0; i < cache->buffer_height; i++)
   {
      for (j = 0; j < cache->buffer_width; j++)
      {
         ptr1 = newbuf + (j * cache->buffer_height +
            (cache->buffer_height - i - 1)) * 3;
         ptr2 = cache->buffer +
            (i * cache->buffer_width + j) * 3;
         *ptr1++ = *ptr2++;
         *ptr1++ = *ptr2++;
         *ptr1++ = *ptr2++;
      }
   }
   g_free(cache->buffer);
   cache->buffer = newbuf;
   t = cache->buffer_width;
   cache->buffer_width = cache->buffer_height;
   cache->buffer_height = t;

   /* Display the rotated image */
   gtk_preview_size(GTK_PREVIEW(viewport_image),
      cache->buffer_width, cache->buffer_height);
   gtk_widget_queue_resize(viewport_image);
   for (i = 0; i < cache->buffer_height; i++)
   {
      gtk_preview_draw_row(GTK_PREVIEW(viewport_image),
         cache->buffer + i * cache->buffer_width * 3,
         0, i, cache->buffer_width);
   }
   gtk_widget_set_usize(viewport,
      min(cache->buffer_width, MAX_WIDTH),
      min(cache->buffer_height, MAX_HEIGHT));
   gtk_widget_draw(viewport_image, NULL);
}

void
viewer_toolbar_refresh(GtkWidget *widget, gpointer data)
{
   view_current_image();
}

void
viewer_key_release(GtkWidget *widget, GdkEvent *event, gpointer data)
{
   GList *tmplist;
   ImageInfo *tmp;
   gboolean full_screen;

   if (viewer_busy) return;

   full_screen = rc_get_boolean("full_screen");

   switch (event->key.keyval)
   {
     case 'f':
     case 'F':
      rc_set_boolean("fit_screen", !full_screen);
      if (viewer_busy) return;
      view_current_image();
      //viewer_popup_fitscreen(widget, NULL);
      break;

     case ' ':
      if (imagelist == NULL)
      {
         tmplist = g_list_next(image_infos);
         if (tmplist == NULL) break;
         image_infos = tmplist;
         tmp = (ImageInfo*) image_infos->data;
      } else
      {
         tmp = image_list_get_next(IMAGE_LIST(imagelist), current);
      }
      if (tmp != NULL)
      {
         viewer_toolbar_next_image(widget, NULL);
         break;
      }
      /* Loop to the first if we are at the end of the list */
     case GDK_Home:
      if (imagelist == NULL)
      {
         image_infos = g_list_first(image_infos);
         current = (ImageInfo*) image_infos->data;
         if (!full_screen)
         {
            viewer_next_enable(g_list_next(image_infos) != NULL);
            viewer_prev_enable(FALSE);
         }
      } else
      {
         current = image_list_get_first(IMAGE_LIST(imagelist));
         if (!full_screen)
         {
            viewer_next_enable(image_list_get_next(IMAGE_LIST(imagelist), current) != NULL);
            viewer_prev_enable(FALSE);
         }
      }
      view_current_image();
      break;
     case GDK_End:
      if (imagelist == NULL)
      {
         image_infos = g_list_last(image_infos);
         current = (ImageInfo*) image_infos->data;
         if (!full_screen)
         {
            viewer_prev_enable(g_list_previous(image_infos) != NULL);
            viewer_next_enable(FALSE);
         }
      } else
      {
         current = image_list_get_last(IMAGE_LIST(imagelist));
         if (!full_screen)
         {
            viewer_prev_enable(image_list_get_previous(IMAGE_LIST(imagelist), current) != NULL);
            viewer_next_enable(FALSE);
         }
      }
      view_current_image();
      break;
     case GDK_Page_Up:
      viewer_toolbar_prev_image(widget, NULL);
      break;
     case GDK_Page_Down:
      viewer_toolbar_next_image(widget, NULL);
      break;
     case GDK_Escape:
      viewer_toolbar_browse(widget, NULL);
      break;
     case GDK_Left:
      dnd_viewport_move(DND_VIEWPORT(viewport), -MOVEMENT_INCREASE, 0);
      break;
     case GDK_Right:
      dnd_viewport_move(DND_VIEWPORT(viewport), MOVEMENT_INCREASE, 0);
      break;
     case GDK_Up:
      dnd_viewport_move(DND_VIEWPORT(viewport), 0, -MOVEMENT_INCREASE);
      break;
     case GDK_Down:
      dnd_viewport_move(DND_VIEWPORT(viewport), 0, MOVEMENT_INCREASE);
      break;
     default:
      break;
   }
}

void
viewer_menu_popup(GtkWidget *widget, gpointer data)
{
   GtkWidget *menu, *menu_item;
   gboolean full_screen, fit_screen;

   if (viewer_busy) return;

   menu = gtk_menu_new();

   menu_item = gtk_menu_item_new();
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_widget_show(menu_item);

   menu_item = gtk_menu_item_new_with_label(_("Next image"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
      GTK_SIGNAL_FUNC(viewer_toolbar_next_image), NULL);
   if (imagelist == NULL)
   {
      gtk_widget_set_sensitive(
         menu_item,
         g_list_next(image_infos) != NULL);
   } else
   {
      gtk_widget_set_sensitive(
         menu_item,
         image_list_get_next(IMAGE_LIST(imagelist), current) != NULL);
   }
   gtk_widget_show(menu_item);

   menu_item = gtk_menu_item_new_with_label(_("Previous image"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
      GTK_SIGNAL_FUNC(viewer_toolbar_prev_image), NULL);
   if (imagelist == NULL)
   {
      gtk_widget_set_sensitive(
         menu_item,
         g_list_previous(image_infos) != NULL);
   } else
   {
      gtk_widget_set_sensitive(
         menu_item,
         image_list_get_previous(IMAGE_LIST(imagelist), current) != NULL);
   }
   gtk_widget_show(menu_item);

   menu_item = gtk_check_menu_item_new_with_label(_("Slide show"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   if (slide_show)
   {
      gtk_check_menu_item_set_state(
         GTK_CHECK_MENU_ITEM(menu_item),
         TRUE);
      gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
         GTK_SIGNAL_FUNC(viewer_popup_slideshow), NULL);
   } else
   {
      gtk_check_menu_item_set_state(
         GTK_CHECK_MENU_ITEM(menu_item),
         FALSE);
      gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
         GTK_SIGNAL_FUNC(viewer_popup_slideshow), NULL);
   }
   gtk_widget_show(menu_item);

   menu_item = gtk_menu_item_new();
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_widget_show(menu_item);

   menu_item = gtk_check_menu_item_new_with_label(_("Fit screen"));
   gtk_menu_append(GTK_MENU(menu), menu_item);

   fit_screen = rc_get_boolean("fit_screen");
   if (fit_screen)
   {
      gtk_check_menu_item_set_state(
         GTK_CHECK_MENU_ITEM(menu_item),
         TRUE);
      gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
         GTK_SIGNAL_FUNC(viewer_popup_fitscreen), NULL);
   } else
   {
      gtk_check_menu_item_set_state(
         GTK_CHECK_MENU_ITEM(menu_item),
         FALSE);
      gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
         GTK_SIGNAL_FUNC(viewer_popup_fitscreen), NULL);
   }
   gtk_widget_show(menu_item);

   menu_item = gtk_check_menu_item_new_with_label(_("Full screen"));
   gtk_menu_append(GTK_MENU(menu), menu_item);

   full_screen = rc_get_boolean("full_screen");
   if (full_screen)
   {
      gtk_check_menu_item_set_state(
         GTK_CHECK_MENU_ITEM(menu_item),
         TRUE);
      gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
         GTK_SIGNAL_FUNC(viewer_popup_view), NULL);
   } else
   {
      gtk_check_menu_item_set_state(
         GTK_CHECK_MENU_ITEM(menu_item),
         FALSE);
      gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
         GTK_SIGNAL_FUNC(viewer_toolbar_full_screen), NULL);
   }
   gtk_widget_show(menu_item);

   menu_item = gtk_menu_item_new();
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_widget_show(menu_item);

   menu_item = gtk_menu_item_new_with_label(_("Browser"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
      GTK_SIGNAL_FUNC(viewer_toolbar_browse), NULL);
   gtk_widget_show(menu_item);

   menu_item = gtk_menu_item_new_with_label(_("Exit"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
      GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
   gtk_widget_show(menu_item);

   gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, 1);
}

void
viewer_popup_view(GtkWidget *widget, gpointer data)
{
   if (viewer_busy) return;

   rc_set_boolean("full_screen", FALSE);
   rc_save_gtkseerc();
   gtk_widget_destroy(window);
   get_viewer_window_internal();
}

void
viewer_popup_slideshow(GtkWidget *widget, gpointer data)
{
   if (viewer_busy) return;

   viewer_slideshow_toggled_internal(GTK_CHECK_MENU_ITEM(widget)->active);
}

void
viewer_popup_fitscreen(GtkWidget *widget, gpointer data)
{
   if (viewer_busy) return;

   rc_set_boolean("fit_screen", GTK_CHECK_MENU_ITEM(widget)->active);
   rc_save_gtkseerc();
   view_current_image();
}

GtkWidget*
get_viewer_window(GtkWidget *il)
{
   gboolean full_screen;

   /* setting pointers and values... */
   imagelist = il;
   image_infos = NULL;
   if (viewer_watch_cursor == NULL)
   {
      viewer_watch_cursor = gdk_cursor_new(GDK_WATCH);
   }

   viewer_busy = FALSE;

   /* view the selected image if selected,
    * otherwise, view the first.
    */
   current = image_list_get_selected(IMAGE_LIST(imagelist));
   if (current == NULL || !current->valid || current->width < 0)
      current = image_list_get_first(IMAGE_LIST(imagelist));

   full_screen = rc_get_boolean("full_screen");
   have_xgrab = FALSE;
   if (full_screen)
   {
      return get_full_view_window();
   } else
   {
      return get_viewer_window_internal();
   }
}

GtkWidget*
get_viewer_window_with_files(GList *infos)
{
   gboolean full_screen;

   /* setting pointers and values... */
   imagelist = NULL;
   image_infos = g_list_first(infos);
   if (viewer_watch_cursor == NULL)
   {
      viewer_watch_cursor = gdk_cursor_new(GDK_WATCH);
   }

   viewer_busy = FALSE;

   /* view the selected image if selected,
    * otherwise, view the first.
    */
   current = (ImageInfo*) image_infos->data;
   if (current == NULL || !current->valid || current->width < 0)
      gtksee_main();

   full_screen = rc_get_boolean("full_screen");
   have_xgrab = FALSE;
   if (full_screen)
   {
      return get_full_view_window();
   } else
   {
      return get_viewer_window_internal();
   }
}

GtkWidget*
get_viewer_window_internal()
{
   GtkWidget *toolbar, *table, *statusbar;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GtkStyle *style;
   GdkEventMask event_mask;

   /* creating viewer window */
   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_signal_connect (GTK_OBJECT(window), "delete_event",
      GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
   gtk_container_border_width(GTK_CONTAINER(window), 2);
   gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, TRUE);

   gtk_window_set_title(GTK_WINDOW(window), "GTK See");
   gtk_window_set_wmclass(GTK_WINDOW(window), "viewer", "GTKSee");
   event_mask = gtk_widget_get_events(window);
   event_mask |= GDK_KEY_RELEASE_MASK;
   gtk_widget_set_events(window, event_mask);
   gtk_signal_connect(
      GTK_OBJECT(window),
      "key_release_event",
      GTK_SIGNAL_FUNC(viewer_key_release),
      NULL);
#if ((GTK_MAJOR_VERSION>1) || \
     (GTK_MAJOR_VERSION==1 && GTK_MINOR_VERSION>2) || \
     (GTK_MAJOR_VERSION==1 && GTK_MINOR_VERSION==2 && GTK_MICRO_VERSION>=5))
   gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ALWAYS);
#else
   gtk_widget_set_uposition(window, 0, 0);
#endif

   gtk_widget_realize(window);

   style = gtk_widget_get_style(window);
   pixmap = gdk_pixmap_create_from_xpm_d(
      window->window,
      &mask, &style->bg[GTK_STATE_NORMAL],
      (gchar **)gtksee_xpm);
   gdk_window_set_icon(window->window, NULL, pixmap, mask);
   gdk_window_set_icon_name(window->window, "GTK See Icon");
   g_free(pixmap);
   g_free(mask);

   /* disable toolbar events. */
   viewer_busy = 1;

   /* creating toolbar */
   toolbar = get_viewer_toolbar(window);
   viewer_slideshow_set_state(slide_show);
   gtk_widget_show(toolbar);

   viewer_busy = 0;

   /* creating frame */
   frame = gtk_frame_new(NULL);
   gtk_widget_show(frame);

   viewport = dnd_viewport_new();
   gtk_signal_connect(
      GTK_OBJECT(viewport),
      "double_clicked",
      GTK_SIGNAL_FUNC(viewer_toolbar_browse),
      NULL);
   gtk_container_add(GTK_CONTAINER(frame), viewport);
   gtk_widget_show(viewport);

   viewport_image = gtk_preview_new(GTK_PREVIEW_COLOR);
   gtk_container_add(GTK_CONTAINER(viewport), viewport_image);
   gtk_widget_show(viewport_image);

   /* creating viewer statusbar */
   pixmap = gdk_pixmap_create_from_xpm_d(
      window->window,
      &mask, &style->bg[GTK_STATE_NORMAL],
      (gchar **)blank_xpm);
   statusbar = get_viewer_status_bar(pixmap, mask);
   gtk_widget_show(statusbar);

   /* creating viewer table */
   table = gtk_table_new(3, 1, FALSE);
   gtk_widget_show(table);

   /* adding toolbar, frame & statusbar into table */
   gtk_table_attach(GTK_TABLE(table), toolbar, 0, 1, 0, 1, GTK_FILL|GTK_EXPAND, 0, 0, 2);
   gtk_table_attach_defaults(GTK_TABLE(table), frame, 0, 1, 1, 2);
   gtk_table_attach(GTK_TABLE(table), statusbar, 0, 1, 2, 3, GTK_FILL|GTK_EXPAND, 0, 0, 2);

   /* adding table into viewer window */
   gtk_container_add(GTK_CONTAINER(window), table);
   gtk_widget_show(window);

   if (imagelist == NULL)
   {
      image_list_load_pixmaps(window);
      scanline_init(gdk_color_context_new(
         gdk_window_get_visual(window->window),
         gdk_window_get_colormap(window->window)),
         style);
      viewer_next_enable(g_list_next(image_infos) != NULL);
      viewer_prev_enable(g_list_previous(image_infos) != NULL);
   } else
   {
      viewer_next_enable(image_list_get_next(IMAGE_LIST(imagelist), current) != NULL);
      viewer_prev_enable(image_list_get_previous(IMAGE_LIST(imagelist), current) != NULL);
   }
   view_current_image();

   return window;
}

GtkWidget*
get_full_view_window()
{
   GdkEventMask event_mask;
   GtkStyle *style;

   window = gtk_window_new(GTK_WINDOW_POPUP);

   gtk_container_border_width(GTK_CONTAINER(window), 0);
   gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, FALSE);
   gtk_widget_set_usize(window, MAX_FULL_WIDTH, MAX_FULL_HEIGHT);
   gtk_widget_set_uposition(window, 0, 0);
   event_mask = gtk_widget_get_events(window);
   event_mask |= GDK_KEY_RELEASE_MASK;
   gtk_widget_set_events(window, event_mask);
   gtk_signal_connect(
      GTK_OBJECT(window),
      "key_release_event",
      GTK_SIGNAL_FUNC(viewer_key_release),
      NULL);
   style = gtk_style_new();
   style->bg[GTK_STATE_NORMAL] = window->style->black;
   gtk_widget_set_style(window, style);

   /* cheating frame */
   frame = window;

   viewport = dnd_viewport_new();
   gtk_signal_connect(
      GTK_OBJECT(viewport),
      "double_clicked",
      GTK_SIGNAL_FUNC(viewer_toolbar_browse),
      NULL);
   gtk_widget_set_style(viewport, style);
   /* use popup menu only in full-screen mode */
   gtk_signal_connect(
      GTK_OBJECT(viewport),
      "popup",
      GTK_SIGNAL_FUNC(viewer_menu_popup),
      NULL);
   gtk_container_add(GTK_CONTAINER(frame), viewport);
   gtk_widget_show(viewport);

   viewport_image = gtk_preview_new(GTK_PREVIEW_COLOR);
   gtk_widget_set_style(viewport_image, style);
   gtk_container_add(GTK_CONTAINER(viewport), viewport_image);
   gtk_widget_show(viewport_image);

   gtk_widget_realize(window);
   gdk_window_set_decorations(window->window, 0);
   gdk_window_set_functions(window->window, 0);
   gtk_widget_show(window);

   if (imagelist == NULL)
   {
      image_list_load_pixmaps(window);
      scanline_init(gdk_color_context_new(
         gdk_window_get_visual(window->window),
         gdk_window_get_colormap(window->window)),
         style);
   }

   view_current_image();

   /* Make X grab to full-screen window */
   if ((gdk_pointer_grab(window->window, TRUE,
      GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
      GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
      GDK_POINTER_MOTION_MASK,
      NULL, NULL, 0) == 0))
   {
      if (gdk_keyboard_grab(window->window, TRUE, 0) != 0)
      {
         gdk_pointer_ungrab(0);
      } else
      {
         have_xgrab = TRUE;
      }
   }

   return window;
}
