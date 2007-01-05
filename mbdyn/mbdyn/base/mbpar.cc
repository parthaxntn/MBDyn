/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2007
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

/* parser */

#ifdef HAVE_CONFIG_H
#include "mbconfig.h"           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include "mbpar.h"

#if defined(USE_HYDRAULIC_NODES)
#include "hfluid.h"
#endif /* USE_HYDRAULIC_NODES */

#if defined(USE_AERODYNAMIC_ELEMS)
#include "aerodc81.h"
#include "c81data.h"
#endif /* USE_AERODYNAMIC_ELEMS */

#ifdef HAVE_LTDL_H
#include <ltdl.h>
#elif defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif /* !HAVE_LTDL_H && HAVE_DLFCN_H */

#include "dataman.h"
#include "modules.h"

/* MBDynParser - begin */

void
mbdyn_license(void)
{
	silent_cout("license not available yet;"
		" see GPL at http://www.gnu.org/" << std::endl);
}

void
mbdyn_warranty(void)
{
	silent_cout("From GPL 2.1:\n"
"\n"
"  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
"  FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n"
"  OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
"  PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
"  OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
"  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n"
"  TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n"
"  PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
"  REPAIR OR CORRECTION.\n"
"\n"
"  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
"  WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n"
"  REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
"  INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
"  OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
"  TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
"  YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
"  PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
"  POSSIBILITY OF SUCH DAMAGES.\n"
<< std::endl);
}

MBDynParser::MBDynParser(MathParser& MP, 
		InputStream& streamIn,
		const char *initial_file)
: IncludeParser(MP, streamIn, initial_file),
pDM(0)
{
	/* make sure this is init'ed */
	InitDriveData();
	InitCL();
	InitSF();
}   


MBDynParser::~MBDynParser(void)
{   
	DestroyDriveData();
	DestroyCL();
	DestroySF();

#if defined(USE_STRUCT_NODES)
	for (RFType::iterator i = RF.begin(); i != RF.end(); i++) {
		SAFEDELETE(i->second);
	}
#endif /* USE_STRUCT_NODES */

#if defined(USE_HYDRAULIC_NODES)
	for (HFType::iterator i = HF.begin(); i != HF.end(); i++) {
		SAFEDELETE(i->second);
	}
#endif /* USE_HYDRAULIC_NODES */

#if defined(USE_AERODYNAMIC_ELEMS)
	for (ADType::iterator i = AD.begin(); i != AD.end(); i++) {
		SAFEDELETE(i->second);
	}
#endif /* USE_AERODYNAMIC_ELEMS */

	for (C1DType::iterator i = C1D.begin(); i != C1D.end(); i++) {
		SAFEDELETE(i->second);
	}

	for (C3DType::iterator i = C3D.begin(); i != C3D.end(); i++) {
		SAFEDELETE(i->second);
	}

	for (C6DType::iterator i = C6D.begin(); i != C6D.end(); i++) {
		SAFEDELETE(i->second);
	}

	for (DCType::iterator i = DC.begin(); i != DC.end(); i++) {
		SAFEDELETE(i->second);
	}
}

void
MBDynParser::SetDataManager(DataManager *pdm)
{
	ASSERT(pdm != NULL);
	pDM = pdm;
	const DriveHandler *pDH = pDM->pGetDrvHdl();
	if (pDH == 0) {
		silent_cerr("no drive handler is associated to data manager?"
				<< std::endl);

	} else {
		/* add the drive handler to the drive callers... */
		for (DCType::const_iterator i = DC.begin(); i != DC.end(); i++) {
			((DriveCaller *)i->second)->SetDrvHdl(pDH);
		}
	}
}

#if defined(USE_STRUCT_NODES)
const ReferenceFrame AbsRefFrame(0, 
				 Vec3(0.), 
				 Mat3x3(1., 0., 0., 
					0., 1., 0.,
					0., 0., 1.),
				 Vec3(0.),
				 Vec3(0.));


void 
MBDynParser::Reference_int(void)
{
	if (FirstToken() == UNKNOWN) {
		silent_cerr("Parser error in MBDynParser::Reference_int(),"
			" colon expected at line " 
			<< GetLineData() << std::endl);
		throw HighParser::ErrColonExpected();
	}
	
	unsigned int uLabel(GetInt());
	
	/* Nome del reference */
	const char *sName = NULL;
	if (IsKeyWord("name")) {
		const char *sTmp = GetStringWithDelims();
		SAFESTRDUP(sName, sTmp);
	}
	
	DEBUGLCOUT(MYDEBUG_INPUT, "Reference frame " << uLabel << std::endl);
	
	if (!IsKeyWord("position")) {
		pedantic_cerr("ReferenceFrame(" << uLabel
			<< "): missing keyword \"position\" at line "
			<< GetLineData() << std::endl);
	}
	Vec3 x(GetPosAbs(AbsRefFrame));
	if (!IsKeyWord("orientation")) {
		pedantic_cerr("ReferenceFrame(" << uLabel
			<< "): missing keyword \"orientation\" at line "
			<< GetLineData() << std::endl);
	}
	Mat3x3 R(GetRotAbs(AbsRefFrame));
	Vec3 v(0.);
	Vec3 w(0.);
	if (IsArg()) {
		if (!IsKeyWord("velocity")) {
			pedantic_cerr("ReferenceFrame(" << uLabel
				<< "): missing keyword \"velocity\" at line "
				<< GetLineData() << std::endl);
		}
		v = GetVelAbs(AbsRefFrame, x);
		if (IsArg()) {
			if (!IsKeyWord("angular" "velocity")) {
				pedantic_cerr("ReferenceFrame(" << uLabel
					<< "): missing keyword \"angular velocity\" at line "
					<< GetLineData() << std::endl);
			}
			w = GetOmeAbs(AbsRefFrame);
		}
	}
	
	DEBUGLCOUT(MYDEBUG_INPUT, std::endl
		   << "\tX = " << x << std::endl
		   << "\tR = " << R << std::endl
		   << "\tV = " << v << std::endl
		   << "\tW = " << w << std::endl);
	
	ReferenceFrame* pRF = NULL;
	SAFENEWWITHCONSTRUCTOR(pRF,
		ReferenceFrame,
		ReferenceFrame(uLabel, x, R, v, w));
	if (!RF.insert(RFType::value_type(uLabel, pRF)).second) {
		silent_cerr("Reference frame " << uLabel
			<< " already defined at line " << GetLineData()
			<< std::endl);
		throw MBDynParser::ErrReferenceAlreadyDefined();
	}
	
	if (sName != NULL) {
		pRF->PutName(sName);
		SAFEDELETEARR(sName);
	}
}
#endif /* USE_STRUCT_NODES */

#if defined(USE_HYDRAULIC_NODES)
void 
MBDynParser::HydraulicFluid_int(void)
{
	if (FirstToken() == UNKNOWN) {
		silent_cerr("Parser error in MBDynParser::HydraulicFluid_int(),"
			" colon expected at line "
			<< GetLineData() << std::endl);
		throw HighParser::ErrColonExpected();
	}
	
	unsigned int uLabel(GetInt());
	
	/* Nome del fluido */
	const char *sName = NULL;
	if (IsKeyWord("name")) {
		const char *sTmp = GetStringWithDelims();
		SAFESTRDUP(sName, sTmp);
	}
	
	HydraulicFluid* pHF = ReadHydraulicFluid(*this, uLabel);
	if (pHF == NULL) {
		silent_cerr("unable to read hydraulic fluid " << uLabel
				<< std::endl);
		throw ErrGeneric();
	}
	
	if (!HF.insert(HFType::value_type(uLabel, pHF)).second) {
		silent_cerr("hydraulic fluid " << uLabel
			<< " already defined at line " << GetLineData()
			<< std::endl);
		throw MBDynParser::ErrGeneric();
	}
	
	if (sName != NULL) {
		pHF->PutName(sName);
		SAFEDELETEARR(sName);
	}
}
#endif /* USE_HYDRAULIC_NODES */

#if defined(USE_AERODYNAMIC_ELEMS)
void 
MBDynParser::C81Data_int(void)
{
	if (FirstToken() == UNKNOWN) {
		silent_cerr("Parser error in MBDynParser::C81Data_int(),"
			" colon expected at line " << GetLineData()
			<< std::endl);
		throw HighParser::ErrColonExpected();
	}
	
	unsigned int uLabel(GetInt());
	
	/* Nome del profilo c81 */
	const char *sName = NULL;
	if (IsKeyWord("name")) {
		const char *sTmp = GetStringWithDelims();
		SAFESTRDUP(sName, sTmp);
	}
	
	const char* filename = GetFileName();
	std::ifstream in(filename);
	if (!in) {
		silent_cerr("unable to open file '" << filename << "' at line " 
			<< GetLineData() << std::endl);
		throw ErrGeneric();
	}
	
	DEBUGLCOUT(MYDEBUG_INPUT, "reading c81 data " << uLabel 
		   << " from file '" << filename << "'" << std::endl);
	
	C81Data* data = NULL;
	SAFENEWWITHCONSTRUCTOR(data, C81Data, C81Data(uLabel));

	if (IsKeyWord("fc511")) {
		if (read_fc511_data(in, data) != 0) {
			silent_cerr("unable to read c81 data " << uLabel 
				<< " from file '" << filename << "' "
				"in fc511 format at line " << GetLineData() << std::endl);
			throw ErrGeneric();
		}

	} else if (IsKeyWord("free" "format")) {
		if (read_c81_data_free_format(in, data) != 0) {
			silent_cerr("unable to read c81 data " << uLabel 
				<< " from file '" << filename << "' "
				"in free format at line " << GetLineData() << std::endl);
			throw ErrGeneric();
		}

	} else {
		if (read_c81_data(in, data) != 0) {
			silent_cerr("unable to read c81 data " << uLabel 
				<< " from file '" << filename << "' "
				"at line " << GetLineData() << std::endl);
			throw ErrGeneric();
		}
	}
	
#ifdef DEBUG
	if (DEBUG_LEVEL(MYDEBUG_INPUT)) {
		write_c81_data(std::cout, data);
	}
#endif

	if (!AD.insert(ADType::value_type(uLabel, data)).second) {
		silent_cerr("c81 data " << uLabel
			<< " already defined at line " << GetLineData()
			<< std::endl);
		throw MBDynParser::ErrGeneric();
	}
	
	if (sName != NULL) {
		data->PutName(sName);
		SAFEDELETEARR(sName);
	}
}
#endif /* USE_AERODYNAMIC_ELEMS */

void 
MBDynParser::ConstitutiveLaw_int(void)
{
	if (FirstToken() == UNKNOWN) {
		silent_cerr("Parser error in MBDynParser::ConstitutiveLaw_int(),"
			" colon expected at line "
			<< GetLineData() << std::endl);
		throw HighParser::ErrColonExpected();
	}

	if (pDM == 0) {
		silent_cerr("constitutive law parsing at line "
				<< GetLineData() << " allowed "
				"only after control data block" << std::endl);
		throw MBDynParser::ErrGeneric();
	}
	
	unsigned int uLabel(GetInt());
	
	/* Constitutive law name */
	const char *sName = NULL;
	if (IsKeyWord("name")) {
		const char *sTmp = GetStringWithDelims();
		SAFESTRDUP(sName, sTmp);
	}

	int dim = GetInt();
	ConstLawType::Type clt;
	switch (dim) {
	case 1:
	{
		ConstitutiveLaw1D *pCL = pDM->ReadConstLaw1D(*this, clt);
		if (pCL == NULL) {
			silent_cerr("unable to read constitutive law 1D " 
					<< uLabel);
			if (sName) {
				silent_cerr(" (" << sName << ")");
			}
			silent_cerr(" at line " << GetLineData()
					<< std::endl);
			throw MBDynParser::ErrGeneric();
		}

		pCL->PutLabel(uLabel);
		if (sName) {
			pCL->PutName(sName);
		}
	
		if (!C1D.insert(C1DType::value_type(uLabel, pCL)).second) {
			silent_cerr("constitutive law 1D " << uLabel);
			if (sName) {
				silent_cerr(" (" << sName << ")");
			}
			silent_cerr(" already defined at line " 
					<< GetLineData() << std::endl);
			throw MBDynParser::ErrGeneric();
		}
		break;
	}

	case 3:
	{
		ConstitutiveLaw3D *pCL = pDM->ReadConstLaw3D(*this, clt);
		if (pCL == NULL) {
			silent_cerr("unable to read constitutive law 1D " 
					<< uLabel);
			if (sName) {
				silent_cerr(" (" << sName << ")");
			}
			silent_cerr(" at line " << GetLineData()
					<< std::endl);
			throw MBDynParser::ErrGeneric();
		}
	
		pCL->PutLabel(uLabel);
		if (sName) {
			pCL->PutName(sName);
		}
	
		if (!C3D.insert(C3DType::value_type(uLabel, pCL)).second) {
			silent_cerr("constitutive law 3D " << uLabel);
			if (sName) {
				silent_cerr(" (" << sName << ")");
			}
			silent_cerr(" already defined at line " 
					<< GetLineData() << std::endl);
			throw MBDynParser::ErrGeneric();
		}
		break;
	}

	case 6:
	{
		ConstitutiveLaw6D *pCL = pDM->ReadConstLaw6D(*this, clt);
		if (pCL == NULL) {
			silent_cerr("unable to read constitutive law 6D " 
					<< uLabel);
			if (sName) {
				silent_cerr(" (" << sName << ")");
			}
			silent_cerr(" at line " << GetLineData()
					<< std::endl);
			throw MBDynParser::ErrGeneric();
		}
	
		pCL->PutLabel(uLabel);
		if (sName) {
			pCL->PutName(sName);
		}
	
		if (!C6D.insert(C6DType::value_type(uLabel, pCL)).second) {
			silent_cerr("constitutive law 6D " << uLabel);
			if (sName) {
				silent_cerr(" (" << sName << ")");
			}
			silent_cerr(" already defined at line " 
					<< GetLineData() << std::endl);
			throw MBDynParser::ErrGeneric();
		}
		break;
	}

	default:
		silent_cerr("unknown constitutive law dimensionality " 
				<< dim << " at line " << GetLineData()
				<< std::endl);
		throw ErrGeneric();
	}

	if (sName) {
		SAFEDELETEARR(sName);
	}
}

void 
MBDynParser::DriveCaller_int(void)
{
	if (FirstToken() == UNKNOWN) {
		silent_cerr("Parser error in MBDynParser::DriveCaller_int(), "
			" colon expected at line "
			<< GetLineData() << std::endl);
		throw HighParser::ErrColonExpected();
	}

	unsigned int uLabel(GetInt());
	
	/* drive name */
	const char *sName = NULL;
	if (IsKeyWord("name")) {
		const char *sTmp = GetStringWithDelims();
		SAFESTRDUP(sName, sTmp);
	}

	bool bDeferred(false);
	if (IsKeyWord("deferred")) {
		bDeferred = true;
	}

	DriveCaller *pDC = 0;
	try {
		pDC = ReadDriveData(pDM, *this, bDeferred);
	}
	catch (DataManager::ErrNeedDataManager) {
		silent_cerr("the \"drive caller\" statement must appear "
				"inside or after the \"control data\" block"
				<< std::endl);
		throw DataManager::ErrNeedDataManager();
	}
	if (pDC == NULL) {
		silent_cerr("unable to read drive caller " << uLabel);
		if (sName) {
			silent_cerr(" (" << sName << ")");
		}
		silent_cerr(" at line " << GetLineData()
				<< std::endl);
		throw MBDynParser::ErrGeneric();
	}

	pDC->PutLabel(uLabel);
	if (sName) {
		pDC->PutName(sName);
	}
	
	if (!DC.insert(DCType::value_type(uLabel, pDC)).second) {
		silent_cerr("drive caller " << uLabel);
		if (sName) {
			silent_cerr(" (" << sName << ")");
		}
		silent_cerr(" already defined at line " 
				<< GetLineData() << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	if (sName) {
		SAFEDELETEARR(sName);
	}
}

void 
MBDynParser::ScalarFunction_int(void)
{
	if (FirstToken() == UNKNOWN) {
		silent_cerr("Parser error in MBDynParser::ScalarFunction_int(), "
			" colon expected at line "
			<< GetLineData() << std::endl);
		throw HighParser::ErrColonExpected();
	}

	(void)ParseScalarFunction(*this, pDM);
}

void 
MBDynParser::ModuleLoad_int(void)
{
#if !defined(HAVE_RUNTIME_LOADING)
	silent_cerr("ModuleLoad_int: dynamic modules not supported"
			<< std::endl);
	throw ErrGeneric();
#else /* HAVE_RUNTIME_LOADING */
	if (FirstToken() == UNKNOWN) {
		silent_cerr("Parser error in MBDynParser::ModuleLoad_int(), "
			" colon expected at line "
			<< GetLineData() << std::endl);
		throw HighParser::ErrColonExpected();
	}

   	/* nome del modulo */
   	const char* s = GetFileName();
	if (s == NULL) {
		silent_cerr("ModuleLoad_int: unable to get module name"
			<< std::endl);
		throw ErrGeneric();
	}

	char *module_name = 0;
   	SAFESTRDUP(module_name, s);

	module_initialize();

#ifdef HAVE_LTDL_H
	lt_dlhandle handle = lt_dlopenext(module_name);
#elif defined(HAVE_DLFCN_H)
   	void* handle = dlopen(module_name, RTLD_NOW /* RTLD_LAZY */ );
#endif /* !HAVE_LTDL_H && HAVE_DLFCN_H */

	if (handle == NULL) {
#ifdef HAVE_LTDL_H
      		const char* err = lt_dlerror();
#elif defined(HAVE_DLFCN_H)
      		const char* err = dlerror();
#endif /* !HAVE_LTDL_H && HAVE_DLFCN_H */

		if (err == 0) {
			err = "";
		}

      		silent_cerr("ModuleLoad_int: "
			<< "unable to open module <" << module_name 
			<< "> (" << err << ") at line " << GetLineData()
			<< std::endl);
      		throw ErrGeneric();
   	}

	typedef int (*sym_t)(const char *, void *, void *);
#ifdef HAVE_LTDL_H
	sym_t sym = (sym_t)lt_dlsym(handle, "module_init");
#elif defined(HAVE_DLFCN_H)
	sym_t sym = (sym_t)dlsym(handle, "module_init");
#endif /* !HAVE_LTDL_H && HAVE_DLFCN_H */
	
   	if (sym == NULL) {
#ifdef HAVE_LTDL_H
      		const char* err = lt_dlerror();
#elif defined(HAVE_DLFCN_H)
      		const char* err = dlerror();
#endif /* !HAVE_LTDL_H && HAVE_DLFCN_H */

      		if (err == NULL) {
			silent_cerr("ModuleLoad_int: module_init() "
				"function not available "
				"in module <" << module_name
				<< "> at line " << GetLineData()
				<< std::endl);
      		} else {
	 		silent_cerr("ModuleLoad_int: "
	   			<< "error while binding to symbol "
				"module_init() in module <" << module_name
	   			<< "> (" << err << ") at line " << GetLineData()
				<< std::endl);
      		}
      		throw ErrGeneric();
   	}

	if ((*sym)(module_name, (void *)pDM, (void *)this)) {
		silent_cerr("ModuleLoad_int: module_init() "
				"of module <" << module_name
				<< "> failed at line " << GetLineData()
				<< std::endl);
		throw ErrGeneric();
	}

   	SAFEDELETEARR(module_name);
#endif /* HAVE_RUNTIME_LOADING */
}

bool
MBDynParser::GetDescription_int(const char *s)
{
	/* Se trova un sistema di riferimento, lo gestisce direttamente */
	if (!strcmp(s, "reference")) {
#if defined(USE_STRUCT_NODES)      
		Reference_int();
		return true;
#else /* USE_STRUCT_NODES */
		throw MBDynParser::ErrGeneric();
#endif /* USE_STRUCT_NODES */

	/* Se trova un fluido idraulico, lo gestisce direttamente */
	} else if (!strcmp(s, "hydraulic" "fluid")) {
#if defined(USE_HYDRAULIC_NODES)
		HydraulicFluid_int();
		return true;
#else /* USE_HYDRAULIC_NODES */
		throw MBDynParser::ErrGeneric();
#endif /* USE_HYDRAULIC_NODES */

	/* Se trova dati aerodinamici c81, li gestisce direttamente */
	} else if (!strcmp(s, "c81" "data")) {
#if defined(USE_AERODYNAMIC_ELEMS)
		C81Data_int();
		return true;
#else /* USE_AERODYNAMIC_ELEMS */
		throw MBDynParser::ErrGeneric();
#endif /* USE_AERODYNAMIC_ELEMS */

	/* Reads a constitutive law */
	} else if (!strcmp(s, "constitutive" "law")) {
		ConstitutiveLaw_int();
		return true;

	/* Reads a drive caller */
	} else if (!strcmp(s, "drive" "caller")) {
		DriveCaller_int();
		return true;

	/* Reads a scalar function */
	} else if (!strcmp(s, "scalar" "function")) {
		ScalarFunction_int();
		return true;

	/* Loads a dynamic module */
	} else if (!strcmp(s, "module" "load" )) {
		ModuleLoad_int();
		return true;

	/* Scrive la licenza */
	} else if (!strcmp(s, "license")) {
		mbdyn_license();
		CurrLowToken = LowP.GetToken(*pIn);
		return true;

	/* Scrive il disclaimer */
	} else if (!strcmp(s, "warranty")) {
		mbdyn_warranty();
		CurrLowToken = LowP.GetToken(*pIn);
		return true;
	}

	/* altrimenti e' una description normale */
	return IncludeParser::GetDescription_int(s);
}

#if defined(USE_STRUCT_NODES)
MBDynParser::Frame 
MBDynParser::GetRef(ReferenceFrame& rf)
{
	if (!IsKeyWord("reference")) {
		return MBDynParser::UNKNOWNFRAME;
	}
	
	if (IsKeyWord("global")) {
		return MBDynParser::GLOBAL;
	}
	
	if (IsKeyWord("node")) {
		return MBDynParser::NODE;
	}
	
	if (IsKeyWord("local")) {
		return MBDynParser::LOCAL;
	}
	
	unsigned int uLabel((unsigned int)GetInt());
	RFType::const_iterator i = RF.find(uLabel);
	if (i == RF.end()) {
		silent_cerr("reference " << uLabel << " is undefined at line " 
			<< GetLineData() << std::endl);
		throw MBDynParser::ErrReferenceUndefined();
	}

	rf = *(i->second);

	return MBDynParser::REFERENCE;   
}

void 
MBDynParser::OutputFrames(std::ostream& out) const
{
	for (RFType::const_iterator i = RF.begin(); i != RF.end(); i++) {
		i->second->Output(out);
	}
}

Vec3 
MBDynParser::GetPosRel(const ReferenceFrame& rf)
{
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
		return (rf.GetR()).Transpose()*(GetVec3()-rf.GetX());
	
	case NODE:
	case UNKNOWNFRAME:
		return GetVec3();
	
	case LOCAL: {
		Mat3x3 R(GetMatR2vec());
		return R*GetVec3();
	}
	
	case REFERENCE:
		return rf.GetR().Transpose()*((rfOut.GetX()
			+rfOut.GetR()*GetVec3()
			)-rf.GetX());
			
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Vec3 
MBDynParser::GetPosAbs(const ReferenceFrame& rf)
{
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
	case UNKNOWNFRAME:
		return GetVec3();
	
	case NODE:
		return rf.GetX()+rf.GetR()*GetVec3();
		
	case LOCAL: {
		Mat3x3 R(GetMatR2vec());
		return rf.GetX()+rf.GetR()*(R*GetVec3());
	}
	
	case REFERENCE:
		return rfOut.GetX()+rfOut.GetR()*GetVec3();
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Vec3 
MBDynParser::GetVelRel(const ReferenceFrame& rf, const Vec3& x)
{
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
		return (rf.GetR()).Transpose()*(GetVec3()
			-rf.GetV()
			-rf.GetW().Cross(x-rf.GetX()));
	
	case NODE:
	case UNKNOWNFRAME:
		return GetVec3();
	
	case LOCAL: {
		Mat3x3 R(GetMatR2vec());
		return R*GetVec3();
	}
	
	case REFERENCE:
		return (rf.GetR()).Transpose()*(
			rfOut.GetV()
			+rfOut.GetR()*GetVec3()
			+rfOut.GetW().Cross(x-rfOut.GetX())
			-rf.GetV()
			-rf.GetW().Cross(x-rf.GetX())
			);
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Vec3 
MBDynParser::GetVelAbs(const ReferenceFrame& rf, const Vec3& x)
{
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
	case UNKNOWNFRAME:
		return GetVec3();
	
	case NODE:
		return rf.GetV()+rf.GetR()*GetVec3()
			+rf.GetW().Cross(x-rf.GetX());
	
	case LOCAL: {
		Mat3x3 R(GetMatR2vec());
		return rf.GetV()+rf.GetR()*(R*GetVec3())
			+rf.GetW().Cross(x-rf.GetX());	  
	}
	
	case REFERENCE:
		return rfOut.GetV()+rfOut.GetR()*GetVec3()
			+rfOut.GetW().Cross(x-rfOut.GetX());
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Vec3 
MBDynParser::GetOmeRel(const ReferenceFrame& rf)
{
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
		return (rf.GetR()).Transpose()*(GetVec3()-rf.GetW());
	
	case NODE:
	case UNKNOWNFRAME:
		return GetVec3();
	
	case LOCAL: {
		Mat3x3 R(GetMatR2vec());
		return R*GetVec3();
	}
	
	case REFERENCE:
		return (rf.GetR()).Transpose()*((rfOut.GetW()
			+rfOut.GetR()*GetVec3()
			)-rf.GetW());
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Vec3 
MBDynParser::GetOmeAbs(const ReferenceFrame& rf)
{
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
	case UNKNOWNFRAME:
		return GetVec3();
	
	case NODE:
		return rf.GetW()+rf.GetR()*GetVec3();
	
	case LOCAL: {
		Mat3x3 R(GetMatR2vec());
		return rf.GetW()+rf.GetR()*(R*GetVec3());
	}
	
	case REFERENCE:
		return rfOut.GetW()+rfOut.GetR()*GetVec3();
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Vec3 
MBDynParser::GetVecRel(const ReferenceFrame& rf)
{   
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
		return (rf.GetR()).Transpose()*GetVec3();
	
	case UNKNOWNFRAME: /* global */
		if (IsKeyWord("from" "node")) {
			/* FIXME */
			silent_cerr("'from node' at line " << GetLineData()
				<< " not implemented yet :)" << std::endl);
			throw MBDynParser::ErrGeneric();
			
			unsigned int uLabel = GetInt();
			StructNode *pNode1 = NULL; /* get node 1 */
			if (IsKeyWord("to" "node")) {
				silent_cerr("missing keyword 'to node' at line "
					<< GetLineData() << std::endl);
				throw MBDynParser::ErrGeneric();
			}
			uLabel = GetInt();
			StructNode *pNode2 = NULL; /* get node 2 */

			Vec3 v = pNode2->GetXCurr()-pNode1->GetXCurr();
			return (rf.GetR()).Transpose()*v;
		} /* else local */
	case NODE:
		return GetVec3();
	
	case LOCAL: {
		Mat3x3 R(GetMatR2vec());
		return R*GetVec3();
	}
	
	case REFERENCE:
		return (rf.GetR()).Transpose()*(rfOut.GetR()*GetVec3());
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Vec3 
MBDynParser::GetVecAbs(const ReferenceFrame& rf)
{   
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case UNKNOWNFRAME: /* global */
		if (IsKeyWord("from" "node")) {
			/* FIXME */
			silent_cerr("'from node' at line " << GetLineData()
				<< " not implemented yet :)" << std::endl);
			throw MBDynParser::ErrGeneric();
			
			unsigned int uLabel = GetInt();
			StructNode *pNode1 = NULL; /* get node 1 */
			if (IsKeyWord("to" "node")) {
				silent_cerr("missing keyword 'to node' at line "
					<< GetLineData() << std::endl);
				throw MBDynParser::ErrGeneric();
			}
			uLabel = GetInt();
			StructNode *pNode2 = NULL; /* get node 2 */

			return pNode2->GetXCurr() - pNode1->GetXCurr();
		} /* else global: fallthru */

	case GLOBAL:
		return GetVec3();
	
	case NODE:
		return rf.GetR()*GetVec3();
	
	case LOCAL: {
		Mat3x3 R(GetMatR2vec());
		return rf.GetR()*(R*GetVec3());
	}
	
	case REFERENCE:
		return rfOut.GetR()*GetVec3();
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Mat3x3
MBDynParser::GetMatRel(const ReferenceFrame& rf)
{
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
		return rf.GetR().Transpose()*(GetMat3x3()*rf.GetR());
		
	case NODE:
	case LOCAL:
	case UNKNOWNFRAME:
		return GetMat3x3();
	
	case REFERENCE:
		return rf.GetR().Transpose()
			*(rfOut.GetR()
			*(GetMat3x3()
			*(rfOut.GetR().Transpose()*rf.GetR())));
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Mat3x3
MBDynParser::GetMatAbs(const ReferenceFrame& rf)
{
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
	case UNKNOWNFRAME:
		return GetMat3x3();
	
	case NODE:
	case LOCAL:
		return rf.GetR()*(GetMat3x3()*rf.GetR().Transpose());

	case REFERENCE:
		return rfOut.GetR()*(GetMat3x3()*rfOut.GetR().Transpose());

	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Mat3x3 
MBDynParser::GetRotRel(const ReferenceFrame& rf)
{   
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
		return rf.GetR().Transpose()*GetMatR2vec();
	
	case NODE:
	case LOCAL:
	case UNKNOWNFRAME:
		return GetMatR2vec();
	
	case REFERENCE:
		return rf.GetR().Transpose()*(rfOut.GetR()*GetMatR2vec());
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}

Mat3x3 
MBDynParser::GetRotAbs(const ReferenceFrame& rf)
{   
	ReferenceFrame rfOut;
	switch (GetRef(rfOut)) {
	case GLOBAL:
	case UNKNOWNFRAME:
		return GetMatR2vec();
	
	case NODE:
	case LOCAL:
		return rf.GetR()*GetMatR2vec();
	
	case REFERENCE:
		return rfOut.GetR()*GetMatR2vec();
	
	default:
		ASSERTMSG(0, "You shouldn't have reached this point");
		throw MBDynParser::ErrGeneric();
	}
}
#endif /* USE_STRUCT_NODES */

#if defined(USE_HYDRAULIC_NODES)
HydraulicFluid* 
MBDynParser::GetHydraulicFluid(void)
{
	/* verifica che sia stato chiamato con "hydraulic" "fluid" */
	if (!IsKeyWord("hydraulic" "fluid") && !IsKeyWord("fluid")) {
		silent_cerr("hydraulic fluid expected at line "
			<< GetLineData() << std::endl);
		throw ErrGeneric();
	}
	
	/* se non trova "reference", legge direttamente un fluido */
	if (!IsKeyWord("reference")) {
		return ReadHydraulicFluid(*this, 0);
	}
	
	/* altrimenti usa un fluido predefinito, se lo trova */
	unsigned int uLabel = GetInt();
	HFType::const_iterator i = HF.find(uLabel);
	if (i == HF.end()) {
		silent_cerr("hydraulic fluid " << uLabel
			<< " is undefined at line " << GetLineData()
			<< std::endl);
		throw MBDynParser::ErrGeneric();
	}

	return i->second->pCopy();
}
#endif /* USE_HYDRAULIC_NODES */

#if defined(USE_AERODYNAMIC_ELEMS)
const c81_data* 
MBDynParser::GetC81Data(integer profile)
{
	/* cerca i dati predefiniti, se li trova */
	ADType::const_iterator i = AD.find(profile);
	if (i == AD.end()) {
		silent_cerr("c81 data " << profile << " is undefined at line " 
			<< GetLineData() << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	if (i == AD.end()) {
		silent_cerr("c81 data " << profile << " is undefined at line " 
			<< GetLineData() << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	return i->second;
}
#endif /* USE_AERODYNAMIC_ELEM */

ConstitutiveLaw1D *
MBDynParser::GetConstLaw1D(ConstLawType::Type& clt)
{
	if (pDM == 0) {
		silent_cerr("consitutive law parsing at line "
				<< GetLineData() << " allowed "
				"only after control data block" << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	if (!IsKeyWord("reference")) {
		return pDM->ReadConstLaw1D(*this, clt);
	}

	unsigned int uLabel = GetInt();
	C1DType::const_iterator i = C1D.find(uLabel);
	if (i == C1D.end()) {
		silent_cerr("constitutive law 1D " << uLabel
				<< " is undefined at line "
				<< GetLineData() << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	clt = i->second->GetConstLawType();
	return i->second->pCopy();
}

ConstitutiveLaw3D *
MBDynParser::GetConstLaw3D(ConstLawType::Type& clt)
{
	if (pDM == 0) {
		silent_cerr("consitutive law parsing at line "
				<< GetLineData() << " allowed "
				"only after control data block" << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	if (!IsKeyWord("reference")) {
		return pDM->ReadConstLaw3D(*this, clt);
	}

	unsigned int uLabel = GetInt();
	C3DType::const_iterator i = C3D.find(uLabel);
	if (i == C3D.end()) {
		silent_cerr("constitutive law 3D " << uLabel
				<< " is undefined at line "
				<< GetLineData() << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	clt = i->second->GetConstLawType();
	return i->second->pCopy();
}

ConstitutiveLaw6D *
MBDynParser::GetConstLaw6D(ConstLawType::Type& clt)
{
	if (pDM == 0) {
		silent_cerr("consitutive law parsing at line "
				<< GetLineData() << " allowed "
				"only after control data block" << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	if (!IsKeyWord("reference")) {
		return pDM->ReadConstLaw6D(*this, clt);
	}

	unsigned int uLabel = GetInt();
	C6DType::const_iterator i = C6D.find(uLabel);
	if (i == C6D.end()) {
		silent_cerr("constitutive law 3D " << uLabel
				<< " is undefined at line "
				<< GetLineData() << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	clt = i->second->GetConstLawType();
	return i->second->pCopy();
}

DriveCaller *
MBDynParser::GetDriveCaller(bool bDeferred)
{
	if (!IsKeyWord("reference")) {
		DriveCaller *pDC = 0;
		try {
			pDC = ReadDriveData(pDM, *this, bDeferred);
		}
		catch (DataManager::ErrNeedDataManager) {
			silent_cerr("the required drive caller must appear "
					"inside or after the \"control data\" "
					"block"
					<< std::endl);
			throw DataManager::ErrNeedDataManager();
		}
		return pDC;
	}

	unsigned int uLabel = GetInt();
	DCType::const_iterator i = DC.find(uLabel);
	if (i == DC.end()) {
		silent_cerr("drive caller " << uLabel
				<< " is undefined at line "
				<< GetLineData() << std::endl);
		throw MBDynParser::ErrGeneric();
	}

	return i->second->pCopy();
}

const BasicScalarFunction *
MBDynParser::GetScalarFunction(void)
{
	if (!IsKeyWord("reference")) {
		return ParseScalarFunction(*this, pDM);
	}

	std::string s(GetStringWithDelims());
	return GetScalarFunction(s);
}

const BasicScalarFunction *
MBDynParser::GetScalarFunction(const std::string &s)
{
	SFType::const_iterator i = SF.find(s);
	if (i != SF.end()) {
		return i->second;
	}
	return 0;
}

bool
MBDynParser::SetScalarFunction(const std::string &s, const BasicScalarFunction *sf)
{
	return SF.insert(SFType::value_type(s, sf)).second;
}

/* MBDynParser - end */

