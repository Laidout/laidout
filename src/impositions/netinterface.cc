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



#include "netinterface.h"

#include <lax/language.h>
#include <lax/strmanip.h>
#include <lax/freedesktop.h>
#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/fileutils.h>
#include <lax/filedialog.h>


#include <lax/debug.h>

using namespace std;
using namespace Laxkit;


namespace Laidout {



//---------------------------- misc ----------------------
enum PaperPart {
	PAPER_None = 0,
	PAPER_Left,
	PAPER_Right,
	PAPER_Top,
	PAPER_Bottom,
	PAPER_Inside
};


//! Save a Polyptych file
/*! Return 0 for success or nonzero for error.
 */
int PanoramaInfo::SavePolyptych(const char *saveto)
{
	FILE *f = fopen(saveto,"w");
	if (!f) {
		DBGW("WARNING: could not open "<<saveto<<" for saving!");
		return 1;
	}
	
	setlocale(LC_ALL, "C");

	if (polyhedronfile && *polyhedronfile) fprintf(f,"polyhedronfile %s\n",polyhedronfile);
	if (spherefile && *spherefile) fprintf(f,"spherefile %s\n",spherefile);
	
	
	fprintf(f,"basis\n"
			  "  p %.10g,%.10g,%.10g\n"
			  "  x %.10g,%.10g,%.10g\n"
			  "  y %.10g,%.10g,%.10g\n"
			  "  z %.10g,%.10g,%.10g\n",
			   extra_basis.p.x, extra_basis.p.y, extra_basis.p.z,
			   extra_basis.x.x, extra_basis.x.y, extra_basis.x.z,
			   extra_basis.y.x, extra_basis.y.y, extra_basis.y.z,
			   extra_basis.z.x, extra_basis.z.y, extra_basis.z.z
		   );
	if (nets.n) {
		for (int c = 0; c < nets.n; c++) {
			if (nets.e[c]->numActual() == 1) continue;  // don't bother with single face nets
			fprintf(f, "net\n");
			nets.e[c]->dump_out(f, 2, 0, nullptr);
		}
	}

	setlocale(LC_ALL, "");
	fclose(f);
	touch_recently_used_xbel(saveto, "application/x-polyptych-doc", nullptr,nullptr, "Polyptych", true,true, nullptr);
	return 0;
}

int PanoramaInfo::RenderPanorama(Document *doc, const Laxkit::Basis &basis_tweak)
{
	//*** // This should be some kind of image importer.
	DBGE("IMPLEMENT ME!!!");
	return -1;
}


//--------------------------------- NetInterface modes and other things -------------------------------


// hovering indicators
enum NetInterfaceHover {
	NETI_None = 0,

	// 3-d things:
	NETI_Face,
	NETI_Potential,
	NETI_Paper,
	 
	NETI_HOVER_MAX
};

enum NetInterfaceAction {
	NETI_Help,
	NETI_Papers,
	NETI_LoadPolyhedron,
	NETI_LoadImage,
	NETI_ToggleMode,
	NETI_Render,
	NETI_Save,
	NETI_SaveAs,
	NETI_ResetNets,
	NETI_TotalUnwrap,
	NETI_DrawSeams,
	NETI_ToggleTexture,
	NETI_DrawEdges,
	NETI_NextFace,
	NETI_PrevFace,
	// NETI_NextObject,
	// NETI_ScaleUp,
	// NETI_ScaleDown,
	// NETI_ResetView,
	// NETI_Stereo,
	NETI_MAX
};



//--------------------------- NetInterface -------------------------------


/*! class NetInterface
 * \brief Editor for NetImposition.
 *
 * poly->face->cache->facemode==-net.object_id if face is seed face.
 * facemode==0 if face still on hedron.
 * facemode==net.object_id if face is a non-seed face of that net.
 */
/*! \var Laxkit::RefPtrStack<Net> nets
 * \brief Stack of nets attached to hedron faces.
 *
 * Each net->info is the index of the original face that acts as the seed
 * for that net.
 */


/*! If newpoly, then inc it's count. It does NOT create a copy from it current, so watch out.
 */
NetInterface::NetInterface(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		NetImposition *new_net)
 	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,nullptr,0,0)
{
	font = app->defaultlaxfont;
	font->inc_count();

	net_imp = new_net;
	if (net_imp) net_imp->inc_count();

	draw_papers      = true;
	draw_texture     = true;
	draw_axes        = true;
	draw_info        = true;
	draw_overlays    = true;
	draw_edges       = true;
	draw_unwrap_path = true;

	mouseover_overlay = -1;
	mouseover_index   = -1;
	mouseover_group   = NETI_None;
	mouseover_paper   = -1;
	grab_overlay      = -1;  // mouse down on this overlay
	active_action     = ACTION_None;
	touchmode         = 0;

	pad = 20;

	curobj           = 0;
	currentface      = -1;
	currentpotential = -1;
	currentnet       = nullptr;

	sc = nullptr;
}

NetInterface::~NetInterface()
{
	if (original_netimp) original_netimp->dec_count();
	if (current_netimp)  current_netimp ->dec_count();
	if (currentnet)      currentnet     ->dec_count();
	if (poly)            poly           ->dec_count();
	if (sc)              sc             ->dec_count();
	if (doc)             doc            ->dec_count();
	if (paper_interface) paper_interface->dec_count();
}

const char *NetInterface::Name()
{
	return _("Nets");
}

Imposition NetInterface::GetImposition()
{
	return current_netimp;
}

/*! Used by ImpositionEditor when imposearg specifies dimensions. */
int NetInterface::SetTotalDimensions(double width, double height)
{
	PaperStyle *p = new PaperStyle("Custom",width,height,0,300,NULL);
	SetPaper(p);
	p->dec_count();
	return 0;
}

/*! Return default paper size. */
int NetInterface::GetDimensions(double &width, double &height)
{
	PaperStyle *paper = nullptr;
	if (!current_netimp || !current_netimp->papergroup || !current_netimp->papergroup->papers.n)
		paper = laidout->GetDefaultPaper();
	else paper = papergroup->papers.e[0];

	width = paper->w();
	height = paper->h();

	return 0;
}

/*! Install dup of paper. */
int NetInterface::SetPaper(PaperStyle *paper)
{
	if (current_netimp) current_netimp->SetPaperSize(paper);
	return 0;
}

int NetInterface::UseThisDocument(Document *new_doc)
{
	if (!new_doc) return 1;
	if (new_doc != doc) {
		if (doc) doc->dec_count();
		doc = new_doc;
		doc->inc_count();
	}
	return 0;
}

int NetInterface::UseThisImposition(Imposition *imp)
{
	if (!imp) return 1;

	if (original_netimp != imp) {
		if (original_netimp) original_netimp->dec_count();
		original_netimp = imp;
		if (imp) imp->inc_count();
	}
	if (current_netimp != original_netimp) {
		if (current_netimp) current_netimp->dec_count();
		current_netimp = original_netimp;
		current_netimp->inc_count();
	}

	needtodraw = 1;
	return 0;
}

int NetInterface::ShowThisPaperSpread(int index)
{
	if (index < 0 || !current_netimp || index >= current_netimp->NumSpreads(PAPERLAYOUT)) return 1;
	current_paper_spread = index;
	needtodraw = 1;
	return current_paper_spread;
}


int NetInterface::init()
{
	if (!paper_interface) {
		paper_interface = new PaperInterface();
		if (papers) paper_interface->UseThis(papers);
	}

	// if (!poly) {
	// 	poly = defineCube();
	// 	nets.flush();
	// 	if (currentnet) currentnet->dec_count();
	// 	currentnet = nullptr;
	// 	makestr(polyhedronfile, nullptr);
	// 	currentface = currentpotential = -1;
	// 	needtodraw = 1;
	// }

	return 0;
}


int NetInterface::Refresh()
{ ***
	if (!win_on) return 0;
	needtodraw = 0;

	int c;

	// //---- draw things
	// if (draw_texture) {
	// 	DBG cerr <<"draw texture: "<<(spheremap_data?"yes ":"no ")<<" w="<<spheremap_width<<" h="<<spheremap_height<<endl;
		
	// 	// glEnable(GL_TEXTURE_2D);
	// }

	//------ perhaps save for future 3d?
	// // draw frame of hedron (without nets)
	// if (draw_edges) {
	// 	//GLUquadricObj *q;
	// 	glDisable(GL_TEXTURE_2D);

	// 	glPushMatrix();
	// 	glMultMatrixf(hedron->m);
	// 	//glScalef(hedron->scale[0],hedron->scale[1],hedron->scale[2]);
	// 	spacepoint p1,p2,a,v;
	// 	//double h;
	// 	//double ang;

	// 	glLineWidth(3);
	// 	glBegin(GL_LINES);
	// 	for (int c2=0; c2<poly->edges.n; c2++) {
	// 		if (poly->edges.e[c2]->p1<0 || poly->edges.e[c2]->p2<0) continue;

	// 		p1=poly->vertices[poly->edges.e[c2]->p1];
	// 		p2=poly->vertices[poly->edges.e[c2]->p2];
	// 		v=p1-p2;
	// 		a=v/spacepoint(0,0,1);
	// 		//h=norm(v);
	// 		//a=spacepoint(0,0,1)/v;
	// 		//ang=acos(v.z/h)/M_PI*180;

	// 		glVertex3f(p1.x,p1.y,p1.z);
	// 		glVertex3f(p2.x,p2.y,p2.z);

	// 	 //draw lines around any faces up in nets
	// 	for (int c2=0; c2<poly->faces.n; c2++) {
	// 		if (poly->faces.e[c2]->cache->facemode>=0) continue;
	// 		for (int c3=0; c3<poly->faces.e[c2]->pn; c3++) {
	// 			p1=poly->faces.e[c2]->cache->points3d[c3];
	// 			p2=(c3==0?poly->faces.e[c2]->cache->points3d[poly->faces.e[c2]->pn-1]
	// 					:poly->faces.e[c2]->cache->points3d[c3-1]);
	// 			v=p1-p2;
	// 			a=v/spacepoint(0,0,1);
	// 			//h=norm(v);
	// 			//a=spacepoint(0,0,1)/v;
	// 			//ang=acos(v.z/h)/M_PI*180;

	// 			glVertex3f(p1.x,p1.y,p1.z);
	// 			glVertex3f(p2.x,p2.y,p2.z);
	// 		}
	// 	}
	// 	glEnd();
	// 	glLineWidth(1);

	// 	glPopMatrix();
	// }
	// glDisable(GL_TEXTURE_2D);
	


	//  //------draw red cylinders on seam edges--
	// if ((draw_seams&1) && nets.n) {
	// 	int face;
	// 	//basis bas=poly->basisOfFace(face);
	// 	spacepoint p;
	// 	flatpoint v;
	// 	NetFaceEdge *edge;

	// 	for (int n=0; n<nets.n; n++) {
	// 	  for (int c=0; c<nets.e[n]->faces.n; c++) {
	// 		if (nets.e[n]->faces.e[c]->tag!=FACE_Actual) continue;
	// 		 //------draw red cylinders on seam edges--
	// 		spacepoint p1,p2;
	// 		for (int c2=0; c2<nets.e[n]->faces.e[c]->edges.n; c2++) {
	// 			 //continue if edge is not a seam
	// 			edge=nets.e[n]->faces.e[c]->edges.e[c2];
	// 			if (!(edge->toface==-1 || (edge->toface>=0 && nets.e[n]->faces.e[edge->toface]->tag!=FACE_Actual)))
	// 				continue;

	// 			//------------------------
	// 			face=nets.e[n]->faces.e[c]->original;
	// 			p1=poly->VertexOfFace(face,c2, 1);
	// 			p2=poly->VertexOfFace(face,(c2+1)%poly->faces.e[face]->pn, 1);
	// 			//------------------------
	// 			//v=transform_point(nets.e[n]->faces.e[c]->matrix, edge->points->p());
	// 			//p1=bas.p + v.x*bas.x + v.y*bas.y;
	// 			//v=transform_point(nets.e[n]->faces.e[c]->matrix, 
	// 							  //nets.e[n]->faces.e[c]->edges.e[(c2+1)%nets.e[n]->faces.e[c]->edges.n]->points->p());
	// 			//p2=bas.p + v.x*bas.x + v.y*bas.y;
	// 			//------------------------

	// 			setmaterial(1,.3,.3);
	// 			drawCylinder(p1,p2,cylinderscale,hedron->m);
	// 		}
	// 	  }
	// 	}
	// }

	
	 //---- draw dotted face outlines for each potential face of currentnet
	if (currentnet) {
		int face = currentnet->info; //the seed face for the net
		Basis bas = poly->basisOfFace(face);
		spacepoint p;
		flatpoint v;

		for (int c = 0; c < currentnet->faces.n; c++) {

			if (currentnet->faces.e[c]->tag!=FACE_Potential) continue;
			if (poly->faces.e[currentnet->faces.e[c]->original]->cache->facemode!=0) continue;

			glPushMatrix();
			glMultMatrixf(hedron->m);
			setmaterial(1,1,1);
			glLineStipple(1, 0x00ff);//factor, bit pattern 0=off 1=on
			glEnable(GL_LINE_STIPPLE);
			glBegin (GL_LINE_LOOP);

			for (int c2=0; c2<currentnet->faces.e[c]->edges.n; c2++) {
				v=transform_point(currentnet->faces.e[c]->matrix,currentnet->faces.e[c]->edges.e[c2]->points->p());
				p=bas.p + v.x*bas.x + v.y*bas.y;
				glVertex3f(p.x, p.y, p.z);
			}

			glEnd();
			glPopMatrix();
			glDisable(GL_LINE_STIPPLE);

		}

		if (currentpotential >= 0) {
			 //draw textured polygon if hovering over a potential in current net
			drawPotential(currentnet,currentpotential);
		}

		if (draw_papers) {
			paper_interface->Refresh();
		}
	}


	 //---draw current face
	if (buttondown.isdown(0,RIGHTBUTTON)) {
		if (rbdown >= 0) transparentFace(rbdown,1,0,0,.3);
		if (currentface >= 0 && rbdown!=currentface) transparentFace(currentface,0,1,0,.3);
	
	} else if (currentface >= 0) {
		if (currentfacestatus == 1) transparentFace(currentface,0,1,0,.3);//over a leaf
		else if (currentfacestatus == 2) transparentFace(currentface,0,0,1,.3);
		else transparentFace(currentface,1,0,0,.3);
	}

	return 0;
}


//! Currently draw face as a colored transparent overlay (not textured).
void NetInterface::drawPotential(Net *net, int netface)
{ ***
	DBG cerr <<"--drawing potential "<<netface<<endl;
	if (!net || netface<0 || netface>=net->faces.n) return;

	glPushMatrix();
	glMultMatrixf(hedron->m);

	int face=net->info; //the seed face for the net
	Basis bas=poly->basisOfFace(face);

	double r,g,b,a;
	r=1;
	b=1;
	g=1;
	a=.5;

	 //enable transparent
	glEnable (GL_BLEND); glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
	glColor4f(r,g,b,a);

	glBegin(GL_TRIANGLE_FAN);
	flatpoint fp;
	for (int c=0; c<net->faces.e[netface]->edges.n; c++) {
		fp+=net->faces.e[netface]->edges.e[c]->points->p();
	}
	fp/=net->faces.e[netface]->edges.n;
	fp=transform_point(net->faces.e[netface]->matrix,fp);
	spacepoint center=bas.p + fp.x*bas.x + fp.y*bas.y;
	spacepoint normal=center/norm(center);
	spacepoint p;
	//flatpoint v;

	 //center of fan
	glNormal3f(normal.x,normal.y,normal.z);
	glVertex3f(center.x,center.y,center.z);

	 //rest of fan
	for (int c2=0; c2<=net->faces.e[netface]->edges.n; c2++) {
		fp=transform_point(net->faces.e[netface]->matrix,
						   net->faces.e[netface]->edges.e[c2%net->faces.e[netface]->edges.n]->points->p());
		p=bas.p + fp.x*bas.x + fp.y*bas.y;

		normal=p/norm(p);
		glNormal3f(normal.x,normal.y,normal.z);
		glVertex3f(p.x,p.y,p.z);
	}

	glEnd();

	glDisable (GL_BLEND);

	glPopMatrix();
}


int NetInterface::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{ ***
	int group = NETI_None;
	Overlay *overlay = scanOverlays(x,y, nullptr,nullptr,&group);
	int index = (overlay?overlay->id:-1);

	if (group == NETI_None) {
		int c = findCurrentPotential();
		if (c >= 0) group = NETI_Potential;
	}

	if (group == NETI_None && draw_papers) {
		int c = scanPaper(x,y, index);
		DBG cerr <<"scan paper lbd: "<<c<<endl;
		if (c != PAPER_None) {
			group = NETI_Paper;
			mouseover_index=c;
		}
	}

	mbdown = x;
	if (active_action==ACTION_Unwrap) rbdown=currentface;
	if (active_action==ACTION_Reseed) rbdown=currentface;

	buttondown.down(mouse->id,LEFTBUTTON,x,y, group,index);

	return 0;
}

int NetInterface::LBUp(int x,int y,unsigned int state,const LaxMouse *mouse)
{ ***
	int overlayid=-1;
	int group=NETI_None, orig_group=NETI_None;
	//int dragged=
	buttondown.up(mouse->id,LEFTBUTTON, &orig_group,&overlayid);

	Overlay *overlay=scanOverlays(x,y, nullptr,nullptr,&group);
	if ((orig_group==NETI_TouchHelpers || orig_group==NETI_Papers) && overlayid>0) {
		if (overlay && overlay->id==overlayid) {
			if (group==NETI_TouchHelpers) {
				 //changing active_action
				if (active_action==overlay->action) {
					active_action=ACTION_None;
				} else active_action=overlay->action;
				needtodraw=1;
				return 0;

			} else if (group==NETI_Papers) {
				 //paper management buttons
				if (overlay->action==ACTION_AddPaper) {
					papers.push(new PaperBound(default_paper),1);
					remapPaperOverlays();
					needtodraw=1;
					return 0;
				}

				if (overlay->action==ACTION_Paper && overlay->index>=0) {
					if (currentnet) {
						currentnet->whichpaper=overlay->index;
						needtodraw=1;
						return 0;
					}
				} else if (overlay->action==ACTION_Paper && overlay->index<0) {
					draw_papers=0; //turn off showing of papers
					needtodraw=1;
					return 0;
				}

			} else if (group==NETI_ImageStack) {
				 //image stack management
				// ***
			}
		}
	}

	if (active_action == ACTION_Unwrap) RBUp(x,y,0,mouse);
	else if (active_action == ACTION_Reseed) RBUp(x,y,ControlMask,mouse);
	return 0;
}

int NetInterface::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *mouse)
{ ***
	rbdown=currentface;
	//currentface=-1;
	buttondown.down(mouse->id,RIGHTBUTTON,x,y,currentface);
	return 0;
}

int NetInterface::RBUp(int x,int y,unsigned int state,const LaxMouse *mouse)
{ ***
	DBG cerr <<"NetInterface::RBUp: rbdown=="<<rbdown<<"  currentface=="<<currentface<<endl;

	if (!(buttondown.isdown(mouse->id,RIGHTBUTTON))) return 0;

	int rbdown;
	buttondown.up(mouse->id,RIGHTBUTTON, &rbdown);
	//if (active_action!=ACTION_Unwrap && active_action!=ACTION_Reseed) return 0;

	if ((state&LAX_STATE_MASK)==ControlMask) {
		 //reseed net of currentface, if any
		if (rbdown==-1 || rbdown!=currentface) return 0;
		Reseed(currentface);
		remapCache();
		needtodraw=1;
		return 0;
	}


	if (currentpotential>=0) {
		 //right click on a potential drops it down
		int orig=currentnet->faces.e[currentpotential]->original;
		poly->faces.e[orig]->cache->facemode=currentnet->object_id;
		currentnet->Drop(currentpotential);

		//remapCache();
		remapCache(orig,orig);
		needtodraw=1;
		rbdown=currentface=currentpotential=-1;
		return 0;

	} else if (rbdown!=-1 && rbdown==currentface) {
		 //right click down and up on same face...
		DBG cerr <<"...right click down and up on same face"<<endl;
		 
		 //if face is on hedron, not a seed, then create a new net with that face
		if (poly->faces.e[currentface]->cache->facemode==0) {
			DBG cerr <<"...establish a net on currentface"<<endl;

			 //unwrap as a seed when face is on polyhedron, not in net
			Net *net=establishNet(currentface);
			if (currentnet) {
				if (currentnet->numActual()==1) removeNet(currentnet);
				if (currentnet) currentnet->dec_count();
			}

			currentnet=net;//using count from above
			//remapCache(); //no remapping necessary since face stays on hedron
			needtodraw=1;
			rbdown=currentface=-1;
			return 0;

		} else if (poly->faces.e[currentface]->cache->facemode!=0) {
			DBG cerr <<"...make net of currentface the current net"<<endl;

			 //make current net the net of currentface
			Net *net=findNet(poly->faces.e[currentface]->cache->facemode);
			if (currentnet!=net) {
				if (currentnet && currentnet->numActual()==1) removeNet(currentnet);
				if (currentnet) currentnet->dec_count();
				currentnet=net;
				if (currentnet) currentnet->inc_count();
				needtodraw=1;
			}

			if (currentfacestatus==1) {
				 //face is a leaf. pick it up
				int i=-1;
				net->findOriginalFace(currentface,1,0,&i); //it is assumed the face is actually there
				net->PickUp(i,-1);
				poly->faces.e[currentface]->cache->facemode=0;
				remapCache(currentface,currentface);
				needtodraw=1;
			}
			rbdown=currentface=-1;
			return 0;

		}

	} else if (rbdown==-1 && currentface==-1) {
		if (currentnet) {
			 //If the net only has the seed face, and we are selecting off, then remove that net
			if (currentnet->numActual()==1) removeNet(currentnet);
			if (currentnet) { currentnet->dec_count(); currentnet=nullptr; }
			needtodraw=1;
		}
	}
	rbdown=currentface=-1;
	return 0;
}

int NetInterface::MouseMove(int x,int y,unsigned int state,const LaxMouse *mouse)
{ ***
	// first off, check if mouse in any overlays
	int action = 0, index = -1, group = NETI_None;
	
	 //map pointer to 3 space
	double d=win_h/2/tan(fovy/2);
	double sx,sy,sz;
	
	//map mouse point to a point in space 50 units out
	sz=50;
	sx=(x-win_w/2.)*sz/d;
	sy=(y-win_h/2.)*sz/d;

	tracker=cameras.e[current_camera]->m.p
		+ sx*cameras.e[current_camera]->m.x
		- sy*cameras.e[current_camera]->m.y
		- sz*cameras.e[current_camera]->m.z;

	pointer.p=cameras.e[current_camera]->m.p;
	pointer.v=tracker-pointer.p;
	

	// no buttons pressed
	if (!buttondown.any()) {
		//search for mouse overs

		int c=-1;
		int index=-1;
		if (currentnet) {
			if (draw_papers) c=scanPaper(x,y, index);
			if (c>=0 && c!=PAPER_None) {
				mouseover_paper=currentnet->whichpaper;
				mouseover_group=NETI_Paper;
				mouseover_index=c;
				DBG cerr <<"   ==== scanned as over paper:"<<c<<endl;
				return 0;
			} else mouseover_paper=-1;
			 
			 //scan for mouse over potential faces
			c=findCurrentPotential(); //-1 for none, -2 for over paper
			DBG cerr <<"MouseMove found potential: "<<c<<endl;
			if (c>=0) {
				if (currentface!=-1) { 	currentface=-1; needtodraw=1; }
				if (c!=currentpotential) {
					currentpotential=c;
					needtodraw=1;
				}
				DBG cerr <<"current potential original: "<<currentpotential<<endl;

				return 0;

			} else if (c==-2) {
				 //mouse is over paper
				mouseover_paper=currentnet->whichpaper;
				currentpotential=-1;

			} else {
				if (currentpotential!=-1) {
					currentpotential=-1;
					needtodraw=1;
				}
			}

		} else {
			mouseover_paper=-1;
		}

		 //scan for mouse over actual faces
		c=findCurrentFace();

		DBG cerr<<"MouseMove findCurrentFace original: "<<c;
		DBG if (c>=0) {
		DBG		cerr <<" facemode:"<<poly->faces.e[c]->cache->facemode<<endl;
		DBG 	Net *net=findNet(poly->faces.e[c]->cache->facemode);
		DBG 	int i=-1;
		DBG 	if (net) {
		DBG			cerr <<"net found "<<nets.findindex(net)<<"..."<<endl;
		DBG			int status=net->findOriginalFace(c,1,0,&i); //it is assumed the face is actually there
		DBG			cerr <<" links: status:"<<status<<" on neti:"<<i<<"  links:"<<net->actualLink(i,-1)<<"   ";
		DBG		} else cerr <<"(not in a net) ";
		DBG		cerr <<"net face: "<<i<<endl;
		DBG } else cerr <<endl;

		if (c!=currentface) {
			needtodraw=1;
			currentface=c;
			if (currentface>=0) {
				currentfacestatus=0;
				//DBG cerr <<"face "<<currentface<<" facemode "<<poly->faces.e[currentface]->cache->facemode<<endl;
				if (poly->faces.e[currentface]->cache->facemode>0) {
					 //face is in a net. If it is a leaf in currentnet, color it green, rather then red
					Net *net=findNet(poly->faces.e[currentface]->cache->facemode);
					if (net && net==currentnet) {
						int i=-1;
						net->findOriginalFace(currentface,1,0,&i); //it is assumed the face is actually there

						DBG cerr <<"find original neti:"<<i<<" actual links:"<<net->actualLink(i,-1)<<endl;

						if (net->actualLink(i,-1)==1) {
							 //only touches 1 actual face
							currentfacestatus=1;
						}
					}
				} else if (poly->faces.e[currentface]->cache->facemode<0) currentfacestatus=2; //for seeds
			}
		}
		return 0;
	}


	// below is for when button is down

	int lx,ly;
	int bgroup=NETI_None,bindex=-1;
	buttondown.move(mouse->id, x,y, &lx,&ly);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &bgroup,&bindex);
	flatpoint leftb=flatpoint(lx,ly);


	// deal with 2 finger zoom first
	if (buttondown.isdown(0,LEFTBUTTON)==2 && buttondown.isdown(mouse->id,LEFTBUTTON)) {
		 //double move
		int xp1,yp1, xp2,yp2;
		int xc1,yc1, xc2,yc2;
		int device1=buttondown.whichdown(0,LEFTBUTTON);
		int device2=buttondown.whichdown(device1,LEFTBUTTON);
		buttondown.getinfo(device1,LEFTBUTTON, nullptr,nullptr, &xp1,&yp1, &xc1,&yc1);
		buttondown.getinfo(device2,LEFTBUTTON, nullptr,nullptr, &xp2,&yp2, &xc2,&yc2);

		double oldd=norm(flatpoint(xp1,yp1)-flatpoint(xp2,yp2));
		double newd=norm(flatpoint(xc1,yc1)-flatpoint(xc2,yc2));
		double zoom=newd/oldd;
		if (zoom==0) return 0;

		 //apply zoom
		double amount=10;
		//DBG cerr <<" ZZZZZZOOM d1:"<<device1<<" d2:"<<device2<<"    "<<amount<<"  z:"<<zoom<<endl;
		if (zoom>1) amount=-amount*(zoom-1);
		else amount=amount*(1/zoom-1);

		cameras.e[current_camera]->m.p+=amount*cameras.e[current_camera]->m.z;
		cameras.e[current_camera]->transformTo();

		 //apply rotation
//		flatvector v1=flatpoint(xc1,yc1)-flatpoint(xp1,yp1);
//		flatvector v2=flatpoint(xc2,yc2)-flatpoint(xp2,yp2);
//		double angle=0;
//		double epsilon=1e-10;
//		DBG cerr <<"   NNNNNORM  a:"<<norm(v1)<<"  b:"<<norm(v2)<<endl;
//		if (norm(v1)>epsilon) {
//			 //point 1 moved
//			v1=flatpoint(xc1,yc1)-flatpoint(xp2,yp2);
//			v2=flatpoint(xp1,yp1)-flatpoint(xp2,yp2);
//			angle=atan2(v2.y,v2.x)-atan2(v1.y,v1.x);
//		} else if (norm(v2)>epsilon) {
//			 //point 2 moved
//			v1=flatpoint(xc2,yc2)-flatpoint(xp1,yp1);
//			v2=flatpoint(xp2,yp2)-flatpoint(xp1,yp1);
//			angle=atan2(v2.y,v2.x)-atan2(v1.y,v1.x);
//		}
//		DBG  cerr <<" RRRRROTATE "<<angle<< "  deg:"<<angle*180/M_PI<<endl;
//
//		if (angle) {
//			spacepoint axis;
//			axis=cameras.e[current_camera]->m.z;
//			things.e[curobj]->RotateGlobal(angle*180/M_PI, axis.x,axis.y,axis.z);
//		}

		needtodraw=1;
		return 0;
	}



	if (!buttondown.isdown(mouse->id,LEFTBUTTON) && !buttondown.isdown(mouse->id,RIGHTBUTTON) && !buttondown.isdown(mouse->id,RIGHTBUTTON))
		return 0;

	if (bgroup==NETI_Paper && currentnet) {
		flatpoint fp,fpn;
		fpn=pointInNetPlane(x,y);
		fp =pointInNetPlane(lx,ly);

		PaperBound *paper=papers.e[bindex];
		if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
			 //shift papers
			if ((state&LAX_STATE_MASK)==ShiftMask) {
				 //move net relative to papers
				currentnet->origin(currentnet->origin()-fpn+fp);
			} else {
				 //move paper relative to other papers
				flatpoint d=transform_point(currentnet->m(),fpn)-transform_point(currentnet->m(),fp);
				paper->matrix.Translate(d);
			}

		} else if ((state&LAX_STATE_MASK)==ControlMask) {
			 //scale papers
			double s=1;
			if (x>lx) s=.98;
			else s=1.02;
			for (int c=0; c<6; c++) {
				currentnet->m(c, currentnet->m(c)*s);
			}

		} else {
			 //rotate papers
			double s=0;
			if (x>lx) s=1./180*M_PI;
			else s=-1./180*M_PI;
			currentnet->xaxis(rotate(currentnet->xaxis(),s));
			currentnet->yaxis(rotate(currentnet->yaxis(),s));
		}

		needtodraw=1;
		return 0;
	}

	ActionType current_action = active_action;
	if (buttondown.isdown(0,RIGHTBUTTON)) current_action = ACTION_Unwrap;

	if (current_action==ACTION_Zoom) {
		 //zoom
		if (x-mbdown>10) { WheelUp(x,y,0,0,mouse); mbdown=x; }
		else if (mbdown-x>10) { WheelDown(x,y,0,0,mouse); mbdown=x; }

	}




	 //right button
	if (current_action == ACTION_Unwrap && rbdown >= 0) {
		 //Unwrap
		int c=findCurrentFace();
		DBG cerr <<"rb move: "<<c<<endl;
		if (c==rbdown) { currentface=rbdown; return 0; }

		 //if c is attached to rbdown then we have a winner maybe
		int cc;

		//DBG cerr <<"faces attached to "<<rbdown<<": ";
		for (cc=0; cc<poly->faces.e[rbdown]->pn; cc++) {
			//DBG cerr<<poly->faces.e[rbdown]->f[cc]<<" ";

			if (poly->faces.e[rbdown]->f[cc]==c) break;
		}
		//DBG cerr <<endl;
		if (cc==poly->faces.e[rbdown]->pn) { 
			 //c is not attached to any edge of rbdown
			if (currentface!=-1) needtodraw=1;
			currentface=-1;
		} else {
			if (currentface!=c) needtodraw=1;
			currentface=c;
		}

		DBG cerr<<" rb-unwrap from "<<rbdown<<" to: "<<currentface<<endl;
		if (rbdown!=currentface && currentface>=0 && unwrapTo(rbdown,currentface)==0) {
			rbdown=currentface;
			currentface=-1;
			needtodraw=1;
		}
		DBG cerr<<" ..rb-unwrap done"<<endl;

		return 0;
	}

	if (current_action == ACTION_None) {
		if       ((state & LAX_STATE_MASK) == 0)                      current_action = ACTION_Roll;
		else if  ((state & LAX_STATE_MASK) == ShiftMask)              current_action = ACTION_Rotate;
		else if  ((state & LAX_STATE_MASK) == ControlMask)            current_action = ACTION_Shift_Texture;
		else if  ((state & LAX_STATE_MASK) ==(ShiftMask|ControlMask)) current_action = ACTION_Rotate_Texture;
	}

	currentface = -1;

	 //shift-drag -- rotates camera around axis through viewer
	if (current_action == ACTION_Rotate) {
		flatpoint d ,p = flatpoint(x,y);
		d = p-leftb;

		//-----------rotate camera
		//			if (d.y) {
		//				rotate(cameras.e[current_camera]->m,    //basis
		//					   cameras.e[current_camera]->m.x,  //axis
		//					   d.y/5/180*M_PI); //angle
		//				cameras.e[current_camera]->transformTo();
		//				needtodraw=1;
		//			}
		//			if (d.x) {
		//				rotate(cameras.e[current_camera]->m,    //basis
		//					   cameras.e[current_camera]->m.y,  //axis
		//					   d.x/5/180*M_PI); //angle
		//				cameras.e[current_camera]->transformTo();
		//				needtodraw=1;
		//			}
		if (d.x) { //rotate thing around camera z
			spacepoint axis;
			axis=d.x*cameras.e[current_camera]->m.z;
			things.e[curobj]->RotateGlobal(norm(d)/5, axis.x,axis.y,axis.z);
			needtodraw=1;
		}
		leftb=p;

		//plain drag -- rolls shape
	} else if (current_action==ACTION_Roll) { //rotate current thing
		flatpoint d,p=flatpoint(x,y);
		d=p-leftb;

		spacepoint axis;
		axis=d.y*cameras.e[current_camera]->m.x + d.x*cameras.e[current_camera]->m.y;
		needtodraw=1;

		//-------------------------
		things.e[curobj]->RotateGlobal(norm(d)/5, axis.x,axis.y,axis.z);
		//things.e[curobj]->RotateLocal(norm(d)/5, axis.x,axis.y,axis.z);

		//-------------------------
		//			if (d.y) {
		//				things.e[curobj]->RotateLocal(d.y/5, 0,0,1);
		//				needtodraw=1;
		//			}
		//			if (d.x) {
		//				things.e[curobj]->RotateLocal(d.x/5, 0,1,0);
		//				needtodraw=1;
		//			}
		//-------------------------
		leftb=p;


		//shift-control-drag -- rotates texture around z
	} else if (current_action==ACTION_Rotate_Texture) { //rotate texture around z
		flatpoint d,p=flatpoint(x,y);
		d=p-leftb;

		if (d.x) {
			spacepoint axis;
			axis=d.x*cameras.e[current_camera]->m.z;
			things.e[curobj]->updateBasis();
			transform(axis,things.e[curobj]->bas);
			rotate(extra_basis,axis,norm(d)/5/180*M_PI);
			needtodraw=1;
			leftb=p;
			mapPolyhedronTexture(hedron);
		}

		// control-drag -- shift texture
	} else if (current_action==ACTION_Shift_Texture) { //rotate texture
		flatpoint d,p=flatpoint(x,y);
		d=p-leftb;

		spacepoint axis;
		axis=d.y*cameras.e[current_camera]->m.x + d.x*cameras.e[current_camera]->m.y;
		things.e[curobj]->updateBasis();
		transform(axis,things.e[curobj]->bas);
		rotate(extra_basis,axis,norm(d)/5/180*M_PI);

		mapPolyhedronTexture(hedron);
		needtodraw=1;
		leftb=p;
	}

	return 0;
}

/*! A new net will be returned. it's count will have to be dec_count()'d.
 */
Net *NetInterface::establishNet(int original)
{ ***
	DBGL("create net around original "<<original);

	Net *net = new Net;
	net->_config = 1;
	net->Basenet(poly);
	net->Unwrap(-1, original);
	poly->faces.e[original]->cache->facemode = -net->object_id;
	net->info = original;
	nets.push(net);

	return net;
}

//! Reseed the net containing original face, an index in poly->faces.
/*! Reseeding means to orient the net to appear stuck to the hedron at face original.
 * The origin of the net will be the origin of the seed face. The matrices of all the
 * faces will be changed accordingly.
 *
 * Return 0 for reseeded, or no reseeding necessary. Nonzero for error.
 *
 * NOTE: remapCache() is NOT called here.
 */
int NetInterface::Reseed(int original)
{ ***
	if (original < 0 || original >= poly->faces.n) return 1;

	if (poly->faces.e[original]->cache->facemode == 0) return 2;  // not in a net
	if (poly->faces.e[original]->cache->facemode < 0)  return 0;  // is already a seed

	// reseed net 
	Net *net = findNet(poly->faces.e[original]->cache->facemode);

	// update poly<->net crosslinks
	poly->faces.e[net->info]->cache->facemode=net->object_id;
	poly->faces.e[original]->cache->facemode=-net->object_id;
	net->info=original;

	double m[6],m2[6];
	int neti;
	net->findOriginalFace(original,1,0,&neti);
	DBG cerr <<"--reseed: original:"<<currentface<<" to neti:"<<neti<<endl;

	if (neti<0) {
		DBGW("************WARNING!!!!! negative neti, this should NEVER happen!!");
		return 0;
	}

	transform_invert(m,net->faces.e[neti]->matrix);
	for (int c = 0; c < net->faces.n; c++) {
		//transform_mult(m2,m,net->faces.e[c]->matrix);
		transform_mult(m2,net->faces.e[c]->matrix,m);
		transform_copy(net->faces.e[c]->matrix,m2);
	}

	// adjust paper transform, if any paper
	if (net->whichpaper >= -1) {
		double m3[6];
		transform_invert(m3,net->m());
		transform_mult(m2,m3,m);
		transform_invert(m3,m2);
		net->m(m3);
	}

	needtodraw = 1;

	return 0;
}

//! Return 0 for net removed, or nonzero for not removed.
int NetInterface::removeNet(Net *net)
{ ***
	return removeNet(nets.findindex(net));
}

//! Remove net and set all faces to be on hedron (facemode==0).
int NetInterface::removeNet(int netindex)
{ ***
	if (netindex<0 || netindex>=nets.n) return 1;
	Net *net=nets.e[netindex];
	if (currentnet==net) {
		currentnet->dec_count();
		currentnet=nullptr;
	}
	 //reset all faces in net to normal
	DBG int n=0;
	for (int c=0; c<net->faces.n; c++) {
		if (net->faces.e[c]->tag!=FACE_Actual) continue;
		DBG n++;
		poly->faces.e[net->faces.e[c]->original]->cache->facemode=0;
	}
	DBG cerr <<"removeNet() reset "<<n<<" faces"<<endl;
	nets.remove(netindex);
	return 0;
}

//! Change paper index to a different type of paper.
/*! towhich==-2 for previous paper, -1 for next, or >=0 for absolute paper num
 *
 * Return 1 for successful change. 0 for no change.
 */
int NetInterface::changePaper(int towhich,int index)
{ ***
	//default does nothing
	return 0;
}

//! Find the point in currentnet coordinates laying on screen point x,y.
flatpoint NetInterface::pointInNetPlane(int x,int y)
{ ***
	if (!currentnet) return flatpoint();

	spacevector spacev;
	spaceline line;
	spacepoint p;
	int err = 0;
	
	// map mouse point to a point in space 50 units out
	double d  = win_h / 2 / tan(fovy / 2);
	double sz = 50;
	double sx = (x - win_w / 2.) * sz / d;
	double sy = (y - win_h / 2.) * sz / d;
	hedron->updateBasis();

	spacev = cameras.e[current_camera]->m.p
		+ sx*cameras.e[current_camera]->m.x
		- sy*cameras.e[current_camera]->m.y
		- sz*cameras.e[current_camera]->m.z;

	line.p = cameras.e[current_camera]->m.p;
	line.v = spacev - line.p;

	flatpoint fp;
	transform(line, hedron->bas);
	Basis nbasis = poly->basisOfFace(currentnet->info);  // basis of seed face on polyhedron

	p = intersection(line, Plane(nbasis.p, nbasis.z), err);
	if (err != 0) return flatpoint();
	fp = flatten(p, nbasis);

	return fp;
}

int NetInterface::scanPaper(int x,int y, int &index)
{ ***
	if (!currentnet || !papers.n) return PAPER_None;

	flatpoint fp=pointInNetPlane(x,y);
	flatpoint fpp;
	DBG cerr <<" ---- scanPaper from pointInNetPlane: netface:"<<currentnet->info<<"  "<<fp.x<<","<<fp.y<<endl;

	double pw,ph;
	for (index=0; index<papers.n; index++) {
		pw=papers.e[index]->width;
		ph=papers.e[index]->height;

		fpp=transform_point(currentnet->m(),fp);
		fpp=papers.e[index]->matrix.transformPointInverse(fpp);

		DBG cerr <<" ---- scanPaper "<<index<<" from pointInNetPlane: transf to paper: "<<fpp.x<<","<<fpp.y<<endl;
		DBG const double *nm=currentnet->m();
		DBG cerr <<" ----   netm: "<<nm[0]<<", "<<nm[1]<<", "<<nm[2]<<", "<<nm[3]<<", "<<nm[4]<<", "<<nm[5]<<", "<<endl;
		DBG nm=papers.e[index]->matrix.m();
		DBG cerr <<" ---- paperm: "<<nm[0]<<", "<<nm[1]<<", "<<nm[2]<<", "<<nm[3]<<", "<<nm[4]<<", "<<nm[5]<<", "<<endl;

		if (fpp.x > 0 && fpp.x < pw * .1 && fpp.y >= 0 && fpp.y < ph)
			return PAPER_Left;

		if (fpp.x > pw + -pw * .1 && fpp.x < pw && fpp.y >= 0 && fpp.y < ph)
			return PAPER_Right;

		if (fpp.y > 0 && fpp.y < ph * .1 && fpp.x >= 0 && fpp.x < pw)
			return PAPER_Bottom;

		if (fpp.y > ph + -ph * .1 && fpp.y < ph && fpp.x >= 0 && fpp.x < pw)
			return PAPER_Top;

		if (fpp.x >= 0 && fpp.y >= 0 && fpp.x < pw && fpp.y < ph)
			return PAPER_Inside;
	}

	return PAPER_None;
}

/*! Return index in currentnet->faces, or -2 over paper (if any) or -1 for not over anything.
 */
int NetInterface::findCurrentPotential()
{ ***
	if (!currentnet) return -1;

	spacepoint p;
	flatpoint fp;
	int err;
	int index = -1;

	spaceline line;
	hedron->updateBasis();
	line = pointer;
	transform(line,hedron->bas);
	Basis nbasis = poly->basisOfFace(currentnet->info); //basis of seed face on polyhedron

	p=intersection(line,Plane(nbasis.p,nbasis.z),err);
	if (err != 0) return -1;
	fp=flatten(p,nbasis);

	 //intersect with the net face's shape
	index = currentnet->pointinface(fp, 1);

	if (index < 0) {
		//not inside any potential face!
		return -1;
	}

	if (currentnet->faces.e[index]->tag!=FACE_Potential) return -1;
	if (poly->faces.e[currentnet->faces.e[index]->original]->cache->facemode!=0) return -1;

	return index;
}

//! Assuming the line pointer is set properly, find the closest face it intersects.
/*! Returns the face index or -1 if not found.
 */
int NetInterface::findCurrentFace()
{ ***
	 //using cached basis in each face, intersect pointer with that plane,
	 //then see if that face contains that point
	spacepoint p;
	flatpoint fp;
	int err;
	int index=-1;
	double dist=10000000,d;

	spaceline line;
	hedron->updateBasis();
	line=pointer;
	transform(line,hedron->bas);

	for (int c=0; c<poly->faces.n; c++) {
		 //intersect with the face's cached axis, against its cached 2-d points
		p=intersection(line,Plane(poly->faces.e[c]->cache->axis.p,poly->faces.e[c]->cache->axis.z),err);
		fp=flatten(p, poly->faces.e[c]->cache->axis);
		if (err!=0) continue;
		if (point_is_in(fp, poly->faces.e[c]->cache->points2d,poly->faces.e[c]->pn)) {
			d=(p-line.p)*(p-line.p);
			//DBG cerr <<"point in "<<c<<", dist: "<<d<<endl;
			if (d<dist) { index=c; dist=d; }
		}
	}
	//DBG cerr <<dist<<"  ";
	return index;
}

//! Find the net with object_id==id.
Net *NetInterface::findNet(int id)
{ ***
	if (id<0) id=-id;
	for (int c=0; c<nets.n; c++) if ((int)nets.e[c]->object_id==id) return nets.e[c];
	return nullptr;
}

/*! If the faces are not adjacent, then return 1, else return 0 for successful unwrap.
 */
int NetInterface::recurseUnwrap(Net *netf, int fromneti, Net *nett, int toneti)
{ ***
	 //recursively traverse:
	 //  add face to nett by dropping at appropriate nett face and edge.
	 //  set tick corresponding to fromneti in netf to 1

	if (netf->faces.e[fromneti]->tick==1) return 0;

	 //mark this face as handled
	netf->faces.e[fromneti]->tick=1;

	int c=-1;
	 //find the edge in nett->toneti that touches fromneti. Since the from face is not in
	 //nett yet, the edge tag will be FACE_Potential
	for (c=0; c<nett->faces.e[toneti]->edges.n; c++) {
		//if (nett->faces.e[toneti]->edges.e[c]->tag!=
		if (netf->faces.e[fromneti]->original==nett->faces.e[toneti]->edges.e[c]->tooriginal) break;
	}
	if (c==nett->faces.e[toneti]->edges.n) {
		DBG cerr <<"Warning: recurseUnwrap() from and to are not adjacent"<<endl;
		return 1; //faces not adjacent
	}
	DBG cerr <<"recurseUnwrap: fromneti="<<fromneti<<" (orig="<<netf->faces.e[fromneti]->original
	DBG 	<<")  toneti="<<toneti<<" (orig="<<nett->faces.e[toneti]->original<<")"<<endl;

	nett->Unwrap(toneti,c); //attach face fromneti in netf
	DBG cerr <<"nett->toneti->toface:"<<nett->faces.e[toneti]->edges.e[c]->toface<<endl;
	DBG cerr <<"nett->toneti->tag:"<<nett->faces.e[toneti]->edges.e[c]->tag<<endl;
	int i=nett->findOriginalFace(netf->faces.e[fromneti]->original,1,0, &toneti);
	DBG cerr <<"recurseUnwrap newly laid face: find orig "<<netf->faces.e[fromneti]->original<<":neti="<<toneti<<", status="<<i<<endl;

	//-------------------------
	DBG for (int c=0; c<nett->faces.n; c++) {
	DBG 	if (nett->faces.e[c]->tag!=FACE_Actual) continue;
	DBG 	for (int c2=0; c2<nett->faces.e[c]->edges.n; c2++) {
	DBG 		if (nett->faces.e[c]->edges.e[c2]->tag==FACE_None) {
	DBG 			cerr <<"recurse:AAAAAAARRG! edge points to -1:"<<endl;
	DBG 			cerr <<"  net->face("<<c<<")->edge("<<c2<<")->tag="<<nett->faces.e[c]->edges.e[c2]->tag<<endl;
	DBG 		}
	DBG 	}
	DBG }
	//-------------------------



	if (toneti<=0) {
		cerr <<"****AARG! cannot find original "<<netf->faces.e[fromneti]->original<<", this should not happen, fix me!!"<<endl;
		return 0;
	}
	poly->faces.e[nett->faces.e[toneti]->original]->cache->facemode=nett->object_id;
	// now fromneti and toneti are the same face..


	for (c=0; c<netf->faces.e[fromneti]->edges.n; c++) {
		i=netf->faces.e[fromneti]->edges.e[c]->toface;
		if (i<0 || netf->faces.e[i]->tick==1 || netf->faces.e[i]->tag!=FACE_Actual) continue;

		 //now i is a face in netf at edge c of face fromneti
		recurseUnwrap(netf,i, nett,toneti);
	}

	return 0;
}

//! Mouse dragged: from -> to. Either unwrap face or net, or split a net.
/*! If both faces are seeds, then combine the nets. If one face is a seed,
 * and the other an adjacent net face, then that face becomes a seed of
 * a new net. All other cases ignored.
 *
 * from and to are both original face numbers.
 */
int NetInterface::unwrapTo(int from,int to)
{ ***
	if (from<0 || to<0) return 1;
	int reseed=-1;	

	int modef=poly->faces.e[from]->cache->facemode;
	int modet=poly->faces.e[ to ]->cache->facemode;

	 //we are mostly acting on seed to seed, so we need nets to begin with:
	if (modef==0) {
		 //from was an ordinary face, make from into a net
		Net *net=establishNet(from);
		modef=poly->faces.e[from]->cache->facemode=-net->object_id;
		net->dec_count();
	}
	if (modet==0) {
		 //to was an ordinary face, make to into a net
		Net *net=establishNet(to);
		modet=poly->faces.e[to]->cache->facemode=-net->object_id;
		net->dec_count();
	}

	//now modet and modef will not be 0

	if (modef<0 && modet<0) {
		 //both seeds, so combine
		Net *netf,*nett;
		netf=findNet(modef);
		nett=findNet(modet);

		if (netf->faces.n>nett->faces.n) {
			 //drop the smaller net onto the larger net to save time
			Net *nf=netf;
			netf=nett;
			nett=nf;

			reseed=modef;
			modef=modet;
			modet=reseed;

			reseed=from;
			from=to;
			to=reseed;

			reseed=netf->info;
		}

		int fromi,toi;
		DBG int status =
		netf->findOriginalFace(from,1,0,&fromi);
		DBG cerr <<"from: find orig "<<from<<":"<<fromi<<", status="<<status<<endl;

		DBG status = 
		nett->findOriginalFace(to,1,0,&toi);
		DBG cerr <<"  to: find orig "<<to<<":"<<toi<<", status="<<status<<endl;

		netf->resetTick(0);
		recurseUnwrap(netf,fromi, nett,toi);

		DBG //------------------------
		DBG for (int c=0; c<nett->faces.n; c++) {
		DBG 	if (nett->faces.e[c]->tag!=FACE_Actual) continue;
		DBG 	for (int c2=0; c2<nett->faces.e[c]->edges.n; c2++) {
		DBG 		if (nett->faces.e[c]->edges.e[c2]->tag==FACE_None) {
		DBG 			cerr <<"AAAAAAARRG! edge points to null face:"<<endl;
		DBG 			cerr <<"  net->face("<<c<<")->edge("<<c2<<")"<<endl;
		DBG 			exit(1);
		DBG 		}
		DBG 	}
		DBG }
		DBG //------------------------

		removeNet(netf);

		 //make sure all faces in nett now have proper facemode
		for (int c=0; c<nett->faces.n; c++) {
			if (nett->faces.e[c]->tag!=FACE_Actual) continue;
			if (nett->faces.e[c]->original==to) //the seed face
				poly->faces.e[nett->faces.e[c]->original]->cache->facemode=-nett->object_id;
			else poly->faces.e[nett->faces.e[c]->original]->cache->facemode=nett->object_id;
		}
		if (reseed>=0) Reseed(reseed);
		remapCache();
		needtodraw=1;
		return 0;
		
	} else if (modef<0 && modet>0 && modet==-modef) {
		 //from a seed to a net face within the same net

		 //split net along edge between from and to
		cerr <<" *** must implement split nets!"<<endl;
	}
	return 1;
}

/*! Return 1 for in an overlay
 */
Overlay *NetInterface::scanOverlays(int x,int y, int *action,int *index,int *group)
{ ***
	if (draw_overlays && overlays.n) {
		int c=0;
		for (c=0; c<overlays.n; c++) if (overlays.e[c]->PointIn(x,y)) break;
		if (c!=overlays.n) {
			 //mouse is in an overlay
			if (action) *action=overlays.e[c]->action;
			if (index) *index=overlays.e[c]->index;
			if (group) *group=NETI_TouchHelpers;
			return overlays.e[c]; //mouse over overlay, do nothing else
		}
	}

	if (draw_overlays && paperoverlays.n) {
		int c=0;
		for (c=0; c<paperoverlays.n; c++) if (paperoverlays.e[c]->PointIn(x,y)) break;
		if (c!=paperoverlays.n) {
			 //mouse is in an overlay
			if (action) *action=paperoverlays.e[c]->action;
			if (index) *index=paperoverlays.e[c]->index;
			if (group) *group=NETI_Papers;
			return paperoverlays.e[c]; //mouse over overlay, do nothing else
		}
	}
	if (action) *action=0;
	if (index)  *index=-1;
	if (group)  *group=NETI_None;

	return nullptr;
}

/*! Adds a duplicate.
 */
int NetInterface::AddNet(Net *net)
{ ***
	Net *newnet=net->duplicate();
	nets.push(newnet);
	newnet->dec_count();
	needtodraw=1;
	return 1;
}

/*! Copies paper. Return 0 for success, 1 for error.
 */
int NetInterface::AddPaper(PaperBound *paper)
{ ***
	if (!paper) return 1;
	papers.push(new PaperBound(*paper),1);
	remapPaperOverlays();
	needtodraw=1;
	return 1;
}

//! If non-null, only SETS files, does no loading of any kind.
int NetInterface::SetFiles(const char *hedron, const char *image, const char *project)
{ ***
	int n=0;
	if (hedron)  { n++; makestr(polyhedronfile,hedron); }
	if (image)   { n++; makestr(spherefile,image); }
	if (project) { n++; makestr(polyptychfile,project); }
	return n;
}

/*! Uses ph, does not duplicate.
 */
int NetInterface::InstallPolyhedron(Polyhedron *ph)
{ ***
	if (!ph) return 1;
	if (poly) poly->dec_count();
	poly=ph;
	ph->inc_count();

	poly->makeedges();
	poly->BuildExtra();
	
	nets.flush();
	if (currentnet) currentnet->dec_count();
	currentnet=nullptr;
	remapCache();

	currentface=currentpotential=-1;
	needtodraw=1;
	return 0;
}

//! Install a polyhedron contained in file.
/*! Return 0 for success, 1 for failure.
 */
int NetInterface::InstallPolyhedron(const char *file)
{ ***
	DBG cerr <<"...installPolyhedron("<<file<<")"<<endl;

	char *error=nullptr;
	if (poly->dumpInFile(file,&error)) { //file not polyhedron
		char message[200];
		sprintf(message,"Could not load polyhedron: %s",file);
		PostMessage(message);
		return 1;
	}
	touch_recently_used_xbel(file, "application/x-polyptych-doc", nullptr,nullptr, "Polyptych", true,false, nullptr);
	char str[200];
	sprintf(str,"Loaded new polyhedron: %s",lax_basename(file));
	PostMessage(str);

	poly->makeedges();
	poly->BuildExtra();
	
	nets.flush();
	if (currentnet) currentnet->dec_count();
	currentnet=nullptr;
	makestr(polyhedronfile,file);
	remapCache();
	currentface=currentpotential=-1;
	needtodraw=1;
	return 0;
}

/*! Return 0 for image installed, or nonzero for error installing, and old image kept.
 * Updates spheremap_data.
 */
int NetInterface::InstallImage(const char *file)
{ ***
	DBG cerr <<"attempting to install image at "<<file<<endl;

	const char *error=nullptr;

	try {
		LaxImage *image = ImageLoader::LoadImage(file, nullptr,0,0,nullptr, 0, LAX_IMAGE_DEFAULT, nullptr, false, 0);
		if (!image) throw _("Could not load ");
		
		int width;
		width  = image->w();
		//height = image->w();

		 //gl texture dimensions must be powers of 2
		if (width>2048) { spheremap_width=2048; spheremap_height=1024; }
		else if (width>1024) { spheremap_width=1024; spheremap_height=512; }
		else if (width>512) { spheremap_width=512; spheremap_height=256; }
		else { spheremap_width=256; spheremap_height=128; }

		if (spheremap_data) delete[] spheremap_data;
		spheremap_data = new unsigned char[spheremap_width*spheremap_height*3];

		 //crop and scale image to image_scaled
		LaxImage *image_scaled = ImageLoader::NewImage(spheremap_width,spheremap_height);
		Displayer *dp = GetDefaultDisplayer();
		dp->MakeCurrent(image_scaled);
		dp->imageout(image, 0,0, spheremap_width,spheremap_height);
		dp->EndDrawing();

		unsigned char *data = image_scaled->getImageBuffer();

		int i=0,i2=0;
		for (int c=0; c<spheremap_width; c++) {
		  for (int c2=0; c2<spheremap_height; c2++) {
			spheremap_data[i++] = data[i2++];
			spheremap_data[i++] = data[i2++];
			spheremap_data[i++] = data[i2++];
			i2++; //skip alpha
		  }
		}

		makestr(spherefile,file);
	} catch (const char *err) {
		error=err;
	}

	char e[400];
	if (error) sprintf(e,_("Error with %s: %s"),error,file);
	else {
		installSpheremapTexture(0);
		sprintf(e,_("Loaded image %s"),file);
	}

	if (error) app->postmessage(e);
	//else remapCache();

	DBG cerr <<"\n"
	DBG	 <<"Using sphere file:"<<spherefile<<endl
	DBG	 <<" scaled width: "<<spheremap_width<<endl
	DBG	 <<"       height: "<<spheremap_height<<endl;

	return error?1:0;
}

//! Zap the image to be a generic lattitude and longitude
/*! Pass in a number outside of [0..1] to use default for that color.
 *
 * This updates spheremap_data, spheremap_width, and spheremap_height.
 * It does not reapply the image to the GL texture area.
 */
void NetInterface::UseGenericImageData(double fg_r, double fg_g, double fg_b,  double bg_r, double bg_g, double bg_b)
{ ***
	cerr <<"Generating generic latitude and longitude lines..."<<endl;

	int fr=(fg_r*255+.5),
		fg=(fg_g*255+.5),
		fb=(fg_b*255+.5);
	int br=(bg_r*255+.5),
		bg=(bg_g*255+.5),
		bb=(bg_b*255+.5);
	
	if (fr<0 || fr>255) { fr=100; fg=100; fb=200; }
	if (br<0 || br>255) { br=0; bg=0; bb=0; }


	spheremap_width=512;
	spheremap_height=256;
	if (spheremap_data) delete[] spheremap_data;
	spheremap_data=new unsigned char[spheremap_width*spheremap_height*3];

	 //set background color
	//memset(spheremap_data,0, spheremap_width*spheremap_height*3);
	for (int y=0; y<spheremap_height; y++) {
		for (int x=0; x<spheremap_width; x++) {
			spheremap_data[3*(x+y*spheremap_width)  ]=bb;
			spheremap_data[3*(x+y*spheremap_width)+1]=bg;
			spheremap_data[3*(x+y*spheremap_width)+2]=br;
		}
	}

	 //draw longitude
	for (int x=0; x<spheremap_width; x+=(int)((double)spheremap_width/36)) {
		for (int y=0; y<spheremap_height; y++) {
			spheremap_data[3*(x+y*spheremap_width)  ]=fb;
			spheremap_data[3*(x+y*spheremap_width)+1]=fg;
			spheremap_data[3*(x+y*spheremap_width)+2]=fr;
		}
	}
	 //draw latitude
	for (int y=0; y<spheremap_height; y+=(int)((double)spheremap_height/18)) {
		for (int x=0; x<spheremap_width; x++) {
			spheremap_data[3*(x+y*spheremap_width)  ]=fb;
			spheremap_data[3*(x+y*spheremap_width)+1]=fg;
			spheremap_data[3*(x+y*spheremap_width)+2]=fr;
		}
	}
}

int NetInterface::Event(const EventData *data,const char *mes)
{ ***
	if (!strcmp(mes,"new poly")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(data);
		if (!s) return 1;
		installPolyhedron(s->str);
		return 0;
	}

	if (!strcmp(mes,"new image")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(data);
		if (!s) return 1;
		installImage(s->str);
		return 0;
	}

	if (!strcmp(mes,"saveas")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(data);
		if (!s || !s->str || !*s->str) return 1;

		makestr(polyptychfile,s->str);
		//if (Save(s->str)==0) PostMessage2("%s saved",s->str);
		//else PostMessage2("Error saving %s, not saved!",s->str);

		char ss[strlen(s->str)+50];
		if (Save(s->str)==0) sprintf(ss,"%s saved",s->str);
		else sprintf(ss,"Error saving %s, not saved!",s->str);
		PostMessage(ss);

		return 0;
	}

	if (!strcmp(mes,"renderto")) {
		cout <<"**** must implement background rendering, and multinet output!!"<<endl;

		const StrEventData *s=dynamic_cast<const StrEventData*>(data);
		if (!s || !s->str || !*s->str) {
			DBG cerr <<" Missing destination to render to."<<endl;
			return 1;
		}

		if (!currentnet) { 
			DBG cerr <<" Missing current net to render with."<<endl;
			return 0;
		}
		DBG cerr <<"\n\n-------Rendering, please wait-----------\n"<<endl;

		currentnet->rebuildLines();
		Laxkit::ErrorLog log;
		int status=SphereToPoly(spherefile,
				 poly,
				 currentnet, 
				 2300,      // Render images no more than this many pixels wide
				 s->str, // Say filebase="blah", files will be blah000.png, ...this creates default filebase name
				 OUT_SVG,          // Output format
				 3, // oversample*oversample per image pixel. 3 seems pretty good in practice.
				 draw_texture, // Whether to actually render images, or just output new svg, for intance.
				 !draw_texture, // Whether to only draw lines, or include file names too
				 &extra_basis,
				 papers.n,
				 papers.e,
				 &log
				);
		
		if (log.Errors() || status!=0) {
			char *err=log.FullMessageStr();
			if (err) {
				PostMessage(err);
				delete[] err;
			} else PostMessage("Unknown error encountered during rendering. The developers need to account for this!!!");
		} else PostMessage(_("Rendered."));

		DBG cerr <<"result of render call: "<<status<<endl;
		return 0;
	}

	return anXWindow::Event(data,mes);
}

Laxkit::MenuInfo *NetInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu = new MenuInfo();

	// if (singles) menu->AddToggleItem(_("Double sided"), SINGLES_ToggleDoubleSided, 0, singles->double_sided);
	menu->AddSep(_("New net"));
	menu->AddSubMenu(_("New"));
	menu->AddItem(_("Blank"));
	menu->AddItem(_("Box"));
	menu->AddItem(_("Accordion"));
	menu->AddItem(_("Polyhedron"));
	menu->EndSubMenu();

	return menu;
}

Laxkit::ShortcutHandler *NetInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager = GetDefaultShortcutManager();
	sc = manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc = new ShortcutHandler(whattype());

	sc->AddShortcut(LAX_F1,0,0, NETI_Help);
	sc->Add(NETI_Papers,         'p',0,0,           _("TogglePapers"), _("Toggle paper overlay"),nullptr,0);
	sc->Add(NETI_LoadPolyhedron, 'p',ControlMask,0, _("Hedron"),       _("Load new polyhedron"),nullptr,0);
	sc->Add(NETI_LoadImage,      'i',ControlMask,0, _("Image"),        _("Load image"),nullptr,0);
	sc->Add(NETI_Render,         'r',ControlMask,0, _("Render"),       _("Render"),nullptr,0);
	sc->Add(NETI_Save,           's',ControlMask,0, _("Save"),         _("Save"),nullptr,0);
	sc->Add(NETI_SaveAs,         'S',ControlMask|ShiftMask,0, _("SaveAs"), _("Save as"),nullptr,0);
	sc->Add(NETI_ResetNets,      'D',ShiftMask,0,   _("ResetNets"),    _("Delete all defined nets"),nullptr,0);
	sc->Add(NETI_TotalUnwrap,    'A',ShiftMask,0,   _("TotalUnwrap"),  _("Totally unwrap"),nullptr,0);
	sc->Add(NETI_DrawSeams,      'n',0,0,           _("DrawSeams"),    _("Toggle drawing seams"),nullptr,0);
	sc->Add(NETI_ToggleTexture,  't',0,0,           _("ToggleTexture"),_("Toggle using texture"),nullptr,0);
	sc->Add(NETI_DrawEdges,      'l',0,0,           _("DrawEdges"),    _("Toggle drawing edges"),nullptr,0);
	sc->Add(NETI_NextFace,       'w',0,0,           _("NextFace"),     _("Select next face"),nullptr,0);
	sc->Add(NETI_PrevFace,       'v',0,0,           _("PrevFace"),     _("Select previous face"),nullptr,0);
	// sc->Add(NETI_ScaleUp,        '0',0,0,           _("ScaleUp"),      _("Scale object up"),nullptr,0);
	// sc->Add(NETI_ScaleDown,      '9',0,0,           _("ScaleDown"),    _("Scale object down"),nullptr,0);
	// sc->Add(NETI_ResetView,      ' ',0,0,           _("ResetView"),    _("Make camera point at object from a reasonable distance"),nullptr,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

/*! Return 0 for action performed, else 1.
 */
int NetInterface::PerformAction(int action)
{ ***
	if (action == NETI_Papers) {
		draw_papers = !draw_papers;
		needtodraw = 1;
		return 0;

	} else if (action == NETI_LoadPolyhedron) {
		// change polyhedron
		FileDialog *f = new FileDialog(nullptr,
									_("New Polyhedron"),_("New Polyhedron"),
									ANXWIN_REMEMBER,
									0,0,800,600,0,
									object_id,"new poly",
									FILES_OPEN_ONE
									);
		f->RecentGroup("Polyhedron");
		app->rundialog(f);
		return 0;

	} else if (action == NETI_LoadImage) {
		// change image
		app->rundialog(new FileDialog(nullptr,_("New Image"),_("New Image"),
									ANXWIN_REMEMBER,
									0,0,800,600,0, object_id,"new image",
									FILES_PREVIEW|FILES_OPEN_ONE
									));
		return 0;

	} else if (action == NETI_Render) {
		// render with spheretopoly
		app->rundialog(new FileDialog(nullptr, _("Select render directory"),_("Select render directory"),
									ANXWIN_REMEMBER,
									0,0,800,600,0, object_id,"renderto",
									FILES_SAVE_AS,
									"render###.png",polyptychfile));
		return 0;

	} else if (action == NETI_Save || action == NETI_SaveAs) {
		// save to a polyptych file
		char *file=nullptr;
		int saveas=(action==NETI_SaveAs);

		if (!polyptychfile) {
			saveas=1;
			char *p=nullptr;
			if (spherefile && *spherefile) {
				appendstr(file,spherefile);
				p=strrchr(file,'.');
				if (p) *p='\0';
				const char *bname=lax_basename(polyhedronfile);
				if (bname) {
					appendstr(file,"-");
					appendstr(file,bname);
				}

			} else {
				if (polyhedronfile) appendstr(file,polyhedronfile);
				else makestr(file,"default-cube");
			}
			const char *ptr=strrchr(file,'/');
			p=strrchr(file,'.');
			if (p>ptr) *p='\0';
			appendstr(file,".polyptych");
		} else makestr(file,polyptychfile);

		Displayer *dp = MakeCurrent();
		dp->NewFG(rgbcolor(255,255,255));
		dp->textout(0,app->defaultlaxfont->textheight(), file,-1, LAX_LEFT|LAX_TOP);

		if (saveas) {
			 //saveas
			app->rundialog(new FileDialog(nullptr,_("Save as..."),_("Save as..."),
							ANXWIN_REMEMBER,
							0,0,0,0,0, object_id, "saveas",
							FILES_SAVE_AS|FILES_ASK_TO_OVERWRITE,
							file));
		} else {
			 //normal save
			DBG cerr<<"Saving to "<<file<<endl;
			if (Save(file)==0) dp->textout(0,2*app->defaultlaxfont->textheight(), "...Saved",-1, LAX_LEFT|LAX_TOP);
			else dp->textout(0,2*app->defaultlaxfont->textheight(), "...ERROR!!! Not saved",-1, LAX_LEFT|LAX_TOP);
		}

		delete[] file;
		return 0;

	} else if (action == NETI_ResetNets) {
		// remove the current net, reverting all faces to no net
		if (!currentnet) return 0;
		for (int c=0; c<currentnet->faces.n; c++) {
			if (currentnet->faces.e[c]->tag!=FACE_Actual) continue;
			poly->faces.e[currentnet->faces.e[c]->original]->cache->facemode=0;
		}
		removeNet(currentnet);
		remapCache();
		needtodraw=1;
		return 0;

	} else if (action == NETI_TotalUnwrap) {
		// total unwrap
		while (nets.n>1) nets.remove(nets.n-1);
		if (nets.n==0) {
			Net *net=new Net;
			net->_config=1;
			net->Basenet(poly);
			net->Unwrap(-1,0);
			poly->faces.e[0]->cache->facemode=-net->object_id;
			net->info=0;

			nets.push(net);
			net->dec_count();
		}
		nets.e[0]->TotalUnwrap();
		for (int c=0; c<poly->faces.n; c++) {
			if (c==nets.e[0]->info) poly->faces.e[c]->cache->facemode=-nets.e[0]->object_id;
			poly->faces.e[c]->cache->facemode=nets.e[0]->object_id;
		}
		remapCache();
		needtodraw=1;
		return 0;

	} else if (action == NETI_DrawSeams) {
		draw_seams++;
		if (draw_seams>3) draw_seams=0;
		if      (draw_seams==0) PostMessage(_("Do not draw net edges or unwrap path."));
		else if (draw_seams==1) PostMessage(_("Draw net edges, but not unwrap path."));
		else if (draw_seams==2) PostMessage(_("Draw unwrap path, but not net edges."));
		else if (draw_seams==3) PostMessage(_("Draw both net edges, and unwrap path."));
		needtodraw=1;
		return 0;

	} else if (action == NETI_ToggleTexture) {
		draw_texture=!draw_texture;
		needtodraw=1;
		return 0;

	} else if (action == NETI_DrawEdges) {
		draw_edges=!draw_edges;
		needtodraw=1;
		return 0;

	} else if (action == NETI_NextFace) {
		currentface++;
		if (currentface>=poly->faces.n) currentface=0;
		needtodraw=1;
		return 0;

	} else if (action == NETI_PrevFace) {
		currentface--;
		if (currentface<0) currentface=poly->faces.n-1;
		needtodraw=1;
		return 0;

	// } else if (action==NETI_ScaleUp) {
	// 	hedron->SetScale(1.2,1.2,1.2);
	// 	needtodraw=1;
	// 	return 0; 

	// } else if (action==NETI_ScaleDown) {
	// 	hedron->SetScale(.8,.8,.8);
	// 	needtodraw=1;
	// 	return 0; 

	// } else if (action == NETI_ResetView) {
	// 	//***
	// 	needtodraw=1;
	// 	return 0;

	}


	return 1;
}

int NetInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const LaxKeyboard *kb)
{
	if (!win_on) return 0;

	if (ch == LAX_Esc) { 
		if (currentnet) {
			// deactivate current net
			if (currentnet->numActual() == 1) removeNet(currentnet);
			if (currentnet) currentnet->dec_count();
			currentnet = nullptr;
			needtodraw = 1;
		}
		return 0;
	}

	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch, state & LAX_STATE_MASK, 0);
	if (action >= 0) {
		return PerformAction(action);
	}

	return 1;
}


} //namespace Laidout


