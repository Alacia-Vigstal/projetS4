"""
Librairie permettant la conversion de path SVG en commandes g-code.
Responsable code librairie: Charles-William Lambert
Description: 
"""

# imports
import math
import os
import re
import sys
import numpy

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'svgpathtools'))
import svgpathtools

# classes
class line:
    """
    une ligne droite, represente un deplacement g1 en gcode vers le point specifie
    """
    def __init__(self, x = None, y = None, z = None):
        self.x = x
        self.y = y
        self.z = z
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
        
        if self.z is not None:
            if lineHasPos:
                lineContent += ", "

            lineContent += "z=%.4f" % self.z
            lineHasPos = True

        lineContent += ")"
        return lineContent

class arc:
    """
    un arc de cercle, represente un deplacement g2 ou g3 en gcode vers le point specifie

    ne pas utiliser directement, plutot utiliser une des sous classes precisant le sens de la rotation
    """
    def __init__(self, x = None, y = None, z = None, i = None, j = None):
        self.x = x
        self.y = y
        self.z = z
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

class SVG:
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
    GCODE_ORIGIN_IS_VIEWBOX_LOWER_LEFT = 0
    GCODE_ORIGIN_IS_SVG_ORIGIN = 1

    def __init__(self, SVG, origin = GCODE_ORIGIN_IS_VIEWBOX_LOWER_LEFT):
        self.SVGFileName = SVG
        self.SVGFile = svgpathtools.Document(self.SVGFileName)
        self.GcodeOrigin = origin
        print("Gcode origin: ", origin, file = sys.stderr)

        paths, attributes, svgAttributes = svgpathtools.svg2paths2(self.SVGFileName)
        self.SVGAttributes = svgAttributes  # Use this in place of self.SVGFile.root.attrib
        self.SVGPaths = []

        for path in paths:
            for continuous_path in path.continuous_subpaths():
                self.SVGPaths.append(continuous_path)

        """
        viewport definition
        """
        # height et width sont 'inversées'
        val, units, scale = self.findWidthHeight(self.SVGAttributes['height'])
        self.viewportHeight = val
        self.viewportUnitsY = units
        self.scaleX = scale

        val, units, scale = self.findWidthHeight(self.SVGAttributes['width'])
        self.viewportWidth = val
        self.viewportUnitsX = units
        self.scaleY = scale

        print("SVG viewport: ", file = sys.stderr)
        print("width: %.3f%s" % (self.viewportWidth, self.viewportUnitsX), file = sys.stderr)
        print("height: %.3f%s" % (self.viewportHeight, self.viewportUnitsY), file = sys.stderr)

        """
        viewbox definition
        """
        if 'viewBox' in self.SVGAttributes:
            print("svg viewBox:", self.SVGAttributes['viewBox'], file=sys.stderr)

            (xOrigin, yOrigin, width, height) = re.split(',|(?: +(?:, *)?)', self.SVGAttributes['viewBox'])
            self.viewBoxX = float(xOrigin)
            self.viewBoxY = float(yOrigin)
            self.viewBoxWidth = float(width)
            self.viewBoxHeight = float(height)

            self.scaleX *= self.viewportWidth / self.viewBoxWidth
            self.scaleY *= self.viewportHeight / self.viewBoxHeight

            print("viewBox_x:", self.viewBoxX, file=sys.stderr)
            print("viewBox_y:", self.viewBoxY, file=sys.stderr)
            print("viewBox_width:", self.viewBoxWidth, file=sys.stderr)
            print("viewBox_height:", self.viewBoxHeight, file=sys.stderr)
            print("x_scale:", self.scaleX, file=sys.stderr)
            print("y_scale:", self.scaleY, file=sys.stderr)

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
        
        if self.GcodeOrigin == self.GCODE_ORIGIN_IS_SVG_ORIGIN:
            out = x * self.scaleX

        elif self.GcodeOrigin == self.GCODE_ORIGIN_IS_VIEWBOX_LOWER_LEFT:
            out = (x - self.viewBoxX) * self.scaleX

        return out

    def y_mm(self, y):
        if type(y) not in [ float, numpy.float64 ]:
            raise SystemExit(f"non-float input, it's {type(y)}")
        
        # Dans un SVG y est positif vers le bas, mais en Gcode c'est positif vers le haut
        if self.GcodeOrigin == self.GCODE_ORIGIN_IS_SVG_ORIGIN:
            out = y * self.scaleY

        elif self.GcodeOrigin == self.GCODE_ORIGIN_IS_VIEWBOX_LOWER_LEFT:
            out = (self.viewBoxHeight - (y - self.viewBoxY)) * self.scaleY

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
        (segment1, segment2) = linkSegments(segment1, segment2)
        path[i] = segment1
        path[i+1] = segment2

    segment1 = path[-1]
    segment2 = path[0]
    (segment1, segment2) = linkSegments(segment1, segment2)
    path[-1] = segment1
    path[0] = segment2

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
                intersections[i][1] += 1 

            if intersections[i][1] >= otherSegIndex:
                intersections[i][1] += 1

        # Ajout de la nouvelle intersection qui vient d'être créée
        i = [currentSegIndex, otherSegIndex]
        intersections.append(i)

        # Incrémentation de l'index pour passer au prochain segment
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
    global currentZ

    if type(segment) == svgpathtools.path.Line:
        (start_x, start_y) = SVG.xy_mm(segment.start)
        (end_x, end_y) = SVG.xy_mm(segment.end)
        angle = computeOrientation(segment, t = 1.0)  # Calcul de l'orientation en fin de segment
        g1(x = end_x, y = end_y, z = currentZ, Zrot = angle)
        
    elif type(segment) is svgpathtools.path.Arc and (segment.radius.real == segment.radius.imag):
        (end_x, end_y) = SVG.xy_mm(segment.end)
        (center_x, center_y) = SVG.xy_mm(segment.center)
        startAngle = computeOrientation(segment, t = 0.0)  # Orientation au début de l'arc
        orientationList = computeOrientationList(segment, steps = 10)

        # Dans un SVG sweep == True -> clockwise et sweep == False -> counter-clockwise
        # Dans svgpathtools c'est l'inverse
        if segment.sweep:
            g2(x = end_x, y = end_y, z = currentZ, Zrot = startAngle, i = center_x, j = center_y, ZrotList = orientationList)

        else:
            g3(x = end_x, y = end_y, z = currentZ, Zrot = startAngle, i = center_x, j = center_y, ZrotList = orientationList)

    else:
        # Le segment n'es ni une ligne, ni un arc de cercle 
        # c'est donc arc elliptique ou une courbe de bezier
        # on utilise une approximation linéaire (ajuster le nombre de points au besoin)
        steps = 10

        for k in range(steps + 1):
            t = k / float(steps)
            end = segment.point(t)
            (end_x, end_y) = SVG.xy_mm(end)

            if k > 0:
                angle = computeOrientation(segment, t = t)
            else:
                angle = computeOrientation(segment, t = t + 1e-5)

            g1(x = end_x, y = end_y, z = currentZ, Zrot = angle)

# Z0 veut dire up (pas en contact avec le tapis de découpe)
def path2Gcode(SVG, path, zRapid = 3.0, zCutDepth = 0.0):
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
        raise ValueError("Path is empty")

    # tolérance pour déterminer si besoin de retrait lors de changements de direction
    ANGLE_TOLERANCE = 1.0
    global currentX
    currentX = 0.0
    global currentY
    currentY = 0.0
    global currentZ
    currentZ = zRapid
    global currentZrot
    currentZrot = 0.0

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

    # s'assure que l'outil est en position haute
    #g1(x = currentX, y = currentY, z = zRapid, Zrot = currentZrot)

    # déplacement de l'outil vers la position de début de découpe
    start_x, start_y = SVG.xy_mm(path[0].start)
    startAngle = computeOrientation(path[0], t = 0.0)
    g0(x = start_x, y = start_y, z = zRapid, Zrot = startAngle)
    currentX = start_x
    currentY = start_y
    currentZrot = startAngle

    #comment("Début de la découpe simple avec retrait lors de changements de direction")

    # Descente de l'outil
    toolDown()

    prevDirection = unitDirectionVector(path[0])

    # Coupe du premier segment
    pathSegment2Gcode(SVG, path[0])

    # Coupe des segments suivants avec retrait lors de changements de direction
    # clean up cette section pour match ce qu'on veut comme gcode de sortie
    for i in range(1, len(path)):
        currentSegment = path[i]
        currentDirection = unitDirectionVector(currentSegment)

        angleDeg = angleBetween(prevDirection, currentDirection)

        if not (abs(angleDeg - 0)   <= ANGLE_TOLERANCE or abs(angleDeg - 180) <= ANGLE_TOLERANCE):
            toolUp()
            seg_x, seg_y = SVG.xy_mm(currentSegment.start)
            nextAngle = computeOrientation(currentSegment, t = 0.0)
            g0(x = seg_x, y = seg_y, z = currentZ, Zrot = nextAngle)

            toolDown()

        pathSegment2Gcode(SVG, currentSegment)
        prevDirection = currentDirection

    comment("Fin de la découpe")

# Paramètres pour track la position de l'outil
currentX = None
currentY = None
# Z False veut dire up (pas en contact avec le tapis de découpe)
currentZ = None
currentZrot = None

"""
def init():
    print()
    print("; init")
    print("G20          (pouces)")
    print("G17          (plan xy)")
    print("G90          (position absolue)")
    print("G91.1        (le centre des arcs est relatif à la position de départ des arcs)")
    print("G54          (système de coordonnées de travail)")
    print()
"""

def comment(msg):
    if msg:
        print(";", msg)

    else:
        print()

"""
def absolute():
    print("G90")

def absoluteArcCenters():
    print("G90.1")

def relativeArcCenters():
    print("G91.1")
"""

def toolUp():
    comment('')
    global currentX
    global currentY
    global currentZ 
    global currentZrot
    currentZ = 3.0
    g1(x = currentX, y = currentY, z = currentZ, Zrot = currentZrot)

def toolDown():
    comment('')
    global currentX
    global currentY
    global currentZ
    global currentZrot
    currentZ = 0.0
    g1(x = currentX, y = currentY, z = currentZ, Zrot = currentZrot)

"""
def presentationPosition():
    imperial()
    absolute()
    toolUp()

    print("G53 G0 X9 Y12")
    global currentX
    global currentY
    currentX = None
    currentY = None

def m2():
    print()
    print("M2")
"""

def done():
    print()
    print("; done")
    #presentationPosition()
    print("M2")

"""
def imperial():
    print("G20")

def metric():
    print("G21")
"""

def coordToStr(val=None):
    if val == None:
        return ""
    
    if okToRound(val, 0.0):
        val = 0.000

    return "%.4f" % val

def g0(path = None, x = None, y = None, z = None, Zrot = None):
    global currentX
    global currentY
    global currentZ
    global currentZrot

    if path is not None:
        print()
        print("; g0 path")

        for waypoint in path:
            g0(**waypoint)

        print()

    else:
        print("G0", end='')

        if x is not None:
            currentX = x
            print(" X%s" % coordToStr(x), end = '')

        if y is not None:
            currentY = y
            print(" Y%s" % coordToStr(y), end = '')

        if z is not None:
            currentZ = z
            print(" Z%s" % coordToStr(z), end = '')
        
        if Zrot is not None:
            currentZrot = Zrot
            print(" Zrot%s" % coordToStr(Zrot), end='') # vérifier que coordToStr fonctionne bien dans ce cas
        
        print()

def g1(path = None, x = None, y = None, z = None, Zrot = None):
    global currentX
    global currentY
    global currentZ
    global currentZrot

    if path is not None:
        print()
        print("; g1 path")

        for waypoint in path:
            g1(**waypoint)
        print()

    else:
        print("G1", end = '')
        if x is not None:
            currentX = x
            print(" X%s" % coordToStr(x), end = '')

        if y is not None:
            currentY = y
            print(" Y%s" % coordToStr(y), end = '')
        
        if z is not None:
            currentZ = z
            print(" Z%s" % coordToStr(z), end = '')
        
        if Zrot is not None:
            currentZrot = Zrot
            print(" Zrot%s" % coordToStr(Zrot), end='') # vérifier que coordToStr fonctionne bien dans ce cas

        print()

def g2(x = None, y = None, z = None, Zrot = None, i = None, j = None, ZrotList = None):
    global currentX
    global currentY
    global currentZ
    global currentZrot

    if i is None and j is None:
        raise TypeError("gcoder.g2() without i or j")
    
    print("G2", end = '')

    if x is not None:
        currentX = x
        print(" X%s" % coordToStr(x), end = '')

    if y is not None:
        currentY = y
        print(" Y%s" % coordToStr(y), end = '') 
    
    if z is not None:
        currentZ = z
        print(" Z%s" % coordToStr(z), end = '')
    
    if Zrot is not None:
        print(" Zrot%s" % coordToStr(Zrot), end='')

    if i is not None: 
        print(" I%s" % coordToStr(i), end = '')

    if j is not None: 
        print(" J%s" % coordToStr(j), end = '')

    if ZrotList is not None:
        currentZrot = ZrotList
        ZrotStr = ",".join(str(coordToStr(angle)) for angle in ZrotList)
        print(" ZrotL:%s" % ZrotStr, end='')

    print()

def g3(x = None, y = None, z = None, Zrot = None, i = None, j = None, ZrotList = None):
    global currentX
    global currentY
    global currentZ
    global currentZrot

    if i is None and j is None:
        raise TypeError("gcoder.g3() without i or j")
    
    print("G3", end = '')

    if x is not None:
        currentX = x
        print(" X%s" % coordToStr(x), end = '')

    if y is not None:
        currentY = y
        print(" Y%s" % coordToStr(y), end = '')
    
    if z is not None:
        currentZ = z
        print(" Z%s" % coordToStr(z), end = '')
    
    if Zrot is not None:
        print(" Zrot%s" % coordToStr(Zrot), end='')

    if i is not None: 
        print(" I%s" % coordToStr(i), end = '')

    if j is not None: 
        print(" J%s" % coordToStr(j), end = '')

    if ZrotList is not None:
        currentZrot = ZrotList
        ZrotStr = ",".join(str(coordToStr(angle)) for angle in ZrotList)
        print(" ZrotL:%s" % ZrotStr, end='')

    print()

def okToRound(a, b):
    """
    Est vrai si la différence entre a et b est moins que (1e-6) 
    Sinon, retourne faux.
    """
    
    return abs(a - b) < 1e-6

def computeOrientation(segment, t = 1.0):
    """
    Calcul l'orientation de l'outil pour le prochain segment à découper
    """
    tangent = segment.derivative(t)  # tangent est un nombre complexe
    angle = math.degrees(math.atan2(tangent.imag, tangent.real))

    return angle

def computeOrientationList(segment, steps = 10):
    """
    Retourne une liste d'angles (en degrés) pour le segment à des positions t allant de 0 à 1.
    """
    orientationList = []

    for k in range(steps + 1):
        t = k / float(steps)
        orientationList.append(computeOrientation(segment, t))

    return orientationList