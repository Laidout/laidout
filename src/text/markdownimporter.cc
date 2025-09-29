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


#include "markdownimporter.h"

namespace Laidout {


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



/*! Import file as a new Document. */
int MarkdownImporter::ImportMarkdownFile(const char *filename)
{

}

const char *MarkdownImporter::FileType(const char *first100bytes)
{
	// *** is file all utf8?
	return "Markdown";
}

/*! Standard import pathway.
 */
int MarkdownImporter::In(const char *file, Laxkit::anObject *context, Laxkit::ErrorLog &log, const char *filecontents, int contentslen)
{
	***
}

/*! Import directly to a StreamChunk.
 */
int MarkdownImporter::ImportTo(const char *text, int n, StreamChunk *addto, bool after)
{
	***
}


int MarkdownImporter::Parse(const char *buffer, int len)
{
    MD_PARSER parser_funcs;
    parser_funcs.abi_version = 0;
    parser_funcs.flags = MD_DIALECT_GITHUB; //see MD_FLAG_*
    parser_funcs.enter_block = enter_block;
    parser_funcs.leave_block = leave_block;
    parser_funcs.enter_span = enter_span;
    parser_funcs.leave_span = leave_span;
    parser_funcs.text = text;
    parser_funcs.debug_log = debug_log;
    parser_funcs.syntax = nullptr;

	int ret = md_parse(buf.get_data(), buf.length(), &parser_funcs, this);
}


// static relay function
int MarkdownImporter::enter_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    return ((MarkdownImporter*)userdata)->_enter_block(type, detail);
}

int MarkdownImporter::_enter_block(MD_BLOCKTYPE type, void* detail)
{
    Array params;
    params.push_back(type);

    if (type == MD_BLOCK_UL) {
         /* Detail: Structure MD_BLOCK_UL_DETAIL. */
        MD_BLOCK_UL_DETAIL *det = (MD_BLOCK_UL_DETAIL *)detail;
        Dictionary dict;
        dict["is_tight"] = det->is_tight;
        dict["mark"]     = String::utf8(&det->mark, 1);
        params.push_back(dict);

    } else if (type == MD_BLOCK_OL) {
         /* Detail: Structure MD_BLOCK_OL_DETAIL. */
        MD_BLOCK_OL_DETAIL *det = (MD_BLOCK_OL_DETAIL *)detail;
        Dictionary dict;
        dict["start"]          = det->start;         /* Start index of the ordered list. */
        dict["is_tight"]       = det->is_tight;   /* Non-zero if tight list, zero if loose. */
        dict["mark_delimiter"] = String::utf8(&det->mark_delimiter, 1); /* Character delimiting the item marks in MarkDown source, e.g. '.' or ')' */
        params.push_back(dict);

    } else if (type == MD_BLOCK_LI) {
         /* Detail: Structure MD_BLOCK_LI_DETAIL. */
        MD_BLOCK_LI_DETAIL *det = (MD_BLOCK_LI_DETAIL *)detail;
        Dictionary dict;
        dict["is_task"]          = det->is_task;            /* Can be non-zero only with MD_FLAG_TASKLISTS */
        dict["task_mark"]        = String::utf8(&det->task_mark, 1); /* If is_task, then one of 'x', 'X' or ' '. Undefined otherwise. */
        dict["task_mark_offset"] = det->task_mark_offset;  /* If is_task, then offset in the input of the char between '[' and ']'. */
        params.push_back(dict);

    } else if (type == MD_BLOCK_H) {
        //DBG cerr << "plugin MD_BLOCK_H "<<endl;

        MD_BLOCK_H_DETAIL *det = (MD_BLOCK_H_DETAIL *)detail;
        params.push_back(det->level);
        //DBG cerr << "plugin MD_BLOCK_H " << det->level << endl;

    } else if (type == MD_BLOCK_CODE) {
        MD_BLOCK_CODE_DETAIL *det = (MD_BLOCK_CODE_DETAIL*)detail;
        Dictionary dict;
        dict["info"]       = String::utf8(det->info.text, det->info.size);
        dict["lang"]       = String::utf8(det->lang.text, det->lang.size);
        dict["fence_char"] = String::utf8(&det->fence_char, 1);
        params.push_back(dict);

   } else if (type == MD_BLOCK_TABLE) {
         /* Detail: Structure MD_BLOCK_TABLE_DETAIL (for MD_BLOCK_TABLE),*/
        MD_BLOCK_TABLE_DETAIL *det = (MD_BLOCK_TABLE_DETAIL*)detail;
        Dictionary dict;

        dict["col_count"]      = det->col_count;           /* Count of columns in the table. */
        dict["head_row_count"] = det->head_row_count; /* Count of rows in the table header (currently always 1) */
        dict["body_row_count"] = det->body_row_count; /* Count of rows in the table body */

        params.push_back(dict);

    } else if (type == MD_BLOCK_TH || type == MD_BLOCK_TD) {
         /*         structure MD_BLOCK_TD_DETAIL (for MD_BLOCK_TH and MD_BLOCK_TD)*/
         /* Note all of these are used only if extension MD_FLAG_TABLES is enabled. */
        MD_BLOCK_TD_DETAIL *det = (MD_BLOCK_TD_DETAIL*)detail;
        Dictionary dict;

        dict["align"] = (int)det->align;

        params.push_back(dict);

    } else {
        params.push_back(0);
    }

    return 0;
}

// static relay function
int MarkdownImporter::leave_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
	return ((MarkdownImporter*)userdata)->_leave_block(type, detail);
}

int MarkdownImporter::leave_block(MD_BLOCKTYPE type, void* detail)
{
	***
    return 0;
}

// static relay function
int MarkdownImporter::enter_span(MD_SPANTYPE type, void* detail, void* userdata)
{
	return ((MarkdownImporter*)userdata)->_enter_span(type, detail);
}

int MarkdownImporter::enter_span(MD_SPANTYPE type, void* detail)
{
	DBG cerr << "plugin enter span: " << type << endl;

	Array params;
	params.push_back(type);

	if (type == MD_SPAN_A) {
////		typedef struct MD_SPAN_A_DETAIL {
////			MD_ATTRIBUTE href;
////			MD_ATTRIBUTE title;
////		} MD_SPAN_A_DETAIL;
		MD_SPAN_A_DETAIL *det = (MD_SPAN_A_DETAIL*)detail;
		Dictionary dict;

		dict["href"]  = String::utf8(det->href.text,  det->href.size);
		dict["title"] = String::utf8(det->title.text, det->title.size);

		params.push_back(dict);

	} else if (type == MD_SPAN_IMG) {
//		/* Detailed info for MD_SPAN_IMG. */
//		typedef struct MD_SPAN_IMG_DETAIL {
//			MD_ATTRIBUTE src;
//			MD_ATTRIBUTE title;
//		} MD_SPAN_IMG_DETAIL;
		MD_SPAN_IMG_DETAIL *det = (MD_SPAN_IMG_DETAIL*)detail;
		Dictionary dict;

		dict["src"]   = String::utf8(det->src.text,   det->src.size);
		dict["title"] = String::utf8(det->title.text, det->title.size);

		params.push_back(dict);

	} else if (type == MD_SPAN_WIKILINK) {
//		/* Detailed info for MD_SPAN_WIKILINK. */
//		typedef struct MD_SPAN_WIKILINK {
//			MD_ATTRIBUTE target;
//		} MD_SPAN_WIKILINK_DETAIL;
		MD_SPAN_WIKILINK_DETAIL *det = (MD_SPAN_WIKILINK_DETAIL*)detail;
		Dictionary dict;

		dict["target"] = String::utf8(det->target.text, det->target.size);

		params.push_back(dict);

	} else {
		params.push_back(0);
	}
	

    return 0;
}

// static relay function
int MarkdownImporter::leave_span(MD_SPANTYPE type, void* detail, void* userdata)
{
	return ((MarkdownImporter*)userdata)->_leave_span(type, detail);
}

int MarkdownImporter::_leave_span(MD_SPANTYPE type, void* detail)
{
	***
    return 0;
}


// static relay function
int MarkdownImporter::text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
	return ((MarkdownImporter*)userdata)->_text(type, text, size);
}

int MarkdownImporter::_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size)
{
	***
    return 0;
}


// static relay function
void MarkdownImporter::debug_log(const char* msg, void* userdata)
{
	((MarkdownImporter*)userdata)->_debug_log(msg);
}

void MarkdownImporter::_debug_log(const char* msg)
{
	((MarkdownImporter*)userdata)->_debug_log(msg);
}


} // namespace Laidout
