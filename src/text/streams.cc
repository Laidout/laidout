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


//----------------------------------- CharacterStyle -----------------------------------

class CharacterStyle : public Style
{
  public:
	LFont *font;

	Color *color; //including alpha
	Color *bg_color; //highlighting

	double space_shrink;
	double space_grow;  //as percent of overriding font size
	double inner_shrink, inner_grow; //char spacing within words

	 //basic positioning info
	double kerning_before;
	double kerning_after;
	double h_scaling, v_scaling; //contentious glyph scaling

	 //more thorough text effects
	PtrStack<TextEffect> glyph_effects; //adjustments to characters or glyphs that changes metrics, making them different shapes or chars, like small caps, all caps, no caps, scramble, etc
	PtrStack<TextEffect> below_effects; //additions like highlighting, that exist under the glyphs
	PtrStack<TextEffect>  char_effects; //transforms to glyphs that do not change metrics, fake outline, fake bold, etc
	PtrStack<TextEffect> above_effects; //lines and such drawn after the glyphs are drawn

	CharacterStyle();
	virtual ~CharacterStyle();

	virtual void ParseFrom(StreamChunk *chunk);

	static Style *Default(Style *s = nullptr);
};

/*! Add to s if s!=nullptr.
 */
Style *CharacterStyle::Default(Style *s)
{
	if (!s) s = new Style("Character");

	s->pushString("fontfamily","serif");
	s->pushString("fontstyle","normal");
	s->pushString("fontcolor","black");
	s->pushLength("fontsize","11pt");
	s->pushLength("kern","0%");

	return s;
}


//----------------------------------- ParagraphStyle -----------------------------------

class ParagraphStyle : public Style
{
  public:
	int hyphens; //whether to have them or not
	double h_width,h_shrink,h_grow, h_penalty; //hyphen metrics, still use the font's character, though

	int gap_above_type; //0 for  % of font size, 1 for absolute number
	double gap_above;

	int gap_after_type; //0 for  % of font size, 1 for absolute number
	double gap_after;

	int gap_between_type; //0 for  % of font size, 1 for absolute number
	double gap_between_lines;


	int lines_of_first_indent; //default is 1
	double first_indent;
	double normal_indent;
	double far_margin; //for left to right, this would be the right margin for instance

	int tab_strategy; //regular intervals, by lines
	TabStops *tabs;

	int justify_type; //0 for allow widow-like lines, 1 allow widows but do not fill space to edges, 2 for force justify all
	double justification; // (0..100) left=0, right=100, center=50, full justify
	int flow_direction;
	
	PtrStack<TextEffect> conditional_effects; //like initial drop caps, first line char style, etc

	CharacterStyle *default_charstyle;

	ParagraphStyle();
	virtual ~ParagraphStyle();

	virtual void ParseFrom(StreamChunk *chunk);

	static Style *Default(Style *s = nullptr);
};

/*! Add to s if s!=nullptr. Else return a new Style.
 */
Style *ParagraphStyle::Default(Style *s)
{
	if (!s) s = new Style("Paragraph");

	s->pushDouble("firstIndent",0.);
	s->pushDouble("leftIndent",0.);
	s->pushDouble("rightIndent",0.);
	s->pushDouble("spaceBefore",0.);
	s->pushDouble("spaceAfter",0.);
	s->pushDouble("spaceBetween",0.);

	return s;
}



//----------------------------------- LengthValue -----------------------------------


class LengthValue : public Value
{
  public:
	double v;
	double v_tr; //context dependent computed value. Not set internally.

	enum LengthType {
		LEN_Number,
		LEN_Percent_Parent,
		LEN_Percent_Paper
	};

	LengthType type;

	Unit units;

	LengthValue(); 
	LengthValue(double val, LengthType len_type)
	LengthValue(const char *val);
};

LengthValue::LengthValue()
{
	value = 0;
	type = LEN_Number;
}

LengthValue::LengthValue(const char *val)
  : LengthValue()
{
	if (isblank(val)) return;
	char *endptr = nullptr;
	value = strtod(val, &endptr);
	if (endptr != val) {
		while (isspace(*endptr)) endptr++;
		if (*endptr == '%') {
			type = LEN_Percent;
		} else {
			*** parse units
		}
	}
}



//----------------------------------- Style -----------------------------------
/*! \class Style
 * \brief Class to hold cascading styles.
 *
 * Style objects are either temporary objects embedded in StreamStyle, or they are resources.
 */

Style::Style()
{
    parent = nullptr;
}

Style::~Style()
{
}

/*! Create and return a new Style cascaded upward from this.
 * Values in *this override values in parents.
 */
Style *Style::Collapse()
{
    Style *s = new Style();
    Style *f = this;
    while (f) {
        s->MergeFromMissing(f);
        f = f->parent;
    }
    return s;
}

/*! Adds to *this any key+value in s that are not already included in this->values. It does not replace values.
 * Note ONLY checks against *this, NOT this->parent or this->kids.
 * Return number of items added.
 *
 * So, say you have:
 *     s:
 *       bold: true
 *       color: red
 *     this: 
 *       bold: false
 *       width: 5
 *
 * you will end up with:
 *     this:
 *       bold: false
 *       width: 5
 *       color: red
 */
int Style::MergeFromMissing(Style *s)
{
    const char *str;
    int n = 0;
    for (int c = 0; c < s->n(); c++) {
        str = s->key(c);
        if (findIndex(str) >= 0) continue; //don't use if we have it already
        push(str, s->e(c));
        n++;
    }
    return n;
}

/*! Adds or replaces any key+value from s into this->values.
 * Note ONLY checks against *this, NOT this->parent or this->kids.
 * Return number of items added.
 *
 * So, say you have:
 *     s:
 *       bold: true
 *       color: red
 *     this: 
 *       bold: false
 *       width: 5
 *
 * you will end up with:
 *     this:
 *       bold: true
 *       width: 5
 *       color: red
 */
int Style::MergeFrom(Style *s)
{
    const char *str;
    int n = 0;
    for (int c=0; c<s->n(); c++) {
        str = s->key(c);
        push(str, s->values.e(c)); //adds or replaces
        n++;
    }
    return n;
}


//----------------------------------- StreamStyle -----------------------------------
/*! \class StreamStyle 
 * \brief Class to hold a stream broken down into a tree.
 * 
 * Please note this is different than general cascaded styles in that parents
 * AND kids exist as a specific hierarchy. Children are kept track of, and the order of children
 * depends on the associated stream. General styles don't care about particular
 * children, only parents.
 *
 * Streams need to traverse styles up and down when changing their contents. Leaves in
 * the tree are the actual content, and are derived from StreamChunk.
 *
 * These are very much like XML blocks, in that style holds the element attributes,
 * and kids holds the element content. It differs from XML in that Style implements
 * a lot of management of styles.
 *
 */

StreamStyle::StreamStyle(StreamStyle *nparent, Style *nstyle)
  : StreamTreeElement(nparent)
{
	style = nstyle;
	if (style) style->inc_count();
	style_is_temp = true;
}

StreamStyle::~StreamStyle()
{
	if (style) style->dec_count();
}


//----------------------------------- StreamChunk ------------------------------------



/*! \class StreamChunk
 * \brief One part of a stream that can be considered to be all the same type.
 *
 * This can be, for instance, an image chunk, text chunk, a break, a non-printing anchor.
 *
 * These are explicitly leaf nodes in a StreamStyle tree, that point to other leaf nodes
 * for convenience. Anything in style is ignored. All actual style information should be contained in the parent tree.
 */

StreamChunk::StreamChunk(StreamTreeElement *nparent)
{
	parent = nparent;
	next = prev = nullptr;
}

/* Default do nothing.
 */
StreamChunk::~StreamChunk()
{
	//if (next) delete next; no deleting, as chunks must ALWAYS be contained somewhere in a single StreamTreeElement tree.
}

/*! Insert a currently unstyled list of chunks after ourself.
 * 
 * Assumes chunk->prev == nullptr. Ok for chunk->next to not be nullptr.
 * Inserts between this and this->next.
 *
 * If chunk->style == nullptr, then use same style as *this. 
 * Otherwise, integrate the contained styles into the StreamElement tree.
 *
 * Returns next most StreamChunk of chunk.
 */
StreamChunk *StreamChunk::AddAfter(StreamChunk *chunk)
{ *** need to work out style management

	if (!chunk) return;
	if (!chunk->parent) chunk->parent = parent;

	StreamChunk *chunkend = chunk;
	while (chunkend->next) chunkend = chunkend->next;

	if (!next) { next=chunk; chunk->prev = this; }
	else {
		chunkend->next = next;
		if (chunkend->next) chunkend->next->prev = chunkend;
		next = chunk;
		chunk->prev = this;
	}

	return chunkend;
}

/*! Assumes chunk->prev==nullptr. Ok for chunk->next to not be nullptr.
 * Inserts between this and this->next.
 *
 * If chunk->style==nullptr, then use same style as *this.
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


LaxFiles::Attribute *StreamBreak::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
{
	if (break_type == BREAK_Paragraph)    att->push("break","pp");
	else if (break_type == BREAK_Column)  att->push("break","column");
	else if (break_type == BREAK_Section) att->push("break","section");
	else if (break_type == BREAK_Page)    att->push("break","page");
	else if (break_type == BREAK_Tab)     att->push("break","tab");
	else if (break_type == BREAK_Weak)    att->push("break","weak");
	else if (break_type == BREAK_Hyphen)  att->push("break","hyphen");
}

/*! NOTE! Uses whole attribute, not just kids. Meaning it compares att->value to various break types.
 */
void StreamBreak::dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if      (!strcmp(att->value,"pp"     )) break_type = BREAK_Paragraph;
	else if (!strcmp(att->value,"column" )) break_type = BREAK_Column;
	else if (!strcmp(att->value,"section")) break_type = BREAK_Section;
	else if (!strcmp(att->value,"page"   )) break_type = BREAK_Page;
	else if (!strcmp(att->value,"tab"    )) break_type = BREAK_Tab;
	else if (!strcmp(att->value,"weak"   )) break_type = BREAK_Weak;
	else if (!strcmp(att->value,"hyphen" )) break_type = BREAK_Hyphen;
}


//----------------------------------- StreamImage ------------------------------------
/*! \class StreamImage
 * \brief Type of stream component made of a single DrawableObject.
 *
 * Note this can be ANY drawable object, not just images.
 */

StreamImage::StreamImage(DrawableObject *nimg,StreamStyle *pstyle)
  : StreamChunk(pstyle)
{
	img = nimg;
	if (img) img->inc_count();
}

StreamImage::~StreamImage()
{
	if (img) img->dec_count();
}

LaxFiles::Attribute *StreamImage::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
{
	if (!img) {
		att->push("object");
		return att;
	}
	Attribute *att2 = att->PushSubatt("object", img->whattype());
	img->dump_out_atts(att2, what, context);
	return att;
}

void StreamImage::dump_in_atts (Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	DrawableObject *newimg = DrawableObject::CreateFromAttribute(att, att->value, context);

	if (img) img->dec_count();
	img = newimg;
}



//----------------------------------- StreamText ------------------------------------
/*! \class StreamText
 * \brief Type of stream chunk made only of simple text.
 *
 * One StreamText object contains a string of characters not including a newline.
 * This string is a single style of text. Any links, font changes, or anything else
 * that distinguishes this text from adjacent text requires a different StreamText.
 */

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

//! Return the number of possible (not actual) breaks within the string of characters.
/*! This can include hyphen points, spaces, or newlines.
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
int StreamText::BreakInfo(int index)
{ ***
}


//------------------------------------- Stream ----------------------------------
/*! \class Stream
 * \brief Hold a stream of things, usually text.
 * 
 * These are essentially heads of StreamStyle + StreamChunk objects.
 */


Stream::Stream()
{
	modtime = 0;
	file    = nullptr;
	id      = nullptr;

	top    = nullptr;
	chunks = nullptr;
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
LaxFiles::Attribute *Stream::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
{
	if (!chunks) return;

	if (!att) att = new Attribute;

	Attribute *att2 = att->Subatt("streamdata");
	dump_out_recursive(value, att2,top,context);
}

void Stream::dump_out_recursive(Attribute *att,StreamTreeElement *element,LaxFiles::DumpContext *context)
{
	StreamChunk *chunk = dynamic_cast<StreamChunk*>(element);
	if (chunk) {
		 //no kids, leaf node
		chunk->dump_out_atts(att,0,context);
		return;
	}


	StreamStyle *sstyle = dynamic_cast<StreamStyle*>(element);
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
			 //  importer, from list of know StreamImporter objects
			 //  encoding, such as latin-1, utf8, etc
			 //
			 const char *file     = value;
			 const char *format   = nullptr;
			 const char *importer = nullptr;
			 const char *encoding = nullptr;

			 for (int c2 = 0; c2 < att->attributes.e[c]->attributes.n; c2++) {
				 name  = att->attributes.e[c]->attributes.e[c2]->name;
				 value = att->attributes.e[c]->attributes.e[c2]->value;

				 if (!strcmp(name, "format"))
					 format = value;
				 else if (!strcmp(name, "importer"))
					 importer = value;
				 else if (!strcmp(name, "encoding"))
					 encoding = value;
			}
			if (format && !strcasecmp(format,"txt")) format="text";

			int status = -2;

			if (importer) {
				 //find and try importer on file with format
				StreamImporter *importer_obj = laidout->GetStreamImporter(importer,format);
				if (importer_obj) status = importer_obj->ImportStream(this, file, format, encoding);
				else {
					status = 1;
					log.AddWarning(_("Unknown importer %s, ignoring file %s"), importer, file);
				}

			} else { //importer not found
				 //try to guess from extension
				if (!format) {
					const char *ext = lax_extension(file);

					if (!ext)
						format = "text";
					else if (!strncasecmp(ext, "html"))
						format = "html";
					else if (!strncasecmp(ext, "md"))
						format = "markdown";
				}

				if (!format) format = "text";

				int len = 0;
				char *contents = read_in_whole_file(file, &len);
				if (!contents) {
					log.AddWarning(0,0,0, _("Cannot load stream file %s!"), file);

				} else {
					 //no importer found, try some defaults based on format
					if (!strcmp(format, "html")) {
						ImportXMLFile(contents, len);

					} else if (!strcmp(format, "markdown")) {
						ImportMarkdown(contents, len);

					} else { //text
						char *str = FileToStr(file, encoding); // *** must convert to utf8
						if (!isblank(str)) {
							ImportText(contents, len);
						}
						delete[] str;
					}

				}
			}
		}
	}
	tms tms_;
	modtime = times(&tms_);
}


// Headings:  # blah   .. recommended to have space after the hash
//            ## blah   ... up to 6 #s
//            # Heading {#custom-id}  <- makes like <h1 id="custom-id">Heading</h1>
// Like h1:   =====
// Like h2:   -----
// Paragraphs separated by blank line.
// Force newline with 2 or more spaces at eol, or use <br>
// **bold**  __bold__
// Bold**in**word
// *Italic*  _Italic_
// ***Bold+Italic***  ___B+I___  __*B+I*__
// > Blockquote
// > More blockquote
// >> Nested
// 1. List
// 2. List
// 1. List
// - Olist
// - Olist
// * olist
// + olist
//   - sublist
// - 1\. thing
// Code blocks:  indent 4 spaces or one tab.. in list, indent 8 spaces or 2 tabs
// ![Alt text](/path/to/image.png)
// ![Alt](/path/to/image.png "Title")
// `code`
// ```
// code block
// ```
// ```json
// {"stuff":"syntax highlighted"}
// ```
// `` <- escaped backtick
// ***     <- <hr>
// ---     <- <hr>
// ______  <- <hr>
// [Link text](https://somewhere.org)
// [Text](#header-id)
// <https://fastlink.org>
// http://autolink.com
// `http://not_autolink.com`
// [`code link`](#code)
// [Shortcut links][1]
// [1]: https://something.org    <- link will be rendered in original place
// Escaping chars: \ ` * _ { } [ ] ( ) # + - . ! |
// 
// Word
// : definition   ->  <dl><dt>Word</dt><dd>definition</dd></dl>
// ~~Strikethrough~~
// - [ ] Task
// - [x] Checked task
// :emoji:
// --------- Extended ----------
// footnote [^1]
// [^1]: footnote text
// Tables:
//   | Normal | Left | Center|Right|
//   |---|:-----|:-----:|---:|
//   | 1   | 2   |3|4|
//   
// Docfx extension:
// [!NOTE]
//    code
int Stream::ImportMarkdown(const char *text, int n, StreamChunk *addto, bool after)
{
	cerr << " *** must implement a markdown importer! defaulting to text"<<endl;
	return ImportText(text,n,addto,after);
}

/*! afterthis MUST be a chunk of *this or nullptr. No verification is done.
 * If nullptr, then add to end of chunks.
 *
 * Return 0 for success or nonzero for error.
 */
int Stream::ImportText(const char *text, int n, StreamChunk *addto, bool after)
{
	if (!text) return 0;
	if (n < 0) n = strlen(text);
	if (n == 0) return 0;
	
	StreamText *txt=new StreamText(text,n, nullptr);

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
	XMLFileToAttribute(&att,nfile,nullptr);
	return ImportXMLAtt(att, nullptr, top);
}

/*! From string value, assume it is xml formatted text, and parse into StreamChunk objects, completely
 * replacing old stream chucks.
 */
int Stream::ImportXML(const char *value)
{
	delete chunks;
	chunks=nullptr;

	long pos=0;
	Attribute att;
	XMLChunkToAttribute(&att, value, strlen(value), &pos, nullptr, nullptr);

	return ImportXMLAtt(att, nullptr, top);
}



/*! Something like:
 *  - local(blah)
 *  - local("Blah")
 *  - local('Blah')
 *  - url(https://thing)
 *
 *  Return 1 for successful parse, else 0.
 */
int ParseFofS(const char *value, int *f_len_ret, const char **s_ret, int *s_len_ret, const char **end_ret)
{
	//while (isspace(*value)) value++;
	const char *ptr = value;
	while (isalnum(*ptr)) ptr++;
	if (ptr == value) return 0;
	*f_len_ret = ptr - value;

	value = ptr;
	while (isspace(*value)) value++;
	if (*value != '(') return 0;
	value++;

	while (isspace(*value)) value++;
	char has_quote = '\0';
	if (*value == '"' || *value == '\'') { has_quote = *value; value++; }
	ptr = value;
	*s_ret = value;
	while (*ptr && *ptr != has_quote && *ptr != ')') ptr++;
	*s_len_ret = ptr - value;
	if (has_quote) {
		if (*ptr != has_quote) return 0;
		ptr++;
	}
	if (*ptr != ')') return 0;

	value = ptr+1; 
	*end_ret = value;
	return 1;
}

/*! 
 * See https://www.w3.org/TR/css-fonts-3/#font-face-rule.
 *
 * Return nullptr on could not process.
 */
Style *ProcessCSSFontFace(Style *style, Attribute *att, Laxkit::ErrorLog *log)
{
	// CSS spec says font-family and src must be present. Otherwise ignore.
	// Given font-family overrides any name given in the actual font.

	if (!att) return style;

	const char *name;
	const char *value;

	int err = 0;

	try {
		for (int c=0; c<att.attributes.n; c++) {
			name =att.attributes.e[c]->name;
			value=att.attributes.e[c]->value;

			if (!strcmp(name, "font-family")) {
				style->push("font-family", value);

			} else if (!strcmp(name, "src")) {
				//a comma separated list, go with first found:
				//
				// src: local("Some Name"),
				//	   local(SomeOtherNameWithoutQuotes),
				//	   url(/absolute/path),
				//	   url(rel/to/css/file),
				//	   url(file.svg#fontIdInFile)
				//	   url(fontcollection.woff2#fontId) format('woff2')
				//	   
				//format is optional, and string can be: woff, woff2, truetype, opentype, embedded-opentype, or svg

				const char *what = nullptr;
				int flen, slen;
				while (*value) {
					while (isspace(*value)) value++;
					const char *endptr = value;

					//int ParseFofS(const char *value, int *f_len_ret, const char **s_ret, int *s_len_ret, const char **end_ret)
					if (!strncmp(value, "local(", 6)) {
						if (ParseFofS(value, &flen, &what, &slen, &endptr)) {
							style->push("src-local", what,slen);
						} else {
							throw(_("Bad css"));
						};

					} else if (!strncmp(value, "url(", 4)) {
						if (ParseFofS(value, &flen, &what, &slen, &endptr)) {
							style->push("src", what,slen);
						} else {
							throw(_("Bad css"));
						};
					}
					value = endptr;

					while (isspace(*value)) value++;
					if (*value != ',') break;
					value++;
				}

			} else if (!strcmp(name, "unicode-range")) {
				//something like: unicode-range: U+0460-052F, U+1C80-1C88, U+20B4, U+2DE0-2DFF, U+A640-A69F, U+FE2E-FE2F;

				//single codepoint (e.g. U+416)
				//	a Unicode codepoint, represented as one to six hexadecimal digits
				//interval range (e.g. U+400-4ff)
				//	represented as two hyphen-separated Unicode codepoints indicating the inclusive start and end codepoints of a range
				//wildcard range (e.g. U+4??)
				//	defined by the set of codepoints implied when trailing ‘?’ characters signify any hexadeximal digit

				// ***  punting for now:
				style->push("unicode-range", value);

			} else if (!strcmp(name, "font-feature-settings")) {
				// ***  punting for now:
				style->push("font-feature-settings", value);

			} else if (!strcmp(name, "font-style")) {
				//Name: 	font-style
				//Value: 	normal | italic | oblique
				//Initial: 	normal

				int italic = 0;
				if (!strcmp(value,"normal"))  italic=0; 
				else if (!strcmp(value,"italic"))  italic=1;
				else if (!strcmp(value,"oblique")) italic=1; //technically oblique is distorted normal, italic is actual new glyphs

				style->push("italic", italic);

			} else if (!strcmp(name, "font-weight")) {
				//like usual font-weight, but no relative tags "bolder" or "lighter"
				//font-weight: normal | bold | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit

				const char *endptr = nullptr;
				weight = CSSFontWeight(value, endptr, nullptr);
				value = endptr;

			} else if (!strcmp(name, "font-stretch")) {
				//Name: 	font-stretch
				//Value: 	normal | ultra-condensed | extra-condensed | condensed | semi-condensed | semi-expanded | expanded | extra-expanded | ultra-expanded
				//Initial: 	normal

                // Only works if font family has width-variant faces.
				//stretch refers to physically stretching the letters
				//CSS4 allows percentages >= 0.
				style->push(value); //always just a string
			}
		}

	} catch (const char *str) {
		if (log) log->AddError(str);
		return nullptr;
	}

	return style;
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
		//  text-align: left | right | center | justify | justify-all | start | end | match-parent inherit | initial | unset    //start and end are relative to text direction
		//               //char-based alignment in a table column
		//              "." | "." center
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

	if (!style) style = new Style;
	Attribute att;
	CSSBlockToAttribute(&att, cssvalue);

	const char *variant=nullptr;
	const char *familylist=nullptr;
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
			else if (!strcmp(value,"oblique")) italic=1; //technically oblique is distorted normal, italic is actual new glyphs

			if (font_style>0) style->pushInteger("font-style", font_style);
			 
		} else if (!strcmp(name, "font-weight")) {
			//normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
			const char *endptr;
			weight = CSSFontWeight(value, endptr, nullptr);
			value = endptr;

			if (weight > 0) style->pushInteger("font-weight", w);
			//if (weight > 0 && is_relative) style->pushInteger("font-weight-offset", w);

		} else if (!strcmp(name, "font-size")) {
			 //  font-size: 	<absolute-size> | <relative-size> | <length> | <percentage> | inherit
			 //     absolute-size == xx-small | x-small | small | medium | large | x-large | xx-large 
			 //     relative-size == larger | smaller
			double v=-1;
			int type=0; //0 for absolute size
			Unit units;
			CSSName relative = CSS_Unknown;
			const char *endptr = nullptr;

			if (!CssFontSize(value, &v, &relative, &units, &endptr)) {
				*** error parsing!
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

				const char *endptr=nullptr;
				int font_style=CSSFontStyle(value, &endptr);
				char *font_variant=nullptr;
				int font_weight=-1;

				if (endptr==value) {
					font_variant = CSSFontVariant(value, &endptr);
					if (endptr == value) {
						font_weight = CSSFontWeight(value, &endptr, nullptr);
					} else value = endptr+1;
				} else value = endptr+1;
				
				double v = -1;
				Unit units;
				CSSName relative = CSS_Unknown;
				const char *endptr = nullptr;
				
				if (!CSSFontSize(value, &v,&relative,&units, &endptr) || v < 0) {
					*** error! font-size is required
				} else {
					if (v >= 0) style->pushLength(v,units,type);
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
				LengthValue *v=ParseLengthOrPercent(value, "font-size", nullptr);
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
		LFont *font = MatchCSSFont(fontfamily, italic, variant, weight);
		if (font) style->push("font",font);
	}

	return style;
}

LengthValue *ParseLengthOrPercent(const char *value, const char *relative_to, const char **endptr)
{
	*** % em, ex, px, and usual cm, etc-- --double val = -1;

	char *endptr = nullptr;
	DoubleAttribute(value, &val, &endptr);

	if (endptr == value) {
		// error! can't use this!
		return nullptr;
		
	} 
	while (isspace(*endptr)) endptr++;

	LengthValue *v = nullptr;
	if (*endptr == '%') {
		v = new LengthValue(l, "font-size");

	} else if (*endptr == 'e' && *endptr == 'm') {
	} else {
	}

	return v;
}


LFont *MatchCSSFont(const char *font_family, int italic, const char *variant, int weight)
{
	***
	// font_family is the css attribute:
	//  font-family: Bookman, serif, ..... <-- use first one found
	//    or generic:  serif  sans-serif  cursive  fantasy  monospace
	LFont *font=nullptr;
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

int ParseBidi(const char *str, int len)
{
	if (len < 0) len = strlen(str);
	int bidi=-2;

	if      (!strncasecmp(value,"ltr" , len))  bidi = LAX_LRTB;
	else if (!strncasecmp(value,"rtl" , len))  bidi = LAX_RLTB;
	else if (!strncasecmp(value,"auto", len)) bidi = -1;
	else if (!strncasecmp(value,"lrtb", len)) bidi = LAX_LRTB;
	else if (!strncasecmp(value,"ltbt", len)) bidi = LAX_LTBT;
	else if (!strncasecmp(value,"rltb", len)) bidi = LAX_RLTB;
	else if (!strncasecmp(value,"rlbt", len)) bidi = LAX_RLBT;
	else if (!strncasecmp(value,"tblr", len)) bidi = LAX_TBLR;
	else if (!strncasecmp(value,"tbrl", len)) bidi = LAX_TBRL;
	else if (!strncasecmp(value,"btlr", len)) bidi = LAX_BTLR;
	else if (!strncasecmp(value,"btrl", len)) bidi = LAX_BTRL;

	return bidi;
}

void Stream::ImportXMLAtt(Attribute *att, StreamChunk *&chunk, StreamStyle *laststyle)
{
	const char *name;
	const char *value;
	
	char *title = nullptr;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

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
			const char *alt=nullptr, *src=nullptr, *img_title=nullptr;

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
			img->LoadImage(src,nullptr, 0,0,0,0);
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

			StreamText *txt=new StreamText(nullptr,0, laststyle);
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


		StreamStyle *newstyle = nullptr;

		 //Parse global attributes: style class id dir="ltr|rtl|auto" title
		for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
			name  = att->attributes.e[c]->attributes.e[c2]->name;
			value = att->attributes.e[c]->attributes.e[c2]->value;

			if (!strcmp(name,"style")) {
				 //update newstyle with style found
				newstyle = ProcessCSSBlock(newstyle, laststyle, value);

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
	
				int bidi = ParseBidi(value,-1);

				if (!newstyle) newstyle = new StreamStyle();
				if (bidi >= 0) newstyle->set("bidi",bidi); ***
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


		if (title) { delete[] title; title = nullptr; }
		if (newstyle) newstyle->dec_count();
	}
}


//------------------------------------- StreamAttachment ----------------------------------

/*! Held by DrawableObjects, these define various types of attachments of Stream objects to the parent
 * DrawableObject, as well as cached rendering info in a StreamCache.
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

StreamAttachment::StreamAttachment(DrawableObject *nobject, Stream *nstream)
{
    owner = nobject; //assume *this is a child of object

    stream = nstream;
    if (stream) stream->inc_count();

    cache = nullptr;
}

StreamAttachment::~StreamAttachment()
{
	if (stream) stream->dec_count();
	if (cache) cache->dec_count();
}


//------------------------------------- StreamCache ----------------------------------
/*! \class StreamCache
 * \brief Rendering info for Stream objects.
 *
 * This is stored on each StreamAttachment object that connects the streams to objects.
 *
 * One StreamChunk object will correspond to one or more contiguous StreamCache objects.
 */

class StreamCache : public Laxkit::SquishyBox, public Laxkit::RefCounted
{
  public:
	clock_t modtime;
	StreamCache *next, *prev;

	StreamStyle *style;
	StreamChunk *chunk;
	long offset; //how many breaks into chunk to start
	long len; //how many breaks long in chunk is this cache

	Affine transform;

	StreamCache();
	StreamCache(StreamChuck *ch, long noffset, long nlen);
	virtual ~StreamCache();
};

StreamCache::StreamCache(StreamChuck *ch, long noffset, long nlen)
{
	modtime = 0;
	next    = nullptr;
	chunk   = ch;
	offset  = noffset;
	len     = nlen;
	if (ch) style = ch->style; else style=nullptr;
}

StreamCache::StreamCache()
{
	next   = nullptr;
	style  = nullptr;
	chunk  = nullptr;
	offset = len = 0;
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
 * or create and return a new one if cache==nullptr.
 */
StreamCache *RemapAreaStream(DrawableObject *target, StreamAttachment *attachment, int start_chunck, int chunk_offset)
{ ***
	if (!attachment) return 1;
	
	Stream      *stream = attachment->stream;
	StreamCache *cache  = attachment->cache;

	if (cache->modtime > 0 && stream->modtime < cache->modtime) return 0; //probably set ok


	 //-------- compute possible breaks if necessary
	 ***


	 //----------compute layout area:
	 //  start with target inset path
	 //  We need to remove the wrap path of any lesser siblings and aunts/uncles from that path
	PathsData *area = target->GetInsetPath()->duplicate();

	DrawableObject *so = target;
	DrawableObject *o = target->parent;
	while (o) {
		for (int c = 0; c < o->n() && o->e(c) != so; c++) {
			area->CutOut(o->e(c)->GetWrapPath());
		}
		so = o;
		o = o->parent;
	}


	 //--------------compute actual breaks within area
	***
}


} // namespace Laidout

