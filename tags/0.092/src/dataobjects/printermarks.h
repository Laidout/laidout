//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2011 by Tom Lechner
//
#ifndef PRINTERMARKS_H
#define PRINTERMARKS_H

#include <lax/interfaces/somedata.h>
#include <lax/palette.h>


LaxInterfaces::SomeData *RegistrationMark(double pointsize, double linewidthinpoints);
LaxInterfaces::SomeData *BWColorBars(double pointsize, int colorsystem);
LaxInterfaces::SomeData *ColorBars(double pointsize, Laxkit::Palette *palette, int numrows, int numcols);
//LaxInterfaces::SomeData *SpreadInfoMark(const char *format, const char *dateformat, const char *timeformat);


#endif

