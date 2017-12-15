** Development on this project has stopped. It will no longer be updated. **

This is `img2eps`, a raster image to EPSF converter.

`img2eps` packages raster images into EPS (Encapsulated PostScript)
files, using whatever PostScript features are advantageous.  If
possible, the compressed image data is copied directly to the EPS
file.

Supported image file formats are GIF, JPEG, JPEG 2000, PNG, TIFF,
and XPM.

For generic installation instructions, see file [INSTALL](INSTALL); for
documentation read the man page; for internal documentation see the
documents in the doc subdirectory.

If you make a binary distribution, please include a pointer to the
distribution site:
	http://nih.at/img2eps

Mail suggestions and bug reports to <img2eps@nih.at>.


## Requirements

`img2eps` does not require the following packages, but certain
features will be omitted if they are missing:

- For image format support: libpng, jpeg, jasper, tiff, and either
giflib or libungif.

- For compression support: zlib (FlateEncode), jpeg (DCTEncode).

- For EXIF support (to auto-rotate jpeg images): libexif.

`img2eps`'s configure script searches for these libraries with
pkg-config (if the library supports it) and in the directories the
compiler searches.  If you have them installed elsewhere, pass
`CPPFLAGS` and `LDFLAGS` to configure to make the compiler find them.  For
example, if you have the libraries installed in `/usr/local`, the
following should work:
``` sh
CPFLAGS='-I/usr/local/include' LDFLAGS='-L/usr/local/lib' ./configure
```

If your runtime linker needs to be told to search extra directories
too (`img2eps` will link in that case, but complain about missing
libraries when run), either of the following `LDFLAGS` settings should
work:
``` sh
LDFLAGS='-L/usr/local/lib -R/usr/local/lib'
LDFLAGS='-L/usr/local/lib -Wl,-R/usr/local/lib'
```
(The first works on Solaris, the second on NetBSD.)
