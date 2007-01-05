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

#ifndef CRYPT_H
#define CRYPT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef NEED_CRYPT_DECL
/*
 * FIXME: sometimes it is not defined even if present
 *
 * TODO: automate check in configure
 */
char *crypt(const char *key, const char *salt);
#endif /* NEED_CRYPT_DECL */

extern char *
mbdyn_make_salt(char *salt, size_t saltlen, const char *salt_format);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CRYPT_H */
