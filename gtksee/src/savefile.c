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
 * 2005-02-02: Added for save modificated images.
 *
 */

#include "config.h"

#include <stdio.h>

#ifdef HAVE_LIBJPEG
   #include <jpeglib.h>
#endif
#ifdef HAVE_LIBPNG
   #include <png.h>
#endif

#include "common_tools.h"
#include "gtksee.h"
#include "intl.h"
#include "savefile.h"

#define   JPEG   1
#define   PNG    2
#define   PPM    3

gchar       typefile = PPM;
ImageCache  *imagecache;

GtkWidget *
make_menu_item( gchar *name, GtkSignalFunc callback, gpointer data )
{
   GtkWidget *item;

   item = gtk_menu_item_new_with_label (name);
   gtk_signal_connect (GTK_OBJECT (item), "activate", callback, data);
   gtk_widget_show (item);

   return(item);
}

void
choice_type(GtkWidget *widget, gpointer data)
{
   typefile = (gint) data;
}

GtkWidget *
make_menu_filetypes()
{
   GtkWidget *opt, *menu, *item;

   opt = gtk_option_menu_new();
   menu = gtk_menu_new();

   item = gtk_menu_item_new_with_label(_("Extensions"));
   gtk_widget_show (item);
   gtk_menu_append (GTK_MENU (menu), item);

   item = gtk_menu_item_new();
   gtk_widget_show (item);
   gtk_menu_append (GTK_MENU (menu), item);

   item = make_menu_item ("JPEG",
                           GTK_SIGNAL_FUNC(choice_type),
                           (gint *) JPEG);
   gtk_menu_append (GTK_MENU (menu), item);
#ifndef HAVE_LIBJPEG
   gtk_widget_set_sensitive(item, FALSE);
#endif

   item = make_menu_item ("PNG",
                           GTK_SIGNAL_FUNC (choice_type),
                           (gint *) PNG);
   gtk_menu_append (GTK_MENU (menu), item);
#ifndef HAVE_LIBPNG
   gtk_widget_set_sensitive(item, FALSE);
#endif

   item = make_menu_item ("PPM",
                           GTK_SIGNAL_FUNC (choice_type),
                           (gint *) PPM);
   gtk_menu_append (GTK_MENU (menu), item);

   gtk_option_menu_set_menu (GTK_OPTION_MENU (opt), menu);
   gtk_widget_show (opt);

   return (opt);
}

void
save2jpeg(gchar *filename)
{
#ifdef HAVE_LIBJPEG
   struct jpeg_compress_struct   cinfo;
   struct jpeg_error_mgr         jerr;
   JSAMPLE     *image_buffer;
   FILE        *outfile;
   JSAMPROW    row_pointer[1];
   gint        row_stride;

   cinfo.err = jpeg_std_error(&jerr);

   jpeg_create_compress(&cinfo);

   if ((outfile = fopen(filename, "wb")) == NULL) {
      alert_dialog("Problem saving file");
      return;
   }
   jpeg_stdio_dest(&cinfo, outfile);

   image_buffer            = imagecache->buffer;
   cinfo.image_width       = imagecache->buffer_width;
   cinfo.image_height      = imagecache->buffer_height;
   cinfo.input_components  = 3;
   cinfo.in_color_space    = JCS_RGB;

   jpeg_set_defaults(&cinfo);
   jpeg_set_quality(&cinfo, 75, TRUE);

   jpeg_start_compress(&cinfo, TRUE);

   row_stride = cinfo.image_width * 3;

   while (cinfo.next_scanline < cinfo.image_height) {
      row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
      (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
   }

   jpeg_finish_compress(&cinfo);

   fclose(outfile);

   jpeg_destroy_compress(&cinfo);

#endif
}

void
save2png(gchar *filename)
{
#ifdef HAVE_LIBPNG
   FILE        *fp;
   png_structp png_ptr;
   png_infop   info_ptr;
   png_uint_32 k, height, width;
   png_bytep   row_pointers[imagecache->buffer_height];

   if ((fp = fopen(filename, "wb")) == NULL) {
      alert_dialog("Problem saving file");
      return;
   }

   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (png_ptr == NULL)
   {
      alert_dialog("Problem saving file");
      fclose(fp);
      return;
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      alert_dialog("Problem saving file");
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return;
   }

   if (setjmp(png_ptr->jmpbuf))
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return;
   }

   png_init_io(png_ptr, fp);

   png_set_compression_level(png_ptr, 6);

   width  = imagecache->buffer_width;
   height = imagecache->buffer_height;

   png_set_IHDR(png_ptr, info_ptr,
                  width, height,
                  8,
                  PNG_COLOR_TYPE_RGB,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_BASE,
                  PNG_FILTER_TYPE_BASE);

   png_write_info(png_ptr, info_ptr);

   for (k = 0; k < height; k++)
   {
      row_pointers[k] = imagecache->buffer + k * width * 3;
   }

   png_write_image(png_ptr, row_pointers);
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

   fclose(fp);

   return;
#endif
}


void
save2ppm(gchar *filename)
{
   FILE     *fp;

   fp = fopen (filename, "wb");
   if (fp == NULL)
   {
      perror("Error: ");
   }

   fprintf (fp,
            "P6\n"
            "#Created by GTK See\n"
            "%d %d\n255\n",
            imagecache->buffer_width, imagecache->buffer_height);

   fwrite(imagecache->buffer
            , imagecache->buffer_width * imagecache->buffer_height * 3,
            1,
            fp);

   fclose(fp);

}

void
selection_ok(GtkWidget *widget, GtkWidget *fs)
{
   gchar *name;
   gchar *filename;

   name = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));

   switch (typefile)
   {
      case JPEG:
         filename = g_strconcat(name, ".jpg", NULL);
         save2jpeg(filename);
         refresh_all();
         break;

      case PNG :
         filename = g_strconcat(name, ".png", NULL);
         save2png(filename);
         refresh_all();
         break;

      case PPM :
         filename = g_strconcat(name, ".ppm", NULL);
         save2ppm(filename);
         refresh_all();
         break;

   }

   gtk_widget_destroy(fs);

   g_free(filename);
   g_free(name);

   return;
}

void
savefile (ImageCache *im_cache)
{
   GtkWidget         *fileselector;
   GtkWidget         *label;
   GtkWidget         *frame;
   GtkWidget         *hbox;
   GtkFileSelection  *fs;

   imagecache = im_cache;

   fileselector= gtk_file_selection_new(_("Save file as.."));
   fs          = GTK_FILE_SELECTION(fileselector);

   frame = gtk_frame_new(_("Save Options"));
   gtk_container_add (GTK_CONTAINER (fs->main_vbox), frame);
   gtk_widget_show(frame);

   hbox  = gtk_hbox_new(FALSE, 10);
   gtk_container_add (GTK_CONTAINER (frame), hbox);
   gtk_widget_show(hbox);

   label = gtk_label_new (_("File Type:"));
   gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
   gtk_widget_show(label);

   /* Insert menu of file types */
   gtk_box_pack_start (GTK_BOX (hbox), make_menu_filetypes(), TRUE, TRUE, 0);

   gtk_file_selection_hide_fileop_buttons(fs);

   gtk_signal_connect (GTK_OBJECT (fileselector),
                        "destroy",
                        GTK_SIGNAL_FUNC (gtk_widget_destroy),
                        &fileselector);

   gtk_signal_connect (GTK_OBJECT (fs->ok_button),
                        "clicked",
                        GTK_SIGNAL_FUNC (selection_ok),
                        fileselector);

   gtk_signal_connect_object (GTK_OBJECT (fs->cancel_button),
                        "clicked",
                        GTK_SIGNAL_FUNC (gtk_widget_destroy),
                        GTK_OBJECT(fileselector));

   /* Display that dialog */

   gtk_widget_show (fileselector);

   return;
}
