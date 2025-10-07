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

#include "streams.h"
#include "cssparse.h"
#include "lengthvalue.h"
#include "../language.h"
#include "../dataobjects/drawableobject.h"
#include "../dataobjects/fontvalue.h"
#include "../laidout.h"

#include <lax/units.h>
#include <lax/utf8utils.h>
#include <lax/cssutils.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/interfaces/somedatafactory.h>

#include <lax/debug.h>


using namespace Laxkit;


namespace Laidout {

/*! \file
 *
 * DrawableObjects can have text streams attached in various ways. general structure is:
 *
 * ```
 * DrawableObject
 *   StreamAttachment
 *     StreamCache - a particular rendering of Stream to the DrawableObject
 *     Stream
 *       top: StreamElement - a tree of Style objects, with leaf elements pointing to lists of StreamChunk
 *       chunk_start: a linked list of final leaf chunks
 */



//----------------------------------- General helper functions -----------------------------------



//----------------------------------- TabStopInfo -----------------------------------


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


//----------------------------------- CharacterStyle -----------------------------------

class CharacterStyle : public Style
{
  public:
  	// // these member variables are cached from collapsed Style tree
	// FontValue *font;

	// Laxkit::Color *color; //including alpha
	// Laxkit::Color *bg_color; //highlighting

	// double space_shrink;
	// double space_grow;  //as percent of overriding font size
	// double inner_shrink, inner_grow; //char spacing within words

	//  //basic positioning info
	// double kerning_before;
	// double kerning_after;
	// double h_scaling, v_scaling; //contentious glyph scaling

	//  //more thorough text effects
	// PtrStack<TextEffect> glyph_effects; //adjustments to characters or glyphs that changes metrics, making them different shapes or chars, like small caps, all caps, no caps, scramble, etc
	// PtrStack<TextEffect> below_effects; //additions like highlighting, that exist under the glyphs
	// PtrStack<TextEffect>  char_effects; //transforms to glyphs that do not change metrics, fake outline, fake bold, etc
	// PtrStack<TextEffect> above_effects; //lines and such drawn after the glyphs are drawn

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

	FontValue *font = new FontValue(); //todo: *** this needs to be some configurable default font
	DBGE(": fix me!");

	s->push("font",       font, -1, true);
	s->push("fontfamily", "serif");
	s->push("fontstyle",  "normal");
	s->push("fontsize",   new LengthValue("11pt"), -1, true);
	s->push("color",      new ColorValue("black"), -1, true);
	s->push("bg_color",   (Value*)nullptr);
	s->push("kern_before",new LengthValue("0%"), -1, true);
	s->push("kern_after", new LengthValue("0%"), -1, true);
	// s->push("h_scaling", 1.0);
	// s->push("v_scaling", 1.0);
	s->push("space_shrink", 0.0);
	s->push("space_grow",   0.0);
	s->push("inner_shrink", 0.0);

	s->push("glpyh_effects", new SetValue(), -1, true);
	s->push("below_effects", new SetValue(), -1, true);
	s->push("char_effects",  new SetValue(), -1, true);
	s->push("above_effects", new SetValue(), -1, true);
	
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
	
	Laxkit::PtrStack<TextEffect> conditional_effects; //like initial drop caps, first line char style, etc

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

	s->push("first_indent_lines", 1);
	s->push("first_indent", 0.0);
	s->push("normal_indent", 0.0);
	s->push("far_indent", 0.0); //for left to right, this would be the right margin for instance

	s->push("gap_above",   new LengthValue(1.0, UNITS_em), -1, true); //0 for  % of font size, 1 for absolute number
	s->push("gap_after",   new LengthValue(1.0, UNITS_em), -1, true); //0 for  % of font size, 1 for absolute number
	s->push("gap_between", new LengthValue(1.0, UNITS_em), -1, true); //0 for  % of font size, 1 for absolute number

	//todo: tabs
	//int tab_strategy; //regular intervals, by lines
	//TabStops *tabs;

	s->push("justify_type", 0); //0 for allow widow-like lines, 1 allow widows but do not fill space to edges, 2 for force justify all
	s->push("justification", 0.0); // (0..100) left=0, right=100, center=50, full justify
	s->push("flow_direction", 0); //*** need to implement all LAX_LRTB
	
	s->push("hyphens", new BooleanValue(false), -1, true); //whether to have them or not
	s->push("hyphen_width",   1.0);
	s->push("hyphen_shrink",  0.0);
	s->push("hyphen_grow",    0.0);
	s->push("hyphen_penalty", 1.0);


	//PtrStack<TextEffect> conditional_effects; //like initial drop caps, first line char style, etc

	s->push("default_charstyle", (Value*)nullptr);

	return s;
}


//----------------------------------- StreamElement -----------------------------------
/*! \class StreamElement
 * \brief Class to hold a stream broken down into a tree.
 * 
 * Please note this is different than general cascaded styles in that parents
 * AND kids exist as a specific hierarchy. Children are kept track of, and the order of children
 * depends on the associated stream. General Style objects don't care about particular
 * children, only parents from which they are derived.
 *
 * Streams need to traverse styles up and down when changing their contents. Leaves in
 * the tree are the actual content, and are derived from StreamChunk.
 *
 * StreamElement are very much like XML blocks, in that style holds the element attributes,
 * and kids holds the element content.
 *
 */

/*! Default constructor will create a new empty Style(). */
StreamElement::StreamElement()
{
	style = new Style();
	style_is_temp = true;
}

/*! nparent should have already added *this to its kids. */
StreamElement::StreamElement(StreamElement *nparent, Style *nstyle)
{
	treeparent = nparent;

	style = nstyle;
	if (style) style->inc_count();
	style_is_temp = true;
}

StreamElement::~StreamElement()
{
	if (style) style->dec_count();
}

//TODO: this should probably be more flexible for the future, not hard coded for three cases.
StreamChunk *NewStreamChunk(const char *type, StreamElement *parent)
{
	if (!strcmp(type, "text"))  return new StreamText(parent);
	if (!strcmp(type, "image")) return new StreamImage(parent);
	if (!strcmp(type, "break")) return new StreamBreak((int)StreamBreakTypes::BREAK_Unknown, parent);
	return nullptr;
}


/*! Return true for success, or false for some error.
 */
bool StreamElement::dump_in_att_stream(Laxkit::Attribute *att, Laxkit::DumpContext *context)
{
	const char *name, *value;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"style")) {
			if (!isblank(value)) {
				if (strstr(value, "resource:") == value) {
					//value += 9;
					// *** find resource!!
				}

			} else {
				if (!style) style = new Style();
				style->dump_in_atts(att->attributes.e[c], 0, context);
			}

		} else if (!strcmp(name,"kids")) {
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				value = att->attributes.e[c]->attributes.e[c2]->value;

				StreamElement *el = new StreamElement(this, nullptr);
				kids.push(el);

				el->dump_in_att_stream(att->attributes.e[c]->attributes.e[c2], context);
			}
			
		} else if (!strcmp(name,"chunks")) {
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				//value = att->attributes.e[c]->attributes.e[c2]->value;

				StreamChunk *chunk = NewStreamChunk(name, this);
				if (!chunk) {
					if (context->log) context->log->AddError(0,0,0, _("Unknown chunk type %s"), name);
					return false;
				}
				chunks.push(chunk);

				chunk->dump_in_atts(att->attributes.e[c]->attributes.e[c2], 0, context);
			}
		}
	}

	return true;
}
//----------------------------------- StreamChunk ------------------------------------




StreamChunk::StreamChunk(StreamElement *nparent)
{
	parent = nparent;
	next = prev = nullptr;
}

/* Default do nothing.
 */
StreamChunk::~StreamChunk()
{
	//if (next) delete next; no deleting, as chunks must ALWAYS be owned somewhere by a single StreamElement.
}

/*! Insert a currently unstyled list of chunks after ourself.
 * 
 * Assumes chunk->prev == nullptr. Ok for chunk->next to not be nullptr.
 * Inserts between this and this->next.
 *
 * If chunk->parent->style == nullptr, then use same style as *this. 
 * Otherwise, integrate the contained styles into the StreamElement tree.
 * It is assumed that chunk has a self contained network of styles.
 *
 * Returns next most StreamChunk of chunk.
 */
StreamChunk *StreamChunk::AddAfter(StreamChunk *chunk)
{
	if (!chunk) {
		DBGE("fix me!");
		return nullptr;
	}
	
	StreamChunk *chunkend = chunk;
	while (chunkend->next) chunkend = chunkend->next;

	if (!next) { next = chunk; chunk->prev = this; }
	else {
		chunkend->next = next;
		if (chunkend->next) chunkend->next->prev = chunkend;
		next = chunk;
		chunk->prev = this;
	}

	StreamElement *new_tree_parent = parent;
	StreamChunk *ch = chunk;
	do {
		if (ch->parent == nullptr) {
			ch->parent = new_tree_parent;
		} else { // it came with a parent style, so we need to child it within existing tree
			StreamElement *el = ch->parent;
			while (el->treeparent) {
				if (el == new_tree_parent) break; //chunk style has already been attached to tree
				el = el->treeparent;
			}
			if (el->treeparent == nullptr) el->treeparent = new_tree_parent;
		}

		ch = ch->next;
	} while (ch != chunkend->next);

	return chunkend;
}

/*! Assumes chunk->prev==nullptr. Ok for chunk->next to not be nullptr.
 * Inserts between this and this->next.
 *
 * If chunk->style==nullptr, then use same style as *this.
 */
void StreamChunk::AddBefore(StreamChunk *chunk)
{
	if (!chunk) return;
	
	StreamChunk *chunkend = chunk;
	while (chunkend->next) chunkend = chunkend->next;

	if (prev) prev->next = chunk;
	chunk->prev = prev;

	chunkend->next = this;
	prev = chunkend;

	StreamElement *new_tree_parent = parent;
	StreamChunk *ch = chunk;
	do {
		if (ch->parent == nullptr) {
			ch->parent = new_tree_parent;
		} else { // it came with a parent style, so we need to child it within existing tree
			StreamElement *el = ch->parent;
			while (el->treeparent) {
				if (el == new_tree_parent) break; //chunk style has already been attached to tree
				el = el->treeparent;
			}
			if (el->treeparent == nullptr) el->treeparent = new_tree_parent;
		}

		ch = ch->next;
	} while (ch != chunkend->next);
}


//----------------------------------- StreamBreak ------------------------------------


Laxkit::Attribute *StreamBreak::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att = new Laxkit::Attribute();
	if      (break_type == (int)StreamBreakTypes::BREAK_Paragraph) att->push("break", "pp");
	else if (break_type == (int)StreamBreakTypes::BREAK_Column)    att->push("break", "column");
	else if (break_type == (int)StreamBreakTypes::BREAK_Section)   att->push("break", "section");
	else if (break_type == (int)StreamBreakTypes::BREAK_Page)      att->push("break", "page");
	else if (break_type == (int)StreamBreakTypes::BREAK_Tab)       att->push("break", "tab");
	else if (break_type == (int)StreamBreakTypes::BREAK_Weak)      att->push("break", "weak");
	else if (break_type == (int)StreamBreakTypes::BREAK_Hyphen)    att->push("break", "hyphen");
	return att;
}

/*! NOTE! Uses whole attribute, not just kids. Meaning it compares att->value to various break types.
 */
void StreamBreak::dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if      (!strcmp(att->value,"pp"     )) break_type = (int)StreamBreakTypes::BREAK_Paragraph;
	else if (!strcmp(att->value,"column" )) break_type = (int)StreamBreakTypes::BREAK_Column;
	else if (!strcmp(att->value,"section")) break_type = (int)StreamBreakTypes::BREAK_Section;
	else if (!strcmp(att->value,"page"   )) break_type = (int)StreamBreakTypes::BREAK_Page;
	else if (!strcmp(att->value,"tab"    )) break_type = (int)StreamBreakTypes::BREAK_Tab;
	else if (!strcmp(att->value,"weak"   )) break_type = (int)StreamBreakTypes::BREAK_Weak;
	else if (!strcmp(att->value,"hyphen" )) break_type = (int)StreamBreakTypes::BREAK_Hyphen;
}


//----------------------------------- StreamImage ------------------------------------
/*! \class StreamImage
 * \brief Type of stream component made of a single DrawableObject.
 *
 * Note this can be ANY drawable object, not just images.
 */

StreamImage::StreamImage(StreamElement *parent_el)
  : StreamChunk(parent_el)
{
	img = nullptr;
}

StreamImage::StreamImage(DrawableObject *nimg,StreamElement *parent_el)
  : StreamChunk(parent_el)
{
	img = nimg;
	if (img) img->inc_count();
}

StreamImage::~StreamImage()
{
	if (img) img->dec_count();
}

Laxkit::Attribute *StreamImage::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!img) {
		att->push("object");
		return att;
	}
	Laxkit::Attribute *att2 = att->pushSubAtt("object", img->whattype());
	img->dump_out_atts(att2, what, context);
	return att;
}

void StreamImage::dump_in_atts (Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
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

StreamText::StreamText(StreamElement *parent_el)
  : StreamChunk(parent_el)
{
	len = 0;
	text = nullptr;
}

StreamText::StreamText(const char *txt,int n, StreamElement *parent_el)
  : StreamChunk(parent_el)
{
	if (n < 0) n = strlen(txt);
	text = newnstr(txt, n);
	len  = n;
}

StreamText::~StreamText()
{
	delete[] text;
}


/*! Returns the found number of breaks.
 */
int StreamText::DetectBreaks()
{
	const char *s = text;
	num_breaks = 0;
	while (*s) {
		while(*s && isspace(*s)) s++;
		if (!*s) break;
		while (*s && !isspace(*s)) s++;
		num_breaks++;
	}
	return num_breaks;
}

int StreamText::SetText(const char *txt,int n)
{
	if (n < 0) n = strlen(txt);
	delete[] text;
	text = newnstr(txt, n);
	len  = n;
	return 0;
}

int StreamText::SetFromEntities(const char *cdata, int cdata_len)
{
	//***TODO: compress whitespace

	//big json file with entities list: https://html.spec.whatwg.org/entities.json
	int new_n = 0;
	if (cdata_len < 0) cdata_len = strlen(cdata);
	char *new_text = new char[cdata_len+1];
	htmlchars_decode(cdata, new_text);
	delete[] text;
	text = new_text;
	len = new_n;
	return new_n;
}

//! Return the number of possible (not actual) breaks within the string of characters.
/*! This can include hyphen points, spaces, or newlines.
 */
int StreamText::NumBreaks()
{
	if (!text) return 0;

	const char *s = text;
	int n = 0;
	while (*s) {
		while (*s && !isspace(*s)) s = utf8fwd(s, s, text+len);
		if (*s == ' ') {
			n++;
			s++; //assumes all white space is ' '. Assumed '\n' and '\t' previously converted to explicit breaks
		}
	}
	return n;
}

/*! Return an indicator of the severity of the break.
 * 0 <= index < NumBreaks().
 * 
 * For instance, in between letters in a word there is almost no break potential,
 * hyphens have low break potential, spaces have a lot, but not as much
 * as line breaks.
 */
int StreamText::BreakSeverity(int index)
{
	//assume we only have spaces, and that tabs and returns previously converted to explicit break chunks.
	return 1;
}

Laxkit::Attribute *StreamText::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att = new Attribute();
	att->push("text", text);
	return att;
}

void StreamText::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	Attribute *txt = att->find("text");
	if (!txt) return;
	makestr(text, txt->value);
	if (text) len = strlen(text);
	else len = 0;
}


//------------------------------------- StreamImportData ----------------------------------

StreamImportData::~StreamImportData()
{
	if (importer) importer->dec_count();
	if (text_object) text_object->dec_count();
	if (file) file->dec_count();
}


//------------------------------------- Stream ----------------------------------

/*! \class Stream
 * \brief Hold a stream of things, usually text.
 * 
 * These are essentially heads of StreamElement + StreamChunk objects.
 */


Stream::Stream()
{
	modtime = 0;
}

Stream::~Stream()
{
	if (import_data) import_data->dec_count();
	//chunks are not explicitly deleted, since they exist somewhere in top tree
}

void Stream::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what,context);
	att.dump_out(f,indent);
}

/*! Starting with top, dump out the whole tree recursively with dump_out_recursive().
 *
 * <pre>
 *   streamdata
 *     style
 *       basedon laidout_default_style
 *       font-size     10
 *       line-spacing  110%
 *     chunks
 *     kids
 *       text "first text"
 *         style ...
 *         kids
 *           image /blah/blah.jpg
 *           text "Some text all up in here"
 *           dynamic context.page.label+"/"+string
 *           break line
 *           break column
 *           break section #or page
 *
 *   htmltext \\
 *     <span class="#laidout_default_style" style="font-size:10; line-spacing:110%;">first text<img file="/blah/blah.jpg"/>Some text all up in here</span>
 * </pre>
 */
Laxkit::Attribute *Stream::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!chunk_start) return nullptr;

	if (!att) att = new Attribute;

	Attribute *att2 = att->pushSubAtt("streamdata");
	dump_out_recursive(att2, &top, context);

	if (import_data) {
		if (import_data->file) att->push("file", import_data->file->filename);
		if (import_data->text_object) att->push("text_object", import_data->text_object->Id()); //assume the text object is a resource??
		if (import_data->importer) att->push("importer", import_data->importer->whattype());
	}

	return att;
}


void Stream::dump_out_recursive(Laxkit::Attribute *att, StreamElement *element, Laxkit::DumpContext *context)
{
	if (element->style) {
		if (!element->style_is_temp) {
			Utf8String str("resource:");
			str.Append(element->style->Id());
			att->push("style",str.c_str()); //is a style recorded somewhere

		} else {
			Attribute *att2 = att->pushSubAtt("style");
			element->style->dump_out_atts(att2, 0, context);
		}
	}

	if (element->kids.n) {
		Attribute *att2 = att->pushSubAtt("kids");
		for (int c=0; c < element->kids.n; c++) {
			dump_out_recursive(att2, element->kids.e[c], context);
		}
	}

	if (element->chunks.n) {
		Attribute *att2 = att->pushSubAtt("chunks");
		for (int c=0; c < element->chunks.n; c++) {
			Attribute *att3 = att2->pushSubAtt("chunk", element->chunks.e[c]->whattype());
			element->chunks.e[c]->dump_out_atts(att3, 0, context);
		}
	}
}



void Stream::dump_in_atts(Attribute *att,int flag, Laxkit::DumpContext *context)
{
	const char *name, *value;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"streamdata")) {
			 //default Laidout format.... maybe should just use htmltext for simplicity??
			top.dump_in_att_stream(att->attributes.e[c], context);

		} else if (!strcmp(name,"htmltext")) {
			ImportXML(value, -1, context->log);

		} else if (!strcmp(name,"text")) {
			ImportText(value,-1, nullptr, true, context->log);

		} else if (!strcmp(name,"markdown")) {
			ImportMarkdown(value,-1, nullptr,true, context->log);

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

			DBGM("Import text: "<<(format?format:"null")<<" "<<(importer?importer:"null")<<" "<<(encoding?encoding:"null"));

			int status = -2;

			if (importer) {
				 //find and try importer on file with format
				StreamImporter *importer_obj = nullptr; //FIXME *** laidout->GetStreamImporter(importer, format);
				if (importer_obj) status = importer_obj->In(this, file, context, nullptr,0);
				else {
					status = 1;
					if (context->log) context->log->AddWarning(0,0,0,_("Unknown importer %s, ignoring file %s"), importer, file);
				}

			} else { //importer not found
				 //try to guess from extension
				if (!format) {
					const char *ext = lax_extension(file);

					if (!ext)
						format = "text";
					else if (!strncasecmp(ext, "html", 4))
						format = "html";
					else if (!strncasecmp(ext, "md", 2))
						format = "markdown";
				}

				if (!format) format = "text";

				int len = 0;
				char *contents = read_in_whole_file(file, &len);
				if (!contents) {
					if (context->log) context->log->AddWarning(0,0,0, _("Cannot load stream file %s!"), file);

				} else {
					 //no importer found, try some defaults based on format
					if (!strcmp(format, "html")) {
						ImportXML(contents, len, context->log);

					} else if (!strcmp(format, "markdown")) {
						ImportMarkdown(contents, len, nullptr,true, context->log);

					} else { //text
						char *str = read_in_whole_file(file, &len);
						if (!isblank(str)) {
							ImportText(contents, len, nullptr,true, context->log);
						}
						delete[] str;
					}

					delete[] contents;
				}
			}

			if (status != 0) {
				if (context->log) context->log->AddWarning("Stream import failed or something");
			}
		}
	}

	tms tms_;
	modtime = times(&tms_);
}


int Stream::ImportMarkdown(const char *text, int n, StreamChunk *addto, bool after, Laxkit::ErrorLog *log)
{
	DBGE(" *** must implement a markdown importer! defaulting to text");

	// MarkdownImporter *importer = laidout->GetStreamImporter("markdown", nullptr);
	// importer->ImportTo(text, n, addto, after);

	return ImportText(text,n,addto,after, log);
}

void Stream::EstablishDefaultStyle()
{
	DBGE(" IMPME!");
}

/*! afterthis MUST be a chunk of *this or nullptr. No verification is done.
 * If nullptr, then add to end of chunks.
 *
 * Return 0 for success or nonzero for error.
 */
int Stream::ImportText(const char *text, int n, StreamChunk *addto, bool after, Laxkit::ErrorLog *log)
{
	if (!text) return 0;
	if (n < 0) n = strlen(text);
	if (n == 0) return 0;
	
	StreamText *txt = new StreamText(text,n, nullptr);

	if (addto) {
		if (after) addto->AddAfter(txt);
		else {
			addto->AddBefore(txt);
			//FIXME *** 
			//if (addto == chunks) chunks = addto; //reposition head
		}
	} else {
		if (!chunk_start) {
			// blank stream! install first chunk...
			if (!top.style || !top.style->n()) EstablishDefaultStyle();
		   	chunk_start = txt;
			chunk_start->parent = &top;
			top.chunks.push(chunk_start);

	   	} else {
			// append or prepend to addto
			if (!addto) {
				// prepend to beginning of all, or append to end of all
				addto = chunk_start;
				if (after) while (addto->next) addto = addto->next;
			}
			if (after) {
				addto->AddAfter(txt); //AddAfter() and AddBefore() install a style
			} else {
				addto->AddBefore(txt);
				if (addto == chunk_start) chunk_start = addto; //reposition head
			}
		}
	}

	return 0;
}


/*! Read in the file to a string, then call ImportXML(). Updates this->file.
 */
int Stream::ImportXMLFile(const char *nfile, Laxkit::ErrorLog *log)
{
	//makestr(file,nfile);
	Laxkit::Attribute att;
	XMLFileToAttribute(&att,nfile,nullptr);
	StreamChunk *last_chunk = nullptr;
	StreamElement *next_style = nullptr;
	return ImportXMLAtt(&att, last_chunk, next_style, nullptr, log);
}

/*! From string value, assume it is xml formatted text, and parse into StreamChunk objects, completely
 * replacing old stream chucks.
 *
 * Supported tags:
 * ```
 *    <b>bold</b> <i>italic</i>
 *    <br> <tab>
 *    <br section>
 *    <br page>
 *    <br column>
 *    <br tab>
 *    <img width="100" height="100" alt=".." src=".."   style class id ... />
 *    <hr>
 *    <div>stuff</div>
 *    <span>stuff</span>
 *    <code>stuff</code>
 *    <pre>stuff</pre>
 *    <ol> <li/> </ol>
 *    <ul> <li/> </ul>
 * ```
 */
int Stream::ImportXML(const char *value, int len, Laxkit::ErrorLog *log)
{
	long pos = 0;
	Laxkit::Attribute att;
	if (len < 0) len = strlen(value);
	XMLChunkToAttribute(&att, value, len, &pos, nullptr, nullptr);

	StreamChunk *last_chunk = nullptr;
	StreamElement *next_style = nullptr;
	return ImportXMLAtt(&att, last_chunk, next_style, nullptr, log);
}


/*! Import att as chunks under last_style_el, adding sub-StreamElement objects as needed.
 * last_chunk must be either null or a chunk within last_style_el.
 *
 * Return 0 for success, or nonzero for some kind of error.
 */
int Stream::ImportXMLAtt(Attribute *att, StreamChunk *&last_chunk, StreamElement *&next_element, StreamElement *last_style_el, Laxkit::ErrorLog *log)
{
	const char *name;
	const char *value;
	
	//char *title = nullptr;

	StreamChunk *chunk = nullptr;
	StreamElement *cur_style = nullptr; //ParseCommonStyle(att, nullptr, log);

	if (cur_style) {
		if (last_style_el) {
			// we need to install a subelement, since cur_style has additional styling.
			// if last_chunk is not at edge of last_style_el, we'll need to split it
		}
	}

	Attribute *elements = att->find("content:");
	if (!elements) return 0;

	for (int c=0; c<elements->attributes.n; c++) {
		name  = elements->attributes.e[c]->name;
		value = elements->attributes.e[c]->value;

		//br tab wbr img hr cdata:
		
		//
		//first process elements net expected to have kids ... currently: br wbr hr img tab "cdata:"
		//
		if (!strcmp(name,"br") || !strcmp(name,"tab")) {
			 //extended to have: <br section> <br chapter> <br column> <br page>
			 //also my tag: <tab>
	
			int type = (int)StreamBreakTypes::BREAK_Paragraph;

			if (!strcmp(name,"tab")) type = (int)StreamBreakTypes::BREAK_Tab;
			else {
				for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
					name =att->attributes.e[c]->attributes.e[c2]->name;
					value=att->attributes.e[c]->attributes.e[c2]->value;

					if      (!strcmp(name,"section")) type = (int)StreamBreakTypes::BREAK_Section;
					else if (!strcmp(name,"page"))    type = (int)StreamBreakTypes::BREAK_Page;
					else if (!strcmp(name,"column"))  type = (int)StreamBreakTypes::BREAK_Column;
					else if (!strcmp(name,"tab"))     type = (int)StreamBreakTypes::BREAK_Tab;
					else type = (int)StreamBreakTypes::BREAK_Unknown;
				}
			}
			
			if (type != (int)StreamBreakTypes::BREAK_Unknown) {
				StreamBreak *br = new StreamBreak(type, nullptr);
				if (!chunk) { last_chunk = chunk = br; }
				else chunk = chunk->AddAfter(br);

			} else {
				if (log) {
					setlocale(LC_ALL, "");
					log->AddWarning(0,0,0, _("Unknown break %s"), value);
					setlocale(LC_ALL, "C");
				}
			}
			continue;

		} else if (!strcmp(name,"wbr")) {
			//html: Word Break Opportunity
			//*** if occurs between cdata blocks, then insert as weak break point in a combined StreamText
			DBGE(" *** NEED TO IMPLEMENT wbr in Stream::ImportXMLAtt");

		} else if (!strcmp(name,"img")) {
			//<img width="100" height="100" alt=".." src=".."   style class id ... />
			
			//double w=-1, h=-1;
			LengthValue *w_value  = nullptr;
			LengthValue *h_value  = nullptr;
			const char *alt       = nullptr;
			const char *src       = nullptr;
			const char *img_title = nullptr;

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				value = att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"width")) {
					w_value = LengthValue::Parse(value);
					// DoubleAttribute(value,&w);

				} else if (!strcmp(name,"height")) {
					h_value = LengthValue::Parse(value);
					// DoubleAttribute(value,&h);

				} else if (!strcmp(name,"title")) {
					img_title = value;

				} else if (!strcmp(name,"alt")) {
					alt = value;

				} else if (!strcmp(name,"src")) {
					src = value;
				}
			}

			LaxInterfaces::ImageData *img = dynamic_cast<LaxInterfaces::ImageData*>(LaxInterfaces::somedatafactory()->NewObject(LaxInterfaces::LAX_IMAGEDATA));
			img->LoadImage(src,nullptr, 0,0,0,0);
			if (alt) makestr(img->description, alt);
			if (img_title) makestr(img->title, img_title);

			// img->maxx is pixel width, so we want w_value * xaxis().norm() == maxx
			DBGE("FIXME! do proper aspect ratio for inline image");
			if (w_value) {
				img->xaxis(w_value->value / img->maxx * img->xaxis());
			}
			if (h_value) {
				img->yaxis(h_value->value / img->maxy * img->yaxis());
			}
			//if (h <= 0) *** //make same size as font (ascent+descent)?
			//*** if box style attributes, make a group with path border around image

			StreamImage *image = new StreamImage(dynamic_cast<DrawableObject*>(img), nullptr);
			if (!chunk) { last_chunk = chunk = image; }
			else chunk = chunk->AddAfter(image);
			continue;

		} else if (!strcmp(name,"hr")) {
			 //<hr>  add a filled path rectangle?
			DBGE(" *** <hr> not implemented!! Rats!");
			continue;

		} else if (!strcmp(name,"cdata:")) {
			// *** &quot; and stuff

			StreamText *txt = new StreamText(nullptr,0, nullptr);
			txt->SetFromEntities(value,-1); // <- needs to compress whitespace, and change &stuff; to chars
			//char *text=ConvertEntitiesToChars(value);
			//delete[] text;

			if (!chunk) { last_chunk = chunk = txt; }
			else chunk = chunk->AddAfter(txt);
			continue;


		//
		// now process those that can have sub elements...
		//

		} else if (!strcmp(name,"div") || !strcmp(name,"p")) {
			//int Stream::ImportXMLAtt(Attribute *att, StreamChunk *&last_chunk, StreamElement *&next_style, StreamElement *last_style_el, Laxkit::ErrorLog *log)
			ImportXMLAtt(elements->attributes.e[c], chunk, next_element, nullptr, log);

			 //add a line break after div if not right after one
			 //*** should one be added before too? if first div should not be one
			if (chunk->Type()!=CHUNK_Break) {
				StreamBreak *br = new StreamBreak((int)StreamBreakTypes::BREAK_Paragraph, nullptr);
				if (!chunk) { last_chunk = chunk = br; }
				else chunk = chunk->AddAfter(br);
			}

		} else if (!strcmp(name,"span")) {
			ImportXMLAtt(elements->attributes.e[c], chunk, next_element, nullptr, log);

		} else if (!strcmp(name,"strong") || !strcmp(name,"b")) {
			//bold
			//search for a bold equivalent of current font
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (!strcmp(name,"em") || !strcmp(name,"i")) {
			//italic
			//search for an italic equivalent of current font
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (!strcmp(name,"blockquote")) {
			//*** //set paragraph indent
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (!strcmp(name,"code") || !strcmp(name, "tt")) {
			//*** //use a monospace font
				//if "code" then do like a div?
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (!strcmp(name,"pre")) {
			// *** convert \n chars to <br> elements
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (!strcmp(name,"font")) { //deprecated as of html4.1
			//<font size="-1">blah</font>
			DBGE(" *** "<<name<<" not implemented!! Rats!");
			
		} else if (!strcmp(name,"a")) {
			// *** anchors.. use for html links, bookmark designations, citations?
			//<a name="..." href="..." class="..." id="..">...</a>
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (!strcmp(name,"dl")) {
			//<dl> <dt> <dd>
			DBGE( " *** need to implement dl lists!");

		} else if (!strcmp(name,"ol")) {
			//<ol> <li> ...</li> </ol>
			DBGE( " *** need to implement ol lists!");

		} else if (!strcmp(name,"ul")) {
			//<ul> <li> ...</li> </ul>
			DBGE( " *** need to implement ul lists!");

		} else if (!strcmp(name,"table")) {
			//<table><tr><td colspan rowspan>
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (name[0]=='h' && name[1]>='1' && name[1]<='6' && name[2]=='\0') {
			//int heading = name[1]-'1'+1;
			//***
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (!strcmp(name,"math")) {
			//*** //mathml stuff
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else if (!strcmp(name,"texmath")) {
			//*** math markup
			//<math> (for mathml) or not html: <latex> <tex> $latex stuff$  $$latex eqns$$
			DBGE(" *** "<<name<<" not implemented!! Rats!");

		} else {
			DBGE(" *** "<<name<<" not implemented!! Rats!");
		}


		//if (newstyle) newstyle->dec_count();
	}

	return 0;
}


//------------------------------ BaselineGrid ---------------------------------

Value *NewBaselineGridFunc() { return new FlatvectorValue; }

ObjectDef default_BaselineGrid_ObjectDef(nullptr,"BaselineGrid",_("Baseline grid"),_("For area streams, a guild to sync baseline across objects"),
							 "class", nullptr, nullptr,
							 nullptr, 0,
							 NewBaselineGridFunc, nullptr);

ObjectDef *Get_BaselineGrid_ObjectDef()
{
	ObjectDef *def = &default_BaselineGrid_ObjectDef;
	if (def->fields) return def;

	// virtual int pushVariable(const char *name,const char *nName, const char *ndesc, const char *type, unsigned int fflags, Value *v,int absorb);

	def->pushVariable("default_spacing", _("Default spacing"), _("Distance between baselines"), "real", 0, new DoubleValue(15.0),true);
	def->pushVariable("direction", _("Direction"), _("Direction of baselines"), "Vector2", 0, new FlatvectorValue(1,0),true);
	def->pushVariable("offset", _("Offset"), _("Offset from origin to start a line"), "Vector2", 0, new FlatvectorValue(0,0),true);
	
	return def;
}

ObjectDef *BaselineGrid::makeObjectDef()
{
	Get_BaselineGrid_ObjectDef()->inc_count();
	return Get_BaselineGrid_ObjectDef();
}


//------------------------------------- StreamAttachment ----------------------------------


/*! Create this StreamAttachment as being owned by nobject.
 *  nstream will be inc_counted.
 *  cache is set to null, it is not computed here.
 */
StreamAttachment::StreamAttachment(DrawableObject *nobject, Stream *nstream)
{
    owner = nobject; //assume *this is owned by nobject

    stream = nstream;
    if (stream) stream->inc_count();

    baseline_dir.x = 1;
    cache = nullptr;
}

StreamAttachment::~StreamAttachment()
{
	if (stream) stream->dec_count();
	if (cache)  cache ->dec_count();
	if (baseline_grid) baseline_grid->dec_count();
}


//------------------------------------- StreamCache ----------------------------------


StreamCache::StreamCache(StreamChunk *ch, long noffset, long nlen)
{
	modtime = 0;
	next    = nullptr;
	chunk   = ch;
	offset  = noffset;
	len     = nlen;
	element = nullptr;

	if (ch) element = ch->parent;
}

StreamCache::StreamCache()
{
	modtime = 0;
	next    = nullptr;
	chunk   = nullptr;
	element = nullptr;
	offset  = len = 0;
}

StreamCache::~StreamCache()
{
	if (next) delete next;
}


//------------------------------- Stream mapping ------------------------------------------


/*! Return path object when changed, else nullptr if unchanged.
 * If modify_in_place, then modify the area object to be the result.
 */
LaxInterfaces::PathsData *PathBooleanSubtract(LaxInterfaces::PathsData *area, LaxInterfaces::PathsData *to_remove, bool modify_in_place)
{
	DBGE("  IMPLEMENT ME!!");
	return nullptr;
}



/*! Update cache for stream laid into target->GetInsetPath(), as blocked by appropriate objects in
 * the same page. Note that only page objects affect wrapping. Paper and limbo objects do not
 * affect wrapping.
 *
 * Overwrites anything in cache chain, adding more nodes if necessary, and deleting unused ones.
 * or create and return a new one if cache == nullptr.
 */
StreamCache *RemapAreaStream(DrawableObject *target, StreamAttachment *attachment)
{
	if (!attachment) return nullptr;
	
	Stream      *stream = attachment->stream;
	StreamCache *cache  = attachment->cache;
	StreamChunk *start_chunk = attachment->starting_chunk;
	// StreamChunk *end_chunk   = attachment->ending_chunk;

	if (cache && cache->modtime > 0 && stream->modtime < cache->modtime && target->modtime < cache->modtime)
		return cache; //probably set ok


	//-------- compute possible breaks if necessary
	StreamChunk *chunk = start_chunk;
	if (!chunk) return nullptr;
	while (chunk) {
		chunk->DetectBreaks();
		chunk = chunk->next;
	}
	chunk = start_chunk;


	//----------compute layout area:
	//  start with target inset path
	//  We need to remove the wrap path of any lesser siblings and aunts/uncles from that path
	LaxInterfaces::PathsData *area = dynamic_cast<LaxInterfaces::PathsData*>(target->GetInsetPath()->duplicateData(nullptr));

	DrawableObject *so = target;
	DrawableObject *o = target->GetDrawableParent();
	while (o) {
		for (int c = 0; c < o->n(); c++) {
			if (o->e(c) == so) continue;
			DrawableObject *dro = dynamic_cast<DrawableObject*>(o->e(c));
			PathBooleanSubtract(area, dro->GetWrapPath(), true);
			//area->CutOut(dro->GetWrapPath());
		}
		so = o;
		o = o->GetDrawableParent();
	}


	//--------------compute actual breaks within area
	
	// determine baseline
	flatvector baseline = attachment->baseline_dir;
	flatvector baseline_offset;
	if (attachment->baseline_grid) {
		Affine trn = target->GetTransformToContext(true, 0);
		baseline = trn.transformVector(attachment->baseline_grid->direction);
		baseline_offset = trn.transformPoint(attachment->baseline_grid->offset);
	}
	
	// intersect baseline with area path
	// *** there may be many intersections, need to break down in-out segments
	
	// add chunk pieces along trimmed baseline until max reached
	// ***
	
	// next line... repeat until out of bounds for area..
	// ***
	
	// compute chunks/cache for next area in chain if any
	// ***


	return cache;
}


} // namespace Laidout

