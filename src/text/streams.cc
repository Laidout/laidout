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
// Copyright (C) 2013 by Tom Lechner
//

#include "streams.h"
#include "../language.h"



namespace Laidout {


//----------------------------------- LengthValue -----------------------------------


class LengthValue : public Value
{
  public:
	double value;
	enum LengthType {
		LEN_Number,
		LEN_Percent
	};

	LengthType type;
	Unit units;

	LengthValue(); 
};

LengthValue::LengthValue()
{
	value=0;
	type=LEN_Number;
}



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

	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual void dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_out (FILE *f,int indent,int what,LaxFiles::DumpContext *context);
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


//----------------------------------- StreamTreeElement -----------------------------------
/*! \class StreamTreeElement
 * Base class for StreamStyle and StreamChunk objects.
 */
class StreamTreeElement
{
  public:
	StreamTreeElement *treeparent;

	StreamTreeElement(StreamTreeElement *nparent);
	virtual ~StreamTreeElement();

	PtrStack<StreamElement> kids;
}

StreamTreeElement::StreamTreeElement(StreamTreeElement *nparent)
{
	treeparent=nparent;
}

StreamTreeElement::~StreamTreeElement()
{
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
class StreamStyle : public StreamTreeElement
{
  public:
	bool style_is_temp;
	Style *style;

	StreamStyle(StreamStyle *nparent, Style *nstyle);
	virtual ~StreamStyle();
    virtual const char *whattype() { return "StreamStyle"; }
};

StreamStyle::StreamStyle(StreamStyle *nparent, Style *nstyle)
  : StreamTreeElement(nparent)
{
	***
	style=nstyle;
	if (style) style->inc_count();
	style_is_temp=true;
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
 * for convenience. Anything in style is ignored. All actual style information should be contained in the parent tree.
 */
class StreamChunk : public StreamTreeElement
{
 public:
	StreamChunk *next, *prev;

	StreamChunk(StreamTreeElement *nparent);
	virtual ~StreamChunk();
	virtual int NumBreaks()=0;
	virtual int BreakInfo(int *type)=0;
	virtual int Type()=0;

	virtual StreamChunk *AddAfter(StreamChunk *chunk);
	virtual void AddBefore(StreamChunk *chunk);

	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context) = 0;
	virtual void dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context) = 0; //output label + content
	virtual void dump_out (FILE *f,int indent,int what,LaxFiles::DumpContext *context) = 0;
};

StreamChunk::StreamChunk(StreamTreeElement *nparent)
  : StreamTreeElement(nparent)
{
	next=prev=NULL;
}

/* Default do nothing.
 */
StreamChunk::~StreamChunk()
{
	//if (next) delete next; no deleting, as chunks must ALWAYS be contained somewhere in a single StreamTreeElement tree.
}

/*! Assumes chunk->prev==NULL. Ok for chunk->next to not be null.
 * Inserts between this and this->next.
 *
 * If chunk->style==NULL, then use same style as *this. 
 *
 * Returns next most StreamChunk of chunk.
 */
StreamChunk *StreamChunk::AddAfter(StreamChunk *chunk)
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
{ *** need to work out style management
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

	virtual void dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context) = 0; //output label + content
	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context) = 0;
};

void StreamBreak::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context) //output label + content
{
	if (break_type==BREAK_Paragraph)    att->push("break","pp");
	else if (break_type==BREAK_Column)  att->push("break","column");
	else if (break_type==BREAK_Section) att->push("break","section");
	else if (break_type==BREAK_Page)    att->push("break","page");
	else if (break_type==BREAK_Tab)     att->push("break","tab");
}

/*! NOTE! Uses whole attribute, not just kids. Meaning it compares att->value to various break types.
 */
void StreamBreak::dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if      (!strcmp(att->value,"pp"     )) break_type=BREAK_Paragraph;
	else if (!strcmp(att->value,"column" )) break_type=BREAK_Column;
	else if (!strcmp(att->value,"section")) break_type=BREAK_Section;
	else if (!strcmp(att->value,"page"   )) break_type=BREAK_Page;   
	else if (!strcmp(att->value,"tab"    )) break_type=BREAK_Tab;     

}


//----------------------------------- StreamImage ------------------------------------
/*! \class StreamImage
 * \brief Type of stream component made of a single DrawableObject.
 *
 * Note this can be ANY drawable object, not just images.
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

	virtual void dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context) = 0; //output label + content
	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context) = 0;
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

void StreamImage::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{
	***
}

void StreamImage::dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	***
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

	int numspaces;

	StreamText(const char *txt,int n, StreamStyle *pstyle);
	virtual ~StreamText();
	virtual int NumBreaks();
	virtual int BreakInfo(int *type);
	virtual int Type() { return CHUNK_Text; }

	virtual void dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context) = 0; //output label + content
	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context) = 0;
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
		while (*s && !isspace(*s)) s=utf8next(s);
		if (s==' ') {
			n++;
			s++; //assumes all white space is ' '. Assumed '\n' and '\t' previously converted to explicit breaks
		}
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

	StreamStyle *top;
	StreamChunk *chunks;

	Stream();
	virtual ~Stream();

	virtual void dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual void dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_out (FILE *f,int indent,int what,LaxFiles::DumpContext *context);
};


/*! for more standardized contexts, with bare minimum filename accessible.
 * maybe base on Boost file path classes for portability??
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

	top=NULL;
	chunks=NULL;
}

Stream::~Stream()
{
	delete[] file;
	delete[] id;

	if (top) top->dec_count();
	//chunks is not deleted, since they exist in top tree
}

/*! Starting with top, dump out the whole tree recursively with dump_out_recursive().
 *
 * <pre>
 *   streamdata
 *     style
 *       basedon laidout_default_style
 *       font-size     10
 *       line-spacing  110%
 *
 *       kids:
 *         text "first text"
 *         style ...
 *           kids:
 *             image /blah/blah.jpg
 *             text "Some text all up in here"
 *             dynamic context.page.label+"/"+string
 *             break line
 *             break column
 *             break section #or page
 *
 *   htmltext \\
 *     <span class="#laidout_default_style" style="font-size:10; line-spacing:110%;">first text<img file="/blah/blah.jpg"/>Some text all up in here</span>
 * </pre>
 */
void Stream::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!chunks) return;

	if (!att) att=new Attribute;

	Attribute *att2=att->Subatt("streamdata");
	dump_out_recursive(value, att2,top,context);
}

void Stream::dump_out_recursive(Attribute *att,StreamTreeElement *element,LaxFiles::DumpContext *context)
{
	StreamChunk *chunk=dynamic_cast<StreamChunk*>(element);
	if (chunk) {
		 //no kids, leaf node
		chunk->dump_out_atts(att,0,context);
		return;
	}


	StreamStyle *sstyle=dynamic_cast<StreamStyle*>(element);
	if (!sstyle) {
		DBG cerr <<"Warning! unrecognized StreamTreeElement object!"<<endl;
		return;
	}

	Attribute *att2;
	if (sstyle->style) {
		att->push("style",style->name); //is a style recorded somewhere
	}

	if (sstyle->kids.n) {
		att2=att->Subattribute("child:");
		for (int c=0; c<sstyle->kids.n; c++) {
			dump_out_recursive(att2,sstyle->kids.e[c],context);
		}
	}
}

void Stream::dump_out (FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what,context);
	att.dump_out(f,indent);
}

void Stream::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context, Laxkit::ErrorLog &log)
{
	const char *name, *value;

	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"streamdata")) {
			 //default Laidout format.... maybe should just use htmltext for simplicity??
			***
			dump_in_att_stream(att->attributes.e[c], context, log);

		} else if (!strcmp(name,"htmltext")) {
			ImportXML(value);

		} else if (!strcmp(name,"text")) {
			ImportText(value,-1);

		} else if (!strcmp(name,"markdown")) {
			ImportMarkdown(value);

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
			if (format && !strcasecmp(format,"txt")) format="text";

			int status=-2;
			if (importer) {
				 //find and try importer on file with format
				TextImporter *importer_obj=laidout->GetTextImporter(importer,format);
				if (importer_obj) status=importer_obj->ImportStream(this, file, format, encoding);
				else {
					status=1;
					log.AddWarning(_("Unknown importer %s, ignoring file %s"), importer,file);
				}

			} else { //importer not found
				 //try to guess from extension
				if (!format) {
					const char *s=hasExtension(file,"txt");
					if (s) format="text";
					if (!format) {
						s=hasExtension(file,"html");
						if (s) format="html";
					}
					if (!format) {
						s=hasExtension(file,"md");
						if (s) format="markdown";
					}
				}
				if (!format) format="text";

				 //no importer found, try some defaults based on format
				if (!strcmp(format,"html")) ImportXMLFile(value);
				else if (!strcmp(format,"markdown")) ImportMarkdown(value,-1);
				else { //text
					char *str=FileToStr(file, encoding); // *** must convert to utf8
					if (!isblank(str)) {
						if (!strcasecmp(format,"text")) ImportText(str,-1);
					}
					delete[] str;
				}
			}
		}
	}
	modtime=times(NULL);
}

int Stream::ImportMarkdown(const char *text, int n, StreamChunk *addto, bool after)
{
	cerr << " *** must implement a markdown importer! defaulting to text"<<endl;
	return ImportText(text,n,addto,after);
}

/*! afterthis MUST be a chunk of *this or NULL. No verification is done.
 * If NULL, then add to end of chunks.
 *
 * Return 0 for success or nonzero for error.
 */
int Stream::ImportText(const char *text, int n, StreamChunk *addto, bool after)
{
	if (!text) return 0;
	if (n<0) n=strlen(text);
	if (n==0) return 0;
	
	StreamText *txt=new StreamText(text,n, NULL);

	if (addto) {
		if (after) addto->AddAfter(txt);
		else {
			addto->AddBefore(txt);
			if (addto==chunks) chunks=addto; //reposition head
		}
	} else {
		if (!chunks) {
			 //blank stream! install first chunk...
			if (!top) EstablishDefaultStyle();
		   	chunks=txt;
			chunks->Reparent(top);

	   	} else {
			 //append or prepend to addto
			if (!addto) {
				 //prepend to beginning of all, or append to end of all
				addto=chunks;
				if (after) while (addto->next) addto=addto->next;
			}
			if (after) {
				addto->AddAfter(txt); //AddAfter() and AddBefore() install a style
			} else {
				chunk->AddBefore(txt);
				if (addto==chunks) chunks=addto; //reposition head
			}
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
	return ImportXMLAtt(att, NULL, top);
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

	return ImportXMLAtt(att, NULL, top);
}


Style *ProcessCSSBlock(Style *style, const char *cssvalue)
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
		//  line-height: normal | <number> | <length> | <percentage> | none
		//
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

	if (!style) style=new Style;
	Attribute att;
	CSSBlockToAttribute(&att, cssvalue);

	const char *variant=NULL;
	const char *familylist=NULL;
	int weight=400;
	int italic=-1;

	const char *name;
	const char *value;
	for (int c=0; c<att.attributes.n; c++) {
		name =att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;

		if (!strcmp(name, "font-family")) {
			//  font-family: Bookman, serif, ..... <-- use first one found
			//    or generic:  serif  sans-serif  cursive  fantasy  monospace
			familylist=value;			

		} else if (!strcmp(name, "font-variant")) {
			 //normal | small-caps | inherit
			 if (!strcmp(value,"small-caps")) variant=value;

		} else if (!strcmp(name, "font-style")) {
			 //normal | italic | oblique | inherit
			if (!strcmp(value,"inherit")) ;
			else if (!strcmp(value,"normal"))  italic=0; 
			else if (!strcmp(value,"italic"))  italic=1;
			else if (!strcmp(value,"oblique")) italic=1;

			if (font_style>0) style->pushInteger("font-style", font_style);
			 
		} else if (!strcmp(name, "font-weight")) {
			//normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
			const char *endptr;
			weight = CSSFontWeight(value, endptr);
			value = endptr;

			if (weight > 0) style->pushInteger("font-weight", w);

		} else if (!strcmp(name, "font-size")) {
			 //  font-size: 	<absolute-size> | <relative-size> | <length> | <percentage> | inherit
			 //     absolute-size == xx-small | x-small | small | medium | large | x-large | xx-large 
			 //     relative-size == larger | smaller
			double v=-1;
			int type=0; //0 for absolute size
			Unit units;

			if (!strcmp(value,"inherit")) ; //do nothing special

			 //named absolute sizes, these are relative to some platform specific table of numbers:
			else if (!strcmp(value,"xx-small")) v=.5;
			else if (!strcmp(value,"x-small" )) v=.75;
			else if (!strcmp(value,"small"   )) v=.8;
			else if (!strcmp(value,"medium"  )) v=1;
			else if (!strcmp(value,"large"   )) v=1.2;
			else if (!strcmp(value,"x-large" )) v=1.4;
			else if (!strcmp(value,"xx-large")) v=1.7;

		   	 //for relative size
			else if (!strcmp(value,"larger")) { v=1.2; type=1; }
			else if (!strcmp(value,"smaller")) { v=.8; type=1; }

			 //percentage
			else if (strchr(value,"%")) {
				DoubleAttribute(value,&v,NULL); //relative size
				type=1;

			 //length
			} else {
				LengthAttribute(value, &v, &units, NULL);
			}

			if (v>=0) style->pushLength(v,units,type);

		} else if (!strcmp(name, "font")) {
			//  font: 	[ [ <'font-style'> || <'font-variant'> || <'font-weight'> ]? <'font-size'> [ / <'line-height'> ]? <'font-family'> ] | caption | icon | menu | message-box | small-caption | status-bar | inherit
			*** 

			if (!strcmp(value,"inherit")) {
				//do nothing
			} else if (!strcmp(value,"caption")) {
			} else if (!strcmp(value,"icon")) {
			} else if (!strcmp(value,"menu")) {
			} else if (!strcmp(value,"message-box")) {
			} else if (!strcmp(value,"small-caption")) {
			} else if (!strcmp(value,"status-bar")) {
			} else {

				const char *endptr=NULL;
				int font_style=CSSFontStyle(value, &endptr);
				char *font_variant=NULL;
				int font_weight=-1;

				if (endptr==value) {
					font_variant=CSSFontVariant(value, &endptr);
					if (endptr==value) {
						font_weight=CSSFontWeight(value, &endptr);
					} else value=endptr+1;
				} else value=endptr+1;
				
				Value *fontsize=CSSFontSize(value,endptr);
				if (endptr==value) {
					*** error! font-size is required
				} else {
				}
			}

		} else if (!strcmp(name, "color")) {
			int color_what=-2;
			int error_ret=0;
			Color *color=CSSColorValue(value, &color_what);
			if (color) {
				style->set("color", color);
			}

		} else if (!strcmp(name, "line-height")) {
			 //line-height: normal | <number>  | <length> | <percentage> | none 
			 //                      <number> == (the number)*(font-size) 
			 //                      percent is of font-size   
			 //                      none == font-size
			if (!strcmp(value,"none")) {
				//should be 100% of font-size
			} else {
				LengthValue *v=ParseLengthOrPercent(value, "font-size", NULL);
				if (v) {
					style->set("line-height",v,1);
				} else {
					log.AddWarning(_("bad css attribute %s: %s"), "line-height",value);
				}
			}

		//} else if (!strcmp(name, "text-indent")) {
		//} else if (!strcmp(name, "text-align")) {
		//} else if (!strcmp(name, "text-decoration")) {  //<text-decoration-line> || <text-decoration-style> || <text-decoration-color>
		//      text-decoration-line:     none | [ underline || overline || line-through || blink ]
		//		text-underline-position:  auto | [ under || [ left | right ] ] 
		//		text-decoration-skip:     none | [ objects || spaces || ink || edges || box-decoration ]
		//      text-decoration-style:    solid | double | dotted | dashed | wavy 
		//      text-decoration-color:    <color>
		//
		//} else if (!strcmp(name, "letter-spacing ")) {   //  letter-spacing:  normal | <length> | inherit
		//} else if (!strcmp(name, "word-spacing")) {      //  word-spacing:  normal | <length> | inherit  <- space in addition to default space
		//} else if (!strcmp(name, "text-transform")) {  text-transform:  capitalize | uppercase | lowercase | none | inherit
		//} else if (!strcmp(name, "white-space")) {  white-space:  normal | pre | nowrap | pre-wrap | pre-line | inherit 

		//boxes:  margin-border-padding-content
		//  margin-left  margin-right  margin-top  margin-bottom  margin: t r b l
		//   -> margins collapse with adjacent margins
		//  padding-left  padding-right  padding-top  padding-bottom  padding: t r b l
		//  border-left-width  border-right-width  border-top-width  border-bottom-width  border-width: t r b l
		//  border-left-color  padding-right-color  padding-top-color  padding-bottom-color  padding-color: t r b l,  <color> or transparent
		//  border-left-style  padding-right-style  padding-top-style  padding-bottom-style  padding-style: t r b l
		//   -> none hidden dotted dashed solid double groove ridge inset outset
		//  border-left  border-top  border-right  border-bottom  border: 	[ <border-width> || <border-style> || <'border-top-color'> ] | inherit
		
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


		} else {
			DBG cerr << "Unimplemented attribute "<<name<<"!!"<<endl;
		}
	}

	if (fontfamily) {
		LFont *font=MatchCSSFont(fontfamily, italic, variant, weight);
		if (font) style->push("font",font);
	}

	return style;
}

Value *ParseLengthOrPercent(const char *value, const char *relative_to, const char **endptr)
{
	*** em, ex, px, and usual cm, etc
	----
	double l=-1;
	char *endptr=NULL;
	DoubleAttribute(value,&l,&endptr);
	if (endptr==value) {
		//error! can't use this!
	} else {
		if (endptr && *endptr=='%') {
			PercentageValue *v=new PercentageValue(l,"font-size");
			style->set("line-height", v);
		} else {

		}
	}
}


/*! <pre>
 *  normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
 *  </pre>
 */
int CSSFontWeight(const char *value, const char *&endptr)
{
	int weight=-1;
	if (!strncmp(value,"inherit",7))       { endptr = value+7; } //do nothing special
	else if (!strncmp(value,"normal",6))   { endptr = value+6; weight=400; }
	else if (!strncmp(value,"bold",4))     { endptr = value+4; weight=700; }
	else if (!strncmp(value,"bolder", 6))  { endptr = value+6; weight=900; } //120% ?
	else if (!strncmp(value,"lighter", 7)) { endptr = value+7; weight=300; } //80% ? 
	else if (value[0]>='1' && value[0]<='9') { //scan in any integer... not really css compliant, but what the hay
		weight = strtol(value,10,&endptr);
	} else endptr=value;

	return weight;
}

LFont *MatchCSSFont(const char *font_family, int italic, const char *variant, int weight)
{
	***
	// font_family is the css attribute:
	//  font-family: Bookman, serif, ..... <-- use first one found
	//    or generic:  serif  sans-serif  cursive  fantasy  monospace
	LFont *font=NULL;
	const char *v;
	while (value && *value) {
		while (isspace(*value)) value++;
		v=value;
		while (*v && *v!=',' && !isspace(v)) v++;
		fontlist.push(newnstr(value,v-value+1));
		font=MatchFont(value, v-value);
		if (font) break;

		while (isspace(*v)) v++;
		if (*v==',') v++;
		value=v;
	}

	return font;
}

/*! If inherit, set ret_type=-1, return NULL.
 * If there was an error converting to a color, ret_type=-2, return NULL.
 * Else return a new Color, and ret_type==0.
 */
Color *CSSColorValue(const char *value, int *ret_type)
{
	//css3 has 147 named colors
	//css3 has rgba, and hsl/hsla
	//  not implemented: x11 colors
	//  not implemented: system colors: see http://www.w3.org/TR/CSS2/ui.html#system-colors

	if (!strcmp(value,"inherit")) {
		*ret_type=-1;
		return NULL;
	}

	Color *color=NULL;
	int r=-1,g=-1,b=-1, a=0xff;

	 //css named colors
	if      (!strcmp(value,"white"))   { r=0xff; g=0xff; b=0xff; }
	else if (!strcmp(value,"black"))   { r=0x00; g=0x00; b=0x00; }
	else if (!strcmp(value,"maroon"))  { r=0x80; g=0x00; b=0x00; }
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

	else if (!strcmp(value,"cyan"))    { r=0x00; g=0xff; b=0xff; } //not css, but is x11 color, widely accepted

	if (r>=0) color=newRGBColor(r,g,b,255, 255);
	else if (!color) ColorAttribute(value, &color, error_ret);

	if (!color) {
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
				else chunk=chunk->AddAfter(br);
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
			else chunk=chunk->AddAfter(image);
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
			else chunk=chunk->AddAfter(txt);
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
				else chunk=chunk->AddAfter(br);
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
 * DrawableObject, as well as cached rendering info in a StreamCache.
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

