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
    def __init__(self, x=None, y=None, z=None):
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

# fonctions
def linkPaths(path):

def handleIntersections(pathList):

def path2Gcode(SVG, segment):
