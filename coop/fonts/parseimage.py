#!/usr/bin/python

#
# Convert an svg file containing colored glyphs into individual svg files for each glyph.
# Assumes you are ONLY using flat colored paths, no images or path effects.
# Currently ascii only
#
# Output is a ttx file you can convert to woff/otf with the ttx program.
#
# Usage:
#  ./parseimage.py fontletters1.svg [letters directory default="letters"]
#
#   then:
# ttx -f --flavor woff VillaPazza.ttx
#

#ToDO:
# - command line option to specify the ttx template
# - command line option to specify the output ttx file
# - command line option for outputting layers, instead of svg
# - command line option for how many cells to chop the original svg into
# - Do a proper guillotine, not the current major hack that assumes each path object is the whole layer color.
#    current system only works when you use Inkscape to trace bitmap right from image, but then you don't edit
#    the resulting svg at all.
# - auto kerning
# - for glyf table, either convert cubic to quadratic, or figure out if opentype type1 (cubic glyfs) is suitable for
#    the fallback glyf table
# - finish alt glyph system
# - implement the CPAL color name substitution for SVG table: replace found colors in the individual svg
#    with refs to colors in CPAL



import os, sys, re, math, copy


if len(sys.argv)==1:
    print('Usage:\n  parseimage.py svg_master_file.svg [letters directory (defaults to "letters")]')
    sys.exit()


filename = sys.argv[1]

if len(sys.argv)>=3: 
    letterdir= sys.argv[2]
else: letterdir="letters"

print ("filename: "+filename);
print ("svg files to :  "+letterdir);


dolayered = False
#dolayered = False
layer=0

startindex = 32

gridwidth = 16
gridheight = 6


# *** todo: somehow automate namerecord table
#
# [nameid, platformid, platEncId, langid, text]
# platform = 0=unicode, 1=mac, 2=iso(deprecated), 3=windows, 4=custom
# platEncId for 0(unicode): 0=unicode 1.0, 1=unicode 1.1, 2=10646, 3=unicode 2.0 BMP only, 4=unicode 2.0
# platEncId for 3(windows): 1=unicode BMP
# platEndId for 1(mac) should be 0
# for windows, language for usa english is 0x409
#
# 0=copyright, 1=font family, 2=style,        3=unique font id, 4=full font name, 5=Version number.number
# 6=psname,    7=trademark,   8=manufacturer, 9=designer,       10=description,   11=url vendor
# 12=url designer, 13=license desc, 14=license info url, 15=(reserved),
# 16=typographic fam name (def is 1), 17=typo style name (def is 2),
# 18=some mac thing, 19=sample text
namerecord = {}
namerecord["copyright"] = [0, 1, 0, "0x0", "(c) 2016 Tom Lechner. All rights reserved."]



#---------------------
#codes[unicode] == name, description
#glyphmap[name] == glyph index
#hmtx[unicode] == bounds(xmin, xmax,  ymin, ymax)
#glyfs[glyphname] == first non-color path


#------------------- borrowing simplepath.py from inkscape install --------------
"""
simplepath.py
functions for digesting paths into a simple list structure

Copyright (C) 2005 Aaron Spike, aaron@ekips.org

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

"""

def lexPath(d):
    """
    returns and iterator that breaks path data 
    identifies command and parameter tokens
    """
    offset = 0
    length = len(d)
    delim = re.compile(r'[ \t\r\n,]+')
    command = re.compile(r'[MLHVCSQTAZmlhvcsqtaz]')
    parameter = re.compile(r'(([-+]?[0-9]+(\.[0-9]*)?|[-+]?\.[0-9]+)([eE][-+]?[0-9]+)?)')
    while 1:
        m = delim.match(d, offset)
        if m:
            offset = m.end()
        if offset >= length:
            break
        m = command.match(d, offset)
        if m:
            yield [d[offset:m.end()], True]
            offset = m.end()
            continue
        m = parameter.match(d, offset)
        if m:
            yield [d[offset:m.end()], False]
            offset = m.end()
            continue
        #TODO: create new exception
        raise Exception, 'Invalid path data!'
'''
pathdefs = {commandfamily:
    [
    implicitnext,
    #params,
    [casts,cast,cast],
    [coord type,x,y,0]
    ]}
'''
pathdefs = {
    'M':['L', 2, [float, float], ['x','y']], 
    'L':['L', 2, [float, float], ['x','y']], 
    'H':['H', 1, [float], ['x']], 
    'V':['V', 1, [float], ['y']], 
    'C':['C', 6, [float, float, float, float, float, float], ['x','y','x','y','x','y']], 
    'S':['S', 4, [float, float, float, float], ['x','y','x','y']], 
    'Q':['Q', 4, [float, float, float, float], ['x','y','x','y']], 
    'T':['T', 2, [float, float], ['x','y']], 
    'A':['A', 7, [float, float, float, int, int, float, float], ['r','r','a',0,'s','x','y']], 
    'Z':['L', 0, [], []]
    }
def parsePath(d):
    """
    Parse SVG path and return an array of segments.
    Removes all shorthand notation.
    Converts coordinates to absolute.
    """
    retval = []
    lexer = lexPath(d)

    pen = (0.0,0.0)
    subPathStart = pen
    lastControl = pen
    lastCommand = ''
    
    while 1:
        try:
            token, isCommand = lexer.next()
        except StopIteration:
            break
        params = []
        needParam = True
        if isCommand:
            if not lastCommand and token.upper() != 'M':
                raise Exception, 'Invalid path, must begin with moveto.'    
            else:                
                command = token
        else:
            #command was omited
            #use last command's implicit next command
            needParam = False
            if lastCommand:
                if lastCommand.isupper():
                    command = pathdefs[lastCommand][0]
                else:
                    command = pathdefs[lastCommand.upper()][0].lower()
            else:
                raise Exception, 'Invalid path, no initial command.'    
        numParams = pathdefs[command.upper()][1]
        while numParams > 0:
            if needParam:
                try: 
                    token, isCommand = lexer.next()
                    if isCommand:
                        raise Exception, 'Invalid number of parameters'
                except StopIteration:
                    raise Exception, 'Unexpected end of path'
            cast = pathdefs[command.upper()][2][-numParams]
            param = cast(token)
            if command.islower():
                if pathdefs[command.upper()][3][-numParams]=='x':
                    param += pen[0]
                elif pathdefs[command.upper()][3][-numParams]=='y':
                    param += pen[1]
            params.append(param)
            needParam = True
            numParams -= 1
        #segment is now absolute so
        outputCommand = command.upper()
    
        #Flesh out shortcut notation    
        if outputCommand in ('H','V'):
            if outputCommand == 'H':
                params.append(pen[1])
            if outputCommand == 'V':
                params.insert(0,pen[0])
            outputCommand = 'L'
        if outputCommand in ('S','T'):
            params.insert(0,pen[1]+(pen[1]-lastControl[1]))
            params.insert(0,pen[0]+(pen[0]-lastControl[0]))
            if outputCommand == 'S':
                outputCommand = 'C'
            if outputCommand == 'T':
                outputCommand = 'Q'

        #current values become "last" values
        if outputCommand == 'M':
            subPathStart = tuple(params[0:2])
            pen = subPathStart
        if outputCommand == 'Z':
            pen = subPathStart
        else:
            pen = tuple(params[-2:])

        if outputCommand in ('Q','C'):
            lastControl = tuple(params[-4:-2])
        else:
            lastControl = pen
        lastCommand = command

        retval.append([outputCommand,params])
    return retval

def formatPath(a):
    """Format SVG path data from an array"""
    return "".join([cmd + " ".join([str(p) for p in params]) for cmd, params in a])

def translatePath(p, x, y):
    for cmd,params in p:
        defs = pathdefs[cmd]
        for i in range(defs[1]):
            if defs[3][i] == 'x':
                params[i] += x
            elif defs[3][i] == 'y':
                params[i] += y

def scalePath(p, x, y):
    for cmd,params in p:
        defs = pathdefs[cmd]
        for i in range(defs[1]):
            if defs[3][i] == 'x':
                params[i] *= x
            elif defs[3][i] == 'y':
                params[i] *= y
            elif defs[3][i] == 'r':         # radius parameter
                params[i] *= x
            elif defs[3][i] == 's':         # sweep-flag parameter
                if x*y < 0:
                    params[i] = 1 - params[i]
            elif defs[3][i] == 'a':         # x-axis-rotation angle
                if y < 0:
                    params[i] = - params[i]

def rotatePath(p, a, cx = 0, cy = 0):
    if a == 0:
        return p
    for cmd,params in p:
        defs = pathdefs[cmd]
        for i in range(defs[1]):
            if defs[3][i] == 'x':
                x = params[i] - cx
                y = params[i + 1] - cy
                r = math.sqrt((x**2) + (y**2))
                if r != 0:
                    theta = math.atan2(y, x) + a
                    params[i] = (r * math.cos(theta)) + cx
                    params[i + 1] = (r * math.sin(theta)) + cy

#--^^^^----------------------------- end simplepath.py --------------


#--vvvv----------------------------- GetCharNames()--------------
def GetCharNames():
    standard_char_names = """0020;space;SPACE
0021;exclam;EXCLAMATION MARK
0022;quotedbl;QUOTATION MARK
0023;numbersign;NUMBER SIGN
0024;dollar;DOLLAR SIGN
0025;percent;PERCENT SIGN
0026;ampersand;AMPERSAND
0027;quotesingle;APOSTROPHE
0028;parenleft;LEFT PARENTHESIS
0029;parenright;RIGHT PARENTHESIS
002A;asterisk;ASTERISK
002B;plus;PLUS SIGN
002C;comma;COMMA
002D;hyphen;HYPHEN-MINUS
002E;period;FULL STOP
002F;slash;SOLIDUS
0030;zero;DIGIT ZERO
0031;one;DIGIT ONE
0032;two;DIGIT TWO
0033;three;DIGIT THREE
0034;four;DIGIT FOUR
0035;five;DIGIT FIVE
0036;six;DIGIT SIX
0037;seven;DIGIT SEVEN
0038;eight;DIGIT EIGHT
0039;nine;DIGIT NINE
003A;colon;COLON
003B;semicolon;SEMICOLON
003C;less;LESS-THAN SIGN
003D;equal;EQUALS SIGN
003E;greater;GREATER-THAN SIGN
003F;question;QUESTION MARK
0040;at;COMMERCIAL AT
0041;A;LATIN CAPITAL LETTER A
0042;B;LATIN CAPITAL LETTER B
0043;C;LATIN CAPITAL LETTER C
0044;D;LATIN CAPITAL LETTER D
0045;E;LATIN CAPITAL LETTER E
0046;F;LATIN CAPITAL LETTER F
0047;G;LATIN CAPITAL LETTER G
0048;H;LATIN CAPITAL LETTER H
0049;I;LATIN CAPITAL LETTER I
004A;J;LATIN CAPITAL LETTER J
004B;K;LATIN CAPITAL LETTER K
004C;L;LATIN CAPITAL LETTER L
004D;M;LATIN CAPITAL LETTER M
004E;N;LATIN CAPITAL LETTER N
004F;O;LATIN CAPITAL LETTER O
0050;P;LATIN CAPITAL LETTER P
0051;Q;LATIN CAPITAL LETTER Q
0052;R;LATIN CAPITAL LETTER R
0053;S;LATIN CAPITAL LETTER S
0054;T;LATIN CAPITAL LETTER T
0055;U;LATIN CAPITAL LETTER U
0056;V;LATIN CAPITAL LETTER V
0057;W;LATIN CAPITAL LETTER W
0058;X;LATIN CAPITAL LETTER X
0059;Y;LATIN CAPITAL LETTER Y
005A;Z;LATIN CAPITAL LETTER Z
005B;bracketleft;LEFT SQUARE BRACKET
005C;backslash;REVERSE SOLIDUS
005D;bracketright;RIGHT SQUARE BRACKET
005E;asciicircum;CIRCUMFLEX ACCENT
005F;underscore;LOW LINE
0060;grave;GRAVE ACCENT
0061;a;LATIN SMALL LETTER A
0062;b;LATIN SMALL LETTER B
0063;c;LATIN SMALL LETTER C
0064;d;LATIN SMALL LETTER D
0065;e;LATIN SMALL LETTER E
0066;f;LATIN SMALL LETTER F
0067;g;LATIN SMALL LETTER G
0068;h;LATIN SMALL LETTER H
0069;i;LATIN SMALL LETTER I
006A;j;LATIN SMALL LETTER J
006B;k;LATIN SMALL LETTER K
006C;l;LATIN SMALL LETTER L
006D;m;LATIN SMALL LETTER M
006E;n;LATIN SMALL LETTER N
006F;o;LATIN SMALL LETTER O
0070;p;LATIN SMALL LETTER P
0071;q;LATIN SMALL LETTER Q
0072;r;LATIN SMALL LETTER R
0073;s;LATIN SMALL LETTER S
0074;t;LATIN SMALL LETTER T
0075;u;LATIN SMALL LETTER U
0076;v;LATIN SMALL LETTER V
0077;w;LATIN SMALL LETTER W
0078;x;LATIN SMALL LETTER X
0079;y;LATIN SMALL LETTER Y
007A;z;LATIN SMALL LETTER Z
007B;braceleft;LEFT CURLY BRACKET
007C;bar;VERTICAL LINE
007D;braceright;RIGHT CURLY BRACKET
007E;asciitilde;TILDE
00A1;exclamdown;INVERTED EXCLAMATION MARK
00A2;cent;CENT SIGN
00A3;sterling;POUND SIGN
00A4;currency;CURRENCY SIGN
00A5;yen;YEN SIGN
00A6;brokenbar;BROKEN BAR
00A7;section;SECTION SIGN
00A8;dieresis;DIAERESIS
00A9;copyright;COPYRIGHT SIGN
00AA;ordfeminine;FEMININE ORDINAL INDICATOR
00AB;guillemotleft;LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
00AC;logicalnot;NOT SIGN
00AE;registered;REGISTERED SIGN
00AF;macron;MACRON
00B0;degree;DEGREE SIGN
00B1;plusminus;PLUS-MINUS SIGN
00B4;acute;ACUTE ACCENT
00B5;mu;MICRO SIGN
00B6;paragraph;PILCROW SIGN
00B7;periodcentered;MIDDLE DOT
00B8;cedilla;CEDILLA
00BA;ordmasculine;MASCULINE ORDINAL INDICATOR
00BB;guillemotright;RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
00BC;onequarter;VULGAR FRACTION ONE QUARTER
00BD;onehalf;VULGAR FRACTION ONE HALF
00BE;threequarters;VULGAR FRACTION THREE QUARTERS
00BF;questiondown;INVERTED QUESTION MARK
00C0;Agrave;LATIN CAPITAL LETTER A WITH GRAVE
00C1;Aacute;LATIN CAPITAL LETTER A WITH ACUTE
00C2;Acircumflex;LATIN CAPITAL LETTER A WITH CIRCUMFLEX
00C3;Atilde;LATIN CAPITAL LETTER A WITH TILDE
00C4;Adieresis;LATIN CAPITAL LETTER A WITH DIAERESIS
00C5;Aring;LATIN CAPITAL LETTER A WITH RING ABOVE
00C6;AE;LATIN CAPITAL LETTER AE
00C7;Ccedilla;LATIN CAPITAL LETTER C WITH CEDILLA
00C8;Egrave;LATIN CAPITAL LETTER E WITH GRAVE
00C9;Eacute;LATIN CAPITAL LETTER E WITH ACUTE
00CA;Ecircumflex;LATIN CAPITAL LETTER E WITH CIRCUMFLEX
00CB;Edieresis;LATIN CAPITAL LETTER E WITH DIAERESIS
00CC;Igrave;LATIN CAPITAL LETTER I WITH GRAVE
00CD;Iacute;LATIN CAPITAL LETTER I WITH ACUTE
00CE;Icircumflex;LATIN CAPITAL LETTER I WITH CIRCUMFLEX
00CF;Idieresis;LATIN CAPITAL LETTER I WITH DIAERESIS
00D0;Eth;LATIN CAPITAL LETTER ETH
00D1;Ntilde;LATIN CAPITAL LETTER N WITH TILDE
00D2;Ograve;LATIN CAPITAL LETTER O WITH GRAVE
00D3;Oacute;LATIN CAPITAL LETTER O WITH ACUTE
00D4;Ocircumflex;LATIN CAPITAL LETTER O WITH CIRCUMFLEX
00D5;Otilde;LATIN CAPITAL LETTER O WITH TILDE
00D6;Odieresis;LATIN CAPITAL LETTER O WITH DIAERESIS
00D7;multiply;MULTIPLICATION SIGN
00D8;Oslash;LATIN CAPITAL LETTER O WITH STROKE
00D9;Ugrave;LATIN CAPITAL LETTER U WITH GRAVE
00DA;Uacute;LATIN CAPITAL LETTER U WITH ACUTE
00DB;Ucircumflex;LATIN CAPITAL LETTER U WITH CIRCUMFLEX
00DC;Udieresis;LATIN CAPITAL LETTER U WITH DIAERESIS
00DD;Yacute;LATIN CAPITAL LETTER Y WITH ACUTE
00DE;Thorn;LATIN CAPITAL LETTER THORN
00DF;germandbls;LATIN SMALL LETTER SHARP S
00E0;agrave;LATIN SMALL LETTER A WITH GRAVE
00E1;aacute;LATIN SMALL LETTER A WITH ACUTE
00E2;acircumflex;LATIN SMALL LETTER A WITH CIRCUMFLEX
00E3;atilde;LATIN SMALL LETTER A WITH TILDE
00E4;adieresis;LATIN SMALL LETTER A WITH DIAERESIS
00E5;aring;LATIN SMALL LETTER A WITH RING ABOVE
00E6;ae;LATIN SMALL LETTER AE
00E7;ccedilla;LATIN SMALL LETTER C WITH CEDILLA
00E8;egrave;LATIN SMALL LETTER E WITH GRAVE
00E9;eacute;LATIN SMALL LETTER E WITH ACUTE
00EA;ecircumflex;LATIN SMALL LETTER E WITH CIRCUMFLEX
00EB;edieresis;LATIN SMALL LETTER E WITH DIAERESIS
00EC;igrave;LATIN SMALL LETTER I WITH GRAVE
00ED;iacute;LATIN SMALL LETTER I WITH ACUTE
00EE;icircumflex;LATIN SMALL LETTER I WITH CIRCUMFLEX
00EF;idieresis;LATIN SMALL LETTER I WITH DIAERESIS
00F0;eth;LATIN SMALL LETTER ETH
00F1;ntilde;LATIN SMALL LETTER N WITH TILDE
00F2;ograve;LATIN SMALL LETTER O WITH GRAVE
00F3;oacute;LATIN SMALL LETTER O WITH ACUTE
00F4;ocircumflex;LATIN SMALL LETTER O WITH CIRCUMFLEX
00F5;otilde;LATIN SMALL LETTER O WITH TILDE
00F6;odieresis;LATIN SMALL LETTER O WITH DIAERESIS
00F7;divide;DIVISION SIGN
00F8;oslash;LATIN SMALL LETTER O WITH STROKE
00F9;ugrave;LATIN SMALL LETTER U WITH GRAVE
00FA;uacute;LATIN SMALL LETTER U WITH ACUTE
00FB;ucircumflex;LATIN SMALL LETTER U WITH CIRCUMFLEX
00FC;udieresis;LATIN SMALL LETTER U WITH DIAERESIS
00FD;yacute;LATIN SMALL LETTER Y WITH ACUTE
00FE;thorn;LATIN SMALL LETTER THORN
00FF;ydieresis;LATIN SMALL LETTER Y WITH DIAERESIS
0100;Amacron;LATIN CAPITAL LETTER A WITH MACRON
0101;amacron;LATIN SMALL LETTER A WITH MACRON
0102;Abreve;LATIN CAPITAL LETTER A WITH BREVE
0103;abreve;LATIN SMALL LETTER A WITH BREVE
0104;Aogonek;LATIN CAPITAL LETTER A WITH OGONEK
0105;aogonek;LATIN SMALL LETTER A WITH OGONEK
0106;Cacute;LATIN CAPITAL LETTER C WITH ACUTE
0107;cacute;LATIN SMALL LETTER C WITH ACUTE
0108;Ccircumflex;LATIN CAPITAL LETTER C WITH CIRCUMFLEX
0109;ccircumflex;LATIN SMALL LETTER C WITH CIRCUMFLEX
010A;Cdotaccent;LATIN CAPITAL LETTER C WITH DOT ABOVE
010B;cdotaccent;LATIN SMALL LETTER C WITH DOT ABOVE
010C;Ccaron;LATIN CAPITAL LETTER C WITH CARON
010D;ccaron;LATIN SMALL LETTER C WITH CARON
010E;Dcaron;LATIN CAPITAL LETTER D WITH CARON
010F;dcaron;LATIN SMALL LETTER D WITH CARON
0110;Dcroat;LATIN CAPITAL LETTER D WITH STROKE
0111;dcroat;LATIN SMALL LETTER D WITH STROKE
0112;Emacron;LATIN CAPITAL LETTER E WITH MACRON
0113;emacron;LATIN SMALL LETTER E WITH MACRON
0114;Ebreve;LATIN CAPITAL LETTER E WITH BREVE
0115;ebreve;LATIN SMALL LETTER E WITH BREVE
0116;Edotaccent;LATIN CAPITAL LETTER E WITH DOT ABOVE
0117;edotaccent;LATIN SMALL LETTER E WITH DOT ABOVE
0118;Eogonek;LATIN CAPITAL LETTER E WITH OGONEK
0119;eogonek;LATIN SMALL LETTER E WITH OGONEK
011A;Ecaron;LATIN CAPITAL LETTER E WITH CARON
011B;ecaron;LATIN SMALL LETTER E WITH CARON
011C;Gcircumflex;LATIN CAPITAL LETTER G WITH CIRCUMFLEX
011D;gcircumflex;LATIN SMALL LETTER G WITH CIRCUMFLEX
011E;Gbreve;LATIN CAPITAL LETTER G WITH BREVE
011F;gbreve;LATIN SMALL LETTER G WITH BREVE
0120;Gdotaccent;LATIN CAPITAL LETTER G WITH DOT ABOVE
0121;gdotaccent;LATIN SMALL LETTER G WITH DOT ABOVE
0124;Hcircumflex;LATIN CAPITAL LETTER H WITH CIRCUMFLEX
0125;hcircumflex;LATIN SMALL LETTER H WITH CIRCUMFLEX
0126;Hbar;LATIN CAPITAL LETTER H WITH STROKE
0127;hbar;LATIN SMALL LETTER H WITH STROKE
0128;Itilde;LATIN CAPITAL LETTER I WITH TILDE
0129;itilde;LATIN SMALL LETTER I WITH TILDE
012A;Imacron;LATIN CAPITAL LETTER I WITH MACRON
012B;imacron;LATIN SMALL LETTER I WITH MACRON
012C;Ibreve;LATIN CAPITAL LETTER I WITH BREVE
012D;ibreve;LATIN SMALL LETTER I WITH BREVE
012E;Iogonek;LATIN CAPITAL LETTER I WITH OGONEK
012F;iogonek;LATIN SMALL LETTER I WITH OGONEK
0130;Idotaccent;LATIN CAPITAL LETTER I WITH DOT ABOVE
0131;dotlessi;LATIN SMALL LETTER DOTLESS I
0132;IJ;LATIN CAPITAL LIGATURE IJ
0133;ij;LATIN SMALL LIGATURE IJ
0134;Jcircumflex;LATIN CAPITAL LETTER J WITH CIRCUMFLEX
0135;jcircumflex;LATIN SMALL LETTER J WITH CIRCUMFLEX
0138;kgreenlandic;LATIN SMALL LETTER KRA
0139;Lacute;LATIN CAPITAL LETTER L WITH ACUTE
013A;lacute;LATIN SMALL LETTER L WITH ACUTE
013D;Lcaron;LATIN CAPITAL LETTER L WITH CARON
013E;lcaron;LATIN SMALL LETTER L WITH CARON
013F;Ldot;LATIN CAPITAL LETTER L WITH MIDDLE DOT
0140;ldot;LATIN SMALL LETTER L WITH MIDDLE DOT
0141;Lslash;LATIN CAPITAL LETTER L WITH STROKE
0142;lslash;LATIN SMALL LETTER L WITH STROKE
0143;Nacute;LATIN CAPITAL LETTER N WITH ACUTE
0144;nacute;LATIN SMALL LETTER N WITH ACUTE
0147;Ncaron;LATIN CAPITAL LETTER N WITH CARON
0148;ncaron;LATIN SMALL LETTER N WITH CARON
0149;napostrophe;LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
014A;Eng;LATIN CAPITAL LETTER ENG
014B;eng;LATIN SMALL LETTER ENG
014C;Omacron;LATIN CAPITAL LETTER O WITH MACRON
014D;omacron;LATIN SMALL LETTER O WITH MACRON
014E;Obreve;LATIN CAPITAL LETTER O WITH BREVE
014F;obreve;LATIN SMALL LETTER O WITH BREVE
0150;Ohungarumlaut;LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
0151;ohungarumlaut;LATIN SMALL LETTER O WITH DOUBLE ACUTE
0152;OE;LATIN CAPITAL LIGATURE OE
0153;oe;LATIN SMALL LIGATURE OE
0154;Racute;LATIN CAPITAL LETTER R WITH ACUTE
0155;racute;LATIN SMALL LETTER R WITH ACUTE
0158;Rcaron;LATIN CAPITAL LETTER R WITH CARON
0159;rcaron;LATIN SMALL LETTER R WITH CARON
015A;Sacute;LATIN CAPITAL LETTER S WITH ACUTE
015B;sacute;LATIN SMALL LETTER S WITH ACUTE
015C;Scircumflex;LATIN CAPITAL LETTER S WITH CIRCUMFLEX
015D;scircumflex;LATIN SMALL LETTER S WITH CIRCUMFLEX
015E;Scedilla;LATIN CAPITAL LETTER S WITH CEDILLA
015F;scedilla;LATIN SMALL LETTER S WITH CEDILLA
0160;Scaron;LATIN CAPITAL LETTER S WITH CARON
0161;scaron;LATIN SMALL LETTER S WITH CARON
0164;Tcaron;LATIN CAPITAL LETTER T WITH CARON
0165;tcaron;LATIN SMALL LETTER T WITH CARON
0166;Tbar;LATIN CAPITAL LETTER T WITH STROKE
0167;tbar;LATIN SMALL LETTER T WITH STROKE
0168;Utilde;LATIN CAPITAL LETTER U WITH TILDE
0169;utilde;LATIN SMALL LETTER U WITH TILDE
016A;Umacron;LATIN CAPITAL LETTER U WITH MACRON
016B;umacron;LATIN SMALL LETTER U WITH MACRON
016C;Ubreve;LATIN CAPITAL LETTER U WITH BREVE
016D;ubreve;LATIN SMALL LETTER U WITH BREVE
016E;Uring;LATIN CAPITAL LETTER U WITH RING ABOVE
016F;uring;LATIN SMALL LETTER U WITH RING ABOVE
0170;Uhungarumlaut;LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
0171;uhungarumlaut;LATIN SMALL LETTER U WITH DOUBLE ACUTE
0172;Uogonek;LATIN CAPITAL LETTER U WITH OGONEK
0173;uogonek;LATIN SMALL LETTER U WITH OGONEK
0174;Wcircumflex;LATIN CAPITAL LETTER W WITH CIRCUMFLEX
0175;wcircumflex;LATIN SMALL LETTER W WITH CIRCUMFLEX
0176;Ycircumflex;LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
0177;ycircumflex;LATIN SMALL LETTER Y WITH CIRCUMFLEX
0178;Ydieresis;LATIN CAPITAL LETTER Y WITH DIAERESIS
0179;Zacute;LATIN CAPITAL LETTER Z WITH ACUTE
017A;zacute;LATIN SMALL LETTER Z WITH ACUTE
017B;Zdotaccent;LATIN CAPITAL LETTER Z WITH DOT ABOVE
017C;zdotaccent;LATIN SMALL LETTER Z WITH DOT ABOVE
017D;Zcaron;LATIN CAPITAL LETTER Z WITH CARON
017E;zcaron;LATIN SMALL LETTER Z WITH CARON
017F;longs;LATIN SMALL LETTER LONG S
0192;florin;LATIN SMALL LETTER F WITH HOOK
01A0;Ohorn;LATIN CAPITAL LETTER O WITH HORN
01A1;ohorn;LATIN SMALL LETTER O WITH HORN
01AF;Uhorn;LATIN CAPITAL LETTER U WITH HORN
01B0;uhorn;LATIN SMALL LETTER U WITH HORN
01E6;Gcaron;LATIN CAPITAL LETTER G WITH CARON
01E7;gcaron;LATIN SMALL LETTER G WITH CARON
01FA;Aringacute;LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE
01FB;aringacute;LATIN SMALL LETTER A WITH RING ABOVE AND ACUTE
01FC;AEacute;LATIN CAPITAL LETTER AE WITH ACUTE
01FD;aeacute;LATIN SMALL LETTER AE WITH ACUTE
01FE;Oslashacute;LATIN CAPITAL LETTER O WITH STROKE AND ACUTE
01FF;oslashacute;LATIN SMALL LETTER O WITH STROKE AND ACUTE
02C6;circumflex;MODIFIER LETTER CIRCUMFLEX ACCENT
02C7;caron;CARON
02D8;breve;BREVE
02D9;dotaccent;DOT ABOVE
02DA;ring;RING ABOVE
02DB;ogonek;OGONEK
02DC;tilde;SMALL TILDE
02DD;hungarumlaut;DOUBLE ACUTE ACCENT
0300;gravecomb;COMBINING GRAVE ACCENT
0301;acutecomb;COMBINING ACUTE ACCENT
0303;tildecomb;COMBINING TILDE
0309;hookabovecomb;COMBINING HOOK ABOVE
0323;dotbelowcomb;COMBINING DOT BELOW
0384;tonos;GREEK TONOS
0385;dieresistonos;GREEK DIALYTIKA TONOS
0386;Alphatonos;GREEK CAPITAL LETTER ALPHA WITH TONOS
0387;anoteleia;GREEK ANO TELEIA
0388;Epsilontonos;GREEK CAPITAL LETTER EPSILON WITH TONOS
0389;Etatonos;GREEK CAPITAL LETTER ETA WITH TONOS
038A;Iotatonos;GREEK CAPITAL LETTER IOTA WITH TONOS
038C;Omicrontonos;GREEK CAPITAL LETTER OMICRON WITH TONOS
038E;Upsilontonos;GREEK CAPITAL LETTER UPSILON WITH TONOS
038F;Omegatonos;GREEK CAPITAL LETTER OMEGA WITH TONOS
0390;iotadieresistonos;GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
0391;Alpha;GREEK CAPITAL LETTER ALPHA
0392;Beta;GREEK CAPITAL LETTER BETA
0393;Gamma;GREEK CAPITAL LETTER GAMMA
0395;Epsilon;GREEK CAPITAL LETTER EPSILON
0396;Zeta;GREEK CAPITAL LETTER ZETA
0397;Eta;GREEK CAPITAL LETTER ETA
0398;Theta;GREEK CAPITAL LETTER THETA
0399;Iota;GREEK CAPITAL LETTER IOTA
039A;Kappa;GREEK CAPITAL LETTER KAPPA
039B;Lambda;GREEK CAPITAL LETTER LAMDA
039C;Mu;GREEK CAPITAL LETTER MU
039D;Nu;GREEK CAPITAL LETTER NU
039E;Xi;GREEK CAPITAL LETTER XI
039F;Omicron;GREEK CAPITAL LETTER OMICRON
03A0;Pi;GREEK CAPITAL LETTER PI
03A1;Rho;GREEK CAPITAL LETTER RHO
03A3;Sigma;GREEK CAPITAL LETTER SIGMA
03A4;Tau;GREEK CAPITAL LETTER TAU
03A5;Upsilon;GREEK CAPITAL LETTER UPSILON
03A6;Phi;GREEK CAPITAL LETTER PHI
03A7;Chi;GREEK CAPITAL LETTER CHI
03A8;Psi;GREEK CAPITAL LETTER PSI
03AA;Iotadieresis;GREEK CAPITAL LETTER IOTA WITH DIALYTIKA
03AB;Upsilondieresis;GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA
03AC;alphatonos;GREEK SMALL LETTER ALPHA WITH TONOS
03AD;epsilontonos;GREEK SMALL LETTER EPSILON WITH TONOS
03AE;etatonos;GREEK SMALL LETTER ETA WITH TONOS
03AF;iotatonos;GREEK SMALL LETTER IOTA WITH TONOS
03B0;upsilondieresistonos;GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
03B1;alpha;GREEK SMALL LETTER ALPHA
03B2;beta;GREEK SMALL LETTER BETA
03B3;gamma;GREEK SMALL LETTER GAMMA
03B4;delta;GREEK SMALL LETTER DELTA
03B5;epsilon;GREEK SMALL LETTER EPSILON
03B6;zeta;GREEK SMALL LETTER ZETA
03B7;eta;GREEK SMALL LETTER ETA
03B8;theta;GREEK SMALL LETTER THETA
03B9;iota;GREEK SMALL LETTER IOTA
03BA;kappa;GREEK SMALL LETTER KAPPA
03BB;lambda;GREEK SMALL LETTER LAMDA
03BD;nu;GREEK SMALL LETTER NU
03BE;xi;GREEK SMALL LETTER XI
03BF;omicron;GREEK SMALL LETTER OMICRON
03C0;pi;GREEK SMALL LETTER PI
03C1;rho;GREEK SMALL LETTER RHO
03C2;sigma1;GREEK SMALL LETTER FINAL SIGMA
03C3;sigma;GREEK SMALL LETTER SIGMA
03C4;tau;GREEK SMALL LETTER TAU
03C5;upsilon;GREEK SMALL LETTER UPSILON
03C6;phi;GREEK SMALL LETTER PHI
03C7;chi;GREEK SMALL LETTER CHI
03C8;psi;GREEK SMALL LETTER PSI
03C9;omega;GREEK SMALL LETTER OMEGA
03CA;iotadieresis;GREEK SMALL LETTER IOTA WITH DIALYTIKA
03CB;upsilondieresis;GREEK SMALL LETTER UPSILON WITH DIALYTIKA
03CC;omicrontonos;GREEK SMALL LETTER OMICRON WITH TONOS
03CD;upsilontonos;GREEK SMALL LETTER UPSILON WITH TONOS
03CE;omegatonos;GREEK SMALL LETTER OMEGA WITH TONOS
03D1;theta1;GREEK THETA SYMBOL
03D2;Upsilon1;GREEK UPSILON WITH HOOK SYMBOL
03D5;phi1;GREEK PHI SYMBOL
03D6;omega1;GREEK PI SYMBOL
1E80;Wgrave;LATIN CAPITAL LETTER W WITH GRAVE
1E81;wgrave;LATIN SMALL LETTER W WITH GRAVE
1E82;Wacute;LATIN CAPITAL LETTER W WITH ACUTE
1E83;wacute;LATIN SMALL LETTER W WITH ACUTE
1E84;Wdieresis;LATIN CAPITAL LETTER W WITH DIAERESIS
1E85;wdieresis;LATIN SMALL LETTER W WITH DIAERESIS
1EF2;Ygrave;LATIN CAPITAL LETTER Y WITH GRAVE
1EF3;ygrave;LATIN SMALL LETTER Y WITH GRAVE
2012;figuredash;FIGURE DASH
2013;endash;EN DASH
2014;emdash;EM DASH
2017;underscoredbl;DOUBLE LOW LINE
2018;quoteleft;LEFT SINGLE QUOTATION MARK
2019;quoteright;RIGHT SINGLE QUOTATION MARK
201A;quotesinglbase;SINGLE LOW-9 QUOTATION MARK
201B;quotereversed;SINGLE HIGH-REVERSED-9 QUOTATION MARK
201C;quotedblleft;LEFT DOUBLE QUOTATION MARK
201D;quotedblright;RIGHT DOUBLE QUOTATION MARK
201E;quotedblbase;DOUBLE LOW-9 QUOTATION MARK
2020;dagger;DAGGER
2021;daggerdbl;DOUBLE DAGGER
2022;bullet;BULLET
2024;onedotenleader;ONE DOT LEADER
2025;twodotenleader;TWO DOT LEADER
2026;ellipsis;HORIZONTAL ELLIPSIS
2030;perthousand;PER MILLE SIGN
2032;minute;PRIME
2033;second;DOUBLE PRIME
2039;guilsinglleft;SINGLE LEFT-POINTING ANGLE QUOTATION MARK
203A;guilsinglright;SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
203C;exclamdbl;DOUBLE EXCLAMATION MARK
2044;fraction;FRACTION SLASH
20A1;colonmonetary;COLON SIGN
20A3;franc;FRENCH FRANC SIGN
20A4;lira;LIRA SIGN
20A7;peseta;PESETA SIGN
20AB;dong;DONG SIGN
20AC;Euro;EURO SIGN
2111;Ifraktur;BLACK-LETTER CAPITAL I
2118;weierstrass;SCRIPT CAPITAL P
211C;Rfraktur;BLACK-LETTER CAPITAL R
211E;prescription;PRESCRIPTION TAKE
2122;trademark;TRADE MARK SIGN
2126;Omega;OHM SIGN
212E;estimated;ESTIMATED SYMBOL
2135;aleph;ALEF SYMBOL
2153;onethird;VULGAR FRACTION ONE THIRD
2154;twothirds;VULGAR FRACTION TWO THIRDS
215B;oneeighth;VULGAR FRACTION ONE EIGHTH
215C;threeeighths;VULGAR FRACTION THREE EIGHTHS
215D;fiveeighths;VULGAR FRACTION FIVE EIGHTHS
215E;seveneighths;VULGAR FRACTION SEVEN EIGHTHS
2190;arrowleft;LEFTWARDS ARROW
2191;arrowup;UPWARDS ARROW
2192;arrowright;RIGHTWARDS ARROW
2193;arrowdown;DOWNWARDS ARROW
2194;arrowboth;LEFT RIGHT ARROW
2195;arrowupdn;UP DOWN ARROW
21A8;arrowupdnbse;UP DOWN ARROW WITH BASE
21B5;carriagereturn;DOWNWARDS ARROW WITH CORNER LEFTWARDS
21D0;arrowdblleft;LEFTWARDS DOUBLE ARROW
21D1;arrowdblup;UPWARDS DOUBLE ARROW
21D2;arrowdblright;RIGHTWARDS DOUBLE ARROW
21D3;arrowdbldown;DOWNWARDS DOUBLE ARROW
21D4;arrowdblboth;LEFT RIGHT DOUBLE ARROW
2200;universal;FOR ALL
2202;partialdiff;PARTIAL DIFFERENTIAL
2203;existential;THERE EXISTS
2205;emptyset;EMPTY SET
2206;Delta;INCREMENT
2207;gradient;NABLA
2208;element;ELEMENT OF
2209;notelement;NOT AN ELEMENT OF
220B;suchthat;CONTAINS AS MEMBER
220F;product;N-ARY PRODUCT
2211;summation;N-ARY SUMMATION
2212;minus;MINUS SIGN
2217;asteriskmath;ASTERISK OPERATOR
221A;radical;SQUARE ROOT
221D;proportional;PROPORTIONAL TO
221E;infinity;INFINITY
221F;orthogonal;RIGHT ANGLE
2220;angle;ANGLE
2227;logicaland;LOGICAL AND
2228;logicalor;LOGICAL OR
2229;intersection;INTERSECTION
222A;union;UNION
222B;integral;INTEGRAL
2234;therefore;THEREFORE
223C;similar;TILDE OPERATOR
2245;congruent;APPROXIMATELY EQUAL TO
2248;approxequal;ALMOST EQUAL TO
2260;notequal;NOT EQUAL TO
2261;equivalence;IDENTICAL TO
2264;lessequal;LESS-THAN OR EQUAL TO
2265;greaterequal;GREATER-THAN OR EQUAL TO
2282;propersubset;SUBSET OF
2283;propersuperset;SUPERSET OF
2284;notsubset;NOT A SUBSET OF
2286;reflexsubset;SUBSET OF OR EQUAL TO
2287;reflexsuperset;SUPERSET OF OR EQUAL TO
2295;circleplus;CIRCLED PLUS
2297;circlemultiply;CIRCLED TIMES
22A5;perpendicular;UP TACK
22C5;dotmath;DOT OPERATOR
2302;house;HOUSE
2310;revlogicalnot;REVERSED NOT SIGN
2320;integraltp;TOP HALF INTEGRAL
2321;integralbt;BOTTOM HALF INTEGRAL
2329;angleleft;LEFT-POINTING ANGLE BRACKET
232A;angleright;RIGHT-POINTING ANGLE BRACKET
2500;SF100000;BOX DRAWINGS LIGHT HORIZONTAL
2502;SF110000;BOX DRAWINGS LIGHT VERTICAL
250C;SF010000;BOX DRAWINGS LIGHT DOWN AND RIGHT
2510;SF030000;BOX DRAWINGS LIGHT DOWN AND LEFT
2514;SF020000;BOX DRAWINGS LIGHT UP AND RIGHT
2518;SF040000;BOX DRAWINGS LIGHT UP AND LEFT
251C;SF080000;BOX DRAWINGS LIGHT VERTICAL AND RIGHT
2524;SF090000;BOX DRAWINGS LIGHT VERTICAL AND LEFT
252C;SF060000;BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
2534;SF070000;BOX DRAWINGS LIGHT UP AND HORIZONTAL
253C;SF050000;BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
2550;SF430000;BOX DRAWINGS DOUBLE HORIZONTAL
2551;SF240000;BOX DRAWINGS DOUBLE VERTICAL
2552;SF510000;BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
2553;SF520000;BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
2554;SF390000;BOX DRAWINGS DOUBLE DOWN AND RIGHT
2555;SF220000;BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
2556;SF210000;BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
2557;SF250000;BOX DRAWINGS DOUBLE DOWN AND LEFT
2558;SF500000;BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
2559;SF490000;BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
255A;SF380000;BOX DRAWINGS DOUBLE UP AND RIGHT
255B;SF280000;BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
255C;SF270000;BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
255D;SF260000;BOX DRAWINGS DOUBLE UP AND LEFT
255E;SF360000;BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
255F;SF370000;BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
2560;SF420000;BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
2561;SF190000;BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
2562;SF200000;BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
2563;SF230000;BOX DRAWINGS DOUBLE VERTICAL AND LEFT
2564;SF470000;BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
2565;SF480000;BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
2566;SF410000;BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
2567;SF450000;BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
2568;SF460000;BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
2569;SF400000;BOX DRAWINGS DOUBLE UP AND HORIZONTAL
256A;SF540000;BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
256B;SF530000;BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
256C;SF440000;BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
2580;upblock;UPPER HALF BLOCK
2584;dnblock;LOWER HALF BLOCK
2588;block;FULL BLOCK
258C;lfblock;LEFT HALF BLOCK
2590;rtblock;RIGHT HALF BLOCK
2591;ltshade;LIGHT SHADE
2592;shade;MEDIUM SHADE
2593;dkshade;DARK SHADE
25A0;filledbox;BLACK SQUARE
25A1;H22073;WHITE SQUARE
25AA;H18543;BLACK SMALL SQUARE
25AB;H18551;WHITE SMALL SQUARE
25AC;filledrect;BLACK RECTANGLE
25B2;triagup;BLACK UP-POINTING TRIANGLE
25BA;triagrt;BLACK RIGHT-POINTING POINTER
25BC;triagdn;BLACK DOWN-POINTING TRIANGLE
25C4;triaglf;BLACK LEFT-POINTING POINTER
25CA;lozenge;LOZENGE
25CB;circle;WHITE CIRCLE
25CF;H18533;BLACK CIRCLE
25D8;invbullet;INVERSE BULLET
25D9;invcircle;INVERSE WHITE CIRCLE
25E6;openbullet;WHITE BULLET
263A;smileface;WHITE SMILING FACE
263B;invsmileface;BLACK SMILING FACE
263C;sun;WHITE SUN WITH RAYS
2640;female;FEMALE SIGN
2642;male;MALE SIGN
2660;spade;BLACK SPADE SUIT
2663;club;BLACK CLUB SUIT
2665;heart;BLACK HEART SUIT
2666;diamond;BLACK DIAMOND SUIT
266A;musicalnote;EIGHTH NOTE
266B;musicalnotedbl;BEAMED EIGHTH NOTES"""

    codes = {}
    for line in standard_char_names.splitlines():
        s = line.split(';')
        codes[int(s[0],16)] = [s[1], s[2]]

    return codes

codes = GetCharNames()
#print (str(sorted(codes)))

#--^^^^----------------------------- end GetCharNames()--------------


instructions = """    <instructions><assembly> </assembly></instructions>\n """
# instructions = """
#       <instructions><assembly>
#           PUSH[ ]	/* 1 value pushed */
#           0
#           CALL[ ]	/* CallFunction */
#           SVTCA[0]	/* SetFPVectorToAxis */
#           PUSH[ ]	/* 1 value pushed */
#           0
#           RCVT[ ]	/* ReadCVT */
#           IF[ ]	/* If */
#           PUSH[ ]	/* 1 value pushed */
#           16
#           MDAP[1]	/* MoveDirectAbsPt */
#           ELSE[ ]	/* Else */
#           PUSH[ ]	/* 2 values pushed */
#           16 5
#           MIAP[0]	/* MoveIndirectAbsPt */
#           EIF[ ]	/* EndIf */
#           PUSH[ ]	/* 3 values pushed */
#           13 12 3
#           CALL[ ]	/* CallFunction */
#           PUSH[ ]	/* 3 values pushed */
#           1 0 3
#           CALL[ ]	/* CallFunction */
#           PUSH[ ]	/* 3 values pushed */
#           9 8 3
#           CALL[ ]	/* CallFunction */
#           PUSH[ ]	/* 3 values pushed */
#           5 4 3
#           CALL[ ]	/* CallFunction */
#           PUSH[ ]	/* 1 value pushed */
#           16
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           17
#           MDRP[11100]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           0
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           25
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           4
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           29
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           8
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           33
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           12
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           37
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           1
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           40
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           5
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           44
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           9
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           48
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           13
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           52
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           17
#           SRP0[ ]	/* SetRefPoint0 */
#           PUSH[ ]	/* 1 value pushed */
#           56
#           MDRP[10000]	/* MoveDirectRelPt */
#           PUSH[ ]	/* 1 value pushed */
#           59
#           MDRP[10000]	/* MoveDirectRelPt */
#           IUP[0]	/* InterpolateUntPts */
#           IUP[1]	/* InterpolateUntPts */
#         </assembly></instructions>
# """





#define mapping of glyph id to names for later use in hmtx
glyphmap = {}         
glyphmap[".notdef"         ] = 0
glyphmap[".null"           ] = 1
glyphmap["nonmarkingreturn"] = 2

glyph_start_index = len(glyphmap)

#glyphmap["space"           ] = 3



glyfs = {}



transform = [1,0,0,1,0,0]
lasttag = ""


docwidth=0
docheight=0
columnpixwidth  = 0
columnpixheight = 0

colors=[]
paths_d=[]
alts = {}

 #first parse in the original svg
svg = open(filename)

curcolor = None
intspan = False
x=0
y=0

for line in svg:
    #print (line[:40].rstrip()+"...")

    #find opening tag
    match = re.search("^\s*<(\w+)", line)
    if (match) :
        #print (match.group(0) + ",   "+match.group(1))
        lasttag = match.group(1)
        intspan = False

    match = re.search("^\s*([a-zA-Z]+)", line)
    if match:
        attribute = match.group(1)
        #print ("att: "+attribute)
    else:
        attribute = ""

    if lasttag == "svg" and line.find("width")>=0:
        match = re.search("\"(\d*)\"", line)
        if (match):
            docwidth = float(match.group(1))
            columnpixwidth  = float(docwidth)  / gridwidth
            #print ("doc width: "+str(docwidth)+ " from line "+line)

    elif lasttag == "svg" and line.find("height")>=0:
        match = re.search("\"(\d*)\"", line)
        if (match):
            docheight = float(match.group(1))
            columnpixheight = float(docheight) / gridheight
            #print ("doc height: "+str(docheight)+ " from line "+line)

    elif line.find("transform")>=0:
        #*** append to transform
        match = re.search("transform=\"translate\(([^,]*),([^)]*)\)\"", line)
        if match:
            #print ("transform translate: "+match.group(1)+", "+match.group(2))
            transform[4] = transform[4] + float(match.group(1))
            transform[5] = transform[5] + float(match.group(2))
            #print (str(transform))

    if lasttag == "path" and attribute == "style": 
        attr_string = line[line.find("\"")+1:line.rfind("\"")]
        #print ("attr string: "+attr_string)

        attr_dict = {pair.split(":")[0]:pair.split(":")[1] for pair in attr_string.split(";")}
        #print ("attrs: "+str(attr_dict))
        if "fill" in attr_dict:
            curcolor = attr_dict["fill"]
            colors.append(curcolor)

    elif lasttag == "path" and attribute == "d": 
        d = line[line.find("\"")+1:line.rfind("\"")]

        paths_d.append(d)
#        if dolayered == False:
#            paths_d.append(d)
#        else:
#            if len(paths_d)==layer:
#                paths_d.append(d)
#            else:
#                paths_d.append("")

    elif lasttag == "text":
        #look for alternates, labeled with text objects in the svg

        if line.find("<tspan")>=0:
            intspan = True

        elif attribute == 'x' and intspan == False:
            match = re.search("\"([0123456789.-]*)\"", line)
            x = int(float(match.group(1))/columnpixwidth)

        elif attribute == 'y' and intspan == False:
            match = re.search("\"([0123456789.-]*)\"", line)
            y = int(float(match.group(1))/columnpixheight)
        else: 
            match = re.search(">([^<]+)</tspan", line)
            if match:
                tstr = match.group(1)
                print ("found text: "+tstr+' at '+str(x)+', '+str(y))
                #alts[str(x)+','+str(y)] = tstr


svg.close()

            
#svg.seek(0)
#for line in svg:
#    print ("redo!!" + line[:40].rstrip()+"...")


print ("Offset: "+str(transform))
print ("Colors: "+str(colors))
print ("Number of d atts: "+str(len(paths_d)))


paths=[]
for path_d in paths_d:
    pathlist = parsePath(path_d)
    translatePath(pathlist, transform[4],transform[5])
    paths.append(pathlist)

columnpixwidth  = float(docwidth)  / gridwidth
columnpixheight = float(docheight) / gridheight

hmtx = {}

#process the svg and output to individual files
for row in range (0, gridheight):
    for column in range(0, gridwidth):
        charindex  = startindex + row*gridwidth + column
        #if charindex < 65 or charindex > 90: continue

        glyphindex = glyph_start_index + charindex - startindex

        if charindex in codes:
            glyphname  = codes[charindex][0]
        else: glyphname = "AAAAARRRRR!!! ERRORORRORRR"

        outfile = letterdir+"/char-" + glyphname + ".svg"  #file name by glyph name
        #outfile = letterdir+"/char-" + hex(charindex) + ".svg"      #file name by unicode
        #print ("Outputting "+outfile)

        xoff = 5
        yoff = -5
        minx = columnpixwidth  * column - xoff
        maxx = columnpixwidth  * (column+1) - xoff
        #miny = columnpixheight * (gridheight - row -1)
        #maxy = columnpixheight * (gridheight - row)
        miny = columnpixheight * (row) - yoff
        maxy = columnpixheight * (row+1) - yoff

        bounds = [-10000,0,0,0]

        #print ("\nnew grid box "+hex(charindex)+": x: "+str(minx)+"-"+str(maxx)+",  y:"+str(miny)+"-"+str(maxy))

        out = open(outfile, "w")
        out.write ('<?xml version="1.0" encoding="UTF-8"?>\n'
                   '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" version="1.2"\n'+
                   '   id="glyph'+str(len(glyphmap))+'"\n'
                   '   width="'+str(columnpixwidth)+'"\n'
                   '   height="'+str(columnpixheight)+'"\n'
                   '   viewBox="0 250 '+str(columnpixwidth)+' '+str(columnpixheight)+'"\n'
                   #'   viewBox="50 250 '+str(columnpixwidth)+' '+str(columnpixheight)+'"\n'
                   #'   viewBox="0 -250 '+str(columnpixwidth)+' '+str(columnpixheight)+'"\n'
                   '   >\n'
                   '<g>\n')

        whichpath=0
        for pathlist in paths:
            bboxminx = -10000
            bboxminy = -10000
            bboxmaxx = -10000
            bboxmaxy = -10000

             #figure out which paths have points within current grid cell
            path=[]
            
            index=0
            pathstart=-1
            usecurrent = False

            for cmd,params in pathlist:
                defs = pathdefs[cmd]
                #for i in range(defs[1]):
                
                if cmd=='M':
                    if pathstart!=-1:
                        if usecurrent==True:
                             #end of previous path, append to path
                            for pp in pathlist[pathstart:index]:
                                path.append(pp)
                                #print ("hit! "+str(pp))
                            usecurrent = False
                    pathstart = index

                     #check if new point is in box
                    x,y = params[0],params[1]
                    #print ("M compare "+str(x)+", "+str(y))

                    if x>=minx and x<=maxx and y>=miny and y<=maxy:
                        if bboxminx==-10000:
                            bboxminx = x
                            bboxmaxx = x
                            bboxminy = y
                            bboxmaxy = y
                        if   x<bboxminx: bboxminx=x
                        elif x>bboxmaxx: bboxmaxx=x
                        if   y<bboxminy: bboxminy=y
                        elif y>bboxmaxy: bboxmaxy=y
                        usecurrent=True

                elif cmd=='Z':
                    if usecurrent == True:
                         #end of previous path, append to path
                        for pp in pathlist[pathstart:index+1]:
                            path.append(pp)
                            #print ("hit! "+str(pp))
                        usecurrent = False

                elif cmd=='L':
                    x,y = params[0],params[1]
                    #print ("L compare "+str(x)+", "+str(y))
                    if x>=minx and x<=maxx and y>=miny and y<=maxy:
                        if   x<bboxminx: bboxminx=x
                        elif x>bboxmaxx: bboxmaxx=x
                        if   y<bboxminy: bboxminy=y
                        elif y>bboxmaxy: bboxmaxy=y
                        usecurrent=True

                elif cmd=='C':
                    c1x,c1y,c2x,c2y, x,y = params[0],params[1],params[2],params[3],params[4],params[5]

                    #print ("C compare  c1:"+str(c1x)+", "+str(c1y)+"  c2: "+str(c2x)+", "+str(c2y)+"  "+str(x)+", "+str(y))
                    if x>=minx and x<=maxx and y>=miny and y<=maxy:
                        if   x<bboxminx: bboxminx=x
                        elif x>bboxmaxx: bboxmaxx=x
                        if   y<bboxminy: bboxminy=y
                        elif y>bboxmaxy: bboxmaxy=y
                        usecurrent=True
                    if c1x>=minx and c1x<=maxx and c1y>=miny and c1y<=maxy:
                        if   c1x<bboxminx: bboxminx=c1x
                        elif c1x>bboxmaxx: bboxmaxx=c1x
                        if   c1y<bboxminy: bboxminy=c1y
                        elif c1y>bboxmaxy: bboxmaxy=c1y
                        usecurrent=True
                    if c2x>=minx and c2x<=maxx and c2y>=miny and c2y<=maxy:
                        if   c2x<bboxminx: bboxminx=c2x
                        elif c2x>bboxmaxx: bboxmaxx=c2x
                        if   c2y<bboxminy: bboxminy=c2y
                        elif c2y>bboxmaxy: bboxmaxy=c2y
                        usecurrent=True


                index = index + 1

            if len(path)>0:
                #print ("outputting path..")
                path = copy.deepcopy(path)
                translatePath(path, -minx,-miny)
                #translatePath(path, 0,-250)

                bboxminx -= minx
                bboxmaxx -= minx
                bboxminy -= miny
                bboxmaxy -= miny
                    
                if bounds[0]==-10000:
                    bounds[0] = bboxminx
                    bounds[1] = bboxmaxx
                    bounds[2] = bboxminy
                    bounds[3] = bboxmaxy

                if bboxminx<bounds[0]: bounds[0]=bboxminx
                if bboxmaxx>bounds[1]: bounds[1]=bboxmaxx
                if bboxminy<bounds[2]: bounds[2]=bboxminy
                if bboxmaxy<bounds[3]: bounds[3]=bboxmaxy

                out.write ('<path\n'+
                           '  style="fill:'+colors[whichpath]+';"\n'+
                           '  d="'+formatPath(path)+'"\n'+
                           '/>\n')

                if (dolayered==True and whichpath==layer) or (dolayered==False and whichpath==0):
                    glyfs[glyphname] = path
 
            whichpath = whichpath +1


        if bounds[0]!=-10000:
            print ("pushing glyph "+glyphname+", x:"+str(bounds[0])+','+str(bounds[1])+"  y:"+str(bounds[2])+','+str(bounds[3]))
            hmtx[charindex] = bounds


        if glyphname!="AAAAARRRRR!!! ERRORORRORRR":
            glyphmap[glyphname] = len(glyphmap)
#        else:
#            glyphmap[glyphname] = len(glyphmap)


        out.write ('</g>\n</svg>\n')
        out.close()


#------------------------- output various tables

#
#the template file should be a basic ttx, with the following tables replaced with a single comment line:
#	glyf, cmap, hmtx, SVG, GlyphOrder, CPAL
#For instance, instead of the SVG table, there should be a single line like this:
#  <!-- SVG -->
#
template = open("VillaPazza-TEMPLATE.ttx", "r")

#set output file
if dolayered:
    ttx = open("VillaPazza"+str(layer)+".ttx", "w")
else:
    ttx = open("VillaPazza.ttx", "w")


for line in template:

  if dolayered==False and line.find("<!-- SVG -->")>=0:
    ###
    ###   SVG
    ###     ---specified by glyphid, not name!!
    ###
    ttx.write('<SVG>\n')
    #for each defined glpyh.....
    for gvalue in sorted(glyphmap.values()):
        glyphname = glyphmap.keys()[list(glyphmap.values()).index(gvalue)]

        outfile = letterdir+"/char-" + glyphname + ".svg"  #file name by glyph name
        if os.path.isfile(outfile):
            ttx.write('  <svgDoc startGlyphID="'+str(gvalue)+'" endGlyphID="'+str(gvalue)+'">\n')
            ttx.write('    <![CDATA[\n')

            with open(outfile, 'r') as content_file:
                content = content_file.read()
            ttx.write(content+"\n")

            ttx.write(']]>\n')
            ttx.write('  </svgDoc>\n')
    ttx.write('  <colorPalettes></colorPalettes>\n')
    ttx.write('</SVG>\n')


  elif line.find("<!-- GlyphOrder -->")>=0:
    ###
    ###  GlyphOrder
    ###
    ###    <GlyphID id="0" name=".notdef"/>
    ###    <GlyphID id="1" name=".null"/>
    ###    <GlyphID id="2" name="nonmarkingreturn"/>
    ###    ...
    ttx.write ('  <GlyphOrder>\n')
    #ttx.write ('  <GlyphID id="0" name=".notdef"/>')
    #ttx.write ('  <GlyphID id="1" name=".null"/>')
    #ttx.write ('  <GlyphID id="2" name="nonmarkingreturn"/>')
    #ttx.write ('  <GlyphID id="3" name="space"/>')
    #ttx.write ("".join(['  <GlyphID id="'+str(glyphmap[gname])+'" name="'+gname+'"/>\n' for gname in sorted(glyphmap.keys())]))
    ttx.write ("".join(['    <GlyphID id="'+str(gvalue)+'" name="'+glyphmap.keys()[list(glyphmap.values()).index(gvalue)]+'"/>\n' for gvalue in sorted(glyphmap.values())]))
    ttx.write ('  </GlyphOrder>\n')



  elif line.find("<!-- cmap -->")>=0:
    ###
    ###  cmap
    ###
    ###     2 byte codes, platform 0 is unicode, 1 is mac, 3 is windows
    ###    <cmap_format_4 platformID="0" platEncID="3" language="0">
    ###          <map code="0x0" name=".null"/><!-- ???? -->
    ttx.write ('<cmap>\n')
    ttx.write ('  <tableVersion version="0"/>\n')
    ttx.write ('  <cmap_format_4 platformID="0" platEncID="3" language="0">\n')
    ttx.write ('    <map code="0x0" name=".null"/><!-- ???? -->\n')
    ttx.write ('    <map code="0xd" name="nonmarkingreturn"/><!-- ???? -->\n')
    for charcode in hmtx:
        if charcode not in codes: continue
        #glyphname = codes[charcode]
        ttx.write ('    <map code="'+hex(charcode)+'" name="'+codes[charcode][0]+'"/><!-- '+codes[charcode][1]+' -->\n')

    ttx.write ('  </cmap_format_4>\n')

    ttx.write ('  <cmap_format_4 platformID="3" platEncID="1" language="0">\n')
    ttx.write ('    <map code="0x0" name=".null"/><!-- ???? -->\n')
    ttx.write ('    <map code="0xd" name="nonmarkingreturn"/><!-- ???? -->\n')
    for charcode in hmtx:
        if charcode not in codes: continue
        #glyphname = codes[charcode]
        ttx.write ('    <map code="'+hex(charcode)+'" name="'+codes[charcode][0]+'"/><!-- '+codes[charcode][1]+' -->\n')

    ttx.write ('  </cmap_format_4>\n')

    ttx.write ('</cmap>\n')





  elif line.find("<!-- glyf -->")>=0:
    ###
    ###  glyf, with flat, noncolor outline
    ###     ---specified by name, not glyphid!!
    ###
    ttx.write ('<glyf>\n')
    
    SCALING = 1000./columnpixheight
    print ("scaling glyphs by "+str(SCALING))

    for glyfname in glyphmap:
        #ttx.write ("".join(['  <GlyphID id="'+str(gvalue)+'" name="'+glyphmap.keys()[list(glyphmap.values()).index(gvalue)]+'"/>\n' for gvalue in sorted(glyphmap.values())]))
            
        if glyfname in glyfs:
            ttx.write ('  <TTGlyph name="'+glyfname+'" xMin="0" yMin="0" xMax="1" yMax="1">\n') #bounds are recalculated by compiler

            glyf = glyfs[glyfname]
            hascontour = False

            for cmd, params in glyf:
                if len(params)>=2:
                    x1 = params[0]*SCALING
                    y1 = params[1]*SCALING
                if len(params)>=4:
                    x2 = params[2]*SCALING
                    y2 = params[3]*SCALING
                if len(params)>=6:
                    x3 = params[4]*SCALING
                    y3 = params[5]*SCALING

                if cmd=='M':
                    if hascontour:
                        ttx.write ('    </contour>\n')
                    hascontour = True
                    ttx.write ('    <contour>\n')
                    ttx.write ('      <pt x="'+str(x1)+'" y="'+str(750-y1)+'" on="1"/>\n')
                elif cmd=='L':
                    ttx.write ('      <pt x="'+str(x1)+'" y="'+str(750-y1)+'" on="1"/>\n')
                elif cmd=='C':
                    ttx.write ('      <pt x="'+str(x1)+'" y="'+str(750-y1)+'" on="0"/>\n')
                    ttx.write ('      <pt x="'+str(x3)+'" y="'+str(750-y3)+'" on="1"/>\n')

            if hascontour:
                ttx.write ('    </contour>\n')

            ttx.write (instructions)
            ttx.write ('  </TTGlyph>\n')
        else:
            ttx.write ('  <TTGlyph name="'+glyfname+'" xMin="0" yMin="0" xMax="0" yMax="0" />\n') #bounds are recalculated by compiler

    ttx.write ('</glyf>\n')




  elif line.find("<!-- hmtx -->")>=0:
    ###
    ###  hmtx
    ###
    #ttx.write ('len(hmtx)='+str(len(hmtx)))
    ttx.write ('<hmtx>\n')
    ttx.write ('  <mtx name=".notdef" width="500" lsb="30"/>"\n') #special mtx 
    ttx.write ('  <mtx name=".null" width="0" lsb="0"/>"\n')      #special mtx 
    ttx.write ('  <mtx name="space" width="200" lsb="0"/>\n')     #special for space
    ttx.write ('  <mtx name="nonmarkingreturn" width="250" lsb="0"/>\n')

    for charcode in hmtx:
        bounds = hmtx[charcode]
        #ttx.write (str(glyph)+" = "+str(bounds))

        if bounds[0]==-10000: 
            print ("No bounds info for "+charcode);
            continue

        #glyphname = codes[charcode]

        if charcode in codes:
            ttx.write('  <mtx name="'+codes[charcode][0]+'" width="'+str(int(2.9*(bounds[1]-bounds[0])))+'" lsb="'+str(int(bounds[0]))+'"/>\n')
        else:
            print('  missing glyph name for charcode '+str(charcode)+' !!!')

    ttx.write ("</hmtx>")


  elif line.find("<!-- CPAL -->")>=0:
    if (len(colors)>0):
        ttx.write('  <CPAL>\n    <version value="0"/>\n')
        ttx.write('    <numPaletteEntries value="'+str(len(colors))+'"/>\n')
        ttx.write('    <palette index="0">\n')

        index=0
        for color in colors:
            ttx.write('      <color index="'+str(index)+'" value="'+color+'FF"/>\n')
            index=index+1


        ttx.write('    </palette>\n')
        ttx.write('  </CPAL>\n')



  else:
    ttx.write(line)
        


print ("Done!")



# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 fileencoding=utf-8 textwidth=99
