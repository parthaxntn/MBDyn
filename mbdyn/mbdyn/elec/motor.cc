/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2003
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
 * the Free Software Foundation (version 2 of the License).
 * 
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

#include <elec.h>
#include <strnode.h>
#include <elecnode.h>
#include <motor.h>

/*
 * Electric motor: an internal couple between two structural nodes
 * whose value is represented by an internal state that is a current,
 * according to equations:

	C1 = - Gain * i
	
	C2 = Gain * i

	i1 = - i

	i2 = i
	
	    d i
	L * --- + R * i = - Gain * (Omega2 - Omega1) + V2 - V1
	    d t
 
 */

Motor::Motor(unsigned int uL, const DofOwner* pD, 
		const StructNode* pN1, const StructNode* pN2,
		const ElectricNode* pV1, const ElectricNode* pV2,
		const Vec3& TmpDir, doublereal dG,
		doublereal dl, doublereal dr,
		flag fOut)
: Elem(uL, Elem::ELECTRIC, fOut), 
Electric(uL, Electric::MOTOR, pD, fOut),
pStrNode1(pN1), pStrNode2(pN2), pVoltage1(pV1), pVoltage2(pV2),
Dir(TmpDir), dGain(dG), dL(dl), dR(dr)
{
	NO_OP;
}

Motor::~Motor(void)
{
	NO_OP;
}

/* Contributo al file di restart */
std::ostream&
Motor::Restart(std::ostream& out) const
{
	return out << "electric: " << GetLabel()
		<< ", motor, "
		<< pStrNode1->GetLabel() << ", ",
		Dir.Write(out, ", ") << ", "
		<< pStrNode2->GetLabel() << ", "
		<< pVoltage1->GetLabel() << ", "
		<< pVoltage2->GetLabel() << ", "
		<< dGain << ", "
		<< dL << ", "
		<< dR << ";" << std::endl;
}
   
unsigned int
Motor::iGetNumDof(void) const
{
	return 1;
}

DofOrder::Order
Motor::SetDof(unsigned int i) const
{
	return DofOrder::DIFFERENTIAL;
}

void
Motor::WorkSpaceDim(integer* piNumRows, integer* piNumCols) const
{
	*piNumRows = 9;
	*piNumCols = 9;
}
      
VariableSubMatrixHandler&
Motor::AssJac(VariableSubMatrixHandler& WorkMat,
		doublereal dCoef,
		const VectorHandler& XCurr, 
		const VectorHandler& XPrimeCurr)
{
	FullSubMatrixHandler& WM = WorkMat.SetFull();

	/* Dimensiona e resetta la matrice di lavoro */
	integer iNumRows = 0;
	integer iNumCols = 0;
	WorkSpaceDim(&iNumRows, &iNumCols);
	WM.ResizeInit(iNumRows, iNumCols, 0.);

	integer iStrNode1FirstPosIdx = pStrNode1->iGetFirstPositionIndex() + 3;
	integer iStrNode2FirstPosIdx = pStrNode2->iGetFirstPositionIndex() + 3;
	integer iStrNode1FirstMomIdx = pStrNode1->iGetFirstMomentumIndex() + 3;
	integer iStrNode2FirstMomIdx = pStrNode2->iGetFirstMomentumIndex() + 3;
	integer iElecNode1FirstIndex = pVoltage1->iGetFirstRowIndex() + 1;
	integer iElecNode2FirstIndex = pVoltage2->iGetFirstRowIndex() + 1;
	integer iFirstIndex = iGetFirstIndex() + 1;

	for (integer iCnt = 1; iCnt <= 3; iCnt++) {
		WM.fPutRowIndex(iCnt, iStrNode1FirstMomIdx + iCnt);
		WM.fPutRowIndex(3+iCnt, iStrNode2FirstMomIdx + iCnt);

		WM.fPutColIndex(iCnt, iStrNode1FirstPosIdx + iCnt);
		WM.fPutColIndex(3+iCnt, iStrNode2FirstPosIdx + iCnt);
	}

	WM.fPutRowIndex(7, iElecNode1FirstIndex);
	WM.fPutRowIndex(8, iElecNode2FirstIndex);
	WM.fPutRowIndex(9, iFirstIndex);

	WM.fPutColIndex(7, iElecNode1FirstIndex);
	WM.fPutColIndex(8, iElecNode2FirstIndex);
	WM.fPutColIndex(9, iFirstIndex);

	Vec3 TmpDir(pStrNode1->GetRRef()*Dir);
	Vec3 Cdi(TmpDir*(dGain*dCoef));
	doublereal i = XCurr.dGetCoef(iFirstIndex);
	Vec3 C(Cdi*i);
	Vec3 Tmp((TmpDir + pStrNode2->GetWRef().Cross(TmpDir*dCoef))*dGain);

	WM.Sub(1, 1, Mat3x3(C));
	WM.Add(4, 1, Mat3x3(C));

	for (unsigned int iCnt = 1; iCnt <= 3; iCnt++) {
		doublereal d = Cdi.dGet(iCnt);
		WM.fIncCoef(iCnt, 9, d);
		WM.fDecCoef(3 + iCnt, 9, d);

		d = Tmp.dGet(iCnt);
		WM.fIncCoef(9, iCnt, d);
		WM.fDecCoef(9, 3 + iCnt, d);
	}

	WM.fIncCoef(7, 9, dCoef);
	WM.fDecCoef(8, 9, dCoef);

	WM.fIncCoef(9, 7, dCoef);
	WM.fDecCoef(9, 8, dCoef);

	WM.fIncCoef(9, 9, dL + dCoef*dR);

	return WorkMat;
}

SubVectorHandler&
Motor::AssRes(SubVectorHandler& WorkVec,
		doublereal dCoef,
		const VectorHandler& XCurr, 
		const VectorHandler& XPrimeCurr)
{
	/* Dimensiona e resetta la matrice di lavoro */
	integer iNumRows = 0;
	integer iNumCols = 0;
	WorkSpaceDim(&iNumRows, &iNumCols);
	WorkVec.Resize(iNumRows);
	WorkVec.Reset(0.);

	integer iStrNode1FirstIndex = pStrNode1->iGetFirstMomentumIndex() + 3;
	integer iStrNode2FirstIndex = pStrNode2->iGetFirstMomentumIndex() + 3;
	integer iElecNode1FirstIndex = pVoltage1->iGetFirstRowIndex() + 1;
	integer iElecNode2FirstIndex = pVoltage2->iGetFirstRowIndex() + 1;
	integer iFirstIndex = iGetFirstIndex() + 1;

	for (integer iCnt = 1; iCnt <= 3; iCnt++) {
		WorkVec.fPutRowIndex(iCnt, iStrNode1FirstIndex + iCnt);
		WorkVec.fPutRowIndex(3+iCnt, iStrNode2FirstIndex + iCnt);
	}

	WorkVec.fPutRowIndex(7, iElecNode1FirstIndex);
	WorkVec.fPutRowIndex(8, iElecNode2FirstIndex);
	WorkVec.fPutRowIndex(9, iFirstIndex);

	doublereal i = XCurr.dGetCoef(iFirstIndex);
	doublereal iP = XPrimeCurr.dGetCoef(iFirstIndex);
	doublereal dV = pVoltage2->dGetX() - pVoltage1->dGetX();

	Vec3 TmpDir(pStrNode1->GetRCurr()*Dir);
	Vec3 C(TmpDir*(dGain*i));
	doublereal omega =
		TmpDir*(pStrNode2->GetWCurr() - pStrNode1->GetWCurr());

	WorkVec.Sub(1, C);
	WorkVec.Add(4, C);
	WorkVec.fDecCoef(7, i);
	WorkVec.fIncCoef(8, i);
	WorkVec.fIncCoef(9, dV - dGain*omega - dL*iP - dR*i);
      
	return WorkVec;
}

void
Motor::SetInitialValue(VectorHandler& /* X */ ) const
{
	NO_OP;
}

void
Motor::SetValue(VectorHandler& X, VectorHandler& /* XP */ ) const
{
	NO_OP;
}

