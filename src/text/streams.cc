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
// Copyright (C) 2013 by Tom Lechner
//

#include "streams.h"
#include "../language.h"



namespace Laidout {


//----------------------------------- Base text related styles -----------------------------------

/*! Add to s if s!=NULL.
 */
Style *CreateDefaultCharStyle(Style *s)
{
	if (!s) s=new Style("Character");

	s->pushString("fontfamily","serif");
	s->pushString("fontstyle","normal");
	s->pushString("fontcolor","black");
	s->pushLength("fontsize","11pt");
	s->pushLength("kern","0%");

	return s;
}

/*! Add to s if s!=NULL.
 */
Style *CreateDefaultParagraphStyle(Style *s)
{
	if (!s) s=new Style("Paragraph");

	s->pushDouble("firstIndent",0.);
	s->pushDouble("leftIndent",0.);
	s->pushDouble("rightIndent",0.);
	s->pushDouble("spaceBefore",0.);
	s->pushDouble("spaceAfter",0.);
	s->pushDouble("spaceBetween",0.);

	return s;
}

//----------------------------------- Style -----------------------------------
/*! \class Style
 * \brief Class to hold cascading styles.
 */
class Style : public ValueHash
{
  public:
    Style *parent;

    Style();
    virtual ~Style();
    virtual const char *whattype() { return "Style"; }

    virtual Value *FindValue(int attribute_name);
    virtual int CascadeUp(Style *s);
    virtual Style *Collapse();
};

Style::Style()
{
    parent=NULL;
}

Style::~Style()
{
}

/*! Create and return a new Style cascaded upward from this.
 * Values in *this override values in parents.
 */
Style *Style::Collapse()
{
    Style *s=new Style();
    Style *f=this;
    while (f) {
        s->CascadeUp(f);
        f=f->parent;
    }
    return s;
}

/*! Adds to *this any key+value in s that are not already included in this->values. It does not replace values.
 * Note only checks against *this, not this->parent or this->kids.
 * Return number of items added.
 */
int Style::CascadeUp(Style *s)
{
    const char *str;
    int n=0;
    for (int c=0; c<s->n(); c++) {
        str=s->key(c);
        if (FindValue(str)) continue; //don't use if we have it already
        push(str, s->e(c));
        n++;
    }
    return n;
}

/*! Adds or replaces any key+value from s into this->values.
 * Note only checks against *this, not this->parent or this->kids.
 * Return number of items added.
 */
int Style::CascadeDown(Style *s)
{
    const char *str;
    int n=0;
    for (int c=0; c<s->n(); c++) {
        str=s->key(c);
        Set(str, s->values.e(c));
        n++;
    }
    return n;
}


//----------------------------------- StreamStyle -----------------------------------
/*! \class StreamStyle 
 * \brief Class to hold a stream broken down into a tree
 *
 * Please note this is different than general cascaded styles in that parents
 * AND kids exist as a specific hierarchy. Children are kept track of, and the order of children
 * depends on the associated stream. General styles don't care about particular
 * children, only parents.
 *
 * Streams need to traverse styles up and down when changing their contents. Leaves in
 * the tree are the actual content, and are derived from StreamChunk.
 */
class StreamStyle : public Style
{
  public:
	Style *style;
	StreamStyle *parent;
	RefPtrStack<StreamStyle> kids;

	StreamStyle();
	virtual ~StreamStyle();
    virtual const char *whattype() { return "StreamStyle"; }
};

StreamStyle::StreamStyle(StreamStyle *nparent)
{
	parent=nparent;
	style=NULL;
	parent_style=dynamic_cast<StreamStyle*>(parent);
}

StreamStyle::~StreamStyle()
{
	if (style) style->dec_count();
}


//----------------------------------- StreamChunk ------------------------------------

enum StreamChunkTypes {
	CHUNK_Text,
	CHUNK_Image,
	CHUNK_Break,
	CHUNK_MAX
};


/*! \class StreamChunk
 * \brief One part of a stream that can be considered to be all the same type.
 *
 * This can be, for instance, an image chunk, text chunk, a break, a non-printing anchor.
 *
 * These are explicitly leaf nodes in a StreamStyle tree, that point to other leaf nodes
 * for convenience.
 */
class StreamChunk : public StreamStyle
{
 public:
	StreamChunk *next, *prev;

	StreamChunk();
	virtual ~StreamChunk();
	virtual int NumBreaks()=0;
	virtual int BreakInfo(int *type)=0;
	virtual int Type()=0;

	virtual StreamChunk *Add(StreamChunk *chunk);
	virtual void AddBefore(StreamChunk *chunk);
};

StreamChunk::StreamChunk(StreamStyle *pstyle)
{
	parent=pstyle;
	parent_style=pstyle;
	if (parent_style) parent_style->kids.push(this);
	next=prev=NULL;
}

/* Default do nothing.
 */
StreamChunk::~StreamChunk()
{ ***
	//if (next) delete next;
}

/*! Assumes chunk->prev==NULL. Ok for chunk->next to not be null.
 * Inserts between this and this->next.
 *
 * If chunk->style==NULL, then use same style as *this. 
 *
 * Returns next most StreamChunk of chunk.
 */
StreamChunk *StreamChunk::Add(StreamChunk *chunk)
{ *** need to work out style management

	if (!chunk) return;
	if (!chunk->parent) chunk->parent=parent;

	StreamChunk *chunkend=chunk;
	while (chunkend->next) chunkend=chunkend->next;

	if (!next) { next=chunk; chunk->prev=this; }
	else {
		chunkend->next=next;
		if (chunkend->next) chunkend->next->prev=chunkend;
		next=chunk;
		chunk->prev=this;
	}

	return chunkend;
}

/*! Assumes chunk->prev==NULL. Ok for chunk->next to not be null.
 * Inserts between this and this->next.
 *
 * If chunk->style==NULL, then use same style as *this.
 */
void StreamChunk::AddBefore(StreamChunk *chunk)
{
	if (!chunk) return;
	if (!chunk->style) chunk->style=style;

	StreamChunk *chunkend=chunk;
	while (chunkend->next) chunkend=chunkend->next;

	if (prev) prev->next=chunk;
	chunk->prev=prev;

	chunkend->next=this;
	prev=chunkend;
}

//----------------------------------- StreamBreak ------------------------------------
typedef enum StreamBreakTypes {
    BREAK_Paragraph,
    BREAK_Column,
    BREAK_Section,
    BREAK_Page,
    BREAK_Tab,
    BREAK_MAX
};

class StreamBreak : public StreamChunk
{
	int break_type; //newline (paragraph), column, section, page

	StreamBreak (int ntype) { type=ntype; }
	virtual int NumBreaks() { return 0; }
	virtual int BreakInfo(int *type) { return 0; }
	virtual int Type() { return CHUNK_Break; }
};

//----------------------------------- StreamImage ------------------------------------
/*! \class StreamImage
 * \brief Type of stream component made of a single DrawableObject.
 *
 */
class StreamImage : public StreamChunk
{
 public:
	DrawableObject *img;

	StreamImage(DrawableObject *nimg,StreamStyle *pstyle);
	virtual ~StreamImage();
	virtual int NumBreaks() { return 0; }
	virtual int BreakInfo(int *type) { return 0; }
	virtual int Type() { return CHUNK_Image; }
};

StreamImage::StreamImage(DrawableObject *nimg,StreamStyle *pstyle)
  : StreamChunk(pstyle)
{
	img=nimg;
	if (img) img->inc_count();
}

StreamImage::~StreamImage()
{
	if (img) img->dec_count();
}


//----------------------------------- StreamText ------------------------------------
/*! \class StreamText
 * \brief Type of stream component made of text.
 *
 * One StreamText object contains a string of characters not including a newline.
 * This string is a single style of text. Any links, font changes, or anything else
 * that distinguishes this text from adjacent text requires a different StreamText.
 */
class StreamText : public StreamChunk
{
 public:
	int len; //number of bytes in text (excluding any '\0')
	char *text; //utf8 string

	StreamText(const char *txt,int n, StreamStyle *pstyle);
	virtual ~StreamText();
	virtual int NumBreaks();
	virtual int BreakInfo(int *type);
	virtual int Type() { return CHUNK_Text; }
};

StreamText::StreamText(const char *txt,int n, StreamStyle *pstyle)
  : StreamChunk(pstyle)
{
	if (n<0) {
		len=strlen(txt);
		text=newstr(txt,len);
	} else {
		text=newnstr(txt,n);
		len=n;
	}
}

StreamText::~StreamText()
{
	delete[] text;
}

int StreamText::ChunkInfo(double *width,double *height)
{ ***
}

//! Return the number of possible (not actual) breaks within the string of characters.
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
/*! For instance, in between letters in a word has almost no break potential,
 * hyphens have low break potential, spaces have a lot, but not as much
 * as line breaks.
 */
int StreamText::BreakInfo(int *type)
{ ***
}


//------------------------------------- Stream ----------------------------------
/*! \class Stream
 * \brief The head of StreamChunk objects.
 */

class Stream : public Laxkit::anObject, public Laxkit::DumpUtility
{
  public:
	char *id;
	char *file; 

	clock_t modtime;

	StreamStyle *defaultstyle;
	StreamChunk *chunks;

	Stream();
	virtual ~Stream();

	virtual void dump_in_atts (Attribute *att,int flag,Laxkit::anObject *context);
	virtual void dump_out_atts(Attribute *att,int what,Laxkit::anObject *context);
	virtual void dump_out (FILE *f,int indent,int what,Laxkit::anObject *context);
};


/*! for more standardized contexts, with bare minimum filename accessible.
 */
class FileIOContext
{
  public:
	char *filename;
};



Stream::Stream()
{
	modtime=0;
	file=NULL;
	id=NULL;

	defaultstye=NULL;
	chunks=NULL;
}

Stream::~Stream()
{
	delete[] file;
	delete[] id;

	if (defaultstyle) defaultstyle->dec_count();
	//chunks is not deleted, since they exist in defaultstyle tree
}

/*! Starting with defaultstyle, dump out the whole tree recursively with dump_out_recursive().
 */
void Stream::dump_out_atts(Attribute *att,int what,Laxkit::anObject *context)
{
	if (!chunks) return;

	if (!att) att=new Attribute;

	dump_out_recursive(att,defaultstyle,context);
}

void Stream::dump_out_recursive(Attribute *att,StreamStyle *style,Laxkit::anObject *context)
{
	ValueHash::dump_out_atts(att,0,context);

	Attribute *att2;
	for (int c=0; c<style->kids.n; c++) {
		att2=att->Subattribute("child:");
		dump_out_recursive(att2,style->kids.e[c],context);
	}
}

void Stream::dump_out (FILE *f,int indent,int what,Laxkit::anObject *context)
{
	Attribute att;
	dump_out_atts(&att, what,context);
	att.dump_out(f,indent);
}

void Stream::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context, Laxkit::ErrorLog &log)
{
	const char *name, *value;

	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"htmltext")) {
			ImportXML(value);

		} else if (!strcmp(name,"text")) {
			ImportText(value,-1);

		//} else if (!strcmp(name,"markdown")) {
		//	ImportMarkdown(value);

		} else if (!strcmp(name,"file")) {
			 //Try to read in a file, which can be formatted any which way.
			 //Scan for hints about:
			 //  format, such as txt, html, etc
			 //  importer, from list of know TextImporter objects
			 //  encoding, such as latin-1, utf8, etc
			 //
			const char *file=value;
			const char *format=NULL;
			const char *importer=NULL;
			const char *encoding=NULL;

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name =att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"format")) format=value;
				else if (!strcmp(name,"importer")) importer=value;
				else if (!strcmp(name,"encoding")) encoding=value;
			}

			int status=-2;
			if (importer) {
				 //find and try importer on file with format
				TextImporter *importer_obj=laidout->GetTextImporter(importer,format);
				log.AddWarning(_("Unknown importer %s, ignoring file %s"), importer,file);
				if (importer_obj) status=importer_obj->ImportStream(this, file, format, encoding);

			} else {
				 //no importer found, try some defaults
				if (format && !strcmp(format,"html")) ImportXMLFile(value);
				else {
					char *str=FileToStr(file, encoding);
					if (!isblank(str)) {
						if (!strcasecmp(format,"txt") || !strcasecmp(format,"text")) ImportText(str,-1);
						//else if (!strcmp(format,"markdown")) ImportMarkdown(value);
					}
					delete[] str;
				}
			}
		}
	}
}

/*! afterthis MUST be a chunk of *this or NULL. No verification is done.
 * If NULL, then add to end of chunks.
 */
int Stream::ImportText(const char *text, int n, StreamChunk *addto, bool after)
{
	if (n<0) n=strlen(text);
	
	StreamText *txt=new StreamText(text,n, NULL);

	if (afterthis) afterthis->Add(txt);
	else {
		if (!chunks) {
			 //blank stream! install first chunk...
			if (!defaultstyle) EstablishDefaultStyle();
		   	chunks=txt;
			chunks->parent_style=chunks->parent=defaultstyle;
	   	} else {
			 //append or prepend to addto
			StreamChunk *chunk=chunks;
			if (after) {
				while (chunk->next) chunk=chunk->next;
				chunk->Add(txt); //Add() and AddBefore() install a style
			} else chunk->AddBefore(txt);
		}
	}

	return 0;
}

/*! Read in the file to a string, then call ImportXML(). Updates this->file.
 */
int Stream::ImportXMLFile(const char *nfile)
{
	makestr(file,nfile);
	Attribute att;
	XMLFileToAttribute(&att,nfile,NULL);
	return ImportXMLAtt(att, NULL, defaultstyle);
}

/*! From string value, assume it is xml formatted text, and parse into StreamChunk objects, completely
 * replacing old stream chucks.
 */
int Stream::ImportXML(const char *value)
{
	delete chunks;
	chunks=NULL;

	long pos=0;
	Attribute att;
	XMLChunkToAttribute(&att, value, strlen(value), &pos, NULL, NULL);

	return ImportXMLAtt(att, NULL, defaultstyle);
}


StreamStyle *ProcessCSSBlock(newstyle, laststyle, cssvalue)
{
		//CSS:
		// selectors:    selector[, selector] { ... }
		//    E     -> any E
		//    E F   -> any F that is descended from an E
		//    E F G -> any G descended from an F, which must be descended from E
		//    E > F -> any F that is a direct child of E
		//    E:first-child -> E when E is first child of parent
		//    E:link E:visited E:active E:hover E:focus
		//    E:lang(c)  -> E when language is c
		//    E+F   -> any F occuring right after E
		//    E[foo] -> any with foo attribute
		//    E[foo="bar"] attribute equal to bar
		//    E[foo~="bar"] bar in foo attribute's list
		//    .class
		//    #id 
		//    p:first-line
		//    p:first-letter
		//    p:before { content: "Blah"; }
		//     content: normal | none | [ <string> | <uri> | <counter> | attr(<identifier>) | open-quote | close-quote | no-open-quote | no-close-quote ]+ | inherit
		//    p:after
		//
		//  font-family: Bookman, serif, ..... <-- use first one found
		//    generic:
		//      serif  sans-serif  cursive  fantasy  monospace
		//  font-variant: 	normal | small-caps | inherit
		//  font-style:  normal | italic | oblique | inherit 
		//  font-weight: normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
		//       normal==400, bold==700
		//  font-size: 	<absolute-size> | <relative-size> | <length> | <percentage> | inherit
		//     absolute-size == xx-small | x-small | small | medium | large | x-large | xx-large 
		//     relative-size == larger | smaller
		//  font: 	[ [ <'font-style'> || <'font-variant'> || <'font-weight'> ]? <'font-size'> [ / <'line-height'> ]? <'font-family'> ] | caption | icon | menu | message-box | small-caption | status-bar | inherit
		//  text-indent: 5%  <-  % of containing block, or abs
		//  text-align:  left | right | center | justify | inherit
		//  text-decoration:  	none | [ underline || overline || line-through || blink ] | inherit
		//  letter-spacing:  normal | <length> | inherit
		//  word-spacing:  normal | <length> | inherit  <- space in addition to default space
		//  text-transform:  capitalize | uppercase | lowercase | none | inherit
		//  white-space:  normal | pre | nowrap | pre-wrap | pre-line | inherit 
		//
		//lengths: ex = x-height, height of lowercase letters
		//         em = basically font size
		//   abs: in cm mm 
		//        pt = 1/72 inch
		//        pc = 1/6 inch, 12 pts
		//        px = .75 pt
		//  percents: 50%, but % items might refer to diff items, ie line-height: 120% means 120% of font-size
		//
		//  blah: url(http://...)
		//  blah: url(relative_url_to_this)
		//
		//colors:
		//  keywords:
		//		maroon #800000      red #ff0000   orange #ffA500  yellow #ffff00    olive #808000
		//		purple #800080  fuchsia #ff00ff    white #ffffff    lime #00ff00    green #008000
		//		  navy #000080     blue #0000ff     aqua #00ffff    teal #008080
		//  system colors: see http://www.w3.org/TR/CSS2/ui.html#system-colors
		//  color: #aabbcc  ==  color: #abc
		//  color: rgb(50%, 50%, 50%)
		//  color: rgb(255, 255, 255)  <- [0..255]
		//
		//boxes:  margin-border-padding-content
		//  margin-left  margin-right  margin-top  margin-bottom  margin: t r b l
		//   -> margins collapse with adjacent margins
		//  padding-left  padding-right  padding-top  padding-bottom  padding: t r b l
		//  border-left-width  border-right-width  border-top-width  border-bottom-width  border-width: t r b l
		//  border-left-color  padding-right-color  padding-top-color  padding-bottom-color  padding-color: t r b l,  <color> or transparent
		//  border-left-style  padding-right-style  padding-top-style  padding-bottom-style  padding-style: t r b l
		//   -> none hidden dotted dashed solid double groove ridge inset outset
		//  border-left  border-top  border-right  border-bottom  border: 	[ <border-width> || <border-style> || <'border-top-color'> ] | inherit
		//
		// list-style-type: disc | circle | square | decimal | decimal-leading-zero | lower-roman | upper-roman | lower-greek | lower-latin | upper-latin | armenian | georgian | lower-alpha | upper-alpha | none | inherit
		// list-style-image: 	<uri> | none | inherit
		// list-style-position:  	inside | outside | inherit
		// list-style:  	[ <'list-style-type'> || <'list-style-position'> || <'list-style-image'> ] | inherit
		//
		//default table styles for html:
		//  table    { display: table }
		//  tr       { display: table-row }
		//  thead    { display: table-header-group }
		//  tbody    { display: table-row-group }
		//  tfoot    { display: table-footer-group }
		//  col      { display: table-column }
		//  colgroup { display: table-column-group }
		//  td, th   { display: table-cell }
		//  caption  { display: table-caption }
		// 

	Attribute att;
	CSSBlockToAttribute(&att, cssvalue);

	const char *name;
	const char *value;
	for (int c=0; c<att.attributes.n; c++) {
		name =att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;

		if (!strcmp(name, "font-family")) {
			//  font-family: Bookman, serif, ..... <-- use first one found
			//    or generic:  serif  sans-serif  cursive  fantasy  monospace
			***

		//} else if (!strcmp(name, "font-variant")) {
		//	 //normal | small-caps | inherit
		//	 ***

		} else if (!strcmp(name, "font-style")) {
			 //normal | italic | oblique | inherit
			 int font_style=-1;
			 if (!strcmp(value,"inherit")) ;
			 else if (!strcmp(value,"normal")) font_style=0; 
			 else if (!strcmp(value,"italic")) font_style=1;
			 else if (!strcmp(value,"oblique")) font_style=1;
			 
		} else if (!strcmp(name, "font-weight")) {
			//normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
			int w=-1;
			if (!strcmp(value,"inherit")) ; //do nothing special
			else if (!strcmp(value,"normal")) w=400;
			else if (!strcmp(value,"bold")) w=700;
			else if (!strcmp(value,"bolder")) w=***; //120%
			else if (!strcmp(value,"lighter")) w=***; //80%
			else if (value[0]>='1' && value[0]<='9' && value[1]=='0' && value[2]=='0') {
				w=strtol(value,10,NULL);
			}
			***

		} else if (!strcmp(name, "font-size")) {
			 //  font-size: 	<absolute-size> | <relative-size> | <length> | <percentage> | inherit
			 //     absolute-size == xx-small | x-small | small | medium | large | x-large | xx-large 
			 //     relative-size == larger | smaller
			***

		} else if (!strcmp(name, "font")) {
			//  font: 	[ [ <'font-style'> || <'font-variant'> || <'font-weight'> ]? <'font-size'> [ / <'line-height'> ]? <'font-family'> ] | caption | icon | menu | message-box | small-caption | status-bar | inherit
			*** 

		} else if (!strcmp(name, "color")) {
			int color_what=-2;
			Color *color=CSSColorValue(value, &color_what);
			***

		//} else if (!strcmp(name, "text-indent")) {
		//} else if (!strcmp(name, "text-align")) {
		//} else if (!strcmp(name, "text-decoration")) {
		//} else if (!strcmp(name, "letter-spacing ")) {
		//} else if (!strcmp(name, "word-spacing")) {

		} else {
			DBG cerr << "Unimplemented attribute "<<name<<"!!"<<endl;
		}
	}

	return newstyle;
}

/*! If inherit, set ret_type=-1, return NULL.
 */
Color *CSSColorValue(const char *value, int *ret_type)
{
		//  system colors: see http://www.w3.org/TR/CSS2/ui.html#system-colors

	if (!strcmp(value,"inherit")) {
		*ret_type=-1;
		return NULL;
	}

	int r=-1,g=-1,b=-1;

	if      (!strcmp(value,"maroon"))  { r=0x80; g=0x00; b=0x00; }
	else if (!strcmp(value,"red"))     { r=0xff; g=0x00; b=0x00; }
	else if (!strcmp(value,"orange"))  { r=0xff; g=0xA5; b=0x00; }
	else if (!strcmp(value,"yellow"))  { r=0xff; g=0xff; b=0x00; }
	else if (!strcmp(value,"olive"))   { r=0x80; g=0x80; b=0x00; }
	else if (!strcmp(value,"purple"))  { r=0x80; g=0x00; b=0x80; }
	else if (!strcmp(value,"fuchsia")) { r=0xff; g=0x00; b=0xff; }
	else if (!strcmp(value,"white"))   { r=0xff; g=0xff; b=0xff; }
	else if (!strcmp(value,"lime"))    { r=0x00; g=0xff; b=0x00; }
	else if (!strcmp(value,"green"))   { r=0x00; g=0x80; b=0x00; }
	else if (!strcmp(value,"navy"))    { r=0x00; g=0x00; b=0x80; }
	else if (!strcmp(value,"blue"))    { r=0x00; g=0x00; b=0xff; }
	else if (!strcmp(value,"aqua"))    { r=0x00; g=0xff; b=0xff; }
	else if (!strcmp(value,"teal"))    { r=0x00; g=0x80; b=0x80; }
	else if (value[0]=='#')  {
		//  color: #aabbcc  ==  color: #abc
		***
		
	} else if (strstr(value,"rgb")==value) { 
		//  color: rgb(50%, 50%, 50%)
		//  color: rgb(255, 255, 255)  <- [0..255]
		***

	//} else if (***system colors) {
	//	//  system colors: see http://www.w3.org/TR/CSS2/ui.html#system-colors

	} else {
		*ret_type=-2;
		return NULL;
	}

	*ret_type=0;
	return color;
}

void Stream::ImportXMLAtt(Attribute *att, StreamChunk *&chunk, StreamStyle *laststyle)
{
	const char *name;
	const char *value;
	
	char *title=NULL;

	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		//
		//first process one off elements... currently: br wbr hr img tab "cdata:"
		//
		if (!strcmp(name,"br") || !strcmp(name,"tab")) {
			 //extended to have: <br section> <br chapter> <br column> <br page>
			 //also my tag: <tab>
	
			int type=BREAK_Paragraph;

			if (!strcmp(name,"tab")) type=BREAK_Tab;
			else {
				for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
					name =att->attributes.e[c]->attributes.e[c2]->name;
					value=att->attributes.e[c]->attributes.e[c2]->value;

					if (!strcmp(name,"section")) type=BREAK_Section;
					else if (!strcmp(name,"page")) type=BREAK_Page;
					else if (!strcmp(name,"column")) type=BREAK_Column;
					else if (!strcmp(name,"tab")) type=BREAK_Tab;
					else type=BREAK_Unknown;
				}
			}
			 
			if (type!=BREAK_Unknown) {
				StreamBreak *br=new StreamBreak(type);
				if (!chunk) { chunks=chunk=br; }
				else chunk=chunk->Add(br);
			} else {
				setlocale(***);
				log.AddMessage(ERROR_Warning, _("Unknown break %s"), value);
				setlocale(***);
			}
			continue;

		} else if (!strcmp(name,"wbr")) {
			//html: Word Break Opportunity
			*** if occurs between cdata blocks, then insert as weak break point in a combined StreamText

		} else if (!strcmp(name,"img")) {
			//<img width="100" height="100" alt=".." src=".."   style class id ... />
			
			double w=-1, h=-1;
			const char *alt=NULL, *src=NULL, *img_title=NULL;

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name =att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"width")) {
					*** watch for relative dims
					DoubleAttribute(value,&w);

				} else if (!strcmp(name,"height")) {
					*** watch for relative dims
					*** maybe allow em, ascent, descent, xh, %font-size
					DoubleAttribute(value,&h);

				} else if (!strcmp(name,"title")) {
					img_title=value;

				} else if (!strcmp(name,"alt")) {
					alt=value;

				} else if (!strcmp(name,"src")) {
					src=value;
				}
			}

			ImageData *img=datafactory->NewObject("ImageData");
			img->LoadImage(src,NULL, 0,0,0,0);
			if (alt) *** make as description;
			if (title) ***;
			if (h<=0) *** //make same size as font (ascent+descent)?
			*** if box style attributes, make a group with path border around image

			StreamImage *image=new StreamImage(img);
			if (!chunk) { chunks=chunk=image; }
			else chunk=chunk->Add(image);
			continue;

		} else if (!strcmp(name,"hr")) {
			 //<hr>  add a filled path rectangle?
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;
			continue;

		} else if (!strcmp(name,"cdata:")) {
			// *** &quot; and stuff

			StreamText *txt=new StreamText(NULL,0, laststyle);
			txt->SetFromEntities(value); *** //<- needs to compress whitespace, and change &stuff; to chars
			//char *text=ConvertEntitiesToChars(value);
			//delete[] text;

			if (!chunk) { chunks=chunk=txt; }
			else chunk=chunk->Add(txt);
			continue;
		}



		//
		//now process those that can have sub elements...
		//

		Attribute *elements=att->attributes.e[c]->find("content:");
		if (!elements) continue; //ignore if no actual content!
		// *** //make exception for <a> elements?


		StreamStyle *newstyle=NULL;

		 //Parse global attributes: style class id dir="ltr|rtl|auto" title
		for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
			name =att->attributes.e[c]->attributes.e[c2]->name;
			value=att->attributes.e[c]->attributes.e[c2]->value;

			if (!strcmp(name,"style")) {
				 //update newstyle with style found
				newstyle=ProcessCSSBlock(newstyle, laststyle, value);

			} if (!strcmp(name,"class")) {
				 //*** update newstyle with style found
				cerr << " *** class attribute not implemented! lazy programmer!"<<endl;

			} if (!strcmp(name,"id")) {
				 //*** update newstyle with style found
				cerr << " *** id attribute not implemented! lazy programmer!"<<endl;

			} if (!strcmp(name,"title")) {
				makestr(title,value);

			} if (!strcmp(name,"dir")) {
				 // flow direction: html has "ltr" "rtl" "auto"
				 // 				-> extend with all 8 lrtb, lrbt, tblr, etc?
	
				int bidi=-2;

				if (!strcasecmp(value,"ltr")) bidi=LAX_LRTB;
				else if (!strcasecmp(value,"rtl")) bidi=LAX_RLTB;
				else if (!strcasecmp(value,"auto")) bidi=-1;

				else if (!strcasecmp(value,"lrtb")) bidi=LAX_LRTB;
				else if (!strcasecmp(value,"ltbt")) bidi=LAX_LTBT;
				else if (!strcasecmp(value,"rltb")) bidi=LAX_RLTB;
				else if (!strcasecmp(value,"rlbt")) bidi=LAX_RLBT;
				else if (!strcasecmp(value,"tblr")) bidi=LAX_TBLR;
				else if (!strcasecmp(value,"tbrl")) bidi=LAX_TBRL;
				else if (!strcasecmp(value,"btlr")) bidi=LAX_BTLR;
				else if (!strcasecmp(value,"btrl")) bidi=LAX_BTRL;

				if (!newstyle) newstyle=new StreamStyle();
				if (bidi>=0) newstyle->set("bidi",bidi); ***
			}
		}


		if (!strcmp(name,"div") || !strcmp(name,"p")) {
			ImportXMLAtt(elements, chunk, newstyle);

			 //add a line break after div if not right after one
			 //*** should one be added before too? if first div should not be one
			if (chunk->Type()!=CHUNK_Break) {
				StreamBreak *br=new StreamBreak(BREAK_Paragraph);
				if (!chunk) { chunks=chunk=br; }
				else chunk=chunk->Add(br);
			}

		} else if (!strcmp(name,"span")) {
			ImportXMLAtt(elements, chunk, newstyle);

		} else if (!strcmp(name,"strong") || !strcmp(name,"b")) {
			//bold
			//search for a bold equivalent of current font
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"em") || !strcmp(name,"i")) {
			//italic
			//search for an italic equivalent of current font
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"blockquote")) {
			//*** //set paragraph indent
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"code") || !strcmp("tt")) {
			//*** //use a monospace font
				//if "code" then do like a div?
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"pre")) {
			// *** convert \n chars to <br> elements
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"font")) { //deprecated as of html4.1
			//<font size="-1">blah</font>
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;
			
		} else if (!strcmp(name,"a")) {
			// *** anchors.. use for html links, bookmark designations, citations?
			//<a name="..." href="..." class="..." id="..">...</a>
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"dl")) {
			//<dl> <dt> <dd>
			cerr << " *** need to implement dl lists!"<<endl;

		} else if (!strcmp(name,"ol")) {
			//<ol> <li> ...</li> </ol>
			cerr << " *** need to implement ol lists!"<<endl;

		} else if (!strcmp(name,"ul")) {
			//<ul> <li> ...</li> </ul>
			cerr << " *** need to implement ul lists!"<<endl;

		} else if (!strcmp(name,"table")) {
			//<table><tr><td colspan rowspan>
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (name[0]=='h' && name[1]>='1' && name[1]<='6' && name[2]=='\0') {
			int heading=name[1]-'1'+1;
			***
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"math")) {
			*** //mathml stuff
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else if (!strcmp(name,"texmath")) {
			*** math markup
			//<math> (for mathml) or not html: <latex> <tex> $latex stuff$  $$latex eqns$$
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;

		} else {
			cerr <<" *** "<<name<<" not implemented!! Rats!"<<endl;
		}


		if (title) { delete[] title; title=NULL; }
		if (newstyle) newstyle->dec_count();
	}
}


//------------------------------------- StreamAttachment ----------------------------------

/*! Held by DrawableObjects, these define various types of attachments of Stream objects to the parent
 * DrawableObject.
 */

class StreamAttachment
{
  public:
    DrawableObject *owner;
	int attachment_target; //area path, inset path, outset path, inset area

    Stream *stream;
    StreamCache *cache;

    StreamAttachment(DrawableObject *nobject, Stream *nstream);
    ~StreamAttachment();
};

StreamAttachment::StreamAttachment(DrawableObject *nobject, Stream *nstream)
{
    owner=nobject; //assume *this is a child of object

    stream=nstream;
    if (stream) stream->inc_count();

    cache=NULL;
}

StreamAttachment::~StreamAttachment()
{
	if (stream) stream->dec_count();
	if (cache) delete cache;
}

//------------------------------------- StreamCache ----------------------------------
/*! \class StreamCache
 * \brief Rendering info for Stream objects.
 *
 * This is stored on each StreamAttachment object that connects the streams to objects.
 */

class StreamCache : public SquishyBox
{
  public:
	clock_t modtime;
	StreamCache *next;

	StreamStyle *style;
	StreamChunk *chunk;
	long offset;
	long len;

	Affine transform;

	StreamCache();
	StreamCache(StreamChuck *ch, long noffset, long nlen);
	virtual ~StreamCache();
};

StreamCache::StreamCache(StreamChuck *ch, long noffset, long nlen)
{
	modtime=0;
	next=NULL;
	chunk=ch;
	if (ch) style=ch->style; else style=NULL;
	offset=noffset;
	len=nlen;
}

StreamCache::StreamCache()
{
	next=NULL;
	style=NULL;
	chunk=NULL;
	offset=len=0;
}

StreamCache::~StreamCache()
{
	if (next) delete next;
}


/*! Update cache for stream laid into target->GetInsetPath(), as blocked by appropriate objects in
 * the same page. Note that only page objects affect wrapping. Paper and limbo objects do not
 * affect wrapping.
 *
 * Overwrites anything in cache chain, adding more nodes if necessary, and deleting unused ones.
 * or create and return a new one if cache==NULL.
 */
StreamCache *RemapAreaStream(DrawableObject *target, StreamAttachment *attachment, int startchuck, int chunkoffset)
{ ***
	if (!attachment) return 1;
	
	Stream      *stream=attachment->stream;
	StreamCache *cache=attachment->cache;

	if (cache->modtime>0 && stream->modtime<cache->modtime) return 0; //probably set ok


	 //-------- compute possible breaks if necessary
	 ***


	 //----------compute layout area:
	 //  start with target inset path
	 //  We need to remove the wrap path of any lesser siblings and aunts/uncles from that path
	PathsData *area=target->GetInsetPath()->duplicate();

	DrawableObject *so=target;
	DrawableObject *o=target->parent;
	while (o) {
		for (int c=0; c<o->n() && o->e(c)!=so; c++) {
			area->CutOut(o->e(c)->GetWrapPath());
		}
		so=o;
		o=o->parent;
	}


	 //--------------compute actual breaks within area
	***
}


} // namespace Laidout

