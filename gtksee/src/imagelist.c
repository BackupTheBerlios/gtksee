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

/*
 * 2003-09-10: Little modifications.
 *             - Suffixes and colors are now in a matrix.
 *             - Changes from strcpy to strncpy
 *             - Change format of image_type_get_color function
 *             - Added image_type_get_type function
 *             - Added image_cmp_ascend_by_type and
 *                     image_cmp_descend_by_type functions
 *             - Added image_get_load_file
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>

#if HAVE_DIRENT_H
#  include <dirent.h>
#  define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen
#  if HAVE_SYS_NDIR_H
#     include <sys/ndir.h>
#  endif
#  if HAVE_SYS_DIR_H
#     include <sys/dir.h>
#  endif
#  if HAVE_NDIR_H
#     include <ndir.h>
#  endif
#endif

#include "detect.h"
#include "gtypes.h"
#include "util.h"
#include "rc.h"

#include "imagelist.h"
#include "imageclist.h"
#include "imagetnlist.h"
#include "imagesilist.h"

#include "gtkseestatus.h"

#include "pixmaps/file_xpm.xpm"
#include "pixmaps/file_gif.xpm"
#include "pixmaps/file_jpg.xpm"
#include "pixmaps/file_bmp.xpm"
#include "pixmaps/file_ico.xpm"
#include "pixmaps/file_pcx.xpm"
#include "pixmaps/file_tif.xpm"
#include "pixmaps/file_png.xpm"
#include "pixmaps/file_pnm.xpm"
#include "pixmaps/file_psd.xpm"
#include "pixmaps/file_xbm.xpm"
#include "pixmaps/file_xcf.xpm"
#include "pixmaps/file_tga.xpm"
#include "pixmaps/file_xwd.xpm"
#include "pixmaps/file_sun.xpm"
#include "pixmaps/file_eps.xpm"
#include "pixmaps/file_sgi.xpm"
#include "pixmaps/file_unknown.xpm"

#define SUFFIX(str, suf) (strstr(str,suf)>str)

/* From gtypes.h
   typedef enum
   {
      JPG,
      PNG,
      GIF,
      XCF,
      XPM,
      BMP,
      ICO,
      PCX,
      TIF,
      PNM,
      PSD,
      XBM,
      TGA,
      XWD,
      SUN,
      EPS,
      SGI,
      UNKNOWN,
      MAX_IMAGE_TYPES
   } ImageType;
*/

const gchar *suffix_color[][2]= {
                              {"Jpeg", "#E0E8C0"},
                              {"Png" , "#E0D8D0"},
                              {"Gif" , "#D0E8D0"},
                              {"Xcf" , "#BFEFE9"},
                              {"Xpm" , "#DED9EA"},
                              {"Bmp" , "#E5D5D5"},
                              {"Icon", "#F9F0D0"},
                              {"Pcx" , "#D8E7D8"},
                              {"Tiff", "#C8C8D5"},
                              {"Pnm" , "#DAEDC0"},
                              {"Psd" , "#FFFFDF"},
                              {"Xbm" , "#E0E0E0"},
                              {"Tga" , "#E6FFFF"},
                              {"Xwd" , "#DEEEFD"},
                              {"Sun" , "#DEDEDE"},
                              {"Eps" , "#EDEDED"},
                              {"Sgi" , "#A8F6FF"},
                              {"Unk" , "#FFFFFF"},
                              {NULL  ,  NULL    }
                            };

const gchar *ncolors[] = {"0", "2", "4", "8", "16", "32", "64", "128",
   "256", "512", "1K", "2K", "4K", "8K", "16K", "32K", "64K", "128K",
   "256K", "512K", "1M", "2M", "4M", "8M", "16M", "32M", "64M", "128M",
   "256M", "512M", "1G", "2G", "4G"};

static GdkColor *gdkcolors[MAX_IMAGE_TYPES - 1];

enum
{
   SELECT_IMAGE_SIGNAL,
   UNSELECT_IMAGE_SIGNAL,
   MAX_SIGNALS
};

static gint image_list_signals [MAX_SIGNALS];

static void image_list_class_init   (ImageListClass *class);
static void image_list_init         (ImageList *il);

static void image_list_selected     (GtkObject *obj);
static void image_list_unselected   (GtkObject *obj);
static void image_list_construct    (ImageList *il);

static GdkColor* image_tooltips_bgcolor (GdkWindow *window, GdkColormap *colormap);

static GdkPixmap *pixmaps[MAX_IMAGE_TYPES];
static GdkBitmap *masks  [MAX_IMAGE_TYPES];

guint
image_list_get_type()
{
   static guint il_type = 0;

   if (!il_type)
   {
      GtkTypeInfo il_info  = {  "ImageList",
                                sizeof (ImageList),
                                sizeof (ImageListClass),
                                (GtkClassInitFunc) image_list_class_init,
                                (GtkObjectInitFunc) image_list_init,
                                (GtkArgSetFunc) NULL,
                                (GtkArgGetFunc) NULL
                             };

      il_type = gtk_type_unique (gtk_hbox_get_type (), &il_info);
   }
   return il_type;
}

static void
image_list_class_init(ImageListClass *class)
{
   GtkObjectClass *object_class;
   object_class = (GtkObjectClass*) class;
   image_list_signals[SELECT_IMAGE_SIGNAL] = gtk_signal_new ("select_image",
                                                GTK_RUN_FIRST,
                                                object_class->type,
                                                GTK_SIGNAL_OFFSET (ImageListClass, select_image),
                                                gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

   image_list_signals[UNSELECT_IMAGE_SIGNAL] = gtk_signal_new ("unselect_image",
                                                GTK_RUN_FIRST,
                                                object_class->type,
                                                GTK_SIGNAL_OFFSET (ImageListClass, unselect_image),
                                                gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

   gtk_object_class_add_signals (object_class, image_list_signals, MAX_SIGNALS);
   class->select_image  = NULL;
   class->unselect_image= NULL;
}

static void
image_list_init(ImageList *il)
{
   ImageListType t;

   GTK_BOX (il)->spacing      = 0;
   GTK_BOX (il)->homogeneous  = TRUE;
   t = rc_get_int("image_list_type");
   il -> list_type   = (t == RC_NOT_EXISTS ? IMAGE_LIST_DETAILS : t);
   il -> parent      = NULL;
   il -> child_list  = NULL;
   il -> nfiles      = 0;
   il -> total_size  = 0;
   il -> dir[0]      = '\0';
}

void
image_list_load_pixmaps(GtkWidget *parent)
{
   static gboolean loaded = FALSE;
   static GtkStyle *style;

   if (!loaded)
   {
      loaded   = TRUE;
      style    = gtk_widget_get_style(parent);

      pixmaps[XPM] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[XPM], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_xpm_xpm);
      pixmaps[GIF] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[GIF], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_gif_xpm);
      pixmaps[JPG] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[JPG], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_jpg_xpm);
      pixmaps[BMP] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[BMP], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_bmp_xpm);
      pixmaps[ICO] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[ICO], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_ico_xpm);
      pixmaps[PCX] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[PCX], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_pcx_xpm);
      pixmaps[TIF] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[TIF], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_tif_xpm);
      pixmaps[PNG] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[PNG], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_png_xpm);
      pixmaps[PNM] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[PNM], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_pnm_xpm);
      pixmaps[PSD] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[PSD], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_psd_xpm);
      pixmaps[XBM] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[XBM], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_xbm_xpm);
      pixmaps[XCF] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[XCF], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_xcf_xpm);
      pixmaps[TGA] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[TGA], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_tga_xpm);
      pixmaps[XWD] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[XWD], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_xwd_xpm);
      pixmaps[SUN] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[SUN], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_sun_xpm);
      pixmaps[EPS] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[EPS], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_eps_xpm);
      pixmaps[SGI] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[SGI], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_sgi_xpm);
      pixmaps[UNKNOWN] = gdk_pixmap_create_from_xpm_d(
                           parent->window,
                           &masks[UNKNOWN], &style->bg[GTK_STATE_NORMAL],
                           (gchar **)file_unknown_xpm);
   }
}

GtkWidget*
image_list_new(GtkWidget *parent)
{
   static gboolean loaded = FALSE;
   static GdkColormap *colormap;

   ImageList *il;
   gint i;

   if (!loaded)
   {
      loaded = TRUE;
      colormap = gdk_window_get_colormap(parent->window);
      image_list_load_pixmaps(parent);
      for (i = 0; i < MAX_IMAGE_TYPES - 1; i++)
      {
         gdkcolors[i] = g_malloc(sizeof(GdkColor));
         gdkcolors[i]->pixel = 0;
         gdk_color_parse(suffix_color[i][1], gdkcolors[i]);
         gdk_color_alloc(colormap, gdkcolors[i]);
      }
   }

   il          = IMAGE_LIST(gtk_type_new(image_list_get_type()));
   il->parent  = parent;

   image_list_construct(il);

   return GTK_WIDGET(il);
}

static void
image_list_construct(ImageList *il)
{
   GtkWidget *list;
   GtkWidget *scrolled_win;

   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         scrolled_win = image_tnlist_new();
         list = GTK_WIDGET(gtk_container_children(GTK_CONTAINER(gtk_container_children(GTK_CONTAINER(scrolled_win))->data))->data);
         il->child_scrolled_win = scrolled_win;
         image_tnlist_set_width(IMAGE_TNLIST(list),
                                 GTK_WIDGET(il)->allocation.width);
         il->child_list = list;
         gtk_signal_connect_object(GTK_OBJECT(list),
                                    "select_image",
                                    GTK_SIGNAL_FUNC(image_list_selected),
                                    GTK_OBJECT(il));

         gtk_widget_show(scrolled_win);
         gtk_box_pack_start_defaults(GTK_BOX(il), scrolled_win);
         image_tnlist_set_dir(IMAGE_TNLIST(list), il->dir);
         il->nfiles     = IMAGE_TNLIST(il->child_list)->nfiles;
         il->total_size = IMAGE_TNLIST(il->child_list)->total_size;
         break;

      case IMAGE_LIST_SMALL_ICONS:
         scrolled_win = image_silist_new();
         list = GTK_WIDGET(gtk_container_children(GTK_CONTAINER(gtk_container_children(GTK_CONTAINER(scrolled_win))->data))->data);
         il->child_scrolled_win = scrolled_win;
         image_silist_set_height(IMAGE_SILIST(list),
                                 GTK_WIDGET(il)->allocation.height);
         il->child_list = list;
         gtk_signal_connect_object(GTK_OBJECT(list),
                                    "select_image",
                                    GTK_SIGNAL_FUNC(image_list_selected),
                                    GTK_OBJECT(il));

         gtk_widget_show(scrolled_win);
         gtk_box_pack_start_defaults(GTK_BOX(il), scrolled_win);
         image_silist_set_dir(IMAGE_SILIST(list), il->dir);
         il->nfiles     = IMAGE_SILIST(il->child_list)->nfiles;
         il->total_size = IMAGE_SILIST(il->child_list)->total_size;
         break;

      case IMAGE_LIST_DETAILS:
      default:
#ifdef GTK_HAVE_FEATURES_1_1_4
         scrolled_win = image_clist_new();
         list = GTK_WIDGET(gtk_container_children(GTK_CONTAINER(scrolled_win))->data);
         il->child_scrolled_win = scrolled_win;
#else
         list = image_clist_new();
#endif
         il->child_list = list;
         image_clist_set_dir(IMAGE_CLIST(list), il->dir);
         il->nfiles     = IMAGE_CLIST(il->child_list)->nfiles;
         il->total_size = IMAGE_CLIST(il->child_list)->total_size;
         gtk_signal_connect_object(GTK_OBJECT(list),
                                    "select_row",
                                    GTK_SIGNAL_FUNC(image_list_selected),
                                    GTK_OBJECT(il));
         gtk_signal_connect_object(GTK_OBJECT(list),
                                    "unselect_row",
                                    GTK_SIGNAL_FUNC(image_list_unselected),
                                    GTK_OBJECT(il));
#ifdef GTK_HAVE_FEATURES_1_1_4
         gtk_widget_show(scrolled_win);
         gtk_box_pack_start_defaults(GTK_BOX(il), scrolled_win);
#else
         gtk_widget_show(list);
         gtk_box_pack_start_defaults(GTK_BOX(il), list);
#endif
         break;
   }
}

void
image_list_set_dir(ImageList *il, guchar *dir)
{
   GtkWidget *list = il->child_list;
   /* we must remember the current directory */
   strncpy(il->dir, dir, sizeof(il->dir));

   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         image_tnlist_set_width(IMAGE_TNLIST(list),
                                 GTK_WIDGET(il)->allocation.width);
         image_tnlist_set_dir(IMAGE_TNLIST(list), dir);
         il->nfiles     = IMAGE_TNLIST(list)->nfiles;
         il->total_size = IMAGE_TNLIST(list)->total_size;
         break;

      case IMAGE_LIST_SMALL_ICONS:
         image_silist_set_height(IMAGE_SILIST(list),
                                 GTK_WIDGET(il)->allocation.height);
         image_silist_set_dir(IMAGE_SILIST(list), dir);
         il->nfiles     = IMAGE_SILIST(list)->nfiles;
         il->total_size = IMAGE_SILIST(list)->total_size;
         break;

      case IMAGE_LIST_DETAILS:
      default:
         image_clist_set_dir(IMAGE_CLIST(list), dir);
         il->nfiles     = IMAGE_CLIST(list)->nfiles;
         il->total_size = IMAGE_CLIST(list)->total_size;
         break;
   }
}

guchar*
image_list_get_dir(ImageList *il)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         return IMAGE_TNLIST(il->child_list)->dir;

      case IMAGE_LIST_SMALL_ICONS:
         return IMAGE_SILIST(il->child_list)->dir;

      case IMAGE_LIST_DETAILS:
      default:
         return IMAGE_CLIST(il->child_list)->dir;
   }
}

void
image_list_refresh(ImageList *il)
{
   GtkWidget *list = il->child_list;

   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         image_tnlist_set_width(IMAGE_TNLIST(list),
                                 GTK_WIDGET(il)->allocation.width);
         image_tnlist_refresh(IMAGE_TNLIST(list));
         il->nfiles     = IMAGE_TNLIST(list)->nfiles;
         il->total_size = IMAGE_TNLIST(list)->total_size;
         break;

      case IMAGE_LIST_SMALL_ICONS:
         image_silist_set_height(IMAGE_SILIST(list),
                                    GTK_WIDGET(il)->allocation.height);
         image_silist_refresh(IMAGE_SILIST(list));
         il->nfiles     = IMAGE_SILIST(list)->nfiles;
         il->total_size = IMAGE_SILIST(list)->total_size;
         break;

      case IMAGE_LIST_DETAILS:
      default:
         image_clist_refresh(IMAGE_CLIST(list));
         il->nfiles     = IMAGE_CLIST(list)->nfiles;
         il->total_size = IMAGE_CLIST(list)->total_size;
         break;
   }
}

void
image_list_clear(ImageList *il)
{
   if (il->child_list != NULL)
   {
      switch (il->list_type)
      {
         case IMAGE_LIST_THUMBNAILS:
            image_tnlist_clear(IMAGE_TNLIST(il->child_list));
            gtk_container_remove(GTK_CONTAINER(il), il->child_scrolled_win);
            break;

         case IMAGE_LIST_SMALL_ICONS:
            image_silist_clear(IMAGE_SILIST(il->child_list));
            gtk_container_remove(GTK_CONTAINER(il), il->child_scrolled_win);
            break;

         case IMAGE_LIST_DETAILS:
         default:
            image_clist_clear(IMAGE_CLIST(il->child_list));
#ifdef GTK_HAVE_FEATURES_1_1_4
            gtk_container_remove(GTK_CONTAINER(il), il->child_scrolled_win);
#else
            gtk_container_remove(GTK_CONTAINER(il), il->child_list);
#endif
            break;
      }

      il->child_list = NULL;
   }
}

ImageInfo*
image_list_get_selected(ImageList *il)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         return IMAGE_TNLIST(il->child_list)->info;

      case IMAGE_LIST_SMALL_ICONS:
         return IMAGE_SILIST(il->child_list)->info;

      case IMAGE_LIST_DETAILS:
      default:
         return IMAGE_CLIST(il->child_list)->info;
   }
}

GList*
image_list_get_selection(ImageList *il)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         return NULL;

      case IMAGE_LIST_SMALL_ICONS:
         return NULL;

      case IMAGE_LIST_DETAILS:
      default:
         return GTK_CLIST(il->child_list)->selection;
   }
}

void
image_list_update_info(ImageList *il, ImageInfo *info)
{
   info->type_pixmap = image_type_get_pixmap(info->type);
   info->type_mask = image_type_get_mask(info->type);
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         image_tnlist_update_info(IMAGE_TNLIST(il->child_list), info);
         break;

      case IMAGE_LIST_SMALL_ICONS:
         image_silist_update_info(IMAGE_SILIST(il->child_list), info);
         break;

      case IMAGE_LIST_DETAILS:
      default:
         image_clist_update_info(IMAGE_CLIST(il->child_list), info);
         break;
   }
}

void
image_convert_info(ImageInfo *info, guchar *buffer)
{
   if (info -> valid)
   {
      /*
       * if width is not -1, the headers of the image have
       * been read. Otherwise, just give the image type
       */
      if (info -> width >=0)
      {
         if (info->real_ncolors)
         {
            sprintf(buffer, "%ix%ix%li%s %s",
               info->width, info->height,
               info->ncolors,
               (info->alpha ? "+" : ""),
               suffix_color[info->type][0] );
         } else
         {
            sprintf(buffer, "%ix%ix%s%s %s",
               info->width, info->height,
               ncolors[info->ncolors],
               (info->alpha ? "+" : ""),
               suffix_color[info->type][0] );
         }
      } else
      {
         strncpy(buffer, suffix_color[info->type][0], sizeof(buffer));
      }
   } else
   {
      *buffer = '\0';
   }
}

ImageInfo*
image_list_get_first(ImageList *il)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         return image_tnlist_get_first(IMAGE_TNLIST(il->child_list));

      case IMAGE_LIST_SMALL_ICONS:
         return image_silist_get_first(IMAGE_SILIST(il->child_list));

      case IMAGE_LIST_DETAILS:
      default:
         return image_clist_get_first(IMAGE_CLIST(il->child_list));
   }
}

ImageInfo*
image_list_get_last(ImageList *il)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         return image_tnlist_get_last(IMAGE_TNLIST(il->child_list));

      case IMAGE_LIST_SMALL_ICONS:
         return image_silist_get_last(IMAGE_SILIST(il->child_list));

      case IMAGE_LIST_DETAILS:
      default:
         return image_clist_get_last(IMAGE_CLIST(il->child_list));
   }
}

ImageInfo*
image_list_get_next(ImageList *il, ImageInfo *info)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         return image_tnlist_get_next(IMAGE_TNLIST(il->child_list), info);

      case IMAGE_LIST_SMALL_ICONS:
         return image_silist_get_next(IMAGE_SILIST(il->child_list), info);

      case IMAGE_LIST_DETAILS:
      default:
         return image_clist_get_next(IMAGE_CLIST(il->child_list), info);
   }
}

ImageInfo*
image_list_get_previous(ImageList *il, ImageInfo *info)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         return image_tnlist_get_previous(IMAGE_TNLIST(il->child_list), info);

      case IMAGE_LIST_SMALL_ICONS:
         return image_silist_get_previous(IMAGE_SILIST(il->child_list), info);

      case IMAGE_LIST_DETAILS:
      default:
         return image_clist_get_previous(IMAGE_CLIST(il->child_list), info);
   }
}

void
image_list_select_by_serial(ImageList *il, gint serial)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         image_tnlist_select_by_serial(IMAGE_TNLIST(il->child_list), serial);
         break;

      case IMAGE_LIST_SMALL_ICONS:
         image_silist_select_by_serial(IMAGE_SILIST(il->child_list), serial);
         break;

      case IMAGE_LIST_DETAILS:
      default:
         gtk_clist_select_row(GTK_CLIST(il->child_list), serial, -1);
         break;
   }
}

void
image_list_select_all(ImageList *il)
{
   int i;
   GtkCList *clist;

   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS :  break;

      case IMAGE_LIST_SMALL_ICONS:  break;

      case IMAGE_LIST_DETAILS:
      default:
               clist = GTK_CLIST(il->child_list);
               for (i = 0; i < clist->rows; i++)
               {
                  gtk_clist_select_row(clist, i, -1);
               }
               break;
   }
}

void
image_list_remove_cache(ImageList *il)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         image_tnlist_remove_cache(IMAGE_TNLIST(il->child_list));
         break;

      case IMAGE_LIST_SMALL_ICONS:
         image_silist_remove_cache(IMAGE_SILIST(il->child_list));
         break;
      case IMAGE_LIST_DETAILS:
      default:
         image_clist_remove_cache(IMAGE_CLIST(il->child_list));
         break;
   }
}

void
image_list_set_list_type(ImageList *il, ImageListType type)
{
   image_list_clear(il);
   il->list_type = type;
   image_list_construct(il);
}

void
image_list_set_sort_type(ImageList *il, ImageSortType type)
{
   rc_set_int("image_sort_type", type);
   

   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         image_tnlist_set_sort_type(IMAGE_TNLIST(il->child_list), type);
         break;

      case IMAGE_LIST_SMALL_ICONS:
         image_silist_set_sort_type(IMAGE_SILIST(il->child_list), type);
         break;

      case IMAGE_LIST_DETAILS:
      default:
         image_clist_set_sort_type(IMAGE_CLIST(il->child_list), type);
         break;
   }
}

static void
image_list_selected(GtkObject *obj)
{
   gtk_signal_emit(obj, image_list_signals[SELECT_IMAGE_SIGNAL]);
}

static void
image_list_unselected(GtkObject *obj)
{
   gtk_signal_emit(obj, image_list_signals[UNSELECT_IMAGE_SIGNAL]);
}

gint
image_cmp_default(ImageInfo *i1, ImageInfo *i2)
{
   return strcmp(i1->name, i2->name);
}

void
image_set_tooltips(GtkWidget *widget, ImageInfo *info)
{
   guchar *size, prop[256], tip[256];
   GtkTooltips *tooltips;

   size = fsize(info->size);

   if (info->type < UNKNOWN)
   {
      image_convert_info(info, prop);
      sprintf(tip, "%s (%s) - %s", info->name, prop, size);
   } else
   {
      sprintf(tip, "%s - %s", info->name, size);
   }

   g_free(size);
   tooltips = gtk_tooltips_new();

   gtk_tooltips_set_colors(tooltips,
      image_tooltips_bgcolor(widget->window, gtk_widget_get_colormap(widget)),
      &widget->style->fg[GTK_STATE_NORMAL]);

   gtk_tooltips_set_tip(tooltips, widget, tip, NULL);
}

GdkPixmap*
image_type_get_pixmap(ImageType type)
{
   return pixmaps[type];
}

GdkBitmap*
image_type_get_mask(ImageType type)
{
   return masks[type];
}

GdkColor
image_type_get_color(ImageType type)
{
   GdkColor color;

   if (type < UNKNOWN)
   {
      color.pixel   = gdkcolors[type]->pixel;
      color.red     = gdkcolors[type]->red;
      color.green   = gdkcolors[type]->green;
      color.blue    = gdkcolors[type]->blue;
   }

   return (GdkColor) color;
}

static GdkColor*
image_tooltips_bgcolor(GdkWindow *window, GdkColormap *colormap)
{
   static GdkColor color;
   static gboolean alloced = FALSE;
   if (!alloced)
   {
      color.red   = 61669;
      color.green = 59113;
      color.blue  = 35979;
      color.pixel = 0;
      alloced     = TRUE;
      gdk_color_alloc(colormap, &color);
   }
   return &color;
}

ImageInfo*
image_list_get_by_serial(ImageList *il, guint serial)
{
   switch (il->list_type)
   {
      case IMAGE_LIST_THUMBNAILS:
         return NULL;

      case IMAGE_LIST_SMALL_ICONS:
         return NULL;

      case IMAGE_LIST_DETAILS:
      default:
         return gtk_clist_get_row_data(GTK_CLIST(il->child_list), serial);
   }
}

GList*
image_get_load_file(guchar *dir,
                     gint  *nfiles,
                     glong *total_size,
                     ImageSortType sort_type,
                     TypeList type_list)
{
   DIR            *dp;
   struct dirent  *item;
   gchar          fullname[1024];
   struct stat    st;
   GList          *list;
   ImageInfo      *info;
   gint           type;
   gulong         ntotalfiles, count = 0;
   gboolean       show_hidden, hide_non_images,
                  show_bar_progress = FALSE;

   list = NULL;

   if ((dp = opendir(dir)) == NULL) return (GList *) NULL;

   *nfiles      = 0;
   *total_size  = 0;
   ntotalfiles  = 0;

   show_hidden       = rc_get_boolean("show_hidden");
   hide_non_images   = rc_get_boolean("hide_non_images");

   while ((item = readdir(dp)) != NULL)
   {
      ntotalfiles ++;
   }

   if (ntotalfiles > 30)
   {
      show_bar_progress = TRUE;
   }

   rewinddir(dp);
   if (show_bar_progress)
   {
      start_progress_data();
   }

   while ((item = readdir(dp)) != NULL)
   {
      if (show_bar_progress)
      {
         count ++;
         update_progress_data(count, ntotalfiles);
      }




      if (item -> d_ino == 0                  ||
            strcmp(item -> d_name, "." ) == 0 ||
            strcmp(item -> d_name, "..") == 0 ||
            (!show_hidden && item->d_name[0] == '.')
         )
         continue;

      strcpy(fullname, dir);
      if (fullname[strlen(fullname) - 1] != '/')
         strcat(fullname,"/");
      strcat(fullname, item -> d_name);

      if (stat(fullname, &st) != 0)
         continue;
      if ((st.st_mode & S_IFMT) != S_IFREG)
         continue;

      if (  SUFFIX(item -> d_name, ".xpm")
          || SUFFIX(item -> d_name, ".XPM"))       type = XPM;

      else if (   SUFFIX(item -> d_name, ".gif")
               || SUFFIX(item -> d_name, ".GIF"))  type = GIF;

      else if (   SUFFIX(item -> d_name, ".jpg")
               || SUFFIX(item -> d_name, ".JPG")
               || SUFFIX(item -> d_name, ".jpeg")
               || SUFFIX(item -> d_name, ".JPEG")) type = JPG;

      else if (   SUFFIX(item -> d_name, ".bmp")
               || SUFFIX(item -> d_name, ".BMP"))  type = BMP;

      else if (   SUFFIX(item -> d_name, ".ico")
               || SUFFIX(item -> d_name, ".ICO"))  type = ICO;

      else if (   SUFFIX(item -> d_name, ".pcx")
               || SUFFIX(item -> d_name, ".PCX"))  type = PCX;

      else if (   SUFFIX(item -> d_name, ".tif")
               || SUFFIX(item -> d_name, ".TIF")
               || SUFFIX(item -> d_name, ".tiff")
               || SUFFIX(item -> d_name, ".TIFF")) type = TIF;

      else if (   SUFFIX(item -> d_name, ".png")
               || SUFFIX(item -> d_name, ".PNG"))  type = PNG;

      else if (   SUFFIX(item -> d_name, ".ppm")
               || SUFFIX(item -> d_name, ".PPM")
               || SUFFIX(item -> d_name, ".pnm")
               || SUFFIX(item -> d_name, ".PNM")
               || SUFFIX(item -> d_name, ".pgm")
               || SUFFIX(item -> d_name, ".PGM")
               || SUFFIX(item -> d_name, ".pbm")
               || SUFFIX(item -> d_name, ".PBM"))  type = PNM;

      else if (   SUFFIX(item -> d_name, ".psd")
               || SUFFIX(item -> d_name, ".PSD"))  type = PSD;

      else if (   SUFFIX(item -> d_name, ".xbm")
               || SUFFIX(item -> d_name, ".XBM")
               || SUFFIX(item -> d_name, ".icon")
               || SUFFIX(item -> d_name, ".ICON")
               || SUFFIX(item -> d_name, ".bitmap")
               || SUFFIX(item -> d_name, ".BITMAP")) type = XBM;

      else if (   SUFFIX(item -> d_name, ".xcf")
               || SUFFIX(item -> d_name, ".XCF"))  type = XCF;

      else if (   SUFFIX(item -> d_name, ".tga")
               || SUFFIX(item -> d_name, ".TGA"))  type = TGA;

      else if (   SUFFIX(item -> d_name, ".xwd")
               || SUFFIX(item -> d_name, ".XWD"))  type = XWD;

      else if (   SUFFIX(item -> d_name, ".sun")
               || SUFFIX(item -> d_name, ".SUN")
               || SUFFIX(item -> d_name, ".im1")
               || SUFFIX(item -> d_name, ".IM1")
               || SUFFIX(item -> d_name, ".ras")
               || SUFFIX(item -> d_name, ".RAS"))  type = SUN;

      else if (   SUFFIX(item -> d_name, ".eps")
               || SUFFIX(item -> d_name, ".EPS")
               || SUFFIX(item -> d_name, ".ps" )
               || SUFFIX(item -> d_name, ".PS" ))  type = SUN;

      else type = UNKNOWN;

      info  = g_malloc(sizeof(ImageInfo));
      strncpy(info -> name, item -> d_name, sizeof(info->name)-1);
      info -> type         = type;
      info -> size         = st.st_size;
      info -> time         = st.st_mtime;
      info -> width        = -1;
      info -> height       = -1;
      info -> has_desc     = FALSE;
      info -> ncolors      = -1;
      info -> real_ncolors = TRUE;
      info -> alpha        = 0;
      info -> bpp          = 0;
      info -> loaded       = FALSE;
      info -> cache.buffer = NULL;
      info -> valid        = TRUE;
      /*
       * We should detect image type and setup header imformations
       */

      if (detect_image_type(fullname, info) == FALSE)
         info -> type = UNKNOWN;

      if (type_list == IS_CLIST && hide_non_images && info->width < 0)
      {
         /* Do not show this image in the list */
         g_free(info);
         continue;
      }

      if (type_list == IS_THUMB && (info->width < 0 || info->type == UNKNOWN))
      {
         /* Do not show this image in the list */
         g_free(info);
         continue;
      }

      if (type_list == IS_SILIST && hide_non_images && (info->width < 0 || info->type == UNKNOWN))
      {
         /* Do not show this image in the list */
         g_free(info);
         continue;
      }

      info -> type_pixmap  = image_type_get_pixmap(info->type);
      info -> type_mask    = image_type_get_mask(info->type);
      list                 = g_list_insert_sorted(list,
                                 info,
                                 image_list_get_sort_func(sort_type));
      *nfiles += 1;
      *total_size += info -> size;
   }

   if (show_bar_progress)
   {
      stop_progress_data();
   }

   g_free(item);
   closedir(dp);

   return (GList *) list;
}


/* Sorting section for every click in any column */
GCompareFunc
image_list_get_sort_func(ImageSortType type)
{
   switch (type)
   {
      case IMAGE_SORT_ASCEND_BY_TYPE:
         return (GCompareFunc)image_cmp_ascend_by_type;

      case IMAGE_SORT_DESCEND_BY_TYPE:
         return (GCompareFunc)image_cmp_descend_by_type;

      case IMAGE_SORT_ASCEND_BY_SIZE:
         return (GCompareFunc)image_cmp_ascend_by_size;

      case IMAGE_SORT_DESCEND_BY_SIZE:
         return (GCompareFunc)image_cmp_descend_by_size;

      case IMAGE_SORT_ASCEND_BY_PROPERTY:
         return (GCompareFunc)image_cmp_ascend_by_property;

      case IMAGE_SORT_DESCEND_BY_PROPERTY:
         return (GCompareFunc)image_cmp_descend_by_property;

      case IMAGE_SORT_ASCEND_BY_DATE:
         return (GCompareFunc)image_cmp_ascend_by_date;

      case IMAGE_SORT_DESCEND_BY_DATE:
         return (GCompareFunc)image_cmp_descend_by_date;

      case IMAGE_SORT_DESCEND_BY_NAME:
         return (GCompareFunc)image_cmp_descend_by_name;

      case IMAGE_SORT_ASCEND_BY_NAME:
      default:
         return (GCompareFunc)image_cmp_ascend_by_name;
   }
}

gint
image_cmp_ascend_by_name(ImageInfo *i1, ImageInfo *i2)
{
   return strcmp(i1->name, i2->name);
}

gint
image_cmp_descend_by_name(ImageInfo *i1, ImageInfo *i2)
{
   return strcmp(i2->name, i1->name);
}

gint
image_cmp_ascend_by_size(ImageInfo *i1, ImageInfo *i2)
{
   if (i1->size > i2->size) return 1;
   if (i1->size < i2->size) return -1;
   return 0;
}

gint
image_cmp_descend_by_size(ImageInfo *i1, ImageInfo *i2)
{
   if (i2->size > i1->size) return 1;
   if (i2->size < i1->size) return -1;
   return 0;
}

gint
image_cmp_ascend_by_property(ImageInfo *i1, ImageInfo *i2)
{
   glong ncolors1, ncolors2;

   ncolors1 = i1->alpha + ((i1->real_ncolors) ? i1->ncolors : (1L << i1->ncolors));
   ncolors2 = i2->alpha + ((i2->real_ncolors) ? i2->ncolors : (1L << i2->ncolors));

   if (ncolors1 > ncolors2) return 1;
   if (ncolors1 < ncolors2) return -1;

   if (i1->width * i1->height > i2->width * i2->height) return 1;
   if (i1->width * i1->height < i2->width * i2->height) return -1;

   return 0;
}

gint
image_cmp_descend_by_property(ImageInfo *i1, ImageInfo *i2)
{
   glong ncolors1, ncolors2;

   ncolors1 = i1->alpha + ((i1->real_ncolors) ? i1->ncolors : (1L << i1->ncolors));
   ncolors2 = i2->alpha + ((i2->real_ncolors) ? i2->ncolors : (1L << i2->ncolors));

   if (ncolors2 > ncolors1) return 1;
   if (ncolors2 < ncolors1) return -1;

   if (i2->width * i2->height > i1->width * i1->height) return 1;
   if (i2->width * i2->height < i1->width * i1->height) return -1;

   return 0;
}

gint
image_cmp_ascend_by_date(ImageInfo *i1, ImageInfo *i2)
{
   if (i1->time < i2->time) return -1;
   if (i1->time > i2->time) return 1;

   return 0;
}

gint
image_cmp_descend_by_date(ImageInfo *i1, ImageInfo *i2)
{
   if (i1->time < i2->time) return 1;
   if (i1->time > i2->time) return -1;

   return 0;
}

gint
image_cmp_ascend_by_type(ImageInfo *i1, ImageInfo *i2)
{
   return strcmp(suffix_color[i1->type][0], suffix_color[i2->type][0]);
}

gint
image_cmp_descend_by_type(ImageInfo *i1, ImageInfo *i2)
{
   return strcmp(suffix_color[i2->type][0], suffix_color[i1->type][0]);
}

guchar*
image_type_get_type(ImageType type)
{
   static guchar the_type[20];

   if (type < UNKNOWN)
   {
      snprintf(the_type,
                  sizeof(the_type),
                  "%s Image",
                  suffix_color[type][0]);
   } else
   {
      snprintf(the_type,
                  sizeof(the_type),
                  "Unk Image");
   }

   return (guchar *) the_type;
}
