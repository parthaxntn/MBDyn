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

/* Atti di moto piani */

#ifndef PLANEDJ_H
#define PLANEDJ_H

#include "joint.h"
#include "drive.h"

/* PlaneDispJoint - begin */

class PlaneDispJoint : virtual public Elem, public Joint {
 private:
   /* 
    * Atto di moto piano - simile ad un inplane, definisce un piano
    * rispetto ad un nodo in cui e' consentito lo spostamento, in piu' 
    * vincola la rotazione attorno agli assi che non sono normali
    * al piano
    */
   const StructNode* pNode1;
   const StructNode* pNode2;
   Vec3 d1;
   Mat3x3 R1h;
   Vec3 d2;
   Mat3x3 R2h;
   
   Vec3 F;
   Vec3 M;
   
 public:
   /* Costruttore non banale */
   PlaneDispJoint(unsigned int uL, const DofOwner* pDO,
		  const StructNode* pN1, const StructNode* pN2,
		  const Vec3& dTmp1, const Vec3& dTmp2,
		  const Mat3x3& R1hTmp, const Mat3x3& R2hTmp, flag fOut);
   
   /* Distruttore */
   ~PlaneDispJoint(void);

   virtual inline void* pGet(void) const { 
      return (void*)this;
   };

   /* Contributo al file di restart */
   virtual std::ostream& Restart(std::ostream& out) const;

   /* Tipo di Joint */
   virtual Joint::Type GetJointType(void) const { 
      return Joint::PLANEDISP;
   };
   
   virtual unsigned int iGetNumDof(void) const { 
      return 3;
   };
   
   DofOrder::Order SetDof(unsigned int i) const {
      ASSERT(i >= 0 && i < 3);
      return DofOrder::ALGEBRAIC; 
   };

   void WorkSpaceDim(integer* piNumRows, integer* piNumCols) const { 
      *piNumRows = 15;
      *piNumCols = 15; 
   };
   
      
   VariableSubMatrixHandler& AssJac(VariableSubMatrixHandler& WorkMat,
				    doublereal dCoef,
				    const VectorHandler& XCurr, 
				    const VectorHandler& XPrimeCurr);
   SubVectorHandler& AssRes(SubVectorHandler& WorkVec,
			    doublereal dCoef,
			    const VectorHandler& XCurr, 
			    const VectorHandler& XPrimeCurr);
   
   void Output(OutputHandler& OH) const;
 

   /* funzioni usate nell'assemblaggio iniziale */
   
   virtual unsigned int iGetInitialNumDof(void) const { 
      return 6;
   };
   virtual void InitialWorkSpaceDim(integer* piNumRows,
				    integer* piNumCols) const {
      *piNumRows = 30; 
      *piNumCols = 30;
   };
   
   /* Contributo allo jacobiano durante l'assemblaggio iniziale */
   VariableSubMatrixHandler& InitialAssJac(VariableSubMatrixHandler& WorkMat,
					   const VectorHandler& XCurr);
   
   /* Contributo al residuo durante l'assemblaggio iniziale */   
   SubVectorHandler& InitialAssRes(SubVectorHandler& WorkVec,
				   const VectorHandler& XCurr);
   
   /* Dati privati */
   virtual unsigned int iGetNumPrivData(void) const;
   virtual unsigned int iGetPrivDataIdx(const char *s) const;
   virtual doublereal dGetPrivData(unsigned int i = 0) const; 

    /* *******PER IL SOLUTORE PARALLELO******** */        
   /* Fornisce il tipo e la label dei nodi che sono connessi all'elemento
      utile per l'assemblaggio della matrice di connessione fra i dofs */
   virtual void GetConnectedNodes(int& NumNodes, Node::Type* NdTyps, unsigned int* NdLabels) {
     NumNodes = 2;
     NdTyps[0] = pNode1->GetNodeType();
     NdLabels[0] = pNode1->GetLabel();
     NdTyps[1] = pNode2->GetNodeType();
     NdLabels[1] = pNode2->GetLabel();
   };
   /* ************************************************ */
};

/* PlaneDispJoint - end */


/* PlaneDispPinJoint - begin */

/* Incastro con liberta' di rotazione su un asse */

class PlaneDispPinJoint : virtual public Elem, public Joint {
 private:
   const StructNode* pNode;
   Vec3 X0;
   Mat3x3 R0;
   Vec3 d;
   Mat3x3 Rh;
   Vec3 F;
   Vec3 M;
   
 public:
   /* Costruttore non banale */
   PlaneDispPinJoint(unsigned int uL, const DofOwner* pDO,
		     const StructNode* pN,
		     const Vec3& X0Tmp, const Mat3x3& R0Tmp, 
		     const Vec3& dTmp, const Mat3x3& RhTmp, flag fOut);
   
   ~PlaneDispPinJoint(void);

   virtual inline void* pGet(void) const { 
      return (void*)this;
   };
   
   /* Tipo di Joint */
   virtual Joint::Type GetJointType(void) const { 
      return Joint::PLANEDISPPIN; 
   };

   /* Contributo al file di restart */
   virtual std::ostream& Restart(std::ostream& out) const;

   virtual unsigned int iGetNumDof(void) const { 
      return 3;
   };
   
   virtual DofOrder::Order SetDof(unsigned int i) const {
      ASSERT(i >= 0 && i < 3);
      return DofOrder::ALGEBRAIC;
   };

   virtual void WorkSpaceDim(integer* piNumRows, integer* piNumCols) const { 
      *piNumRows = 9; 
      *piNumCols = 9;
   };
   
   VariableSubMatrixHandler& AssJac(VariableSubMatrixHandler& WorkMat,
				    doublereal dCoef,
				    const VectorHandler& XCurr, 
				    const VectorHandler& XPrimeCurr);
   SubVectorHandler& AssRes(SubVectorHandler& WorkVec,
			    doublereal dCoef,
			    const VectorHandler& XCurr, 
			    const VectorHandler& XPrimeCurr);
   
   virtual void Output(OutputHandler& OH) const;
 
   
   /* funzioni usate nell'assemblaggio iniziale */
   
   virtual unsigned int iGetInitialNumDof(void) const { 
      return 6;
   };
   virtual void InitialWorkSpaceDim(integer* piNumRows,
				    integer* piNumCols) const { 
      *piNumRows = 18; 
      *piNumCols = 18; 
   };
   
   /* Contributo allo jacobiano durante l'assemblaggio iniziale */
   VariableSubMatrixHandler& InitialAssJac(VariableSubMatrixHandler& WorkMat,
					   const VectorHandler& XCurr);
   
   /* Contributo al residuo durante l'assemblaggio iniziale */   
   SubVectorHandler& InitialAssRes(SubVectorHandler& WorkVec,
				   const VectorHandler& XCurr);

  /* *******PER IL SOLUTORE PARALLELO******** */        
   /* Fornisce il tipo e la label dei nodi che sono connessi all'elemento
      utile per l'assemblaggio della matrice di connessione fra i dofs */
   virtual void GetConnectedNodes(int& NumNodes, Node::Type* NdTyps, unsigned int* NdLabels) {
     NumNodes = 1;
     NdTyps[0] = pNode->GetNodeType();
     NdLabels[0] = pNode->GetLabel();
   };
   /* ************************************************ */
};

/* PlaneDispPinJoint - end */

#endif
