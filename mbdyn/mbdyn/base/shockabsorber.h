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

/*
 * Copyright (C) 1985-2003 GRAALL
 *
 * Gian Luca Ghiringhelli        <ghiringhelli@aero.polimi.it>
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 */

#ifndef SHOCKABSORBER_H
#define SHOCKABSORBER_H

#include "constltp.h"

const doublereal defaultEpsMin = -.5;
const doublereal defaultEpsMax = 0.;
const doublereal defaultPenalty = 1.e9;
const doublereal defaultPenaltyPrime = 0.e9;

/* ShockAbsorberConstitutiveLaw - begin */

template <class T, class Tder>
class ShockAbsorberConstitutiveLaw 
: public ElasticConstitutiveLaw<T, Tder> {
private:
   
public:
	ShockAbsorberConstitutiveLaw(
			DataManager* pDM,
			TplDriveCaller<T>* pDC,
			MBDynParser& HP
	) : ElasticConstitutiveLaw<T, Tder>(pDC, 0.) {
		THROW(ErrGeneric());
	};
   
	virtual
	~ShockAbsorberConstitutiveLaw(void) {
		NO_OP;
	};

	virtual ConstitutiveLaw<T, Tder>*
	pCopy(void) const {
		return NULL;
	};

	virtual std::ostream&
	Restart(std::ostream& out) const {
		return out
			<< "/* Shock absorber, not implemented yet */"
			<< std::endl;
	};

	virtual void
	Update(const T& /* Eps */ , const T& /* EpsPrime */  = 0.) {
		NO_OP;
	};

	virtual void
	IncrementalUpdate(
			const T& /* DeltaEps */ ,
			const T& /* EpsPrime */ = 0.
	) {
		NO_OP;
	};
};


class ShockAbsorberConstitutiveLaw<doublereal, doublereal> 
: public ElasticConstitutiveLaw<doublereal, doublereal> {
private:
	/*
	 * Il legame costitutivo scalare riceve in ingresso
	 * la "deformazione", ovvero la relazione
	 *
	 * 	           l - l0
	 * 	epsilon = --------
	 * 	             l0
	 * 
	 * Per comodita' dell'utente, si prende come lunghezza
	 * di riferimento la distanza tra i punti estremi
	 * dell'ammortizzatore, l0.
	 * Se si parte da una condizione diversa da completamente
	 * esteso, occorre aggiungere una deformazione iniziale.
	 *
	 * Il volume della camera e' calcolato come:
	 *
	 * 	 V = l0 * ( 1 + Cint * epsilon ) * A0
	 *
	 * dove Cint e' un coefficiente di interazione che lega la
	 * corsa esterna del pistone alla effettiva variazione di
	 * lunghezza della camera.
	 * Il rapporto tra i volumi iniziale e istantaneo e'
	 *
	 * 	 V0            1
	 * 	--- = -------------------
	 * 	 V     1 + Cint * epsilon
	 *
	 * La forza e' calcolata mediante una politropica:
	 *
	 * 		         V0
	 * 	F = A0 * P0 * ( ---- )^gamma
	 * 	                 V
	 */

	/*
	 * Parte elastica
	 */
	doublereal P0;
	doublereal A0;
	doublereal Cint;
	doublereal Gamma;

	/*
	 * Limiti e fine-corsa con penalty function
	 */
	doublereal EpsMax;
	doublereal EpsMin;
	doublereal Penalty;
	doublereal PenaltyPrime;
	doublereal FMax;
	doublereal FMin;

	/*
	 * Dissipazione
	 */
	DriveCaller *pAreaPin;
	DriveCaller *pAreaOrifices;

	doublereal AreaFluid;
	doublereal RhoFluid; /* FIXME: usare i fluidi idraulici? */
	doublereal Cd;

	doublereal EpsPrimeRef;
	doublereal FrictionAmpl;

	/*
	 * Dati necessari per output/priv data
	 */
	doublereal dPressure;
	doublereal dArea;
	doublereal dFelastic;
	doublereal dFviscous;

	/*
	 * Costruttore per pCopy
	 */
	ShockAbsorberConstitutiveLaw(
			const ShockAbsorberConstitutiveLaw* p
	) : ElasticConstitutiveLaw<doublereal, doublereal>(
		p->pGetDriveCaller()->pCopy(),
		0.
	) {
		P0 = p->P0;
		A0 = p->A0;
		Cint = p->Cint;
		Gamma = p->Gamma;

		EpsMax = p->EpsMax;
		EpsMin = p->EpsMin;
		Penalty = p->Penalty;
		PenaltyPrime = p->PenaltyPrime;
		FMax = p->FMax;
		FMin = p->FMin;

		pAreaPin = p->pAreaPin->pCopy();
		pAreaOrifices = p->pAreaOrifices->pCopy();

		AreaFluid = p->AreaFluid;
		RhoFluid = p->RhoFluid;
		Cd = p->Cd;

		EpsPrimeRef = p->EpsPrimeRef;
		FrictionAmpl = p->FrictionAmpl;
	};
   
public:
	ShockAbsorberConstitutiveLaw(
			DataManager* pDM,
			TplDriveCaller<doublereal>* pDC,
			MBDynParser& HP
	) : ElasticConstitutiveLaw<doublereal, doublereal>(pDC, 0.),
	EpsMax(defaultEpsMax), EpsMin(defaultEpsMin), 
	Penalty(defaultPenalty), PenaltyPrime(defaultPenaltyPrime),
	pAreaPin(NULL), pAreaOrifices(NULL),
	EpsPrimeRef(1.), FrictionAmpl(0.), dPressure(0.) {
		if (HP.IsKeyWord("help")) {

			std::cout <<
"\n"
"this help refers to the specific \"shock absorber\" constitutive\n"
"law input data.  The prestrain value, if required, must be inserted\n"
"at the beginning of the data.  The syntax is:\n"
"\n"
"\t[ prestrain , <value> , ]\n"
"\t<reference pressure> ,\n"
"\t<reference area for force computation> ,\n"
"\t<interaction coefficient (kinematic scale * ( L * A / V0 ) )> ,\n"
"\t<gamma (polytropic exponent)> ,\n"
"\t[ epsilon max , <upper strain bound, > prestrain; defaults to " << defaultEpsMax << ")> , ]\n"
"\t[ epsilon min , <lower strain bound, < prestrain; defaults to " << defaultEpsMin << ")> , ]\n"
"\t[ penalty , <penalty factor for strain bound enforcement, defaults to " << defaultPenalty << "> ,\n"
"\t\t<penalty factor for strain rate, active only when strain bounds are violated; defaults to " << defaultPenaltyPrime << "> ]\n"
"\t[ metering , <metering area (drive, strain dependent)> , ]\n"
"\t[ orifice , <orifice area (drive, strain rate dependent)> , ]\n"
"\t<fluid area> ,\n"
"\t<fluid density> ,\n"
"\t<drag coefficient / reference length (scales strain rate to velocity)>\n"
"\t[ , friction, <reference epsilon prime> ,\n"
"\t\t <friction amplitude coefficient> ] ;\n"
"\n"
"Note: at least one of \"metering\" and \"orifice\" must be defined.\n"
"Note: if 'friction' is enabled, the elastic force is multiplied\n"
"      by the factor\n"
"\n"
"\t\t1. - <friction amplitude coefficient> * tanh( <epsilon prime> / <reference epsilon prime> )\n"
"\n"
				<< std::endl;

			if (!HP.fIsArg()) {
				/*
				 * Exit quietly if nothing else is provided
				 */
				THROW(NoErr());
			}
		
		}
		
		dPressure = P0 = HP.GetReal();
		A0 = HP.GetReal();
		Cint = HP.GetReal();
		Gamma = HP.GetReal();

		if (HP.IsKeyWord("epsilon" "max")) {
			EpsMax = HP.GetReal(defaultEpsMax);
		}
		
		if (HP.IsKeyWord("epsilon" "min")) {
			EpsMin = HP.GetReal(defaultEpsMin);
			if ( EpsMin >= EpsMax) {
				std::cerr << "line " << HP.GetLineData()
					<< ": epsilon min must be less"
					" than epsilon max" << std::endl;
				THROW(ErrGeneric());
			}
		}

		if (HP.IsKeyWord("penalty")) {
			Penalty = HP.GetReal(defaultPenalty);
			PenaltyPrime = HP.GetReal(defaultPenaltyPrime);
		}

		if (HP.IsKeyWord("metering")) { 
			pAreaPin = ReadDriveData(pDM, HP, pDM->pGetDrvHdl());
		}

		if (HP.IsKeyWord("orifice")) {
			pAreaOrifices = ReadDriveData(pDM, HP, pDM->pGetDrvHdl());
		}

		if (pAreaPin == NULL && pAreaOrifices == NULL) {
			std::cerr << "line " << HP.GetLineData()
				<< ": at least one area (metering or orifice)"
				" must be defined" << std::endl;
			THROW(ErrGeneric());
		}

		AreaFluid = HP.GetReal();
		RhoFluid = HP.GetReal();
		Cd = HP.GetReal();

		if (HP.IsKeyWord("friction")) {
			EpsPrimeRef = HP.GetReal();
			if (EpsPrimeRef <= 0.) {
				std::cerr << "Illegal Reference Epsilon Prime " << EpsPrimeRef << " at line " << HP.GetLineData() << std::endl;
				THROW(ErrGeneric());
			}

			FrictionAmpl = HP.GetReal();
			if (FrictionAmpl < 0. || FrictionAmpl > 1.) {
				std::cerr << "Illegal Friction Amplitude Coefficient " << FrictionAmpl << " at line " << HP.GetLineData() << std::endl;
				THROW(ErrGeneric());
			}
		}

		Update(EpsMax, 0.);
		FMin = F;
		Update(EpsMin, 0.);
		FMax = F;
	};
	
	virtual
	~ShockAbsorberConstitutiveLaw(void) {
		NO_OP;
	};

	virtual ConstitutiveLaw<doublereal, doublereal>*
	pCopy(void) const {
		typedef ShockAbsorberConstitutiveLaw<doublereal, doublereal> L;

		L* p = NULL;
		SAFENEWWITHCONSTRUCTOR(p, L, L(this));
		return p;
	};

	virtual std::ostream&
	Restart(std::ostream& out) const {
		
		/*
		 * FIXME: devo trovare il modo di ripristinare
		 * le deformazioni iniziali senza grossi danni :)
		 */
		if (pGetDriveCaller()) {
			out
				<< ", prestrain, single, ",
				Write(out, -Epsilon, ", ") << ", one /* ",
				pGetDriveCaller()->Restart(out) << " */ , ";
		}
		
		/*
		 * dati sempre presenti
		 */
		out
			<< P0 << ", "
			<< A0 << ", "
			<< Cint << ", "
			<< Gamma << ", "
			<< "epsilon max, " << EpsMax << ", "
			<< "epsilon min, " << EpsMin << ", "
			<< "penalty, " << Penalty << ", " << PenaltyPrime;

		/*
		 * drive delle aree (solo quelli definiti)
		 */
		if (pAreaPin) {
			out
				<< ", metering, ",
				pAreaPin->Restart(out);
		}
		
		if (pAreaOrifices) {
			out
				<< ", orifice, ",
				pAreaOrifices->Restart(out);
		}

		/*
		 * dati parte viscosa
		 */
		out
			<< ", "
			<< AreaFluid << ", "
			<< RhoFluid << ", "
			<< Cd;
		
		return out;
	};

	virtual void
	Update(const doublereal& Eps, const doublereal& EpsPrime = 0.) {
		Epsilon = Eps;
		EpsilonPrime = EpsPrime;

		FDEPrime = 0.;

		/*
		 * Parte elastica
		 */

		doublereal CurrEpsilon = Epsilon-Get();
		doublereal VRatio = 1./(1.+Cint*CurrEpsilon);
		doublereal Adiab = pow(VRatio, Gamma);

		dPressure = P0*Adiab;

		if (CurrEpsilon > EpsMax) {
			F = FMin+Penalty*(CurrEpsilon-EpsMax)
				+PenaltyPrime*EpsPrime;
			FDE = Penalty;
			FDEPrime = PenaltyPrime;

			dFelastic = F;

		} else if (CurrEpsilon < EpsMin) {
			F = FMax + Penalty*(CurrEpsilon-EpsMin)
				+PenaltyPrime*EpsPrime;
			FDE = Penalty;
			FDEPrime = PenaltyPrime;

			dFelastic = F;

		} else {
			F = -A0*dPressure;

			dFelastic = F;

			if (FrictionAmpl != 0.) {
				F *= (1.-FrictionAmpl*tanh(EpsPrime/EpsPrimeRef));
			}

			FDE = Gamma*Cint*VRatio*F;
		}


		/*
		 * Parte viscosa
		 *
		 * FIXME: manca L0 per scalare correttamente la velocita'
		 * (basta metterla in Cd!!!)
		 */
		dArea = 0.;
		if (pAreaPin != NULL) {
			dArea += pAreaPin->dGet(CurrEpsilon);
		}
		
		if (pAreaOrifices != NULL) {
			dArea += pAreaOrifices->dGet(EpsPrime);
		}

		if (dArea <= 0.) {
			std::cerr << "ShockAbsorberConstitutiveLaw::Update:"
				" null or negative area" << std::endl;
			THROW(ErrGeneric());
		}
		
		doublereal d = .5*RhoFluid*AreaFluid*pow(AreaFluid/(dArea*Cd), 2);

		dFviscous = d*EpsPrime*fabs(EpsPrime);

		F += dFviscous;
		FDEPrime += d*fabs(EpsPrime);
	}

	virtual void
	IncrementalUpdate(
		const doublereal& DeltaEps,
		const doublereal& EpsPrime = 0.
	) {
		Update(Epsilon+DeltaEps, EpsPrime);
	};

	/*
	 * Metodi per l'estrazione di dati "privati".
	 * Si suppone che l'estrattore li sappia interpretare.
	 * Come default non ci sono dati privati estraibili
	 */
	virtual unsigned int iGetNumPrivData(void) const {
		/*
		 * deve essere pari al totale di dati che
		 * si intende esportare
		 */
		return 4;
	};

	/*
	 * Maps a string (possibly with substrings) to a private data;
	 * returns a valid index ( > 0 && <= iGetNumPrivData()) or 0 
	 * in case of unrecognized data; error must be handled by caller
	 */
	virtual unsigned int iGetPrivDataIdx(const char *s) const {
		ASSERT(s != NULL);

		if (strcmp(s, "p") == 0) {
			return 1;
		}

		if (strcmp(s, "A") == 0) {
			return 2;
		}

		if (strcmp(s, "Fe") == 0) {
			return 3;
		}

		if (strcmp(s, "Fv") == 0) {
			return 4;
		}

		/*
		 * aggiungere i nomi dei dati che si intende esportare
		 */

		/* error; handle later */
		return 0;
	};

	/*
	 * Returns the current value of a private data
	 * with 0 < i <= iGetNumPrivData()
	 */
	virtual doublereal dGetPrivData(unsigned int i) const {
		ASSERT(i > 0 && i <= iGetNumPrivData());

		switch (i) {
		case 1:
			return dPressure;

		case 2:
			return dArea;

		case 3:
			return dFelastic;

		case 4:
			return dFviscous;

		/* aggiungere ulteriori case */
		}

		THROW(ErrGeneric());
	};
};

/* ShockAbsorberConstitutiveLaw - begin */

#endif /* SHOCKABSORBER_H */

