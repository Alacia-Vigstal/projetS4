# Programme lisant les paths d'un SVG et produisant les commandes Gcode correspondantes (appelle la librairie)

# imports
import argparse
import json
import jsonschema
import math
import os
import sys

import Gcode_Generator

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'svgpathtools'))
import svgpathtools

# classes
class opResult(object):
    """
    classe parente pour contenir le resultat de n'importe quelle operation
    """
    def __init__(self):
        self.comment = []
        pass

    def emit(self):
        pass

class opComment(opResult):
    """
    classe pour contenir un commentaire sur une operation
    """
    def __init__(self, comment):
        self.comment = [comment]

    def emit(self):
        for comment in self.comment:
            print(";", comment)

class opFeed(opResult):
    """
    classe pour contenir un F-word (le feed, en gros la vitesse de l'effecteur)
    """
    def __init__(self, feed):
        self.comment = []
        self.feed = feed

    def emit(self):
        Gcode_Generator.set_feed_rate(self.feed)

class g0Result(opResult):
    """
    classe contenant une commande g0
    """
    def __init__(self, x = None, y = None, z = None):
        self.comment = []
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        s = "ResultG0("
        separator = ""
        if self.x is not None:
            s += "%sx=%.4f" % (separator, self.x)
            separator = ", "
        if self.y is not None:
            s += "%sy=%.4f" % (separator, self.y)
            separator = ", "
        if self.z is not None:
            s += "%sz=%.4f" % (separator, self.z)
        s += ")"
        return s

    def emit(self):
        gcoder.g0(x=self.x, y=self.y, z=self.z)

class g1Result(opResult):
    """
    classe contenant une commande g1
    """

class pathResult(opResult):
    """
    classe contenant un path a transformer en une sequence de commandes Gcode
    """

# fonctions
def makeOpDict():


def followPath():


def removeIsland():


def createGcodeFile():


def handlePath():