//
// Laidout Signature Editor, using Processing.js
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




//------------------------------- setup() --------------------------------
SignatureInterface interf;
//fullscreen, in processing 2, not processing.js:
//int DISPLAYWIDTH =displayWidth;
//int DISPLAYHEIGHT=displayHeight;

//absolute size
int DISPLAYWIDTH =700;
int DISPLAYHEIGHT=700;

//for use with processing js:   **** COMMENT OUT IF USING JAVA PROCESSING ****
DISPLAYWIDTH =window.innerWidth-40;
DISPLAYHEIGHT=window.innerHeight*.9;




float TEXTHEIGHT=20;

void setup()
{
  size(DISPLAYWIDTH, DISPLAYHEIGHT);

  textFont(createFont( "Arial", 20, true));
  textAlign(CENTER);
  TEXTHEIGHT=textAscent()+textDescent();
  
  if (TEXTHEIGHT*4>DISPLAYHEIGHT/8) {
      TEXTHEIGHT=DISPLAYHEIGHT/8/4;
      if (TEXTHEIGHT<5) TEXTHEIGHT=5;
      textSize(TEXTHEIGHT);
  }

  interf=new SignatureInterface();
}

void draw()
{
    interf.Refresh();
}


//------------------------------- various state --------------------------------

int needtodraw=1;
int ShiftMask=1;
int ControlMask=2;
String statusmessage="";

int initialmousex=0;
int initialmousey=0;
int mouseinfo1=0;
int mouseinfo2=0;

int kbstate=0;


//--------------------------- key and mouse event relaying ----------------------------
void mousePressed()
{
    initialmousex=mouseX;
    initialmousey=mouseY;

        if (interf.showsplash) {
          interf.showsplash=false;
          needtodraw=1;
        }    

    if (mouseButton == LEFT) interf.LBDown(mouseX, mouseY, kbstate);
    //else if (mouseButton == RIGHT) interf.RBDown(mouseX, mouseY);
    // No mouse wheels in processing.js!!! #@&$*@^F#$&^*^@#&*$#*@$^*
    //see https://github.com/annasob/processing-js/commit/6083bac3d20e41a6c7d857a272efd8fcda3cf120
    // for a hackaround
}

void mouseReleased()
{
    if (mouseButton == LEFT) interf.LBUp(mouseX, mouseY, kbstate);
    //else if (mouseButton == RIGHT) interf.RBUp(mouseX, mouseY);
    // No mouse wheels in processing!!! #@&$*@^F#$&^*^@#&*$#*@$^*
}

void mouseMoved()
{
    interf.MouseMove(mouseX, mouseY, kbstate);
}

void mouseDragged()
{
    interf.MouseDrag(mouseX, mouseY, kbstate);
}

void keyPressed()
{
    if (key==CODED) {
        if (keyCode==CONTROL) kbstate|=ControlMask;
        else if (keyCode==SHIFT) kbstate|=ShiftMask;

    } else CharDown(key);
}

void keyReleased()
{
    if (key==CODED) {
        if (keyCode==CONTROL) kbstate&=~ControlMask;
        else if (keyCode==SHIFT) kbstate&=~ShiftMask;

    } 
}


void CharDown(int ch)
{
    if (interf.showsplash) {
        interf.showsplash=false;
        needtodraw=1;
        return;
    }


    int action=-1;
        Shortcut shortcut;
    for (int c=0; c<interf.sc.size(); c++) {
                shortcut=(Shortcut)interf.sc.get(c);
        if (shortcut.key==ch && shortcut.state==kbstate) {
            action=shortcut.id;
            break;
        }
    }
    if (action>=0) {
        interf.PerformAction(action);
    }

}


//------------------------------ various helper stuff -----------------------
void PostMessage(String message)
{
    statusmessage=message;
    needtodraw=1;
}

void DebugMessage(String message)
{
    println(message);
}


String masktostr(int m)
{
    if (m==15) return ("all");
    if (m==1)  return ("top");
    if (m==2)  return ("right");
    if (m==4)  return ("bottom");
    if (m==8)  return ("left");
    return "?";
}

//menu ids
int SIGM_Portrait = 2000;
int SIGM_Landscape = 2001;
int SIGM_SaveAsResource = 2002;
int SIGM_FinalFromPaper = 2003;
int SIGM_CustomPaper = 2004;

//shortcut ids
int SIA_Decorations = 1;
int SIA_Thumbs = 2;
int SIA_Center = 3;
int SIA_InsetMask = 4;
int SIA_InsetInc = 5;
int SIA_InsetDec = 6;
int SIA_GapInc = 7;
int SIA_GapDec = 8;
int SIA_TileXInc = 9;
int SIA_TileXDec = 10;
int SIA_TileYInc = 11;
int SIA_TileYDec = 12;
int SIA_NumFoldsVInc = 13;
int SIA_NumFoldsVDec = 14;
int SIA_NumFoldsHInc = 15;
int SIA_NumFoldsHDec = 16;
int SIA_BindingEdge = 17;
int SIA_BindingEdgeR = 18;
int SIA_PageOrientation = 19;
int SIA_TrimMask = 20;
int SIA_TrimInc = 21;
int SIA_TrimDec = 22;
int SIA_MarginMask = 23;
int SIA_MarginInc = 24;
int SIA_MarginDec = 25;
int SIA_ZoomIn = 26;      
int SIA_ZoomOut = 28;     
int SIA_ZoomInControls = 29;
int SIA_ZoomOutControls = 30;



class Shortcut
{
    int id;
    int key;
    int state;
    String name;
    String description;
    Shortcut(int i,int k,int s,String n,String d) { id=i; key=k; state=s; name=n; description=d; }
}


//----------------------------------------- papersizes ------------------------------
String currentPaper="Letter";
int currentOrientation=0; //1 is landscape.. w and h are swapped when switched
float currentPaperWidth=8.5;
float currentPaperHeight=11;
String currentPaperUnits="in";
int    currentPaperIndex=0;

//      PAPERSIZE   Width  Height  Units
//      ----------------------------------------
String[] BuiltinPaperSizes=
    {
        "Letter"   ,"8.5" ,"11"  ,"in",
        "Legal"    ,"8.5" ,"14"  ,"in",
        "Tabloid"  ,"11"  ,"17"  ,"in",
        "A4"       ,"210" ,"297" ,"mm",
        "A3"       ,"297" ,"420" ,"mm",
        "A2"       ,"420" ,"594" ,"mm",
        "A1"       ,"594" ,"841" ,"mm",
        "A0"       ,"841" ,"1189","mm",
        "A5"       ,"148" ,"210" ,"mm",
        "A6"       ,"105" ,"148" ,"mm",
        "A7"       ,"74"  ,"105" ,"mm",
        "A8"       ,"52"  ,"74"  ,"mm",
        "A9"       ,"37"  ,"52"  ,"mm",
        "A10"      ,"26"  ,"37"  ,"mm",
        "B0"       ,"1000","1414","mm",
        "B1"       ,"707" ,"1000","mm",
        "B2"       ,"500" ,"707" ,"mm",
        "B3"       ,"353" ,"500" ,"mm",
        "B4"       ,"250" ,"353" ,"mm",
        "B5"       ,"176" ,"250" ,"mm",
        "B6"       ,"125" ,"176" ,"mm",
        "B7"       ,"88"  ,"125" ,"mm",
        "B8"       ,"62"  ,"88"  ,"mm",
        "B9"       ,"44"  ,"62"  ,"mm",
        "B10"      ,"31"  ,"44"  ,"mm",
        "C0"       ,"917" ,"1297","mm",
        "C1"       ,"648" ,"917" ,"mm",
        "C2"       ,"458" ,"648" ,"mm",
        "C3"       ,"324" ,"458" ,"mm",
        "C4"       ,"229" ,"324" ,"mm",
        "C5"       ,"162" ,"229" ,"mm",
        "C6"       ,"114" ,"162" ,"mm",
        "C7"       ,"81"  ,"114" ,"mm",
        "C8"       ,"57"  ,"81"  ,"mm",
        "C9"       ,"40"  ,"57"  ,"mm",
        "C10"      ,"28"  ,"40"  ,"mm",
        "ArchA"    ,"9"   ,"12"  ,"in",
        "ArchB"    ,"12"  ,"18"  ,"in",
        "ArchC"    ,"18"  ,"24"  ,"in",
        "ArchD"    ,"24"  ,"36"  ,"in",
        "ArchE"    ,"36"  ,"48"  ,"in",
        "Flsa"     ,"8.5" ,"13"  ,"in",
        "Flse"     ,"8.5" ,"13"  ,"in",
        "Index"    ,"3"   ,"5"   ,"in",
        "Executive","7.25","10.5","in",
        "Ledger"   ,"17"  ,"11"  ,"in",
        "Halfletter","5.5","8.5" ,"in",
        "Note"      ,"7.5","10"  ,"in",
        "Custom"    ,"8.5","11"  ,"in"
    };

//! w,h,units only used if "Custom" paper.
void ChangePaper(int index, float w,float h, String units)
{
    if (index==-1) {
        index=currentPaperIndex+1;        
    } else if (index==-2) {
                index=currentPaperIndex-1;
    }
    if (index<0) index=BuiltinPaperSizes.length/4-4;
    else if (index>BuiltinPaperSizes.length/4-4) index=0;
    //println("change paper:"+index+", "+BuiltinPaperSizes.length);

        currentPaperIndex =index;
    currentPaper      =BuiltinPaperSizes[index*4];
    currentPaperWidth =float(BuiltinPaperSizes[index*4+1]);
    currentPaperHeight=float(BuiltinPaperSizes[index*4+2]);
    currentPaperUnits =BuiltinPaperSizes[index*4+3];

    if (currentPaper=="Custom") {
        currentPaperWidth=w;
        currentPaperHeight=h;
        currentPaperUnits=units;
    }

    if (currentOrientation==1) {
        float t=currentPaperWidth;
        currentPaperWidth=currentPaperHeight;
        currentPaperHeight=t;
    }
}

void ChangePaperOrientation()
{
    if (currentOrientation==0) currentOrientation=1; else currentOrientation=0;
    float t=currentPaperWidth;
    currentPaperWidth=currentPaperHeight;
    currentPaperHeight=t;

}

//----------------------------- DoubleBBox -----------------------------
class DoubleBBox
{
    float minx;
    float maxx;
    float miny;
    float maxy;

    DoubleBBox() { minx=miny=0; maxx=maxy=-1; }
        DoubleBBox(flatpoint p) { minx=maxx=p.x; miny=maxy=p.y; }
    DoubleBBox(float nminx, float nmaxx, float nminy, float nmaxy)
      { minx=nminx; miny=nminy; maxx=nmaxx; maxy=nmaxy; }

    void addtobounds(flatpoint p)
    {
        if (maxx<minx || maxy<miny) {
            minx=maxx=p.x;
            miny=maxy=p.y;
        } else {
            if (p.x<minx) minx=p.x;
            else if (p.x>maxx) maxx=p.x;
            if (p.y<miny) miny=p.y;
            else if (p.y>maxy) maxy=p.y;
        }
    }
}

//--------------------------- Displayer ----------------------------

color bg_color;
color fg_color;

//for text alignment
int LAX_LEFT      = (1<<0);
int LAX_HCENTER   = (1<<1);
int LAX_RIGHT     = (1<<2);
int LAX_TOP       = (1<<5);
int LAX_VCENTER   = (1<<6);
int LAX_BOTTOM    = (1<<7);
int LAX_BASELINE  = (1<<8);
int LAX_CENTER    = (1<<1|1<<6);



class flatpoint
{
    float x,y;
        flatpoint() { x=y=0; }
    flatpoint (float nx,float ny) { x=nx; y=ny; }
        flatpoint (flatpoint p) { x=p.x; y=p.y; }
        void copy(flatpoint p) { x=p.x; y=p.y; }
    void add(flatpoint p) { x+=p.x; y+=p.y; }
    void subtract(flatpoint p) { x-=p.x; y-=p.y; }
    void multiply(float d) { x*=d; y*=d; }
    float dot(flatpoint p) { return x*p.x+y*p.y; }
    float norm() { return sqrt((float)(x*x+y*y)); }
        float distanceto(flatpoint p) { return sqrt((p.x-x)*(p.x-x)+(p.y-y)*(p.y-y)); }
}

flatpoint rotate(flatpoint p, float angle, boolean decimal)
{
    if (decimal) angle*=(PI/180);
    return new flatpoint(p.x*cos(angle)-p.y*sin(angle),
                p.y*cos(angle)+p.x*sin(angle));

}


void transform_copy(float[] m1, float[] m2)
{
  m1[0]=m2[0];
  m1[1]=m2[1];
  m1[2]=m2[2];
  m1[3]=m2[3];
  m1[4]=m2[4];
  m1[5]=m2[5];
}

int THING_None          = 0;
int THING_Arrow_Down    = 1;
int THING_Arrow_Up      = 2;
int THING_Arrow_Left    = 3;
int THING_Arrow_Right   = 4;
int THING_Triangle_Down = 5;
int THING_Circle        = 6;
int THING_Square        = 7;

class Displayer
{
    int linetype; //0 for solid, 1 for dashed
    boolean drawreal;
    float strokewidth;

    float[] ctm ={1,0,0,-1,0,0};
    float[] ictm={1,0,0,-1,0,0};
    ArrayList axesstack;

    Displayer() {
            strokewidth=1;
            linetype=0;
            drawreal=true;
            axesstack=new ArrayList();
            findictm();
         }

    float textheight() { return textAscent()+textDescent(); }
        
    void NewFG(float r,float g,float b) {
        fg_color = color(int(r*255), int(g*255), int(b*255));
                stroke(fg_color);
                fill(fg_color);
    }
    void NewFG(long l) {
        fg_color = color(l&0xff, l&0xff00, l&0xff0000);
                stroke(fg_color);
                fill(fg_color);
    }

    void NewFG(color c) {
        fg_color = color(red(c),green(c),blue(c));
                stroke(fg_color);
                fill(fg_color);
    }

    void StrokeWidth(float  width) { strokewidth=width; strokeWeight(width); }
    //void SetDashedLine() { linetype=1; } <- currently ignored
    //void SetSolidLine()  { linetype=0; }

    float Getmag() {
          return sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]);
    }

    void DrawReal() { drawreal=true; }
    void DrawScreen() { drawreal=false; }

    flatpoint screentoreal(int x,int y) {
                //dumpctm();
                //println("screentoreal "+x+','+y+"  ->  "+(ictm[4] + ictm[0]*(float)x + ictm[2]*(float)y)+", "+(ictm[5]+ictm[1]*(float)x+ictm[3]*(float)y));
                
        return new flatpoint(ictm[4] + ictm[0]*(float)x + ictm[2]*(float)y, ictm[5]+ictm[1]*(float)x+ictm[3]*(float)y);
    }

    flatpoint realtoscreen(float x,float y) {
        return new flatpoint(ctm[4] + ctm[0]*x + ctm[2]*y, ctm[5]+ctm[1]*x+ctm[3]*y);
    }
    void ShiftReal(float dx, float dy) {
        float[] newt={0,0,0,0,0,0};
        transform_copy(newt,ctm);
        newt[4]+=dx*newt[0]+dy*newt[2];
        newt[5]+=dx*newt[1]+dy*newt[3];
        transform_copy(ctm,newt);
        findictm();
    }
    void findictm()
    {
        float d=ctm[0]*ctm[3]-ctm[1]*ctm[2];
        ictm[0]=ctm[3]/d;
        ictm[1]=-ctm[1]/d;
        ictm[2]=-ctm[2]/d;
        ictm[3]=ctm[0]/d;
        ictm[4]=(ctm[2]*ctm[5]-ctm[4]*ctm[3])/d;
        ictm[5]=(ctm[1]*ctm[4]-ctm[0]*ctm[5])/d;
         }
        void dumpctm() {
              println("ctm:  {"+ctm[0]+", "+ctm[1]+", "+ctm[2]+", "+ctm[3]+", "+ctm[4]+", "+ctm[5]+'}');
              println("ictm: {"+ictm[0]+", "+ictm[1]+", "+ictm[2]+", "+ictm[3]+", "+ictm[4]+", "+ictm[5]+'}');          
        }
    void ShiftScreen(int dx,int dy)
    {
        ctm[4]+=dx;
        ctm[5]+=dy;
        findictm();
    }

    void Zoom (float m)
    {
        ctm[0]*=m;
        ctm[1]*=m;
        ctm[2]*=m;
        ctm[3]*=m;
        
        findictm();
    }

    void Center(float minx,float maxx,float miny,float maxy) {
                DoubleBBox  bb=new DoubleBBox(realtoscreen(minx,miny));
                bb.addtobounds(realtoscreen(maxx,miny));
                bb.addtobounds(realtoscreen(maxx,maxy));
                bb.addtobounds(realtoscreen(minx,maxy));

                if ((bb.maxx-bb.minx)/(bb.maxy-bb.miny)>float(DISPLAYWIDTH)/(DISPLAYHEIGHT)) Zoom((DISPLAYWIDTH)/(bb.maxx-bb.minx));
                else Zoom((DISPLAYHEIGHT)/(bb.maxy-bb.miny));
                CenterPoint(new flatpoint((minx+maxx)/2,(miny+maxy)/2));
    
    }


    //! Centers the view at real point p.
    /*! This finds the position of real point p on screen, then uses ShiftScreen() to center it.
     */
    void CenterPoint(flatpoint p)
    {
        flatpoint s;
        s= new flatpoint((DISPLAYWIDTH)/2,(DISPLAYHEIGHT)/2);
        s.subtract(realtoscreen(p.x,p.y));
        ShiftScreen((int)s.x,(int)s.y);
    }

    void drawline(float x1,float y1, float x2, float y2) {
                   if (drawreal) {
            flatpoint p=realtoscreen(x1,y1);
            x1=p.x;
            y1=p.y;
            p=realtoscreen(x2,y2);
            x2=p.x;
            y2=p.y;
        }
                
    
         //solid
        line(x1,y1, x2,y2);
    } 

    void drawline_dashed(float x1,float y1, float x2, float y2) {
        if (drawreal) {
            flatpoint p=realtoscreen(x1,y1);
            x1=p.x;
            y1=p.y;
            p=realtoscreen(x2,y2);
            x2=p.x;
            y2=p.y;
        }
                
         //dashed: no builtin!!!
        float dashdist=5*strokewidth;
        float dx=(x2-x1);
        float dy=(y2-y1);
        float d=sqrt(dx*dx+dy*dy);
        float dd=d/dashdist;
        dx=dx/dd;
        dy=dy/dd;
        
        float x=x1;
        float y=y1;
        float xx,yy;
        for (float dc=0; dc<d; dc+=dashdist) {
            xx=x+dx/2;  yy=y+dy/2;
            line(x,y, xx,yy);
            x=x+dx;
            y=y+dy;
        }
    } 

    void drawlines_dashed(flatpoint[] pts, int closed) {

        for (int c=0; c<pts.length-1; c++) {
            drawline_dashed(pts[c].x,pts[c].y, pts[c+1].x,pts[c+1].y);
        }
        if (closed==1){
            drawline_dashed(pts[pts.length-1].x,pts[pts.length-1].y,  pts[0].x,pts[0].y);
        }
    }

    void drawlines(flatpoint[] pts, int closed, int filled) {
        if (pts==null) return;

        if (filled==0) noFill();
        else fill(fg_color);

                flatpoint p=new flatpoint();
                
        beginShape();
        for (int c=0; c<pts.length; c++) {
                        if (drawreal) {
                            p=realtoscreen(pts[c].x,pts[c].y);
                            vertex(p.x,p.y);
                        } else {
                           vertex(pts[c].x,pts[c].y);
                        }
            
        }
        if (closed==1){
            if (drawreal) {
                            p=realtoscreen(pts[0].x,pts[0].y);
                            vertex(p.x,p.y);
                        } else {
                           vertex(pts[0].x,pts[0].y);
                        }
        }
        endShape();
    }

    void drawarrow(flatpoint p,flatpoint p2)
    {
        float x1=p.x;
        float y1=p.y;
        float x2=p2.x;
        float y2=p2.y;
        if (drawreal) {
            flatpoint pp=realtoscreen(x1,y1);
            x1=pp.x;
            y1=pp.y;
            pp=realtoscreen(x2,y2);
            x2=pp.x;
            y2=pp.y;
        }

        float vx=(x2-x1)/4;
        float vy=(y2-y1)/4;

        line(x1,y1, x2,y2);
        line(x2,y2,  x2-vx+vy, y2-vy-vx);
        line(x2,y2,  x2-vx-vy, y2-vy+vx);
    }

    void PushAxes() {
        axesstack.add(ctm);
        float[] m={0,0,0,0,0,0};
        transform_copy(m,ctm);
        ctm=m;

        //pushMatrix();
    }
    void PopAxes() {
        float[] m=(float[])axesstack.get(axesstack.size()-1);
        transform_copy(ctm,m);
                findictm();
        
        //popMatrix();
    }

    void textout(float x,float y, String str, int align)
    {
                int halign=CENTER,valign=CENTER;
                
                if (drawreal) {
                  flatpoint p=realtoscreen(x,y);
                  x=p.x; y=p.y;
                }
                
                if ((align&LAX_RIGHT)!=0) halign=RIGHT;
                else if ((align&LAX_LEFT)!=0) halign=LEFT;
                if ((align&LAX_TOP)!=0) valign=TOP;
                else if ((align&LAX_BOTTOM)!=0) valign=BOTTOM;

                textAlign(halign,valign);
        text(str, x,y);
    }

    void drawthing(float x, float y, float xr, float yr, int filled, int what) {
        if (filled==0) noFill();
        else fill(fg_color);

         //square, circle, tri down, or arrows
        if (what==THING_Arrow_Down   ) {
            drawarrow(new flatpoint(x,y-yr), new flatpoint(x,y+yr));

        } else if (what==THING_Arrow_Up     ) {
            drawarrow(new flatpoint(x,y+yr), new flatpoint(x,y-yr));

        } else if (what==THING_Arrow_Left   ) {
            drawarrow(new flatpoint(x+xr,y), new flatpoint(x-xr,y));

        } else if (what==THING_Arrow_Right  ) {
            drawarrow(new flatpoint(x-xr,y), new flatpoint(x+xr,y));

        } else if (what==THING_Triangle_Down) {
            beginShape();
            vertex(x-xr,y-yr);
            vertex(x+xr,y-yr);
            vertex(x,   y+yr);
            endShape();

        } else if (what==THING_Circle       ) {
            beginShape();
            vertex(x  ,y-yr);
            bezierVertex(x+xr*.56,y-yr,  x+xr,y-yr*.56,  x+xr,y);
            bezierVertex(x+xr,y+yr*.56,  x+xr*.56,y+yr,  x,y+yr);
            bezierVertex(x-xr*.56,y+yr,  x-xr,y+yr*.56,  x-xr,y);
            bezierVertex(x-xr,y-yr*.56,  x-xr*.56,y-yr,  x,y-yr);
            endShape();

        } else if (what==THING_Square       ) {
            beginShape();
            vertex(x-xr,y-yr);
            vertex(x+xr,y-yr);
            vertex(x+xr,y+yr);
            vertex(x-xr,y+yr);
            endShape();

        }
    }
}



//------------------------------------- ActionArea ---------------------------

/*! \class ActionArea
 * \brief This can define areas on screen for various purposes.
 */
/*! \var int ActionArea::offset
 * The outline is at location offset+(the points in outline).
 */
/*! \var int ActionArea::hotspot
 * \brief Defines a point of focus of the area.
 *
 * Say an area defines an arrow, the hotspot would be the point of the arrow.
 * When you call Position(float,float,int) to set the area's position, you are
 * actually setting the position of the hotspot.
 */

int AREA_Display_Only = 1 ;
int AREA_Handle       = 2 ;
int AREA_Slider       = 3 ;
int AREA_Labled_Slider= 4 ;
int AREA_Num_Slider   = 5 ;
int AREA_Button       = 6 ;
int AREA_Toggle       = 7 ;
int AREA_Mode         = 8 ;
int AREA_H_Pan        = 9 ;
int AREA_V_Pan        = 10;
int AREA_Menu_Trigger = 11;
int AREA_H_Slider     = 12;
int AREA_V_Slider     = 13;

int VIS_No   = 0;
int VIS_Fill = 1;
int VIS_Selectable_only=2;
int VIS_Outline=3;
int VIS_Text_Only=4;

class ActionArea
{
    String tip;
    String text;

    flatpoint offset;
    flatpoint tail; //graphic is oriented pointing from tail to hotspot
    flatpoint hotspot;

    int thing_type;
    float minx, maxx;
    float miny, maxy;
    flatpoint[] outline;

    float value; //for sliders:   < label: _value_ >
    float step;

    color col;
    int visible; //1 for yes and filled, 2 for selectable, but not drawn,3 for outline only
    boolean hidden; //skip checks for this one
    int real; //0=screen coordinates, 1=real coordinates, 2=position is real, but outline is screen

    int action; //id for the action this overlay corresponds to
    int mode; //mode that must be active for this area to be active
    int category; //extra identifier
    int type; //basic type this overlay is: AREA_*: handle (movable), slider, button, display only, pan, menu trigger

    ActionArea(int what,
               int thing,
               int ntype,
               String txt,
               String ntooltip,
               int isreal,
               int isvisible,
               color ncolor,
               int ncategory)
    {
        thing_type=thing;

        text=txt;
        tip=ntooltip;
        visible=isvisible;
        real=isreal;
        col=color(red(ncolor), green(ncolor), blue(ncolor));
        action=what;
        type=ntype;
        hidden=false;
        mode=0;
        category=ncategory;
        step=10; //10 px

        offset    =new flatpoint(0,0);
        tail      =new flatpoint(0,0);
        hotspot   =new flatpoint(0,0);

        minx=0;  miny=0;
        maxx=-1; maxy=-1;
    }

    void SetQuad(flatpoint p1, flatpoint p2, flatpoint p3, flatpoint p4)
    {
        outline=new flatpoint[4];
        outline[0]=new flatpoint(p1.x,p1.y);
        outline[1]=new flatpoint(p2.x,p2.y);
        outline[2]=new flatpoint(p3.x,p3.y);
        outline[3]=new flatpoint(p4.x,p4.y);
        FindBBox();
    }

    void SetRect(float x,float y,float w,float h)
    {
        outline=new flatpoint[4];
        outline[0]=new flatpoint(x,y);
        outline[1]=new flatpoint(x+w,y);
        outline[2]=new flatpoint(x+w,y+h);
        outline[3]=new flatpoint(x,y+h);
        FindBBox();
    }

    //! Create outline points.
    /*! If takethem!=0, then make outline=pts, and that array will be delete[]'d in the destructor.
     * Otherise, the points are copied into a new array.
     *
     * If pts==NULL, but n>0, then allocate (or reallocate) the array.
     * It is presumed that the calling code will then adjust the points.
     *
     * outline will be reallocated only if n>npoints.
     *
     * Please note that this function does not set the min/max bounds. You can use FindBBox for that.
     */
    flatpoint[] Points(flatpoint[] pts, int n, boolean takethem)
    {
        if (n<=0) return null;

        if (pts==null) {
            if (outline!=null && n>outline.length) { outline=null; }
            if (outline==null) outline=new flatpoint[n];

        } else if (takethem) {
            outline=pts;

        } else {
            if (outline!=null && n>outline.length) { outline=null; }
            outline=new flatpoint[n];
            for (int c=0; c<n; c++) { outline[c].x=pts[c].x; outline[c].y=pts[c].y; }
        }

        return outline;
    }

    flatpoint Center() {
      if (maxx<minx || maxy<miny) return new flatpoint(minx,miny);
      return new flatpoint((minx+maxx)/2,(miny+maxy)/2);
    }
        
    //! Make the bounds be the actual bounds of outline, in outline space.
    /*! If there are no points in outline, then do nothing.
     */
    void FindBBox()
    {
                minx=0;  miny=0;
                maxx=-1; maxy=-1;
                
        if (outline==null) {
                    flatpoint p=new flatpoint(hotspot);
                    p.add(tail);
                    p.multiply(.5);
                    float r=p.distanceto(hotspot);
                    addtobounds(new flatpoint(p.x-r, p.y-r));
                    addtobounds(new flatpoint(p.x-r, p.y+r));
                    addtobounds(new flatpoint(p.x+r, p.y+r));
                    addtobounds(new flatpoint(p.x+r, p.y-r));
                    return;
                }

        for (int c=0; c<outline.length; c++) addtobounds(outline[c]);
    }

  
    void addtobounds(flatpoint p)
    {
        if (maxx<minx || maxy<miny) {
            minx=maxx=p.x;
            miny=maxy=p.y;
        } else {
            if (p.x<minx) minx=p.x;
            else if (p.x>maxx) maxx=p.x;
            if (p.y<miny) miny=p.y;
            else if (p.y>maxy) maxy=p.y;
        }
    }

    flatpoint Position()
    { 
        return new flatpoint(offset.x+hotspot.x, offset.y+hotspot.y);
    }

    //! Change the position of the area, where pos==offset+hotspot.
    /*! If which&1, adjust x. If which&2, adjust y.
     *
     * To be clear, this will make the x and/or y coordinate of the area's hotspot be
     * at the given (x,y).
     */
    void Position(float x,float y,int which)
    {
        if ((which&1)!=0) offset.x=x-hotspot.x;
        if ((which&2)!=0) offset.y=y-hotspot.y;
    }

    //! Return if point is within the outline (If outline is not NULL), or within the ActionArea DoubleBBox bounds otherwise.
    /*! No point transformation is done. It is assumed that x and y are transformed already to the proper coordinate space.
     * This is to say that the calling code must account for this->real.
     */
    boolean PointIn(float x,float y)
    {
        x-=offset.x; y-=offset.y;
        //if (outline!=null) return point_is_in(new flatpoint(x,y));
        return x>=minx && x<=maxx && y>=miny && y<=maxy;
    }
        

}; //class ActionArea


//----------------------------- naming helper functions -----------------------------

//! Return the name of the fold direction.
/*! This will be "Right" or "Under Left" or some such.
 *
 * If translate!=0, then return that translated, else return english.
 */
String FoldDirectionName(char dir, boolean under)
{
    String str="";

    if (under) {
        if (dir=='r')      str= "Under Right";
        else if (dir=='l') str= "Under Left";
        else if (dir=='b') str= "Under Bottom";
        else if (dir=='t') str= "Under Top";
    } else {
        if (dir=='r')      str= "Right";
        else if (dir=='l') str= "Left";
        else if (dir=='b') str= "Bottom";
        else if (dir=='t') str= "Top";
    }

    return str;
}

/*! \ingroup misc
 *
 * For case insensitive comparisons to "left", "right", "top", "bottom", return
 * 'l', 'r', 't', 'b'.
 *
 * If one of those are found, return 0, else 1.
 */
char LRTBAttribute(String v)
{
    if (v.equalsIgnoreCase("left"))   { return 'l'; }
    if (v.equalsIgnoreCase("right"))  { return 'r'; }
    if (v.equalsIgnoreCase("top"))    { return 't'; }
    if (v.equalsIgnoreCase("bottom")) { return 'b'; }

    return 1;
}

//! Convert 't','b','l','r' to top,bottom,left,right.
/*! \ingroup misc
 */
String CtoStr(char c)
{
    if (c=='t' || c=='u') return "Top";
    if (c=='b' || c=='d') return "Bottom";
    if (c=='l') return "Left";
    if (c=='r') return "Right";
    return "";
}


//------------------------------------- Fold --------------------------------------

/*! \class Fold
 * \brief Line description node in a Signature
 *
 * Each line can be folds or cuts. Cuts can be tiling cuts or finishing cuts.
 */
/*! \var FoldDirectionType Fold::folddirection
 * \brief l over to r, l under to r, rol, rul, tob, tub, bot
 */
/*! \var int FoldDirection::whichfold
 * \brief Index from the left or bottom side of completely unfolded paper of which fold to act on.
 *
 * 1 is the left (or bottom) most fold. numhfolds is the top most fold, and numvfolds is the right most fold.
 */

class Fold
{
    char direction; //l,r,t, or b
    boolean under; //1 for folding under in direction
    int whichfold; //index from the left or top side of completely unfolded paper of which fold to act on
    Fold(char dir,boolean u, int which) { direction=dir; under=u; whichfold=which; }
};


//------------------------------------- FoldedPageInfo --------------------------------------
/*! \class FoldedPageInfo
 * \brief Info about partial folded states of signatures.
 */

class FoldedPageInfo
{
    int currentrow, currentcol; //where this original is currently
    int y_flipped, x_flipped; //how this one is flipped around in its current location
    int finalxflip, finalyflip;
    int finalindexfront, finalindexback;
    ArrayList pages; //what pages are actually there, r,c are pushed

    FoldedPageInfo()
    {
        x_flipped=y_flipped=0;
        currentrow=currentcol=0;

        finalxflip=finalyflip=0;
        finalindexback=finalindexfront=-1;
        
        pages=new ArrayList();
    }
};



//------------------------------------- Signature --------------------------------------
/*! \class Signature
 * \brief A folding pattern used as the basis for an SignatureImposition. 
 *
 * The steps for creating a signature are:
 *
 * 1. Initial paper inset, with optional gap between the sections\n
 * 2. Sectioning, into (for now) equal sized sections (see tilex and tiley). The sections
 *    can be stacked onto each other (pilesections==1 (unimplemented!)) or each section will use the
 *    same content (pilesections==0).
 * 3. folding per section\n
 * 4. finishing trim on the folded page size
 * 5. specify which edge of the folded up paper to use as the binding edge, if any\n
 * 6. specify a default margin area
 *
 *
 */

class Signature
{
    String name;
    String description;

    float totalwidth, totalheight;

    int sheetspersignature;
    boolean autoaddsheets; //whether you increase the num of sheets per sig, or num sigs when adding pages, 
    float insetleft, insetright, insettop, insetbottom;

    float tilegapx, tilegapy;
    int tilex, tiley; //how many partitions per sheet of paper

    float creep;
    float rotation; //in practice, not really sure how this works, 
                     //it is mentioned here and there that minor rotation is sometimes needed
                     //for some printers
    char work_and_turn; //0 for no, 1 work and turn == rotate on axis || to major dim, 2 work and tumble=rot axis || minor dim
    int pilesections; //if tiling, whether to repeat content, or continue content (ignored currently)

    int numhfolds, numvfolds;
    ArrayList folds;

    float trimleft, trimright, trimtop, trimbottom; // number<0 means don't trim that edge (for accordion styles)
    float marginleft, marginright, margintop, marginbottom;

    char up; //which direction is up 'l|r|t|b', ie 'l' means points toward the left
    char binding;    //direction to place binding 'l|r|t|b'
    char positivex;  //direction of the positive x axis: 'l|r|t|b'
    char positivey;  //direction of the positive y axis: 'l|r|t|b'

    int hint_numpages;  //used by impose-only mode to have custom behavior for number of pages and signatures
    int hint_numsigs;

     //for easy storing of final arrangement:
    FoldedPageInfo[][] foldinfo;


    Signature()
    {
        description=null;
        name=null;

        totalwidth =currentPaperWidth;
        totalheight=currentPaperHeight;

        sheetspersignature=1;
        insetleft=insetright=insettop=insetbottom=0;
        tilegapx=tilegapy=0;

        tilex=tiley=1;

        autoaddsheets=true;

        numhfolds=numvfolds=0;
        trimleft=trimright=trimtop=trimbottom=0;
        marginleft=marginright=margintop=marginbottom=0;

        up='t';         //which direction is up 'l|r|t|b', ie 'l' means points toward the left
        binding='l';    //direction to place binding 'l|r|t|b'
        positivex='r';  //direction of the positive x axis: 'l|r|t|b'
        positivey='t';  //direction of the positive x axis: 'l|r|t|b', for when up might not be positivey!

                folds=new ArrayList();
        reallocateFoldinfo();
        foldinfo[0][0].finalindexfront=0;
        foldinfo[0][0].finalindexback=1;

        hint_numpages=PagesPerSignature();
        hint_numsigs=1;

    }

    void reallocateFoldinfo() //Signature::reallocateFoldinfo
    {
        foldinfo=new FoldedPageInfo[numhfolds+2][numvfolds+2];
        int r;
        for (r=0; r<numhfolds+1; r++) {
            for (int c=0; c<numvfolds+1; c++) {
                foldinfo[r][c]=new FoldedPageInfo();
                foldinfo[r][c].pages.add(r);
                foldinfo[r][c].pages.add(c);
            }
        }
    }

    //! Flush all the foldinfo pages stacks, as if there have been no folds yet.
    /*! Please note this does not create or reallocate foldinfo. 
     * If finfo==null then use this.foldinfo.
     *
     * It is assumed that finfo is allocated properly for the number of folds.
     */
    void resetFoldinfo(FoldedPageInfo[][] finfo)
    {
        if (finfo==null) finfo=foldinfo;

        for (int r=0; r<numhfolds+1; r++) {
            for (int c=0; c<numvfolds+1; c++) {
                finfo[r][c].pages.clear();
                finfo[r][c].pages.add(r);
                finfo[r][c].pages.add(c);

                finfo[r][c].x_flipped=0;
                finfo[r][c].y_flipped=0;
            }
        }
    }

    /*! If foldlevel==0, then fill with info about when there are no folds.
     * foldlevel==1 means after the 1st fold in the folds stack, etc.
     * foldlevel==-1 means apply all the folds, and apply final page settings.
     *
     * First the fold info is reset, then each fold is applied up to foldlevel.
     * If foldlevel==0, then the result is for no folds done.
     */
    int applyFold(FoldedPageInfo[][] finfo, int foldlevel) //Signature::applyFold  
    {
        if (finfo==null) {
            if (foldinfo==null) reallocateFoldinfo();
            finfo=foldinfo;
        }

        resetFoldinfo(finfo);

        if (foldlevel<0) foldlevel=folds.size();
        Fold fold;
        for (int c=0; c<foldlevel; c++) {
            fold=(Fold)folds.get(c);
            applyFold(finfo, fold.direction, fold.whichfold, fold.under);
        }
        return 0;
    }

    //! Low level flipping across folds.
    /*! This will flip everything on one side of a fold to the other side (if possible).
     * It is not a selective flipping.
     *
     * This is called to ONLY apply the fold. It does not check and apply final index settings
     * or check for validity of the fold.
     *
     * If finfo==null, then use this.foldinfo.
     *
     * index==1 is the first fold from left or bottom.
     */
    void applyFold(FoldedPageInfo[][] finfo, char folddir, int index, boolean under) //Signature::applyFold
    {
        if (finfo==null) finfo=foldinfo;

        int newr,newc, tr,tc;
        int fr1,fr2, fc1,fc2;

         //find the cells that must be moved
        if (folddir=='l') {
            fr1=0;
            fr2=numhfolds;
            fc1=index;
            fc2=numvfolds;
        } else if (folddir=='r') {
            fr1=0;
            fr2=numhfolds;
            fc1=0;
            fc2=index-1;
        } else if (folddir=='b') {
            fr1=index;
            fr2=numhfolds;
            fc1=0;
            fc2=numvfolds;
        } else { //if (folddir=='t') {
            fr1=0;
            fr2=index-1;
            fc1=0;
            fc2=numvfolds;
        }

         //move the cells
        for (int r=fr1; r<=fr2; r++) {
          for (int c=fc1; c<=fc2; c++) {
            if (finfo[r][c].pages.size()==0) continue; //skip blank cells

             //find new positions
            if (folddir=='b') {
                newc=c;
                newr=index-(r-(index-1));
            } else if (folddir=='t') {
                newc=c;
                newr=index+(index-1-r);
            } else if (folddir=='l') {
                newr=r;
                newc=index-(c-(index-1));
            } else { //if (folddir=='r') {
                newr=r;
                newc=index+(index-1-c);
            }

             //swap old and new positions
            if (under) {
                while(finfo[r][c].pages.size()>0) {
                    tr=(Integer)finfo[r][c].pages.get(0);
                                        finfo[r][c].pages.remove(0);
                    tc=(Integer)finfo[r][c].pages.get(0);
                                        finfo[r][c].pages.remove(0);
                    finfo[newr][newc].pages.add(0,tc);
                    finfo[newr][newc].pages.add(0,tr);

                     //flip the original place.
                    if (folddir=='b' || folddir=='t') finfo[tr][tc].y_flipped=(finfo[tr][tc].y_flipped==1?0:1);
                    else finfo[tr][tc].x_flipped=(finfo[tr][tc].x_flipped==1?0:1);
                }
            } else {
                while(finfo[r][c].pages.size()>0) {
                    tc=(Integer)finfo[r][c].pages.get(finfo[r][c].pages.size()-1);
                                        finfo[r][c].pages.remove(finfo[r][c].pages.size()-1);
                    tr=(Integer)finfo[r][c].pages.get(finfo[r][c].pages.size()-1);
                                        finfo[r][c].pages.remove(finfo[r][c].pages.size()-1);
                    finfo[newr][newc].pages.add(tr);
                    finfo[newr][newc].pages.add(tc);

                     //flip the original place.
                    if (folddir=='b' || folddir=='t') finfo[tr][tc].y_flipped=(finfo[tr][tc].y_flipped==1?0:1);
                    else finfo[tr][tc].x_flipped=(finfo[tr][tc].x_flipped==1?0:1);
                }
            }
          }
        }
    }

    //! Check if the signature is totally folded or not.
    /*! If we are totally folded, then apply the page indices to finfo.
     *
     * Returns int[3].
     * i[0]=1 if we are totally folded. Otherwise 0. If we are totally folded, then
     * return the final position in finalrow,finalcol. Put -1 in each if we are not totally folded.
     *
     * i[1]=finalrow, i[2]=finalcol.
     *
     * If finfo==null, then use this.foldinfo.
     */
    int[] checkFoldLevel(FoldedPageInfo[][] finfo)
    {
        if (finfo==null) finfo=foldinfo;

         //check the immediate neighors of the first cell with pages.
         //If there are no neighbors, then we are totally folded.

         //find a non blank cell
        int newr=0,newc=0;
        for (newr=0; newr<=numhfolds; newr++) {
          for (newc=0; newc<=numvfolds; newc++) {
            if (finfo[newr][newc].pages.size()!=0) break;
          }
          if (newc!=numvfolds+1) break;
        }

        int hasfinal=0;
        int stillmore=4;
        int tr,tc;

         //check above
        tr=newr-1; tc=newc;
        if (tr<0 || finfo[tr][tc].pages.size()==0) stillmore--;

         //check below
        tr=newr+1; tc=newc;
        if (tr>numhfolds || finfo[tr][tc].pages.size()==0) stillmore--;
        
         //check left
        tr=newr; tc=newc-1;
        if (tc<0 || finfo[tr][tc].pages.size()==0) stillmore--;

         //check if right
        tr=newr; tc=newc+1;
        if (tc>numvfolds || finfo[tr][tc].pages.size()==0) stillmore--;

        if (stillmore==0) {
             //apply final flip values
            int finalr=newr;
            int finalc=newc;

            int page=0;
                        boolean xflip,yflip;
            for (int c=finfo[finalr][finalc].pages.size()-2; c>=0; c-=2) {
                tr=(Integer)finfo[finalr][finalc].pages.get(c);
                tc=(Integer)finfo[finalr][finalc].pages.get(c+1);

                xflip=(finfo[tr][tc].x_flipped==1);
                yflip=(finfo[tr][tc].y_flipped==1);
                finfo[tr][tc].finalyflip=finfo[tr][tc].y_flipped;
                finfo[tr][tc].finalxflip=finfo[tr][tc].x_flipped;

                if ((xflip && !yflip) || (!xflip && yflip)) {
                     //back side of paper is up
                    finfo[tr][tc].finalindexback=page;
                    finfo[tr][tc].finalindexfront=page+1;
                } else {
                    finfo[tr][tc].finalindexback=page+1;
                    finfo[tr][tc].finalindexfront=page;
                }
                page+=2;
            }
            hasfinal=1;
        } else hasfinal=0;

        int i[]={hasfinal, (hasfinal==1 ? newr : -1), (hasfinal==1 ? newc : -1)};
        return i;
    }

    int IsVertical()
    {
        if  ((up=='t' || up=='b') && (binding=='t' || binding=='b')) return 1;
        if  ((up=='l' || up=='r') && (binding=='r' || binding=='l')) return 1;
        return 0;
    }

    float PatternHeight()
    {
        float h=currentPaperHeight;
        if (tiley>1) h-=(tiley-1)*tilegapy;
        h-=insettop+insetbottom;
        return h/tiley;
    }

    float PatternWidth()
    {
        float w=currentPaperWidth;
        if (tilex>1) w-=(tilex-1)*tilegapx;
        w-=insetleft+insetright;
        return w/tilex;
    }

    float PageHeight(int part)
    {
        if (part==0) return PatternHeight()/(numhfolds+1);
        return PatternHeight()/(numhfolds+1) - trimtop - trimbottom;
    }

    float PageWidth(int part)
    {
        if (part==0) return PatternWidth()/(numvfolds+1);
        return PatternWidth()/(numvfolds+1) - trimleft - trimright;
    }

    //! Return the bounds for various parts of a final folded page.
    /*! If part==0, then return the bounds of a trimmed cell. This has minumums of 0,
     * and maximums of PageWidth(1),PageHeight(1).
     *
     * If part==1, then return the margin area, as it would sit in a region defined by part==0.
     *
     * If part==2, then return the whole page cell, as it would sit around a region defined by part==0.
     *
     * If bbox!=null, then set in that. If bbox==null, then return a new DoubleBBox.
     */
    DoubleBBox PageBounds(int part, DoubleBBox bbox)
    {
        if (bbox==null) bbox=new DoubleBBox();

        if (part==0) { //trim box
            bbox.minx=bbox.miny=0;
            bbox.maxx=PageWidth(1);
            bbox.maxy=PageHeight(1);

        } else if (part==1) { //margin box
            bbox.minx=marginleft-trimleft;
            bbox.miny=marginbottom-trimbottom;
            bbox.maxx=PageWidth(0)  - marginright - trimleft;
            bbox.maxy=PageHeight(0) - margintop - trimbottom;

        } else { //page cell box
            bbox.minx=-trimleft;
            bbox.miny=-trimbottom;
            bbox.maxx=-trimleft+PageWidth(0);
            bbox.maxy=-trimbottom+PageHeight(0);
        }

        return bbox;
    }


    /*! Say a pattern is 4x3 cells, then 2*(3*4)=24 is returned.
     */
    int PagesPerPattern()
    {
        return 2*(numvfolds+1)*(numhfolds+1);
    }

    //! Taking into account sheetspersignature, return the number of pages that can be arranged in one signature.
    /*! So say you have a pattern with 2 sheets folded together. Then 2*PagesPerPattern() is returned.
     *
     * It is assumed that sheetspersignature reflects a desired number of sheets in the signature,
     * whether or not autoaddsheets==true.
     *
     * Please note this is only the number for ONE tile.
     */
    int PagesPerSignature()
    {
        return (sheetspersignature>0?sheetspersignature:1)*PagesPerPattern();
    }

    //! With the final trimmed page size (w,h), set the paper size to the proper size to just contain it.
    void SetPaperFromFinalSize(float w,float h)
    {
         //find cell dims
        w+=trimleft+trimright;
        h+=trimtop +trimbottom;

         //find pattern dims
        w*=(numvfolds+1);
        h*=(numhfolds+1);

         //find whole dims
        w=w*tilex + (tilex-1)*tilegapx + insetleft + insetright;
        h=h*tiley + (tiley-1)*tilegapy + insettop  + insetbottom;

        SetPaper(w,h);
        ChangePaper((BuiltinPaperSizes.length/4), w,h,currentPaperUnits);
    }


    //! Set the size of the signature (totalwidth,height) to this.
    int SetPaper(float w, float h)
    {
        totalheight=h;
        totalwidth =w;

        return 0;
    }

    //! Return which paper spread contains the given document page number.
    /*! 
     * Note that this row,col is for a paper spread, and by convention the backside of a sheet of
     * paper is flipped left to right in relation to the front side.
     *
     * returns int[3] with:
     * i[0]==paper number,
     * i[1]==row,
     * i[2]==column
     */
    int[] locatePaperFromPage(int pagenumber)
    {
        int whichsig=pagenumber/PagesPerSignature();//takes into account sheetspersignature
        int pageindex=pagenumber%PagesPerSignature();//page index within the signature
        int pagespercell=2*(sheetspersignature>0?sheetspersignature:1); //total pages per cell
        int sigindex      =pageindex/(pagespercell/2);//page index assuming a single page in signature
        int sigindexoffset=pageindex%(pagespercell/2);//index within cell of the page

         //sigindex is the same as pageindex when there is only 1 sheet per signature.
         //Now we need to find which side of the paper sigindex is on, then map that if necessary
         //to the right piece of paper.
         //
         //To do this, we find where it is in the pattern...

        int front;  //Whether sigindex is on top or bottom of unfolded pattern
        boolean countdir=false; //Whether a pattern cell has a higher page number on top (1) or not (0).
        int rr=-1, cc=-1;
        for (rr=0; rr<numhfolds+1; rr++) {
          for (cc=0; cc<numvfolds+1; cc++) {
            if (sigindex==foldinfo[rr][cc].finalindexfront) {
                front=1;
                countdir=(foldinfo[rr][cc].finalindexfront>foldinfo[rr][cc].finalindexback);
                break;
            } else if (sigindex==foldinfo[rr][cc].finalindexback) {
                front=0;
                countdir=(foldinfo[rr][cc].finalindexfront>foldinfo[rr][cc].finalindexback);
                break;
            }
          } //cc
          if (cc!=numvfolds+1) break;
        }  //rr


         //now rr,cc is the cell that contains sigindex.
         //We must figure out how it maps to pieces of paper
        int papernumber;
        if (countdir) papernumber=pagespercell-1-sigindexoffset;
        else papernumber=sigindexoffset;

        int[] ret=new int[3];
        ret[0]=whichsig*PagesPerSignature() + papernumber; //which paper
        ret[1]=rr; //row
        ret[2]=((sigindexoffset%1==1) ? (numvfolds-cc) : cc); //column

        return ret;
    }

    //! Return something like "2 fold, 8 page signature".
    String BriefDescription()
    {
        if (description!=null) return description;

        String desc="";
        if (tilex>1 || tiley>1) 
            desc= folds.size()+", "+PagesPerPattern()
                +" page signature, tiled "+tilex+" x "+tiley;
        else desc= folds.size()+" fold, "+PagesPerPattern()+" page signature";

        return desc;
    }

}; //class Signature



//------------------------------------- SignatureInterface enum -----------------------------

//size of the fold indicators on left of screen
float INDICATOR_SIZE=10;



int SP_None = 0;

int SP_Tile_X_top = 1;
int SP_Tile_X_bottom = 2;
int SP_Tile_Y_left = 3;
int SP_Tile_Y_right = 4;
int SP_Tile_Gap_X = 5;
int SP_Tile_Gap_Y = 6;

int SP_Inset_Top = 7;
int SP_Inset_Bottom = 8;
int SP_Inset_Left = 9;
int SP_Inset_Right = 10;

int SP_H_Folds_left = 11;
int SP_H_Folds_right = 12;
int SP_V_Folds_top = 13;
int SP_V_Folds_bottom = 14;

int SP_Trim_Top = 15;
int SP_Trim_Bottom = 16;
int SP_Trim_Left = 17;
int SP_Trim_Right = 18;

int SP_Margin_Top = 19;
int SP_Margin_Bottom = 20;
int SP_Margin_Left = 21;
int SP_Margin_Right = 22;

int SP_Binding = 23;

int SP_Sheets_Per_Sig = 24;
int SP_Num_Pages      = 25;
int SP_Num_Sigs       = 26;

int SP_Paper_Name    = 27;
int SP_Paper_Orient  = 28;
int SP_Paper_Width   = 29;
int SP_Paper_Height  = 30;

int SP_Current_Sheet = 31;
int SP_Front_Or_Back = 32;

int SP_Save            = 34;
int SP_ZoomControls    = 35;
int SP_ZoomOutControls = 36;
int SP_ZoomIn          = 37;
int SP_ZoomOut         = 38;



 //first index of which fold we are on
int SP_FOLDS = 100;



//------------------------------------- SignatureInterface --------------------------------------
    

class SignatureInterface
{
    Displayer dp;
        
    int folderoffset;
    boolean firsttime;
    boolean showdecs;
    boolean showsplash;
    boolean showthumbs;
    ArrayList controls;
    int buttonheight;

    color color_inset, color_margin, color_trim, color_binding;
    color color_status;
        color color_bg, color_fg, color_button;
    int insetmask, trimmask, marginmask;

    int lbdown_row, lbdown_col;
        int mslidex,mslidey;

     //to keep track of current partial fold:
    int foldr1, foldc1, foldr2, foldc2;
    int folddirection;
    boolean foldunder;
    int foldindex;
    float foldprogress;

    int finalr,finalc; //cell location of totally folded pages
    boolean hasfinal; //whether the pattern has been totally folded yet or not

    int activetilex,activetiley;
    int onoverlay;  //nonzero if mouse clicked down on and is over an overlay
    int overoverlay; //nonzero if mouse is over an overlay
    float arrowscale;

    int currentPaperSpread;

    int foldlevel; //how hany folds are actively displayed
    FoldedPageInfo[][] ifoldinfo;


    Signature signature;

    SignatureInterface()
    {
        folderoffset=0;
        
        dp=new Displayer();
        int buttonheight=int((textAscent()+textDescent())*1.2);

        currentPaperSpread=0;
                
        firsttime=true;
        showdecs=true;
        showthumbs=true;
        showsplash=true;

        insetmask=15;
        trimmask=15;
        marginmask=15;

        signature=new Signature();
        signature.SetPaper(currentPaperWidth,currentPaperHeight);

        foldlevel=0; //how many of the folds are active in display. must be < sig.folds.n
        hasfinal=false;
        ifoldinfo=signature.foldinfo;
        checkFoldLevel();
        signature.resetFoldinfo(null);

        foldr1=foldc1=foldr2=foldc2=-1;
        folddirection=0;
        lbdown_row=lbdown_col=-1;
        mslidex=mslidey=0;

        color_bg     =color(200,200,200);
        color_fg     =color(red(fg_color),green(fg_color),blue(fg_color));
        color_button =color(220,220,220);
        color_inset  =color(128,  0,  0);
        color_margin =color(192,192,192);
        color_trim   =color(255,  0,  0);
        color_binding=color(  0,255,  0);
        color_status =color(0,0,0);

        onoverlay=0;
        overoverlay=0;
        arrowscale=1;
        activetilex=activetiley=0;

        DefineShortcuts();
        createHandles();
    }
    
    ArrayList sc;

    void DefineShortcuts()
    {
        if (sc!=null) return;

        sc=new ArrayList();

        sc.add(new Shortcut(SIA_Decorations,     'd',0,          "Decorations",    ("Toggle decorations")));
        sc.add(new Shortcut(SIA_Thumbs,          'p',0,          "Thumbs",         ("Toggle showing of page thumbnails")));
        sc.add(new Shortcut(SIA_Center,          ' ',0,          "Center",         ("Center view")));
        sc.add(new Shortcut(SIA_InsetMask,       'i',ControlMask,"InsetMask",      ("Toggle which inset to change")));
        sc.add(new Shortcut(SIA_InsetInc,        'i',0,          "InsetInc",       ("Increment inset")));
        sc.add(new Shortcut(SIA_InsetDec,        'I',ShiftMask,  "InsetDec",       ("Decrement inset")));
        sc.add(new Shortcut(SIA_GapInc,          'g',0,          "GapInc",         ("Increment gap")));
        sc.add(new Shortcut(SIA_GapDec,          'G',ShiftMask,  "GapDec",         ("Decrement gap")));
        sc.add(new Shortcut(SIA_TileXInc,        'x',0,          "TileXInc",       ("Increase number of tiles horizontally")));
        sc.add(new Shortcut(SIA_TileXDec,        'X',ShiftMask,  "TileXDec",       ("Decrease number of tiles horizontally")));
        sc.add(new Shortcut(SIA_TileYInc,        'y',0,          "TileYInc",       ("Increase number of tiles vertically")));
        sc.add(new Shortcut(SIA_TileYDec,        'Y',ShiftMask,  "TileYDec",       ("Decrease number of tiles vertically")));
        sc.add(new Shortcut(SIA_NumFoldsVInc,    'v',0,          "NumFoldsVInc",   ("Increase number of vertical folds")));
        sc.add(new Shortcut(SIA_NumFoldsVDec,    'V',ShiftMask,  "NumFoldsVDec",   ("Decrease number of vertical folds")));
        sc.add(new Shortcut(SIA_NumFoldsHInc,    'h',0,          "NumFoldsHInc",   ("Increase number of horizontal folds")));
        sc.add(new Shortcut(SIA_NumFoldsHDec,    'H',ShiftMask,  "NumFoldsHDec",   ("Decrease number of horizontal folds")));
        sc.add(new Shortcut(SIA_BindingEdge,     'b',0,          "BindingEdge",    ("Next binding edge")));
        sc.add(new Shortcut(SIA_BindingEdgeR,    'B',ShiftMask,  "BindingEdgeR",   ("Previous binding edge")));
        sc.add(new Shortcut(SIA_PageOrientation, 'o',0,          "PageOrientation",("Change page orientation")));
        sc.add(new Shortcut(SIA_TrimMask,        't',ControlMask,"TrimMask",       ("Toggle which trim to change")));
        sc.add(new Shortcut(SIA_TrimInc,         't',0,          "TrimInc",        ("Increase trim value")));
        sc.add(new Shortcut(SIA_TrimDec,         'T',ShiftMask,  "TrimDec",        ("Decrease trim value")));
        sc.add(new Shortcut(SIA_MarginMask,      'm',ControlMask,"MarginMask",     ("Toggle which margin to change")));
        sc.add(new Shortcut(SIA_MarginInc,       'm',0,          "MarginInc",      ("Increase margin value")));
        sc.add(new Shortcut(SIA_MarginDec,       'M',ShiftMask,  "MarginDec",      ("Decrease margin value")));

        sc.add(new Shortcut(SIA_ZoomIn,          '+',0        ,  "ZoomIn",         ("Zoom in")));
        sc.add(new Shortcut(SIA_ZoomIn,          '=',0        ,  "ZoomIn",         ("Zoom in")));
        sc.add(new Shortcut(SIA_ZoomOut,         '-',0        ,  "ZoomOut",        ("Zoom out")));
        sc.add(new Shortcut(SIA_ZoomInControls,  'z',0        ,  "ZoomInControls", ("Zoom in controls")));
        sc.add(new Shortcut(SIA_ZoomOutControls, 'Z',ShiftMask,  "ZoomOutControls",("Zoom out controls")));

    }


    ActionArea control(int what)
    {
        for (int c=0; c<controls.size(); c++) if (what==((ActionArea)controls.get(c)).action) return (ActionArea)controls.get(c);
        return null;
    }

    /*! i<999 is select that paper index. else SIGM_FinalFromPaper, SIGM_Landscape, or  SIGM_Portrait.
     */
    int MenuEvent(int i)
    {
        if (i==SIGM_FinalFromPaper) {
            signature.SetPaperFromFinalSize(currentPaperWidth, currentPaperHeight);
            remapHandles(0);
            needtodraw=1;

            return 0;

        } else if (i==SIGM_Landscape) {
            if (currentOrientation==1) return 0;
            currentOrientation=1;

            float t=currentPaperWidth;
            currentPaperWidth=currentPaperHeight;
            currentPaperWidth=t;

            signature.SetPaper(currentPaperWidth,currentPaperHeight);
            return 0;

        } else if (i==SIGM_Portrait) {
            if (currentOrientation==0) return 0;
            currentOrientation=0;

            float t=currentPaperWidth;
            currentPaperWidth=currentPaperHeight;
            currentPaperWidth=t;

            signature.SetPaper(currentPaperWidth,currentPaperHeight);
            return 0;

        } else if (i<999) {
             //selecting new paper size
            ChangePaper(i, currentPaperWidth,currentPaperHeight,currentPaperUnits);
            signature.SetPaper(currentPaperWidth,currentPaperHeight);
            remapHandles(0);
            needtodraw=1;
            return 0;
        }
        return 1;
    }

    //! Reallocate foldinfo, usually after adding fold lines.
    /*! this will flush any folds stored in the signature.
     */
    void reallocateFoldinfo()
    {
        signature.folds.clear();
        signature.reallocateFoldinfo();
        ifoldinfo=signature.foldinfo;
        hasfinal=false;

        String str;
        str="Base holds "+(2*(signature.numvfolds+1)*(signature.numhfolds+1))+" pages.";
        PostMessage(str);
    }

    void applyFold(char folddir, int index, boolean under)
    {
        signature.applyFold(ifoldinfo, folddir,index,under);
    }

    void applyFold(Fold fold)
    {
        if (fold==null) return;
        applyFold(fold.direction, fold.whichfold, fold.under);
    }


    //! Check if the signature is totally folded or not.
    /*! Remember that if there are no fold lines, then we need to be hasfinal==1 for
     * totally folded, letting us set margin, final trim, and binding.
     *
     * If update!=0, then if the pattern is not totally folded, then make hasfinal=0,
     * make sure binding and updirection is applied to foldinfo.
     *
     * Returns foldlevel.
     */
    int checkFoldLevel()
    {
        int[] i=signature.checkFoldLevel(ifoldinfo);
        hasfinal=(i[0]==1);
        finalr=i[1];
        finalc=i[2];
        return foldlevel;
    }

    boolean OnBack() { return currentPaperSpread%2==1; }
    
    void Refresh()
    {
        if (needtodraw==0) return;
        if (firsttime) {
            remapHandles(0);
            PerformAction(SIA_Center);
            firsttime=false;
        }

        needtodraw=0;

        background(color_bg);
                
        float patternheight=signature.PatternHeight();
        float patternwidth =signature.PatternWidth();

        float x,y,w,h;
        String str;

         //----------------draw whole outline
        dp.NewFG(1.,0.,1.); //purple for paper outline, like custom papergroup border color
        w=signature.totalwidth;
        h=signature.totalheight;
        dp.StrokeWidth(1);
        dp.drawline(0,0, w,0);
        dp.drawline(w,0, w,h);
        dp.drawline(w,h, 0,h);
        dp.drawline(0,h, 0,0);
        
         //----------------draw inset
        dp.NewFG(color_inset); //dark red for inset
        if (signature.insetleft!=0) dp.drawline(signature.insetleft,0, signature.insetleft,h);
        if (signature.insetright!=0) dp.drawline(w-signature.insetright,0, w-signature.insetright,h);
        if (signature.insettop!=0) dp.drawline(0,h-signature.insettop, w,h-signature.insettop);
        if (signature.insetbottom!=0) dp.drawline(0,signature.insetbottom, w,signature.insetbottom);


         //------------------draw fold pattern in each tile
        float ew=patternwidth/(signature.numvfolds+1);
        float eh=patternheight/(signature.numhfolds+1);
        w=patternwidth;
        h=patternheight;

        flatpoint[] pts=new flatpoint[4];
        flatpoint fp;
        //int facedown=0;
        int hasface;
        int rrr,ccc;
        int urr,ucc;
        int ff,tt;
        float xx,yy;
        //int xflip;
        int yflip;

        x=signature.insetleft;
        for (int tx=0; tx<signature.tilex; tx++) {
          y=signature.insetbottom;
          for (int ty=0; ty<signature.tiley; ty++) {

             //fill in light gray for elements with no current faces
             //or draw orientation arrow and number for existing faces
            for (int rr=0; rr<signature.numhfolds+1; rr++) {
              for (int cc=0; cc<signature.numvfolds+1; cc++) {
                hasface=ifoldinfo[rr][cc].pages.size();

                if (foldlevel==0 && currentPaperSpread%2==1) {
                    urr=rr;
                    ucc=signature.numvfolds-cc;
                } else {
                    urr=rr;
                    ucc=cc;
                }

                 //first draw filled face, grayed if no current faces
                dp.StrokeWidth(1);
                pts[0]=new flatpoint(x+ucc*ew,y+urr*eh);
                pts[1]=new flatpoint(pts[0].x+ew, pts[0].y);
                pts[2]=new flatpoint(pts[0].x+ew, pts[0].y+eh);
                pts[3]=new flatpoint(pts[0].x   , pts[0].y+eh);;

                if (hasface>0) dp.NewFG(1.,1.,1.);
                else dp.NewFG(.75,.75,.75);

                dp.drawlines(pts,1,1);

                //if (hasface && foldlevel==0) {
                if (hasface>0) {
                    rrr=(Integer)ifoldinfo[rr][cc].pages.get(ifoldinfo[rr][cc].pages.size()-2);
                    ccc=(Integer)ifoldinfo[rr][cc].pages.get(ifoldinfo[rr][cc].pages.size()-1);

                    if (ifoldinfo[rr][cc].finalindexfront>=0) {
                         //there are faces in this spot, draw arrow and page number
                        dp.NewFG(.5,.5,.5);
                        if (hasfinal) {
                            //xflip=foldinfo[rrr][ccc].x_flipped^foldinfo[rrr][ccc].finalxflip;
                            yflip=ifoldinfo[rrr][ccc].y_flipped^ifoldinfo[rrr][ccc].finalyflip;
                        } else {
                            //xflip=foldinfo[rrr][ccc].x_flipped;
                            yflip=ifoldinfo[rrr][ccc].y_flipped;
                        }


                         //compute range of pages for this cell
                        ff=ifoldinfo[rrr][ccc].finalindexfront;
                        tt=ifoldinfo[rrr][ccc].finalindexback;
                        if (true) { //!signature.autoaddsheets) {
                            if (ff>tt) {
                                tt*=signature.sheetspersignature;
                                ff=tt+2*signature.sheetspersignature-1;
                            } else {
                                ff*=signature.sheetspersignature;
                                tt=ff+2*signature.sheetspersignature-1;
                            }
                        }

                         //show thumbnails
                        if (showthumbs) {
                            //todo!
                        }

                        dp.StrokeWidth(1);

                        pts[0]=new flatpoint(x+(ucc+.5)*ew,y+(urr+.4+.2*(yflip==1?1:0))*eh);
                        dp.drawarrow(pts[0],new flatpoint(pts[0].x,pts[0].y + (yflip==1?-1:1)*eh/3));
                        fp=pts[0];
                        fp.y-=(yflip==1?-1:1)*(textAscent()+textDescent())/dp.Getmag()/2;

                         if (foldlevel==0) {
                           //text out range of pages at bottom of arrow
                           //str=str(ff+1)+"-"+str(tt+1)+": "; //complete range
                           if (ff>tt) str=""+(ff-currentPaperSpread+1);
                           else str=""+(ff+currentPaperSpread+1);
                           dp.textout(fp.x,fp.y, str, LAX_CENTER);
                         }
                    }
                }

                 //draw markings for final page binding edge, up, trim, margin
                if (hasfinal && foldlevel==signature.folds.size() && rr==finalr && cc==finalc) {
                    dp.StrokeWidth(2);

                    xx=x+ucc*ew;
                    yy=y+urr*eh;

                     //draw gray margin edge
                    dp.NewFG(color_margin);
                    dp.drawline(xx,yy+signature.marginbottom,   xx+ew,yy+signature.marginbottom);            
                    dp.drawline(xx,yy+eh-signature.margintop,   xx+ew,yy+eh-signature.margintop);            
                    dp.drawline(xx+signature.marginleft,yy,     xx+signature.marginleft,yy+eh);            
                    dp.drawline(xx+ew-signature.marginright,yy, xx+ew-signature.marginright,yy+eh);            

                     //draw red trim edge
                    dp.StrokeWidth(1);
                    dp.NewFG(color_trim);
                    if (signature.trimbottom>0) dp.drawline(xx,yy+signature.trimbottom, xx+ew,yy+signature.trimbottom);            
                    if (signature.trimtop>0)    dp.drawline(xx,yy+eh-signature.trimtop, xx+ew,yy+eh-signature.trimtop);            
                    if (signature.trimleft>0)   dp.drawline(xx+signature.trimleft,yy, xx+signature.trimleft,yy+eh);            
                    if (signature.trimright>0)  dp.drawline(xx+ew-signature.trimright,yy, xx+ew-signature.trimright,yy+eh);            

                     //draw green binding edge
                    dp.StrokeWidth(overoverlay==SP_Binding?5:2);
                    dp.NewFG(color_binding);

                     //todo: draw a solid line, but a dashed line toward the inner part of the page...??
                    int b=signature.binding;
                    float iin=ew*.05;
                    if (b=='l')      dp.drawline(xx+iin,yy,    xx+iin,yy+eh);
                    else if (b=='r') dp.drawline(xx-iin+ew,yy, xx-iin+ew,yy+eh);
                    else if (b=='t') dp.drawline(xx,yy-iin+eh, xx+ew,yy-iin+eh);
                    else if (b=='b') dp.drawline(xx,yy+iin,    xx+ew,yy+iin);

                } //final page markings
              } //cc
            }  //rr

             //draw fold pattern outline
            dp.NewFG(1.,0.,0.);
            dp.StrokeWidth(1);
            dp.drawline(x,    y, x+w,  y);
            dp.drawline(x+w,  y, x+w,y+h);
            dp.drawline(x+w,y+h, x  ,y+h);
            dp.drawline(x,  y+h, x,y);

             //draw all fold lines
            dp.NewFG(.5,.5,.5);
            dp.StrokeWidth(1);
            for (int c=0; c<signature.numvfolds; c++) { //verticals
                dp.drawline_dashed(x+(c+1)*ew,y, x+(c+1)*ew,y+h);
            }

            for (int c=0; c<signature.numhfolds; c++) { //horizontals
                dp.drawline_dashed(x,y+(c+1)*eh, x+w,y+(c+1)*eh);
            }
            
            y+=patternheight+signature.tilegapy;
          } //tx
          x+=patternwidth+signature.tilegapx;
        } //ty

         //draw in progress folding
        int device=0;
        
        if (mouseButton==LEFT && folddirection!=0 && folddirection!='x') {
             //this will draw a light gray tilting region across foldindex, in folddirection, with foldunder.
             //it will correspond to foldr1,foldr2, and foldc1,foldc2.


            //flatpoint p=dp.screentoreal(mx,my);
            flatpoint dir=new flatpoint();
            if (folddirection=='t') dir.y=1;
            else if (folddirection=='b') dir.y=-1;
            else if (folddirection=='l') dir.x=-1;
            else if (folddirection=='r') dir.x=1;

            dp.StrokeWidth(1);


             //draw partially folded region foldr1..foldr2, foldc1..foldc2
            //float ew=patternwidth/(signature.numvfolds+1);
            //float eh=patternheight/(signature.numhfolds+1);
            w=ew*(foldc2-foldc1+1);
            h=eh*(foldr2-foldr1+1);

            float rotation=foldprogress*PI;
            if (folddirection=='r' || folddirection=='t') rotation=PI-rotation;
            flatpoint axis;
            if (folddirection=='l' || folddirection=='r')
                axis=rotate(new flatpoint(1,0),rotation,false);
            else axis=rotate(new flatpoint(0,1),-rotation,false);

            if (foldunder) {
                dp.NewFG(.2,.2,.2);
                dp.StrokeWidth(2);
                if (folddirection=='l' || folddirection=='r') axis.y=-axis.y;
                else axis.x=-axis.x;
            } else {
                dp.NewFG(.9,.9,.9);
                dp.StrokeWidth(1);
            }

            x=signature.insetleft;
            pts=new flatpoint[4];
            for (int tx=0; tx<signature.tilex; tx++) {
              y=signature.insetbottom;
              for (int ty=0; ty<signature.tiley; ty++) {

                if (folddirection=='l' || folddirection=='r') { //horizontal fold
                    pts[0]=new flatpoint(x+foldindex*ew,y+eh*foldr1);
                    pts[1]=new flatpoint(pts[0].x+axis.x*w, pts[0].y+axis.y*w);
                    pts[2]=new flatpoint(pts[1].x, pts[1].y+h);
                    pts[3]=new flatpoint(pts[0].x, pts[0].y+h);
                } else {                             //vertical fold
                    pts[0]=new flatpoint(x+ew*foldc1,y+foldindex*eh);
                    pts[1]=new flatpoint(pts[0].x+axis.x*h,  pts[0].y+axis.y*h);
                    pts[2]=new flatpoint(pts[1].x+w,pts[1].y);
                    pts[3]=new flatpoint(pts[0].x+w,pts[0].y);
                }
                if (foldunder) dp.drawlines_dashed(pts,0);
                else dp.drawlines(pts,1,1);
                
                y+=patternheight+signature.tilegapy;
              }
              x+=patternwidth+signature.tilegapx;
            }
        }


         //draw fold indicator overlays on left side of screen
        int thing;
        dp.StrokeWidth(1);
        dp.DrawScreen();
        for (int c=signature.folds.size()-1; c>=-1; c--) {
            if (c==-1) thing=THING_Circle;
            else if (c==signature.folds.size()-1 && hasfinal) thing=THING_Square;
            else thing=THING_Triangle_Down;

            float[] ret;
            ret=getFoldIndicatorPos(c);
            x=ret[0];
            y=ret[1];
            w=ret[2];
            h=ret[3];

             //color hightlighted to show which fold we are currently on
            if (foldlevel==c+1) dp.NewFG(1.,.5,1.);
            else dp.NewFG(1.,1.,1.);

            dp.drawthing(x+w/2,y+h/2, w/2,h/2, 1, thing); //filled
            dp.NewFG(1.,0.,1.);
            dp.drawthing(x+w/2,y+h/2, w/2,h/2, 0, thing); //outline
        }
        dp.DrawReal();


         //write out final page dimensions
        dp.NewFG(0.,0.,0.);
        str="Paper: "+currentPaperWidth+" x "+currentPaperHeight+" "+currentPaperUnits
                      +",  Final size: "+(round(100*signature.PageWidth(1))/100.)+" x "+(round(100*signature.PageHeight(1))/100.) +" "+ currentPaperUnits;
        dp.DrawScreen();
        dp.textout(0,buttonheight*3, str, LAX_LEFT|LAX_TOP);
        dp.DrawReal();



         //-----------------draw control handles
        ActionArea area;
        float d;
        Signature s=signature;
        flatpoint dv=new flatpoint();
        for (int c=controls.size()-1; c>=0; c--) {
            area=(ActionArea)controls.get(c);
            if (area.hidden) continue;

            if (area.action==SP_Sheets_Per_Sig) {
                 //draw sheet per signature indicator
                dp.StrokeWidth(1);
                dp.NewFG(.5,0.,0.); //dark red for inset
                y=area.maxy;
                dp.drawline(0,y, signature.totalwidth,y);
                y-=signature.totalheight*.01;
                for (int c2=1; c2<5 && c2<signature.sheetspersignature; c2++) {
                    dp.drawline(0,y, signature.totalwidth,y);
                    y-=signature.totalheight*.01;
                }
                if (signature.autoaddsheets) str="Many sheets in a single signature";
                else if (signature.sheetspersignature==1) str="1 sheet per signature";
                else str= ""+(signature.sheetspersignature)+"sheets per signature";
                pts[0]=new flatpoint(signature.totalwidth/2,y);
                dp.textout(pts[0].x,pts[0].y, str, LAX_HCENTER|LAX_TOP);

            } else {
                if (area.action==SP_Tile_Gap_X) {
                     //draw thick line for gap
                    if (overoverlay==SP_Tile_Gap_X) {
                        dp.StrokeWidth(5);
                		dp.NewFG(.5,0.,0.); //dark red for inset
                        for (int c2=0; c2<signature.tilex-1; c2++) {
                            d=s.insetleft+(c2+1)*(s.tilegapx+s.PatternWidth()) - s.tilegapx/2;
                            dp.drawline(d,0, d,s.totalheight);
                        }
                    }

                } else if (area.action==SP_Tile_Gap_Y) {
                     //draw thick line for gap
                    if (overoverlay==SP_Tile_Gap_Y) {
                        dp.StrokeWidth(5);
                		dp.NewFG(.5,0.,0.); //dark red for inset
                        for (int c2=0; c2<signature.tiley-1; c2++) {
                            d=s.insetbottom+(c2+1)*(s.tilegapy+s.PatternHeight()) - s.tilegapy/2;
                            dp.drawline(0,d, s.totalwidth,d);
                        }
                    }

                } else if (area.visible!=0) {
                     //default handle drawing (well, with special exception for page specific stuff)
                    dp.StrokeWidth(1);
                    dv.x=dv.y=0;
                    if (area.category==2) { //page specific
                        dv.x=s.insetleft  +activetilex*(s.PatternWidth() +s.tilegapx) + finalc*s.PageWidth(0);
                        dv.y=s.insetbottom+activetiley*(s.PatternHeight()+s.tilegapy) + finalr*s.PageHeight(0);
                    }
                    drawHandle(area,dv);
                } else {
                    //println("skipping "+area.tip);
                }

            }
        }
        dp.DrawReal();

        if (showsplash) {
                        
            dp.DrawScreen();
            
            dp.NewFG(lerpColor(color_bg,color(255,255,255),.2));
            rect(0,DISPLAYHEIGHT/2-5*dp.textheight(), DISPLAYWIDTH,10*dp.textheight());
                        
            dp.NewFG(0,0,0);
            String scratch=  "Laidout Paper Folder\n"
                            +"version 0.0000000001\n"
							+"By Tom Lechner, 2013\n"
                            +"-- web version using Processing --\n"
                            +"This thing is adapted from\n"
                            +"Laidout, open source layout software.\n"
                            +"laidout.org  --  tomlechner.com"
                        ;
            dp.textout(DISPLAYWIDTH/2,DISPLAYHEIGHT/2,
                    scratch, 
                    LAX_CENTER);
            dp.DrawReal();
        }

        if (statusmessage!=null && statusmessage!="" && statusmessage!=" ") {
            dp.NewFG(color_button);
            float th=textAscent()+textDescent();
            float tw=textWidth(statusmessage)+th;
            rect(DISPLAYWIDTH/2-tw/2, DISPLAYHEIGHT-th, tw,th);
                        
            dp.NewFG(color_status);
            dp.DrawScreen();
            dp.textout(DISPLAYWIDTH/2,DISPLAYHEIGHT,  statusmessage, LAX_HCENTER|LAX_BOTTOM);
            dp.DrawReal();
        }

                // *** debugging: indicate axes
                //dp.StrokeWidth(10);
                //dp.NewFG(1.,0.,0.);
                //dp.drawline(0,0, 1,0);
                //dp.NewFG(0.,1.,0.);
                //dp.drawline(0,0, 0,1);
                //dp.StrokeWidth(1);
    } //Refresh()
    
    void drawHandle(ActionArea area, flatpoint offset) //:drawHandle
    {
        if (area.visible==0) return;
                
        //println("area "+area.tip+" at "+area.offset.x+','+area.offset.y);
                
        if (area.real!=0) dp.DrawReal(); else dp.DrawScreen();
        dp.NewFG(area.col);
        dp.PushAxes();
        if (area.real!=0) {
           dp.ShiftReal(offset.x,offset.y);
           dp.ShiftReal(area.offset.x,area.offset.y);
        } else {
                  //dp.ShiftScreen(area.offset.x,area.offset.y);
        }
        
        if (area.type==AREA_V_Slider) {
           if (area.action==overoverlay) {
             dp.DrawScreen();
             dp.NewFG(color_fg);
             //flatpoint pc=dp.realtoscreen(signature.totalwidth/2,signature.totalheight/2);
             flatpoint mr=dp.screentoreal(mouseX,mouseY);
             int xoff=-10;
             if (mr.x>=0 && mr.x<currentPaperWidth/2 || mr.x>currentPaperWidth) xoff=10;
             flatpoint c=new flatpoint(mouseX+xoff,mouseY);
             
             dp.drawarrow(c,new flatpoint(c.x,c.y+10));
             dp.drawarrow(c,new flatpoint(c.x,c.y-10));
             dp.DrawReal();
           }
           
        } else if (area.type==AREA_H_Slider) {
           if (area.action==overoverlay) {
             dp.DrawScreen();
             dp.NewFG(color_fg);
             //flatpoint pc=dp.realtoscreen(signature.totalwidth/2,signature.totalheight/2);
             flatpoint mr=dp.screentoreal(mouseX,mouseY);
             int yoff=15;
             if (mr.y>=0 && mr.y<currentPaperHeight/2 || mr.y>currentPaperHeight) yoff=-10;
             flatpoint c=new flatpoint(mouseX,mouseY+yoff);
             
             dp.drawarrow(c,new flatpoint(c.x-10,c.y));
             dp.drawarrow(c,new flatpoint(c.x+10,c.y));
             dp.DrawReal();
           }
           
           
        } else if (area.outline==null) {
           dp.drawarrow(area.tail, area.hotspot);
        } else {
              if (area.action==overoverlay) {
                 //fill
                if (area.type!=AREA_Handle) dp.NewFG(color_button);
                dp.drawlines(area.outline,1,1);
                          
              } else {
                   //outline only
                  dp.drawlines(area.outline,1,0);
              }
        }
        dp.NewFG(area.col);
        if (area.text!=null && area.text!="") {
                  flatpoint p=area.Center();
                  p.add(area.offset);
                  //println("area "+area.text+" at "+p.x+','+p.y);
                  dp.textout(p.x,p.y, area.text, LAX_CENTER);
        }
                
        dp.PopAxes();
        dp.DrawReal();
    }

 

    int PerformAction(int action)
    {
        if (action==SIA_Decorations) {
            showdecs=!showdecs;
            remapHandles(0);
            needtodraw=1;
            return 0;

        } else if (action==SIA_Thumbs) {
            showthumbs=!showthumbs;
            needtodraw=1;
            return 0;

        } else if (action==SIA_Center) {
            float h=signature.totalheight;
            float w=signature.totalwidth;
            //dp.Center(-w*.1,w*1.1, -h*.1,h*1.1+dp.textheight()*2/dp.Getmag());
                        dp.Center(-w*.1,w*1.1, -h*.15,h*1.3);
            remapHandles(0);
            needtodraw=1;
            return 0;
            
        } else if (action==SIA_InsetMask) {
            if (insetmask==15) insetmask=1;
            else { insetmask<<=1; if (insetmask>15) insetmask=15; }
            String str="Sets "+masktostr(insetmask)+" inset";
            PostMessage(str);
            return 0;

        } else if (action==SIA_InsetInc) {
            float step=signature.totalheight*.01;
            if ((insetmask&1)!=0) offsetHandle(SP_Inset_Top,    new flatpoint(0,-step));
            if ((insetmask&4)!=0) offsetHandle(SP_Inset_Bottom, new flatpoint(0,step));
            if ((insetmask&8)!=0) offsetHandle(SP_Inset_Left,   new flatpoint(step,0));
            if ((insetmask&2)!=0) offsetHandle(SP_Inset_Right,  new flatpoint(-step,0));
            needtodraw=1;
            return 0;

        } else if (action==SIA_InsetDec) {
            float step=signature.totalheight*.01;
            if ((insetmask&1)!=0) offsetHandle(SP_Inset_Top,    new flatpoint(0,step));
            if ((insetmask&4)!=0) offsetHandle(SP_Inset_Bottom, new flatpoint(0,-step));
            if ((insetmask&8)!=0) offsetHandle(SP_Inset_Left,   new flatpoint(-step,0));
            if ((insetmask&2)!=0) offsetHandle(SP_Inset_Right,  new flatpoint(step,0));
            needtodraw=1;
            return 0;

        } else if (action==SIA_GapInc) {
            float step=signature.totalheight*.01;
            offsetHandle(SP_Tile_Gap_X, new flatpoint(step,0));
            offsetHandle(SP_Tile_Gap_Y, new flatpoint(0,step));
            return 0;

        } else if (action==SIA_GapDec) {
            float step=-signature.totalheight*.01;
            offsetHandle(SP_Tile_Gap_X, new flatpoint(step,0));
            offsetHandle(SP_Tile_Gap_Y, new flatpoint(0,step));
            needtodraw=1;
            return 0;

        } else if (action==SIA_TileXInc) {
            adjustControl(SP_Tile_X_top, 1);
            return 0;

        } else if (action==SIA_TileXDec) {
            adjustControl(SP_Tile_X_top, -1);
            return 0;

        } else if (action==SIA_TileYInc) {
            adjustControl(SP_Tile_Y_left, 1);
            return 0;

        } else if (action==SIA_TileYDec) {
            adjustControl(SP_Tile_Y_left, -1);
            return 0;

        } else if (action==SIA_NumFoldsVInc) {
            adjustControl(SP_V_Folds_top, 1);
            return 0;

        } else if (action==SIA_NumFoldsVDec) {
            adjustControl(SP_V_Folds_top, -1);
            return 0;

        } else if (action==SIA_NumFoldsHInc) {
            adjustControl(SP_H_Folds_left, 1);
            return 0;

        } else if (action==SIA_NumFoldsHDec) {
            adjustControl(SP_H_Folds_left, -1);
            return 0;

        } else if (action==SIA_BindingEdge) {
            if (signature.binding=='l') signature.binding='t';
            else if (signature.binding=='t') signature.binding='r';
            else if (signature.binding=='r') signature.binding='b';
            else if (signature.binding=='b') signature.binding='l';
            needtodraw=1;
            return 0;

        } else if (action==SIA_BindingEdgeR) {
            if (signature.binding=='l') signature.binding='b';
            else if (signature.binding=='b') signature.binding='r';
            else if (signature.binding=='r') signature.binding='t';
            else if (signature.binding=='t') signature.binding='l';
            needtodraw=1;
            return 0;

        } else if (action==SIA_PageOrientation) {
            if (signature.up=='l') signature.up='t';
            else if (signature.up=='t') signature.up='r';
            else if (signature.up=='r') signature.up='b';
            else if (signature.up=='b') signature.up='l';
            needtodraw=1;
            return 0;

        } else if (action==SIA_TrimMask) {
            if (trimmask==15) trimmask=1;
            else { trimmask<<=1; if (trimmask>15) trimmask=15; }
            String str="Set "+masktostr(trimmask)+" trim";
            PostMessage(str);
            return 0;

        } else if (action==SIA_TrimInc) {
            float step=signature.PageHeight(0)*.01;
            if ((trimmask&1)!=0) offsetHandle(SP_Trim_Top,    new flatpoint(0,-step));
            if ((trimmask&4)!=0) offsetHandle(SP_Trim_Bottom, new flatpoint(0,step));
            if ((trimmask&8)!=0) offsetHandle(SP_Trim_Left,   new flatpoint(step,0));
            if ((trimmask&2)!=0) offsetHandle(SP_Trim_Right,  new flatpoint(-step,0));
            return 0;

        } else if (action==SIA_TrimDec) {
            float step=-signature.PageHeight(0)*.01;
            if ((trimmask&1)!=0) offsetHandle(SP_Trim_Top,    new flatpoint(0,-step));
            if ((trimmask&4)!=0) offsetHandle(SP_Trim_Bottom, new flatpoint(0,step));
            if ((trimmask&8)!=0) offsetHandle(SP_Trim_Left,   new flatpoint(step,0));
            if ((trimmask&2)!=0) offsetHandle(SP_Trim_Right,  new flatpoint(-step,0));
            return 0;

        } else if (action==SIA_MarginMask) {
            if (marginmask==15) marginmask=1;
            else { marginmask<<=1; if (marginmask>15) marginmask=15; }
            String str="Set "+masktostr(marginmask)+" margin";
            PostMessage(str);
            return 0;

        } else if (action==SIA_MarginInc) {
            float step=signature.PageHeight(0)*.01;
            if ((marginmask&1)!=0) offsetHandle(SP_Margin_Top,    new flatpoint(0,-step));
            if ((marginmask&4)!=0) offsetHandle(SP_Margin_Bottom, new flatpoint(0,step));
            if ((marginmask&8)!=0) offsetHandle(SP_Margin_Left,   new flatpoint(step,0));
            if ((marginmask&2)!=0) offsetHandle(SP_Margin_Right,  new flatpoint(-step,0));
            return 0;

        } else if (action==SIA_MarginDec) {
            float step=-signature.PageHeight(0)*.01;
            if ((marginmask&1)!=0) offsetHandle(SP_Margin_Top,    new flatpoint(0,-step));
            if ((marginmask&4)!=0) offsetHandle(SP_Margin_Bottom, new flatpoint(0,step));
            if ((marginmask&8)!=0) offsetHandle(SP_Margin_Left,   new flatpoint(step,0));
            if ((marginmask&2)!=0) offsetHandle(SP_Margin_Right,  new flatpoint(-step,0));
            return 0;

        } else if (action==SIA_ZoomIn || action==SIA_ZoomOut) {     
            flatpoint p=dp.screentoreal(DISPLAYWIDTH/2,DISPLAYHEIGHT/2);
            dp.Zoom(action==SIA_ZoomIn?1.1:.9);
            dp.CenterPoint(p);
            needtodraw=1;
            return 0;

        } else if (action==SIA_ZoomInControls) {
            INDICATOR_SIZE*=1.1;
            TEXTHEIGHT*=1.1;
            textSize(TEXTHEIGHT);
            remapHandles(0);
            needtodraw=1;
            return 0;

        } else if (action==SIA_ZoomOutControls) {
            INDICATOR_SIZE*=.9;
            if (INDICATOR_SIZE<5) INDICATOR_SIZE=3;
            TEXTHEIGHT*=.9;
            if (TEXTHEIGHT<5) TEXTHEIGHT=5;
            textSize(TEXTHEIGHT);

            remapHandles(0);
            needtodraw=1;
            return 0;

        }

        return 1;
    } //int SignatureInterface::PerformAction(int action)


    //! Respond to spinning controls.
    int WheelDown(int x,int y,int state)
    {
        if (showsplash) { showsplash=false; needtodraw=1; }

        //flatpoint fp=screentoreal(x,y);

        if ((state&ControlMask)!=0) {
                    dp.Zoom(1.1);
                    needtodraw=1;
                    return 0;
                }  

        int handle=scanHandle(x,y);
        adjustControl(handle,-1);

        return 0; //this will always absorb plain wheel
    }

    //! Respond to spinning controls.
    int WheelUp(int x,int y,int state)
    {
        if (showsplash) { showsplash=false; needtodraw=1; }

        //flatpoint fp=screentoreal(x,y);

                if ((state&ControlMask)!=0) {
                    dp.Zoom(.9);
                    needtodraw=1;
                    return 0;
                }  

        int handle=scanHandle(x,y);
        adjustControl(handle,1);

        return 0; //this will always absorb plain wheel
    }

    int MouseMove(int x,int y,int state)
    {
        float[] ret=scan(x,y);
        int over=(int)ret[0];
        int row=(int)(ret[1]+.5);
        int col=(int)(ret[2]+.5);
        flatpoint mm=new flatpoint(ret[3],ret[4]);
        int tilerow=(int)(ret[5]+.5);
        int tilecol=(int)(ret[6]+.5);
                
        
        int mx=mouseX, my=mouseY;
        int lx=pmouseX, ly=pmouseY;

        int handle=scanHandle(x,y);

        //println ("scan: {"+ret[0]+","+ret[1]+","+ret[2]+","+ret[3]+","+ret[4]+","+ret[5]+","+ret[6]+"}");
        //println ("scan handle: "+handle);

        if (overoverlay!=handle) needtodraw=1;
        if (handle==SP_None) {
            int i=scanForFoldIndicator(x,y,0);
            if (i>=0) {
              if (statusmessage!="Fold player, click for level, drag plays") {
                PostMessage("Fold player, click for level, drag plays");
                needtodraw=1;
              }
            } else PostMessage(""); 
            overoverlay=0;

        } else {
            overoverlay=handle;
            PostMessage(control(handle).tip);
        }
        return 0;

    } //MouseMove

    int LBDown(int x,int y, int state)
    {
                //println("lbdown "+x+","+y);
                
        if (showsplash) { showsplash=false; needtodraw=1; }

        initialmousex=x;
        initialmousey=y;
        mouseinfo1=0;
        mouseinfo2=0;
        

        onoverlay=SP_None;

         //check overlays first
        int i=scanForFoldIndicator(x,y,0);
        if (i>=0) {
            onoverlay=SP_FOLDS+i;
            mouseinfo1=i;
            mouseinfo2=0;
            foldprogress=-1;
            return 0;
        }
        onoverlay=scanHandle(x,y);
        if (onoverlay!=SP_None) {
            mslidex=x;
            mslidey=y;
            ActionArea a=control(onoverlay);
            if (a.type!=AREA_Display_Only) return 0;
            onoverlay=SP_None;
        }


        float[] ret=scan(x,y);
        int row=(int)(ret[1]);
        int col=(int)(ret[2]);
        int tilerow=(int)(ret[5]);
        int tilecol=(int)(ret[6]);

         //check for on something to fold
        if (row<0 || row>signature.numhfolds || col<0 || col>signature.numvfolds
              || ifoldinfo[row][col].pages.size()==0
              || tilerow<0 || tilecol<0
              || tilerow>=signature.tiley || tilecol>=signature.tilex) {
            lbdown_row=lbdown_col=-1;
        } else {
            if (tilerow>=0 && tilerow<signature.tiley && tilerow!=activetiley) { needtodraw=1; activetiley=tilerow; }
            if (tilecol>=0 && tilecol<signature.tilex && tilecol!=activetilex) { needtodraw=1; activetilex=tilecol; }

            lbdown_row=row;
            lbdown_col=col;
            folddirection=0;
            foldprogress=0;
        }


        return 0;
    } //LBDown();

    int LBUp(int x,int y,int state)
    {
        int dragged=abs(x-initialmousex) + abs(y-initialmousey);

        if (onoverlay>0) {
            if (onoverlay<SP_FOLDS) {
                ActionArea area=control(onoverlay);

                if (area.action==SP_Save) {
                    save("laidout-sheet"+(int(currentPaperSpread/2)+1)+(currentPaperSpread%2==0?"Front":"Back")+".png");

                } else if (area.action==SP_ZoomControls   ) {
                    PerformAction(SIA_ZoomInControls);
                    return 0;

                } else if (area.action==SP_ZoomOutControls) {
                    PerformAction(SIA_ZoomOutControls);
                    return 0;

                } else if (area.action==SP_ZoomIn         ) {
                    PerformAction(SIA_ZoomIn);
                    return 0;

                } else if (area.action==SP_ZoomOut        ) {
                    PerformAction(SIA_ZoomOut);
                    return 0;

                } else if (area.type==AREA_Slider && dragged==0) {
                    flatpoint p=area.Center();
                    p.add(area.offset);
                    if (x>p.x) {
                      adjustControl(onoverlay,1);
                      
                    } else {
                       adjustControl(onoverlay,-1);
                    }
                } 

            } else if (onoverlay>=SP_FOLDS) {
                          
                 //selecting different fold maybe...
                if (dragged==0) {
                     //we clicked down then up on the same overlay

                    int folds=onoverlay-SP_FOLDS; //0 means totally unfolded

                    if (foldlevel==folds) return 0; //already at that fold level

                     //we must remap the folds to reflect the new fold level
                    signature.resetFoldinfo(null);
                    for (int c=0; c<folds; c++) {
                        applyFold((Fold)signature.folds.get(c));
                    }
                    foldlevel=folds;
                    //checkFoldLevel();
                }

                folddirection=0;
                remapHandles(0);
                needtodraw=1;
            }
            
            return 0;
        }

        if (folddirection!=0 && folddirection!='x' && foldprogress>.9) {
             //apply the fold, and add to the signature...

            applyFold((char)folddirection,foldindex,foldunder);

            if (foldlevel<signature.folds.size()) {
                 //we have tried to fold when there are already further folds, so we must remove any
                 //after the current foldlevel.
                while (foldlevel<signature.folds.size()) signature.folds.remove(signature.folds.size()-1);
                hasfinal=false;
            }

            signature.folds.add(new Fold((char)folddirection,foldunder,foldindex));
            foldlevel=signature.folds.size();

            checkFoldLevel();
            if (hasfinal) remapHandles(0);

            folddirection=0;
            foldprogress=0;
            remapHandles(0);
        }

        lbdown_row=lbdown_col=-1;
        folddirection=0;
        needtodraw=1;
        return 0;
    } // int LBUp(int x,int y,int state)


    //! Return a screen rectangle containing the specified fold level indicator.
    /*! which==-1 means all unfolded. 0 or more means that fold.
     */
    float[] getFoldIndicatorPos(int which)
    {
        int radius=(int)INDICATOR_SIZE;

        float[] ret={0,0,0,0};

        ret[0]=folderoffset; //x
        ret[1]=(DISPLAYHEIGHT+0)/2 - (signature.folds.size()+1)*(radius-3); //y
        ret[2]=2*radius; //w
        ret[3]=2*radius; //h

        ret[1]=ret[1]+(which+1)*(2*radius-3);

        return ret;
    }

    //! Returns 0 for the circle, totally unfolded, or else fold index+1, or -1 for not found.
    int scanForFoldIndicator(int x, int y, int ignorex)
    {
        int radius=(int)INDICATOR_SIZE;
                
        if (ignorex==0 && (x-folderoffset>2*radius || x-folderoffset<0)) return -1;

        float xx,yy,w,h;
        float[] ret;
        for (int c=signature.folds.size()-1; c>=-1; c--) {
            ret=getFoldIndicatorPos(c);
            xx=ret[0];
            yy=ret[1];
            w =ret[2];
            h =ret[3];
            if (x>=xx && x<xx+w && y>=yy && y<yy+h) {
                return c+1;
            }
        }
        return -1;
    }

    //! Return 0 for not in pattern, or the row and column in a folding section.
    /*! x and y are real coordinates.
     *
     * Returns the real coordinate within an element of the folding pattern in ex,ey.
     *
     * returns float[]: [found something, row,col, ex,ey, tile_row,tile_col].
     */
    float[] scan(int x,int y) //:scan
    {
        int row, col;
        float ex,ey;
        int tile_row,tile_col;

                //println ("scan with ctm:");
                //dp.dumpctm();
                //flatpoint oo=dp.realtoscreen(0,0);
                //println("scan real origin: "+oo.x+","+oo.y);
        flatpoint fp=dp.screentoreal(x,y);
                //flatpoint fp2=dp.realtoscreen(fp.x,fp.y);
        //println("scan scr:"+x+','+y+"  -> real:"+fp.x+','+fp.y+"  -> scr:"+fp2.x+','+fp2.y);
                
                

        fp.x-=signature.insetleft;
        fp.y-=signature.insetbottom;

        float patternheight=signature.PatternHeight();
        float patternwidth =signature.PatternWidth();
        float elementheight=patternheight/(signature.numhfolds+1);
        float elementwidth =patternwidth /(signature.numvfolds+1);

        int tilex,tiley;
        tilex=floor(fp.x/(patternwidth +signature.tilegapx));
        tiley=floor(fp.y/(patternheight+signature.tilegapy));
        tile_col=tilex;
        tile_row=tiley;

        fp.x-=tilex*(patternwidth +signature.tilegapx);
        fp.y-=tiley*(patternheight+signature.tilegapy);

        row=floor(fp.y/elementheight);
        col=floor(fp.x/elementwidth);

        //println("scanned row,col: "+row+','+col+"  tilex,y: "+tilex+','+tiley);

         //find coordinates within an element cell
        ey=fp.y-(row)*elementheight;
        ex=fp.x-(col)*elementwidth;


        float[] ret=new float[7];
        ret[0]=((row>=0 && row<=signature.numhfolds && col>=0 && col<=signature.numvfolds)?1:0);
        ret[1]=row;
        ret[2]=col;
        ret[3]=ex;
        ret[4]=ey;
        ret[5]=tile_row;
        ret[6]=tile_col;
        return ret;
    } //scan()


    //! Scan for handles to control variables, not for row and column.
    /*! Returns which handle screen point is in, or none.
     */
    int scanHandle(int x,int y) //:scanHandle
    {
        flatpoint fp=dp.screentoreal(x,y);
        flatpoint tp=new flatpoint();
        Signature s=signature;

                ActionArea area;
        for (int c=0; c<controls.size(); c++) {
                        area=(ActionArea)controls.get(c);
            if (area.hidden) continue;

            if (area.category==0) {
                 //normal, single instance handles
                if ((area.real!=0 && area.PointIn(fp.x,fp.y))
                        || (area.real==0 && area.PointIn(x,y))) {
                    return area.action;
                }

            } else if (area.category==1) {
                 //fold lines, in the pattern area
                for (int xx=0; xx<signature.tilex; xx++) {
                  for (int yy=0; yy<signature.tiley; yy++) {
                    tp.x=fp.x-s.insetleft;
                                        tp.y=fp.y-s.insetbottom;
                    tp.subtract(new flatpoint(xx*(s.tilegapx+s.PatternWidth()), yy*(s.tilegapy+s.PatternHeight())));

                    if (area.PointIn(tp.x,tp.y)) return area.action;
                  }
                }

            } else if (area.category==2) {
                 //in a final page area
                float w=s.PageWidth(0);
                float h=s.PageHeight(0);
                tp.x=fp.x-(s.insetleft  +activetilex*(s.PatternWidth() +s.tilegapx) + finalc*w);
                tp.y=fp.y-(s.insetbottom+activetiley*(s.PatternHeight()+s.tilegapy) + finalr*h);

                if (area.action==SP_Binding) {
                    if (s.binding=='l') {
                        if (tp.x>0 && tp.x<w*.1 && tp.y>0 && tp.y<h) return SP_Binding;
                    } else if (s.binding=='r') {
                        if (tp.x>w*.9 && tp.x<w && tp.y>0 && tp.y<h) return SP_Binding;
                    } else if (s.binding=='t') {
                        if (tp.x>0 && tp.x<w && tp.y>h*.9 && tp.y<h) return SP_Binding;
                    } else if (s.binding=='b') {
                        if (tp.x>0 && tp.x<w && tp.y>0 && tp.y<h*.1) return SP_Binding;
                    }
                } else if (area.PointIn(tp.x,tp.y)) return area.action;

            } else if (area.category==3) {
                 //tile gaps
                float d;
                float zone=5/dp.Getmag(); //selection zone override for really skinny gaps

                if (area.action==SP_Tile_Gap_X) {
                    for (int c2=0; c2<signature.tilex-1; c2++) {
                         //first check for inside gap itself
                        d=s.insetleft+(c2+1)*(s.tilegapx+s.PatternWidth())-s.tilegapx/2;
                        if (s.tilegapx>zone) zone=s.tilegapx;
                        if (fp.y>0 && fp.y<s.totalheight && fp.x<=d+zone/2 && fp.x>=d-zone/2) return SP_Tile_Gap_X;
                    }

                } else { //tile gap y
                    for (int c2=0; c2<signature.tiley-1; c2++) {
                         //first check for inside gap itself
                        d=s.insetbottom+(c2+1)*(s.tilegapy+s.PatternHeight())-s.tilegapy/2;
                        if (s.tilegapy>zone) zone=s.tilegapy;
                        if (fp.x>0 && fp.x<s.totalwidth && fp.y<=d+zone/2 && fp.y>=d-zone/2) return SP_Tile_Gap_Y;
                    }
                }
            }
        }

         //check for gross areas for insets 
        if (fp.x>0 && fp.x<signature.totalwidth) {
            if (fp.y>0 && fp.y<signature.insetbottom) return SP_Inset_Bottom;
            if (fp.y>s.totalheight-s.insettop && fp.y<s.totalheight) return SP_Inset_Top;
        }
        if (fp.y>0 && fp.y<signature.totalheight) {
            if (fp.x>0 && fp.x<signature.insetleft) return SP_Inset_Left;
            if (fp.x>s.totalwidth-s.insetright && fp.x<s.totalwidth) return SP_Inset_Right;
        }


        return SP_None;
    } //scanHandle()


    /*! This only allocates the controls, does not position or anything else.
     */
    void createHandles()
    {
                if (controls!=null) return;
        color c;

        //ActionArea categories:
        //  0 single instance
        //  1 in pattern area
        //  2 in page area
        //  3 in gap area

        controls=new ArrayList();

        c=fg_color;
        //controls.add(new ActionArea(SP_Sheets_Per_Sig   , THING_None       , AREA_Slider, null,   ("Drag changes"),1,0,c,0));

        //screen positioned controls:
        controls.add(new ActionArea(SP_Paper_Name       , THING_None       , AREA_Slider, currentPaper, ("Paper to use"),0,1,c,0));
        controls.add(new ActionArea(SP_Paper_Orient     , THING_None       , AREA_Slider, "--", ("Paper orientation"),0,1,c,0));
        //controls.add(new ActionArea(SP_Paper_Width      , THING_None       , AREA_Display_Only, "--", ("Paper width"),0,1,c,0));
        //controls.add(new ActionArea(SP_Paper_Height     , THING_None       , AREA_Display_Only, "--", ("Paper height"),0,1,c,0));

        controls.add(new ActionArea(SP_Current_Sheet    , THING_None       , AREA_Slider, "Sheet", ("Current sheet"),0,1,c,0));
        //controls.add(new ActionArea(SP_Front_Or_Back    , THING_None       , AREA_Slider, "--",    ("Whether showing front of back of current sheet"),0,1,c,0));

        controls.add(new ActionArea(SP_Num_Pages        , THING_None       , AREA_Slider, "Pages", ("Drag changes number of pages"),0,1,c,0));
        //controls.add(new ActionArea(SP_Num_Sigs         , THING_None       , AREA_Slider, "Sigs",  ("Number of signatures needed"),0,1,c,0));

        controls.add(new ActionArea(SP_Save             , THING_None       , AREA_Button, "Save", null,0,1,c,0));
        controls.add(new ActionArea(SP_ZoomControls     , THING_None       , AREA_Button, "aA",   null,0,1,c,0));
        controls.add(new ActionArea(SP_ZoomOutControls  , THING_None       , AREA_Button, "Aa",   null,0,1,c,0));
        controls.add(new ActionArea(SP_ZoomIn           , THING_None       , AREA_Button, "+",    null,0,1,c,0));
        controls.add(new ActionArea(SP_ZoomOut          , THING_None       , AREA_Button, "-",    null,0,1,c,0));


         //handles and sliders
        c=color_inset;
        controls.add(new ActionArea(SP_Inset_Top        , THING_Arrow_Down , AREA_Handle, null, ("Inset top"),2,1,c,0));
        controls.add(new ActionArea(SP_Inset_Bottom     , THING_Arrow_Up   , AREA_Handle, null, ("Inset bottom"),2,1,c,0));
        controls.add(new ActionArea(SP_Inset_Left       , THING_Arrow_Right, AREA_Handle, null, ("Inset left"),2,1,c,0));
        controls.add(new ActionArea(SP_Inset_Right      , THING_Arrow_Left , AREA_Handle, null, ("Inset right"),2,1,c,0));

        c=color_trim;
        controls.add(new ActionArea(SP_Trim_Top         , THING_Arrow_Down , AREA_Handle, null, ("Trim top of page"),2,1,c,2));
        controls.add(new ActionArea(SP_Trim_Bottom      , THING_Arrow_Up   , AREA_Handle, null, ("Trim bottom of page"),2,1,c,2));
        controls.add(new ActionArea(SP_Trim_Left        , THING_Arrow_Right, AREA_Handle, null, ("Trim left of page"),2,1,c,2));
        controls.add(new ActionArea(SP_Trim_Right       , THING_Arrow_Left , AREA_Handle, null, ("Trim right of page"),2,1,c,2));

        c=lerpColor(color_margin,0,.3333);
        controls.add(new ActionArea(SP_Margin_Top       , THING_Arrow_Down , AREA_Handle, null, ("Margin top of page"),2,1,c,2));
        controls.add(new ActionArea(SP_Margin_Bottom    , THING_Arrow_Up   , AREA_Handle, null, ("Margin bottom of page"),2,1,c,2));
        controls.add(new ActionArea(SP_Margin_Left      , THING_Arrow_Right, AREA_Handle, null, ("Margin left of page"),2,1,c,2));
        controls.add(new ActionArea(SP_Margin_Right     , THING_Arrow_Left , AREA_Handle, null, ("Margin right of page"),2,1,c,2));

        c=color_binding;
        controls.add(new ActionArea(SP_Binding          , THING_None       , AREA_Handle, null, ("Binding edge, drag to place"),1,0,c,2));

        c=color_inset;
        controls.add(new ActionArea(SP_Tile_X_top       , THING_None       , AREA_H_Slider, null, ("Tile horizontally, drag changes"),1,1,c,0));
        controls.add(new ActionArea(SP_Tile_X_bottom    , THING_None       , AREA_H_Slider, null, ("Tile horizontally, drag changes"),1,1,c,0));
        controls.add(new ActionArea(SP_Tile_Y_left      , THING_None       , AREA_V_Slider, null, ("Tile vertically, drag changes"),1,1,c,0));
        controls.add(new ActionArea(SP_Tile_Y_right     , THING_None       , AREA_V_Slider, null, ("Tile vertically, drag changes"),1,1,c,0));

        controls.add(new ActionArea(SP_Tile_Gap_X       , THING_None       , AREA_Handle, null, ("Vertical gap between tiles"),1,0,c,3));
        controls.add(new ActionArea(SP_Tile_Gap_Y       , THING_None       , AREA_Handle, null, ("Horizontal gap between tiles"),1,0,c,3));

        c=color_margin;
        controls.add(new ActionArea(SP_H_Folds_left     , THING_None       , AREA_V_Slider, null, ("Number of horizontal folds, drag changes"),1,1,c,1));
        controls.add(new ActionArea(SP_H_Folds_right    , THING_None       , AREA_V_Slider, null, ("Number of horizontal folds, drag changes"),1,1,c,1));
        controls.add(new ActionArea(SP_V_Folds_top      , THING_None       , AREA_H_Slider, null, ("Number of vertical folds, drag changes"),1,1,c,1));
        controls.add(new ActionArea(SP_V_Folds_bottom   , THING_None       , AREA_H_Slider, null, ("Number of vertical folds, drag changes"),1,1,c,1));


    } //void createHandles()


    //! Adjust foldr1,foldr2,foldc1,foldc2 to reflect which cells get moved for whichfold.
    /*! whichfold==0 means what is affected for the 1st fold, as if you were going from totally unfolded
     * to after the 1st fold.
     */
    void remapAffectedCells(int whichfold) //SignatureInterface::remapAffectedCells
    {
        if (whichfold<0 || whichfold>=signature.folds.size()) return;

        FoldedPageInfo[][] finfo;
        if (whichfold==foldlevel) {
             //we can use the current foldinfo
            finfo=ifoldinfo;
        } else {
                        
             //we need to use a temporary foldinfo
            finfo=new FoldedPageInfo[signature.numhfolds+2][signature.numvfolds+2];
            int r;
            for (r=0; r<signature.numhfolds+1; r++) {
                 for (int c=0; c<signature.numvfolds+1; c++) {
                    finfo[r][c]=new FoldedPageInfo();
                    finfo[r][c].pages.add(r);
                    finfo[r][c].pages.add(c);
                }
            }

            signature.applyFold(finfo,whichfold);//applies all folds up to whichfold
        }

        // *** figure out cells based on direction and index
        Fold fold=(Fold)signature.folds.get(whichfold);
        int dir  =fold.direction;
        int index=fold.whichfold;
        int fr1,fr2,fc1,fc2;

         //find the cells that must be moved..
         //First find the whole block that might move, then shrink it down to include only active faces
        if (dir=='l') {
            fc1=index; //the only known so far
            fr1=0;
            fr2=signature.numhfolds;
            fc2=signature.numvfolds;
            while (fr1<fr2 && finfo[fr1][fc1].pages.size()==0) fr1++;
            while (fr2>fr1 && finfo[fr2][fc1].pages.size()==0) fr2--;
            while (fc2>fc1 && finfo[fr1][fc2].pages.size()==0) fc2--;
        } else if (dir=='r') {
            fc2=index-1;
            fr1=0;
            fr2=signature.numhfolds;
            fc1=0;
            while (fr1<fr2 && finfo[fr1][fc2].pages.size()==0) fr1++;
            while (fr2>fr1 && finfo[fr2][fc2].pages.size()==0) fr2--;
            while (fc1<fc2 && finfo[fr1][fc1].pages.size()==0) fc1++;
        } else if (dir=='b') {
            fr1=index;
            fc1=0;
            fc2=signature.numvfolds;
            fr2=signature.numhfolds;
            while (fc1<fc2 && finfo[fr1][fc1].pages.size()==0) fc1++;
            while (fc2>fc1 && finfo[fr1][fc2].pages.size()==0) fc2--;
            while (fr2>fr1 && finfo[fr2][fc1].pages.size()==0) fr2--;
        } else { //if (dir=='t') {
            fr2=index-1;
            fr1=0;
            fc1=0;
            fc2=signature.numvfolds;
            while (fc1<fc2 && finfo[fr2][fc1].pages.size()==0) fc1++;
            while (fc2>fc1 && finfo[fr2][fc2].pages.size()==0) fc2--;
            while (fr1<fr2 && finfo[fr1][fc1].pages.size()==0) fr1++;
        }


        foldr1=fr1;
        foldr2=fr2;
        foldc1=fc1;
        foldc2=fc2;
    }

    //! Position the handles to be where they should.
    /*! which==0 means do all.
     *  which&1 do singletons,
     *  which&2 pattern area (category 1),
     *  which&4 page area (category 2)
     */
    void remapHandles(int which) //:remapH
    {
        if (which==0) which=~0;

        if (controls==null || controls.size()==0) createHandles();

        ActionArea area;
        flatpoint[] p;
        int n;
        float ww=signature.totalwidth, hh=signature.totalheight;
        float w=signature.PageWidth(0), h=signature.PageHeight(0);
        float www=signature.PatternWidth(), hhh=signature.PatternHeight();

        arrowscale=2*INDICATOR_SIZE/dp.Getmag();


         //----- sheets and pages, and other general settings
        if ((which&1)!=0) {
            //     sheets per signature
            //
            // Num pages   --   Num sigs
            //
            // automarks

             //place just under the folding area
            //area=control(SP_Sheets_Per_Sig);
            //p=area.Points(null,4,false);
            //p[0]=new flatpoint(0,-hh*.1);  p[1]=new flatpoint(ww,-hh*.1); p[2]=new flatpoint(ww,-hh*.2); p[3]=new flatpoint(0,-hh*.2);
            //area.FindBBox();

            //area=control(SP_Num_Sigs);
            //p=area.Points(null,4,false);
            //p[0]=new flatpoint(ww/2,-hh*.2);  p[1]=new flatpoint(ww,-hh*.2); p[2]=new flatpoint(ww,-hh*.3); p[3]=new flatpoint(ww/2,-hh*.3);
            //area.FindBBox();
            //buffer=""+(signature.hint_numsigs)+" sigs";
            //area.text=buffer;


             //controls at top of screen
            float textheight=textAscent()+textDescent(); //textheight
            String buffer;
            
            area=control(SP_Paper_Name);
            buffer=currentPaper;
            area.text=buffer;
            float wwww=textWidth("halfletter")+textheight;
            float hhhh=textheight*1.4;
            p=area.Points(null,4,false);
            p[0]=new flatpoint(0,hhhh);  p[1]=new flatpoint(wwww,hhhh); p[2]=new flatpoint(wwww,2*hhhh); p[3]=new flatpoint(0,2*hhhh);
            area.FindBBox();
            
            float xxx=wwww;
            area=control(SP_Paper_Orient);
            if (currentOrientation==0) buffer="Portrait"; else buffer="Landscape";
            area.text=buffer;
            wwww=textWidth("Landscape")+textheight*.5;
            p=area.Points(null,4,false);
            p[0]=new flatpoint(xxx,hhhh);  p[1]=new flatpoint(xxx+wwww,hhhh); p[2]=new flatpoint(xxx+wwww,2*hhhh); p[3]=new flatpoint(xxx,2*hhhh);
            area.FindBBox();

            /*xxx+=wwww;
            area=control(SP_Paper_Width);
            buffer=""+(currentPaperWidth);
            area.text=buffer;
            wwww=textWidth("00000")+textheight;
            p=area.Points(null,4,false);
            p[0]=new flatpoint(xxx,hhhh);  p[1]=new flatpoint(xxx+wwww,hhhh); p[2]=new flatpoint(xxx+wwww,2*hhhh); p[3]=new flatpoint(xxx,2*hhhh);
            area.FindBBox();
            
            xxx+=wwww;
            area=control(SP_Paper_Height);
            buffer=""+(currentPaperHeight);
            area.text=buffer;
            wwww=textWidth("00000")+textheight;
            p=area.Points(null,4,false);
            p[0]=new flatpoint(xxx,hhhh);  p[1]=new flatpoint(xxx+wwww,hhhh); p[2]=new flatpoint(xxx+wwww,2*hhhh); p[3]=new flatpoint(xxx,2*hhhh);
            area.FindBBox();
            */

            xxx=0;
            area=control(SP_Current_Sheet);
            buffer="Sheet "+(int(currentPaperSpread/2)+1)+"/"+signature.sheetspersignature + (OnBack()?", Back":", Front");
            area.text=buffer;
            wwww=textWidth("Sheet 000/00, Front")+textheight*.1;
            p=area.Points(null,4,false);
            p[0]=new flatpoint(xxx,2*hhhh);  p[1]=new flatpoint(xxx+wwww,2*hhhh); p[2]=new flatpoint(xxx+wwww,3*hhhh); p[3]=new flatpoint(xxx,3*hhhh);
            area.FindBBox();
            
            //xxx+=wwww;
            //area=control(SP_Front_Or_Back);
            //buffer=((currentPaperSpread%2)==0?"Front":"Back");
            //area.text=buffer;
            //wwww=textWidth("Front")+textheight;
            //p=area.Points(null,4,false);
            //p[0]=new flatpoint(xxx,2*hhhh);  p[1]=new flatpoint(xxx+wwww,2*hhhh); p[2]=new flatpoint(xxx+wwww,3*hhhh); p[3]=new flatpoint(xxx,3*hhhh);
            //area.FindBBox();

            xxx=0;
            area=control(SP_Num_Pages);
            buffer=""+(signature.hint_numpages)+" pages";
            area.text=buffer;
            wwww=textWidth("000 pages")+textheight;
            //----
            //p=area.Points(null,4,false);
            //p[0]=new flatpoint(xxx,3*hhhh);  p[1]=new flatpoint(xxx+wwww,3*hhhh); p[2]=new flatpoint(xxx+wwww,4*hhhh); p[3]=new flatpoint(xxx,4*hhhh);
            //area.FindBBox();
            //----
            area.SetQuad(new flatpoint(xxx,3*hhhh), new flatpoint(xxx+wwww,3*hhhh), new flatpoint(xxx+wwww,4*hhhh), new flatpoint(xxx,4*hhhh));

            area=control(SP_Save);
            wwww=textWidth(area.text)+textheight;
            area.SetRect(DISPLAYWIDTH-wwww,0, wwww,hhhh);
                 
            area=control(SP_ZoomControls);
            wwww=textWidth(area.text)+textheight;
            area.SetRect(DISPLAYWIDTH-2*wwww,DISPLAYHEIGHT-hhhh, wwww,hhhh);

            area=control(SP_ZoomOutControls);
            area.SetRect(DISPLAYWIDTH-wwww,DISPLAYHEIGHT-hhhh, wwww,hhhh);

            area=control(SP_ZoomIn);
            wwww=textWidth(area.text)+textheight;
            area.SetRect(0,DISPLAYHEIGHT-hhhh, wwww,hhhh);

            area=control(SP_ZoomOut);       
            area.SetRect(wwww,DISPLAYHEIGHT-hhhh, wwww,hhhh);
                      
        }


        if ((which&1)!=0) { //paper specific
            flatpoint off=dp.realtoscreen(-ww*.1,0);
            off.x-=INDICATOR_SIZE*6;
            folderoffset=(int)off.x;
            if (folderoffset<0) folderoffset=0;
        
            area=control(SP_Tile_X_top); //trapezoidal area above paper
            p=area.Points(null,4,false);
            p[0]=new flatpoint(0,hh);  p[1]=new flatpoint(ww,hh); p[2]=new flatpoint(ww*1.1,hh*1.1); p[3]=new flatpoint(-ww*.1,hh*1.1);
            area.FindBBox();

            area=control(SP_Tile_X_bottom); //trapezoidal area below paper
            p=area.Points(null,4,false);
            p[0]=new flatpoint(0,0);  p[1]=new flatpoint(ww,0); p[2]=new flatpoint(ww*1.1,-hh*.1); p[3]=new flatpoint(-ww*.1,-hh*.1);
            area.FindBBox();

            area=control(SP_Tile_Y_left);
            p=area.Points(null,4,false);
            p[0]=new flatpoint(0,0);  p[1]=new flatpoint(0,hh); p[2]=new flatpoint(-ww*.1,hh*1.1); p[3]=new flatpoint(-ww*.1,-hh*.1);
            area.FindBBox();

            area=control(SP_Tile_Y_right);
            p=area.Points(null,4,false);
            p[0]=new flatpoint(ww,0);  p[1]=new flatpoint(ww,hh); p[2]=new flatpoint(ww*1.1,hh*1.1); p[3]=new flatpoint(ww*1.1,-hh*.1);
            area.FindBBox();
                        
            /*
            ***area=control(SP_Tile_Gap_X);
            p=draw_thing_coordinates(THING_Double_Arrow_Horizontal, null,-1, &n, arrowscale);
            area.Points(p,n,1);
            area.hotspot=new flatpoint(arrowscale/2,arrowscale/2);
            area.Position(0,0,3);

            ***area=control(SP_Tile_Gap_Y);
            p=draw_thing_coordinates(THING_Double_Arrow_Vertical, null,-1, &n, arrowscale);
            area.Points(p,n,1);
            area.hotspot=new flatpoint(arrowscale/2,arrowscale/2);
            area.Position(0,0,3);
            */

             //------inset
            area=control(SP_Inset_Top);
            area.hotspot=new flatpoint(arrowscale/2,0);
            area.tail   =new flatpoint(arrowscale/2,arrowscale);
            area.offset=new flatpoint(ww/2-arrowscale/2,hh-signature.insettop);
            area.FindBBox();

            area=control(SP_Inset_Bottom);
            area.hotspot=new flatpoint(arrowscale/2,arrowscale);
			area.tail   =new flatpoint(arrowscale/2,0);
            area.offset=new flatpoint(ww/2-arrowscale/2,signature.insetbottom-arrowscale);
			area.FindBBox();

            area=control(SP_Inset_Left);
            area.hotspot=new flatpoint(arrowscale,arrowscale/2);
			area.tail   =new flatpoint(0,arrowscale/2);
            area.offset=new flatpoint(signature.insetleft-arrowscale,hh/2-arrowscale/2);
			area.FindBBox();

            area=control(SP_Inset_Right);
            area.hotspot=new flatpoint(0,arrowscale/2);
			area.tail   =new flatpoint(arrowscale,arrowscale/2);
            area.offset=new flatpoint(ww-signature.insetright,hh/2-arrowscale/2);
			area.FindBBox();
        } //which&1


        if ((which&2)!=0) { //pattern area specific
             //-----folds
            area=control(SP_H_Folds_left);
            area.hidden=(foldlevel!=0);
            p=area.Points(null,4,false);
            p[0]=new flatpoint(0,0);  p[1]=new flatpoint(www*.1,hhh*.1); p[2]=new flatpoint(www*.1,hhh*.9); p[3]=new flatpoint(0,hhh);
			area.FindBBox();

            area=control(SP_H_Folds_right);
            area.hidden=(foldlevel!=0);
            p=area.Points(null,4,false);
            p[0]=new flatpoint(www,0);  p[1]=new flatpoint(www,hhh); p[2]=new flatpoint(www*.9,hhh*.9); p[3]=new flatpoint(www*.9,hhh*.1);
			area.FindBBox();
                        
            area=control(SP_V_Folds_top);
            area.hidden=(foldlevel!=0);
            p=area.Points(null,4,false);
            p[0]=new flatpoint(0,hhh);  p[1]=new flatpoint(www,hhh); p[2]=new flatpoint(www*.9,hhh*.9); p[3]=new flatpoint(www*.1,hhh*.9);
			area.FindBBox();
                        
            area=control(SP_V_Folds_bottom);
            area.hidden=(foldlevel!=0);
            p=area.Points(null,4,false);
            p[0]=new flatpoint(0,0);  p[1]=new flatpoint(www,0); p[2]=new flatpoint(www*.9,hhh*.1); p[3]=new flatpoint(www*.1,hhh*.1);
			area.FindBBox();
        }


        if ((which&4)!=0) { //final page area specific
             //----trim
            area=control(SP_Trim_Top);
            area.hotspot=new flatpoint(arrowscale/2,0);
            area.tail  =new flatpoint(arrowscale/2,arrowscale);
            area.offset=new flatpoint(w/3-arrowscale/2,h-signature.trimtop);
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
            area.FindBBox();

            area=control(SP_Trim_Bottom);
            area.hotspot=new flatpoint(arrowscale/2,arrowscale);
            area.tail   =new flatpoint(arrowscale/2,0);
            area.offset=new flatpoint(w/3-arrowscale/2,signature.trimbottom-arrowscale);
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
            area.FindBBox();

            area=control(SP_Trim_Left);
            area.hotspot=new flatpoint(arrowscale,arrowscale/2);
            area.tail   =new flatpoint(0,arrowscale/2);
            area.offset=new flatpoint(signature.trimleft-arrowscale,h/3-arrowscale/2);
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
            area.FindBBox();

            area=control(SP_Trim_Right);
            area.hotspot=new flatpoint(0,arrowscale/2);
            area.tail   =new flatpoint(arrowscale,arrowscale/2);  
            area.offset=new flatpoint(w-signature.trimright,h/3-arrowscale/2);
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
            area.FindBBox();

             //------margins
            area=control(SP_Margin_Top);
            area.hotspot=new flatpoint(arrowscale/2,0);
            area.tail   =new flatpoint(arrowscale/2,arrowscale);
            area.offset=new flatpoint(w*2/3-arrowscale/2,h-signature.margintop);
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
            area.FindBBox();

            area=control(SP_Margin_Bottom);
            area.hotspot=new flatpoint(arrowscale/2,arrowscale);
            area.tail   =new flatpoint(arrowscale/2,0);
            area.offset=new flatpoint(w*2/3-arrowscale/2,signature.marginbottom-arrowscale);
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
            area.FindBBox();

            area=control(SP_Margin_Left);
            area.hotspot=new flatpoint(arrowscale,arrowscale/2);
            area.tail   =new flatpoint(0,arrowscale/2);
            area.offset=new flatpoint(signature.marginleft-arrowscale,h*2/3-arrowscale/2);
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
            area.FindBBox();

            area=control(SP_Margin_Right);
            area.hotspot=new flatpoint(0,arrowscale/2);
            area.tail   =new flatpoint(arrowscale,arrowscale/2);
            area.offset=new flatpoint(w-signature.marginright,h*2/3-arrowscale/2);
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
            area.FindBBox();

             //--------binding
            area=control(SP_Binding);
            //area.Points(null,4,1); //doesn't actually need points, it is checked for very manually
            area.hidden=!(hasfinal && foldlevel==signature.folds.size());
        } //page area items
    } //remapHandles()

   //! For controls that are wheel accessible. dir can be 1 or -1.
    /*! Return 0 for control adjusted, or 1 for not able to do that to that control.
     */
    int adjustControl(int handle, int dir) //:adjustControl
    {
        if (handle==SP_None) return 1;

        if (handle==SP_Sheets_Per_Sig) {
            if (dir==-1) {
                signature.sheetspersignature--;
                if (signature.sheetspersignature<=0) {
                    signature.autoaddsheets=true;
                    signature.sheetspersignature=0;
                }
                needtodraw=1;
                return 0;

            } else {
                signature.sheetspersignature++;
                signature.autoaddsheets=false;
                needtodraw=1;
                return 0;
            }

        } else if (handle==SP_Tile_X_top || handle==SP_Tile_X_bottom) {
            if (dir==-1) {
                signature.tilex--;
                if (signature.tilex<=1) signature.tilex=1;
            } else {
                signature.tilex++;
            }
            remapHandles(0);
            needtodraw=1;
            return 0;

        } else if (handle==SP_Tile_Y_left || handle==SP_Tile_Y_right) {
            if (dir==-1) {
                signature.tiley--;
                if (signature.tiley<=1) signature.tiley=1;
            } else {
                signature.tiley++;
            }
            remapHandles(0);
            needtodraw=1;
            return 0;

        } else if (handle==SP_H_Folds_left || handle==SP_H_Folds_right) {
            if (foldlevel!=0) {
                signature.resetFoldinfo(null);
                foldlevel=0;
            }

            if (dir==1) {
                if (foldlevel!=0) return 0;
                signature.numhfolds++;
                reallocateFoldinfo();
                checkFoldLevel();

            } else {
                if (foldlevel!=0) return 0;
                int old=signature.numhfolds;
                signature.numhfolds--;
                if (signature.numhfolds<=0) signature.numhfolds=0;
                if (old!=signature.numhfolds) {
                    reallocateFoldinfo();
                    checkFoldLevel();
                    needtodraw=1;
                }
            }
            remapHandles(0);
            needtodraw=1;
            return 0;

        } else if (handle==SP_V_Folds_top || handle==SP_V_Folds_bottom) {
            if (foldlevel!=0) {
                signature.resetFoldinfo(null);
                foldlevel=0;
            }

            if (dir==1) {
                if (foldlevel!=0) return 0;
                signature.numvfolds++;
                reallocateFoldinfo();
                checkFoldLevel();

            } else {
                if (foldlevel!=0) return 0;
                int old=signature.numvfolds;
                signature.numvfolds--;
                if (signature.numvfolds<=0) signature.numvfolds=0;
                if (old!=signature.numvfolds) {
                    reallocateFoldinfo();
                    checkFoldLevel();
                    needtodraw=1;
                }
            }
            remapHandles(0);
            needtodraw=1;
            return 0;

        } else if (handle==SP_Num_Pages) {
            if (dir==1) {
                signature.hint_numpages+=signature.PagesPerPattern();
            } else {
                signature.hint_numpages-=signature.PagesPerPattern();
                if (signature.hint_numpages<1) signature.hint_numpages=1;
            }
            signature.sheetspersignature=int((signature.hint_numpages-1)/signature.PagesPerPattern()) + 1;
            signature.hint_numpages=signature.sheetspersignature * signature.PagesPerPattern();
            //signature.hint_numsigs=1+(signature.hint_numpages-1)/signature.PagesPerSignature();

            ActionArea area=control(SP_Num_Pages);
            String buffer;
            buffer=""+(signature.hint_numpages)+" pages";
            area.text=buffer;

                        remapHandles(0);

            //area=control(SP_Num_Sigs);
            //buffer=""+(signature.hint_numsigs)+" pages";
            //area.text=buffer;

            needtodraw=1;
            return 0;

        } else if (handle==SP_Num_Sigs) {
            if (dir>0) {
                signature.hint_numsigs++;
            } else {
                signature.hint_numsigs--;
                if (signature.hint_numsigs<1) signature.hint_numsigs=1;
            }
            signature.hint_numpages=signature.PagesPerSignature()*signature.hint_numsigs;

            ActionArea area=control(SP_Num_Sigs);
            String buffer=""+(signature.hint_numsigs)+" pages";
            area.text=buffer;

            area=control(SP_Num_Pages);
            buffer=""+(signature.hint_numpages)+" pages";
            area.text=buffer;
            needtodraw=1;
            return 0;


        } else if (handle==SP_Current_Sheet) {
            currentPaperSpread+=(dir>0?1:-1);
            if (currentPaperSpread>=2*signature.sheetspersignature) currentPaperSpread=0;
            else if (currentPaperSpread<0) currentPaperSpread=(signature.sheetspersignature-1)*2+1;
            if (foldlevel!=0) {
                signature.resetFoldinfo(null);
                foldlevel=0;
            }
            remapHandles(0);
            needtodraw=1;
            return 0;
                        
        } else if (handle==SP_Front_Or_Back) {
            if (currentPaperSpread%2==1) currentPaperSpread--; else currentPaperSpread++;
            if (foldlevel!=0) {
                signature.resetFoldinfo(null);
                foldlevel=0;
            }
            remapHandles(0);
            needtodraw=1;
            return 0;
                        
        } else if (handle==SP_Paper_Name) {
            ChangePaper((dir>0?-1:-2), currentPaperWidth,currentPaperHeight,currentPaperUnits);
            signature.SetPaper(currentPaperWidth,currentPaperHeight);
            PerformAction(SIA_Center);
            needtodraw=1;
            return 0;
                        
        } else if (handle==SP_Paper_Orient) {
            ChangePaperOrientation();
            signature.SetPaper(currentPaperWidth,currentPaperHeight);
            PerformAction(SIA_Center);
            needtodraw=1;
            return 0;
                        
        }

        return 1;
    } //adjustControl(int handle, int dir)


        int offsetHandle(int which, flatpoint d)
        {
          return offsetHandle(which,d,0,0);
        }

    //! Move a handle.
    /*! Return 0 for offsetted, or 1 for could not.
     */
    int offsetHandle(int which, flatpoint d, int screen_dx, int screen_dy) //:offsetHandle
    { 
      ActionArea area=control(which);

      if (area.type==AREA_Handle) {

        area.offset.add(d);
        Signature s=signature;
        String scratch;

        if (which==SP_Inset_Top) {
             //adjust signature
            signature.insettop-=d.y;
            if (signature.insettop<0) {
                signature.insettop=0;
            } else if (s.insettop>s.totalheight - ((s.tiley-1)*s.tilegapy+s.insetbottom)) {
                s.insettop= s.totalheight - ((s.tiley-1)*s.tilegapy+s.insetbottom);
            }

             //adjust handle to match signature
            d=area.Position();
            if (d.x<0) area.Position(0,0,1);
            else if (d.x>signature.totalwidth) area.Position(signature.totalwidth,0,1);
            area.Position(0,signature.totalheight-signature.insettop,2);

            scratch="Top Inset "+(s.insettop);
            PostMessage(scratch);

            remapHandles(2|4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Inset_Bottom  ) {
             //adjust signature
            signature.insetbottom+=d.y;
            if (signature.insetbottom<0) {
                signature.insetbottom=0;
            } else if (s.insetbottom>s.totalheight - ((s.tiley-1)*s.tilegapy+s.insettop)) {
                s.insetbottom= s.totalheight - ((s.tiley-1)*s.tilegapy+s.insettop);
            }

             //adjust handle to match signature
            d=area.Position();
            if (d.x<0) area.Position(0,0,1);
            else if (d.x>signature.totalwidth) area.Position(signature.totalwidth,0,1);
            area.Position(0,signature.insetbottom,2);

            scratch="Bottom Inset "+s.insetbottom;
            PostMessage(scratch);

            remapHandles(2|4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Inset_Left    ) {
            signature.insetleft+=d.x;
            if (signature.insetleft<0) {
                signature.insetleft=0;
            } else if (s.insetleft>s.totalwidth - ((s.tilex-1)*s.tilegapx+s.insetright)) {
                s.insetleft= s.totalwidth - ((s.tilex-1)*s.tilegapx+s.insetright);
            }

             //adjust handle to match signature
            d=area.Position();
            if (d.y<0) area.Position(0,0,2);
            else if (d.y>signature.totalheight) area.Position(0,signature.totalheight,2);
            area.Position(signature.insetleft,0,1);

            scratch="Left Inset "+s.insetleft;
            PostMessage(scratch);

            remapHandles(2|4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Inset_Right   ) {
            signature.insetright-=d.x;
            if (signature.insetright<0) {
                signature.insetright=0;
            } else if (s.insetright>s.totalwidth - ((s.tilex-1)*s.tilegapx+s.insetleft)) {
                s.insetright= s.totalwidth - ((s.tilex-1)*s.tilegapx+s.insetleft);
            }

             //adjust handle to match signature
            d=area.Position();
            if (d.y<0) area.Position(0,0,2);
            else if (d.y>signature.totalheight) area.Position(0,signature.totalheight,2);
            area.Position(signature.totalwidth-signature.insetright,0,1);

            scratch="Right Inset "+s.insetright;
            PostMessage(scratch);

            remapHandles(2|4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Tile_Gap_X    ) {
            s.tilegapx+=d.x;

            if ((s.tilex-1)*s.tilegapx > s.totalwidth-s.insetleft-s.insetright+s.tilex*s.PatternWidth()) {
                if (s.tilex==1) s.tilegapx=0;
                else s.tilegapx=(s.totalwidth-s.insetleft-s.insetright+s.tilex*s.PatternWidth())/(s.tilex-1);
            } else if (s.tilegapx<0) s.tilegapx=0;

            scratch="Tile gap "+s.tilegapx;
            PostMessage(scratch);

            remapHandles(2|4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Tile_Gap_Y    ) {
            s.tilegapy+=d.y;

            if ((s.tiley-1)*s.tilegapy > s.totalheight-s.insettop-s.insetbottom+s.tiley*s.PatternHeight()) {
                if (s.tiley==1) s.tilegapy=0;
                else s.tilegapy=(s.totalheight-s.insettop-s.insetbottom+s.tiley*s.PatternHeight())/(s.tiley-1);
            } else if (s.tilegapy<0) s.tilegapy=0;

            scratch="Tile gap "+s.tilegapy;
            PostMessage(scratch);

            remapHandles(2|4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Trim_Top      ) {
            s.trimtop-=d.y;

            float h=s.PageHeight(0);
            if (s.trimtop<0) s.trimtop=0;
            else if (s.trimtop>h-s.trimbottom) s.trimtop=h-s.trimbottom;

            scratch="Top Trim "+s.trimtop;
            PostMessage(scratch);

            remapHandles(4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Trim_Bottom   ) {
            s.trimbottom+=d.y;

            float h=s.PageHeight(0);
            if (s.trimbottom<0) s.trimbottom=0;
            else if (s.trimbottom>h-s.trimtop) s.trimbottom=h-s.trimtop;

            scratch="Bottom Trim "+s.trimbottom;
            PostMessage(scratch);

            remapHandles(4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Trim_Left     ) {
            s.trimleft+=d.x;

            float w=s.PageWidth(0);
            if (s.trimleft<0) s.trimleft=0;
            else if (s.trimleft>w-s.trimright) s.trimleft=w-s.trimright;

            scratch="Left Trim "+s.trimleft;
            PostMessage(scratch);

            remapHandles(4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Trim_Right    ) {
            s.trimright-=d.x;

            float w=s.PageWidth(0);
            if (s.trimright<0) s.trimright=0;
            else if (s.trimright>w-s.trimleft) s.trimright=w-s.trimleft;

            scratch="Right Trim "+s.trimright;
            PostMessage(scratch);

            remapHandles(4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Margin_Top      ) {
            s.margintop-=d.y;

            float h=s.PageHeight(0);
            if (s.margintop<0) s.margintop=0;
            else if (s.margintop>h-s.marginbottom) s.margintop=h-s.marginbottom;

            scratch="Top Margin "+s.margintop;
            PostMessage(scratch);

            remapHandles(4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Margin_Bottom   ) {
            s.marginbottom+=d.y;

            float h=s.PageHeight(0);
            if (s.marginbottom<0) s.marginbottom=0;
            else if (s.marginbottom>h-s.margintop) s.marginbottom=h-s.margintop;

            scratch="Bottom Margin "+s.marginbottom;
            PostMessage(scratch);

            remapHandles(4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Margin_Left     ) {
            s.marginleft+=d.x;

            float w=s.PageWidth(0);
            if (s.marginleft<0) s.marginleft=0;
            else if (s.marginleft>w-s.marginright) s.marginleft=w-s.marginright;

            scratch="Left Margin "+s.marginleft;
            PostMessage(scratch);

            remapHandles(4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Margin_Right    ) {
            s.marginright-=d.x;

            float w=s.PageWidth(0);
            if (s.marginright<0) s.marginright=0;
            else if (s.marginright>w-s.marginleft) s.marginright=w-s.marginleft;

            scratch="Right Margin "+s.marginright;
            PostMessage(scratch);

            remapHandles(4);
            needtodraw=1;
            return 0;

        } else if (which==SP_Binding       ) {
        }
        
      } //if area.type==AREA_Handle
      
      if (area.type==AREA_H_Slider || area.type==AREA_Slider) {
                int slidestep=15;
                
                int oldpos=floor((pmouseX-initialmousex)/(float)slidestep);
                int newpos=floor(( mouseX-initialmousex)/(float)slidestep);
                
                if (newpos>oldpos) adjustControl(which,1);
                else if (newpos<oldpos) adjustControl(which,-1);
                needtodraw=1;
                return 1;
                
      } else if (area.type==AREA_V_Slider) {
                int slidestep=15;
                
                int oldpos=floor((pmouseY-initialmousey)/(float)slidestep);
                int newpos=floor(( mouseY-initialmousey)/(float)slidestep);
                
                if (newpos>oldpos) adjustControl(which,-1);
                else if (newpos<oldpos) adjustControl(which,1);
                needtodraw=1;
                return 1;
      }

        
       return 1;

    } //offsetHandle(int which, flatpoint d)


    int MouseDrag(int x,int y,int state)
    {
         //println("MouseDrag... onoverlay:"+onoverlay+"  lbrow,col: "+lbdown_row+','+lbdown_col);
                
        float[] ret=scan(x,y);
        int over=(int)(ret[0]);
        int row=(int)(ret[1]);
        int col=(int)(ret[2]);
        flatpoint mm=new flatpoint(ret[3],ret[4]);
        int tilerow=(int)(ret[5]);
        int tilecol=(int)(ret[6]);
        

        //DBG cerr <<"over element "<<over<<": r,c="<<row<<','<<col<<"  mm="<<mm.x<<','<<mm.y<<"  tile r,c:"<<tilerow<<','<<tilecol;
        //DBG if (row>=0 && row<signature.numhfolds+1 && col>=0 && col<signature.numvfolds+1)
        //DBG    cerr <<"  xflip: "<<foldinfo[row][col].x_flipped<<"  yflip:"<<foldinfo[row][col].y_flipped
        //DBG         <<"  pages:"<<foldinfo[row][col].pages.n<<endl;

        int mx,my;
        int lx=pmouseX;
        int ly=pmouseY;
        
        if (mouseButton==RIGHT || (mouseButton==LEFT && onoverlay==SP_None && (lbdown_row<0 || lbdown_col<0))) {
             //viewport shifting
            if (((state&(ControlMask|ShiftMask))|ShiftMask)==ShiftMask) {
                dp.ShiftScreen(x-lx,y-ly);
                //println("ShiftScreen...");
                needtodraw=1;
            } else if ((state&(ControlMask|ShiftMask))==ControlMask) {
                flatpoint center=dp.screentoreal(x,y);
                if (x-lx>0) dp.Zoom(1+(x-lx)*.1);
                else if (x-lx<0) dp.Zoom(1/(1+(-x+lx)*.1));
                flatpoint c2=dp.realtoscreen(center.x,center.y); 
                dp.ShiftScreen(int(x-c2.x), int(y-c2.y));
                needtodraw=1;
            }
            
            flatpoint off=dp.realtoscreen(-signature.totalwidth*.1,0);
            off.x-=INDICATOR_SIZE*6;
            folderoffset=(int)off.x;
            if (folderoffset<0) folderoffset=0;

            return 0;
        }
        
        if (mouseButton!=LEFT) return 0;



        if (onoverlay!=SP_None) {
            if (onoverlay<SP_FOLDS) {
                                 //dragging a handle or a slider
                flatpoint d=dp.screentoreal(x,y);
                d.subtract(dp.screentoreal(lx,ly));

                if ((state&(ControlMask|ShiftMask))==ShiftMask) d.multiply(.1);
                if ((state&(ControlMask|ShiftMask))==ControlMask) d.multiply(.01);
                if ((state&(ControlMask|ShiftMask))==(ControlMask|ShiftMask)) d.multiply(.001);

                if (onoverlay==SP_Binding) {
                    d=dp.screentoreal(x,y);
                    Signature s=signature;
                    d.x-=s.insetleft  +activetilex*(s.PatternWidth() +s.tilegapx) + (finalc+.5)*s.PageWidth(0);
                    d.y-=s.insetbottom+activetiley*(s.PatternHeight()+s.tilegapy) + (finalr+.5)*s.PageHeight(0);
                    d.x/=s.PageWidth(0);
                    d.y/=s.PageHeight(0);
                    if (d.y>d.x) {
                        if (d.y>-d.x) { if (s.binding!='t') needtodraw=1; s.binding='t'; }
                        else  { if (s.binding!='l') needtodraw=1; s.binding='l'; }
                    } else {
                        if (d.y>-d.x) { if (s.binding!='r') needtodraw=1; s.binding='r'; }
                        else  { if (s.binding!='b') needtodraw=1; s.binding='b'; }
                    }
                } else {
                    offsetHandle(onoverlay,d, x-mslidex,y-mslidey);
                    
                } 
                return 0;


            } else if (onoverlay>=SP_FOLDS) {
                  // the cool stuff: --- drag a fold ----
                if (ly-y==0 || signature.folds.size()==0) return 0; //return if not moved vertically

                int startindicator;
                mx=initialmousex;
                my=initialmousey;
                int lasty=pmouseY;
                
                startindicator=mouseinfo1;

                float curdist =(    y-my)/(2.*INDICATOR_SIZE-3) + startindicator;
                float lastdist=(lasty-my)/(2.*INDICATOR_SIZE-3) + startindicator;

                if (curdist<0) curdist=0;
                else if (curdist>signature.folds.size()) curdist=signature.folds.size();
                if (lastdist<0) lastdist=0;
                else if (lastdist>signature.folds.size()) lastdist=signature.folds.size();

                 //curdist is 0 for very start, totally unfolded paper. Each fold adds 1 to curdist

                //DBG cerr <<"curdist:"<<curdist<<"  lastdist:"<<lastdist<<endl;

                if (foldprogress==-1) {
                     //we have not moved before, so we must do an initial map of affected cells
                    remapAffectedCells((int)curdist);
                }
                foldprogress=curdist-floor(curdist);

                if ((int)curdist==(int)lastdist || (lastdist<0 && curdist<0) 
                        || (lastdist>=signature.folds.size() && curdist>=signature.folds.size())) {
                     //only change foldprogress, still in same fold indicator
                    foldprogress=curdist-floor(curdist);
                    if (foldprogress<0) foldprogress=0;
                    if (foldprogress>1) foldprogress=1;

                    int f=(int)curdist;
                    if (f<0) f=0; else if (f>=signature.folds.size()) f=signature.folds.size()-1;
                    folddirection=((Fold)signature.folds.get(f)).direction;
                    foldunder=((Fold)signature.folds.get(f)).under;
                    foldindex=((Fold)signature.folds.get(f)).whichfold;

                    needtodraw=1;
                    return 0;

                } else if ((int)curdist<(int)lastdist) {
                     //need to unfold a previous fold, for this, we need to get the map of state not including
                     //the current fold, to know which cells are actually affected by the unfolding..
                    foldlevel=(int)curdist;
                    signature.applyFold(null,(int)curdist);
                    remapAffectedCells((int)curdist);
                    foldprogress=curdist-floor(curdist);
                    remapHandles(0);
                    needtodraw=1;
                    return 0;

                } else {
                     //need to advance one fold
                    if (curdist>signature.folds.size()) curdist=signature.folds.size();
                    foldlevel=(int)curdist;
                    folddirection='x';
                    applyFold((Fold)signature.folds.get((int)curdist-1));
                    remapAffectedCells((int)curdist);
                    foldprogress=curdist-floor(curdist);
                    remapHandles(0);
                    needtodraw=1;
                    return 0;
                }
            }
            return 0;
        }


                
        if ((hasfinal && foldlevel==signature.folds.size())
                || row<0 || row>signature.numhfolds || col<0 || col>signature.numvfolds) {
             //not folding things..
            if (folddirection!=0) {
                folddirection=0;
                needtodraw=1;
            }
            return 0;
        }


         //...so we've clicked within the pattern and are dragging a fold
        foldunder=((state&(ControlMask|ShiftMask))!=0);

        mx=initialmousex;
        my=initialmousey;
        
        ret=scan(mx,my);
        //int over=ret[0];
        int orow=(int)(ret[1]);
        int ocol=(int)(ret[2]);
        flatpoint om=new flatpoint(ret[3],ret[4]);
        int otrow=(int)(ret[5]);
        int otcol=(int)(ret[6]);


        if (tilerow<otrow) row-=100;
        else if (tilerow>otrow) row+=100;
        if (tilecol<otcol) col-=100;
        else if (tilecol>otcol) col+=100;
        

        flatpoint d=dp.screentoreal(x,y);
        d.subtract(dp.screentoreal(mx,my));

         //find the direction we are trying to fold in
        if (folddirection==0) {
            if (abs(d.x)>abs(d.y)) {
                if (d.x>0) {
                    folddirection='r';
                } else if (d.x<0) {
                    folddirection='l';
                }
            } else {
                if (d.y>0) {
                    folddirection='t';
                } else if (d.y<0) {
                    folddirection='b';
                }
            }
        }

         //find how far we fold based on proximity of mouse to fold crease

         //figure out which elements are affected by folding
        float elementwidth =signature.PageWidth(0);
        float elementheight=signature.PageHeight(0);
        if (folddirection=='r') {
            int adjacentcol=ocol+1; //edge is between ocol and adjacentcol
            int prevcol=ocol;
            while (prevcol>0 && ifoldinfo[orow][prevcol-1].pages.size()!=0) prevcol--;
            int nextcol=adjacentcol;
            while (nextcol<signature.numvfolds) nextcol++; //it's ok to fold over onto blank areas

            if (nextcol>signature.numvfolds || nextcol-adjacentcol+1<ocol-prevcol+1
                    || (adjacentcol<=signature.numvfolds && ifoldinfo[orow][adjacentcol].pages.size()==0)) {
                 //can't do the fold
                folddirection='x';
            } else {
                 //we can do the fold
                foldindex=ocol+1;

                 //find the fold progress
                if (col==ocol) foldprogress=.5-(elementwidth-mm.x)/(elementwidth-om.x)/2;
                else if (col==ocol+1) foldprogress=.5+(mm.x)/(elementwidth-om.x)/2;
                else if (col<ocol) foldprogress=0;
                else foldprogress=1;
                if (foldprogress>1) foldprogress=1;
                if (foldprogress<0) foldprogress=0;

                 //need to find upper and lower affected elements
                foldc1=prevcol;
                foldc2=ocol;
                foldr1=orow;
                foldr2=orow;
                while (foldr1>0 && ifoldinfo[foldr1-1][ocol].pages.size()!=0) foldr1--;
                while (foldr2<signature.numhfolds && ifoldinfo[foldr2+1][ocol].pages.size()!=0) foldr2++;
            }
            needtodraw=1;

        } else if (folddirection=='l') {
            int adjacentcol=ocol-1; //edge is between ocol and adjacentcol
            int nextcol=ocol;
            while (nextcol<signature.numvfolds && ifoldinfo[orow][nextcol+1].pages.size()!=0) nextcol++;
            int prevcol=adjacentcol;
            while (prevcol>0) prevcol--; //it's ok to fold over onto blank areas

            if (prevcol<0 || adjacentcol-prevcol+1<nextcol-ocol+1
                    || (adjacentcol>=0 && ifoldinfo[orow][adjacentcol].pages.size()==0)) {
                 //can't do the fold
                folddirection='x';
            } else {
                 //we can do the fold
                foldindex=ocol;

                 //find the fold progress
                if (col==ocol) foldprogress=.5-mm.x/om.x/2;
                else if (col==ocol-1) foldprogress=.5+(elementwidth-mm.x)/om.x/2;
                else if (col>ocol) foldprogress=0;
                else foldprogress=1;
                if (foldprogress>1) foldprogress=1;
                if (foldprogress<0) foldprogress=0;

                 //need to find upper and lower affected elements
                foldc1=ocol;
                foldc2=nextcol;
                foldr1=orow;
                foldr2=orow;
                while (foldr1>0 && ifoldinfo[foldr1-1][ocol].pages.size()!=0) foldr1--;
                while (foldr2<signature.numhfolds && ifoldinfo[foldr2+1][ocol].pages.size()!=0) foldr2++;
            }
            needtodraw=1;

        } else if (folddirection=='t') {
            int adjacentrow=orow+1; //edge is between ocol and adjacentcol
            int prevrow=orow;
            while (prevrow>0 && ifoldinfo[prevrow-1][ocol].pages.size()!=0) prevrow--;
            int nextrow=adjacentrow;
            while (nextrow<signature.numhfolds) nextrow++; //it's ok to fold over onto blank areas

            if (nextrow>signature.numhfolds || nextrow-adjacentrow+1<orow-prevrow+1
                    || (adjacentrow<=signature.numhfolds && ifoldinfo[adjacentrow][ocol].pages.size()==0)) {
                 //can't do the fold
                folddirection='x';
            } else {
                 //we can do the fold
                foldindex=orow+1;

                 //find the fold progress
                if (row==orow) foldprogress=.5-(elementheight-mm.y)/(elementheight-om.y)/2;
                else if (row==orow+1) foldprogress=.5+(mm.y)/(elementheight-om.y)/2;
                else if (row<orow) foldprogress=0;
                else foldprogress=1;
                if (foldprogress>1) foldprogress=1;
                if (foldprogress<0) foldprogress=0;

                 //need to find upper and lower affected elements
                foldr1=prevrow;
                foldr2=orow;
                foldc1=ocol;
                foldc2=ocol;
                while (foldc1>0 && ifoldinfo[orow][foldc1-1].pages.size()!=0) foldc1--;
                while (foldc2<signature.numvfolds && ifoldinfo[orow][foldc2+1].pages.size()!=0) foldc2++;
            }
            needtodraw=1;

        } else if (folddirection=='b') {
            int adjacentrow=orow-1; //edge is between orow and adjacentrow
            int nextrow=orow;
            while (nextrow<signature.numhfolds && ifoldinfo[nextrow+1][ocol].pages.size()!=0) nextrow++;
            int prevrow=adjacentrow;
            while (prevrow>0) prevrow--; //it's ok to fold over onto blank areas

            if (prevrow<0 || adjacentrow-prevrow+1<nextrow-orow+1
                    || (adjacentrow>=0 && ifoldinfo[adjacentrow][ocol].pages.size()==0)) {
                 //can't do the fold
                folddirection='x';
            } else {
                 //we can do the fold
                foldindex=orow;

                 //find the fold progress
                if (row==orow) foldprogress=.5-mm.y/om.y/2;
                else if (row==orow-1) foldprogress=.5+(elementheight-mm.y)/om.y/2;
                else if (row>orow) foldprogress=0;
                else foldprogress=1;
                if (foldprogress>1) foldprogress=1;
                if (foldprogress<0) foldprogress=0;

                 //need to find upper and lower affected elements
                foldr1=orow;
                foldr2=nextrow;
                foldc1=ocol;
                foldc2=ocol;
                while (foldc1>0 && ifoldinfo[orow][foldc1-1].pages.size()!=0) foldc1--;
                while (foldc2<signature.numvfolds && ifoldinfo[orow][foldc2+1].pages.size()!=0) foldc2++;
            }
            needtodraw=1;
        }

        //DBG cerr <<"folding progress: "<<foldprogress<<",  om="<<om.x<<','<<om.y<<"  mm="<<mm.x<<','<<mm.y<<endl;


        return 0;
    }



}; //class SignatureInterface













