/*

MBDyn (C) is a multibody analysis code. 
http://www.mbdyn.org

Copyright (C) 1996-2006

Pierangelo Masarati	<masarati@aero.polimi.it>
Paolo Mantegazza	<mantegazza@aero.polimi.it>

Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
via La Masa, 34 - 20156 Milano, Italy
http://www.aero.polimi.it

Changing this copyright notice is forbidden.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


------------------------------------------------------------------------------

ADAMS2MBDyn (C) is a translator from ADAMS/View models in adm format
into raw MBDyn input files.

Copyright (C) 1999-2006
Leonardo Cassan		<lcassan@tiscalinet.it>

*/

#ifndef MATHEM_H
#define MATHEM_H

#include <defs.h>
#include <matrix.h>
#include <math.h>
#include <float.h>

/* Usage: Gauss (Vector& solution, Matrix A orig, vector Yor) */

Mat3x3 MomentToInertia(Vec3, Vec3);

/* Parametri di rotazione di Eulero a partire dalla matrice R */
Vec3 gparam (const Mat3x3&);

double square(double);
double a360tan2 (double,double);
void Gauss (Vector&, Matrix&, Vector&);
Matrix Inv (Matrix&);
Matrix operator / (Matrix&, Matrix&);
Mat6x6 KMatrix (double, double, double, double,
		 double, double, double, double, double,
		 double*, double);
Vec3 EulerAngles (const Mat3x3&); 
Mat3x3 RFromEulerAngles (const Vec3&);
Mat3x3 MatR2vec (unsigned short int, const Vec3&,
		 unsigned short int, const Vec3&);

#endif
