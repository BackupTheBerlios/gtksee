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
 * 2004-05-20: Code based in code taked from XMMS
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>

#include "intl.h"
#include "pixmaps/gtksee.xpm"
#include "gtkseeabout.h"

/*
 * The different sections are kept in alphabetical order
 */

static guchar *credit_text[] = {
   N_("Main Programming:") ,
      N_("Lee Luyang"), NULL,
   N_("Additional Programming:"), N_("Daniel R. Ome"), NULL,
   N_("Important Collaboration:"),
      N_("Roman Belenov"),
      N_("Leopoldo Cerbaro"),
      N_("Jean-Pierre Demailly"),
      N_("Dmitry Goroh"),
      N_("Andreas Grosse"),
      N_("huzheng"),
      N_("Jarek"),
      N_("Jan Keirse"),
      N_("Kevin Krumwiede"),
      N_("Laurent Maestracci"),
      N_("Asbjoern Pettersen"),
      N_("Regis Rampnoux"),
      N_("Ulrich Ross"),
      N_("Dirk Ruediger"),
      N_("Holger Weiss"),
      N_("Pyun YongHyeon"), NULL,
   N_("Homepage and Graphics:"),
      N_("keziah"), NULL,
   N_("Man page:"), N_("Dirk Ruediger"), NULL,
 NULL};

static guchar *translators[] =
{
   N_("Chinese:")  , N_("Lee Luyang")     , NULL,
   N_("French:")   , N_("Regis Rampnoux") , NULL,
   N_("German:")   , N_("Dirk Ruediger")  , NULL,
   N_("Polish:")   , N_("Leszek Pietryka"), NULL,
   N_("Russian:")  , N_("Dmitry Goroh")   , NULL,
   N_("Spanish:")  , N_("Daniel R. Ome")  , NULL,
   N_("Ukrainian:"), N_("Dmitry Goroh")   , NULL,
   NULL
};

static guchar *formats[] =
{
   "bmp",         N_("Microsoft Windows and OS/2 bitmap image"), NULL,
   "eps",         N_("Adobe Encapsulated PostScript (need GhostScript)"), NULL,
   "gif, gif87",  N_("CompuServe graphics interchange format"), NULL,
   "ico, icon",   N_("Microsoft icon"), NULL,
   "jpeg, jpg",   N_("Joint Photographic Experts Group JFIF format"), NULL,
   "pbm",         N_("Portable  bitmap  format  (black and white)"), NULL,
   "pcx",         N_("ZSoft IBM PC Paintbrush"), NULL,
   "pgm",         N_("Portable graymap format (gray scale)"), NULL,
   "png",         N_("Portable Network Graphics"), NULL,
   "pnm",         N_("Portable anymap"), NULL,
   "ppm",         N_("Portable pixmap format (color)"), NULL,
   "ps, psd",     N_("Adobe PostScript"), NULL,
   "ras, sun",    N_("SUN Rasterfile"), NULL,
   "sgi",         N_("Irix RGB image"), NULL,
   "tga",         N_("Truevision Targa image"), NULL,
   "tif, tiff",   N_("Tagged Image File Format"), NULL,
   "wmf",         N_("Windows Metafile (need wmftogif)"), NULL,
   "xbm",         N_("X Windows system bitmap (black and white)"), NULL,
   "xcf",         N_("GIMP image"), NULL,
   "xpm",         N_("X Windows system pixmap (color)"), NULL,
   "xwd",         N_("X Windows system window dump (color)"), NULL,
   NULL
};

static guchar *legal_text[] =
{
   "#",
   N_("COPYING\n"),
   N_("GTK See is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n\n"),
   N_("GTK See is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n\n"),
   N_("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"),
   NULL
};

static guchar *usage_text[] = {
   "#",
   N_("Command-line usage:\n"),
   N_(" gtksee [-R[directory]] [-rfisvh] [files...]\n"),
   N_("  -r               Use current directory as root\n"),
   N_("  -R<directory>    Use <directory> as root\n"),
   N_("  -f               Enable full-screen mode\n"),
   N_("  -i               Enable fit-screen mode\n"),
   N_("  -s               Enable slide-show mode\n"),
   N_("  -v               Print package version\n"),
   N_("  -h               Print this help message\n"),
   N_("  files...         Launch gtksee viewer\n\n"),
   "#",
   N_("Keyboard shortcuts:\n"),
   N_(" These keys are bound to viewer:\n"),
   N_(" [Arrow-keys] Move large image\n"),
   N_(" [PageUp] Show previous image\n"),
   N_(" [PageDown] Show next image\n"),
   N_(" [Home] Show the first image\n"),
   N_(" [End] Show the last image\n"),
   N_(" [Space] Show next image(loop)\n"),
   N_(" [Escape] Return to the browser\n\n"),
   "#",
   N_("Mouse hints:\n"),
   N_(" In view mode, double-click on the image to return to the browser.\n\n"),
   "#",
   N_("Have fun!\n"),
   NULL
};

static guchar *known_bugs_text[] = {
   "#",
   N_("None :)\n"),
   NULL
};

static guchar *feedback_text[] = {
   "#",
   N_("Comments, suggestions and bug-reportings:\n"),
   N_("  Maintainer: keziah@users.berlios.de\n\n"),
   "#",
   N_("Original feedback:\n"),
   N_("Comments, suggestions and bug-reportings:\n"),
   N_("  Please send them to my current primary e-mail address:\n"),
   "    jkhotaru@mail.sti.com.cn\n\n",
   N_("Imageware:\n"),
   N_("  I'm always collecting all kinds of images. If you *really* like this program, send your favorite picture(s) to my secondary e-mail address:\n"),
      "  hotaru@163.net\n",
   N_("  Keep sending me things. :) And I'll keep on developing this program.\n"),
   N_("  P.S. My favorite is Anime/Manga. :)\n"),
   NULL
};

/* Begin the code */

void
print_help_message()
{
   gint i = 1;
   
   while (*usage_text[i] != '#')
   {
      printf("%s", _(usage_text[i++]));
   }

   _exit(0);
}

static GtkWidget*
generate_list(guchar *text[], gboolean sec_space)
{
   GtkWidget *clist, *scrollwin;
   int i = 0;

   clist = gtk_clist_new(2);

   while (text[i])
   {
      gchar *temp[2];
      guint row;

      temp[0] = gettext(text[i++]);
      temp[1] = gettext(text[i++]);
      row = gtk_clist_append(GTK_CLIST(clist), temp);
      gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
      temp[0] = "";
      while (text[i])
      {
         temp[1] = gettext(text[i++]);
         row = gtk_clist_append(GTK_CLIST(clist), temp);
         gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
      }
      i++;
      if (text[i] && sec_space)
      {
         temp[1] = "";
         row = gtk_clist_append(GTK_CLIST(clist), temp);
         gtk_clist_set_selectable(GTK_CLIST(clist), row, FALSE);
      }
   }
   gtk_clist_columns_autosize(GTK_CLIST(clist));
   gtk_clist_set_column_justification(GTK_CLIST(clist), 0, GTK_JUSTIFY_RIGHT);

   scrollwin = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_ALWAYS);
   gtk_container_add(GTK_CONTAINER(scrollwin), clist);
   gtk_container_set_border_width(GTK_CONTAINER(scrollwin), 5);
   gtk_widget_set_usize(scrollwin, -1, 130);

   return scrollwin;
}

static GtkWidget*
generate_text(guchar *text[])
{
   GtkWidget *text_area, *scrollv, *scrollh, *table;
   GdkFont   *bold_font;
   gint i;

   table = gtk_table_new (2, 2, FALSE);
   gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
   gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);

   /* creating text area */
   text_area = gtk_text_new(NULL, NULL);
   gtk_text_set_editable(GTK_TEXT(text_area), FALSE);
   gtk_text_set_word_wrap(GTK_TEXT(text_area), TRUE);
   gtk_widget_set_usize(text_area, 400, 300);

   scrollv = gtk_vscrollbar_new(GTK_TEXT(text_area)->vadj);
   scrollh = gtk_hscrollbar_new(GTK_TEXT(text_area)->hadj);

   gtk_table_attach (GTK_TABLE (table), text_area, 0, 1, 0, 1,
                     GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                     GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

   gtk_table_attach (GTK_TABLE (table), scrollv, 1, 2, 0, 1,
                     GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

   gtk_table_attach (GTK_TABLE (table), scrollh, 0, 1, 1, 2,
                     GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 0, 0);

   /* adding texts */
   bold_font = gdk_font_load("-*-*-bold-*-*-*-*-*-*-*-*-*-*-*");
   i = 0;
   gtk_text_freeze(GTK_TEXT(text_area));

   while (text[i])
   {
      if (*text[i] == '#')
      {
         gtk_text_insert(GTK_TEXT(text_area),
                           bold_font,
                           NULL,
                           NULL,
                           _(text[++i]),
                           -1);
         i++;
      }

      while (text[i] && *text[i] != '#')
      {
         gtk_text_insert(GTK_TEXT(text_area),
                           NULL,
                           NULL,
                           NULL,
                           _(text[i++]),
                           -1);
      }
   }

   gtk_text_thaw(GTK_TEXT(text_area));

   return table;
}

GtkWidget *
get_contents()
{
   GtkWidget *contents_notebook;
   GtkWidget *text_area;

   contents_notebook = gtk_notebook_new();

   text_area = generate_text(usage_text);
   gtk_notebook_append_page(GTK_NOTEBOOK(contents_notebook), text_area,
                              gtk_label_new(_("Usage")));

   text_area = generate_text(known_bugs_text);
   gtk_notebook_append_page(GTK_NOTEBOOK(contents_notebook), text_area,
                              gtk_label_new(_("Known bugs")));

   text_area = generate_text(legal_text);
   gtk_notebook_append_page(GTK_NOTEBOOK(contents_notebook), text_area,
                              gtk_label_new(_("Legal")));

   text_area = generate_text(feedback_text);
   gtk_notebook_append_page(GTK_NOTEBOOK(contents_notebook), text_area,
                              gtk_label_new(_("Feedback")));

   return contents_notebook;
}

GtkWidget *
get_about()
{
   GtkWidget *about_notebook;
   GtkWidget *list;

   about_notebook = gtk_notebook_new();

   list = generate_list(credit_text, TRUE);
   gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                              gtk_label_new(_("Credits")));

   list = generate_list(translators, TRUE);
   gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                              gtk_label_new(_("Translators")));

   list = generate_list(formats, FALSE);
   gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                              gtk_label_new(_("Availables formats")));

   return about_notebook;
}

void
generate_dialog(gchar type)
{
   GdkPixmap *gtksee_logo_pmap = NULL,
             *gtksee_logo_mask = NULL;

   GtkWidget *close_btn;
   GtkWidget *window = NULL;
   GtkWidget *vbox;
   GtkWidget *logo_box;
   GtkWidget *logo;
   GtkWidget *label;
   GtkWidget *separator;
   GtkWidget *notebook;
   gchar     *text;

   if (window)
      return;

   window = gtk_window_new(GTK_WINDOW_DIALOG);
   gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
   gtk_window_set_policy(GTK_WINDOW(window), FALSE, TRUE, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(window), 10);
   gtk_signal_connect(GTK_OBJECT(window), "destroy",
            GTK_SIGNAL_FUNC(gtk_widget_destroyed), &window);
   gtk_widget_realize(window);

   vbox = gtk_vbox_new(FALSE, 5);
   gtk_container_add(GTK_CONTAINER(window), vbox);

   if (!gtksee_logo_pmap)
   {
      gtksee_logo_pmap =gdk_pixmap_create_from_xpm_d(window->window,
                           &gtksee_logo_mask, NULL, gtksee_xpm);
   }
   logo_box = gtk_hbox_new(TRUE, 0);
   gtk_box_pack_start(GTK_BOX(vbox), logo_box, FALSE, FALSE, 0);

   logo = gtk_pixmap_new(gtksee_logo_pmap, gtksee_logo_mask);
   gtk_container_add(GTK_CONTAINER(logo_box), logo);

   text = g_strdup_printf(_("GTK See v%s\nURL: http://gtksee.berlios.de"), VERSION);
   label = gtk_label_new(text);
   g_free(text);
   gtk_box_pack_start(GTK_BOX(logo_box), label, FALSE, FALSE, 0);

   separator =  gtk_hseparator_new ();
   gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

   switch(type)
   {
      case ABOUT:
         gtk_window_set_title(GTK_WINDOW(window), _("About GTK See"));
         notebook = get_about();
    break;
      case CONTENTS:
         gtk_window_set_title(GTK_WINDOW(window), _("Contents"));
         notebook = get_contents();
    break;
   }

   gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

   close_btn = gtk_button_new_with_label(_("Close"));
   gtk_signal_connect_object(GTK_OBJECT(close_btn), "clicked",
              GTK_SIGNAL_FUNC(gtk_widget_destroy),
              GTK_OBJECT(window));
   GTK_WIDGET_SET_FLAGS(close_btn, GTK_CAN_DEFAULT);
   gtk_box_pack_start(GTK_BOX(vbox), close_btn, TRUE, TRUE, 0);
   gtk_widget_grab_default(close_btn);

   gtk_widget_show_all(window);

   return;
}

