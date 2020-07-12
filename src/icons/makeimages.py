#!/usr/bin/python3

"""
./makeimages.py                     <-- make all icons
./makeimages.py 24                  <-- make all icons this many pixels wide
./makeimages.py IconId              <-- make only this icon
./makeimages.py 24 IconId           <-- make only this icon, and that wide
./makeimages.py --file file.svg     <-- use "file.svg" instead
./makeimages.py 24 --file file.svg  <-- use "file.svg", and make icon base unit 24 px
./makeimages.py 24 --file file.svg 100 --file otherfile.svg

Look in the specified svg file, and take all top items in base
layers of the document that have ids starting with capital letters,
and export the half inch area around them to TheId.png.

Note this requires ../laidout to do the actual generating.

developer note: if you change this file, also update laxkit/lax/icons/makeimages.py
and vice versa.
"""

import subprocess, sys
#from xml.sax import saxexts
import xml.sax
import types
import locale

iconfile = "icons.svg"

makethisonly = "" #maybe select only one icon to generate
bitmapw = 24 #default pixel unit width


use_inkscape = False #else use Laidout
inkscape_1_0 = True

#not sure if this is necessary: seems to work for me with or without under a french locale
locale.setlocale(locale.LC_ALL, '')


##--------------- Parse args --------------------
skip = False
files = []
pxs = []
for c in range(1, len(sys.argv)):
    if skip:
        skip = False
        continue
    arg = sys.argv[c]
    if arg == "--file" or arg == "-f":
        iconfile = sys.argv[c+1]
        files.append(iconfile)
        pxs.append(bitmapw)
        skip = True
    elif arg.isdecimal():
        bitmapw = int(arg)
    else:
        makethisonly = arg
        if (makethisonly[-4:] == ".png"): makethisonly = makethisonly[0:-4]
        print ("WARNING!!! make only one currently broken!")

if len(files) == 0:
    files.append(iconfile)
    pxs.append(bitmapw)


#print ("files: ", files, " with pxs: ", pxs)
#print ("Icon file: "+iconfile)
#print ("Make icons this many pixels wide: "+str(bitmapw))
if (makethisonly != ""): print ("Make only: "+makethisonly)

print ("Build icons:")
for c in range(len(files)):
    print ("  ", pxs[c],"px,  ", files[c])


if use_inkscape:
    # DEPRECATED!!!!!!
    #
    #
    # If you don't have Inkscape 0.92 or newer, use the "old inkscape" one below
    #
    PPU = 96.0 #new inkscape, >= 0.92
    #PPU = 90.0 #old inkscape

    HALFPPU = PPU/2

    DOCUMENTHEIGHT = 17 #in inches, just a hint here, gets read in later

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
                   DOCUMENTHEIGHT = int(height)
               else:
                   DOCUMENTHEIGHT = int(height)/PPU
               print("found doc height: "+str(DOCUMENTHEIGHT)+" inches from "+height)
               
            if (globals()["depth"]!=3) : return 
            if (name == "marker") : return #prevents rendering marker defs!

            id = attrs.get("id")
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
    p.parse(open(iconfile))

    print ("")
    print ("Making",len(names),"icons:")
    for name in names :
        print ("")
        X=int(float(subprocess.check_output("inkscape -I "+name+" -X "+iconfile + " | tail -n 1", shell=True)))
        Y=int(float(subprocess.check_output("inkscape -I "+name+" -Y "+iconfile + " | tail -n 1", shell=True)))
        W=int(float(subprocess.check_output("inkscape -I "+name+" -W "+iconfile + " | tail -n 1", shell=True)))
        H=int(float(subprocess.check_output("inkscape -I "+name+" -H "+iconfile + " | tail -n 1", shell=True)))

        print ("raw inkscape coords xywh: "+str(X)+","+str(Y)+" "+str(W)+","+str(H))
        print ('doc height: '+str(DOCUMENTHEIGHT))

        x1 = int(int(X/HALFPPU)*HALFPPU)
        if inkscape_1_0:
            y1 = int(HALFPPU + int(Y/HALFPPU)*HALFPPU-HALFPPU)
        else: #### inkscape 0.92:
            y1 = int(DOCUMENTHEIGHT*PPU-int(Y/HALFPPU)*HALFPPU-HALFPPU)
        renderwidth  = bitmapw*(1+int(W/HALFPPU))
        renderheight = bitmapw*(1+int(H/HALFPPU))
        x2 = int(x1+HALFPPU*(1+int(W/HALFPPU)))
        y2 = int(y1+HALFPPU*(1+int(H/HALFPPU)))

        print ("x1,y1:"+str(x1)+","+str(y1)+"  x2,y2:"+str(x2)+"x"+str(y2))
        print ("icon coords x1,y1:"+str(int(x1*2/PPU)) + "," + str(int(y1*2/PPU)))

        ### inkscape 1.0:
        if inkscape_1_0:
            command="inkscape --export-area="+str(x1)+":"+str(y1)+":"+str(x2)+":"+str(y2)+" -w "+ \
                str(renderwidth)+" -h "+str(renderheight)+" -o "+name+".png "+iconfile
        else:
            ### inkscape 0.92:
            command="inkscape -a "+str(x1)+":"+str(y1)+":"+str(x2)+":"+str(y2)+" -w "+ \
                str(renderwidth)+" -h "+str(renderheight)+" -e "+name+".png "+iconfile

        print (command)
        print (subprocess.call(command, shell=True))
       
         #do something special for main Laidout icon, create a 48x48, 
         #which gets placed in /usr/share/icons/hicolor/48x48/apps/laidout.png
        if (name=="LaidoutIcon"):
            ### inkscape 1.0:
            if inkscape_1_0:
                command="inkscape --export-area="+str(x1)+":"+str(y1)+":"+str(x2)+":"+str(y2)+" -w 48 -h 48 -o laidout-48x48.png "+iconfile
            else:
                ### inkscape 0.92:
                command="inkscape -a "+str(x1)+":"+str(y1)+":"+str(x2)+":"+str(y2)+" -w 48 -h 48 -e laidout-48x48.png "+iconfile

            print (command)
            print (subprocess.call(command, shell=True))

else: ### Use Laidout

    pattern = ""
    if makethisonly: pattern = 'pattern = "'+makethisonly+'",'
    command = "../laidout --command 'Laidout.BuildIcons([{}], output_px_size=[{}], {} output_dir=\".\")'" \
                    .format(",".join('"'+f+'"' for f in files), ",".join(str(i) for i in pxs), pattern)
    print (command)
    print (subprocess.call(command, shell=True))




