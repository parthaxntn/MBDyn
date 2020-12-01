/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2017
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

 /*
  * With the contribution of Runsen Zhang <runsen.zhang@polimi.it>
  * during Google Summer of Code 2020
  */


#include "mbconfig.h"           /* This goes first in every *.c,*.cc file */

#include <iostream>
#include <cfloat>
#include <vector>

#include "elem.h"
#include "strnode.h"
#include "dataman.h"
#include "userelem.h"

#include "module-chrono-interface.h"
#include "mbdyn_ce.h"

ChronoInterfaceBaseElem::ChronoInterfaceBaseElem(
	unsigned uLabel, const DofOwner *pDO,
	DataManager* pDM, MBDynParser& HP)
: Elem(uLabel, flag(0)),
UserDefinedElem(uLabel, pDO),
m_pDM(pDM),
MBDyn_CE_CEModel_Converged(pDM),
bMBDyn_CE_FirstSend(true),
bMBDyn_CE_CEModel_DoStepDynamics(true)
{
	// Read element: obtain information from MBDyn script
	// help
	if (HP.IsKeyWord("help")) {
		silent_cout(
"									\n"
"Module: 	module-chrono-interface						\n"
"Organization:	Dipartimento di Ingegneria Aerospaziale			\n"
"		Politecnico di Milano					\n"
"		http://www.aero.polimi.it/				\n"
"									\n"
"	All rights reserved						\n"
"user defined, ID, ... \n"
			<< std::endl);

		if (!HP.IsArg()) {
			/*
			 * Exit quietly if nothing else is provided
			 */
			throw NoErr(MBDYN_EXCEPT_ARGS);
		}
	}
	
	/* read information from script - start*/


	//--------------- read the coupling type
    MBDyn_CE_CouplingIter_Count=0;
	MBDyn_CE_Coupling_Tol = 1.0e-3; // by default, tolerance == 1.0e-6;
	MBDyn_CE_CouplingType = MBDyn_CE_COUPLING::COUPLING_NONE;
	if (HP.IsKeyWord("coupling")){
		if (HP.IsKeyWord("none"))
		{
			MBDyn_CE_CouplingType = MBDyn_CE_COUPLING::COUPLING_NONE;
			MBDyn_CE_CouplingIter_Max = 0;
		}
		else if (HP.IsKeyWord("loose"))
		{
			MBDyn_CE_CouplingType = MBDyn_CE_COUPLING::COUPLING_LOOSE;
			// By default, using embedded scheme.
			MBDyn_CE_CouplingType_loose = MBDyn_CE_COUPLING_LOOSE::LOOSE_EMBEDDED;
			MBDyn_CE_CouplingIter_Max = 1;
			std::cout << "Embedded-loose method is chosen" << std::endl;
			if (HP.IsKeyWord("embedded"))
			{
				MBDyn_CE_CouplingType_loose = MBDyn_CE_COUPLING_LOOSE::LOOSE_EMBEDDED;
			}
			else if (HP.IsKeyWord("jacobian"))
			{
				MBDyn_CE_CouplingType_loose = MBDyn_CE_COUPLING_LOOSE::LOOSE_JACOBIAN;
			}
			else if (HP.IsKeyWord("gauss"))
			{
				MBDyn_CE_CouplingType_loose = MBDyn_CE_COUPLING_LOOSE::LOOSE_GAUSS;
			}
		}
		else if (HP.IsKeyWord("tight"))
		{
			MBDyn_CE_CouplingType = MBDyn_CE_COUPLING::COUPLING_TIGHT;
			MBDyn_CE_CouplingType_loose = MBDyn_CE_COUPLING_LOOSE::TIGHT; // meaningless. 
			MBDyn_CE_CouplingIter_Max = HP.GetInt();
			if (HP.IsKeyWord("tolerance"))
			{
				MBDyn_CE_Coupling_Tol = HP.GetReal();
			}
			else 
			{
				MBDyn_CE_Coupling_Tol = 1.0e-3; //pDM -> GetSolver()->pGetStepIntegrator()->GetIntegratorDTol();
			}
		}
		else{
			MBDyn_CE_CouplingType = HP.GetInt();
			if (MBDyn_CE_CouplingType > 1)
			{
				MBDyn_CE_CouplingIter_Max = HP.GetInt();
				std::cout << "multirate coupling C::E interface: not implemented" << std::endl;
				throw ErrGeneric(MBDYN_EXCEPT_ARGS);
			}
		}
	}


	//--------------- read the motor type	
	MBDyn_CE_CEMotorType = MBDyn_CE_CEMOTORTYPE::VELOCITY; // by default, using the position motor 
	if (HP.IsKeyWord("motor" "type"))
	{
		if (HP.IsKeyWord("vel"))
		{
			MBDyn_CE_CEMotorType = MBDyn_CE_CEMOTORTYPE::VELOCITY; // velocity is imposed to objects in Chrono::Engine
		}
		else if (HP.IsKeyWord("pos"))
		{
			MBDyn_CE_CEMotorType = MBDyn_CE_CEMOTORTYPE::POSITION; // position is imposed to objects in Chrono::Engine
		}
		else
		{
			silent_cerr("Motor type at line " << HP.GetLineData() << " is not implemeted yet \n");
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}	
	}


	//--------------- read the coupling data
	//---------- default coupling data
	//----- scale
	MBDyn_CE_CEScale[0] = 1.0; //- default value for length scale
	MBDyn_CE_CEScale[1] = 1.0; //- default value for mass scale
	MBDyn_CE_CEScale[2] = 1.0; //- default value for force scaleforce scale
	MBDyn_CE_CEScale[3] = 1.0; //- default value for torque scalemoment scale
	//----- nodes number
	MBDyn_CE_NodesNum = 0;
	//----- C::E ground reference
	double mbdyn_ce_ref_x[3] = {0.0, 0.0, 0.0}; 
	double mbdyn_ce_ref_R[9];
	for (unsigned i = 0; i < 3; i++) //- default: two system has the same coordinate
	{
		mbdyn_ce_ref_x[i] = MBDyn_CE_CEScale[0] * mbdyn_ce_ref_x[i];
	}
	for (unsigned i = 0; i < 9; i++) 
	{
		mbdyn_ce_ref_R[i] = 0.0;
	}
	mbdyn_ce_ref_R[0] = 1.0;
	mbdyn_ce_ref_R[4] = 1.0;
	mbdyn_ce_ref_R[8] = 1.0;

	//---------- values obtained from scripts
	if (MBDyn_CE_CouplingType >= -1) //- if coupled.
	{
		//----- read the scale: units relationship between MBDyn and C::E;
		if (HP.IsKeyWord("length" "scale"))
		{
			MBDyn_CE_CEScale[0] = HP.GetReal();		   //- legnth scale
			MBDyn_CE_CEScale[1] = MBDyn_CE_CEScale[0]; //- if not mentioned, mass scale = length scale;
		}
		if (HP.IsKeyWord("mass" "scale"))
		{
			MBDyn_CE_CEScale[1] = HP.GetReal(); //- mass scale
		}
		MBDyn_CE_CEScale[2] = MBDyn_CE_CEScale[1] * MBDyn_CE_CEScale[0]; //- force scale
		MBDyn_CE_CEScale[3] = MBDyn_CE_CEScale[2] * MBDyn_CE_CEScale[0]; //- moment scale

		//----- read the numerb of coupling nodes
		if (HP.IsKeyWord("nodes" "number"))
		{
			MBDyn_CE_NodesNum = HP.GetInt();
		}	
		if (MBDyn_CE_NodesNum <= 0) //- invalid number
		{
			silent_cerr("ChronoInterface(" << uLabel << "): missing node number or invalid node number " << MBDyn_CE_NodesNum <<
				" at line " << HP.GetLineData() << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		//----- preparing for reading information of coupling nodes
		MBDyn_CE_Nodes.resize(MBDyn_CE_NodesNum);
		MBDyn_CE_CEModel_Label.resize(MBDyn_CE_NodesNum + 1);					   //- node number + 1 (C::E Ground);
		MBDyn_CE_CEModel_Label[MBDyn_CE_NodesNum].MBDyn_CE_CEBody_Label = 0;	   //- ID of C::E ground in C::E model =0;
		MBDyn_CE_CEModel_Label[MBDyn_CE_NodesNum].MBDyn_CE_CEMotor_Label = 0;	   //- No motion is imposed to Ground;
		MBDyn_CE_CEModel_Label[MBDyn_CE_NodesNum].bMBDyn_CE_CEBody_Output = false; //- Output: no.
		bMBDyn_CE_Output = false;
		//----- reading information of coupling nodes
		for (int i = 0; i < MBDyn_CE_NodesNum; i++)
		{
			MBDyn_CE_Nodes[i].pMBDyn_CE_Node = pDM->ReadNode<const StructNode, Node::STRUCTURAL>(HP); //- pointer to nodes in MBDyn model;
			const DynamicStructNode *temp_pMBDyn_CE_Node = dynamic_cast<const DynamicStructNode *>(MBDyn_CE_Nodes[i].pMBDyn_CE_Node);
			if(temp_pMBDyn_CE_Node==NULL) //- not a dynamic structural node
			{
				silent_cerr("The node" << MBDyn_CE_Nodes[i].pMBDyn_CE_Node->GetLabel()<< " at line " << HP.GetLineData() << "is not a dynamic structual node"<<std::endl);
				throw ErrGeneric(MBDYN_EXCEPT_ARGS);
			}
			const_cast<DynamicStructNode *> (temp_pMBDyn_CE_Node)->ComputeAccelerations(true); //- also output acceleration information
			ReferenceFrame pNode_RF(MBDyn_CE_Nodes[i].pMBDyn_CE_Node); //- get node reference
			if (HP.IsKeyWord("offset"))
			{
				MBDyn_CE_Nodes[i].MBDyn_CE_Offset = HP.GetPosRel(pNode_RF); //- return offset in node reference
			}
			else 
			{
				MBDyn_CE_Nodes[i].MBDyn_CE_Offset= Zero3;
			}
			MBDyn_CE_Nodes[i].MBDyn_CE_F = Zero3;
			MBDyn_CE_Nodes[i].MBDyn_CE_M = Zero3;
			MBDyn_CE_Nodes[i].MBDyn_CE_uLabel = MBDyn_CE_Nodes[i].pMBDyn_CE_Node->GetLabel(); //- node label (MBDyn). 

			//----- get coupling bodies in the C::E model, bodies' ID;
			MBDyn_CE_Nodes[i].MBDyn_CE_CEBody_Label = HP.GetInt();									   //- bodies' ID, recorded in MBDyn;
			MBDyn_CE_CEModel_Label[i].MBDyn_CE_CEBody_Label = MBDyn_CE_Nodes[i].MBDyn_CE_CEBody_Label; //- bodies' ID, used in C::E;
			MBDyn_CE_CEModel_Label[i].bMBDyn_CE_CEBody_Output = false;								   //- by default: not output;
			if (HP.IsKeyWord("output"))
			{				
				if (HP.IsKeyWord("yes"))
					MBDyn_CE_CEModel_Label[i].bMBDyn_CE_CEBody_Output = true;
				else if (HP.IsKeyWord("no"))
				{
					// no_op
				}
				else
				{
					silent_cerr("ChronoInterface(" << uLabel << "): lacks key word at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric(MBDYN_EXCEPT_ARGS);
				}
			}		
			bMBDyn_CE_Output = (bMBDyn_CE_Output || MBDyn_CE_CEModel_Label[i].bMBDyn_CE_CEBody_Output);
		}
		//----- read C::E ground information (the ground frame in C::E is fixed to the ground frame in MBDyn).
		if (HP.IsKeyWord("ground"))
		{
			MBDyn_CE_CEModel_Label[MBDyn_CE_NodesNum].MBDyn_CE_CEBody_Label = HP.GetInt(); //- the ID of the ground body in C::E
			Vec3 mbdyn_ce_ref_x_Vec3;
			Mat3x3 mbdyn_ce_ref_R_Mat3x3;
			ReferenceFrame mbdynce_ce_ref(MBDyn_CE_Nodes[0].pMBDyn_CE_Node); //- ground ref (keyword 'Node' and 'Local' denotes the ref of first node)
			if (HP.IsKeyWord("position"))
			{
				mbdyn_ce_ref_x_Vec3 = HP.GetPosAbs(mbdynce_ce_ref); //- get C::E ground position information, with respect to the ref GLOBAL
			}
			if (HP.IsKeyWord("orientation"))
			{
				mbdyn_ce_ref_R_Mat3x3 = HP.GetMatAbs(mbdynce_ce_ref);//- get C::E ground orietation information
			}
			MBDyn_CE_CEModel_Label[MBDyn_CE_NodesNum].bMBDyn_CE_CEBody_Output = false; // by default: not output
			if (HP.IsKeyWord("output"))
			{
				if (HP.IsKeyWord("yes"))
					MBDyn_CE_CEModel_Label[MBDyn_CE_NodesNum].bMBDyn_CE_CEBody_Output = true;
				else if (HP.IsKeyWord("no"))
				{
					// no_op
				}
				else
				{
					silent_cerr("ChronoInterface(" << uLabel << "): lacks key word at line " << HP.GetLineData() << std::endl);
					throw ErrGeneric(MBDYN_EXCEPT_ARGS);
				}
			}
			bMBDyn_CE_Output = (bMBDyn_CE_Output || MBDyn_CE_CEModel_Label[MBDyn_CE_NodesNum].bMBDyn_CE_CEBody_Output);
			//- save the ref Vec3 in double [] (can also use functions: void MBDyn_CE_Vec3D() and  void MBDyn_CE_Mat3x3D());
			for (unsigned i = 0; i < 3; i++)
			{
				mbdyn_ce_ref_x[i] = (mbdyn_ce_ref_x_Vec3.pGetVec())[i] * MBDyn_CE_CEScale[0];
			}
			for (unsigned i = 0; i < 9; i++)
			{
				//- matrix in mbdyn:
				//- [ m_11,  m_12,  m_13 ]
				//- [ m_21,  m_22,  m_23 ]
				//- [ m_31,  m_32,  m_33 ]
				//- R[0]=m_11, R[1]=m_21, R[2]=m_31, R[3]=m_12....
				mbdyn_ce_ref_R[i] = (mbdyn_ce_ref_R_Mat3x3.pGetMat())[i];
			}
		}
	}

	//---------- other information
	//----- printf information in screen ? 
	bMBDyn_CE_Verbose = false;
	if (HP.IsKeyWord("verbose"))
	{
		if(HP.IsKeyWord("yes"))
		{
			bMBDyn_CE_Verbose = true;
		}
		else if (HP.IsKeyWord("no"))
		{
			// do nothing
		}
	}
	/* read information from script - end*/
	
	/* initialization (std::vector<>) - start*/
	//---------- allocate space for coupling variables (std::vectors for the coupling variables, motion and force)
	//- kinematic motion + 12 (for the global coordinate in chrono)
	MBDyn_CE_CouplingSize.Size_Kinematic = MBDyn_CE_NodesNum * (3 + 9 + 3 + 3 + 3 + 3) + 12; //- motion
	MBDyn_CE_CouplingSize.Size_Dynamic = MBDyn_CE_NodesNum*(3 + 3); //- dynamic variables

	MBDyn_CE_CouplingKinematic.resize(MBDyn_CE_CouplingSize.Size_Kinematic, 0.0);
	MBDyn_CE_CouplingDynamic.resize(MBDyn_CE_CouplingSize.Size_Dynamic, 0.0);
	MBDyn_CE_CouplingDynamic_pre.resize(MBDyn_CE_CouplingSize.Size_Dynamic,0.0); //- vector for last iteration
	//- pointer to motion
	pMBDyn_CE_CouplingKinematic_x = &MBDyn_CE_CouplingKinematic[0];
	pMBDyn_CE_CouplingKinematic_R = &MBDyn_CE_CouplingKinematic[3*MBDyn_CE_NodesNum];
	pMBDyn_CE_CouplingKinematic_xp = &MBDyn_CE_CouplingKinematic[12*MBDyn_CE_NodesNum];
	pMBDyn_CE_CouplingKinematic_omega = &MBDyn_CE_CouplingKinematic[15*MBDyn_CE_NodesNum];
	pMBDyn_CE_CouplingKinematic_xpp = &MBDyn_CE_CouplingKinematic[18*MBDyn_CE_NodesNum];
	pMBDyn_CE_CouplingKinematic_omegap = &MBDyn_CE_CouplingKinematic[21*MBDyn_CE_NodesNum];
	//- C::E ground coordinate
	pMBDyn_CE_CEFrame = &MBDyn_CE_CouplingKinematic[24 * MBDyn_CE_NodesNum]; //- pointer to Ground information
	memcpy(&pMBDyn_CE_CEFrame[0], mbdyn_ce_ref_x, 3 * sizeof(double));
	memcpy(&pMBDyn_CE_CEFrame[3], mbdyn_ce_ref_R, 9 * sizeof(double));
	//- pointer to force
	pMBDyn_CE_CouplingDynamic_f = &MBDyn_CE_CouplingDynamic[0];
	pMBDyn_CE_CouplingDynamic_m = &MBDyn_CE_CouplingDynamic[3*MBDyn_CE_NodesNum];
	//- pointer to force of last iteration (not time step)
	pMBDyn_CE_CouplingDynamic_f_pre = &MBDyn_CE_CouplingDynamic_pre[0];
	pMBDyn_CE_CouplingDynamic_m_pre = &MBDyn_CE_CouplingDynamic_pre[3*MBDyn_CE_NodesNum];
	/* initialization (std::vector<>) - end*/

	/* initial chrono::engine system - start*/
	//---------- intial CE model, and allocate space for reloading C::E model data
	pMBDyn_CE_CEModel = MBDyn_CE_CEModel_Init(MBDyn_CE_CEModel_Data, MBDyn_CE_CEMotorType, pMBDyn_CE_CEFrame, MBDyn_CE_CEScale, MBDyn_CE_CEModel_Label, MBDyn_CE_CouplingType);
	if (pMBDyn_CE_CEModel == NULL)
	{
		silent_cerr("ChronoInterface(" << uLabel << "): fails at creating C::E model at line " << HP.GetLineData() << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}
	/* initial chrono::engine system - end*/
}

ChronoInterfaceBaseElem::~ChronoInterfaceBaseElem(void)
{
	// destroy private data
	MBDyn_CE_CEModel_Destroy(pMBDyn_CE_CEModel);
}

void 
ChronoInterfaceBaseElem::SetValue(DataManager *pDM,
									   VectorHandler &X, VectorHandler &XP,
									   SimulationEntity::Hints *h)
{
	pedantic_cerr("\tMBDyn::SetValue()\n");
	//- SetValue function doesn't call MBDyn_CE_CEModel_DoStepDynamics()
	//- just save data if using tight coupling scheme for the first step
	MBDyn_CE_CouplingIter_Count = MBDyn_CE_CouplingIter_Max;
	bMBDyn_CE_FirstSend = false;
	bMBDyn_CE_CEModel_DoStepDynamics = false;	

	//---------- set gravity 
	//- the default graivity is: (0.0,-9.81,0.0)
	Vec3 mbdynce_mbdyn_gravity_vec3;
	double mbdynce_mbdyn_gravity[3];
	Vec3 mbdynce_mbdyn_vec3 = Zero3; // arm of the gravity. is not used.
	bool bmbdynce_gravity;
	GravityOwner::bGetGravity(mbdynce_mbdyn_vec3, mbdynce_mbdyn_gravity_vec3);
	MBDyn_CE_Vec3D(mbdynce_mbdyn_gravity_vec3, mbdynce_mbdyn_gravity, MBDyn_CE_CEScale[0]); //- transfer Vec3 to double[]
	std::cout << "\t\tgravity is: " << mbdynce_mbdyn_gravity[0] << "\t"<<mbdynce_mbdyn_gravity[1]<<"\t"<<mbdynce_mbdyn_gravity[2]<<"\n"; //- print in screen
	
	//---------- initial check
	MBDyn_CE_SendDataToBuf(); //- send initial data to BUF, where C::E model can read infromation.
	if (!MBDyn_CE_CEModel_InitCheck(pMBDyn_CE_CEModel, MBDyn_CE_CouplingKinematic, MBDyn_CE_NodesNum, MBDyn_CE_CEModel_Label, mbdynce_mbdyn_gravity))
	{
		silent_cerr("ChronoInterface(" << uLabel << ") data in C::E and in MBDyn are inconsistent " << std::endl);
		throw ErrGeneric(MBDYN_EXCEPT_ARGS);
	}
	switch (MBDyn_CE_CouplingType)
	{
	case MBDyn_CE_COUPLING::COUPLING_NONE:
		break; //- do nothing
	case MBDyn_CE_COUPLING::COUPLING_TIGHT:
		if (MBDyn_CE_CEModel_DataSave(pMBDyn_CE_CEModel, MBDyn_CE_CEModel_Data)) //- save data in MBDyn_CE_CEModel_Data
		{
			silent_cerr("ChronoInterface(" << uLabel << ") data saving process is wrong " << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		break;
	case MBDyn_CE_COUPLING::COUPLING_LOOSE:
		break; //- do nothing
	case MBDyn_CE_COUPLING::COUPLING_STSTAGGERED: //- to do
	default: //- multirate  to do 
		break; 
	}	
}

void 
ChronoInterfaceBaseElem::Update(const VectorHandler &XCurr,
                        const VectorHandler &XprimeCurr)
{
	//---------- A regular step to update C::E model
	//---------- 1. mbdyn writes kinematic coupling variables to buffer;
	//---------- 2. C::E models reload data (only tight coupling scheme);
	//---------- 3. C::E models read the coupling data from buffer;
	//---------- 4. C::E models do integration (one step);
	//---------- 5. C::E models sends data to the buffer;
	pedantic_cout("\tMBDyn::Update()\n");
	if (bMBDyn_CE_CEModel_DoStepDynamics || bMBDyn_CE_FirstSend)
	{
		pedantic_cout("\tMBDyn-C::E::Update a regular step\n");
		//---------- 1. mbdyn writes kinematic coupling variables to buffer;
		MBDyn_CE_SendDataToBuf();
		//---------- 2. C::E models reload data (tight coupling scheme);
		if(MBDyn_CE_CouplingType == MBDyn_CE_COUPLING::COUPLING_TIGHT) //- tight coupling
		{
			if(MBDyn_CE_CEModel_DataReload(pMBDyn_CE_CEModel, MBDyn_CE_CEModel_Data))
			{
				silent_cerr("ChronoInterface(" << uLabel << ") data reloading process is wrong " << std::endl);
				throw ErrGeneric(MBDYN_EXCEPT_ARGS);
			}
		}
		//---------- 3. C::E models read the coupling data from buffer;
		time_step = m_pDM->pGetDrvHdl()->dGetTimeStep(); //- get time step
		if (MBDyn_CE_CEModel_RecvFromBuf(pMBDyn_CE_CEModel, MBDyn_CE_CouplingKinematic, MBDyn_CE_NodesNum, MBDyn_CE_CEModel_Label, MBDyn_CE_CEMotorType, time_step, bMBDyn_CE_Verbose))
		{
			silent_cerr("ChronoInterface(" << uLabel << ") C::E receiving data process is wrong " << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		//---------- 4. C::E models do integration (one step);
		if(MBDyn_CE_CEModel_DoStepDynamics(pMBDyn_CE_CEModel, time_step, bMBDyn_CE_Verbose))
		{
			silent_cerr("ChronoInterface(" << uLabel << ") C::E integration process is wrong " << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		//---------- 5. C::E models sends data to the buffer;
		if(MBDyn_CE_CEModel_SendToBuf(pMBDyn_CE_CEModel, MBDyn_CE_CouplingDynamic,pMBDyn_CE_CEFrame, MBDyn_CE_NodesNum, MBDyn_CE_CEScale, MBDyn_CE_CEModel_Label, bMBDyn_CE_Verbose))
		{
			silent_cerr("ChronoInterface(" << uLabel << ") C::E writting force process is wrong " << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		//---------- 6. MBDyn receives data from Buf;
		MBDyn_CE_RecvDataFromBuf();
		switch (MBDyn_CE_CouplingType)
		{
		case MBDyn_CE_COUPLING::COUPLING_TIGHT:
			bMBDyn_CE_FirstSend = false;
			bMBDyn_CE_CEModel_DoStepDynamics = true;
			MBDyn_CE_CEModel_Converged.Set(Converged::State::NOT_CONVERGED);
			break;
		case MBDyn_CE_COUPLING::COUPLING_LOOSE:
		case MBDyn_CE_COUPLING::COUPLING_STSTAGGERED:
		default:
			bMBDyn_CE_CEModel_DoStepDynamics = false;
			bMBDyn_CE_FirstSend = false;
			MBDyn_CE_CEModel_Converged.Set(Converged::State::CONVERGED);
			break;
		}
	}
}

void 
ChronoInterfaceBaseElem::AfterConvergence(const VectorHandler &X,
                                  const VectorHandler &XP)
{
	switch (MBDyn_CE_CouplingType)
	{
	case MBDyn_CE_COUPLING::COUPLING_NONE:
		pedantic_cout("\tMBDyn::AfterConvergence()\n");
		break; //- do nothing
	case MBDyn_CE_COUPLING::COUPLING_TIGHT:
		pedantic_cout("\tMBDyn::AfterConvergence()\n");
		break;
	case MBDyn_CE_COUPLING::COUPLING_LOOSE: //- to do
		pedantic_cout("\tMBDyn::AfterConvergence()\n");
		break;
	case MBDyn_CE_COUPLING::COUPLING_STSTAGGERED: //- to do
	default:
		break; 
	}
	if (bMBDyn_CE_Verbose)
	{
		double time;
		time = m_pDM->dGetTime();
		std::cout << "\t\ttime in MBDyn: " << time << "\t forces obtained: " << pMBDyn_CE_CouplingDynamic_f[0] << "\n";
		const MBDYN_CE_POINTDATA &mbdynce_point = MBDyn_CE_Nodes[0];
		//- rotation and position
		const Mat3x3 &mbdynce_R = mbdynce_point.pMBDyn_CE_Node->GetRCurr();
		Vec3 mbdynce_f = mbdynce_R * mbdynce_point.MBDyn_CE_Offset;
		Vec3 mbdynce_x = mbdynce_point.pMBDyn_CE_Node->GetXCurr() + mbdynce_f;
		//- angular velocity and velocity
		const Vec3 &mbdynce_w = mbdynce_point.pMBDyn_CE_Node->GetWCurr();
		Vec3 mbdynce_wCrossf = mbdynce_w.Cross(mbdynce_f);
		Vec3 mbdynce_v = mbdynce_point.pMBDyn_CE_Node->GetVCurr() + mbdynce_wCrossf;
		//- angular acceleration and acceleration
		const Vec3 &mbdynce_wp = mbdynce_point.pMBDyn_CE_Node->GetWPCurr();
		Vec3 mbdynce_a = mbdynce_point.pMBDyn_CE_Node->GetXPPCurr() + mbdynce_wp.Cross(mbdynce_f) + mbdynce_w.Cross(mbdynce_wCrossf);

		double mbdynce_tempvec3_x[3];
		double mbdynce_tempvec3_v[3];
		double mbdynce_tempvec3_a[3];
		double mbdynce_tempvec3_w[3];
		double mbdynce_tempvec3_wp[3];
		MBDyn_CE_Vec3D(mbdynce_x, mbdynce_tempvec3_x, MBDyn_CE_CEScale[0]);
		MBDyn_CE_Vec3D(mbdynce_v, mbdynce_tempvec3_v, MBDyn_CE_CEScale[0]);
		MBDyn_CE_Vec3D(mbdynce_a, mbdynce_tempvec3_a, MBDyn_CE_CEScale[0]);
		MBDyn_CE_Vec3D(mbdynce_w, mbdynce_tempvec3_w, 1.0);
		MBDyn_CE_Vec3D(mbdynce_wp, mbdynce_tempvec3_wp, 1.0);
		double mbdynce_tempmat3x3_R[9];
		MBDyn_CE_Mat3x3D(mbdynce_R, mbdynce_tempmat3x3_R);
		std::cout << "\t\tpos: " << mbdynce_tempvec3_x[0] - 1000 << "\t" << mbdynce_tempvec3_x[1] << "\t" << mbdynce_tempvec3_x[2] << "\n"
				  << "\t\tvel: " << mbdynce_tempvec3_v[0] << "\t" << mbdynce_tempvec3_v[1] << "\t" << mbdynce_tempvec3_v[2] << "\n"
				  << "\t\tacc: " << mbdynce_tempvec3_a[0] << "\t" << mbdynce_tempvec3_a[1] << "\t" << mbdynce_tempvec3_a[2] << "\n";
	}
	return;
}

void 
ChronoInterfaceBaseElem::AfterPredict(VectorHandler &X,
                              VectorHandler &XP)
{
	//- first send data after predict in each co-simulation (tight);
	//- after predict save C::E data and call MBDyn_CE_CEModel_DoStepDynamics();
	MBDyn_CE_CouplingIter_Count = 0;
	bMBDyn_CE_FirstSend = true;
	bMBDyn_CE_CEModel_DoStepDynamics = true;

	switch (MBDyn_CE_CouplingType)
	{
	case MBDyn_CE_COUPLING::COUPLING_NONE:
		pedantic_cout("\tMBDyn::AfterPredict() -no_coupling\n");		
		time_step = m_pDM->pGetDrvHdl()->dGetTimeStep(); //- get time step
		if(MBDyn_CE_CEModel_DoStepDynamics(pMBDyn_CE_CEModel, time_step, bMBDyn_CE_Verbose))
		{
			silent_cerr("ChronoInterface(" << uLabel << ") C::E integration process is wrong " << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}	
		bMBDyn_CE_FirstSend = false;
		bMBDyn_CE_CEModel_DoStepDynamics = false; //- only do time integration once
		MBDyn_CE_CEModel_Converged.Set(Converged::State::CONVERGED);
		break; 
	case MBDyn_CE_COUPLING::COUPLING_TIGHT:
		pedantic_cout("\tMBDyn::AfterPredict() -tight_embedded\n");
		if(MBDyn_CE_CEModel_DataSave(pMBDyn_CE_CEModel, MBDyn_CE_CEModel_Data))
		{
			silent_cerr("ChronoInterface(" << uLabel << ") data saving process is wrong " << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
		Update(X, XP); //- A regular step in C::E.
		break;
	case MBDyn_CE_COUPLING::COUPLING_LOOSE: 
	{
		switch (MBDyn_CE_CouplingType_loose)
		{
		case MBDyn_CE_COUPLING_LOOSE::TIGHT:
			break; //- do nothing (the work is finished in case MBDyn_CE_COUPLING::COUPLING_TIGHT)
		case MBDyn_CE_COUPLING_LOOSE::LOOSE_EMBEDDED:
			pedantic_cout("\tMBDyn::AfterPredict() -Loose_embedded\n");	
			Update(X, XP); //- A regular step in C::E.
			break;
		case MBDyn_CE_COUPLING_LOOSE::LOOSE_JACOBIAN: //- to do 
		case MBDyn_CE_COUPLING_LOOSE::LOOSE_GAUSS: //- to do
		default: //- to do
			break;
		}
		break;
	}
	case MBDyn_CE_COUPLING::COUPLING_STSTAGGERED: //- to do
	default: // multirate to do 
		break; 
	}
}


void
ChronoInterfaceBaseElem::WorkSpaceDim(integer* piNumRows, integer* piNumCols) const
{
	*piNumRows = 0;
	*piNumCols = 0;
}

VariableSubMatrixHandler& 
ChronoInterfaceBaseElem::AssJac(VariableSubMatrixHandler& WorkMat,
	doublereal dCoef, 
	const VectorHandler& XCurr,
	const VectorHandler& XPrimeCurr)
{
	pedantic_cout("\tMBDyn::AssJac()" << std::endl);
	WorkMat.SetNullMatrix();
	return WorkMat;
}

SubVectorHandler& 
ChronoInterfaceBaseElem::AssRes(SubVectorHandler& WorkVec,
	doublereal dCoef,
	const VectorHandler& XCurr, 
	const VectorHandler& XPrimeCurr)
{	
	if (MBDyn_CE_CouplingType == MBDyn_CE_COUPLING::COUPLING_NONE)
	{
		pedantic_cout("\tMBDyn::AssRes()\n");
		WorkVec.Resize(0);
		return WorkVec;
	}

	else if (MBDyn_CE_CouplingType >=-1) //- coupling case
	{
		//---------- A regular step in MBDyn
		//---------- 1. detect the convergence of coupling data;
		//---------- 2. add forces to Res;
		pedantic_cout("\tMBDyn::AssRes()\n");
		
		//---------- 1. detect the convergence of coupling data;
		if((bMBDyn_CE_CEModel_DoStepDynamics) && (MBDyn_CE_CouplingType==MBDyn_CE_COUPLING::COUPLING_TIGHT)) //- tight coupling case
		{
			pedantic_cout("\t\tAss_Coupling_Error()\n");
			pedantic_cout("\t\titerations: " << MBDyn_CE_CouplingIter_Count << "\n");			
			MBDyn_CE_CEModel_Converged.Set(Converged::State::NOT_CONVERGED);
			bMBDyn_CE_CEModel_DoStepDynamics = true;
			if (MBDyn_CE_CouplingIter_Count > 0) //- iters 2nd 
			{
				double mbdynce_error;
				mbdynce_error = MBDyn_CE_CalculateError();
				if (mbdynce_error < MBDyn_CE_Coupling_Tol || MBDyn_CE_CouplingIter_Count >= MBDyn_CE_CouplingIter_Max)
				{
					MBDyn_CE_CEModel_Converged.Set(Converged::State::CONVERGED);
					bMBDyn_CE_CEModel_DoStepDynamics = false; //- C::E doesn't need to do time integration until next step;
					pedantic_cout("\t\tCoupling error: " << mbdynce_error << "\n");
				}
				else
				{
					pedantic_cout("\t\tCoupling error: " << mbdynce_error << "\n");
				}
			}
			MBDyn_CE_CouplingIter_Count++;
		}
		//---------- 2. add forces to Nodes;
		const int iOffset = 6;
		WorkVec.ResizeReset(iOffset * MBDyn_CE_NodesNum);
		for (unsigned i = 0; i < MBDyn_CE_NodesNum; i++) 
		{
			const MBDYN_CE_POINTDATA& point = MBDyn_CE_Nodes[i];

			integer iFirstIndex = point.pMBDyn_CE_Node->iGetFirstMomentumIndex();
			for (int r = 1; r <= iOffset; r++) {
				WorkVec.PutRowIndex(i*iOffset + r, iFirstIndex + r);
			}

			WorkVec.Add(i*iOffset + 1, point.MBDyn_CE_F);
			WorkVec.Add(i*iOffset + 4, point.MBDyn_CE_M + (point.pMBDyn_CE_Node->GetRCurr()*point.MBDyn_CE_Offset).Cross(point.MBDyn_CE_F));
			//- save the force of last iteration (only used in tight co-simulation)
			memcpy(&pMBDyn_CE_CouplingDynamic_f_pre[3 * i], &pMBDyn_CE_CouplingDynamic_f[3 * i], 3 * sizeof(double));
			memcpy(&pMBDyn_CE_CouplingDynamic_m_pre[3 * i], &pMBDyn_CE_CouplingDynamic_m[3 * i], 3 * sizeof(double));
		} 
	}
	return WorkVec;
}

void 
ChronoInterfaceBaseElem::InitialWorkSpaceDim(
	integer* piNumRows,
	integer* piNumCols) const
{
	*piNumRows = 0;
	*piNumCols = 0;
}

VariableSubMatrixHandler&
ChronoInterfaceBaseElem::InitialAssJac(
	VariableSubMatrixHandler& WorkMat, 
	const VectorHandler& XCurr)
{
	pedantic_cout("\tInitialAssJac()" << std::endl);
	WorkMat.SetNullMatrix();
	switch (MBDyn_CE_CouplingType)
	{
	case MBDyn_CE_COUPLING::COUPLING_NONE:
		break; //- do nothing
	case MBDyn_CE_COUPLING::COUPLING_TIGHT: //- do nothing
		break;
	case MBDyn_CE_COUPLING::COUPLING_LOOSE: //- do nothing
	case MBDyn_CE_COUPLING::COUPLING_STSTAGGERED: //- do nothing
	default:
		break; 
	}
	return WorkMat;
}

SubVectorHandler& 
ChronoInterfaceBaseElem::InitialAssRes(
	SubVectorHandler& WorkVec,
	const VectorHandler& XCurr)
{
	pedantic_cout("\tInitialAssRes()" << std::endl);
	WorkVec.ResizeReset(0);
	switch (MBDyn_CE_CouplingType)
	{
	case MBDyn_CE_COUPLING::COUPLING_NONE:	
		break; //- do nothing
	case MBDyn_CE_COUPLING::COUPLING_TIGHT: //- do nothing
		break;
	case MBDyn_CE_COUPLING::COUPLING_LOOSE:	   //- do nothing
	case MBDyn_CE_COUPLING::COUPLING_STSTAGGERED: //- do nothing
	default:
		break; 
	}
	return WorkVec;
}

void
ChronoInterfaceBaseElem::MBDyn_CE_SendDataToBuf()
{
	/* formulation for calculation
				mbdynce_x = x + mbdynce_f
				mbdynce_R = R
				mbdynce_v = xp + mbdynce_w cross mbdynce_f
				mbdynce_w = w
				mbdynce_a = xpp + mbdynce_wp cross mbdynce_f + mbdynce_w cross mbdynce_w cross mbdynce_f
				mbdynce_wp = wp
	*/
	for (unsigned mbdynce_i = 0; mbdynce_i < MBDyn_CE_NodesNum; mbdynce_i++)
	{
		const MBDYN_CE_POINTDATA& mbdynce_point = MBDyn_CE_Nodes[mbdynce_i];
		//- rotation and position
		const Mat3x3 & mbdynce_R = mbdynce_point.pMBDyn_CE_Node->GetRCurr();
		Vec3 mbdynce_f = mbdynce_R * mbdynce_point.MBDyn_CE_Offset;
		Vec3 mbdynce_x = mbdynce_point.pMBDyn_CE_Node->GetXCurr() + mbdynce_f;
		//- angular velocity and velocity
		const Vec3 &mbdynce_w = mbdynce_point.pMBDyn_CE_Node->GetWCurr();
		Vec3 mbdynce_wCrossf = mbdynce_w.Cross(mbdynce_f);
		Vec3 mbdynce_v = mbdynce_point.pMBDyn_CE_Node->GetVCurr() + mbdynce_wCrossf;
		//- angular acceleration and acceleration
		const Vec3 &mbdynce_wp = mbdynce_point.pMBDyn_CE_Node->GetWPCurr();
		Vec3 mbdynce_a = mbdynce_point.pMBDyn_CE_Node->GetXPPCurr()+mbdynce_wp.Cross(mbdynce_f) + mbdynce_w.Cross(mbdynce_wCrossf);

		double mbdynce_tempvec3_x[3];
		double mbdynce_tempvec3_v[3];
		double mbdynce_tempvec3_a[3];
		double mbdynce_tempvec3_w[3];
		double mbdynce_tempvec3_wp[3];
		MBDyn_CE_Vec3D(mbdynce_x, mbdynce_tempvec3_x, MBDyn_CE_CEScale[0]);
		MBDyn_CE_Vec3D(mbdynce_v, mbdynce_tempvec3_v, MBDyn_CE_CEScale[0]);
		MBDyn_CE_Vec3D(mbdynce_a, mbdynce_tempvec3_a, MBDyn_CE_CEScale[0]);
		MBDyn_CE_Vec3D(mbdynce_w, mbdynce_tempvec3_w, 1.0);
		MBDyn_CE_Vec3D(mbdynce_wp, mbdynce_tempvec3_wp, 1.0);
		double mbdynce_tempmat3x3_R[9];
		MBDyn_CE_Mat3x3D(mbdynce_R, mbdynce_tempmat3x3_R);

		memcpy(&pMBDyn_CE_CouplingKinematic_x[3 * mbdynce_i], mbdynce_tempvec3_x, 3 * sizeof(double));
		memcpy(&pMBDyn_CE_CouplingKinematic_R[9* mbdynce_i], mbdynce_tempmat3x3_R, 9 * sizeof(double));
		memcpy(&pMBDyn_CE_CouplingKinematic_xp[3 * mbdynce_i], mbdynce_tempvec3_v, 3 * sizeof(double));
		memcpy(&pMBDyn_CE_CouplingKinematic_omega[3 * mbdynce_i], mbdynce_tempvec3_w, 3 * sizeof(double));
		memcpy(&pMBDyn_CE_CouplingKinematic_xpp[3 * mbdynce_i], mbdynce_tempvec3_a, 3 * sizeof(double));
		memcpy(&pMBDyn_CE_CouplingKinematic_omegap[3 * mbdynce_i], mbdynce_tempvec3_wp, 3 * sizeof(double));
	}
}

// Read the data from buffer, and write them to the Vec3 MBDyn_CE_F and Vec3 MBDyn_CE_M;
void 
ChronoInterfaceBaseElem::MBDyn_CE_RecvDataFromBuf()
{
	for (unsigned mbdynce_i = 0; mbdynce_i < MBDyn_CE_NodesNum; mbdynce_i++)
	{
		//- read from the buffer
		MBDYN_CE_POINTDATA& mbdynce_point = MBDyn_CE_Nodes[mbdynce_i];
		mbdynce_point.MBDyn_CE_F = Vec3(pMBDyn_CE_CouplingDynamic_f[3*mbdynce_i],pMBDyn_CE_CouplingDynamic_f[3*mbdynce_i+1],pMBDyn_CE_CouplingDynamic_f[3*mbdynce_i+2]);
		mbdynce_point.MBDyn_CE_M = Vec3(pMBDyn_CE_CouplingDynamic_m[3*mbdynce_i],pMBDyn_CE_CouplingDynamic_m[3*mbdynce_i+1],pMBDyn_CE_CouplingDynamic_m[3*mbdynce_i+2]);
	}
}


void
ChronoInterfaceBaseElem::Output(OutputHandler& OH) const
{
	pedantic_cout("\tMBDyn::Output()\n");
	if (bMBDyn_CE_Output)
	{
		if (!OH.IsOpen(OutputHandler::OutFiles::LOADABLE))
		{
			OH.Open(OutputHandler::OutFiles::LOADABLE);
		}
		std::ostream & mbdynce_cebody_output=OH.Loadable();
		if(MBDyn_CE_CEModel_WriteToFiles(pMBDyn_CE_CEModel, MBDyn_CE_CEModel_Label, pMBDyn_CE_CEFrame, MBDyn_CE_CEScale, mbdynce_cebody_output))
		{
			silent_cerr("ChronoInterface(" << uLabel << ") data writting process is wrong " << std::endl);
			throw ErrGeneric(MBDYN_EXCEPT_ARGS);
		}
	}
}

std::ostream&
ChronoInterfaceBaseElem::Restart(std::ostream& out) const
{
	return out << "# ModuleChronoInterface: is doing now" << std::endl;
}

/* private functions: start*/
void 
ChronoInterfaceBaseElem::MBDyn_CE_Vec3D(const Vec3& mbdynce_Vec3, double* mbdynce_temp, double MBDyn_CE_CELengthScale)
{
	mbdynce_temp[0] = MBDyn_CE_CELengthScale * static_cast<double>(*(mbdynce_Vec3.pGetVec()));
	mbdynce_temp[1] = MBDyn_CE_CELengthScale * static_cast<double>(*(mbdynce_Vec3.pGetVec() + 1));
	mbdynce_temp[2] = MBDyn_CE_CELengthScale * static_cast<double>(*(mbdynce_Vec3.pGetVec() + 2));
}

void 
ChronoInterfaceBaseElem::MBDyn_CE_Mat3x3D(const Mat3x3& mbdynce_Mat3x3, double *mbdynce_temp)
{
	for (unsigned i = 0; i < 9;i++)
	{
		mbdynce_temp[i] = static_cast<double>(mbdynce_Mat3x3.pGetMat()[i]);
	}
}

double 
ChronoInterfaceBaseElem::MBDyn_CE_CalculateError() //- calculate the error of coupling force.
{
	double mbdynce_temp_error = 0.0; //- using Euclidean norm
	for (unsigned i = 0; i < MBDyn_CE_NodesNum; i++)
	{
		for (unsigned j = 0; j < 3; j++)
		{
			mbdynce_temp_error += pow(pMBDyn_CE_CouplingDynamic_f_pre[3 * i + j] - pMBDyn_CE_CouplingDynamic_f[3 * i + j], 2);
			mbdynce_temp_error += pow(pMBDyn_CE_CouplingDynamic_m_pre[3 * i + j] - pMBDyn_CE_CouplingDynamic_m[3 * i + j], 2);
		}
		
	}
	return sqrt(mbdynce_temp_error);
}
/* private functions: end*/

extern "C" int
module_init(const char *module_name, void *pdm, void *php)
{
	UserDefinedElemRead *rf = new UDERead<ChronoInterfaceBaseElem>; //- or new ChronoInterfaceElemRead;
	if (!SetUDE("ChronoInterface", rf)) {
		delete rf;

		silent_cerr("module-Chrono-Interface: "
			"module_init(" << module_name << ") "
			"failed" << std::endl);

		return -1;
	}

	return 0;
}