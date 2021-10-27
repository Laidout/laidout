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



namespace Laidout {


//------------------------------ TextEffect ---------------------------------
class TextEffect : public Laxkit::Resourceable
{
  public:
	virtual ~TextEffect();
	virtual int Compute(StreamCache *start, StreamCache *end, StreamCache **new_start, StreamCache **new_end) = 0;
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

class CharAdjust : public TextEffect
{
  public:
	 //glyph replacement effects:
	int ReplaceLetters(const char *in, char *out, int outbufsize);
	all caps
	no caps
	small caps

	 //glyph transformation effects:
	outline (artificial stroking of contour)
	bold   (artificial growing and filling of contour, or selection of bold font of same type)
	italic (artificial shearing)
};

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
 *  Properties relating to tab stops.
 */
class TabStopInfo
{
  public:
//	enum class TABTYPE {
//		Left,
//		Center,
//		Right,
//		Char
//	};
	double alignment; //left==0, center==50, right==100
	bool use_char;
	char tab_char_utf8[10]; //if use_char

	int positiontype; //automatic position, definite position, path
	double position; //if not path
	PathsData *path; //we assume this is a generally downward path

	TabStopInfo *next;

	TabStopInfo();
	virtual ~TabStopInfo();
};

TabStopInfo::TabStopInfo()
{
	alignment        = 0;
	use_char         = false;
	tab_char_utf8[0] = '\0';
	positiontype     = 0;
	position         = 0;
	path             = nullptr;
}

TabStopInfo::~TabStopInfo()
{
	if (path) path->dec_count();
}

/*! \class TabStops
 * Definition of tab placements for a paragraph style.
 */
class TabStops : public Value
{
  public:
	Laxkit::PtrStack<TabStopInfo> stops;

	int ComputeAt(double height, PtrStack<TabStopInfo> &tabstops); //for path based tab stops, update tabstops at this height in path space
};



//------------------------------ Style ---------------------------------

class Style : public ValueHash, public Laxkit::Resourceable
{
  public:
    Style *parent;

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
	bool style_is_temp;
	Style *style;

	//int type_hint; //like pure char, pure pp, container type

	StreamElement *treeparent;
	PtrStack<StreamElement> kids;

	PtrStack<StreamChunk> chunks; // has chunks only when we are a leaf element

	StreamElement(StreamElement *nparent, Style *nstyle);
	virtual ~StreamElement();
    virtual const char *whattype() { return "StreamElement"; }
};


//------------------------------ StreamChunk ---------------------------------

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

	virtual int NumBreaks() = 0;
	virtual int BreakInfo(int *type) = 0;
	virtual int Type() = 0;

	virtual StreamChunk *AddAfter(StreamChunk *chunk);
	virtual void AddBefore(StreamChunk *chunk);
};


//------------------------------ StreamBreak ---------------------------------

enum class StreamBreakTypes {
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
	int break_type; //newline (paragraph), column, section, page

	StreamBreak (int ntype) { type = ntype; }
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

	StreamImage(DrawableObject *nimg,StreamElement *pstyle);
	virtual ~StreamImage();
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

	StreamText(const char *txt,int n, StreamElement *pstyle);
	virtual ~StreamText();
	virtual int NumBreaks(); //either hyphen, spaces, pp
	virtual int BreakInfo(int index);
	virtual int Type() { return CHUNK_Text; }

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//------------------------------ Stream ---------------------------------

class Stream : public Laxkit::anObject, public Laxkit::DumpUtility
{
  public:
	char *id;
	char *file; 

	clock_t modtime;

	StreamElement *top;
	StreamChunk *chunk_start; //convenience pointer for leftmost leaf element

	Stream();
	virtual ~Stream();

	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_out (FILE *f,int indent,int what,LaxFiles::DumpContext *context);
};


//------------------------------ StreamImporter ---------------------------------

class StreamImporter : public FileFilter, public Value
{
  public:
  	StreamImporter();
  	virtual ~StreamImporter();

  	virtual const char *whattype() { return "StreamImporter"; }
	virtual const char *FileType(const char *first100bytes) = 0;
	virtual int In(const char *file, Laxkit::anObject *context, Laxkit::ErrorLog &log, const char *filecontents,int contentslen) = 0;
};


} // namespace Laidout

#endif

