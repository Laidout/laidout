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
// Copyright (C) 2004-2007 by Tom Lechner
//

#include "filefilters.h"
#include "../language.h"


//------------------------------------- FileFilter -----------------------------------
/*! \class FileFilter
 * \brief Abstract base class of input and output file filters.
 *
 * These filters act on whole files, not on chunks of them. They are for exporting
 * to an entire postscript file, for instance, or importing an entire svg file to 
 * an object or document context.
 *
 * Descendent classes of this class will have a whattype() of "FileInputFilter" or 
 * "FileOutputFilter". Other distinguishing tags are accessible through the proper
 * filter info functions, not through the whattype() function.
 */


/*! \fn ~FileFilter()
 * \brief Empty virtual destructor.
 */
/*! \fn const char *FileFilter::Author()
 * \brief Return who made this filter. For default ones, this is "Laidout".
 */
/*! \fn const char *FileFilter::FilterVersion()
 * \brief Return a string representing the filter version.
 */
/*! \fn const char *FileFilter::Format()
 * \brief Return the general file format the filter deals with.
 *
 * For instance, this would be "Postscript", "Svg", etc.
 */
/*! \fn const char **FileFilter::FormatVersions(int *n)
 * \brief A NULL terminated list of the versions of Format() that the filter knows how to deal with.
 *
 * For PDF, for instance, this might would be {"1.4","1.3",NULL}.
 * For SVG, this could even be broken down into {"1.0","1.1","1.0-inkscape",NULL}, for instance.
 *
 * If n!=NULL, then put the number of versions into it.
 */
/*! \fn const char *FileFilter::VersionName(const char *version)
 * \brief A name for a screen dialog corresponding to version.
 *
 * This might be "Postscript LL3", or "Svg, version 1.1". In the latter case, the
 * format was "SVG" and the version was "1.1", but the name is something composite, that
 * is open to translations.
 */
/*! \fn const char *FileFilter::FilterClass()
 * \brief What the filter dumps from or to.
 *
 * \todo These can currently be "document", "object", "image" (a raster image),
 * or resources such as "gradient", "palette", or "net".
 *
 * LaidoutApp keeps a catalog of filters grouped by FilterClass(), and then by whether
 * they are for input or output.
 */
/*! \fn Laxkit::anXWindow *ConfigDialog()
 * \brief Return a configuration dialog for the filter.
 *
 * Default is to return NULL.
 *
 * \todo *** implement this feature!
 */

//------------------------------------- FileInputFilter -----------------------------------
/*! \class FileInputFilter
 * \brief Abstract base class of input file filters.
 */


/*! \fn const char *FileInputFilter::FileType(const char *first100bytes)
 * \brief Return the version of the filter's format that the file seems to be, or NULL if not recognized.
 */

//------------------------------------- FileOutputFilter -----------------------------------
/*! \class FileOutputFilter
 * \brief Abstract base class of input file filters.
 */
/*! \fn int FileOutputFilter::Verify(Laxkit::anObject *context)
 * \brief Preflight checker
 *
 * \todo Ideally, this function should return some sort of set of objects that cannot be transfered
 *   in the given format, and other objects that can be transfered, but only in a lossless way.
 */




