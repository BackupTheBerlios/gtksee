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

#define LABEL_HEIGHT 17

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "gtypes.h"
#include "util.h"
#include "scanline.h"
#include "rc.h"
#include "imagetnitem.h"

/* CLAMP is defined in glib.h */
#define  CLAMP0255(x)         CLAMP(x,0,255)

typedef struct
{
   GtkPreview *image;
   time_t last_modified;
} Thumbnail;

static GHashTable *thumbnail_cache = NULL;

static void image_tnitem_class_init    (ImageTnItemClass *klass);
static void image_tnitem_init          (ImageTnItem *ii);
static void image_tnitem_draw          (GtkWidget *widget,
                                          GdkRectangle *area);
static gint image_tnitem_expose        (GtkWidget *widget,
                                          GdkEventExpose *event);
static void image_tnitem_paint         (GtkWidget *widget,
                                          GdkRectangle *area);
static void image_tnitem_realize       (GtkWidget *widget);
static void image_tnitem_load_image    (ImageTnItem *item);
static gint image_tnitem_compare_func  (gpointer data1, gpointer data2);

static void
image_tnitem_class_init(ImageTnItemClass *klass)
{
   GtkWidgetClass *widget_class;

   widget_class = (GtkWidgetClass*) klass;

   widget_class->draw = image_tnitem_draw;
   widget_class->realize = image_tnitem_realize;
   widget_class->expose_event = image_tnitem_expose;
}

static void
image_tnitem_init(ImageTnItem *ii)
{
   ii->label[0] = '\0';
   ii->filename[0] = '\0';
   ii->color_set = FALSE;
   ii->image = NULL;
   ii->bg_gc = NULL;
}

guint
image_tnitem_get_type()
{
   static guint ii_type = 0;

   if (!ii_type)
   {
      GtkTypeInfo ii_info =
      {
         "ImageTnItem",
         sizeof (ImageTnItem),
         sizeof (ImageTnItemClass),
         (GtkClassInitFunc) image_tnitem_class_init,
         (GtkObjectInitFunc) image_tnitem_init,
         (GtkArgSetFunc) NULL,
         (GtkArgGetFunc) NULL
      };
      ii_type = gtk_type_unique (gtk_widget_get_type(), &ii_info);
   }
   return ii_type;
}

GtkWidget*
image_tnitem_new(guchar *label)
{
   GtkWidget *ii;

   ii = GTK_WIDGET(gtk_type_new(image_tnitem_get_type()));

   if (label != NULL)
      strcpy(IMAGE_TNITEM(ii)->label, label);

   gtk_widget_set_usize(ii, THUMBNAIL_WIDTH+8, THUMBNAIL_HEIGHT+12+LABEL_HEIGHT);

   return ii;
}

void
image_tnitem_set_image(ImageTnItem *ii, guchar *filename)
{
   strcpy(ii->filename, filename);
   if (GTK_WIDGET_REALIZED(ii))
   {
      image_tnitem_load_image(ii);
      gtk_widget_queue_draw(GTK_WIDGET(ii));
   }
}

void
image_tnitem_set_color(ImageTnItem *ii, GdkColor *color)
{
   if (color == NULL)
   {
      ii->color_set = FALSE;
   } else
   {
      ii->color_set = TRUE;
      ii->color.pixel = color->pixel;
   }
   gtk_widget_queue_draw(GTK_WIDGET(ii));
}

void
image_tnitem_destroy(ImageTnItem *ii)
{
   /*if (ii->image != NULL) gtk_widget_destroy(ii->image);*/
   if (ii->bg_gc != NULL) gdk_gc_destroy(ii->bg_gc);
   gtk_widget_destroy(GTK_WIDGET(ii));
}

static void
image_tnitem_draw(GtkWidget *widget, GdkRectangle *area)
{
   if (GTK_WIDGET_DRAWABLE(widget))
   {
      image_tnitem_paint(widget, area);
      gtk_widget_draw_default(widget);
      gtk_widget_draw_focus(widget);
   }
}

static void
image_tnitem_paint(GtkWidget *widget, GdkRectangle *area)
{
   ImageTnItem *item;
   guchar label_str[80];
   gint i;

   if (GTK_WIDGET_DRAWABLE(widget))
   {
      item = IMAGE_TNITEM(widget);

      /* clear area */
      gtk_style_set_background(
         widget->style,
         widget->window,
         GTK_STATE_NORMAL);
      /* skip this should avoid flickering */
      /*gdk_window_clear_area(widget->window,
         area->x, area->y, area->width, area->height);*/

      /* draw shadows, fill label with bg_gc if needed */
      gtk_draw_shadow(
         widget->style,
         widget->window,
         GTK_STATE_NORMAL,
         GTK_SHADOW_OUT,
         0, 0, THUMBNAIL_WIDTH+8, THUMBNAIL_HEIGHT+8);
      if (GTK_WIDGET_STATE(widget) == GTK_STATE_SELECTED)
      {
         gdk_draw_rectangle(
            widget->window,
            widget->style->bg_gc[GTK_STATE_SELECTED],
            TRUE,
            0, THUMBNAIL_HEIGHT+10,
            THUMBNAIL_WIDTH+8, LABEL_HEIGHT+2);
      } else if (item->color_set)
      {
         gdk_gc_set_foreground(item->bg_gc, &item->color);
         gdk_draw_rectangle(
            widget->window,
            item->bg_gc,
            TRUE,
            0, THUMBNAIL_HEIGHT+10,
            THUMBNAIL_WIDTH+8, LABEL_HEIGHT+2);
      }
      gtk_draw_shadow(
         widget->style,
         widget->window,
         GTK_WIDGET_STATE(widget),
         GTK_SHADOW_IN,
         0, THUMBNAIL_HEIGHT+10,
         THUMBNAIL_WIDTH+8, LABEL_HEIGHT+2);

      /* draw label */
      strcpy(label_str, item->label);
      if (gdk_string_width(widget->style->font, label_str) > THUMBNAIL_WIDTH)
      {
         i = strlen(label_str);
         if (i < 3) return;
         label_str[i-1] = '\0';
         strcat(label_str, "...");
         i = strlen(label_str);
         while (i > 3 && gdk_string_width(widget->style->font, label_str) > THUMBNAIL_WIDTH)
         {
            label_str[i-1] = '\0';
            label_str[i-4] = '.';
            i--;
         }
      }
      gdk_draw_string(
         widget->window,
         widget->style->font,
         widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
         4, THUMBNAIL_HEIGHT+9+widget->style->font->ascent
         +widget->style->font->descent, label_str);

      /* draw the thumbnail image */
      if (item->image != NULL)
      {
         gtk_preview_put(
            item->image,
            widget->window,
            widget->style->fg_gc[GTK_STATE_NORMAL],
            0, 0,
            4+(THUMBNAIL_WIDTH-item->image->buffer_width)/2,
            4+(THUMBNAIL_HEIGHT-item->image->buffer_height)/2,
            item->image->buffer_width,
            item->image->buffer_height);
      }
   }
}

static void
image_tnitem_realize(GtkWidget *widget)
{
   ImageTnItem *item;
   GdkWindowAttr attributes;
   gint attributes_mask;

   g_return_if_fail(widget != NULL);
   g_return_if_fail(IS_IMAGE_TNITEM(widget));

   item = IMAGE_TNITEM(widget);
   GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

   attributes.window_type = GDK_WINDOW_CHILD;
   attributes.x = widget->allocation.x;
   attributes.y = widget->allocation.y;
   attributes.width = widget->allocation.width;
   attributes.height = widget->allocation.height;
   attributes.wclass = GDK_INPUT_OUTPUT;
   attributes.visual = gtk_widget_get_visual (widget);
   attributes.colormap = gtk_widget_get_colormap (widget);
   attributes.event_mask = gtk_widget_get_events (widget);
   attributes.event_mask |= (
      GDK_EXPOSURE_MASK |
      GDK_BUTTON_PRESS_MASK |
      GDK_BUTTON_RELEASE_MASK |
      GDK_ENTER_NOTIFY_MASK |
      GDK_LEAVE_NOTIFY_MASK);
   attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
   widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
   gdk_window_set_user_data (widget->window, item);

   widget->style = gtk_style_attach (widget->style, widget->window);
   gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);

   item->bg_gc = gdk_gc_new(widget->window);

   image_tnitem_load_image(item);
}

static gint
image_tnitem_expose(GtkWidget *widget, GdkEventExpose *event)
{
   ImageTnItem *item;

   g_return_val_if_fail(widget != NULL, FALSE);
   g_return_val_if_fail(IS_IMAGE_TNITEM(widget), FALSE);
   g_return_val_if_fail(event != NULL, FALSE);

   if (GTK_WIDGET_DRAWABLE(widget))
   {
      item = IMAGE_TNITEM(widget);
      image_tnitem_paint(widget, &event->area);
      gtk_widget_draw_default(widget);
      gtk_widget_draw_focus(widget);
   }
   return FALSE;
}

static void
image_tnitem_load_image(ImageTnItem *item)
{
   GtkPreview *cache_img;
   Thumbnail *tn;
   ImageInfo *info;
   gboolean fast_preview;

   /* For thumnbails ... not used yet
   gchar       *pathname, *xvpathname, *thumbname;
   guchar      *buffer;
   FILE        *fp;
   gint        i, j, w, h;
   ImageCache  *cache;
   */

   fast_preview = rc_get_boolean("fast_preview");
   if (strlen(item->filename) > 0)
   {
      /* create cache if necessary */
      if (thumbnail_cache == NULL)
         thumbnail_cache = g_hash_table_new(
            (GHashFunc)strtohash,
            (GCompareFunc)image_tnitem_compare_func);
      info = (ImageInfo*) gtk_object_get_user_data(GTK_OBJECT(item));
      if (info == NULL)
      {
         item->image = NULL;
         return;
      }
      tn = (Thumbnail*) g_hash_table_lookup(thumbnail_cache, item->filename);
      if (tn == NULL || info->time > tn->last_modified)
      {
         if (tn == NULL)
         {
            tn = g_malloc(sizeof(Thumbnail));
            g_hash_table_insert(thumbnail_cache, g_strdup(item->filename), tn);
         } else
         {
            gtk_widget_destroy(GTK_WIDGET(tn->image));
         }
         cache_img = GTK_PREVIEW(gtk_preview_new(GTK_PREVIEW_COLOR));
         load_scaled_image(
            item->filename,
            info,
            THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT,
            GTK_WIDGET(item),
            GTK_WIDGET(cache_img),
            SCANLINE_INTERACT |
            (fast_preview?SCANLINE_PREVIEW:0));
         tn->image = cache_img;
         tn->last_modified = info->time;

         /* make thumbnails ... code not used yet
         pathname   = g_dirname (item->filename);
         xvpathname = g_strconcat (pathname, "/", ".xvpics", NULL);
         thumbname  = g_strconcat (xvpathname, "/", item->label, NULL);

         printf("Directorio: %s\n", xvpathname);
         printf("XV Archivo: %s\n", thumbname);

         mkdir (xvpathname, 0755);

         fp = fopen (thumbname, "wb");
         if (fp == NULL)
         {
            perror("Error: ");
         }

         cache  = scanline_get_cache();
         buffer = cache->buffer;
         h = cache->buffer_height;
         w = cache->buffer_width;

         fprintf (fp,
            "P7 332\n#IMGINFO:%dx%dx (%d %s)\n"
             "#Created by GTK See\n"
             "#END_OF_COMMENTS\n%d %d 255\n",
             info->width, info->height,
             (int)info->size, "bytes",
             w, h);

         // Code taken from GIMP ...
         for (i=0; i<h; i++)
         {
            gint rerr=0, gerr=0, berr=0;

            for (j=0; j<w; j++)
            {
               gint32 r,g,b;

               r = *(buffer++) + rerr;
               g = *(buffer++) + gerr;
               b = *(buffer++) + berr;

               r = CLAMP0255 (r);
               g = CLAMP0255 (g);
               b = CLAMP0255 (b);

               fputc(((r>>5)<<5) | ((g>>5)<<2) | (b>>6), fp);

               rerr = r - ( (r>>5) * 255 ) / 7;
               gerr = g - ( (g>>5) * 255 ) / 7;
               berr = b - ( (b>>6) * 255 ) / 3;
            }
         }
         fclose(fp);
         g_free(pathname);
         g_free(xvpathname);
         g_free(thumbname);
         */
         /* Final de la rutina de thumbnail */
      }
      item->image = tn->image;
   } else
   {
      item->image = NULL;
   }
}

static gint
image_tnitem_compare_func(gpointer data1, gpointer data2)
{
   return (strcmp((guchar*) data1, (guchar*) data2) == 0);
}
