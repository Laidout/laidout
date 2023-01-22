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
// Copyright (C) 2012 by Tom Lechner
//
#ifndef STREAMS_H
#define STREAMS_H


#include <lax/resources.h>


namespace Laidout {


//------------------------------ TextEffect ---------------------------------
/*! \class TextEffect
 * Base class for modifications to parts of a Stream.
 */
class TextEffect : public Laxkit::Resourceable
{
  public:
	virtual ~TextEffect();
	virtual int Compute(StreamCache *start, StreamCache *end, StreamCache **new_start, StreamCache **new_end) = 0;
	//virtual ValueHash *GetValues() = 0;
};

class TextUnderline : public TextEffect
{
  public:
	int linetype; //underline, double underline, strikethrough, etc
	int wordunderline; //nonzero if so
	int underlinestyle;
	double offset; //% of fontsize off baseline, either underline or strikethrough
	LineStyle *linestyle;
};

class DropShadowEffect : public TextEffect
{
  public:
	flatvector direction;
	double blur_amount;
	Color color; //color or color diff
};

 /*! \class CharReplacement
  * TextEffect where characters are replaced before being rendered indo glyphs.
  */
class CharReplacement : public TextEffect
{
  public:
  	enum class Type { AllCaps, NoCaps, SmallCaps };
  	Type type;

	int ReplaceLetters(const char *in, char *out, int outbufsize);
};

/*! \class GlyphReplacement
  * TextEffect for taking a sequence of glyphs and replacing them with some other rendering.
  */
class GlyphReplacement : public TextEffect
{
  public:
	 //glyph transformation effects: ...these are distinct from just using another font for bold, outline, italic
	//outline (artificial stroking of contour)
	//bold   (artificial growing and filling of contour, or selection of bold font of same type)
	//italic (artificial shearing)
};

/*! \class DropCaps
  * TextEffect where blocks of text are elevated from their normal stream flow, and can be
  * made to affect layout of other parts of the stream.
  */
class DropCaps : public TextEffect
{
  public:
	//drop caps at front of pp:
	int numlines;
	int wrap; //how to wrap
	//	hang initial punc.. like "Blah" ...
	//	                          Blah blah...
};


//------------------------------ Tabs ---------------------------------

/*! \class TabStopInfo
 *  Properties relating to stops in a TabStops object.
 */
class TabStopInfo : public Value
{
  public:
//	enum class TABTYPE {
//		Left,
//		Center,
//		Right,
//		Char
//	};
	double alignment = 0; //left==0, center==50, right==100
	bool use_char = false;
	char tab_char_utf8[10]; //if use_char

	int positiontype; //automatic position, definite position, path
	double position; //if not path
	Laxkit::PathsData *path = nullptr; //we assume this is a generally downward path

	TabStopInfo *next = nullptr;

	TabStopInfo();
	virtual ~TabStopInfo();
};


/*! \class TabStops
 * Definition of tab placements for a paragraph style.
 */
class TabStops : public Value
{
  public:
	Laxkit::RefPtrStack<TabStopInfo> stops;

	int ComputeAt(double height, PtrStack<TabStopInfo> &tabstops); //for path based tab stops, update tabstops at this height in path space
};



//------------------------------ Style ---------------------------------

class Style : public ValueHash, public Laxkit::Resourceable
{
  public:
    Style *parent; // if non-null, this MUST be a project resource

    Style();
    virtual ~Style();
    virtual const char *whattype() { return "Style"; }

    virtual Value *FindValue(int attribute_name);
    virtual int MergeFromMissing(Style *s); //only from s not in *this
    virtual int MergeFrom(Style *s); //all in s override *this
    virtual Style *Collapse();

	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_out (FILE *f,int indent,int what,LaxFiles::DumpContext *context);
};


//------------------------------ StreamElement ---------------------------------

class StreamElement : public LaxAttributes::DumpUtility
{
  public:
	bool style_is_temp; // false means it is a project resource
	Style *style;

	//int type_hint; //like pure char, pure pp, container type

	StreamElement *treeparent = nullptr;
	PtrStack<StreamElement> kids;

	PtrStack<StreamChunk> chunks; // has chunks only when we are a leaf element

	StreamElement(); //init with an empty new Style
	StreamElement(StreamElement *nparent, Style *nstyle);
	virtual ~StreamElement();
    virtual const char *whattype() { return "StreamElement"; }
};


//------------------------------ StreamChunk ---------------------------------

/*! \class StreamChunk
 * \brief One part of a stream that can be considered to be all the same type.
 *
 * This can be, for instance, an image chunk, text chunk, a break, a non-printing anchor.
 *
 * These are explicitly leaf nodes in a StreamElement tree, that point to other leaf nodes
 * for convenience. Anything in style is ignored. All actual style information should be contained in the parent tree.
 */


enum StreamChunkTypes {
	CHUNK_Text,
	CHUNK_Image,
	CHUNK_Break,
	CHUNK_MAX
};

class StreamChunk
{
 public:
	StreamElement *parent;
	StreamChunk *next, *prev; //these are maintained by StreamElement

	StreamChunk(StreamElement *nparent);
	virtual ~StreamChunk();
	virtual const char *whattype() { return "StreamChunk"; }

	virtual int NumBreaks() = 0;
	virtual int BreakInfo(int *type) = 0;
	virtual int Type() = 0;

	virtual StreamChunk *AddAfter(StreamChunk *chunk);
	virtual void AddBefore(StreamChunk *chunk);
};


//------------------------------ StreamBreak ---------------------------------

enum class StreamBreakTypes {
	BREAK_Unknown,
    BREAK_Paragraph,
    BREAK_Column,
    BREAK_Section,
    BREAK_Page,
    BREAK_Tab,
	BREAK_Weak,
	BREAK_Hyphen,
    BREAK_MAX
};

class StreamBreak : public StreamChunk
{
	int break_type; //see StreamBreakType. such as: newline (paragraph), column, section, page

	StreamBreak (int ntype, StreamElement *parent_el) : StreamChunk(parent_el) { type = ntype; }
	virtual const char *whattype() { return "StreamBreak"; }

	virtual int NumBreaks() { return 0; }
	virtual int BreakInfo(int *type) { return 0; }
	virtual int Type() { return CHUNK_Break; }

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//------------------------------ StreamImage ---------------------------------

class StreamImage : public StreamChunk
{
  public:
	DrawableObject *img;

	StreamImage(,StreamElement *parent_el = nullptr);
	StreamImage(DrawableObject *nimg,StreamElement *parent_el);
	virtual ~StreamImage();
	virtual const char *whattype() { return "StreamImage"; }
	virtual int NumBreaks() { return 0; }
	virtual int BreakInfo(int *type) { return 0; }
	virtual int Type() { return CHUNK_Image; }

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//------------------------------ StreamText ---------------------------------

class StreamText : public StreamChunk
{
  public:
	int len; //number of bytes in text (excluding any '\0')
	char *text; //utf8 string

	int numspaces;

	StreamText(StreamElement *pstyle = nullptr);
	StreamText(const char *txt,int n, StreamElement *pstyle);
	virtual ~StreamText();
	virtual int NumBreaks(); //either hyphen, spaces, pp
	virtual int BreakInfo(int index);
	virtual int Type() { return CHUNK_Text; }

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//------------------------------ StreamImporter ---------------------------------

class StreamImportData : Laxkit::anObject
{
  public:
	ImportFilter *importer = nullptr;
	TextObject *text_object = nullptr;
	File *file = nullptr;
}


class StreamImporter : public FileFilter, public Value
{
  public:
  	StreamImporter();
  	virtual ~StreamImporter();

  	virtual const char *whattype() { return "StreamImporter"; }
	virtual const char *FileType(const char *first100bytes) = 0;
	virtual int In(const char *file, Laxkit::anObject *context, Laxkit::ErrorLog &log, const char *filecontents,int contentslen) = 0;
};


//------------------------------ Stream ---------------------------------

class Stream : public Laxkit::anObject, public Laxkit::DumpUtility
{
  public:
	StreamImportData *import_data; //relating to if stream is tied to either a file or a TextObject and run through an importer

	clock_t modtime;

	StreamElement top;
	StreamChunk *chunk_start = nullptr; //convenience pointer for leftmost leaf element

	Stream();
	virtual ~Stream();

	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_out (FILE *f,int indent,int what,LaxFiles::DumpContext *context);
};


//------------------------------ StreamAttachment ---------------------------------

/*! Held by DrawableObjects, these define various types of attachments of Stream objects to the parent
 * DrawableObject, as well as cached rendering info in StreamAttachment::cache.
 */

class StreamAttachment : public Laxkit::RefCounted
{
  public:
    DrawableObject *owner;
	int attachment_target; //area path, inset path, outset path, inset area

    Stream *stream;
    StreamCache *cache; //owned by *this, assume any other refs are for temporary rendering purposes

    StreamAttachment(DrawableObject *nobject, Stream *nstream);
    ~StreamAttachment();
};


//------------------------------------- StreamCache ----------------------------------

/*! \class StreamCache
 * \brief Rendering info for Stream objects.
 *
 * This is stored on each StreamAttachment object that connects the Stream to DrawableObject.
 *
 * One StreamChunk object will correspond to one or more contiguous StreamCache objects.
 */

class StreamCache : public Laxkit::SquishyBox, public Laxkit::RefCounted
{
  public:
	clock_t modtime;
	StreamCache *next, *prev;

	StreamElement *style;
	StreamChunk *chunk;
	long offset; //how many breaks into chunk to start
	long len; //how many breaks long in chunk is this cache

	Affine transform;

	StreamCache();
	StreamCache(StreamChuck *ch, long noffset, long nlen);
	virtual ~StreamCache();
};


} // namespace Laidout

#endif

