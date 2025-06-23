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
// Copyright (C) 2023-present by Tom Lechner
//
#ifndef STREAMS_H
#define STREAMS_H


#include <lax/resources.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/boxarrange.h>
#include <lax/transformmath.h>

#include "../calculator/values.h"
#include "../dataobjects/drawableobject.h"
#include "../core/plaintext.h"
#include "../filetypes/filefilters.h"
#include "style.h"


namespace Laidout {


class Stream;
class StreamCache;
class StreamChunk;


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
	LaxInterfaces::LineStyle *linestyle;
};

class DropShadowEffect : public TextEffect
{
  public:
	Laxkit::flatvector direction;
	double blur_amount;
	Laxkit::Color color; //color or color diff
};

 /*! \class CharReplacement
  * TextEffect where characters are replaced before being rendered into glyphs.
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
	LaxInterfaces::PathsData *path = nullptr; //we assume this is a generally downward path

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

	int ComputeAt(double height, Laxkit::PtrStack<TabStopInfo> &tabstops); //for path based tab stops, update tabstops at this height in path space
};


//------------------------------ StreamElement ---------------------------------

class StreamElement //: public Laxkit::DumpUtility
{
  public:
	bool style_is_temp; // false means it is a project resource
	Style *style;

	//int type_hint; //like pure char, pure pp, container type

	StreamElement *treeparent = nullptr;
	Laxkit::PtrStack<StreamElement> kids;

	Laxkit::PtrStack<StreamChunk> chunks; // has chunks only when we are a leaf element

	StreamElement(); //init with an empty new Style
	StreamElement(StreamElement *nparent, Style *nstyle);
	virtual ~StreamElement();
    virtual const char *whattype() { return "StreamElement"; }

    bool dump_in_att_stream(Laxkit::Attribute *att, Laxkit::DumpContext *context);
};


//------------------------------ StreamChunk ---------------------------------

/*! \class StreamChunk
 * \brief One part of a stream that can be considered to be all the same type.
 *
 * This can be, for instance, an image chunk, text chunk, a break, a non-printing anchor.
 *
 * These are explicitly leaf nodes in a StreamElement tree, that point to adjacent leaf nodes
 * for convenience.
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
	virtual int BreakInfo(int index) = 0;
	virtual int Type() = 0; //see StreamChunkTypes

	virtual StreamChunk *AddAfter(StreamChunk *chunk);
	virtual void AddBefore(StreamChunk *chunk);

	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context) = 0;
	virtual void dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context) = 0;
};


//------------------------------ StreamBreak ---------------------------------

enum class StreamBreakTypes : int {
	BREAK_Unknown = 0,
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
  public:
	int break_type; //see StreamBreakType. such as: newline (paragraph), column, section, page

	StreamBreak (int ntype, StreamElement *parent_el) : StreamChunk(parent_el) { break_type = ntype; }
	virtual const char *whattype() { return "StreamBreak"; }

	// StreamChunk overrides:
	virtual int NumBreaks() { return 0; }
	virtual int BreakInfo(int type) { return 0; }
	virtual int Type() { return CHUNK_Break; }

	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};


//------------------------------ StreamImage ---------------------------------

class StreamImage : public StreamChunk
{
  public:
	DrawableObject *img;

	StreamImage(StreamElement *parent_el = nullptr);
	StreamImage(DrawableObject *nimg, StreamElement *parent_el);
	virtual ~StreamImage();
	virtual const char *whattype() { return "StreamImage"; }

	virtual int NumBreaks() { return 0; }
	virtual int BreakInfo(int type) { return 0; }
	virtual int Type() { return CHUNK_Image; }

	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
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
	virtual int SetFromEntities(const char *cdata, int cdata_len);

	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};


//------------------------------ StreamImporter ---------------------------------

class StreamImportData : public Laxkit::anObject
{
  public:
	ImportFilter *importer = nullptr;
	PlainText *text_object = nullptr;
	FileValue *file = nullptr;
	clock_t import_time = 0;

	virtual ~StreamImportData();
};

class StreamImporter : public FileFilter, public Value
{
  public:
  	StreamImporter();
  	virtual ~StreamImporter();

  	virtual const char *whattype() { return "StreamImporter"; }
	virtual const char *FileType(const char *first100bytes) = 0;
	virtual int In(Stream *stream, const char *file, Laxkit::DumpContext *context, const char *filecontents,int contentslen) = 0;

	//Value overrides:
	virtual Value *duplicate();
 	virtual ObjectDef *makeObjectDef(); //calling code responsible for ref
};


//------------------------------ Stream ---------------------------------

class Stream : public Laxkit::anObject, public Laxkit::DumpUtility
{
  protected:
  	void EstablishDefaultStyle();
  	void dump_out_recursive(Laxkit::Attribute *att, StreamElement *element, Laxkit::DumpContext *context);

  public:
	StreamImportData *import_data; //relating to if stream is tied to either a file or a TextObject and run through an importer

	clock_t modtime;

	StreamElement top;
	StreamChunk *chunk_start = nullptr; //convenience pointer for leftmost leaf element

	Stream();
	virtual ~Stream();

	virtual void dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_out (FILE *f,int indent,int what,Laxkit::DumpContext *context);

	virtual int ImportMarkdown(const char *text, int n, StreamChunk *addto, bool after, Laxkit::ErrorLog *log);
	virtual int ImportText(const char *text, int n, StreamChunk *addto, bool after, Laxkit::ErrorLog *log);
	virtual int ImportXML(const char *value, int len, Laxkit::ErrorLog *log);
	virtual int ImportXMLFile(const char *file, Laxkit::ErrorLog *log);
	virtual int ImportXMLAtt(Laxkit::Attribute *att, StreamChunk *&last_chunk, StreamElement *&next_style, StreamElement *last_style_el, Laxkit::ErrorLog *log);
};


//------------------------------ StreamAttachment ---------------------------------

/*! One StreamAttachment is held by each DrawableObject that the stream needs to lay on,
 * StreamAttachment defines how a Stream is meant to lay out on or in the DrawableObject,
 * as well as cached rendering info in StreamAttachment::cache.
 */

class StreamAttachment : public Laxkit::RefCounted
{
  public:
    DrawableObject *owner;
	enum Target { Skip, BoundingBox, ObjectPath, InsetPath, OutsetPath };
	Target attachment_target = InsetPath;
	bool on_path = false;

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
	clock_t modtime = 0;
	StreamCache *next = nullptr, *prev = nullptr;

	StreamElement *element = nullptr;
	StreamChunk *chunk = nullptr;
	long offset = 0; //how many breaks into chunk to start
	long len = 0; //how many breaks long in chunk is this cache

	Laxkit::Affine transform;

	StreamCache();
	StreamCache(StreamChunk *ch, long noffset, long nlen);
	virtual ~StreamCache();
};


} // namespace Laidout

#endif
