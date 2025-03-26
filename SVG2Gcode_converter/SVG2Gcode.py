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
        Gcode_Generator.g0(x=self.x, y=self.y, z=self.z)

class g1Result(opResult):
    """
    classe contenant une commande g1
    """
    def __init__(self, x = None, y = None, z = None):
        self.comment = []
        self.x = x
        self.y = y
        self.z = z
    
    def __str__(self):
        s = "ResultG1("
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
        Gcode_Generator.g1(x=self.x, y=self.y, z=self.z)

class pathResult(opResult):
    """
    classe contenant un path a transformer en une sequence de commandes Gcode
    """
    def __init__(self, SVG, path, zRapid=False, zCutDepth=True):
        self.SVG = SVG
        self.path = path
        self.zRapid = zRapid
        self.zCutDepth = zCutDepth

    def emit(self):
        # Utilise la fonction de conversion du SVG vers G-code
        Gcode_Generator.path2Gcode(self.SVG, self.path, self.zRapid, self.zCutDepth)

class OpInit(opResult):
    """
    Operation d'initialisation.
    """
    def emit(self):
        Gcode_Generator.init()

class OpDone(opResult):
    """
    Operation de finalisation.
    """
    def emit(self):
        Gcode_Generator.done()

# fonctions
def makeOpDict():
    """
    Retourne un dictionnaire liant les noms d'operations aux classes correspondantes.
    """
    return {
        "opComment": opComment,
        "g0": g0Result,
        "g1": g1Result,
        "path": pathResult,
        "init": OpInit,
        "done": OpDone
    }

def followPath(SVG, path, zRapid = False, zCutDepth = True):
    """
    Crée une operation de suivi de path.
    """
    return pathResult(SVG, path, zRapid, zCutDepth)

def removeIsland(pathList, min_length=1.0):
    """
    Retire les petites îles (paths de longueur inférieure au seuil) du SVG.
    """
    filtered_paths = []
    for path in pathList:
        try:
            if path.length() >= min_length:
                filtered_paths.append(path)

        except Exception as e:
            # Si le path ne possède pas de méthode length(), on l'inclut quand même.
            filtered_paths.append(path)

    return filtered_paths

def createGcodeFile(filename, operations):
    """
    Crée un fichier text pour le G-code et le rempli à partir d'une liste d'operations.
    """
    with open(filename, 'w') as f:
        original_stdout = sys.stdout
        sys.stdout = f

        try:
            for op in operations:
                op.emit()

        finally:
            sys.stdout = original_stdout

def handlePath(svgFilename):
    """
    Traite le fichier SVG pour générer une séquence d'operations G-code.
    Retourne la liste des operations.
    """
    # Créer l'objet SVG à partir du fichier
    svgObj = Gcode_Generator.SVG(svgFilename)

    # Récupérer les paths du SVG
    paths = svgObj.SVGPaths
    
    operations = []
    operations.append(opComment("Start of G-code generation"))
    operations.append(opComment("Initialization"))
    operations.append(OpInit())
    
    # Pour chaque path, ajouter l'operation de suivi
    for path in paths:
        operations.append(followPath(svgObj, path))
    
    operations.append(opComment("End of G-code generation"))
    operations.append(OpDone())
    
    return operations

def main():
    parser = argparse.ArgumentParser(description="Convert SVG file to G-code commands.")
    parser.add_argument("svgfile", help="Input SVG file")
    parser.add_argument("output", help="Output G-code text file")
    args = parser.parse_args()
    
    # Traite le SVG pour générer la liste des operations
    operations = handlePath(args.svgfile)
    # Ecrit les opérations dans le fichier de sortie
    createGcodeFile(args.output, operations)
    print(f"G-code file created: {args.output}")

if __name__ == "__main__":
    main()