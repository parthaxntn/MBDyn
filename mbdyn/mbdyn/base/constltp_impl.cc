/* $Header$ */
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

/* Legami costitutivi */

#ifdef HAVE_CONFIG_H
#include "mbconfig.h"           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include "myassert.h"
#include "mynewmem.h"
#include "constltp_impl.h"

#include "symcltp.h"
#include "ginaccltp.h"
#ifdef USE_GRAALLDAMPER
#include "damper.h"
#endif /* USE_GRAALLDAMPER */
#include "shockabsorber.h"
#include "constltp_ann.h"

/* constitutive laws sponsored by Hutchinson CdR */
#include "constltp_nlp.h"
#include "constltp_nlsf.h"

/* used by invariant constitutive law... */
#include "vehj.h"

/* constitutive law containers */
typedef std::map<std::string, ConstitutiveLawRead<doublereal, doublereal> *, ltstrcase> CL1DFuncMapType;
typedef std::map<std::string, ConstitutiveLawRead<Vec3, Mat3x3> *, ltstrcase> CL3DFuncMapType;
typedef std::map<std::string, ConstitutiveLawRead<Vec6, Mat6x6> *, ltstrcase> CL6DFuncMapType;

static CL1DFuncMapType CL1DFuncMap;
static CL3DFuncMapType CL3DFuncMap;
static CL6DFuncMapType CL6DFuncMap;

/* constitutive law parsing checkers */
struct CL1DWordSetType : public HighParser::WordSet {
	bool IsWord(const std::string& s) const {
		return CL1DFuncMap.find(std::string(s)) != CL1DFuncMap.end();
	};
};

struct CL3DWordSetType : public HighParser::WordSet {
	bool IsWord(const std::string& s) const {
		return CL3DFuncMap.find(std::string(s)) != CL3DFuncMap.end();
	};
};

struct CL6DWordSetType : public HighParser::WordSet {
	bool IsWord(const std::string& s) const {
		return CL6DFuncMap.find(std::string(s)) != CL6DFuncMap.end();
	};
};

static CL1DWordSetType CL1DWordSet;
static CL3DWordSetType CL3DWordSet;
static CL6DWordSetType CL6DWordSet;

/* constitutive law registration functions: call to register one */
bool
SetCL1D(const char *name, ConstitutiveLawRead<doublereal, doublereal> *rf)
{
	pedantic_cout("registering constitutive law 1D \"" << name << "\""
		<< std::endl );
	return CL1DFuncMap.insert(CL1DFuncMapType::value_type(name, rf)).second;
}

bool
SetCL3D(const char *name, ConstitutiveLawRead<Vec3, Mat3x3> *rf)
{
	pedantic_cout("registering constitutive law 3D \"" << name << "\""
		<< std::endl );
	return CL3DFuncMap.insert(CL3DFuncMapType::value_type(name, rf)).second;
}

bool
SetCL6D(const char *name, ConstitutiveLawRead<Vec6, Mat6x6> *rf)
{
	pedantic_cout("registering constitutive law 6D \"" << name << "\""
		<< std::endl );
	return CL6DFuncMap.insert(CL6DFuncMapType::value_type(name, rf)).second;
}

/* function that reads a constitutive law */
ConstitutiveLaw<doublereal, doublereal> *
ReadCL1D(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType)
{
	const char *s, *sOrig = HP.IsWord(CL1DWordSet);
	if (sOrig == 0) {
		/* default to linear elastic? */
		s = "linear" "elastic";
		sOrig = "";

	} else {
		s = sOrig;
	}

	CL1DFuncMapType::iterator func = CL1DFuncMap.find(std::string(s));
	if (func == CL1DFuncMap.end()) {
		silent_cerr("unknown constitutive law 1D type "
			"\"" << sOrig << "\" "
			"at line " << HP.GetLineData() << std::endl);
		throw DataManager::ErrGeneric();
	}

	return func->second->Read(pDM, HP, CLType);
}

ConstitutiveLaw<Vec3, Mat3x3> *
ReadCL3D(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType)
{
	const char *s, *sOrig = HP.IsWord(CL3DWordSet);
	if (sOrig == 0) {
#if 0
		s = "linear" "elastic";
		sOrig = "";
#else
		silent_cerr("unknown constitutive law 3D type "
			"at line " << HP.GetLineData() << std::endl);
		throw DataManager::ErrGeneric();
#endif

	} else {
		s = sOrig;
	}

	CL3DFuncMapType::iterator func = CL3DFuncMap.find(std::string(s));
	if (func == CL3DFuncMap.end()) {
		silent_cerr("unknown constitutive law 3D type "
			"\"" << sOrig << "\" "
			"at line " << HP.GetLineData() << std::endl);
		throw DataManager::ErrGeneric();
	}

	return func->second->Read(pDM, HP, CLType);
}

ConstitutiveLaw<Vec6, Mat6x6> *
ReadCL6D(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType)
{
	const char *s, *sOrig = HP.IsWord(CL6DWordSet);
	if (sOrig == 0) {
#if 0
		s = "linear" "elastic";
		sOrig = "";
#else
		silent_cerr("unknown constitutive law 6D type "
			"at line " << HP.GetLineData() << std::endl);
		throw DataManager::ErrGeneric();
#endif

	} else {
		s = sOrig;
	}

	CL6DFuncMapType::iterator func = CL6DFuncMap.find(std::string(s));
	if (func == CL6DFuncMap.end()) {
		silent_cerr("unknown constitutive law 6D type "
			"\"" << sOrig << "\" "
			"at line " << HP.GetLineData() << std::endl);
		throw DataManager::ErrGeneric();
	}

	return func->second->Read(pDM, HP, CLType);
}

/* specific functional object(s) */
template <class T, class Tder>
struct LinearElasticCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::ELASTIC;

		doublereal dS = HP.GetReal();
		DEBUGCOUT("Linear Elastic Isotropic Constitutive Law, stiffness = "
				<< dS << std::endl);

		if (dS <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef LinearElasticIsotropicConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, dS));

		return pCL;
	};
};

template <class T, class Tder>
struct LinearElasticGenericCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::ELASTIC;

		DEBUGCOUT("Linear Elastic Generic Constitutive Law" << std::endl);
		Tder S(0.);
		S = HP.Get(S);

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef LinearElasticGenericConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, S));

		return pCL;
	};
};

template <class T, class Tder>
struct LinearElasticGenericAxialTorsionCouplingCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::ELASTIC;

		DEBUGCOUT("Linear Elastic Generic Constitutive Law with Axial-Torsion Coupling" << std::endl);
		Tder S(0.);
		S = HP.Get(S);

		/* coefficiente di accoppiamento */
		doublereal dCoupl = HP.GetReal();
		DEBUGCOUT("coupling coefficient: " << dCoupl << std::endl);

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef LinearElasticGenericAxialTorsionCouplingConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, S, dCoupl));

		return pCL;
	};
};

template <class T, class Tder>
struct LinearViscoElasticGenericAxialTorsionCouplingCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOELASTIC;

		Tder S(0.);
		S = HP.Get(S);

		Tder SP(0.);
		if (HP.IsKeyWord("proportional")) {
			doublereal k = HP.GetReal();
			SP = S*k;
		} else {
			SP = HP.Get(SP);
		}

		/* coefficiente di accoppiamento */
		doublereal dCoupl = HP.GetReal();
		DEBUGCOUT("coupling coefficient: " << dCoupl << std::endl);

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef LinearViscoElasticGenericAxialTorsionCouplingConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, S, SP, dCoupl));

		return pCL;
	};
};

template <class T, class Tder>
struct LogElasticCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::ELASTIC;

		DEBUGCOUT("Logarithmic Elastic Constitutive Law" << std::endl);
		doublereal dS = HP.GetReal();
		if (dS <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef LogConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, dS));

		return pCL;
	};
};

template <class T, class Tder>
struct DoubleLinearElasticCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::ELASTIC;

		doublereal dS = HP.GetReal();
		DEBUGCOUT("stiffness = " << dS << std::endl);

		if (dS <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dUpp = HP.GetReal();
		if (dUpp <= 0.) {
			silent_cerr("warning, null or negative upper limit strain at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dLow = HP.GetReal();
		if (dLow >= 0.) {
			silent_cerr("warning, null or positive lower limit strain at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dSecondS = HP.GetReal();
		if (dSecondS <= 0.) {
			silent_cerr("warning, null or negative second stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef DoubleLinearElasticConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL,
				L,
				L(pTplDC, PreStress, dS, dUpp, dLow, dSecondS));

		return pCL;
	};
};

template <class T, class Tder>
struct IsotropicHardeningCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::ELASTIC;

		doublereal dS = HP.GetReal();
		DEBUGCOUT("Stiffness = " << dS << std::endl);

		if (dS <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dE = HP.GetReal();
		DEBUGCOUT("Reference strain = " << dE << std::endl);

		if (dE <= 0.) {
			silent_cerr("error, null or negative reference strain at line "
				<< HP.GetLineData() << std::endl);
			throw DataManager::ErrGeneric();
		}

		doublereal dS0 = 0.;
		if (HP.IsKeyWord("linear" "stiffness")) {
			dS0 = HP.GetReal();
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef IsotropicHardeningConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, dS, dS0, dE));

		return pCL;
	};
};

template <class T, class Tder>
struct ContactElasticCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::ELASTIC;

		doublereal dK = HP.GetReal();
		DEBUGCOUT("Stiffness = " << dK << std::endl);

		if (dK <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dGamma = HP.GetReal();
		DEBUGCOUT("Exponent = " << dGamma << std::endl);

		if (dGamma < 1.) {
			silent_cerr("error, exponent < 1. at line "
				<< HP.GetLineData() << std::endl);
			throw DataManager::ErrGeneric();
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef ContactConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, dK, dGamma));

		return pCL;
	};
};

template <class T, class Tder>
struct SymbolicCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		unsigned dim;
		if (typeid(T) == typeid(doublereal)) {
			dim = 1;

		} else if (typeid(T) == typeid(Vec3)) {
			dim = 3;

		} else if (typeid(T) == typeid(Vec6)) {
			dim = 6;

		} else {
			silent_cerr("Invalid dimensionality "
				"for symbolic constitutive law "
				"at line " << HP.GetLineData()
				<< std::endl);
			throw DataManager::ErrGeneric();
		}

		std::vector<std::string> epsilon;
		if (CLType & ConstLawType::ELASTIC) {
			if (!HP.IsKeyWord("epsilon")) {
				silent_cerr("keyword \"epsilon\" expected at line " << HP.GetLineData() << std::endl);
				throw DataManager::ErrGeneric();
			}

			epsilon.resize(dim);

			for (unsigned row = 0; row < dim; row++) {
				const char *tmp = HP.GetStringWithDelims();

				if (tmp == 0) {
					silent_cerr("unable to get \"epsilon\" "
						"symbol #" << row << " "
						"at line " << HP.GetLineData() << std::endl);
					throw DataManager::ErrGeneric();
				}
				epsilon[row] = tmp;
			}
		}

		std::vector<std::string> epsilonPrime;
		if (CLType & ConstLawType::VISCOUS) {
			if (!HP.IsKeyWord("epsilon" "prime")) {
				silent_cerr("keyword \"epsilon prime\" expected at line " << HP.GetLineData() << std::endl);
				throw DataManager::ErrGeneric();
			}

			epsilonPrime.resize(dim);

			for (unsigned row = 0; row < dim; row++) {
				const char *tmp = HP.GetStringWithDelims();

				if (tmp == 0) {
					silent_cerr("unable to get \"epsilonPrime\" "
						"symbol #" << row << " "
						"at line " << HP.GetLineData() << std::endl);
					throw DataManager::ErrGeneric();
				}
				epsilonPrime[row] = tmp;
			}
		}

		if (!HP.IsKeyWord("expression")) {
			silent_cerr("keyword \"expression\" expected at line " << HP.GetLineData() << std::endl);
			throw DataManager::ErrGeneric();
		}

		std::vector<std::string> expression(dim);
		for (unsigned row = 0; row < dim; row++) {
			const char *tmp = HP.GetStringWithDelims();
			if (tmp == 0) {
				silent_cerr("unable to get \"expression\" "
					"#" << row << " "
					"at line " << HP.GetLineData()
					<< std::endl);
				throw DataManager::ErrGeneric();
			}
			expression[row] = tmp;
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		switch (CLType) {
		case ConstLawType::ELASTIC: {
#ifdef HAVE_GINAC
			typedef GiNaCElasticConstitutiveLaw<T, Tder> L;
			SAFENEWWITHCONSTRUCTOR(pCL, L,
					L(pTplDC, PreStress,
						epsilon,
						expression));
#else /* ! HAVE_GINAC */
			silent_cerr("symbolic constitutive law not supported "
				"at line " << HP.GetLineData() << std::endl);
			throw DataManager::ErrGeneric();
#endif /* ! HAVE_GINAC */
			break;
		}

		case ConstLawType::VISCOUS: {
#ifdef HAVE_GINAC
			typedef GiNaCViscousConstitutiveLaw<T, Tder> L;
			SAFENEWWITHCONSTRUCTOR(pCL, L,
					L(pTplDC, PreStress,
						epsilonPrime,
						expression));
#else /* ! HAVE_GINAC */
			silent_cerr("symbolic constitutive law not supported "
				"at line " << HP.GetLineData() << std::endl);
			throw DataManager::ErrGeneric();
#endif /* ! HAVE_GINAC */
			break;
		}

		case ConstLawType::VISCOELASTIC: {
#ifdef HAVE_GINAC
			typedef GiNaCViscoElasticConstitutiveLaw<T, Tder> L;
			SAFENEWWITHCONSTRUCTOR(pCL, L,
					L(pTplDC, PreStress,
						epsilon, epsilonPrime,
						expression));
#else /* ! HAVE_GINAC */
			silent_cerr("symbolic constitutive law not supported "
				"at line " << HP.GetLineData() << std::endl);
			throw DataManager::ErrGeneric();
#endif /* ! HAVE_GINAC */
			break;
		}

		default:
			ASSERT(0);
			throw ErrGeneric();
		}

		return pCL;
	};
};

template <class T, class Tder>
struct SymbolicElasticCLR : public SymbolicCLR<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		CLType = ConstLawType::ELASTIC;
		return SymbolicCLR<T, Tder>::Read(pDM, HP, CLType);
	};
};

template <class T, class Tder>
struct SymbolicViscousCLR : public SymbolicCLR<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		CLType = ConstLawType::VISCOUS;
		return SymbolicCLR<T, Tder>::Read(pDM, HP, CLType);
	};
};

template <class T, class Tder>
struct SymbolicViscoElasticCLR : public SymbolicCLR<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		CLType = ConstLawType::VISCOELASTIC;
		return SymbolicCLR<T, Tder>::Read(pDM, HP, CLType);
	};
};

template <class T, class Tder>
struct LinearViscousCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOUS;

		doublereal dSP = HP.GetReal();
		DEBUGCOUT("stiffness prime = " << dSP << std::endl);

		if (dSP <= 0.) {
			silent_cerr("warning, null or negative stiffness prime at line "
				<< HP.GetLineData() << std::endl);
		}

		/* Prestress (no prestrain) */
		T PreStress(0.);
		GetPreStress(HP, PreStress);

		typedef LinearViscousIsotropicConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(NULL, PreStress, dSP));

		return pCL;
	};
};

template <class T, class Tder>
struct LinearViscousGenericCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOUS;

		Tder SP(0.);
		SP = HP.Get(SP);

		/* Prestress (no prestrain) */
		T PreStress(0.);
		GetPreStress(HP, PreStress);

		typedef LinearViscousGenericConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(NULL, PreStress, SP));

		return pCL;
	};
};

template <class T, class Tder>
struct LinearViscoElasticCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOELASTIC;

		doublereal dS = HP.GetReal();
		DEBUGCOUT("Stiffness = " << dS << std::endl);

		if (dS <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dSP = 0.;
		if (HP.IsKeyWord("proportional")) {
			doublereal k = HP.GetReal();
			dSP = k*dS;
		} else {
			dSP = HP.GetReal();
		}
		DEBUGCOUT("stiffness prime = " << dSP << std::endl);

		if (dSP <= 0.) {
			silent_cerr("warning, null or negative stiffness prime at line "
				<< HP.GetLineData() << std::endl);
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef LinearViscoElasticIsotropicConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, dS, dSP));

		return pCL;
	};
};

template <class T, class Tder>
struct LinearViscoElasticGenericCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOELASTIC;

		Tder S(0.);
		S = HP.Get(S);

		Tder SP(0.);
		if (HP.IsKeyWord("proportional")) {
			doublereal k = HP.GetReal();
			SP = S*k;
		} else {
			SP = HP.Get(SP);
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef LinearViscoElasticGenericConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, S, SP));

		return pCL;
	};
};

template <class T, class Tder>
struct CubicElasticGenericCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::ELASTIC;

		T S1(0.);
		S1 = HP.Get(S1);

		T S2(0.);
		S2 = HP.Get(S2);

		T S3(0.);
		S3 = HP.Get(S3);

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef CubicElasticGenericConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, S1, S2, S3));

		return pCL;
	};
};

template <class T, class Tder>
struct CubicViscoElasticGenericCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOELASTIC;

		T S1(0.);
		S1 = HP.Get(S1);

		T S2(0.);
		S2 = HP.Get(S2);

		T S3(0.);
		S3 = HP.Get(S3);

		Tder SP(0.);
		SP = HP.Get(SP);

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef CubicViscoElasticGenericConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, S1, S2, S3, SP));

		return pCL;
	};
};

template <class T, class Tder>
struct DoubleLinearViscoElasticCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOELASTIC;

		doublereal dS = HP.GetReal();
		DEBUGCOUT("stiffness = " << dS << std::endl);

		if (dS <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dUpp = HP.GetReal();
		if (dUpp <= 0.) {
			silent_cerr("warning, null or negative upper limit strain at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dLow = HP.GetReal();
		if (dLow >= 0.) {
			silent_cerr("warning, null or positive lower limit strain at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dSecondS = HP.GetReal();
		if (dSecondS <= 0.) {
			silent_cerr("warning, null or negative second stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dSP = HP.GetReal();
		DEBUGCOUT("stiffness prime = " << dSP << std::endl);

		if (dSP <= 0.) {
			silent_cerr("warning, null or negative stiffness prime at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dSecondSP = dSP;
		if (HP.IsKeyWord("second" "damping")) {
			dSecondSP = HP.GetReal();
			DEBUGCOUT("second stiffness prime = " << dSecondSP << std::endl);

			if (dSecondSP <= 0.) {
				silent_cerr("warning, null or negative second stiffness prime at line "
					<< HP.GetLineData() << std::endl);
			}
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef DoubleLinearViscoElasticConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL,
				L,
				L(pTplDC, PreStress,
					dS, dUpp, dLow, dSecondS, dSP, dSecondSP));

		return pCL;
	};
};

template <class T, class Tder>
struct TurbulentViscoElasticCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOELASTIC;

		doublereal dS = HP.GetReal();
		DEBUGCOUT("Visco-Elastic Turbulent Rod Joint, stiffness = "
				<< dS << std::endl);

		if (dS <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dParabStiff = HP.GetReal();
		DEBUGCOUT("stiffness prime = " << dParabStiff << std::endl);

		if (dParabStiff <= 0.) {
			silent_cerr("warning, null or negative derivative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dTreshold = 0.;
		if (HP.IsArg()) {
			dTreshold = HP.GetReal(dTreshold);

			/*
			 * Il legame costitutivo ha la forma seguente:
			 *	F = Kp*e + Kd*(de/dt)
			 * con Kp costante e Kd dato dalla seguente legge:
			 *	Kd = cost2                per fabs(de/dt) < Treshold
			 *	Kd = 2*cost1*fabs(de/dt)  per fabs(de/dt) > Treshold
			 * se non viene inserito il valore di treshold, lo si
			 * assume = 0. e quindi il legame e' sempre del secondo
			 * tipo. Altrimenti, se non viene inserita la seconda
			 * costante cost2, si assume che vi sia raccordo tra
			 * i due tipi di legge, ovvero cost2 = cost1*Treshold
			 * altrimenti e' possibile avere un comportamento,
			 * che in prima approssimazione e' valido
			 * per numerosi fluidi, in cui vi e' un salto tra i due
			 * tipi di legge costitutiva. */
		}

		doublereal dSP = dTreshold*dParabStiff;
		if (HP.IsArg()) {
			dSP = HP.GetReal(dSP);
		}

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef TurbulentViscoElasticConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL,
				L,
				L(pTplDC, PreStress,
					dS, dSP, dTreshold, dParabStiff));

		return pCL;
	};
};

template <class T, class Tder>
struct LinearBiStopCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		typedef LinearViscoElasticBiStopConstitutiveLaw<T, Tder> L;

		DEBUGCOUT("Linear Viscoelastic Bi Stop Constitutive Law" << std::endl);
		doublereal dS = HP.GetReal();
		if (dS <= 0.) {
			silent_cerr("warning, null or negative stiffness at line "
				<< HP.GetLineData() << std::endl);
		}

		doublereal dSp = 0.;
		if (CLType == ConstLawType::VISCOELASTIC) {
			dSp = HP.GetReal();
			if (dSp <= 0.) {
				silent_cerr("warning, null or negative stiffness prime at line "
					<< HP.GetLineData() << std::endl);
			}
		}

		typedef typename LinearViscoElasticBiStopConstitutiveLaw<T, Tder>::Status LS;
		LS s = L::INACTIVE;
		if (HP.IsKeyWord("initial" "status")) {
			if (HP.IsKeyWord("active")) {
				s = L::ACTIVE;
			} else if (HP.IsKeyWord("inactive")) {
				s = L::INACTIVE;
			} else {
				silent_cerr("unknown initial status at line "
					<< HP.GetLineData() << std::endl);
				throw ErrGeneric();
			}
		}

		const DriveCaller *pA = HP.GetDriveCaller();
		const DriveCaller *pD = HP.GetDriveCaller();

		/* Prestress and prestrain */
		T PreStress(0.);
		GetPreStress(HP, PreStress);
		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pTplDC, PreStress, dS, dSp, s, pA, pD));

		return pCL;
	};
};

template <class T, class Tder>
struct LinearElasticBiStopCLR : public LinearBiStopCLR<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		CLType = ConstLawType::ELASTIC;
		return LinearBiStopCLR<T, Tder>::Read(pDM, HP, CLType);
	};
};

template <class T, class Tder>
struct LinearViscoElasticBiStopCLR : public LinearBiStopCLR<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		CLType = ConstLawType::VISCOELASTIC;
		return LinearBiStopCLR<T, Tder>::Read(pDM, HP, CLType);
	};
};

#ifdef USE_GRAALLDAMPER
template <class T, class Tder>
struct GRAALLDamperCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOELASTIC;

		const char* filename = HP.GetFileName();
		DEBUGCOUT("Graall damper input file: \""
				<< filename << "\"" << std::endl);

		doublereal rla = HP.GetReal();
		DEBUGCOUT("Reference length: " << rla << std::endl);

		DriveCaller* pDC = NULL;
		SAFENEWWITHCONSTRUCTOR(pDC,
				TimeDriveCaller,
				TimeDriveCaller(pDM->pGetDrvHdl()));

		T t(1.);
		TplDriveCaller<T>* pTplDC = NULL;
		SAFENEWWITHCONSTRUCTOR(pTplDC,
				SingleTplDriveCaller<T>,
				SingleTplDriveCaller<T>(pDC, t));

		typedef GRAALLDamperConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL,
				L,
				L(pTplDC, rla, filename));

		return pCL;
	};
};
#endif /* USE_GRAALLDAMPER */

/*
 * Shock absorber per Stefy:
 *
 * ``Riprogettazione dell'ammortizzatore del carrello anteriore
 * di un velivolo di aviazione generale'',
 * S. Carlucci e S. Gualdi,
 * A.A. 1997-98
 */
template <class T, class Tder>
struct ShockAbsorberCLR : public ConstitutiveLawRead<T, Tder> {
	virtual ConstitutiveLaw<T, Tder> *
	Read(const DataManager* pDM, MBDynParser& HP, ConstLawType::Type& CLType) {
		ConstitutiveLaw<T, Tder>* pCL = 0;

		CLType = ConstLawType::VISCOELASTIC;

		T PreStrain(0.);
		TplDriveCaller<T>* pTplDC = GetPreStrain(pDM, HP, PreStrain);

		typedef ShockAbsorberConstitutiveLaw<T, Tder> L;
		SAFENEWWITHCONSTRUCTOR(pCL, L, L(pDM, pTplDC, HP));

		return pCL;
	};
};

static unsigned done = 0;

/* initialization function */
void
InitCL(void)
{
	if (::done++ > 0) {
		return;
	}

	/* linear elastic */
	SetCL1D("linear" "elastic", new LinearElasticCLR<doublereal, doublereal>);
	SetCL3D("linear" "elastic", new LinearElasticCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "elastic", new LinearElasticCLR<Vec6, Mat6x6>);

	/* linear elastic isotropic */
	SetCL1D("linear" "elastic" "isotropic", new LinearElasticCLR<doublereal, doublereal>);
	SetCL3D("linear" "elastic" "isotropic", new LinearElasticCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "elastic" "isotropic", new LinearElasticCLR<Vec6, Mat6x6>);

	/* linear elastic generic */
	SetCL1D("linear" "elastic" "generic", new LinearElasticGenericCLR<doublereal, doublereal>);
	SetCL3D("linear" "elastic" "generic", new LinearElasticGenericCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "elastic" "generic", new LinearElasticGenericCLR<Vec6, Mat6x6>);

	/* linear (visco)elastic generic axial torsion coupling*/
	SetCL6D("linear" "elastic" "generic" "axial" "torsion" "coupling",
		new LinearElasticGenericAxialTorsionCouplingCLR<Vec6, Mat6x6>);
	SetCL6D("linear" "viscoelastic" "generic" "axial" "torsion" "coupling",
		new LinearViscoElasticGenericAxialTorsionCouplingCLR<Vec6, Mat6x6>);

	/* log elastic */
	SetCL1D("log" "elastic", new LogElasticCLR<doublereal, doublereal>);

	/* double linear elastic */
	SetCL1D("double" "linear" "elastic", new DoubleLinearElasticCLR<doublereal, doublereal>);
	SetCL3D("double" "linear" "elastic", new DoubleLinearElasticCLR<Vec3, Mat3x3>);

	/* isotropic hardening elastic */
	SetCL1D("isotropic" "hardening" "elastic", new IsotropicHardeningCLR<doublereal, doublereal>);
	SetCL3D("isotropic" "hardening" "elastic", new IsotropicHardeningCLR<Vec3, Mat3x3>);
	SetCL6D("isotropic" "hardening" "elastic", new IsotropicHardeningCLR<Vec6, Mat6x6>);

	/* contact elastic */
	SetCL1D("contact" "elastic", new ContactElasticCLR<doublereal, doublereal>);
	SetCL3D("contact" "elastic", new ContactElasticCLR<Vec3, Mat3x3>);

	/* symbolic (elastic, viscous, viscoelastic) */
	SetCL1D("symbolic", new SymbolicCLR<doublereal, doublereal>);
	SetCL3D("symbolic", new SymbolicCLR<Vec3, Mat3x3>);
	SetCL6D("symbolic", new SymbolicCLR<Vec6, Mat6x6>);

	SetCL1D("symbolic" "elastic", new SymbolicElasticCLR<doublereal, doublereal>);
	SetCL3D("symbolic" "elastic", new SymbolicElasticCLR<Vec3, Mat3x3>);
	SetCL6D("symbolic" "elastic", new SymbolicElasticCLR<Vec6, Mat6x6>);

	SetCL1D("symbolic" "viscous", new SymbolicViscousCLR<doublereal, doublereal>);
	SetCL3D("symbolic" "viscous", new SymbolicViscousCLR<Vec3, Mat3x3>);
	SetCL6D("symbolic" "viscous", new SymbolicViscousCLR<Vec6, Mat6x6>);

	SetCL1D("symbolic" "viscoelastic", new SymbolicViscoElasticCLR<doublereal, doublereal>);
	SetCL3D("symbolic" "viscoelastic", new SymbolicViscoElasticCLR<Vec3, Mat3x3>);
	SetCL6D("symbolic" "viscoelastic", new SymbolicViscoElasticCLR<Vec6, Mat6x6>);

	/* linear viscous */
	SetCL1D("linear" "viscous", new LinearViscousCLR<doublereal, doublereal>);
	SetCL3D("linear" "viscous", new LinearViscousCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "viscous", new LinearViscousCLR<Vec6, Mat6x6>);

	/* linear viscous isotropic */
	SetCL1D("linear" "viscous" "isotropic", new LinearViscousCLR<doublereal, doublereal>);
	SetCL3D("linear" "viscous" "isotropic", new LinearViscousCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "viscous" "isotropic", new LinearViscousCLR<Vec6, Mat6x6>);

	/* linear viscous generic */
	SetCL1D("linear" "viscous" "generic", new LinearViscousGenericCLR<doublereal, doublereal>);
	SetCL3D("linear" "viscous" "generic", new LinearViscousGenericCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "viscous" "generic", new LinearViscousGenericCLR<Vec6, Mat6x6>);

	/* linear viscoelastic */
	SetCL1D("linear" "viscoelastic", new LinearViscoElasticCLR<doublereal, doublereal>);
	SetCL3D("linear" "viscoelastic", new LinearViscoElasticCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "viscoelastic", new LinearViscoElasticCLR<Vec6, Mat6x6>);

	/* linear viscoelastic isotropic */
	SetCL1D("linear" "viscoelastic" "isotropic", new LinearViscoElasticCLR<doublereal, doublereal>);
	SetCL3D("linear" "viscoelastic" "isotropic", new LinearViscoElasticCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "viscoelastic" "isotropic", new LinearViscoElasticCLR<Vec6, Mat6x6>);

	/* linear viscoelastic generic */
	SetCL1D("linear" "viscoelastic" "generic", new LinearViscoElasticGenericCLR<doublereal, doublereal>);
	SetCL3D("linear" "viscoelastic" "generic", new LinearViscoElasticGenericCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "viscoelastic" "generic", new LinearViscoElasticGenericCLR<Vec6, Mat6x6>);

	/* cubic elastic generic */
	SetCL1D("cubic" "elastic" "generic", new CubicElasticGenericCLR<doublereal, doublereal>);
	SetCL3D("cubic" "elastic" "generic", new CubicElasticGenericCLR<Vec3, Mat3x3>);

	/* cubic viscoelastic generic */
	SetCL1D("cubic" "viscoelastic" "generic", new CubicViscoElasticGenericCLR<doublereal, doublereal>);
	SetCL3D("cubic" "viscoelastic" "generic", new CubicViscoElasticGenericCLR<Vec3, Mat3x3>);

	/* double linear viscoelastic */
	SetCL1D("double" "linear" "viscoelastic", new DoubleLinearViscoElasticCLR<doublereal, doublereal>);
	SetCL3D("double" "linear" "viscoelastic", new DoubleLinearViscoElasticCLR<Vec3, Mat3x3>);

	/* turbulent viscoelastic */
	SetCL1D("turbulent" "viscoelastic", new TurbulentViscoElasticCLR<doublereal, doublereal>);

	/* linear elastic bistop */
	SetCL1D("linear" "elastic" "bistop", new LinearElasticBiStopCLR<doublereal, doublereal>);
	SetCL3D("linear" "elastic" "bistop", new LinearElasticBiStopCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "elastic" "bistop", new LinearElasticBiStopCLR<Vec6, Mat6x6>);

	/* linear viscoelastic bistop */
	SetCL1D("linear" "viscoelastic" "bistop", new LinearViscoElasticBiStopCLR<doublereal, doublereal>);
	SetCL3D("linear" "viscoelastic" "bistop", new LinearViscoElasticBiStopCLR<Vec3, Mat3x3>);
	SetCL6D("linear" "viscoelastic" "bistop", new LinearViscoElasticBiStopCLR<Vec6, Mat6x6>);

#ifdef USE_GRAALLDAMPER
	/* GRAALL damper */
	SetCL1D("GRAALL" "damper", new GRAALLDamperCLR<doublereal, doublereal>);
#endif /* USE_GRAALLDAMPER */

	/* GRAALL damper */
	SetCL1D("shock" "absorber", new ShockAbsorberCLR<doublereal, doublereal>);

	/* Artificial Neural Network */
	SetCL1D("ann" "elastic", new AnnElasticCLR<doublereal, doublereal>);
	SetCL1D("ann" "viscoelastic", new AnnViscoElasticCLR<doublereal, doublereal>);

	/* constitutive laws sponsored by Hutchinson CdR */
	NLP_init();
	NLSF_init();

	/* invariant constitutive law */
	SetCL3D("invariant" "angular", new InvAngularCLR);

	/* NOTE: add here initialization of new built-in constitutive laws;
	 * alternative ways to register new custom constitutive laws are:
	 * - call SetCL*D() from anywhere in the code
	 * - write a module that calls SetCL*D() from inside a function
	 *   called module_init(), and load it using "module load".
	 */
}

void
DestroyCL(void)
{
	if (::done == 0) {
		silent_cerr("DestroyCL() called once too many" << std::endl);
		throw ErrGeneric();
	}

	if (--::done > 0) {
		return;
	}

	/* free stuff */
	for (CL1DFuncMapType::iterator i = CL1DFuncMap.begin(); i != CL1DFuncMap.end(); i++) {
		delete i->second;
	}

	for (CL3DFuncMapType::iterator i = CL3DFuncMap.begin(); i != CL3DFuncMap.end(); i++) {
		delete i->second;
	}

	for (CL6DFuncMapType::iterator i = CL6DFuncMap.begin(); i != CL6DFuncMap.end(); i++) {
		delete i->second;
	}
}
