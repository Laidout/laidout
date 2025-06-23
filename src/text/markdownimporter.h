//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2023 by Tom Lechner
//
#ifndef _LO_MARKDOWNIMPORTER_H
#define _LO_MARKDOWNIMPORTER_H


#import "md4c/md4c.h"


namespace Laidout {


class MarkdownImporter : StreamImporter
{
  protected:
  	// temporary processing state:
  	Style *current_style = nullptr;
  	int current_level = 0;

  	// md_parse callbacks
	int  _enter_block(MD_BLOCKTYPE type, void* detail);
	int  _leave_block(MD_BLOCKTYPE type, void* detail);
	int  _enter_span(MD_SPANTYPE type, void* detail);
	int  _leave_span(MD_SPANTYPE type, void* detail);
	int  _text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size);
	void _debug_log(const char* msg);

	// relay functions that call the above ones
	static int  enter_block(MD_BLOCKTYPE type, void* detail, void* userdata);
	static int  leave_block(MD_BLOCKTYPE type, void* detail, void* userdata);
	static int  enter_span(MD_SPANTYPE type, void* detail, void* userdata);
	static int  leave_span(MD_SPANTYPE type, void* detail, void* userdata);
	static int  text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata);
	static void debug_log(const char* msg, void* userdata);

  public:
  	MarkdownImporter();
  	~MarkdownImporter();

  	virtual const char *FileType(const char *first100bytes);
	virtual int In(const char *file, Laxkit::anObject *context, Laxkit::ErrorLog &log, const char *filecontents, int contentslen);

	virtual int ImportTo(const char *text, int n, StreamChunk *addto, bool after);
};

} // namespace Laidout

#endif
