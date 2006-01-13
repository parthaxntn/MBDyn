/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2006
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

/* Reference frame: strutura, gestione ecc. */

#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include <reffrm.h>

ReferenceFrame::ReferenceFrame(void)
{
	NO_OP;
}

ReferenceFrame::ReferenceFrame(unsigned int uLabel, 
		const Vec3& xIn, const Mat3x3& RIn,
		const Vec3& vIn, const Vec3& wIn)
: WithLabel(uLabel), x(xIn), R(RIn), v(vIn), w(wIn)
{
	NO_OP;
}

ReferenceFrame::ReferenceFrame(const StructNode* pNode)
: WithLabel(pNode->GetLabel()),
x(pNode->GetXCurr()), R(pNode->GetRCurr()),
v(pNode->GetVCurr()), w(pNode->GetWCurr())
{
	NO_OP;
}

ReferenceFrame::~ReferenceFrame(void) { 
	NO_OP;
}
	
const Vec3&
ReferenceFrame::GetX(void) const
{
	return x; 
}
	
const Mat3x3&
ReferenceFrame::GetR(void) const
{
	return R;
}
	
const Vec3&
ReferenceFrame::GetV(void) const
{
	return v;
}
	
const Vec3&
ReferenceFrame::GetW(void) const
{
	return w;
}

ReferenceFrame&
ReferenceFrame::operator = (const ReferenceFrame& rf)
{
	PutLabel(rf.GetLabel());
	x = rf.x;
	R = rf.R;
	v = rf.v;
	w = rf.w;
	return *this;
}

std::ostream&
ReferenceFrame::Output(std::ostream& out) const
{
	return out 
		<< std::setw(8) << GetLabel() << " "
		<< x << " " << MatR2EulerAngles(R)*dRaDegr << " "
		<< v << " " << w << " " << std::endl;
}

