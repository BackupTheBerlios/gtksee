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
  * 2003-09-12: Added sort option "By Type"
  */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "gtypes.h"
#include "common_tools.h"
#include "rc.h"
#include "options.h"
#include "gtkseemenu.h"
#include "gtksee.h"

static GtkWidget *thumbnails_item, *small_icons_item, *details_item;

static GtkWidget*
#ifdef GTK_HAVE_FEATURES_1_1_0
create_menu(GtkMenuBar *menubar, guchar *label, GtkAccelGroup *accel)
#else
create_menu(GtkMenuBar *menubar, guchar *label, GtkAcceleratorTable *accel)
#endif
{
   GtkWidget *menu, *menu_item;

   menu = gtk_menu_new();

   if (accel != NULL)
   {
#ifdef GTK_HAVE_FEATURES_1_1_0
      gtk_menu_set_accel_group(GTK_MENU(menu), accel);
#else
      gtk_menu_set_accelerator_table(GTK_MENU(menu), accel);
#endif
   }

#ifdef GTK_HAVE_FEATURES_1_1_2
   menu_item = gtk_tearoff_menu_item_new();
   gtk_widget_show(menu_item);
   gtk_menu_append(GTK_MENU(menu), menu_item);
#endif

   menu_item = gtk_menu_item_new_with_label(label);
   gtk_widget_show(menu_item);
   gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
   gtk_menu_bar_append(menubar, menu_item);

   return menu;
}

static GtkWidget*
#ifdef GTK_HAVE_FEATURES_1_1_0
create_menu_item(GtkMenu *menu, guchar *label, GtkSignalFunc func, GtkAccelGroup *accel, guint key, guint mods)
#else
create_menu_item(GtkMenu *menu, guchar *label, GtkSignalFunc func, GtkAcceleratorTable *accel, gchar key, guint mods)
#endif
{
   GtkWidget *menu_item;

   if (label == NULL)
   {
      menu_item = gtk_menu_item_new();
   } else
   {
      menu_item = gtk_menu_item_new_with_label(label);
   }
   gtk_menu_append(menu, menu_item);
   if (func != NULL)
   {
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate", func, NULL);
   }
   if (accel != NULL)
   {
#ifdef GTK_HAVE_FEATURES_1_1_0
      gtk_widget_add_accelerator(menu_item, "activate",
         accel, key, mods, GTK_ACCEL_VISIBLE);
#else
      gtk_widget_install_accelerator(menu_item, accel, "activate",
         key, mods);
#endif
   }
   gtk_widget_show(menu_item);
   return menu_item;
}

GtkWidget*
get_main_menu(GtkWidget *window)
{
   GtkWidget *menubar, *menu, *menu_item, *submenu;
   GSList *group;

#ifdef GTK_HAVE_FEATURES_1_1_0
   GtkAccelGroup *accel;

   accel = gtk_accel_group_new();
   gtk_accel_group_attach(accel, GTK_OBJECT(window));
#else
   GtkAcceleratorTable *accel;

   accel = gtk_accelerator_table_new();
   gtk_window_add_accelerator_table(GTK_WINDOW(window), accel);
#endif

   menubar = gtk_menu_bar_new();
   gtk_widget_show(menubar);

   /* File menu */
   menu = create_menu(GTK_MENU_BAR(menubar), _("File"), accel);

   create_menu_item(GTK_MENU(menu), _("View"),
      GTK_SIGNAL_FUNC(menu_file_view),
      accel,
#ifdef GTK_HAVE_FEATURES_1_1_0
      GDK_Return,
#else
      'V',
#endif
      0);

   create_menu_item(GTK_MENU(menu), _("Full-view"),
      GTK_SIGNAL_FUNC(menu_file_full_view),
      accel,
#ifdef GTK_HAVE_FEATURES_1_1_0
      GDK_Return,
#else
      'V',
#endif
      GDK_SHIFT_MASK);

   create_menu_item(GTK_MENU(menu), NULL, NULL, NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("Quit"),
      GTK_SIGNAL_FUNC(menu_file_quit),
      accel, GDK_X, GDK_MOD1_MASK);

   /* Edit menu */
   menu = create_menu(GTK_MENU_BAR(menubar), _("Edit"), accel);

   create_menu_item(GTK_MENU(menu), _("Cut"),
      GTK_SIGNAL_FUNC(menu_edit_cut),
      accel, GDK_X, GDK_CONTROL_MASK);

   create_menu_item(GTK_MENU(menu), _("Copy"),
      GTK_SIGNAL_FUNC(menu_edit_copy),
      accel, GDK_C, GDK_CONTROL_MASK);

   create_menu_item(GTK_MENU(menu), _("Paste"),
      GTK_SIGNAL_FUNC(menu_edit_paste),
      accel, GDK_V, GDK_CONTROL_MASK);

   create_menu_item(GTK_MENU(menu), _("Paste hard-link"),
      GTK_SIGNAL_FUNC(menu_edit_paste_link),
      accel, GDK_B, GDK_CONTROL_MASK);

   create_menu_item(GTK_MENU(menu), NULL, NULL, NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("Rename"),
      GTK_SIGNAL_FUNC(menu_edit_rename),
      accel, GDK_R, GDK_CONTROL_MASK);

   create_menu_item(GTK_MENU(menu), _("Delete"),
      GTK_SIGNAL_FUNC(menu_edit_delete),
      NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), NULL, NULL, NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("Select all"),
      GTK_SIGNAL_FUNC(menu_edit_select_all),
      accel, GDK_A, GDK_CONTROL_MASK);

   /* View menu */
   menu = create_menu(GTK_MENU_BAR(menubar), _("View"), accel);

   thumbnails_item = gtk_radio_menu_item_new_with_label(NULL, _("Thumbnails"));
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(thumbnails_item));
   gtk_menu_append(GTK_MENU(menu), thumbnails_item);
#ifdef GTK_HAVE_FEATURES_1_1_0
   gtk_widget_add_accelerator(thumbnails_item, "activate",
      accel, GDK_F8, 0, GTK_ACCEL_VISIBLE);
#else
   gtk_widget_install_accelerator(thumbnails_item, accel, "activate",
      '8', 0);
#endif
   gtk_widget_show(thumbnails_item);

   small_icons_item = gtk_radio_menu_item_new_with_label(group, _("Small icons"));
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(small_icons_item));
   gtk_menu_append(GTK_MENU(menu), small_icons_item);
#ifdef GTK_HAVE_FEATURES_1_1_0
   gtk_widget_add_accelerator(small_icons_item, "activate",
      accel, GDK_F9, 0, GTK_ACCEL_VISIBLE);
#else
   gtk_widget_install_accelerator(small_icons_item, accel, "activate",
      '9', 0);
#endif
   gtk_widget_show(small_icons_item);

   details_item = gtk_radio_menu_item_new_with_label(group, _("Details"));
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(details_item));
   gtk_menu_append(GTK_MENU(menu), details_item);
#ifdef GTK_HAVE_FEATURES_1_1_0
   gtk_widget_add_accelerator(details_item, "activate",
      accel, GDK_F10, 0, GTK_ACCEL_VISIBLE);
#else
   gtk_widget_install_accelerator(details_item, accel, "activate",
      '0', 0);
#endif
   gtk_widget_show(details_item);

   menu_set_list_type(rc_get_int("image_list_type"));

   gtk_signal_connect(GTK_OBJECT(thumbnails_item), "toggled",
      GTK_SIGNAL_FUNC(menu_view_thumbnails), NULL);
   gtk_signal_connect(GTK_OBJECT(small_icons_item), "toggled",
      GTK_SIGNAL_FUNC(menu_view_small_icons), NULL);
   gtk_signal_connect(GTK_OBJECT(details_item), "toggled",
      GTK_SIGNAL_FUNC(menu_view_details), NULL);

   create_menu_item(GTK_MENU(menu), NULL, NULL, NULL, 0, 0);

   menu_item = gtk_check_menu_item_new_with_label(_("Show hidden files"));
   gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menu_item),
      rc_get_boolean("show_hidden"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
      GTK_SIGNAL_FUNC(menu_view_show_hidden), NULL);
#ifdef GTK_HAVE_FEATURES_1_1_0
   gtk_widget_add_accelerator(menu_item, "activate",
      accel, GDK_H, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
#else
   gtk_widget_install_accelerator(menu_item, accel, "activate",
      'H', GDK_CONTROL_MASK);
#endif
   gtk_widget_show(menu_item);

   menu_item = gtk_check_menu_item_new_with_label(_("Hide non-images"));
   gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menu_item),
      rc_get_boolean("hide_non_images"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
      GTK_SIGNAL_FUNC(menu_view_hide), NULL);
#ifdef GTK_HAVE_FEATURES_1_1_0
   gtk_widget_add_accelerator(menu_item, "activate",
      accel, GDK_I, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
#else
   gtk_widget_install_accelerator(menu_item, accel, "activate",
      'I', GDK_CONTROL_MASK);
#endif
   gtk_widget_show(menu_item);

   menu_item = gtk_check_menu_item_new_with_label(_("Fast preview"));
   gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menu_item),
      rc_get_boolean("fast_preview"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
      GTK_SIGNAL_FUNC(menu_view_preview), NULL);
   gtk_widget_show(menu_item);

   create_menu_item(GTK_MENU(menu), NULL, NULL, NULL, 0, 0);

   menu_item = create_menu_item(GTK_MENU(menu), _("Sort"), NULL, NULL, 0, 0);
   submenu = gtk_menu_new();
   gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);
   create_menu_item(GTK_MENU(submenu), _("By Name"),
      GTK_SIGNAL_FUNC(menu_view_sort_by_name),
      NULL, 0, 0);
   create_menu_item(GTK_MENU(submenu), _("By Size"),
      GTK_SIGNAL_FUNC(menu_view_sort_by_size),
      NULL, 0, 0);
   create_menu_item(GTK_MENU(submenu), _("By Property"),
      GTK_SIGNAL_FUNC(menu_view_sort_by_property),
      NULL, 0, 0);
   create_menu_item(GTK_MENU(submenu), _("By Date"),
      GTK_SIGNAL_FUNC(menu_view_sort_by_date),
      NULL, 0, 0);
   create_menu_item(GTK_MENU(submenu), _("By Type"),
      GTK_SIGNAL_FUNC(menu_view_sort_by_type),
      NULL, 0, 0);

   create_menu_item(GTK_MENU(submenu), NULL, NULL, NULL, 0, 0);

   create_menu_item(GTK_MENU(submenu), _("Ascend"),
      GTK_SIGNAL_FUNC(menu_view_sort_ascend),
      NULL, 0, 0);
   create_menu_item(GTK_MENU(submenu), _("Descend"),
      GTK_SIGNAL_FUNC(menu_view_sort_descend),
      NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), NULL, NULL, NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("Refresh"),
      GTK_SIGNAL_FUNC(menu_view_refresh),
      accel,
#ifdef GTK_HAVE_FEATURES_1_1_0
      GDK_F5,
#else
      'R',
#endif
      0);

   create_menu_item(GTK_MENU(menu), _("Quick-refresh"),
      GTK_SIGNAL_FUNC(menu_view_quick_refresh),
      accel,
#ifdef GTK_HAVE_FEATURES_1_1_0
      GDK_F5,
#else
      'R',
#endif
      GDK_SHIFT_MASK);

   menu_item = gtk_check_menu_item_new_with_label(_("Auto-refresh"));
   gtk_menu_append(GTK_MENU(menu), menu_item);
   gtk_signal_connect(GTK_OBJECT(menu_item), "toggled",
      GTK_SIGNAL_FUNC(menu_view_auto_refresh), NULL);
   gtk_widget_show(menu_item);

   /* Tools menu */
   menu = create_menu(GTK_MENU_BAR(menubar), _("Tools"), accel);

   create_menu_item(GTK_MENU(menu), _("Slide show"),
      GTK_SIGNAL_FUNC(menu_tools_slide_show),
      accel, GDK_S, GDK_CONTROL_MASK);

   create_menu_item(GTK_MENU(menu), _("The Gimp"),
      GTK_SIGNAL_FUNC(menu_send_gimp),
      NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), NULL, NULL, NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("Options..."),
      GTK_SIGNAL_FUNC(options_show),
      NULL, 0, 0);

   /* Help menu */
   menu = create_menu(GTK_MENU_BAR(menubar), _("Help"), accel);

   create_menu_item(GTK_MENU(menu), _("Legal"),
      GTK_SIGNAL_FUNC(menu_help_legal),
      NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("Usage"),
      GTK_SIGNAL_FUNC(menu_help_usage),
      NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("Known-bugs"),
      GTK_SIGNAL_FUNC(menu_help_known_bugs),
      NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("Feedback"),
      GTK_SIGNAL_FUNC(menu_help_feedback),
      NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), NULL, NULL, NULL, 0, 0);

   create_menu_item(GTK_MENU(menu), _("About"),
      GTK_SIGNAL_FUNC(menu_help_about),
      NULL, 0, 0);

   return menubar;
}

void
menu_set_list_type(ImageListType type)
{
   switch (type)
   {
     case IMAGE_LIST_THUMBNAILS:
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(thumbnails_item), TRUE);
      break;
     case IMAGE_LIST_SMALL_ICONS:
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(small_icons_item), TRUE);
      break;
     case IMAGE_LIST_DETAILS:
     default:
      gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(details_item), TRUE);
      break;
   }
}

void
menu_help_legal(GtkWidget *widget, gpointer data)
{
   static guint legal_sizes[] =
   {
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      0
   };
   static guchar *legal_text[] =
   {
      N_("    GTK See is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n\n"),
      N_("    GTK See is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n\n"),
      N_("    You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"),
      NULL
   };
   GtkWidget *dialog;

   dialog = info_dialog_new(_("Legal"), legal_sizes, legal_text);
   gtk_widget_show(dialog);
}

void
menu_help_usage(GtkWidget *widget, gpointer data)
{
   static guint usage_sizes[] =
   {
      FONT_BIG,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_BIG,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_BIG,
      FONT_SMALL,
      FONT_BIG,
      0
   };
   static guchar *usage_text[] = {
      N_("Command-line usage:\n"),
      N_("    gtksee [-R[directory]] [-fis] [files...]\n"),
      N_("      -r  --  Use current directory as root\n"),
      N_("      -R<directory>  --  Use <directory> as root\n"),
      N_("      -f  --  Enable full-screen mode\n"),
      N_("      -i  --  Enable fit-screen mode\n"),
      N_("      -s  --  Enable slide-show mode\n"),
      N_("      files... --  Launch gtksee viewer\n"),
      N_("      -h  --  Print help messages\n\n"),
      N_("Keyboard shortcuts:\n"),
      N_("    These keys are bound to viewer:\n"),
      N_("    [Arrow-keys] Move large image\n"),
      N_("    [PageUp] Show previous image\n"),
      N_("    [PageDown] Show next image\n"),
      N_("    [Home] Show the first image\n"),
      N_("    [End] Show the last image\n"),
      N_("    [Space] Show next image(loop)\n"),
      N_("    [Escape] Return to the browser\n\n"),
      N_("Mouse hints:\n"),
      N_("    In view mode, double-click on the image to return to the browser.\n\n"),
      N_("Have fun!\n"),
      NULL
   };
   GtkWidget *dialog;

   dialog = info_dialog_new(_("Usage"), usage_sizes, usage_text);
   gtk_widget_show(dialog);
}

void
menu_help_known_bugs(GtkWidget *widget, gpointer data)
{
   static guint known_bugs_sizes[] = {
      FONT_BIG,
      0
   };
   static guchar *known_bugs_text[] = {
      N_("None :)\n"),
      NULL
   };
   GtkWidget *dialog;

   dialog = info_dialog_new(_("Known-bugs"), known_bugs_sizes, known_bugs_text);
   gtk_widget_show(dialog);
}

void
menu_help_feedback(GtkWidget *widget, gpointer data)
{
   static guint feedback_sizes[] =
   {
      FONT_BIG,
      FONT_SMALL,
      FONT_BIG,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      FONT_SMALL,
      0
   };
   static guchar *feedback_text[] = {
      N_("Comments, suggestions and bug-reportings:\n"),
      "    Maintainer: keziah@users.berlios.de\n\n",
      "Original feedback:\n",
      N_("Comments, suggestions and bug-reportings:\n"),
      N_("    Please send them to my current primary e-mail address:\n"),
      "    jkhotaru@mail.sti.com.cn\n\n",
      "Imageware:\n",
      N_("    I'm always collecting all kinds of images. If you *really* like this program, send your favorite picture(s) to my secondary e-mail address:\n"),
      "    hotaru@163.net\n",
      N_("    Keep sending me things. :) And I'll keep on developing this program.\n"),
      N_("    P.S. My favorite is Anime/Manga. :)\n"),
      NULL
   };
   GtkWidget *dialog;

   dialog = info_dialog_new(_("Feedback"), feedback_sizes, feedback_text);
   gtk_widget_show(dialog);
}

void menu_send_gimp(GtkWidget *widget, gpointer data)
{
  char filen[256];
  char *cmd;

  if (get_current_image(filen)) {
    cmd = g_strdup_printf("gimp-remote -n %s &",filen);
    system(cmd);
    g_free(cmd);
  }

  return;
}
