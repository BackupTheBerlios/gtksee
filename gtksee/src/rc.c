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
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "util.h"
#include "rc.h"

static gboolean rc_initialized = FALSE;
static guchar gtksee_dir[128];
static guchar gtkrc_file[128];
static guchar gtkseerc_file[128];
static GHashTable *rc_conf = NULL;

static gint rc_key_compare_func  (gpointer a, gpointer b);
static void rc_save_conf      (guchar *key, guchar *value, FILE *file);
static void rc_load_gtkrc     ();
static void rc_load_gtkseerc  ();

void
rc_init()
{
   guchar *home;
   struct stat st;
   FILE *file;

   /*
    * Setting up gtksee_dir.
    * Should be $HOME/.gtksee
    */
   if ((home = getenv("HOME")) == NULL)
   {
      rc_initialized = FALSE;
      return;
   }
   strcpy(gtksee_dir, home);
   if (gtksee_dir[strlen(gtksee_dir) - 1] != '/') strcat(gtksee_dir, "/");
   strcat(gtksee_dir, ".gtksee");
   if (stat(gtksee_dir, &st))
   {
      /* Cannot stat gtksee_dir.
       * Try to create a new one.
       */
      if (mkdir(gtksee_dir, 00755))
      {
         rc_initialized = FALSE;
         return;
      }
      g_print("Created directory: %s\n", gtksee_dir);
   }

   /*
    * Setting up gtkrc_file.
    * Should be $gtksee_dir/gtkrc
    */
   strcpy(gtkrc_file, gtksee_dir);
   strcat(gtkrc_file, "/gtkrc");
   if (stat(gtkrc_file, &st))
   {
      /* Cannot stat gtkrc file.
       * Try to create a new one.
       */
      if ((file = fopen(gtkrc_file, "w")) == NULL)
      {
         rc_initialized = FALSE;
         return;
      }
      /* Writing contents here for gtkrc */

      fclose(file);
      g_print("Created file: %s\n", gtkrc_file);
   } else
   {
      rc_load_gtkrc();
   }

   /*
    * Setting up gtkseerc_file.
    * Should be $gtksee_dir/gtkseerc
    */
   strcpy(gtkseerc_file, gtksee_dir);
   strcat(gtkseerc_file, "/gtkseerc");
   rc_conf = g_hash_table_new(
      (GHashFunc)strtohash,
      (GCompareFunc)rc_key_compare_func);
   if (stat(gtkseerc_file, &st))
   {
      /* Cannot stat gtkseerc file.
       * Try to create a new one.
       */
      if ((file = fopen(gtkseerc_file, "w")) == NULL)
      {
         rc_initialized = FALSE;
         return;
      }
      fclose(file);
      /* Setting default values */
      rc_set_int("slideshow_delay", 5000);
      rc_set_boolean("full_screen", FALSE);
      rc_set_boolean("fit_screen", FALSE);
      rc_set_boolean("show_hidden", FALSE);
      rc_set_boolean("hide_non_images", FALSE);
      rc_set_boolean("fast_preview", FALSE);
      rc_set_int("image_list_type", 0);
      rc_set_int("image_sort_type", 0);
      rc_set("root_directory", "/");
      rc_set("hidden_directory", "/dev:/proc:.xvpics");
      rc_save_gtkseerc();
      g_print("Created file: %s\n", gtkseerc_file);
   } else
   {
      rc_load_gtkseerc();
   }

   rc_initialized = TRUE;
}

static void
rc_load_gtkrc()
{
   gtk_rc_parse(gtkrc_file);
}

void
rc_set(guchar *key, guchar *value)
{
   g_hash_table_insert(rc_conf, g_strdup(key), g_strdup(value));
}

void
rc_set_boolean(guchar *key, gboolean value)
{
   rc_set(key, value ? "1" : "0");
}

void
rc_set_int(guchar *key, gint value)
{
   guchar buf[20];

   sprintf(buf, "%i", value);
   rc_set(key, g_strdup(buf));
}

guchar*
rc_get(guchar *key)
{
   return (guchar*)g_hash_table_lookup(rc_conf, key);
}

gboolean
rc_get_boolean(guchar *key)
{
   guchar *value = rc_get(key);
   if (value == NULL) return FALSE;
   if (value[0] != '1') return FALSE;
   return TRUE;
}

gint
rc_get_int(guchar *key)
{
   guchar *value = rc_get(key);
   if (value == NULL) return RC_NOT_EXISTS;
   return atoi(value);
}

static void
rc_load_gtkseerc()
{
   FILE *file;
   guchar line[256], *value;
   gint len;

   if ((file = fopen(gtkseerc_file, "r")) == NULL) return;
   while (fgets(line, 255, file) != NULL)
   {
      if (line[0] == '#') continue;
      len = strlen(line);
      if (len < 4) continue;
      if (line[len - 1] == '\n')
      {
         line[len - 1] = '\0';
      }
      value = strchr(line, '=');
      if (value == NULL) continue;
      *value++ = '\0';
      rc_set(line, value);
   }
   fclose(file);
}

void
rc_save_gtkseerc()
{
   FILE *file;

   if ((file = fopen(gtkseerc_file, "w")) == NULL) return;
   fprintf(file, "# GTK See configuration file\n");
   fprintf(file, "#\n");
   fprintf(file, "# DO NOT modify by hand unless you know what you are doing!\n");
   fprintf(file, "#\n");
   g_hash_table_foreach(rc_conf, (GHFunc)rc_save_conf, file);
   fclose(file);
}

static gint
rc_key_compare_func(gpointer a, gpointer b)
{
   return (strcmp((guchar *)a, (guchar *)b) == 0);
}

static void
rc_save_conf(guchar *key, guchar *value, FILE *file)
{
   fprintf(file, "%s=%s\n", key, value);
}
