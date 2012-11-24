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
// Copyright (C) 2012 by Tom Lechner
//

#include "streams.h"
#include "../language.h"


//----------------------------------- StreamStyle -----------------------------------
class StreamStyle
{
 public:
	StreamStyle *parent;
	RefPtrStack<StreamStyle> kids;
	ValueHash *values;

	StreamStyle();
	virtual ~StreamStyle();

	Value *FindValue(int attribute_name);
};


//----------------------------------- StreamChunk ------------------------------------
/*! \class StreamChunk
 * \brief One part of a stream that can be considered to be all the same type.
 *
 * This can be, for instance, an image chunk, text chunk, a break, a non-printing anchor.
 */
class StreamChunk
{
 public:
	StreamStyle *style;
	StreamChunk *next, *prev;

	StreamChunk();
	virtual ~StreamChunk();
	virtual int NumBreaks()=0;
	virtual int BreakInfo(int *type)=0;
};

StreamChunk::StreamChunk()
{
	style=NULL;
	next=prev=NULL;
}

/* Deletes next. Ignores prev.
 */
StreamChunk::~StreamChunk()
{
	if (style) style->dec_count();
	if (next) delete next;
	next=NULL;
}


//----------------------------------- StreamBreak ------------------------------------
class StreamBreak : public StreamChunk
{
	int type; //newline (paragraph), column, section, page
	virtual int NumBreaks() { return 0; }
	virtual int BreakInfo(int *type) { return 0; }
};


//----------------------------------- StreamText ------------------------------------
/*! \class StreamText
 * \brief Type of stream component made of text.
 *
 * One StreamText object contains a string of characters not including a newline.
 * This string is a single style of text. Any links, font changes, or anything else
 * that distinguishes this text from other text requires a different StreamText.
 */
class StreamText : public StreamChunk
{
 public:
	char *text;
	virtual int NumBreaks();
	virtual int BreakInfo(int *type);
};

int StreamText::ChunkInfo(int *width,int *height)
{
}

//! Return the number of possible breaks within the string of characters.
/*! This can include hyphen points, spaces.
 */
int StreamText::NumBreaks()
{
	if (!text) return 0;

	const char *s=text;
	int n=0;
	while (*s) {
		while (!isspace(*s)) s++;
		n++;
	}
	return n;
}

//! Return an indicator of the severity of the break.
int StreamText::BreakInfo(int *type)
{
}


//------------------------------------- Stream ----------------------------------
/*! \class Stream
 * \brief The head of StreamChunk objects.
 */

class Stream
{
 public:
	char *id;
	StreamStyle *defaultstyle;
	StreamChunk *data;

	Stream();
	virtual ~Stream();
};

