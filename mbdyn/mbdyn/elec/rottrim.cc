/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2000
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#ifdef USE_ELECTRIC_NODES
#ifdef USE_AERODYNAMIC_ELEMS

#include <genel.h>
#include <drive.h>
#include <constltp.h>
#include <strnode.h>
#include <rottrim.h>

RotorTrim::RotorTrim(unsigned int uL,
		     const DofOwner* pDO,
		     Rotor* pRot, 
		     ScalarDifferentialNode* pNode1,
		     ScalarDifferentialNode* pNode2,
		     ScalarDifferentialNode* pNode3,
		     DriveCaller* pDrive1,
		     DriveCaller* pDrive2,
		     DriveCaller* pDrive3,
		     const doublereal& dS,	    
		     const doublereal& dG,
		     const doublereal& dp,
		     const doublereal& dT0,
		     const doublereal& dT1,
		     const doublereal& dK0,
		     const doublereal& dK1,
		     flag fOut)
: Elem(uL, Elem::GENEL, fOut),
Genel(uL, Genel::ROTORTRIM, pDO, fOut),
pRotor(pRot),
dSigma(dS),
dCpAlpha(2*M_PI),
dGamma(dG),
dP(dp),
dP2(dP*dP),
dC(8.*(dP*dP-1.)/dGamma),
dC2(dC*dC),
dTau0(dT0),
dTau1(dT1),
dKappa0(dK0),
dKappa1(dK1)
{
	ASSERT(pRotor != NULL);
	ASSERT(pNode1 != NULL);
	ASSERT(pNode2 != NULL);
	ASSERT(pNode3 != NULL);
	ASSERT(dSigma > 0.);
	ASSERT(dCpAlpha > 0.);
	ASSERT(dGamma > 0.);
	ASSERT(dP > 0.);
	
	pvNodes[0] = pNode1;
	pvNodes[1] = pNode2;
	pvNodes[2] = pNode3;
	pvDrives[0].Set(pDrive1);
	pvDrives[1].Set(pDrive2);
	pvDrives[2].Set(pDrive3);
}

RotorTrim::~RotorTrim(void)
{
	NO_OP;
}

unsigned int
RotorTrim::iGetNumDof(void) const
{
 	return 0;
}

/* Scrive il contributo dell'elemento al file di restart */
ostream&
RotorTrim::Restart(ostream& out) const
{
	return out << " /* rotor trim not implemented yet */ ";
}

/* Dimensioni del workspace */
void
RotorTrim::WorkSpaceDim(integer* piNumRows, integer* piNumCols) const
{
	*piNumRows = 3;
	*piNumCols = 3;
}

void
RotorTrim::Output(OutputHandler& /* OH */ ) const
{
	NO_OP;
}

/* assemblaggio jacobiano */
VariableSubMatrixHandler& 
RotorTrim::AssJac(VariableSubMatrixHandler& WorkMat,
		  doublereal dCoef,
		  const VectorHandler& /* XCurr */ ,
		  const VectorHandler& /* XPrimeCurr */ )
{
	DEBUGCOUT("Entering RotorTrim::AssJac()" << endl);
	
	SparseSubMatrixHandler& WM = WorkMat.SetSparse();        
	WM.Resize(3, 0);
        
        integer iRowIndex = 0;
        integer iColIndex = 0;
	
	iRowIndex = pvNodes[0]->iGetFirstRowIndex()+1;
	iColIndex = pvNodes[0]->iGetFirstColIndex()+1;
        WM.fPutItem(1, iRowIndex, iColIndex, dTau0+dCoef);      

 	iRowIndex = pvNodes[1]->iGetFirstRowIndex()+1;
	iColIndex = pvNodes[1]->iGetFirstColIndex()+1;
        WM.fPutItem(2, iRowIndex, iColIndex, dTau1+dCoef);      

 	iRowIndex = pvNodes[2]->iGetFirstRowIndex()+1;
	iColIndex = pvNodes[2]->iGetFirstColIndex()+1;
        WM.fPutItem(3, iRowIndex, iColIndex, dTau1+dCoef);      

        return WorkMat;
}

/* assemblaggio residuo */
SubVectorHandler&
RotorTrim::AssRes(SubVectorHandler& WorkVec,
		  doublereal dCoef,
		  const VectorHandler& /* XCurr */ ,
		  const VectorHandler& /* XPrimeCurr */ )
{
	DEBUGCOUT("Entering RotorTrim::AssRes()" << endl);
	
	WorkVec.Resize(3);
	WorkVec.Reset(0.);
	
	WorkVec.fPutRowIndex(1, pvNodes[0]->iGetFirstRowIndex()+1);
	WorkVec.fPutRowIndex(2, pvNodes[1]->iGetFirstRowIndex()+1);
	WorkVec.fPutRowIndex(3, pvNodes[2]->iGetFirstRowIndex()+1);
	
	doublereal dX1 = pvNodes[0]->dGetX();
	doublereal dX2 = pvNodes[1]->dGetX();
	doublereal dX3 = pvNodes[2]->dGetX();
	doublereal dX1Prime = pvNodes[0]->dGetXPrime();
	doublereal dX2Prime = pvNodes[1]->dGetXPrime();
	doublereal dX3Prime = pvNodes[2]->dGetXPrime();
	
	doublereal dRho = pRotor->dGetAirDensity(pRotor->GetXCurr());
	doublereal dOmega = pRotor->dGetOmega();
	doublereal dRadius = pRotor->dGetRadius();
	if (fabs(dOmega) < 1.e-6) {
		dOmega = 1.e-6;
	}
	doublereal dMu = pRotor->dGetMu();
	doublereal dMu2 = dMu*dMu;
	
	doublereal d = 
		M_PI*pow(dRadius, 4)*dRho*dOmega*dOmega*(dSigma*dCpAlpha);
	doublereal dTraction = pRotor->GetForces().dGet(3)/d;
	doublereal dRollMoment = pRotor->GetMoments().dGet(1)/(d *= dRadius);
	doublereal dPitchMoment = pRotor->GetMoments().dGet(2)/d;
	
	doublereal f = dC/(1+dC2);
	
	Mat3x3 m((1.+1.5*dMu2)/6.,
		-f*dMu/6.*(dC-dGamma/(16.*dP2)),
		f*dMu/6.*(1.+dC*dGamma/(16.*dP2)),
		2./9.*dMu,
		-f/16.*(dC*(1.+1.5*dMu2)-2./9.*dMu2*dGamma/dP2),
		f/16.*((1.+2.*dMu2)+2./9.*dMu2*dC*dGamma/dP2),
		0.,
		-f/16.,
		-f/16.*(1.+.5*dMu2));
	Vec3 v(pvDrives[0].dGet()-dTraction,
		pvDrives[1].dGet()-dRollMoment,
		pvDrives[2].dGet()-dPitchMoment);
	v = m.Inv(v);
	
	WorkVec.fPutCoef(1, v.dGet(1)*dKappa0-dX1-dTau0*dX1Prime);
	WorkVec.fPutCoef(2, v.dGet(2)*dKappa1-dX2-dTau1*dX2Prime);
	WorkVec.fPutCoef(3, v.dGet(3)*dKappa1-dX3-dTau1*dX3Prime);
	
	return WorkVec;
}

void
RotorTrim::SetValue(VectorHandler& /* X */ , VectorHandler& /* XP */ ) const
{
	NO_OP;
}

#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_ELECTRIC_NODES */

