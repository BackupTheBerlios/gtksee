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

/*
 * 2003-09-12: - Changed reading of directory
 *             - Little improved of code
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "intl.h"
#include "gtypes.h"
#include "util.h"
#include "detect.h"
#include "rc.h"

#include "imagelist.h"
#include "imageclist.h"

static const gchar *normal_titles[MAX_TITLES] = {  N_("Name "),
                                                   N_("Size "),
                                                   N_("Image Properties "),
                                                   N_("Type ")};

#ifdef GTK_HAVE_FEATURES_1_1_4
   static GtkTargetEntry image_clist_target_table[] = {{ "STRING", 0, 0 }};
#endif

static void image_clist_class_init  (ImageCListClass *class);
static void image_clist_init        (ImageCList *il);
static void image_clist_selected    (ImageCList *il,
                                       gint row, gint column,
                                       GdkEventButton *event,
                                       gpointer data);
static void image_clist_unselected  (ImageCList *il,
                                       gint row, gint column,
                                       GdkEventButton *event,
                                       gpointer data);
static void image_clist_resort      (ImageCList *il);
static void image_clist_click_column(ImageCList *il, gint column);
static void image_clist_set_titles  (ImageCList *il);
static void image_clist_imprime(GtkWidget *il, GdkEventKey *event);

#ifdef GTK_HAVE_FEATURES_1_1_4
static void image_clist_drag_data_get  (GtkWidget *widget,
                   GdkDragContext *context,
                   GtkSelectionData *selection_data,
                   guint info,
                   guint time,
                   gpointer data);
#endif

static void
image_clist_imprime(GtkWidget *il, GdkEventKey *event)
{
   switch(event -> keyval)
   {
      case GDK_Delete   : menu_edit_delete(il, NULL);
                          break;

      default           : break;
   }
}

guint
image_clist_get_type()
{
   static guint il_type = 0;

   if (!il_type)
   {
      GtkTypeInfo il_info  = {"ImageCList",
                              sizeof (ImageCList),
                              sizeof (ImageCListClass),
                              (GtkClassInitFunc) image_clist_class_init,
                              (GtkObjectInitFunc) image_clist_init,
                              (GtkArgSetFunc) NULL,
                              (GtkArgGetFunc) NULL
                             };
      il_type = gtk_type_unique (gtk_clist_get_type (), &il_info);
   }

   return il_type;
}

static void
image_clist_class_init(ImageCListClass *class)
{
}

static void
image_clist_init(ImageCList *il)
{
   int t;

   il -> dir [0]           = '\0';
   il -> selected_item [0] = '\0';
   t = rc_get_int("image_sort_type");
   il -> sort_type   = (t == RC_NOT_EXISTS ? IMAGE_SORT_ASCEND_BY_NAME : t);
   il -> info        = NULL;
   il -> nfiles      = 0;
   il -> total_size  = 0;
}

#ifdef GTK_HAVE_FEATURES_1_1_4
static void
image_clist_drag_data_get(
   GtkWidget *widget,
   GdkDragContext *context,
   GtkSelectionData *selection_data,
   guint info,
   guint time,
   gpointer data)
{
   ImageInfo *iminfo;
   GtkCList *clist = GTK_CLIST(widget);

   if (clist->click_cell.column < 0) return;

   iminfo = gtk_clist_get_row_data(clist, clist->click_cell.row);
   gtk_selection_data_set(selection_data, selection_data->target, 8, iminfo->name, strlen(iminfo->name));
}
#endif

GtkWidget*
image_clist_new()
{
   ImageCList *il;
#ifdef GTK_HAVE_FEATURES_1_1_4
   GtkWidget *scrolled_win;
#endif
   gint i;

   il = IMAGE_CLIST(gtk_type_new(image_clist_get_type()));

   for (i = 0; i < MAX_TITLES; i++)
   {
      il -> titles[i] = g_strdup(_(normal_titles[i]));
   }

   gtk_clist_construct(GTK_CLIST(il), MAX_TITLES, (gchar**) il -> titles);

   image_clist_set_titles(il);
#ifdef GTK_HAVE_FEATURES_1_1_0
   gtk_clist_set_selection_mode(GTK_CLIST(il), GTK_SELECTION_EXTENDED);
#else
   gtk_clist_set_selection_mode(GTK_CLIST(il), GTK_SELECTION_MULTIPLE);
#endif

#ifdef GTK_HAVE_FEATURES_1_1_4
   scrolled_win = gtk_scrolled_window_new (NULL, NULL);

        /* DND support: drag site */
   gtk_drag_source_set(GTK_WIDGET(il), GDK_BUTTON1_MASK,
                        image_clist_target_table, 1,
                        GDK_ACTION_COPY | GDK_ACTION_MOVE);
   gtk_signal_connect(GTK_OBJECT(il), "drag_data_get",
                        GTK_SIGNAL_FUNC(image_clist_drag_data_get),
                        NULL);

   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
   gtk_widget_show(GTK_WIDGET(il));
   gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET(il));
#else
   gtk_clist_set_policy(GTK_CLIST(il), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#endif

   gtk_clist_set_column_width(GTK_CLIST(il), 0, 140);
   gtk_clist_set_column_auto_resize(GTK_CLIST(il), 1, 1);
   gtk_clist_set_column_auto_resize(GTK_CLIST(il), 2, 1);
   gtk_clist_set_column_width(GTK_CLIST(il), 3, 80);
   gtk_clist_set_row_height(GTK_CLIST(il), 16);
   gtk_clist_set_column_justification(GTK_CLIST(il), 1, GTK_JUSTIFY_RIGHT);
   gtk_clist_set_column_justification(GTK_CLIST(il), 2, GTK_JUSTIFY_RIGHT);
   gtk_signal_connect(GTK_OBJECT(il),
                        "select_row",
                        GTK_SIGNAL_FUNC(image_clist_selected),
                        NULL);
   gtk_signal_connect(GTK_OBJECT(il),
                        "unselect_row",
                        GTK_SIGNAL_FUNC(image_clist_unselected),
                        NULL);
   gtk_signal_connect(GTK_OBJECT(il),
                        "click_column",
                        GTK_SIGNAL_FUNC(image_clist_click_column),
                        NULL);
/* Ojo!! Esta funcion debe ser controlada */
   gtk_signal_connect(GTK_OBJECT(il),
                        "key_press_event",
                        GTK_SIGNAL_FUNC(image_clist_imprime),
                        NULL);

#ifdef GTK_HAVE_FEATURES_1_1_4
   return scrolled_win;
#else
   return GTK_WIDGET(il);
#endif
}

void
image_clist_set_dir(ImageCList *il, guchar *dir)
{
   strcpy(il -> dir, dir);
   image_clist_refresh(il);
}

void
image_clist_refresh(ImageCList *il)
{
   GList          *list, *t_pos;
   gchar          *clist_item[MAX_TITLES];
   ImageInfo      *info;
   gint           row;
   GdkColor       color;
   gchar          prop[256];

   if (strlen(il->dir) < 1) return;

   image_clist_clear(il);

   list = image_get_load_file(il->dir, &il->nfiles, &il->total_size,
                                 il->sort_type, IS_CLIST);

   gtk_clist_freeze(GTK_CLIST(il));
   t_pos = g_list_first(list);

   while (t_pos != NULL)
   {
      info           = t_pos -> data;
      clist_item[0]  = NULL;
      clist_item[1]  = fnumber(info->size);
      image_convert_info(info, prop);
      clist_item[2]  = g_strdup(prop);
      clist_item[3]  = g_strdup(image_type_get_type(info->type));

      row            = gtk_clist_append(GTK_CLIST(il), (char **)clist_item);
      info->serial   = row;

      gtk_clist_set_pixtext(GTK_CLIST(il), row, 0,
         info->name, 1,
         image_type_get_pixmap(info->type),
         image_type_get_mask(info->type));

      gtk_clist_set_row_data(GTK_CLIST(il), row, info);

      if (info -> valid && info -> width >=0 && info -> type < UNKNOWN)
      {
         //image_type_get_color(info->type, &color);
         color = image_type_get_color(info->type);
         gtk_clist_set_background(GTK_CLIST(il), row, &color);
      }
      t_pos = g_list_remove_link(t_pos, t_pos);
   }
   gtk_clist_thaw(GTK_CLIST(il));
}

static void
image_clist_selected(
   ImageCList *il,
   gint row,
   gint column,
   GdkEventButton *event,
   gpointer data)
{
   ImageInfo *info;

#ifdef GTK_HAVE_FEATURES_1_1_0
   if (GTK_CLIST(il)->focus_row == row)
   {
      info = (ImageInfo *)gtk_clist_get_row_data(GTK_CLIST(il), row);
      strcpy(il -> selected_item, info -> name);
      il -> info = info;
   } else
   {
      il->selected_item[0] = '\0';
      il->info = NULL;
   }
#else
   info = (ImageInfo *)gtk_clist_get_row_data(GTK_CLIST(il), row);
   strcpy(il -> selected_item, info -> name);
   il -> info = info;
#endif
}

static void
image_clist_unselected(
   ImageCList *il,
   gint row,
   gint column,
   GdkEventButton *event,
   gpointer data)
{
   il->selected_item[0] = '\0';
   il->info = NULL;
}

void
image_clist_clear(ImageCList *il)
{
   ImageInfo *info;
   gint i;

   for (i = 0; i < GTK_CLIST(il)->rows; i++)
   {
      info = gtk_clist_get_row_data(GTK_CLIST(il), i);
      if (info->loaded && info->cache.buffer != NULL)
         g_free(info->cache.buffer);

      if (info->has_desc)
         g_free(info->desc);

      g_free(info);
   }

   gtk_clist_clear(GTK_CLIST(il));
   il->selected_item[0] = '\0';
   il->info = NULL;
}

void
image_clist_set_sort_type(ImageCList *il, ImageSortType type)
{
   il -> sort_type = type;
   image_clist_set_titles(il);
   image_clist_resort(il);
}

static void
image_clist_resort(ImageCList *il)
{
   GList       *list, *t_pos;
   gint        row, counter, sel_row;
   ImageInfo   *info, *selected;
   gchar       *clist_item[MAX_TITLES];
   GdkColor    color;
   gchar       prop[256];

   if (GTK_CLIST(il) -> rows < 2) return;

   if (GTK_CLIST(il) -> selection == NULL)
   {
      selected = NULL;
   } else
   {
      row = (gint) GTK_CLIST(il) -> selection -> data;
      selected = (ImageInfo *)gtk_clist_get_row_data(GTK_CLIST(il), row);
   }

   gtk_clist_freeze(GTK_CLIST(il));
   list = NULL;

   for (row = 0; row < GTK_CLIST(il) -> rows; row++)
   {
      info = (ImageInfo *)gtk_clist_get_row_data(GTK_CLIST(il), row);
      list = g_list_insert_sorted(list, info, image_list_get_sort_func(il->sort_type));
   }

   gtk_clist_clear(GTK_CLIST(il));
   t_pos    = g_list_first(list);
   sel_row  = -1;
   counter  = 0;

   while (t_pos != NULL)
   {
      info = t_pos -> data;
      if (selected != NULL && selected == info)
         sel_row = counter;

      clist_item[0] = NULL;
      clist_item[1] = fnumber(info->size);
      image_convert_info(info, prop);
      clist_item[2] = g_strdup(prop);
      clist_item[3] = g_strdup(image_type_get_type(info->type));

      row            = gtk_clist_append(GTK_CLIST(il), (char **)clist_item);
      info->serial   = row;
      gtk_clist_set_pixtext(GTK_CLIST(il), row, 0,
                              info->name, 1,
                              image_type_get_pixmap(info->type),
                              image_type_get_mask(info->type)
      );

      gtk_clist_set_row_data(GTK_CLIST(il), row, info);

      if (info -> valid && info -> width >=0 && info -> type < UNKNOWN)
      {
         //image_type_get_color(info->type, &color);
         color = image_type_get_color(info->type);
         gtk_clist_set_background(GTK_CLIST(il), row, &color);
      }

      t_pos = t_pos -> next;
      counter++;
   }

   gtk_clist_thaw(GTK_CLIST(il));

   if (sel_row >=0)
   {
      gtk_clist_select_row(GTK_CLIST(il), sel_row, -1);
      /* To be fixed: weird problem with GtkClist using this here */
      /*gtk_clist_moveto(GTK_CLIST(il), sel_row, 0, 0.5, 0.0);*/
   }
}

static void
image_clist_click_column(ImageCList *il, gint column)
{
   gint new_sort_type;

   switch (column)
   {
      case 0: /* clicked on name label */
         if (il->sort_type == IMAGE_SORT_ASCEND_BY_NAME)
            new_sort_type = IMAGE_SORT_DESCEND_BY_NAME;
         else
            new_sort_type = IMAGE_SORT_ASCEND_BY_NAME;
         break;

      case 1: /* clicked on size label */
         if (il->sort_type == IMAGE_SORT_ASCEND_BY_SIZE)
            new_sort_type = IMAGE_SORT_DESCEND_BY_SIZE;
         else
            new_sort_type = IMAGE_SORT_ASCEND_BY_SIZE;
         break;

      case 2: /* clicked on property label */
         if (il->sort_type == IMAGE_SORT_ASCEND_BY_PROPERTY)
            new_sort_type = IMAGE_SORT_DESCEND_BY_PROPERTY;
         else
            new_sort_type = IMAGE_SORT_ASCEND_BY_PROPERTY;
         break;

      case 3: /* clicked on type label */
         if (il->sort_type == IMAGE_SORT_ASCEND_BY_TYPE)
            new_sort_type = IMAGE_SORT_DESCEND_BY_TYPE;
         else
            new_sort_type = IMAGE_SORT_ASCEND_BY_TYPE;
         break;

      default:
         new_sort_type = IMAGE_SORT_ASCEND_BY_NAME;
   }

   image_clist_set_sort_type(il, new_sort_type);
}

static void
image_clist_set_titles(ImageCList *il)
{
   gint i;
   switch (il -> sort_type)
   {
      case IMAGE_SORT_ASCEND_BY_TYPE:
         il->titles[0][strlen(il->titles[0])-1] = ' ';
         il->titles[1][strlen(il->titles[1])-1] = ' ';
         il->titles[2][strlen(il->titles[2])-1] = ' ';
         il->titles[3][strlen(il->titles[3])-1] = '+';
         break;

      case IMAGE_SORT_DESCEND_BY_TYPE:
         il->titles[0][strlen(il->titles[0])-1] = ' ';
         il->titles[1][strlen(il->titles[1])-1] = ' ';
         il->titles[2][strlen(il->titles[2])-1] = ' ';
         il->titles[3][strlen(il->titles[3])-1] = '-';
         break;

      case IMAGE_SORT_ASCEND_BY_SIZE:
         il->titles[0][strlen(il->titles[0])-1] = ' ';
         il->titles[1][strlen(il->titles[1])-1] = '+';
         il->titles[2][strlen(il->titles[2])-1] = ' ';
         il->titles[3][strlen(il->titles[3])-1] = ' ';
         break;

      case IMAGE_SORT_DESCEND_BY_SIZE:
         il->titles[0][strlen(il->titles[0])-1] = ' ';
         il->titles[1][strlen(il->titles[1])-1] = '-';
         il->titles[2][strlen(il->titles[2])-1] = ' ';
         il->titles[3][strlen(il->titles[3])-1] = ' ';
         break;

      case IMAGE_SORT_ASCEND_BY_PROPERTY:
         il->titles[0][strlen(il->titles[0])-1] = ' ';
         il->titles[1][strlen(il->titles[1])-1] = ' ';
         il->titles[2][strlen(il->titles[2])-1] = '+';
         il->titles[3][strlen(il->titles[3])-1] = ' ';
         break;

      case IMAGE_SORT_DESCEND_BY_PROPERTY:
         il->titles[0][strlen(il->titles[0])-1] = ' ';
         il->titles[1][strlen(il->titles[1])-1] = ' ';
         il->titles[2][strlen(il->titles[2])-1] = '-';
         il->titles[3][strlen(il->titles[3])-1] = ' ';
         break;

      case IMAGE_SORT_ASCEND_BY_DATE:
         il->titles[0][strlen(il->titles[0])-1] = ' ';
         il->titles[1][strlen(il->titles[1])-1] = ' ';
         il->titles[2][strlen(il->titles[2])-1] = ' ';
         il->titles[3][strlen(il->titles[3])-1] = ' ';
         break;

      case IMAGE_SORT_DESCEND_BY_DATE:
         il->titles[0][strlen(il->titles[0])-1] = ' ';
         il->titles[1][strlen(il->titles[1])-1] = ' ';
         il->titles[2][strlen(il->titles[2])-1] = ' ';
         il->titles[3][strlen(il->titles[3])-1] = ' ';
         break;

      case IMAGE_SORT_DESCEND_BY_NAME:
         il->titles[0][strlen(il->titles[0])-1] = '-';
         il->titles[1][strlen(il->titles[1])-1] = ' ';
         il->titles[2][strlen(il->titles[2])-1] = ' ';
         il->titles[3][strlen(il->titles[3])-1] = ' ';
         break;

      case IMAGE_SORT_ASCEND_BY_NAME:
      default:
         il->titles[0][strlen(il->titles[0])-1] = '+';
         il->titles[1][strlen(il->titles[1])-1] = ' ';
         il->titles[2][strlen(il->titles[2])-1] = ' ';
         il->titles[3][strlen(il->titles[3])-1] = ' ';
         break;
   }

   for (i = 0; i < MAX_TITLES; i++)
   {
      gtk_clist_set_column_title(GTK_CLIST(il), i, il->titles[i]);
   }
}

void
image_clist_update_info(ImageCList *il, ImageInfo *info)
{
   GdkColor color;
   guchar buffer[30];

   gtk_clist_set_pixtext(GTK_CLIST(il), info->serial, 0,
      info->name, 1,
      info->type_pixmap, info->type_mask
   );
   image_convert_info(info, buffer);
   gtk_clist_set_text(GTK_CLIST(il), info->serial, 2, buffer);
   if (info -> valid && info -> width >=0 && info -> type < UNKNOWN)
   {
      //image_type_get_color(info->type, &color);
      color = image_type_get_color(info->type);
      gtk_clist_set_background(GTK_CLIST(il), info->serial, &color);
   }
}

ImageInfo*
image_clist_get_first(ImageCList *il)
{
   gint i;
   ImageInfo *info;

   for (i = 0; i < GTK_CLIST(il)->rows; i++)
   {
      info = gtk_clist_get_row_data(GTK_CLIST(il), i);
      if (info->valid && info->width > 0) return info;
   }
   return NULL;
}

ImageInfo*
image_clist_get_last(ImageCList *il)
{
   gint i;
   ImageInfo *info;

   for (i = GTK_CLIST(il)->rows - 1; i >= 0; i--)
   {
      info = gtk_clist_get_row_data(GTK_CLIST(il), i);
      if (info->valid && info->width > 0) return info;
   }
   return NULL;
}

ImageInfo*
image_clist_get_next(ImageCList *il, ImageInfo *info)
{
   gint i;
   ImageInfo *nextinfo;

   for (i = info->serial+1; i < GTK_CLIST(il)->rows; i++)
   {
      nextinfo = gtk_clist_get_row_data(GTK_CLIST(il), i);
      if (nextinfo->valid && nextinfo->width > 0) return nextinfo;
   }
   return NULL;
}

ImageInfo*
image_clist_get_previous(ImageCList *il, ImageInfo *info)
{
   gint i;
   ImageInfo *previnfo;

   for (i = info->serial-1; i >= 0; i--)
   {
      previnfo = gtk_clist_get_row_data(GTK_CLIST(il), i);
      if (previnfo->valid && previnfo->width > 0) return previnfo;
   }
   return NULL;
}

void
image_clist_remove_cache(ImageCList *il)
{
   gint i;
   ImageInfo *info;

   for (i = 0; i < GTK_CLIST(il)->rows; i++)
   {
      info = gtk_clist_get_row_data(GTK_CLIST(il), i);
      if (info->loaded && info->cache.buffer != NULL)
         g_free(info->cache.buffer);
      info->loaded = FALSE;
   }
}
