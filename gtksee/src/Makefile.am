AUTOMAKE_OPTIONS = no-dependencies

bin_PROGRAMS = gtksee
INCLUDES = @IMCFLAGS@ -DLOCALEDIR=\"$(localedir)\"
gtksee_SOURCES = gtypes.h intl.h util.h util.c im_xpm.h im_xpm.c im_jpeg.h im_jpeg.c im_gif.h im_gif.c im_bmp.h im_bmp.c im_ico.h im_ico.c im_pcx.h im_pcx.c im_tiff.h im_tiff.c im_png.h im_png.c im_pnm.h im_pnm.c im_psd.h im_psd.c im_xbm.h im_xbm.c im_xcf.h im_xcf.c scale.h scale.c scanline.h scanline.c dirtree.h dirtree.c detect.h detect.c imageclist.h imageclist.c imagetnitem.h imagetnitem.c imagetnlist.h imagetnlist.c imagesiitem.h imagesiitem.c imagesilist.h imagesilist.c imagelist.h imagelist.c dndviewport.h dndviewport.c common_tools.h common_tools.c rc.h rc.c options.h options.c viewertoolbar.h viewertoolbar.c viewerstatus.h viewerstatus.c viewer.h viewer.c gtkseemenu.h gtkseemenu.c gtkseestatus.h gtkseestatus.c gtkseeabout.h gtkseeabout.c gtkseetoolbar.h gtkseetoolbar.c gtksee.h gtksee.c
gtksee_LDADD = @IMLIBS@
