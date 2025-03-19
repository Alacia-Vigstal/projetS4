# Programme ecrivant du Gcode (la librairie)

# imports
import math
import os
import re
import sys
import numpy

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'svgpathtools'))
import svgpathtools

# classes
class line(object):
    """
    une ligne droite, represente un deplacement g1 en gcode vers le point specifie
    """
    def __init__(self, x=None, y=None):
        self.x = x
        self.y = y
        #x, y sont les coordonees de la position de fin

    def __str__(self):
        lineHasPos = False
        lineContent = "Line("

        if self.x is not None:
            lineContent += "x=%.4f" % self.x
            lineHasPos = True

        if self.y is not None:
            if lineHasPos:
                lineContent += ", "
            lineContent += "y=%.4f" % self.y
            lineHasPos = True

        lineContent += ")"
        return lineContent

class arc(object):
    """
    un arc de cercle, represente un deplacement g2 ou g3 en gcode vers le point specifie

    ne pas utiliser directement, plutot utiliser une des sous classes precisant le sens de la rotation
    """
    def __init__(self, x=None, y=None, z=None, i=None, j=None, p=None):
        self.x = x
        self.y = y
        #x, y, z sont les coordonees de la position de fin

        self.i = i
        self.j = j
        #i, j sont les coordonees du centre de rotation de l'arc

    def __str__(self):
        arcHasPos = False
        arcContent = self.__class__.__name__ + "("

        if self.x is not None:
            arcContent += "x=%.4f" % self.x
            arcHasPos = True

        if self.y is not None:
            if arcHasPos:
                arcContent += ", "
            arcContent += "y=%.4f" % self.y
            arcHasPos = True

        if self.z is not None:
            if arcHasPos:
                arcContent += ", "
            arcContent += "z=%.4f" % self.z
            arcHasPos = True

        if self.i is not None:
            if arcHasPos:
                arcContent += ", "
            arcContent += "i=%.4f" % self.i
            arcHasPos = True

        if self.j is not None:
            if arcHasPos:
                arcContent += ", "
            arcContent += "j=%.4f" % self.j
            arcHasPos = True

        arcContent += ")"
        return arcContent


class arcCW(arc):
    """
    enfant de arc specifiquement pour des arcs de cercle horaires (g2)
    """

class arcCCW(arc):
    """
    enfant de arc specifiquement pour des arcs de cercle anti-horaires (g3)
    """

class SVG():
    """
    set l'origine et convertit les unitees du svg vers des mm

    dans un svg, la hauteur et largeur du svg sont specifies dans une unitee
    conventionnelle et peuvent etre converties en mm

    il se peut que le svg specifie un 'cadre' qui represente en gros l'univers du dessin dans
    lequel un origine est defini. Dans ce cas, les coordonnes de l'origine sont celles du coin
    en bas a gauche du 'cadre'. 

    si il n'y a pas de cadre, on utilise les unitees du viewport

    enfin, on trouve un facteur de conversion convertX et convertY pour reporter les unitees en mm.
    """
    viewboxLowerLeftOrigin = 0
    SVGOrigin = 1

    def __init__(self, SVG, origin = viewboxLowerLeftOrigin):
        self.SVGFileName = SVG
        self.SVGFile = svgpathtools.Document(self.SVGFileName)
        self.GcodeOrigin = origin
        print("Gcode origin: ", origin, file = sys.stderr)

        self.SVGPaths = []

        for path in self.SVGFile.SVGPaths():
            for continuous_path in path.continuous_subpaths():
                self.paths.append(continuous_path)
        
        self.SVGAttributes = self.SVGFile.root.attrib

        """
        viewport definition
        """
        val, units, scale = self._parse_height_width(self.SVGAttributes['height'])
        self.viewportHeight = val
        self.viewportUnitsY = units
        self.scaleX = scale

        val, units, scale = self._parse_height_width(self.SVGAttributes['width'])
        self.viewportWidth = val
        self.viewportUnitsX = units
        self.scaleY = scale

        print("SVG viewport: ", file = sys.stderr)
        print("width: %.3f%s" % (self.viewportWidth, self.viewportUnitsX), file = sys.stderr)
        print("height: %.3f%s" % (self.viewportHeight, self.viewportUnitsY), file = sys.stderr)

        """
        viewbox definition
        """
        if 'viewbox' in self.SVGAttributes:
            print("svg viewBox:", self.svg_attributes['viewBox'], file=sys.stderr)

            (xOrigin, yOrigin, width, height) = re.split(',|(?: +(?:, *)?)', self.svg_attributes['viewBox'])
            self.viewBoxX = float(xOrigin)
            self.viewBoxY = float(yOrigin)
            self.viewBoxWidth = float(width)
            self.viewBoxHeight = float(height)

            self.xScale *= self.viewportWidth / self.viewBoxWidth
            self.yScale *= self.viewportHeight / self.viewBoxHeight

            print("viewBox_x:", self.viewBoxX, file=sys.stderr)
            print("viewBox_y:", self.viewBoxY, file=sys.stderr)
            print("viewBox_width:", self.viewBoxWidth, file=sys.stderr)
            print("viewBox_height:", self.viewBoxHeight, file=sys.stderr)
            print("x_scale:", self.xScale, file=sys.stderr)
            print("y_scale:", self.yScale, file=sys.stderr)

        else:
            self.viewBoxX = 0.0
            self.viewBoxY = 0.0
            self.viewBoxWidth = self.viewportWidth
            self.viewBoxHeight = self.viewportHeight

    def findWidthHeight(self, s):
        """
        trouve la largeur et la hauteur du SVG et convertion en mm
        """

        m = re.match('^([0-9.]+)([a-zA-Z]*)$', s)

        if m == None or len(m.groups()) != 2:
            raise SystemExit("failed to parse SVG viewport height/width: %s" % s)

        val = float(m.group(1))
        unit = m.group(2)

        # dictionnaire pour les conversions en mm
        units = {
            # pixel "px" ou aucune unitée (on assume alors pixel) donne 1 inch/96 px * 25.4 mm/1 inch = 25.4/96 mm/px
            '': 25.4/96,
            'px': 25.4/96,

            # points "pt" donne 1 inch/72 pt * 25.4 mm/1 inch = 25.4/72 mm/pt
            'pt': 25.4/72,

            # picas "pc", donne 1 inch/6 pc * 25.4 mm/1 inch = 25.4/6 mm/pc
            'pc': 25.4/6,

            'cm': 10.0,
            'mm': 1.0,
            'in': 25.4
        }

        if unit not in units:
            raise SystemExit("unknwn SVG viewport units: '%s'" % unit)

        scale = units[unit]

        return (val, unit, scale)

    def x_mm(self, x):
        if type(x) not in [ float, numpy.float64 ]:
            raise SystemExit(f"non-float input, it's {type(x)}")
        
        if self.gcode_origin == self.GCODE_ORIGIN_IS_SVG_ORIGIN:
            out = x * self.x_scale

        elif self.gcode_origin == self.GCODE_ORIGIN_IS_VIEWBOX_LOWER_LEFT:
            out = (x - self.viewBox_x) * self.x_scale

        return out

    def y_mm(self, y):
        if type(y) not in [ float, numpy.float64 ]:
            raise SystemExit(f"non-float input, it's {type(y)}")
        
        # Dans un SVG y est positif vers le bas, mais en Gcode c'est positif vers le haut
        if self.gcode_origin == self.GCODE_ORIGIN_IS_SVG_ORIGIN:
            out = -y * self.y_scale

        elif self.gcode_origin == self.GCODE_ORIGIN_IS_VIEWBOX_LOWER_LEFT:
            out = (self.viewBox_height - (y - self.viewBox_y)) * self.y_scale

        return out

    def xy_mm(self, xy):
        if type(xy) not in [ complex, numpy.complex128 ]:
            raise SystemExit(f"non-complex input, it's {type(xy)}")
        
        x = self.x_mm(xy.real)
        y = self.y_mm(xy.imag)
        return (x, y)
    
# fonctions
def linkPaths(path):
    def linkSegments(segment1, segment2):

        if segment1.end != segment2.start:
            if okToRound(segment1.end, segment2.start):
                avg = (segment1.end + segment2.start) / 2.0
                segment1.end = avg
                segment2.start = avg

            else:
                raise ValueError("segments are not even close to closed: %s, %s" % (segment1, segment2))
        
        return (segment1, segment2)
    
    for i in range(len(path)-1):
        segment1 = path[i]
        segment2 = path[i+1]
        (this_seg, next_seg) = linkSegments(this_seg, next_seg)
        path[i] = this_seg
        path[i+1] = next_seg

    this_seg = path[-1]
    next_seg = path[0]
    (this_seg, next_seg) = linkSegments(this_seg, next_seg)
    path[-1] = this_seg
    path[0] = next_seg

    path.closed = True

    return path

def handleIntersections(pathList):
    """
    Trouve les points ou les segments s'intersectent et les divise en deux segments
    """
    if type(pathList) == svgpathtools.path.Path:
        l = []
        for i in range(len(pathList)):
            l.append(pathList[i])
        pathList = l

    def findFirstIntersection(pathList, currentSegIndex):
        currentSegment = pathList[currentSegIndex]

        earliestCurrent = None
        earliestOtherSegIndex = None
        earliestOther = None

        for otherSegIndex in range(currentSegIndex + 2, len(pathList)):
            otherSegment = pathList[otherSegIndex]
            intersections = currentSegment.intersect(otherSegment)

            if len(intersections) == 0:
                continue

            for intersection in intersections:
                if okToRound(intersection[0], 0.0) or okToRound(intersection[0], 1.0):
                    continue

                if intersection[0] > 1.0 or intersection[0] < 0.0:
                    continue

                if (earliestCurrent == None) or (intersection[0] < earliestCurrent):
                    earliestCurrent = intersection[0]
                    earliestOtherSegIndex = otherSegIndex
                    earliestOther = intersection[1]

        return (earliestCurrent, earliestOtherSegIndex, earliestOther)

    intersections = []
    currentSegIndex = 0

    while currentSegIndex < len(pathList):
        currentSeg = pathList[currentSegIndex]
        current, otherSegIndex, other = findFirstIntersection(pathList, currentSegIndex)

        if current == None:
            currentSegIndex += 1
            continue

        otherSeg = pathList[otherSegIndex]
        currentFirstSeg, currentSecondSeg = currentSeg.split(current)
        otherFirstSeg, otherSecondSeg = otherSeg.split(other)

        otherFirstSeg.end = currentFirstSeg.end
        otherSecondSeg.start = otherFirstSeg.end

        assert(okToRound(currentFirstSeg.end, currentSecondSeg.start))
        assert(okToRound(currentFirstSeg.end, otherFirstSeg.end))
        assert(okToRound(currentFirstSeg.end, otherSecondSeg.start))

        assert(okToRound(currentFirstSeg.start, currentSeg.start))
        assert(okToRound(currentSecondSeg.end, currentSeg.end))

        assert(okToRound(otherFirstSeg.start, otherSeg.start))
        assert(okToRound(otherSecondSeg.end, otherSeg.end))

        # Remplacement de currentSeg par le nouveau premier sous segment dans la liste
        pathList[currentSegIndex] = currentFirstSeg

        # Ajout du nouveau second sous segment juste après le premier
        pathList.insert(currentSegIndex + 1, currentSecondSeg)
        otherSegIndex += 1

        # Remplacement de oherSeg par le nouveau premier sous segment dans la liste
        pathList[otherSegIndex] = otherFirstSeg

        # Ajout du nouveau second sous segment juste après le premier
        pathList.insert(otherSegIndex + 1, otherSecondSeg)

        for i in range(len(intersections)):
            if intersections[i][1] >= currentSegIndex:
                intersections[i][1] += 1  # for currentSeg that got split

            if intersections[i][1] >= otherSegIndex:
                intersections[i][1] += 1  # for otherSeg that got split

        # Add this new intersection we just made.
        i = [currentSegIndex, otherSegIndex]
        intersections.append(i)

        # Look for intersections in the remainder of this_seg (the second
        # part of the split).
        currentSegIndex += 1

        paths = []

        while True:
            path = []

            # Début au premier segment disponible
            segIndex = 0
            for segIndex in range(len(pathList)):
                if pathList[segIndex] != None:
                    break

            while segIndex < len(pathList):
                if pathList[segIndex] == None:
                    # fini avec ce path
                    break

                path.append(pathList[segIndex])
                pathList[segIndex] = None

                i = None
                for i in intersections:
                    if segIndex == i[0] or segIndex == i[1]:
                        break

                if (i is not None) and (i[0] == segIndex):
                    # Trouvée la première entrée d'une intersection, prendre la deuxième sortie
                    segIndex = i[1] + 1

                elif (i is not None) and (i[1] == segIndex):
                    # Déjà dans la deuxième entrée, prendre la première sortie
                    segIndex = i[0] + 1
                else:
                    # Pas une intersection
                    segIndex += 1

            if path == []:
                break

            paths.append(path)

    return paths

def pathSegment2Gcode(SVG, segment):
    if type(segment) == svgpathtools.path.Line:
        (start_x, start_y) = SVG.xy_mm(segment.start)
        (end_x, end_y) = SVG.xy_mm(segment.end)
        g1(x = end_x, y = end_y)
        
    elif type(segment) is svgpathtools.path.Arc and (segment.radius.real == segment.radius.imag):
        (end_x, end_y) = SVG.xy_mm(segment.end)
        (center_x, center_y) = SVG.xy_mm(segment.center)

        # Dans un SVG sweep == True -> clockwise et sweep == False -> counter-clockwise
        # Dans svgpathtools c'est l'inverse
        if segment.sweep:
            g2(x = end_x, y = end_y, i = center_x, j = center_y)

        else:
            g3(x = end_x, y = end_y, i = center_x, j = center_y)

    else:
        # Le segment n'es ni une ligne, ni un arc de cercle 
        # c'est donc arc elliptique ou une courbe de bezier
        # on utilise une approximation linéaire (ajuster le nombre de points au besoin)
        steps = 1000

        for k in range(steps+1):
            t = k / float(steps)
            end = segment.point(t)
            (end_x, end_y) = SVG.xy_to_mm(end)
            g1(x = end_x, y = end_y)

# Possibilité d'ajouter les paramètres leadIn = True, leadOut = True pour fine tune les débuts et fins de parcours
# Discuter de la nécessité d'ajouter un paramètre pour le feed
def path2Gcode(SVG, path, zRapid = 10.0, zCutDepth = 0.0, feed = None, plungeFeed = None):
    """
    Output le Gcode pour les paths d'un SVG donné.
    Le Gcode est généré pour un cutter (pas de spindle) avec retrait entre les coupes pour les changements de direction
    
    arguments:
    - SVG: l'objet SVG
    - path: liste des paths à convertir
    - zRapid: paramètre pour la hauteur de l'outil lors de déplacements rapides
    - zCutDepth: paramètre pour la hauteur de l'outil lors de la coupe
    - feed: paramètre pour la vitesse de coupe horizontale
    - plungeFeed: paramètre pour la vitesse de descente verticale
    """
    if not path:
        raise ValueError("Path is empty; nothing to cut.")

    # tolérance pour déterminer si besoin de retrait lors de changements de direction
    ANGLE_TOLERANCE = 1.0

    def unitDirectionVector(segment):
        xStart, yStart = SVG.xy_mm(segment.start)
        xEnd, yEnd = SVG.xy_mm(segment.end)
        dx = xEnd - xStart
        dy = yEnd - yStart
        length = math.hypot(dx, dy)

        if length < 1e-9:
            return (0.0, 0.0)
        
        return (dx / length, dy / length)

    def angleBetween(vec1, vec2):
        dot = max(-1.0, min(1.0, vec1[0] * vec2[0] + vec1[1] * vec2[1]))
        return math.degrees(math.acos(dot))

    g0(z = zRapid)

    start_x, start_y = SVG.xy_mm(path[0].start)
    g0(x = start_x, y = start_y)

    comment("Begin single-pass cutting with auto-retraction on direction changes")

    if plungeFeed:
        setFeedRate(plungeFeed)

    elif feed:
        setFeedRate(feed)

    g1(z = zCutDepth)

    if feed:
        setFeedRate(feed)

    prevDirection = unitDirectionVector(path[0])
    pathSegment2Gcode(SVG, path[0], z = zCutDepth)

    for i in range(1, len(path)):
        currentSegment = path[i]
        currentDirection = unitDirectionVector(currentSegment)

        angleDeg = angleBetween(prevDirection, currentDirection)

        if not (abs(angleDeg - 0)   <= ANGLE_TOLERANCE or abs(angleDeg - 180) <= ANGLE_TOLERANCE):
            g1(z = zRapid)
            seg_x, seg_y = SVG.xy_mm(currentSegment.start)
            g0(x = seg_x, y = seg_y)

            if plungeFeed:
                setFeedRate(plungeFeed)

            elif feed:
                setFeedRate(feed)

            g1(z = zCutDepth)

            if feed:
                setFeedRate(feed)

        pathSegment2Gcode(SVG, currentSegment, z = zCutDepth)
        prev_direction = currentDirection

    comment("End of cut - no lead-out or spindle logic needed")

# Paramètres pour track la position de l'outil
currentX = None
currentY = None
currentA = None
currentB = None
currentC = None
currentU = None
currentV = None
currentW = None
currentFeed = None

def init():
    print()
    print("; init")
    print("G20          (pouces)")
    print("G17          (plan xy)")
    print("G90          (position absolue)")
    print("G91.1        (le centre des arcs est relatif à la position de départ des arcs)")
    cutter_comp_off()
    print("G54          (système de coordonnées de travail)")
    print("G94          (feed (unités/minute))")
    print()


def comment(msg):
    if msg:
        print(";", msg)

    else:
        print()


def absolute():
    print("G90")

def absolute_arc_centers():
    print("G90.1")

def relative_arc_centers():
    print("G91.1")

def path_blend(tolerance=None):
    print("G64 P%.4f (enable path blending with tolerance)" % tolerance)


def quill_up():
    absolute()
    cutter_comp_off()
    print("G53 G0 Z0")
    current_z = None
    spindle_off()


def presentation_position():
    imperial()
    quill_up()

    # rapid to presentation position
    # table centered in X, all the way forward towards the user
    print("G53 G0 X9 Y12")
    current_x = None
    current_y = None


def m2():
    print()
    print("M2")


def done():
    print()
    print("; done")
    presentation_position()
    print("M2")


def imperial():
    print("G20")

def metric():
    print("G21")

def setFeedRate(feed_rate_units_per_minute):
    print("F %.4f" % feed_rate_units_per_minute)

def speed(spindle_rpm):
    print("S %d" % spindle_rpm)

def coord_to_str(val=None):
    if val == None:
        return ""
    
    if okToRound(val, 0.0):
        val = 0.000

    return "%.4f" % val


# FIXME: g0(path) should be merged or replaced by z_path() somehow
def g0(path=None, x=None, y=None, z=None, a=None, b=None, c=None, u=None, v=None, w=None):
    global current_x
    global current_y
    global current_z
    global current_a
    global current_b
    global current_c
    global current_u
    global current_v
    global current_w

    if path is not None:
        print()
        print("; g0 path")
        for waypoint in path:
            g0(**waypoint)
        print()

    else:
        print("G0", end='')
        if x is not None:
            current_x = x
            print(" X%s" % coord_to_str(x), end='')

        if y is not None:
            current_y = y
            print(" Y%s" % coord_to_str(y), end='')

        if z is not None:
            current_z = z
            print(" Z%s" % coord_to_str(z), end='')

        if a is not None:
            current_a = a
            print(" A%s" % coord_to_str(a), end='')

        if b is not None:
            current_b = b
            print(" B%s" % coord_to_str(b), end='')

        if c is not None:
            current_c = c
            print(" C%s" % coord_to_str(c), end='')

        if u is not None:
            current_u = u
            print(" U%s" % coord_to_str(u), end='')

        if v is not None:
            current_v = v
            print(" V%s" % coord_to_str(v), end='')

        if w is not None:
            current_w = w
            print(" W%s" % coord_to_str(w), end='')

        print()


def g1(path=None, x=None, y=None, z=None, a=None, b=None, c=None, u=None, v=None, w=None):
    global current_x
    global current_y
    global current_z
    global current_a
    global current_b
    global current_c
    global current_u
    global current_v
    global current_w

    if path is not None:
        print()
        print("; g1 path")
        for waypoint in path:
            g1(**waypoint)
        print()

    else:
        print("G1", end='')
        if x is not None:
            current_x = x
            print(" X%s" % coord_to_str(x), end='')

        if y is not None:
            current_y = y
            print(" Y%s" % coord_to_str(y), end='')

        if z is not None:
            current_z = z
            print(" Z%s" % coord_to_str(z), end='')

        if a is not None:
            current_a = a
            print(" A%s" % coord_to_str(a), end='')

        if b is not None:
            current_b = b
            print(" B%s" % coord_to_str(b), end='')

        if c is not None:
            current_c = c
            print(" C%s" % coord_to_str(c), end='')

        if u is not None:
            current_u = u
            print(" U%s" % coord_to_str(u), end='')

        if v is not None:
            current_v = v
            print(" V%s" % coord_to_str(v), end='')

        if w is not None:
            current_w = w
            print(" W%s" % coord_to_str(w), end='')

        print()


def g2(x=None, y=None, z=None, i=None, j=None, p=None):
    global current_x
    global current_y
    global current_z

    if i is None and j is None:
        raise TypeError("gcoder.g2() without i or j")
    
    print("G2", end='')

    if x is not None:
        current_x = x
        print(" X%s" % coord_to_str(x), end='')

    if y is not None:
        current_y = y
        print(" Y%s" % coord_to_str(y), end='')

    if z is not None:
        current_z = z
        print(" Z%s" % coord_to_str(z), end='')

    if i is not None: 
        print(" I%s" % coord_to_str(i), end='')

    if j is not None: 
        print(" J%s" % coord_to_str(j), end='')

    if p is not None: 
        print(" P%s" % coord_to_str(p), end='')

    print()


def g3(x=None, y=None, z=None, i=None, j=None, p=None):
    global current_x
    global current_y
    global current_z

    if i is None and j is None:
        raise TypeError("gcoder.g3() without i or j")
    
    print("G3", end='')

    if x is not None:
        current_x = x
        print(" X%s" % coord_to_str(x), end='')

    if y is not None:
        current_y = y
        print(" Y%s" % coord_to_str(y), end='')

    if z is not None:
        current_z = z
        print(" Z%s" % coord_to_str(z), end='')

    if i is not None: 
        print(" I%s" % coord_to_str(i), end='')

    if j is not None: 
        print(" J%s" % coord_to_str(j), end='')

    if p is not None: 
        print(" P%s" % coord_to_str(p), end='')

    print()

#
# Cutter compensation handling.
#

def cutter_comp_off():
    print("G40          (cutter comp off)")

def cancel_cutter_comp():
    print("; gcoder: calling program used obsolete cancel_cutter_comp() function, use cutter_comp_off() instead")
    cutter_comp_off()

def g40():
    print("; gcoder: calling program used obsolete g40() function, use cutter_comp_off() instead")
    cutter_comp_off()


def cutter_comp_left(**kwargs):

    """Enable cutter diameter compensation on the left side of the
    programmed path.

    When called with no argument, uses the diameter of the currently
    loaded tool (from the tool table).

    When called with the `diameter` argument, uses the specified diameter.

    When called with the `tool` argument (and without the `diameter`
    argument), uses the diameter of the specified tool number (from the
    tool table)."""

    if 'diameter' in kwargs:
        print("G41.1 D%.4f   (cutter comp left, diameter mode)" % kwargs['diameter'])
    elif 'tool' in kwargs:
        print("G41 D%d   (cutter comp left, tool-number mode)" % kwargs['tool'])
    else:
        print("G41   (cutter comp left, current tool)")


def cutter_comp_right(**kwargs):

    """Enable cutter diameter compensation on the right side of the
    programmed path.

    When called with no argument, uses the diameter of the currently
    loaded tool (from the tool table).

    When called with the `diameter` argument, uses the specified diameter.

    When called with the `tool` argument (and without the `diameter`
    argument), uses the diameter of the specified tool number (from the
    tool table)."""

    if 'diameter' in kwargs:
        print("G42.1 D%.4f   (cutter comp right, diameter mode)" % kwargs['diameter'])
    elif 'tool' in kwargs:
        print("G42 D%d   (cutter comp right, tool-number mode)" % kwargs['tool'])
    else:
        print("G42   (cutter comp right, current tool)")

def g42_1(comp_diameter):
    print("; gcoder: calling program used obsolete g42_1() function, use cutter_comp_right() instead")
    cutter_comp_right(diameter=comp_diameter)


def g81(retract, x=None, y=None, z=None):
    global current_x
    global current_y
    global current_z

    print("G81", end='')
    if x is not None:
        current_x = x
        print(" X%s" % coord_to_str(x), end='')
    if y is not None:
        current_y = y
        print(" Y%s" % coord_to_str(y), end='')
    if z is not None:
        print(" Z%s" % coord_to_str(z), end='')
    print(" R%s" % coord_to_str(retract), end='')
    print()
    # FIXME: keep track of retract mode, set Z correctly here
    current_z = None


def g83(retract, delta, x=None, y=None, z=None):
    global current_x
    global current_y
    global current_z

    print("G83", end='')
    if x is not None:
        current_x = x
        print(" X%s" % coord_to_str(x), end='')
    if y is not None:
        current_y = y
        print(" Y%s" % coord_to_str(y), end='')
    if z is not None:
        print(" Z%s" % coord_to_str(z), end='')
    print(" R%s" % coord_to_str(retract), end='')
    print(" Q%s" % coord_to_str(delta), end='')
    print()
    # FIXME: keep track of retract mode, set Z correctly here
    current_z = None

def okToRound(a, b):
    """
    Est vrai si la différence entre `a` et `b` est moins que (1e-6) 
    Sinon, retourne faux.
    """
    
    return abs(a - b) < 1e-6
