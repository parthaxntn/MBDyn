/* 
 * HmFe (C) is a FEM analysis code. 
 *
 * Copyright (C) 1996-2001
 *
 * Marco Morandini  <morandini@aero.polimi.it>
 * Teodoro Merlini  <merlini@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
/*
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2000
 *
 * This code is a partial merge of HmFe and MBDyn.
 *
 * Pierangelo Masarati  <masarati@aero.polimi.it>
 * Paolo Mantegazza     <mantegazza@aero.polimi.it>
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

#include "matvecexp.h"
#include "Rot.hh"

// Input:
// node_pos	array delle posizioni attuali dei due nodi
// node_or		array delle orientazioni attuali dei due nodi
// w		array dei pesi associati ai due nodi nel punto di calcolo
// wder		array dei pesi derivati rispetto all'ascissa curvilinea
// 
// Output:
// pos		posizione del punto interpolato
// or		orinetazione del punto interpolato
// or_delta_w_or	or_delta in funzione di or_delta nodali
// delta_pos_w_or	delta_pos in funzione di or_delta nodali
// delta_pos_w_pos	delta_pos in funzione di delta_pos nodali
// F		derivata della posizione rispetto all'ascissa curvilinea
// om		assiale di d(or)/d(s)*or.Transpose()
// delta_om_ws_or	delta_om in funzione di or_delta nodali
// delta_F_ws_or	delta_F in funzione di or_delta nodali
// delta_F_ws_pos	delta_F in funzione di delta_pos nodali
// 
// Nota:
// Dopo l'uscita bisogna convertire gli or_delta nodali 
// in delta_parametri_or

#ifndef HBEAM_INTERP_H
#define HBEAM_INTERP_H

void ComputeInterpolation(const Vec3 *const node_pos,
			const Mat3x3 *const node_or,
			const doublereal *const w,
			const doublereal *const wder,
			Vec3 &pos,
			Mat3x3 &or,
			Mat3x3 *const or_delta_w_or,
			Mat3x3 *const delta_pos_w_or,
			Mat3x3 *const delta_pos_w_pos,
			Vec3 &F,
			Vec3 &om,
			Mat3x3 *const delta_om_ws_or,
			Mat3x3 *const delta_F_ws_or,
			Mat3x3 *const delta_F_ws_pos);

#endif /* HBEAM_INTERP_H */

