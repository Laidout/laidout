#!/usr/bin/python

"""
./makeimages.py            <-- make all icons
./makeimages.py 24         <-- make all icons this many pixels wide
./makeimages.py IconId     <-- make only this icon
./makeimages.py 24 IconId  <-- make only this icon, and that wide

Using Inkscape, look in icons.svg and take all top items in base
layers of the document that have ids starting with capital letters,
and export the half inch area around them to TheId.png.

Someday I hope to have something to generate the icons internally to
Laidout, the icons should be able to be generated from svg-ish (or
laidout-ish vector specs fast enough to not be irritating maybe.

Tested to run with python 2.7 and above (including python 3).
"""

import subprocess, sys
#from xml.sax import saxexts
import xml.sax
import types
import locale

#not sure if this is necessary: seems to work for me with or without under a french locale
locale.setlocale(locale.LC_ALL, '')

makethisonly="" #maybe select only one icon to generate
bitmapw=24

if (len(sys.argv)>1) :
    arg=sys.argv[1]
    if (arg[0]>='A' and arg[0]<='Z') :
        makethisonly=arg
        print ("Make only: "+makethisonly)
    else :
        bitmapw=int(sys.argv[1])
        if (len(sys.argv)>2) :
            arg=sys.argv[2]
            if (arg[0]>='A' and arg[0]<='Z') : makethisonly=arg

print ("Make icons this many pixels wide: "+str(bitmapw))

dpi = int(96.0*bitmapw/48)
DOCUMENTHEIGHT=17

#get names
names=[]

depth=0
class SAXtracer (xml.sax.handler.ContentHandler):

    def __init__(self,objname):
        self.objname=objname
        self.met_name=""

    def __getattr__(self,name):
        self.met_name=name # UGLY! :)
        return self.trace

    def endElement(self, name):
        globals()["depth"]=globals()["depth"]-1
        #print "depth=",globals()["depth"]

    def startElement(self, name, attrs):
        globals()["depth"]=globals()["depth"]+1
        #print "depth=",globals()["depth"]
  
        #print "depth="+str(globals()["depth"])+", attr name:"+name
        #print "attrs="+str(attrs)
        if (name=="svg"):
           global DOCUMENTHEIGHT
           height = attrs.get("height");
           if (height.find('in')>0):
               height = height[0:-2]
               DOCUMENTHEIGHT=int(height)
           else:
               DOCUMENTHEIGHT=int(height)/96.0
           print("found doc height: "+str(DOCUMENTHEIGHT)+" "+height)
           
        if (globals()["depth"]!=3) : return 


        id=attrs.get("id")
        if (type(id)==type(None)) : return
        if (id=="") : return
        if (makethisonly!="" and id!=makethisonly) : return

         #there is an element that is of depth 3 whose id is capitalized
         #so we specifically have to skip that
        if (id[0]>='A' and id[0]<='Z' and id.find("GridFrom")<0) : 
            names.append(id)
            print (id)

    def trace(self,*rest):
        return

# --- Main prog

print ("")
print ("Detecting icons to make:")
p=xml.sax.make_parser();
curHandler=SAXtracer("my parser")
p.setContentHandler(curHandler)
p.parse(open("icons.svg"))

print ("")
print ("Making icons:")
for name in names :
    print ("")
    X=int(float(subprocess.check_output("inkscape -I "+name+" -X icons.svg", shell=True)))
    Y=int(float(subprocess.check_output("inkscape -I "+name+" -Y icons.svg", shell=True)))
    W=int(float(subprocess.check_output("inkscape -I "+name+" -W icons.svg", shell=True)))
    H=int(float(subprocess.check_output("inkscape -I "+name+" -H icons.svg", shell=True)))

    print ("raw inkscape coords xywh: "+str(X)+","+str(Y)+" "+str(W)+","+str(H))
    x1 = int(X/48)*48
    #y1 = int(Y/48)*48
    print ('doc height: '+str(DOCUMENTHEIGHT))
    y1 = DOCUMENTHEIGHT*96-int(Y/48)*48-48
    renderwidth =bitmapw*(1+int(W/48))
    renderheight=bitmapw*(1+int(H/48))
    x2=x1+48*(1+int(W/48))
    y2=y1+48*(1+int(H/48))

    print ("x1,y1:"+str(x1)+","+str(y1)+"  x2,y2:"+str(x2)+"x"+str(y2))

    command="inkscape -a "+str(x1)+":"+str(y1)+":"+str(x2)+":"+str(y2)+" -w "+ \
        str(renderwidth)+" -h "+str(renderheight)+" -e "+name+".png icons.svg"

    print (command)
    print (subprocess.call(command, shell=True))
   
     #do something special for main Laidout icon, create a 48x48, 
     #which gets placed in /usr/share/icons/hicolor/48x48/apps/laidout.png
    if (name=="LaidoutIcon"):
        command="inkscape -a "+str(x1)+":"+str(y1)+":"+str(x2)+":"+str(y2)+" -w 48 -h 48 -e laidout-48x48.png icons.svg"

        print (command)
        print (subprocess.call(command, shell=True))
