// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
//
//
// A C::E Models: sliding point and a box linked by a spring-dash device;
// The global reference frame has Y up.
//
// =============================================================================

// Modified Date: 2020/06/09

#include "chrono/ChConfig.h"
#include "chrono/utils/ChUtilsCreators.h"
#include "chrono/utils/ChUtilsGenerators.h"
#include "chrono/physics/ChBodyEasy.h"
#include "chrono_parallel/physics/ChSystemParallel.h"
#include "chrono_parallel/solver/ChIterativeSolverParallel.h"
#include "chrono/assets/ChPointPointDrawing.h"
#ifdef CHRONO_OPENGL
#include "chrono_opengl/ChOpenGLWindow.h"
#endif

#include "mbdyn_ce.h"

using namespace chrono;
using namespace chrono::collision;

// --------------------------------------------------------------------------


extern "C" void
MBDyn_CE_CEModel_Create(ChSystemParallelNSC * pMBDyn_CE_CEModel)
{
	GetLog() << "\n\nCopyright (c) 2017 projectchrono.org\nChrono version: " << CHRONO_VERSION << "\n\n";

	// ----------------
	// 1. Parameters
	// ----------------
	const double PI = 3.141592653589793238462643;
	double radius = 0.0040;	

	int num_threads = 4;
	uint max_iteration_normal = 100;
	uint max_iteration_sliding =100;
	uint max_iteration_spinning = 0;
	uint max_iteration_bilateral = 100;

	// ------------------------
	// 2. Create the parallel system
	// --------------------------
	//ChSystemParallelNSC* pCEmodel_system;

	//ChSystemParallelNSC system;
	pMBDyn_CE_CEModel->Set_G_acc(ChVector<>(0, -9.810, 0));


	// Set number of threads

	 int max_threads = CHOMPfunctions::GetNumProcs();
	 if (num_threads < max_threads)
		 num_threads = max_threads-1;
	CHOMPfunctions::SetNumThreads(num_threads);

	// Set solver settings
	//system.SetTimestepperType(ChTimestepper::Type::NEWMARK);
	pMBDyn_CE_CEModel->ChangeSolverType(SolverType::APGDREF);

	pMBDyn_CE_CEModel->GetSettings()->perform_thread_tuning = true;

	pMBDyn_CE_CEModel->GetSettings()->solver.solver_mode = SolverMode::SLIDING;
	pMBDyn_CE_CEModel->GetSettings()->solver.max_iteration_normal = max_iteration_normal;
	pMBDyn_CE_CEModel->GetSettings()->solver.max_iteration_sliding = max_iteration_sliding;
	pMBDyn_CE_CEModel->GetSettings()->solver.max_iteration_spinning = max_iteration_spinning;
	pMBDyn_CE_CEModel->GetSettings()->solver.max_iteration_bilateral = max_iteration_bilateral;
	pMBDyn_CE_CEModel->GetSettings()->solver.alpha = 0;
	pMBDyn_CE_CEModel->GetSettings()->solver.contact_recovery_speed = 1.0;
	pMBDyn_CE_CEModel->GetSettings()->solver.use_full_inertia_tensor = false;
	pMBDyn_CE_CEModel->GetSettings()->solver.tolerance = 1.0e-6;

	pMBDyn_CE_CEModel->GetSettings()->collision.collision_envelope = 0.01 * radius;
	pMBDyn_CE_CEModel->GetSettings()->collision.narrowphase_algorithm = NarrowPhaseType::NARROWPHASE_HYBRID_MPR;
	pMBDyn_CE_CEModel->GetSettings()->collision.bins_per_axis = vec3(50, 50, 50);

	// ------------------------
	// 3. Create the ground
	// --------------------------
	auto ground = std::shared_ptr<ChBody>(pMBDyn_CE_CEModel->NewBody());
	pMBDyn_CE_CEModel->Add(ground);
	ground->SetPos(ChVector<>(0.0, 0.0, 0.0));
	ground->SetRot(ChQuaternion<>(1.0,0.0,0.0,0.0));
	ground->SetBodyFixed(true);
	ground->SetIdentifier(0);
	ground->SetMass(1.0);
	ground->SetCollide(false);
	//ground->GetCollisionModel()->ClearModel();
	//utils::AddBoxGeometry(ground.get(), mat,ChVector<>(0.2,0.005,0.2), ChVector<>(0, -0.005, 0));

	// ------------------------
	// 5. Create the container
	// ------------------------
	// or create a 3-D two-mass oscillator
	// creat 3-D mass
	double L=0.0; //half length of the pendulum (mm)
	double V0=100; //(mm/s)
	double alpha=90.0/180.0*PI; //
	double inertia_xx=1.0e6;
	ChVector<> pendulum_initial_pos(0.0,0.0,0.0);//(L * sin(alpha), -L * cos(alpha), 0.0);
	//ChMatrix33<> pendulum_initial_rot(alpha,ChVector<>(0.0,0.0,1.0));//rotate alpha with respect to z axis
	ChMatrix33<> pendulum_initial_rot(0.0,ChVector<>(0.0,0.0,1.0));// parallel to the coordinate system in mbdyn 
	//ChVector<> pendulum_initial_pos_dt(2*V0*cos(alpha),2*V0*sin(alpha),0.0);
	ChVector<> pendulum_initial_pos_dt(V0, 0.0, 0.0);
	auto Pendulum = std::shared_ptr<ChBody>(pMBDyn_CE_CEModel->NewBody());
	pMBDyn_CE_CEModel->AddBody(Pendulum);
	Pendulum->SetIdentifier(1);
	//Add initial conditions 
	Pendulum->SetPos(pendulum_initial_pos);//pendulum position
	Pendulum->SetRot(pendulum_initial_rot);
	//Chmatrix33 is the rotation matrix, unit: kg*mm^2
	Pendulum->SetInertia(ChMatrix33<>(ChVector<>(inertia_xx,0.0,0.0), ChVector<>(0.0,inertia_xx,0.0), ChVector<>(0.0,0.0,inertia_xx)));
	Pendulum->SetMass(1.0); // mass=1 kg
	Pendulum->SetCollide(false);
	// set initial velocity
	Pendulum->SetPos_dt(pendulum_initial_pos_dt);
	//Pendulum->SetWvel_par(ChVector<>(0.0,0.0,V0/L));
	// std::cout<<Pendulum->GetCoord()<<"\n"<<Pendulum->GetFrame_COG_to_abs()<<"\n";

	// pringf information of the physics system
	std::cout << "there is the C::E model you created:\n";
	std::cout<<"num of links:\t"<<pMBDyn_CE_CEModel->Get_linklist().size()<<"\n";
	std::cout<<"num of other physicslist:\t"<<pMBDyn_CE_CEModel->Get_otherphysicslist().size()<<"\n";
	std::cout<<"num of rigid bodies:\t"<<pMBDyn_CE_CEModel->Get_bodylist().size()<<"\n";
	std::cout<<"num of speed motor:\t"<<pMBDyn_CE_CEModel->data_manager->num_linmotors<<"\n";	
}