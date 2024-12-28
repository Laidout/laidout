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
// Copyright (C) 2025-present by Tom Lechner
//


//
// Utilities for things related to opentype fonts.
//


#include <cstring>

#include "opentype.h"
#include <lax/language.h>


namespace Laidout {


struct OpentypeTagName {
	const char *tag;
	const char *name;
};

static int num_tag_names = -1;
static const OpentypeTagName opentype_tag_names[] = 
{
	{ "aalt", _("Access All Alternates") },
	{ "abvf", _("Above-base Forms") },
	{ "abvm", _("Above-base Mark Positioning") },
	{ "abvs", _("Above-base Substitutions") },
	{ "afrc", _("Alternative Fractions") },
	{ "akhn", _("Akhand") },
	{ "apkn", _("Kerning for Alternate Proportional Widths") },
	{ "blwf", _("Below-base Forms") },
	{ "blwm", _("Below-base Mark Positioning") },
	{ "blws", _("Below-base Substitutions") },
	{ "calt", _("Contextual Alternates") },
	{ "case", _("Case-sensitive Forms") },
	{ "ccmp", _("Glyph Composition / Decomposition") },
	{ "cfar", _("Conjunct Form After Ro") },
	{ "chws", _("Contextual Half-width Spacing") },
	{ "cjct", _("Conjunct Forms") },
	{ "clig", _("Contextual Ligatures") },
	{ "cpct", _("Centered CJK Punctuation") },
	{ "cpsp", _("Capital Spacing") },
	{ "cswh", _("Contextual Swash") },
	{ "curs", _("Cursive Positioning") },
	{ "cv01", _(" 'cv99' 	Character Variant 1 – Character Variant 99") },
	{ "c2pc", _("Petite Capitals From Capitals") },
	{ "c2sc", _("Small Capitals From Capitals") },
	{ "dist", _("Distances") },
	{ "dlig", _("Discretionary Ligatures") },
	{ "dnom", _("Denominators") },
	{ "dtls", _("Dotless Forms") },
	{ "expt", _("Expert Forms") },
	{ "falt", _("Final Glyph on Line Alternates") },
	{ "fin2", _("Terminal Forms #2") },
	{ "fin3", _("Terminal Forms #3") },
	{ "fina", _("Terminal Forms") },
	{ "flac", _("Flattened Accent Forms") },
	{ "frac", _("Fractions") },
	{ "fwid", _("Full Widths") },
	{ "half", _("Half Forms") },
	{ "haln", _("Halant Forms") },
	{ "halt", _("Alternate Half Widths") },
	{ "hist", _("Historical Forms") },
	{ "hkna", _("Horizontal Kana Alternates") },
	{ "hlig", _("Historical Ligatures") },
	{ "hngl", _("Hangul") },
	{ "hojo", _("Hojo Kanji Forms (JIS X 0212-1990 Kanji Forms)") },
	{ "hwid", _("Half Widths") },
	{ "init", _("Initial Forms") },
	{ "isol", _("Isolated Forms") },
	{ "ital", _("Italics") },
	{ "jalt", _("Justification Alternates") },
	{ "jp78", _("JIS78 Forms") },
	{ "jp83", _("JIS83 Forms") },
	{ "jp90", _("JIS90 Forms") },
	{ "jp04", _("JIS2004 Forms") },
	{ "kern", _("Kerning") },
	{ "lfbd", _("Left Bounds") },
	{ "liga", _("Standard Ligatures") },
	{ "ljmo", _("Leading Jamo Forms") },
	{ "lnum", _("Lining Figures") },
	{ "locl", _("Localized Forms") },
	{ "ltra", _("Left-to-right Alternates") },
	{ "ltrm", _("Left-to-right Mirrored Forms") },
	{ "mark", _("Mark Positioning") },
	{ "med2", _("Medial Forms #2") },
	{ "medi", _("Medial Forms") },
	{ "mgrk", _("Mathematical Greek") },
	{ "mkmk", _("Mark to Mark Positioning") },
	{ "mset", _("Mark Positioning via Substitution") },
	{ "nalt", _("Alternate Annotation Forms") },
	{ "nlck", _("NLC Kanji Forms") },
	{ "nukt", _("Nukta Forms") },
	{ "numr", _("Numerators") },
	{ "onum", _("Oldstyle Figures") },
	{ "opbd", _("Optical Bounds") },
	{ "ordn", _("Ordinals") },
	{ "ornm", _("Ornaments") },
	{ "palt", _("Proportional Alternate Widths") },
	{ "pcap", _("Petite Capitals") },
	{ "pkna", _("Proportional Kana") },
	{ "pnum", _("Proportional Figures") },
	{ "pref", _("Pre-base Forms") },
	{ "pres", _("Pre-base Substitutions") },
	{ "pstf", _("Post-base Forms") },
	{ "psts", _("Post-base Substitutions") },
	{ "pwid", _("Proportional Widths") },
	{ "qwid", _("Quarter Widths") },
	{ "rand", _("Randomize") },
	{ "rclt", _("Required Contextual Alternates") },
	{ "rkrf", _("Rakar Forms") },
	{ "rlig", _("Required Ligatures") },
	{ "rphf", _("Reph Form") },
	{ "rtbd", _("Right Bounds") },
	{ "rtla", _("Right-to-left Alternates") },
	{ "rtlm", _("Right-to-left Mirrored Forms") },
	{ "ruby", _("Ruby Notation Forms") },
	{ "rvrn", _("Required Variation Alternates") },
	{ "salt", _("Stylistic Alternates") },
	{ "sinf", _("Scientific Inferiors") },
	{ "size", _("Optical size") },
	{ "smcp", _("Small Capitals") },
	{ "smpl", _("Simplified Forms") },
	{ "ss01", _(" 'ss20' 	Stylistic Set 1 – Stylistic Set 20") },
	{ "ssty", _("Math Script-style Alternates") },
	{ "stch", _("Stretching Glyph Decomposition") },
	{ "subs", _("Subscript") },
	{ "sups", _("Superscript") },
	{ "swsh", _("Swash") },
	{ "titl", _("Titling") },
	{ "tjmo", _("Trailing Jamo Forms") },
	{ "tnam", _("Traditional Name Forms") },
	{ "tnum", _("Tabular Figures") },
	{ "trad", _("Traditional Forms") },
	{ "twid", _("Third Widths") },
	{ "unic", _("Unicase") },
	{ "valt", _("Alternate Vertical Metrics") },
	{ "vapk", _("Kerning for Alternate Proportional Vertical Metrics") },
	{ "vatu", _("Vattu Variants") },
	{ "vchw", _("Vertical Contextual Half-width Spacing") },
	{ "vert", _("Vertical Alternates") },
	{ "vhal", _("Alternate Vertical Half Metrics") },
	{ "vjmo", _("Vowel Jamo Forms") },
	{ "vkna", _("Vertical Kana Alternates") },
	{ "vkrn", _("Vertical Kerning") },
	{ "vpal", _("Proportional Alternate Vertical Metrics") },
	{ "vrt2", _("Vertical Alternates and Rotation") },
	{ "vrtr", _("Vertical Alternates for Rotation") },
	{ "zero", _("Slashed Zero") },
	{ nullptr, nullptr }
};

const char *GetHumanTagName(const char *four_char_tag)
{
	if (num_tag_names < 0) {
		num_tag_names = 0;
		while (opentype_tag_names[num_tag_names].tag) num_tag_names++;
	}

	int s = 0;
	int e = num_tag_names;
	int m;
	int c;
	while (s <= e) {
		c = strcmp(four_char_tag, opentype_tag_names[s].tag);
		if (c < 0) return nullptr;
		if (c == 0) return opentype_tag_names[s].name;
		c = strcmp(four_char_tag, opentype_tag_names[e].tag);
		if (c > 0) return nullptr;
		if (c == 0) return opentype_tag_names[e].name;
		m = (s+e)/2;
		if (m == s || m == e) break;
		c = strcmp(four_char_tag, opentype_tag_names[m].tag);
		if (c == 0) return opentype_tag_names[m].name;		
		if (c > 0) { s = m+1; e--; }
		else { e = m-1; s++; }
	}
	return nullptr;
}


} // namespace Laidout
