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

class opComment(opResult):
    """
    classe pour contenir un commentaire sur une operation
    """

class opFeed(opResult):
    """
    classe pour contenir un F-word (le feed, en gros la vitesse de l'effecteur)
    """

class g0Result(opResult):
    """
    classe contenant une commande g0
    """

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