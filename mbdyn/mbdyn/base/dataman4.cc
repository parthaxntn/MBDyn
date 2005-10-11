/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2005
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

/* Lettura elementi */

#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include <mbdefs.h>

#include <float.h>
#include <vector>
#include <set>

#include <dataman.h>
#include <dataman_.h>

/* Elementi */
#ifdef USE_STRUCT_NODES
#include <autostr.h>   /* Elementi automatici associati ai nodi dinamici */
#include <gravity.h>   /* Elemento accelerazione di gravita' */
#include <body.h>
#include "force.h"
#include "beam.h"
#include "beam2.h"
#include "hbeam.h"
#ifdef USE_AERODYNAMIC_ELEMS
#include <aerodyn.h>   /* Classe di base degli elementi aerodinamici */
#include "rotor.h"
#include "aeroelem.h"
#include "aeromodal.h"
#include <instruments.h>
#ifdef USE_EXTERNAL
#include <aeroext.h>
#endif /* USE_EXTERNAL */
#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_STRUCT_NODES */

#include <driven.h>    /* driven elements */
#include "genel.h"
#include <j2p.h>       /* bind di elementi a nodi parametrici */

#ifdef USE_HYDRAULIC_NODES
#include "preselem.h"
#endif /* USE_HYDRAULIC_NODES */

#ifdef USE_ELECTRIC_NODES
#include "elec.h"
#endif /* USE_ELECTRIC_NODES */

#include "bulk.h"

#include <drive.h>
#include <tpldrive.h>
#include <tpldrive_.h>

#include <loadable.h>

#ifdef USE_RTAI
#include <rtai_out_elem.h>
#endif /* USE_RTAI */

#include <socketstream_out_elem.h>

static int iNumTypes[Elem::LASTELEMTYPE];

/* enum delle parole chiave */
enum KeyWords {
   UNKNOWNKEYWORD = -1,
   END = 0,
   ELEMENTS,
   
   GRAVITY,
   BODY,
   AUTOMATICSTRUCTURAL,
   JOINT,
   COUPLE,
   BEAM3,
   BEAM,		/* same as BEAM3 */
   BEAM2,
   HBEAM,
   
   AIRPROPERTIES,
   ROTOR,
   AERODYNAMICBODY,
   AERODYNAMICBEAM3,
   AERODYNAMICBEAM,	/* same as AERODYNAMICBEAM3 */
   AERODYNAMICBEAM2,
   AERODYNAMICEXTERNAL,
   AERODYNAMICEXTERNALMODAL,
   AEROMODAL,
   AIRCRAFTINSTRUMENTS,
   
   FORCE,
   
   GENEL,
   ELECTRIC,
   
   HYDRAULIC,
   
   BULK,
   LOADABLE,
   DRIVEN,

   SOCKETSTREAM_OUTPUT,
   RTAI_OUTPUT,
   
   INERTIA,
   
   EXISTING,
   OUTPUT,
   BIND,

   LASTKEYWORD
};

void DataManager::ReadElems(MBDynParser& HP)
{
   DEBUGCOUTFNAME("DataManager::ReadElems");
   
   /* parole chiave del blocco degli elementi */
   const char* sKeyWords[] = { 
      "end",
      
      "elements",
      
      "gravity",
      "body",
      "automatic" "structural",
      "joint",
      "couple",
      "beam3",
      "beam",
      "beam2",
      "hbeam",
      
      "air" "properties",
      "rotor",
      "aerodynamic" "body",
      "aerodynamic" "beam3",
      "aerodynamic" "beam",
      "aerodynamic" "beam2",
      "aerodynamic" "external",
      "aerodynamic" "external" "modal",
      "aero" "modal",
      "aircraft" "instruments",
      
      "force",
      
      "genel",
      "electric",
      
      "hydraulic",
      
      "bulk",
      "loadable",
      "driven",

      // "socket" "stream" "output",	/* deprecated */
      "stream" "output",
      "RTAI" "output",

      "inertia",
      
      "existing",
      "output",
      "bind",

      NULL
   };
   
   /* tabella delle parole chiave */
   KeyTable K(HP, sKeyWords);
   
   /* strutture di conteggio degli elementi letti */
   for (int i = 0; i < Elem::LASTELEMTYPE; 
	iNumTypes[i] = ElemData[i].iNum, i++) { 
      NO_OP; 
   }   
         
   int iMissingElems = iTotElem;
   DEBUGLCOUT(MYDEBUG_INPUT, "Expected elements: " << iMissingElems << std::endl);
   
#ifdef USE_STRUCT_NODES
   /* Aggiunta degli elementi strutturali automatici legati ai nodi dinamici */
   if (ElemData[Elem::AUTOMATICSTRUCTURAL].iNum > 0) {	
      StructNode** ppTmpNod = 
	(StructNode**)NodeData[Node::STRUCTURAL].ppFirstNode;
      int iTotNod = NodeData[Node::STRUCTURAL].iNum;
      
      Elem** ppTmpEl = 
	ElemData[Elem::AUTOMATICSTRUCTURAL].ppFirstElem;
      for (StructNode** ppTmp = ppTmpNod; ppTmp < ppTmpNod+iTotNod; ppTmp++) {
	 if ((*ppTmp)->GetStructNodeType() == StructNode::DYNAMIC) {
	    
	    SAFENEWWITHCONSTRUCTOR(*ppTmpEl, AutomaticStructElem,
				   AutomaticStructElem((DynamicStructNode*)(*ppTmp)));
	    
	    ppTmpEl++;
	    iMissingElems--;
	    iNumTypes[Elem::AUTOMATICSTRUCTURAL]--;
	    DEBUGLCOUT(MYDEBUG_INPUT, 
		       "Initialising automatic structural element linked to node " 
		       << (*ppTmp)->GetLabel() << std::endl);
	 }
      }
   }
#endif /* USE_STRUCT_NODES */
   
   KeyWords CurrDesc;   
   while ((CurrDesc = KeyWords(HP.GetDescription())) != END) {
      
      if (CurrDesc == OUTPUT) {
	 DEBUGLCOUT(MYDEBUG_INPUT, "Elements to be output: ");
	 Elem::Type Typ;
	 switch (KeyWords(HP.GetWord())) {
	    
#ifdef USE_STRUCT_NODES
	  case BODY: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "bodies" << std::endl);
	     Typ = Elem::BODY;
	     break;
	  }
	  case AUTOMATICSTRUCTURAL: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "automatic structural" << std::endl);
	     Typ = Elem::AUTOMATICSTRUCTURAL;
	     break;
	  }
	  case JOINT: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "joints" << std::endl);
	     Typ = Elem::JOINT;
	     break;
	  }
	  case BEAM:
	  case BEAM3:			/* same as BEAM */
	  case BEAM2:
	  case HBEAM: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "beams" << std::endl);
	     Typ = Elem::BEAM;
	     break;
	  }
#ifdef USE_AERODYNAMIC_ELEMS
	  case ROTOR: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "rotors" << std::endl);
	     Typ = Elem::ROTOR;
	     break;
	  }
	  case AEROMODAL: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "aeromodal" << std::endl);
	     Typ = Elem::AEROMODAL;
	     break;
	  }
#ifdef USE_EXTERNAL
	  case AERODYNAMICEXTERNAL:
	  case AERODYNAMICEXTERNALMODAL: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "aerodynamic external" << std::endl);
	     Typ = Elem::EXTERNAL;
	     break;
	  }
#endif /* USE_EXTERNAL */
	  case AERODYNAMICBODY:
	  case AERODYNAMICBEAM:
	  case AERODYNAMICBEAM3:	/* same as AERODYNAMICBEAM */
	  case AERODYNAMICBEAM2:
	  case AIRCRAFTINSTRUMENTS: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "aerodynamic" << std::endl);
	     Typ = Elem::AERODYNAMIC;
	     break;
	  }
#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_STRUCT_NODES */

	  case FORCE:
#ifdef USE_STRUCT_NODES
	  case COUPLE:
#endif /* USE_STRUCT_NODES */

	  {		 
	     DEBUGLCOUT(MYDEBUG_INPUT, "forces" << std::endl);
	     Typ = Elem::FORCE;
	     break;
	  }	
#ifdef USE_ELECTRIC_NODES
	  case GENEL: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "genels" << std::endl);
	     Typ = Elem::GENEL;
	     break;
	  }
	  case ELECTRIC: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "electric" << std::endl);
	     Typ = Elem::ELECTRIC;
	     break;
	  }
#endif /* USE_ELECTRIC_NODES */

#ifdef USE_HYDRAULIC_NODES
	  case HYDRAULIC: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "hydraulic elements" << std::endl);
	     Typ = Elem::HYDRAULIC;
	     break;
	  }
#endif /* USE_HYDRAULIC_NODES */

	  case BULK: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "bulk" << std::endl);
	     Typ = Elem::BULK;
	     break;
	  }

	  case LOADABLE: {
	     DEBUGLCOUT(MYDEBUG_INPUT, "loadable" << std::endl);
	     Typ = Elem::LOADABLE;
	     break;
	  }

	  case UNKNOWNKEYWORD: {
	     silent_cerr("Error: unknown element type, cannot modify output"
			     << std::endl);
	     throw DataManager::ErrGeneric();
	  }
	    
	  default: {
	     silent_cerr("Error: element type " << sKeyWords[CurrDesc] 
	       << " at line " << HP.GetLineData() << " is not allowed"
	       << std::endl);
	     throw DataManager::ErrGeneric();	    
	  }
	 }
	 
	 /* Elements list */
	 while (HP.IsArg()) {
	    if (HP.IsKeyWord("range")) {
	       unsigned int uL = (unsigned int)HP.GetInt();
	       unsigned int uEndL = (unsigned int)HP.GetInt();
	       if (uEndL < uL) {
		  silent_cerr("End label " << uEndL << " must be larger "
				  "than or equal to start label " << uL
				  << std::endl);
		  throw ErrGeneric();
	       }
	       for ( ; uL <= uEndL; uL++) {
	          Elem* pE = (Elem*)pFindElem(Typ, uL);
	          if (pE != NULL) {
	             DEBUGLCOUT(MYDEBUG_INPUT, "element " << uL << std::endl);
	             pE->SetOutputFlag(flag(1));
		  }
	       }

	    } else {
	       unsigned int uL = (unsigned int)HP.GetInt();	    	
	       Elem* pE = (Elem*)pFindElem(Typ, uL);
	       if (pE == NULL) {
	          silent_cerr(psElemNames[Typ] << "(" << uL << ") "
			"is not defined; output cannot be modified"
			<< std::endl);
	       } else {
	          DEBUGLCOUT(MYDEBUG_INPUT, "element " << uL << std::endl);
	          pE->SetOutputFlag(flag(1));
	       }
	    }
	 }

      } else if (CurrDesc == INERTIA) {
	 unsigned int uIn = (unsigned int)HP.GetInt();

	 /* Nome dell'elemento */
         const char *sName = NULL;
         if (HP.IsKeyWord("name")) {
	    const char *sTmp = HP.GetStringWithDelims();
	    SAFESTRDUP(sName, sTmp);
	 }

#ifdef USE_STRUCT_NODES
	 Vec3 x(Zero3);
	 if (HP.IsKeyWord("position")) {
		 x = HP.GetPosAbs(AbsRefFrame);
	 }

	 Mat3x3 R(Eye3);
	 Mat3x3 RT(Eye3);
	 if (HP.IsKeyWord("orientation")) {
		 R = HP.GetRotAbs(AbsRefFrame);
		 RT = R.Transpose();
	 }
	 
	 doublereal dM(0.);
	 Vec3 S(0.);
	 Mat3x3 J(0.);

	 std::set<unsigned int> Body_labels;
	 Elem::Type Type = Elem::UNKNOWN;
	 bool bOut(false);
	 bool bLog(true);
	 while (HP.IsArg()) {
		 if (HP.IsKeyWord("output")) {
		 	if (HP.IsKeyWord("no")) {
				bLog = false;

			} else if (HP.IsKeyWord("yes")) {
				bLog = false;
				bOut = true;

			} else if (HP.IsKeyWord("log")) {
				/* bLog = true */ ;

			} else if (HP.IsKeyWord("both")) {
				bOut = true;

			} else {
				silent_cerr("unknown output mode for inertia "
						<< uIn << " at line "
						<< HP.GetLineData()
						<< std::endl);
				throw ErrGeneric();
			}
			break;

		 } else if (HP.IsKeyWord(sKeyWords[BODY])) {
			 Type = Elem::BODY;

		 } else if (HP.IsKeyWord(sKeyWords[JOINT])) {
			 Type = Elem::JOINT;

		 } else if (HP.IsKeyWord(sKeyWords[LOADABLE])) {
			 Type = Elem::LOADABLE;

#if 0
		 } else if (HP.IsKeyWord("...")) {
		 /* other types with inertia */
#endif
		 }

		 if (Type == Elem::UNKNOWN) {
			 silent_cerr("inertia " << uIn);
			 if (sName) {
				silent_cerr(" (" << sName << ")");
			 }
			 silent_cerr(" at line " << HP.GetLineData()
				 << ": unknown or undefined element type"
				 << std::endl);
			 throw ErrGeneric();
		 }

		/*
		 * FIXME: duplicate check?
		 */

		 if (HP.IsKeyWord("all")) {
			 Elem **ppTmpEl = ElemData[Type].ppFirstElem;
			 for (unsigned int cnt = 0; cnt < ElemData[Type].iNum; cnt++) {
				 unsigned int uL = ppTmpEl[cnt]->GetLabel();
 				 std::set<unsigned int>::const_iterator BL_end = Body_labels.end();

				 if (Body_labels.find(uL) == BL_end) {
				 	Body_labels.insert(uL);
				 	ElemGravityOwner *pEl = 
						(ElemGravityOwner *)ppTmpEl[cnt]->pGetElemGravityOwner();

       					dM += pEl->dGetM();
       					S += pEl->GetS();
       					J += pEl->GetJ();
				 } else {
	 				 silent_cerr(psElemNames[Type]
						 << "(" << uL 
						 << ") duplicate label at line "
	 					 << HP.GetLineData() 
						 << " (ignored)" << std::endl);
				 }
			 }

		 } else {
			 unsigned int uL = (unsigned int)HP.GetInt();
			 std::set<unsigned int>::const_iterator BL_end = Body_labels.end();
			 if (Body_labels.find(uL) == BL_end) {			 
			 	Elem **ppTmpEl = (Elem **)ppFindElem(Type, uL);
			 	if (ppTmpEl == NULL || ppTmpEl[0] == NULL) {
				 	silent_cerr("inertia " << uIn 
					 	<< " at line " << HP.GetLineData()
					 	<< ": unable to find " << psElemNames[Type]
					 	<< "( " << uL << ")" << std::endl);
				 	throw ErrGeneric();
			 	}

				Body_labels.insert(uL);
			 	ElemGravityOwner *pEl = 
				 	(ElemGravityOwner *)ppTmpEl[0]->pGetElemGravityOwner();
				ASSERT(pEl != 0);

				/* FIXME: temporary... */
				if (Type == Elem::JOINT) {
					Joint *pJ = dynamic_cast<Joint *>(pEl);
					if (pJ->GetJointType() != Joint::MODAL) {
						silent_cerr("warning, Joint("
								<< uL << ") "
								"is not modal"
								<< std::endl);
					}
				}

			 	dM += pEl->dGetM();
			 	S += pEl->GetS();
			 	J += pEl->GetJ();

		 	} else {
				silent_cerr(psElemNames[Type] << "(" << uL
					<< "): duplicate label at line "
					<< HP.GetLineData() << " (ignored)"
					<< std::endl);
			}
		}
	} 

	 Vec3 Xcg(0.);
	 Mat3x3 Jcg(J);
	 if (dM < DBL_EPSILON) {
	    silent_cerr("inertia " << uIn 
		    << " at line " << HP.GetLineData()
		    << ": mass is null" << std::endl);
	 } else {
	    Xcg = S/dM;
	    
	    /*
	     * FIXME: should also rotate it in the principal 
	     * reference frame, and log the angles
	     */
	    Jcg += Mat3x3(S, Xcg);
	 }

	 if (x != Zero3) {
	    Vec3 Dx = Xcg - x;
	    J += Mat3x3(x, x*dM) + Mat3x3(Dx, x*dM) + Mat3x3(x, Dx*dM);
	 }

	 if (bLog) {
		 OutHdl.Log()
			 << "inertia " << uIn
			 << " (" << ( sName ? sName : "unnamed" ) << ")"
			 	<< std::endl
			 << "    mass:        " << dM << std::endl
			 << "    Xcg:         " << Xcg << std::endl
			 << "    Xcg-X:       " << (Xcg - x) << std::endl
			 << "    R^T*(Xcg-X): " << RT*(Xcg - x) << std::endl
			 << "    J:           " << RT*J*R << std::endl
			 << "    Jcg:         " << RT*Jcg*R << std::endl;
	 }
	 if (bOut) {
		 silent_cout("inertia " << uIn
			 << " (" << ( sName ? sName : "unnamed" ) << ")"
			 	<< std::endl
			 << "    mass:        " << dM << std::endl
			 << "    Xcg:         " << Xcg << std::endl
			 << "    Xcg-X:       " << (Xcg - x) << std::endl
			 << "    R^T*(Xcg-X): " << RT*(Xcg - x) << std::endl
			 << "    J:           " << RT*J*R << std::endl
			 << "    Jcg:         " << RT*Jcg*R << std::endl);
	 }
	 if (sName) {
	    SAFEDELETEARR(sName);
	 }
	 
#else /* !USE_STRUCT_NODES */
	 silent_cerr("inertia " << uIn << " at line " << HP.GetLineData()
		 << " available only if structural nodes are enabled" 
		 << std::endl);
#if 0	/* not critical ... */
	 throw ErrGeneric();
#endif
#endif /* !USE_STRUCT_NODES */

      } else if (CurrDesc == BIND) {
	 /* Label dell'elemento */
	 unsigned int uL = HP.GetInt();
	 
	 /* Tipo dell'elemento */
	 Elem::Type t = Elem::UNKNOWN;	 
	 switch (KeyWords(HP.GetWord())) {
	  case BODY:
	    t = Elem::BODY;
	    break;
	  case AUTOMATICSTRUCTURAL:
	    t = Elem::AUTOMATICSTRUCTURAL;
	    break;
	  case JOINT:
	    t = Elem::JOINT;
	    break;
	  case FORCE:
	  case COUPLE:
	    t = Elem::FORCE;
	    break;
	  case BEAM:
	  case BEAM3:			/* same as BEAM */
	  case BEAM2:
	  case HBEAM:
	    t = Elem::BEAM;
	    break;
	  case ROTOR:
	    t = Elem::ROTOR;
	    break;
	  case AEROMODAL:  
	    t = Elem::AEROMODAL;
	    break;
#ifdef USE_EXTERNAL
          case AERODYNAMICEXTERNAL:
          case AERODYNAMICEXTERNALMODAL:
	    t = Elem::EXTERNAL;
	    break;
#endif /* USE_EXTERNAL */	  
	  case AERODYNAMICBODY:
	  case AERODYNAMICBEAM:	
	  case AERODYNAMICBEAM3:	/* same as AERODYNAMICBEAM */
	  case AERODYNAMICBEAM2:	
	  case AIRCRAFTINSTRUMENTS:
	    t = Elem::AERODYNAMIC;
	    break;
	  case GENEL:
	    t = Elem::GENEL;
	    break;
	  case ELECTRIC:
	    t = Elem::ELECTRIC;
	    break;
	  case HYDRAULIC:     
	    t = Elem::HYDRAULIC;
	    break;
	  case BULK:
	    t = Elem::BULK;
	    break;	 
	  case LOADABLE:
	    t = Elem::LOADABLE;
	    break;

	  case RTAI_OUTPUT:
	  case SOCKETSTREAM_OUTPUT:
	    silent_cerr(psElemNames[Elem::SOCKETSTREAM_OUTPUT]
		    << " does not support bind" << std::endl);
	  default:
	    throw ErrGeneric();
	 }
	 
	 Elem* pEl = ((Elem*)pFindElem(t, uL));
	 if (pEl == NULL) {
	    silent_cerr("can't find " << psElemNames[t] << " (" << uL 
	      << ") at line " << HP.GetLineData() << std::endl);
	    throw ErrGeneric();
	 }
      
	 /* Label del nodo parameter */
	 uL = HP.GetInt();
	 
	 Elem2Param* pNd = ((Elem2Param*)pFindNode(Node::PARAMETER, uL));
	 if (pNd == NULL) {
	    silent_cerr("can't find parameter node (" << uL
	      << ") at line " << HP.GetLineData() << std::endl);
	    throw ErrGeneric();
	 }

	 	/* Numero d'ordine del dato privato a cui fare il binding */
	 unsigned int i = 0;
	 if (HP.IsKeyWord("name") || HP.IsKeyWord("string" /* deprecated */ )) {
	    const char *s = HP.GetStringWithDelims();

	    ASSERT(s != NULL);

	    DEBUGCOUT("binding to " << psElemNames[pEl->GetElemType()]
			    << "(" << pEl->GetLabel() << ") private data \""
			    << s << "\"" << std::endl);

	    i = pEl->iGetPrivDataIdx(s);

	 } else {
	    i = HP.GetInt();
	 }
	 
	 /* indice del dato a cui il parametro e' bound */
	 if (i <= 0 || i > pEl->iGetNumPrivData()) {
	    silent_cerr("error in private data number " << i << " for element "
	      << psElemNames[t] << " (" << pEl->GetLabel() 
	      << ") at line " << HP.GetLineData() << std::endl);
	    throw ErrGeneric();
	 }
	 
	 /* fa il binding del ParameterNode all'elemento */
	 DEBUGLCOUT(MYDEBUG_INPUT, "Binding " << psElemNames[t] 
		    << " (" << pEl->GetLabel() 
		    << ") to Parameter " << pNd->GetLabel() << std::endl);
	 pNd->Bind(pEl, i);

	 
      /* gestisco a parte gli elementi automatici strutturali, perche'
       * sono gia' stati costruiti altrove e li devo solo inizializzare;
       * eventualmente si puo' fare altrimenti */
      } else if (CurrDesc == AUTOMATICSTRUCTURAL) {
	 unsigned int uLabel = HP.GetInt();
	 Elem* pEl = (Elem*)pFindElem(Elem::AUTOMATICSTRUCTURAL, uLabel);
	 if (pEl == NULL) {
	    silent_cerr("line " << HP.GetLineData() 
	      << ": unable to find automatic structural element " 
	      << uLabel << std::endl);
	    throw ErrGeneric();
	 }
	 
	 DEBUGCOUT("reading automatic structural element " << uLabel << std::endl);

	 /* forse e' il caso di usare il riferimento del nodo? */
	 
	 /* nota: i primi due sono gestiti direttamente 
	  * dagli elementi con inerzia, e quindi non sono usati */
	 Vec3 q(HP.GetVecAbs(AbsRefFrame));
	 Vec3 g(HP.GetVecAbs(AbsRefFrame));
	 Vec3 qp(HP.GetVecAbs(AbsRefFrame));
	 Vec3 gp(HP.GetVecAbs(AbsRefFrame));
	 
	 DEBUGCOUT("Q  = " << q << std::endl
		   << "G  = " << g << std::endl
		   << "Qp = " << qp << std::endl
		   << "Gp = " << gp << std::endl);
	   
	 AutomaticStructElem* pAuto = (AutomaticStructElem*)pEl->pGet();
	 pAuto->Init(q, g, qp, gp);

      /* default: leggo un elemento e lo creo */
      } else {
      
      
	 /* puntatore al puntatore all'elemento */
	 Elem** ppE = NULL;
	 
	 unsigned int uLabel;
	 switch (CurrDesc) {
	    /* Qui vengono elencati gli elementi unici, che non richiedono label
	     * (per ora: accelerazione di gravita' e proprieta' dell'aria */
	    
#ifdef USE_STRUCT_NODES
	    /* Accelerazione di gravita' */
	  case GRAVITY: {
	     silent_cout("Reading gravity acceleration" << std::endl);
	     
	     if (iNumTypes[Elem::GRAVITY]-- <= 0) {
		DEBUGCERR("");
		silent_cerr("line " << HP.GetLineData() 
		  << ": gravity acceleration element is not defined"
		  << std::endl);
		
		throw DataManager::ErrGeneric();
	     }
	     
	     ppE = ElemData[Elem::GRAVITY].ppFirstElem;
	     uLabel = 1;
	     
	      
	     TplDriveCaller<Vec3>* pDC = ReadTplDrive(this, HP, Vec3(0.));
	     
	     flag fOut = fReadOutput(HP, Elem::GRAVITY);
	     
	     SAFENEWWITHCONSTRUCTOR(*ppE,
				    Gravity,
				    Gravity(pDC, fOut));
	     
	     break;
	  }
	    
#ifdef USE_AERODYNAMIC_ELEMS
	    /* Elementi aerodinamici: proprieta' dell'aria */
	  case AIRPROPERTIES: {
	     silent_cout("Reading air properties" << std::endl);
	     
	     if(iNumTypes[Elem::AIRPROPERTIES]-- <= 0) {
		DEBUGCERR("");
		silent_cerr("line " << HP.GetLineData() 
		  << ": air properties element is not defined" << std::endl);
		
		throw DataManager::ErrGeneric();
	     }
	     
	     ppE = ElemData[Elem::AIRPROPERTIES].ppFirstElem;
	     uLabel = 1;

	     *ppE = ReadAirProperties(this, HP);
	     
	     break;
	  }
#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_STRUCT_NODES */
	    
	    /* Elemento generico: legge la label e fa un nuovo switch */
	  default: {	      
	     /* legge la label */	
	     uLabel = unsigned(HP.GetInt());

	     /* in base al tipo, avviene l'allocazione */
	     switch (CurrDesc) {
		/* corpo rigido */
		
		
	      case DRIVEN: {
		 /* Reads the driver */
		 DriveCaller* pDC = HP.GetDriveCaller();
		 std::vector<std::string> hints;

		 for (unsigned i = 1; HP.IsKeyWord("hint"); i++) {
			 const char *hint = HP.GetStringWithDelims();
			 if (hint == 0) {
				silent_cerr("Driven(" << uLabel << "): "
					"unable to read hint #" << i 
					<< " at line " << HP.GetLineData()
					<< std::endl);
				throw ErrGeneric();
			 }
			 hints.push_back(hint);
		 }
		 
		 HP.ExpectDescription();
		 KeyWords CurrDriven = KeyWords(HP.GetDescription());
		 
#ifdef DEBUG
		 switch (CurrDriven) {
		  case FORCE:
#ifdef USE_STRUCT_NODES
		  case BODY:
		  case JOINT:
		  case COUPLE:
		  case BEAM:
		  case BEAM3:			/* same as BEAM */
		  case BEAM2:
		  case HBEAM:
#ifdef USE_AERODYNAMIC_ELEMS
		  case ROTOR:
		  case AERODYNAMICBODY:
		  case AERODYNAMICBEAM:
		  case AERODYNAMICBEAM3:	/* same as AERODYNAMICBEAM */
		  case AERODYNAMICBEAM2:
		  case AIRCRAFTINSTRUMENTS:
#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_STRUCT_NODES */
#ifdef USE_ELECTRIC_NODES
		  case GENEL:
		  case ELECTRIC:
#endif /* USE_ELECTRIC_NODES */
#ifdef USE_HYDRAULIC_NODES
		  case HYDRAULIC:
#endif /* USE_HYDRAULIC_NODES */
		  case BULK:
		  case LOADABLE:
		  case EXISTING: {
		     DEBUGLCOUT(MYDEBUG_INPUT, "OK, this element can be driven" << std::endl);
		     break;
		  }

		  case RTAI_OUTPUT:
		  case SOCKETSTREAM_OUTPUT:
		     silent_cerr(psElemNames[Elem::SOCKETSTREAM_OUTPUT]
	 		     << " cannot be driven" << std::endl);
	 	     throw ErrGeneric();
		    
		  default: {
		     DEBUGCERR("warning, this element can't be driven" << std::endl);
		     break;
		  }
		 }		     		  
#endif /* DEBUG */
		 
		 if (CurrDriven == EXISTING) {
		    iMissingElems++;
		    CurrDriven = KeyWords(HP.GetWord());
		    unsigned int uL = (unsigned int)HP.GetInt();
		    if (uL != uLabel) {
		       silent_cerr("Error: the driving element must have "
				       "the same label of the driven"
				       << std::endl);
		       
		       throw DataManager::ErrGeneric();
		    }
		    
		    switch (CurrDriven) {
		     case FORCE: {
			ppE = ppFindElem(Elem::FORCE, uLabel);
			break;
		     }
#ifdef USE_STRUCT_NODES
		     case BODY: {
			ppE = ppFindElem(Elem::BODY, uLabel);
			break;
		     }
		     case JOINT: {
			ppE = ppFindElem(Elem::JOINT, uLabel);
			break;
		     }
		     case COUPLE: {
			ppE = ppFindElem(Elem::FORCE, uLabel);
			break;
		     }
		     case BEAM:
		     case BEAM3:		/* same as BEAM */
		     case BEAM2:
		     case HBEAM: {
			ppE = ppFindElem(Elem::BEAM, uLabel);
			break;
		     }
#ifdef USE_AERODYNAMIC_ELEMS
		     case ROTOR: {
			ppE = ppFindElem(Elem::ROTOR, uLabel);
			break;
		     }		     
		     case AERODYNAMICBODY:
		     case AERODYNAMICBEAM:
		     case AERODYNAMICBEAM3:	/* same as AERODYNAMICBEAM */
		     case AERODYNAMICBEAM2:
		     case AIRCRAFTINSTRUMENTS: {
			ppE = ppFindElem(Elem::AERODYNAMIC, uLabel);
			break;
		     }
#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_STRUCT_NODES */
#ifdef USE_ELECTRIC_NODES
		     case GENEL: {
			ppE = ppFindElem(Elem::GENEL, uLabel);
			break;
		     }
		     case ELECTRIC: {
			ppE = ppFindElem(Elem::ELECTRIC, uLabel);
			break;
		     }
#endif /* USE_ELECTRIC_NODES */
#ifdef USE_HYDRAULIC_NODES
		     case HYDRAULIC: {
			ppE = ppFindElem(Elem::HYDRAULIC, uLabel);
			break;
		     }
#endif /* USE_HYDRAULIC_NODES */
		     case BULK: {
			ppE = ppFindElem(Elem::BULK, uLabel);
			break;
		     }
		     case LOADABLE: {
			ppE = ppFindElem(Elem::LOADABLE, uLabel);
			break;
		     }

		     default: {
			DEBUGCERR("warning, this element can't be driven" << std::endl);
			break;
		     }
		    }
		    
		    if (ppE == NULL) {
		       silent_cerr("Error: element " << uLabel 
			 << "cannot be driven since it doesn't exist"
			 << std::endl);
		       
		       throw DataManager::ErrGeneric();
		    }
		    
		    
		    flag fOut = fReadOutput(HP, (*ppE)->GetElemType());
		    (*ppE)->SetOutputFlag(fOut);
		    
		    
		 } else {
		    unsigned int uDummy = (unsigned int)HP.GetInt();
		    if (uDummy != uLabel) {
		       silent_cerr("Error: the element label must be "
				       "the same of the driving element"
				       << std::endl);
		       
		       throw DataManager::ErrGeneric();
		    }
		    
		    /* Reads the true element */
		    ppE = ReadOneElem(HP, uLabel, CurrDriven);
		    
		    if (*ppE == NULL) {
		       DEBUGCERR("");
		       silent_cerr("error in allocation of element "
			 << uLabel << std::endl);
		       
		       throw ErrMemory();
		    }
		 }

		 SimulationEntity::Hints *pHints = 0;
		 if (hints.size() > 0) {
		 	 pHints = new SimulationEntity::Hints;
		 	 for (unsigned i = 0; i < hints.size(); i++) {
				 SimulationEntity::Hint *ph = (*ppE)->ParseHint(this, hints[i].c_str());
				 if (ph != 0) {
					 pHints->push_back(ph);
				 }
			 }
			 if (pHints->size() == 0) {
				 delete pHints;
				 pHints = 0;
			 }
		 }
		 
		 /* Creates the driver for the element */
		 Elem* pEl = NULL;
		 SAFENEWWITHCONSTRUCTOR(pEl,
					DrivenElem,
					DrivenElem(this, pDC, *ppE, pHints));
		 		 
		 /* Substitutes the element with the driver */
		 *ppE = pEl;
		 
		 break;
	      }
		
		
		/* Normal element */
	      case FORCE:
#ifdef USE_STRUCT_NODES
	      case BODY:
	      case JOINT:
	      case COUPLE:
	      case BEAM:
	      case BEAM3:		/* same as BEAM */
	      case BEAM2:
	      case HBEAM:
#ifdef USE_AERODYNAMIC_ELEMS
	      case ROTOR:
	      case AERODYNAMICBODY:
	      case AERODYNAMICBEAM:
	      case AERODYNAMICBEAM3:	/* same as AERODYNAMICBEAM */
	      case AERODYNAMICBEAM2:
	      case AIRCRAFTINSTRUMENTS:
#ifdef USE_EXTERNAL
              case AERODYNAMICEXTERNAL:
              case AERODYNAMICEXTERNALMODAL:
#endif /* USE_EXTERNAL */ 
	      case AEROMODAL:
#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_STRUCT_NODES */
#ifdef USE_ELECTRIC_NODES
	      case GENEL:
	      case ELECTRIC:
#endif /* USE_ELECTRIC_NODES */
#ifdef USE_HYDRAULIC_NODES
	      case HYDRAULIC:
#endif /* USE_HYDRAULIC_NODES */
	      case BULK:
	      case LOADABLE:
	      case RTAI_OUTPUT:
	      case SOCKETSTREAM_OUTPUT:
		  {		 
		 /* Nome dell'elemento */
             	 const char *sName = NULL;
             	 if (HP.IsKeyWord("name")) {
	            const char *sTmp = HP.GetStringWithDelims();
	            SAFESTRDUP(sName, sTmp);
	         }

#if defined(HAVE_RUNTIME_LOADING) && defined(HAVE_LTDL_H)
		 if (CurrDesc == LOADABLE) {
		    if (loadableElemInitialized == false) {
		       if (lt_dlinit()) {
	   		  silent_cerr("unable to initialize loadable elements"
					  << std::endl);
      			  throw ErrGeneric();
      		       }

		       /*
		        * NOTE: this macro is defined in mbdefs.h
		        */
		       if (lt_dlsetsearchpath(MODULE_LOADPATH) != 0) {
			  silent_cerr("unable to initialize load path"
				  << std::endl);
		       }
		       loadableElemInitialized = true;
		    }
		 }
#endif /* HAVE_RUNTIME_LOADING && HAVE_LTDL_H */
		 
		 ppE = ReadOneElem(HP, uLabel, CurrDesc);
		 
		 if (sName != NULL) {
		    (*ppE)->PutName(sName);
   		    SAFEDELETEARR(sName);
		 }

		 break;
	      }
		
		
		/* in caso di tipo sconosciuto */
	      case UNKNOWNKEYWORD: {
		 DEBUGCERR("");
		 silent_cerr("error - unknown element type at line " 
		   << HP.GetLineData() << std::endl);
		 
		 throw DataManager::ErrGeneric();
	      }
		
	      default: {
		 DEBUGCERR("");
		 silent_cerr("error - element type " << sKeyWords[CurrDesc]
		   << " at line " << HP.GetLineData() 
		   << " is not allowed " << std::endl);
		 
		 throw DataManager::ErrGeneric();
	      }        
	     }	      	      	      
	  }        
	 }
	 
	 /* verifica dell'allocazione */
	 ASSERT(*ppE != NULL);
	 
	 /* Aggiorna le dimensioni massime degli spazi di lavoro 
	  * (qui va bene perche' il puntatore e' gia' stato verificato) */
	 integer iNumRows = 0;
	 integer iNumCols = 0;
	 (*ppE)->WorkSpaceDim(&iNumRows, &iNumCols);
	 if (iNumRows > iMaxWorkNumRows) {
	    iMaxWorkNumRows = iNumRows;
	    DEBUGLCOUT(MYDEBUG_INIT, "Current max work rows number: " 
		       << iMaxWorkNumRows << std::endl);
	 }
	 if (iNumCols > iMaxWorkNumCols) {
	    iMaxWorkNumCols = iNumCols;
	    DEBUGLCOUT(MYDEBUG_INIT, "Current max work cols number: "
		       << iMaxWorkNumCols << std::endl);
	 }
	 
	 /* decrementa il totale degli elementi mancanti */
	 iMissingElems--;	  
      }
   }
   
   if (KeyWords(HP.GetWord()) != ELEMENTS) {
      DEBUGCERR("");
      silent_cerr("<end: elements;> expected at line" 
	<< HP.GetLineData() << std::endl);
           
      throw DataManager::ErrGeneric();
   }
   
   /* Se non c'e' il punto e virgola finale */
   if (HP.IsArg()) {
      DEBUGCERR("");
      silent_cerr("semicolon expected at line " << HP.GetLineData()
		      << std::endl);
     
      throw DataManager::ErrGeneric();
   }   
   
   if (iMissingElems > 0) {
      DEBUGCERR("");
      silent_cerr("warning: " << iMissingElems
	<< " elements are missing;" << std::endl);
      for (int iCnt = 0; iCnt < Elem::LASTELEMTYPE; iCnt++) {
	 if (iNumTypes[iCnt] > 0) {
	    silent_cerr("  " << iNumTypes[iCnt] 
	      << ' ' << psElemNames[iCnt] << std::endl);
	 }	 
      }      
      
      throw DataManager::ErrGeneric();
   }
   
#ifdef USE_STRUCT_NODES
   /* Linka gli elementi che generano forze d'inerzia all'elemento 
    * accelerazione di gravita' */
   if (ElemData[Elem::GRAVITY].iNum > 0) {
      Gravity* pGrav = 
	(Gravity*)(*(ElemData[Elem::GRAVITY].ppFirstElem))->pGet();
      for (int iCnt = 0; iCnt < Elem::LASTELEMTYPE; iCnt++) {
	 if ((ElemData[iCnt].fGeneratesInertialForces == 1)
	     && (ElemData[iCnt].iNum > 0)) {
	    
	    Elem** ppTmp = ElemData[iCnt].ppFirstElem;
	    Elem** ppLastEl = ppTmp+ElemData[iCnt].iNum;
	    while (ppTmp < ppLastEl) {	       
	       ASSERT((*ppTmp)->pGetElemGravityOwner() != NULL);
	       (*ppTmp)->pGetElemGravityOwner()->PutGravity(pGrav);
	       ppTmp++;
	    }	 
	 }      
      }   
   }

#ifdef USE_AERODYNAMIC_ELEMS
   /* Linka gli elementi che usano le proprieta' dell'aria all'elemento
    * proprieta' dell'aria */
   if (ElemData[Elem::AIRPROPERTIES].iNum > 0) {
      AirProperties* pProp =
	(AirProperties*)(*(ElemData[Elem::AIRPROPERTIES].ppFirstElem))->pGet();
      
      for (int iCnt = 0; iCnt < Elem::LASTELEMTYPE; iCnt++) {
	 if ((ElemData[iCnt].fUsesAirProperties == 1)
	     && (ElemData[iCnt].iNum > 0)) {	    
	    
	    Elem** ppTmp = ElemData[iCnt].ppFirstElem;	    
	    Elem** ppLastEl = ppTmp+ElemData[iCnt].iNum;	    
	    while (ppTmp < ppLastEl) {
	       ASSERT((*ppTmp)->pGetAerodynamicElem() != NULL);
	       (*ppTmp)->pGetAerodynamicElem()->PutAirProperties(pProp);
	       ppTmp++;
	    }	 	     
	 }      
      }
      
   } else {
      /* Esegue un controllo per vedere se esistono elementi aerodinamici
       * ma non sono definite le proprieta' dell'aria, nel qual caso
       * il calcolo deve essere arrestato */
      flag fStop(0);
      
      for (int iCnt = 0; iCnt < Elem::LASTELEMTYPE; iCnt++) {
	 if ((ElemData[iCnt].fUsesAirProperties == 1)
	     && (ElemData[iCnt].iNum > 0)) {
	    for (unsigned int iEl = 0; iEl < ElemData[iCnt].iNum; iEl++) {
	       if (ElemData[iCnt].ppFirstElem[iEl]->pGetAerodynamicElem()->NeedsAirProperties()) {
	          if (fStop == 0) {
	             silent_cerr("The following aerodynamic elements "
				     "are defined: " << std::endl);
	             fStop = 1;
		  }
	       }
	    }

	    silent_cerr(ElemData[iCnt].iNum << " " 
	      << psElemNames[iCnt] << std::endl);
	 }
      }
      
      if (fStop) {
	 silent_cerr("while no air properties are defined; aborting ..."
			 << std::endl);
	 
	 throw DataManager::ErrGeneric();
      }
   }
#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_STRUCT_NODES */
   
   DEBUGLCOUT(MYDEBUG_INPUT, "End of elements data" << std::endl);
} /* End of DataManager::ReadElems() */



Elem** 
DataManager::ReadOneElem(MBDynParser& HP,
		   unsigned int uLabel,
		   int CurrType)
{

   Elem** ppE = NULL;
   
   switch (KeyWords(CurrType)) {

      /* forza */
    case FORCE:
#ifdef USE_STRUCT_NODES
    case COUPLE:
#endif /* USE_STRUCT_NODES */
	{
       int iForceType;
       if (KeyWords(CurrType) == FORCE) {
	  iForceType = 0;
	  silent_cout("Reading force " << uLabel << std::endl);
       } else /* if(KeyWords(CurrType) == COUPLE) */ {
	  iForceType = 1;
	  silent_cout("Reading couple " << uLabel << std::endl);
       }
       
       if (iNumTypes[Elem::FORCE]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": force " << uLabel
	    << " exceeds force elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::FORCE, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": force " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       
       /* allocazione e creazione */		      
       int i = ElemData[Elem::FORCE].iNum
	 -iNumTypes[Elem::FORCE]-1;
       ppE = ElemData[Elem::FORCE].ppFirstElem+i;
       
       *ppE = ReadForce(this, HP, uLabel, iForceType);
       
       break;
    }
      
#ifdef USE_STRUCT_NODES
    case BODY: {
       silent_cout("Reading rigid body " << uLabel << std::endl);
       
       if(iNumTypes[Elem::BODY]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": rigid body " << uLabel
	    << " exceeds rigid body elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* verifica che non sia gia' definito */		    
       if(pFindElem(Elem::BODY, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": rigid body " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       
       /* allocazione e creazione */
       int i = ElemData[Elem::BODY].iNum-iNumTypes[Elem::BODY]-1;
       ppE = ElemData[Elem::BODY].ppFirstElem+i;
       
       *ppE = ReadBody(this, HP, uLabel);     
       
       break;
    }
      
      /* vincoli */
    case JOINT: {    
       silent_cout("Reading joint " << uLabel << std::endl);
       
       if (iNumTypes[Elem::JOINT]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": joint " << uLabel
	    << " exceeds joint elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }	  
       
       /* verifica che non sia gia' definito */		      
       if (pFindElem(Elem::JOINT, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": joint " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       
       /* allocazione e creazione */
       int i = ElemData[Elem::JOINT].iNum
	 -iNumTypes[Elem::JOINT]-1;
       ppE = ElemData[Elem::JOINT].ppFirstElem+i;
       DofOwner* pDO = DofData[DofOwner::JOINT].pFirstDofOwner+i;
       
       *ppE = ReadJoint(this, HP, pDO, uLabel);     
       
       /* attenzione: i Joint aggiungono DofOwner e quindi devono
	* completare la relativa struttura */
       break;
    }
      
      /* trave */
    case BEAM:
    case BEAM3:		/* same as BEAM */
    case BEAM2:
    case HBEAM: {      
       silent_cout("Reading beam " << uLabel << std::endl);
       
       if (iNumTypes[Elem::BEAM]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": beam " << uLabel
	    << " exceeds beam elements number" << std::endl);
	 
	  throw DataManager::ErrGeneric();
       }
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::BEAM, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": beam " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* allocazione e creazione */
       int i = ElemData[Elem::BEAM].iNum
	 -iNumTypes[Elem::BEAM]-1;
       ppE = ElemData[Elem::BEAM].ppFirstElem+i;
      
       switch (KeyWords(CurrType)) {
       case BEAM3:
       case BEAM:	/* same as BEAM3 */
	  *ppE = ReadBeam(this, HP, uLabel);
	  break;

       case BEAM2:
	  *ppE = ReadBeam2(this, HP, uLabel);
	  break;

       case HBEAM:
	  *ppE = ReadHBeam(this, HP, uLabel);
	  break;

       default:
	  throw DataManager::ErrGeneric();
       }
       
       break;
    }
      
#ifdef USE_AERODYNAMIC_ELEMS
      /* Elementi aerodinamici: rotori */
    case ROTOR: {
       silent_cout("Reading rotor " << uLabel << std::endl);
       
       if (iNumTypes[Elem::ROTOR]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": rotor " << uLabel
	    << " exceeds rotor elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::ROTOR, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": rotor " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* allocazione e creazione */
       int i = ElemData[Elem::ROTOR].iNum
	 -iNumTypes[Elem::ROTOR]-1;
       ppE = ElemData[Elem::ROTOR].ppFirstElem+i;
       DofOwner* pDO = DofData[DofOwner::ROTOR].pFirstDofOwner+i;
       
       *ppE = ReadRotor(this, HP, pDO, uLabel);     
       
       break;
    }	 
      
      /* Elementi aerodinamici: rotori */
      /* Elementi aerodinamici: modale */
    case AEROMODAL: {
       silent_cout("Reading aero modal " << uLabel << std::endl);
       
       if (iNumTypes[Elem::AEROMODAL]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": aeromodal " << uLabel
	    << " exceeds aeromodal elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::AEROMODAL, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": aeromodal " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* allocazione e creazione */
       int i = ElemData[Elem::AEROMODAL].iNum
	 -iNumTypes[Elem::AEROMODAL]-1;
       ppE = ElemData[Elem::AEROMODAL].ppFirstElem+i;
       DofOwner* pDO = DofData[DofOwner::AEROMODAL].pFirstDofOwner+i;
       
       *ppE = ReadAerodynamicModal(this, HP, pDO, uLabel);     
       
       break;
    }	 
      
      /* Elementi aerodinamici: aeromodal */
    case AERODYNAMICEXTERNAL:
    case AERODYNAMICEXTERNALMODAL: {
#ifdef USE_EXTERNAL
       silent_cout("Reading external element " << uLabel << std::endl);
       
       if (iNumTypes[Elem::EXTERNAL]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": external element " << uLabel
	    << " exceeds external elements number" << std::endl);
	 
	  throw DataManager::ErrGeneric();
       }
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::EXTERNAL, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": external element " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       /* allocazione e creazione */
       int i = ElemData[Elem::EXTERNAL].iNum
	 -iNumTypes[Elem::EXTERNAL]-1;
       ppE = ElemData[Elem::EXTERNAL].ppFirstElem+i;
       
       switch(KeyWords(CurrType)) {
         case AERODYNAMICEXTERNAL:
		*ppE = ReadAerodynamicExternal(this, HP, uLabel);
		break;

         case AERODYNAMICEXTERNALMODAL:
		*ppE = ReadAerodynamicExternalModal(this, HP, uLabel);
		break;
	
         default:
	   ASSERTMSG(0, "You shouldn't have reached this point");
	   break;
       }
#else /* !USE_EXTERNAL */
       silent_cerr("You need mpi and -DUSE_AERODYNAMIC_EXTERNAL " 
       		<< "to use this type of elements." << std::endl);
       throw ErrGeneric();
#endif /* !USE_EXTERNAL */	  
       break;
    }
    case AERODYNAMICBODY:
    case AERODYNAMICBEAM:
    case AERODYNAMICBEAM3:	/* same as AERODYNAMICBEAM */
    case AERODYNAMICBEAM2:
    case AIRCRAFTINSTRUMENTS: {
       silent_cout("Reading aerodynamic element " << uLabel << std::endl);
       
       if (iNumTypes[Elem::AERODYNAMIC]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": aerodynamic element " << uLabel
	    << " exceeds aerodynamic elements number" << std::endl);
	 
	  throw DataManager::ErrGeneric();
       }
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::AERODYNAMIC, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": aerodynamic element " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* allocazione e creazione */
       int i = ElemData[Elem::AERODYNAMIC].iNum
	 -iNumTypes[Elem::AERODYNAMIC]-1;
       ppE = ElemData[Elem::AERODYNAMIC].ppFirstElem+i;
       
       switch(KeyWords(CurrType)) {
	case AERODYNAMICBODY: {
	   *ppE = ReadAerodynamicBody(this, HP, uLabel);
	   break;
	}
	  
	case AERODYNAMICBEAM3:
	case AERODYNAMICBEAM:	/* same as BEAM3 */
	   *ppE = ReadAerodynamicBeam(this, HP, uLabel);
	   break;
	  
	case AERODYNAMICBEAM2:
	   *ppE = ReadAerodynamicBeam2(this, HP, uLabel);
	   break;


	case AIRCRAFTINSTRUMENTS:
	   *ppE = ReadAircraftInstruments(this, HP, uLabel);
	   break;

	default:
	   ASSERTMSG(0, "You shouldn't have reached this point");
	   break;
       }     
       
       break;
    }
#endif /* USE_AERODYNAMIC_ELEMS */
#endif /* USE_STRUCT_NODES */

      
#ifdef USE_ELECTRIC_NODES
      /* genel */
    case GENEL: {
       silent_cout("Reading genel " << uLabel << std::endl);

       if(iNumTypes[Elem::GENEL]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": genel " << uLabel
	    << " exceeds genel elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }	  
       
       /* verifica che non sia gia' definito */		      
       if(pFindElem(Elem::GENEL, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": genel " << uLabel
	    << " already defined" << std::endl);
	 
	  throw DataManager::ErrGeneric();
       }
       
       
       /* allocazione e creazione */
       int i = ElemData[Elem::GENEL].iNum
	 -iNumTypes[Elem::GENEL]-1;
       ppE = ElemData[Elem::GENEL].ppFirstElem+i;
       DofOwner* pDO = DofData[DofOwner::GENEL].pFirstDofOwner+i;
       
       *ppE = ReadGenel(this, HP, pDO, uLabel);
       
       break;
    }
      
#ifdef USE_HYDRAULIC_NODES
      /* elementi idraulici */
    case HYDRAULIC: {
       silent_cout("Reading hydraulic element " << uLabel << std::endl);
       
       if(iNumTypes[Elem::HYDRAULIC]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": hydraulic element " << uLabel
	    << " exceeds hydraulic elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }	  
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::HYDRAULIC, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": hydraulic element " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       
       /* allocazione e creazione */
       int i = ElemData[Elem::HYDRAULIC].iNum
	 -iNumTypes[Elem::HYDRAULIC]-1;
       ppE = ElemData[Elem::HYDRAULIC].ppFirstElem+i;
       DofOwner* pDO = DofData[DofOwner::HYDRAULIC].pFirstDofOwner+i;
       
       *ppE = ReadHydraulicElem(this, HP, pDO, uLabel);
       
       /* attenzione: gli elementi elettrici aggiungono DofOwner 
	* e quindi devono completare la relativa struttura */
       
       break;
    }
#endif /* USE_HYDRAULIC_NODES */

      
      /* elementi elettrici */
    case ELECTRIC: {
       silent_cout("Reading electric element " << uLabel << std::endl);
       
       if(iNumTypes[Elem::ELECTRIC]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": electric element " << uLabel
	    << " exceeds electric elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }	  
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::ELECTRIC, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": electric element " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       
       /* allocazione e creazione */
       int i = ElemData[Elem::ELECTRIC].iNum
	 -iNumTypes[Elem::ELECTRIC]-1;
       ppE = ElemData[Elem::ELECTRIC].ppFirstElem+i;
       DofOwner* pDO = DofData[DofOwner::ELECTRIC].pFirstDofOwner+i;
       
       *ppE = ReadElectric(this, HP, pDO, uLabel);
       
       /* attenzione: gli elementi elettrici aggiungono DofOwner 
	* e quindi devono completare la relativa struttura */
       
       break;
    }
#endif /* USE_ELECTRIC_NODES */
      
      /* elementi bulk */
    case BULK: {
       silent_cout("Reading bulk element " << uLabel << std::endl);
       
       if (iNumTypes[Elem::BULK]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": bulk element " << uLabel
	    << " exceeds bulk elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }	  
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::BULK, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": bulk element " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       
       /* allocazione e creazione */		     
       int i = ElemData[Elem::BULK].iNum
	 -iNumTypes[Elem::BULK]-1;
       ppE = ElemData[Elem::BULK].ppFirstElem+i;
       
       *ppE = ReadBulk(this, HP, uLabel);
       
       break;
    }		 		                     
    
      /* elementi loadable */
    case LOADABLE: {
       silent_cout("Reading loadable element " << uLabel << std::endl);
       
       if (iNumTypes[Elem::LOADABLE]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": loadable element " << uLabel
	    << " exceeds loadable elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }	  
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::LOADABLE, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": loadable element " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* allocazione e creazione */		     
       int i = ElemData[Elem::LOADABLE].iNum
	 -iNumTypes[Elem::LOADABLE]-1;
       ppE = ElemData[Elem::LOADABLE].ppFirstElem+i;
       DofOwner* pDO = DofData[DofOwner::LOADABLE].pFirstDofOwner+i;
              
       *ppE = ReadLoadable(this, HP, pDO, uLabel);
       
       break;
    }		 		                     

    case RTAI_OUTPUT:
#ifndef USE_RTAI
       silent_cout("need --with-rtai to allow RTAI mailboxes; "
		       "using stream output..." << std::endl);
#endif /* ! USE_RTAI */
       /* fall thru... */

    case SOCKETSTREAM_OUTPUT: {
       silent_cout("Reading stream output element " << uLabel
		       << std::endl);
       
       if (iNumTypes[Elem::SOCKETSTREAM_OUTPUT]-- <= 0) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": stream output element " << uLabel
	    << " exceeds stream output elements number" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }	  
       
       /* verifica che non sia gia' definito */
       if (pFindElem(Elem::SOCKETSTREAM_OUTPUT, uLabel) != NULL) {
	  DEBUGCERR("");
	  silent_cerr("line " << HP.GetLineData() 
	    << ": stream output element " << uLabel
	    << " already defined" << std::endl);
	  
	  throw DataManager::ErrGeneric();
       }
       
       /* allocazione e creazione */		     
       int i = ElemData[Elem::SOCKETSTREAM_OUTPUT].iNum
	 -iNumTypes[Elem::SOCKETSTREAM_OUTPUT]-1;
       ppE = ElemData[Elem::SOCKETSTREAM_OUTPUT].ppFirstElem+i;
              
#ifdef USE_RTAI
       if (mbdyn_rtai_task != 0) {
	  silent_cerr("starting RTAI out element" << std::endl);
          *ppE = ReadRTAIOutElem(this, HP, uLabel);
       } else
#endif /* USE_RTAI */
       {
	  pedantic_cout("starting stream element" << std::endl);
          *ppE = ReadSocketStreamElem(this, HP, uLabel);
       }
       break;
    }
     
      /* In case the element type is not correct */
    default: {
       silent_cerr("You shouldn't be here" << std::endl);
       
       throw DataManager::ErrGeneric();
    }
   }
   
   /* Ritorna il puntatore al puntatore all'elemento appena costruito */
   return ppE;
}
