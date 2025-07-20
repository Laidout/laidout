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
// Copyright (C) 2025 by Tom Lechner
//
#include "accordion.h"
#include "../language.h"
#include <lax/strmanip.h>

#include <iostream>
using namespace std;
#define DBG

using namespace Polyptych;
using namespace Laxkit;
using namespace LaxInterfaces;

namespace Laidout {


/*! which can be : "[accordion] [type] [config1] [config 2]"
 * - config1 and config2 are numbers that depend on type. For instance, AccordionNxM requires 2 ints. Accordion1xN requires 1 int. EasyZine requires no ints.
 * - The initial "accordion" can be present or not. It is ignored.
 * - type is one of Accordion::AccordionPresets, other than "unknown".
 */
NetImposition *CreateAccordion(const char *which, double paper_width, double paper_height)
{
	//if (isblank(which)) return nullptr;

	DBG cerr << "CreateAccordion in "<<paper_width<<' '<<paper_height<<endl;
	if (strcasestr(which, "accordion") == which && !isalnum(which[9])) which += 9;
	while (isspace(*which)) which++;

	int n = 0;
	char **strs = splitspace(which, &n);

	Accordion::AccordionPresets type = Accordion::EasyZine;
	int parsing = 0;
	if (n > 0 && isalpha(*(strs[0]))) {
		if      (strcasecmp(strs[0], "HalfFold")       == 0) type = Accordion::HalfFold;
		else if (strcasecmp(strs[0], "VerticalHalf")   == 0) type = Accordion::VerticalHalf;
		else if (strcasecmp(strs[0], "TriFold")        == 0) type = Accordion::TriFold;
		else if (strcasecmp(strs[0], "ZFold")          == 0) type = Accordion::ZFold;
		else if (strcasecmp(strs[0], "ZFoldVertical")  == 0) type = Accordion::ZFoldVertical;
		else if (strcasecmp(strs[0], "FrenchQuarter")  == 0) type = Accordion::FrenchQuarter;
		else if (strcasecmp(strs[0], "Gate")           == 0) type = Accordion::Gate;
		else if (strcasecmp(strs[0], "DoubleGate")     == 0) type = Accordion::DoubleGate;
		else if (strcasecmp(strs[0], "Roll")           == 0) type = Accordion::Roll;
		else if (strcasecmp(strs[0], "DoubleParallel") == 0) type = Accordion::DoubleParallel;
		else if (strcasecmp(strs[0], "MapNxM")         == 0) type = Accordion::MapNxM;
		else if (strcasecmp(strs[0], "EasyZine")       == 0) type = Accordion::EasyZine;
		else if (strcasecmp(strs[0], "Accordion1xN")   == 0) type = Accordion::Accordion1xN;
		else if (strcasecmp(strs[0], "Accordion2xN")   == 0) type = Accordion::Accordion2xN;
		else if (strcasecmp(strs[0], "Accordion4x4")   == 0) type = Accordion::Accordion4x4;
		else if (strcasecmp(strs[0], "AccordionNxM")   == 0) type = Accordion::AccordionNxM;
		else if (strcasecmp(strs[0], "Miura")          == 0) type = Accordion::Miura;
		parsing++;
	}

	int config1 = 0, config2 = 0;
	if (parsing < n) {
		config1 = strtol(strs[parsing], nullptr, 10);
		parsing++;
	}
	if (parsing < n) {
		config2 = strtol(strs[parsing], nullptr, 10);
		parsing++;
	}

	deletestrs(strs, n);

	return Accordion::Build(type, paper_width, paper_height, config1, config2, nullptr);
	//return Accordion::BuildEasyZine(paper_width, paper_height, nullptr);
}


/*! \class Accordion
 * 
 * This class is for a specialized form of accordion style imposition,
 * where the accordion folds horizontally, but the paper itself wraps both vertically and horizontally.
 * The case where you have 1 x n pages can be handled by normal Signatures.
 * When you have n x m (where n >= 2, and both n and m are even), many pages need to fold in the opposite direction
 * from the bottom layer, something normal signature folding cannot do.
 * Until nets are capable of origami, this class is a workaround.
 */

Accordion::Accordion()
{}


//the ObjectDef for the presets enum
Laxkit::SingletonKeeper Accordion::presets;

ObjectDef *Accordion::GetPresets()
{
	ObjectDef *def = dynamic_cast<ObjectDef*>(presets.GetObject());
	if (def) return def;

	def = new ObjectDef(nullptr, "AccordionPresets", _("Accordion Presets"), nullptr /*ndesc*/,
							 "enum", NULL, "EasyZine",
							 NULL, 0,
							 nullptr /*nnewfunc*/, nullptr /*nstylefunc*/);
	presets.SetObject(def, 1);

	def->pushEnumValue("Unknown",       _("Unknown"),        nullptr, Unknown);
	def->pushEnumValue("HalfFold",      _("HalfFold"),       _("pamphlet just folded down the middle")                        , HalfFold       );
	def->pushEnumValue("VerticalHalf",  _("VerticalHalf"),   _("like HalfFold, but vertical")                                 , VerticalHalf   );
	def->pushEnumValue("TriFold",       _("TriFold"),        _("aka C-fold, 3 panels, fold from right, then from left")       , TriFold        );
	def->pushEnumValue("ZFold",         _("ZFold"),          _("3 panel accordion")                                           , ZFold          );
	def->pushEnumValue("ZFoldVertical", _("ZFoldVertical"),  _("3 panel accordion, vertical")                                 , ZFoldVertical  );
	def->pushEnumValue("FrenchQuarter", _("FrenchQuarter"),  _("fold down vertically, then horizontally")                     , FrenchQuarter  );
	def->pushEnumValue("Gate",          _("Gate"),           _("aka Single Open, 3 panel, but center panel is double size")   , Gate           );
	def->pushEnumValue("DoubleGate",    _("DoubleGate"),     _("aka Double Open, 4 panels, like Gate, but fold in center")    , DoubleGate     );
	def->pushEnumValue("Roll",          _("Roll"),           _("4 panel, fold in from R, again from r, then l")               , Roll           );
	def->pushEnumValue("DoubleParallel",_("DoubleParallel"), _("fold in half from r, then fold in half again")                , DoubleParallel );
	def->pushEnumValue("MapNxM",        _("MapNxM"),         _("A generic N x M grid of pages")                               , MapNxM         );
	def->pushEnumValue("EasyZine",      _("EasyZine"),       _("4x2, with cut in the middle. Same as Accordion2xN with n=4")  , EasyZine       );
	def->pushEnumValue("Accordion1xN",  _("Accordion1xN"),   _("1 x n accordion. Not even numbers fold out nicely.")          , Accordion1xN   );
	def->pushEnumValue("Accordion2xN",  _("Accordion2xN"),   _("n is even. fold in half vertically, cut across middle panels"), Accordion2xN   );
	def->pushEnumValue("Accordion4x4",  _("Accordion4x4"),   _("4x4, but partial cuts from left, right, left")                , Accordion4x4   );
	def->pushEnumValue("AccordionNxM",  _("AccordionNxM"),   _("zig zagging accordion, n and m must be even")                 , AccordionNxM   );
	def->pushEnumValue("Miura",         _("Miura"),          _("7x5 with slanting pages for rigid origami-like folding.")     , Miura          );

	return def;
}

int Accordion::NumParams(AccordionPresets type, int *default_1_ret, int *default_2_ret)
{
	switch (type) {
		case Unknown:        if (default_1_ret) { *default_1_ret = 0; *default_2_ret = 0; } return 0;
		case HalfFold:       if (default_1_ret) { *default_1_ret = 2; *default_2_ret = 1; } return 0;
		case VerticalHalf:   if (default_1_ret) { *default_1_ret = 1; *default_2_ret = 2; } return 0;
		case TriFold:        if (default_1_ret) { *default_1_ret = 3; *default_2_ret = 1; } return 0;
		case ZFold:          if (default_1_ret) { *default_1_ret = 3; *default_2_ret = 1; } return 0;
		case ZFoldVertical:  if (default_1_ret) { *default_1_ret = 1; *default_2_ret = 3; } return 0;
		case DoubleGate:     if (default_1_ret) { *default_1_ret = 4; *default_2_ret = 1; } return 0;
		case Roll:           if (default_1_ret) { *default_1_ret = 4; *default_2_ret = 1; } return 0;
		case EasyZine:       if (default_1_ret) { *default_1_ret = 4; *default_2_ret = 2; } return 0;
		case FrenchQuarter:  if (default_1_ret) { *default_1_ret = 2; *default_2_ret = 2; } return 0;
		case Gate:           if (default_1_ret) { *default_1_ret = 3; *default_2_ret = 1; } return 0;
		case Accordion1xN:   if (default_1_ret) { *default_1_ret = 4; *default_2_ret = 1; } return 1;
		case Accordion2xN:   if (default_1_ret) { *default_1_ret = 4; *default_2_ret = 2; } return 2;
		case Accordion4x4:   if (default_1_ret) { *default_1_ret = 4; *default_2_ret = 4; } return 0;
		case AccordionNxM:   if (default_1_ret) { *default_1_ret = 6; *default_2_ret = 4; } return 2;
		case DoubleParallel: if (default_1_ret) { *default_1_ret = 4; *default_2_ret = 1; } return 0;
		case MapNxM:         if (default_1_ret) { *default_1_ret = 4; *default_2_ret = 3; } return 0;
		case Miura:          if (default_1_ret) { *default_1_ret = 7; *default_2_ret = 5; } return 2;
	}
	return 0;
}


NetImposition *Accordion::Build(AccordionPresets preset, double paper_width, double paper_height, int config1, int config2, NetImposition *existing_netimp)
{
	switch (preset)
	{
		case Unknown:
			return nullptr;

		case HalfFold: // pamphlet just folded down the middle
			return BuildAccordion1xN(2, false, paper_width, paper_height, HalfFold, existing_netimp);

		case VerticalHalf: // like HalfFold, but vertical
			return BuildAccordion1xN(2, true, paper_width, paper_height, VerticalHalf, existing_netimp);

		case TriFold: // aka C-fold, 3 panels, one folded forward from right, one folded forward from left
			return BuildAccordion1xN(3, false, paper_width, paper_height, TriFold, existing_netimp);

		case ZFold: // 3 panel accordion
			return BuildAccordion1xN(3, false, paper_width, paper_height, ZFold, existing_netimp);

		case ZFoldVertical: // 3 panel accordion, vertical
			return BuildAccordion1xN(3, true, paper_width, paper_height, HalfFold, existing_netimp);

		case DoubleGate: // aka Double Open, 4 panels, fold in from right to center, fold from left to center, fold at center
			return BuildAccordion1xN(4, false, paper_width, paper_height, DoubleGate, existing_netimp);

		case Roll: // 4 panel, fold in from R, again from r, then l
			return BuildAccordion1xN(4, false, paper_width, paper_height, Roll, existing_netimp);

		case EasyZine: // 2x4, with cut in the middle. Same as Accordion2xN with n=4
			return BuildEasyZine(paper_width, paper_height, existing_netimp);

		case FrenchQuarter: // fold down vertically in half, then horizontally in half
			return BuildAccordionNxM(2, 2, paper_width, paper_height, FrenchQuarter, existing_netimp);

		case Gate: // aka Single Open, 3 panel, but center panel is twice the l and r panels
			return BuildGate(paper_width, paper_height, existing_netimp);

		case Accordion1xN: // 1 x n accordion. Not even numbers fold out nicely.
			return BuildAccordion1xN(config1, false, paper_width, paper_height, Accordion1xN, existing_netimp);

		case Accordion2xN: // n is even. fold in half vertically, cut across middle panels, lower left 2 panels are front/back
			return BuildAccordionNxM(config1, 2, paper_width, paper_height, Accordion2xN, existing_netimp);

		case Accordion4x4: // cut in 3 panels horizontally from left, then right, then left
			return BuildAccordionNxM(4, 4, paper_width, paper_height, Accordion4x4, existing_netimp);

		case AccordionNxM: // zig zagging accordion, n and m must be even
			return BuildAccordionNxM(config1, config2, paper_width, paper_height, AccordionNxM, existing_netimp);

		case DoubleParallel: // fold in half from r, then fold in half again
			return BuildAccordionNxM(2, 2, paper_width, paper_height, DoubleParallel, existing_netimp);

		case MapNxM: // Generic grid of pages
			return BuildAccordionNxM(4, 3, paper_width, paper_height, MapNxM, existing_netimp);

		case Miura: // 7x5 with slanting pages for rigid origami-like folding.
			return BuildMiura(config1, config2, 6, paper_width, paper_height, existing_netimp);
	}

	return nullptr;
}

/*! Returns hedron on success, or nullptr if settings invalid.
 * If hedron == nullptr; return a new Polyhedron.
 */
Polyhedron *CreateRectangleGrid(Polyhedron *hedron,double paper_width, double paper_height, int num_wide, int num_tall, bool alternate_up)
{
	double page_w = paper_width / num_wide;
	double page_h = paper_height / num_tall;

	Polyhedron *poly = hedron;
	if (!poly) poly = new Polyhedron;

	// add vertices
	for (int y = 0; y < num_tall+1; y++) {
		for (int x = 0; x < num_wide+1; x++) {
			poly->AddPoint(spacevector(x*page_w, y*page_h, 0));
		}
	}

	// add faces
	for (int y = 0; y < num_tall; y++) {
		for (int x = 0; x < num_wide; x++) {
			if (alternate_up && y%2 == 1) {
				int i = (num_wide - x - 1) + y*(num_wide+1);
				poly->AddFace(4, i+num_wide+2, i+num_wide+1, i, i+1);
			} else {
				int i = x + y*(num_wide+1);
				poly->AddFace(4, i, i+1, i+num_wide+2, i+num_wide+1);
			}
		}
	}

	poly->makeedges();
	return poly;
}

/*! Returns hedron on success, or nullptr if settings invalid.
 * If hedron == nullptr; return a new Polyhedron.
 */
NetImposition *Accordion::BuildGate(double paper_width, double paper_height, NetImposition *existing_netimp)
{
	NetImposition *netimp = existing_netimp;
	if (netimp) {
		if (netimp->abstractnet) { netimp->abstractnet->dec_count(); netimp->abstractnet = nullptr; }
		netimp->nets.flush();
	} else {
		netimp = new NetImposition();
		netimp->fit_paper_asymmetric = true;
		netimp->margin = 0;
	}
	
	double page_w = paper_width / 4;
	double page_h = paper_height;

	Polyhedron *poly = new Polyhedron;

	// add vertices
	poly->AddPoint(spacevector(0,        0, 0));      // 0
	poly->AddPoint(spacevector(page_w,   0, 0));      // 1
	poly->AddPoint(spacevector(3*page_w, 0, 0));      // 2
	poly->AddPoint(spacevector(4*page_w, 0, 0));      // 3
	poly->AddPoint(spacevector(0,        page_h, 0)); // 4
	poly->AddPoint(spacevector(page_w,   page_h, 0)); // 5
	poly->AddPoint(spacevector(3*page_w, page_h, 0)); // 6
	poly->AddPoint(spacevector(4*page_w, page_h, 0)); // 7

	// add faces
	poly->AddFace(4, 0, 1, 5, 4);
	poly->AddFace(4, 1, 2, 6, 5);
	poly->AddFace(4, 2, 3, 7, 6);
	
	poly->makeedges();

	Net *net = new Net;
	makestr(net->netname, _("Gate"));
	net->basenet = poly;
	net->Anchor(0);
	net->TotalUnwrap(true);
	//net->CollapseEdges();
	net->DetectAndSetEdgeStyles();
	net->rebuildLines();

	netimp->SetNet(net);
	net->dec_count();
	return netimp;
}


// 4x2 with a cut in the middle.
NetImposition *Accordion::BuildEasyZine(double paper_width, double paper_height, NetImposition *existing_netimp)
{
	NetImposition *netimp = existing_netimp;
	if (netimp) {
		if (netimp->abstractnet) { netimp->abstractnet->dec_count(); netimp->abstractnet = nullptr; }
		netimp->nets.flush();
	} else {
		netimp = new NetImposition();
		netimp->fit_paper_asymmetric = true;
		netimp->margin = 0;
	}
	
	// htiles = 4;
	// vtiles = 2;

	Polyhedron *poly = CreateRectangleGrid(nullptr, paper_width, paper_height, 4, 2, true);
	// DBG cerr << "--------make accordion:"<<endl;
	// DBG poly->dump_out(stderr, 2,0,nullptr);
	poly->SetEdgeInfo(6,7, Net::EDGE_Hard_Cut);
	poly->SetEdgeInfo(7,8, Net::EDGE_Hard_Cut);
	poly->SetEdgeInfo(1,6,  Net::EDGE_Fold_Peak);
	poly->SetEdgeInfo(2,7,  Net::EDGE_Fold_Peak);
	poly->SetEdgeInfo(8,9,  Net::EDGE_Fold_Peak);
	poly->SetEdgeInfo(7,12, Net::EDGE_Fold_Peak);
	poly->SetEdgeInfo(5,6,  Net::EDGE_Fold_Peak);

	Net *net = new Net;
	// makestr(net->netname,poly->name);
	net->basenet = poly;

	//unwrap
	net->Anchor(0);
	net->Unwrap(0,1);
	net->Unwrap(1,1);
	net->Unwrap(2,1);
	net->TotalUnwrap(true);
	net->CollapseEdges();
	net->DetectAndSetEdgeStyles();

	// net->faces.e[1]->edges[2]->info = Net::EDGE_Hard_Cut; //todo: this should really be built into the hedron
	// net->faces.e[2]->edges[2]->info = Net::EDGE_Hard_Cut;
	// net->faces.e[5]->edges[2]->info = Net::EDGE_Hard_Cut;
	// net->faces.e[6]->edges[2]->info = Net::EDGE_Hard_Cut;

	// net->faces.e[0]->edges[1]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[0]->edges[2]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[1]->edges[1]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[1]->edges[3]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[2]->edges[3]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[3]->edges[2]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[4]->edges[2]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[5]->edges[1]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[6]->edges[3]->info = Net::EDGE_Fold_Peak;
	// net->faces.e[7]->edges[2]->info = Net::EDGE_Fold_Peak;
	net->rebuildLines();
	
	netimp->SetNet(net);
	net->dec_count();

	return netimp;
}

// 4x2 with a cut in the middle.
NetImposition *Accordion::BuildAccordion1xN(int n, bool vertical, double paper_width, double paper_height, int variation, NetImposition *existing_netimp)
{
	if (n < 2) return nullptr;

	NetImposition *netimp = existing_netimp;
	if (netimp) {
		if (netimp->abstractnet) { netimp->abstractnet->dec_count(); netimp->abstractnet = nullptr; }
		netimp->nets.flush();
	} else {
		netimp = new NetImposition();
		netimp->fit_paper_asymmetric = true;
		netimp->margin = 0;
	}
	
	// if (vertical) { htiles = 1; vtiles = n; }
	// else  { htiles = n; vtiles = 1; }
	
	Polyhedron *poly = CreateRectangleGrid(nullptr, paper_width, paper_height, vertical ? 1 : n, vertical ? n : 1, false);
	Net *net = new Net;
	makestr(net->netname, "Accordion");
	net->basenet = poly;

	//unwrap
	net->Anchor(0);
	net->TotalUnwrap(true);
	// net->CollapseEdges();
	net->DetectAndSetEdgeStyles();
	net->rebuildLines();
	
	netimp->SetNet(net);
	net->dec_count();

	return netimp;
}

// Zig zagging accordion.
NetImposition *Accordion::BuildAccordionNxM(int n, int m, double paper_width, double paper_height, int variation, NetImposition *existing_netimp)
{
	if (n < 2) return nullptr;

	NetImposition *netimp = existing_netimp;
	if (netimp) {
		if (netimp->abstractnet) { netimp->abstractnet->dec_count(); netimp->abstractnet = nullptr; }
		netimp->nets.flush();
	} else {
		netimp = new NetImposition();
		netimp->fit_paper_asymmetric = true;
		netimp->margin = 0;
	}
	
	// if (vertical) { htiles = 1; vtiles = n; }
	// else  { htiles = n; vtiles = 1; }
	
	Polyhedron *poly = CreateRectangleGrid(nullptr, paper_width, paper_height, n, m, variation == MapNxM ? false : true);
	Net *net = new Net;
	makestr(net->netname, "Accordion");
	net->basenet = poly;

	int xoff, i;
	for (int y = 1; y < m; y++) {
		for (int x = 1; x < n; x++) {
			i = y*(n+1) + x;
			xoff = 0;
			if (y % 2 == 1) xoff = 1;
			poly->SetEdgeInfo(i-xoff, i-xoff+1, Net::EDGE_Hard_Cut); // horizontal cuts
			if (x % 2 == 0) {
				poly->SetEdgeInfo(i, i-(n+1), Net::EDGE_Fold_Peak); //peaks below
				if (y == m -1) poly->SetEdgeInfo(i, i+(n+1), Net::EDGE_Fold_Peak); //final peaks above
			}
		}
		// horizontal folds
		i = y * (n+1);
		if (y % 2 == 1) xoff = n-1; else xoff = 0;
		if (m % 2 == 1) {
			if (y % 2 == 1)
				poly->SetEdgeInfo(i+xoff, i+xoff+1, Net::EDGE_Fold_Peak);
			// else
			// 	poly->SetEdgeInfo(i+xoff, i+xoff+1, Net::EDGE_Fold);
		}
		else poly->SetEdgeInfo(i+xoff, i+xoff+1, Net::EDGE_Fold_Peak);
	}

	//unwrap
	net->Anchor(0);
	net->TotalUnwrap(true);
	net->CollapseEdges();
	net->DetectAndSetEdgeStyles();
	net->rebuildLines();
	
	netimp->SetNet(net);
	net->dec_count();

	return netimp;
}


/*! The Miura fold, a rigid origami style of paper fold that kind of folds itself. */
NetImposition *Accordion::BuildMiura(int n, int m, double angle_degrees, double paper_width, double paper_height, NetImposition *existing_netimp)
{
	if (n == 0 && m == 0) { n = 7; m = 5; }
	if (n < 2 || m < 2) return nullptr;

	NetImposition *netimp = existing_netimp;
	if (netimp) {
		if (netimp->abstractnet) { netimp->abstractnet->dec_count(); netimp->abstractnet = nullptr; }
		netimp->nets.flush();
	} else {
		netimp = new NetImposition();
		netimp->fit_paper_asymmetric = true;
		netimp->margin = 0;
	}
	
	// if (vertical) { htiles = 1; vtiles = n; }
	// else  { htiles = n; vtiles = 1; }
	
	// jiggle the vertices left and right
	double h = paper_height / m;
	double d = h * tan(angle_degrees * M_PI/180);
	//double w = (paper_width - d) / n;
	Polyhedron *poly = CreateRectangleGrid(nullptr, paper_width, paper_height, n, m, false);
	
	// define folds
	int i;
	for (int y = 0; y < m; y++) {
		for (int x = 0; x < n; x++) {
			i = y*(n+1) + x;

			if (x % 2 == 0 && x > 0)
				poly->SetEdgeInfo(i, i+n+1, Net::EDGE_Fold_Peak); // vertical fold

			if (y % 2 == 0) {
				if (x % 2 == 0) poly->SetEdgeInfo(i, i+1, Net::EDGE_Fold_Peak);
			} else {
				if (x % 2 == 1) poly->SetEdgeInfo(i, i+1, Net::EDGE_Fold_Peak);
			}

		}
	}

	// apply jiggle
	for (int y = 0; y <= m; y++) {
		for (int x = 1; x < n; x++) {
			int i = x + y*(n+1);
			
			double dd = (y%2 == 0 ? d/2 : -d/2);
			poly->vertices.e[i].x += dd;
		}
	}
	Net *net = new Net;
	makestr(net->netname, "Miura");
	net->basenet = poly;

	//unwrap
	net->Anchor(0);
	net->TotalUnwrap(true);
	net->CollapseEdges();
	net->DetectAndSetEdgeStyles();
	net->rebuildLines();
	
	netimp->SetNet(net);
	net->dec_count();

	return netimp;
}

} // namespace Laidout
