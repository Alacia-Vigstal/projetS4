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

    probablement a utiliser pour reconstruire discretement les paths
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

# fonctions
def linkPaths(path):

def handleIntersections(pathList):

def path2Gcode(SVG, segment):
