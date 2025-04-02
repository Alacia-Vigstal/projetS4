#include <iostream>
#include <cmath>
#include <string>
#include <fstream>

#define PI 3.141592653589793

//traiter la ligne de G-code
void traitergcode(const std:: string ligne, double xprecedent, double yprecedent)
{
    if (ligne.rfind("G00", 0) == 0 || ligne.rfind("G01", 0) == 0 || ligne.rfind("G02", 0) == 0 ) 
    {
        double x=xprecedent, y=yprecedent;

        //Extraire les coordonnées
        size_t indexx=ligne.find('X');
        if (indexx != std :: string  :: npos) 
        {
            x = std :: stod(ligne.substr(indexx + 1));
        }

        size_t indexy=ligne.find('Y');
        if (indexy != std :: string  :: npos) 
        {
            y = std :: stod(ligne.substr(indexy + 1));
        }

        //Calcul de l'angle
        double angle = atan2(y-yprecedent, x-xprecedent)*(180/PI);
        std::cout << "Angle d'orientation : " << angle << " degrés" << std::endl;

        //mettre à jour les coordonnées précédentes
        xprecedent = x;
        yprecedent = y;
    }
}

int main ()
{
    double x1, y1, x2, y2;

    //Ouvrir le fichier G-code 
    std :: ifstream fichiergcode("C:\\Users\\HAKUN\\OneDrive\\Documents\\Génie robotique cohorte 69\\Session 4\\Projet\\Test_GCode.txt");

    if(!fichiergcode.is_open())
    {
        printf("Impossible d'ouvrir le fichier G-Code");
        return 1;
    }
    double xprecedent = 0, yprecedent = 0;

    std :: string ligne;
    while (std :: getline(fichiergcode, ligne))
    {
        traitergcode(ligne, xprecedent, yprecedent);
    }
    fichiergcode.close();
}