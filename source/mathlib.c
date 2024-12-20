/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// mathlib.c -- math primitives

#include <math.h>
#include "quakedef.h"

void Sys_Error (char *error, ...);

vec3_t vec3_origin = {0,0,0};
int nanmask = 255<<23;

int  _mathlib_temp_int1, _mathlib_temp_int2, _mathlib_temp_int3;
float _mathlib_temp_float1, _mathlib_temp_float2, _mathlib_temp_float3;
vec3_t _mathlib_temp_vec1, _mathlib_temp_vec2, _mathlib_temp_vec3;

/*-----------------------------------------------------------------*/

float rsqrt( float number )
{
	float d;
    __asm__ (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.s			s000, %1\n"				// s000 = s
		"vrsq.s			s000, s000\n"			// s000 = 1 / sqrt(s000)
		"sv.s			s000, %0\n"				// d    = s000
		".set			pop\n"					// restore assember option
		: "=m"(d)
		: "m"(number)
	);
	return d;
}

/*
=================
SinCos
=================
*/
void SinCos( float radians, float *sine, float *cosine )
{
	vfpu_sincos(radians,sine,cosine);
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabsf( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabsf( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

void LerpVector (const vec3_t from, const vec3_t to, float frac, vec3_t out)
{
	out[0] = from[0] + frac * (to[0] - from[0]);
	out[1] = from[1] + frac * (to[1] - from[1]);
	out[2] = from[2] + frac * (to[2] - from[2]);
}

float VecLength2(vec3_t v1, vec3_t v2)
{
	vec3_t k;
	VectorSubtract(v1, v2, k);
	return sqrt(k[0]*k[0] + k[1]*k[1] + k[2]*k[2]);
} 

#ifdef _WIN32
#pragma optimize( "", off )
#endif


void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy_vfpu( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	zrot[0][0] = vfpu_cosf( DEG2RAD( degrees ) );
	zrot[0][1] = vfpu_sinf( DEG2RAD( degrees ) );
	zrot[1][0] = -vfpu_sinf( DEG2RAD( degrees ) );
	zrot[1][1] = vfpu_cosf( DEG2RAD( degrees ) );

	R_ConcatRotations( m, zrot, tmpmat );
	R_ConcatRotations( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ )
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif

/*-----------------------------------------------------------------*/


float	anglemod(float a)
{
#if 0
	if (a >= 0)
		a -= 360*(int)(a/360);
	else
		a += 360*( 1 + (int)(-a/360) );
#endif
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}

/*
==================
BOPS_Error

Split out like this for ASM to call.
==================
*/
void BOPS_Error (void)
{
	Sys_Error ("BoxOnPlaneSide:  Bad signbits");
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
// crow_bar's enhanced boxonplaneside 
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, mplane_t *p)
{
	int	sides;
	__asm__ (
		".set		push\n"					// save assembler option
		".set		noreorder\n"			// suppress reordering
		"lv.s		S000,  0 + %[normal]\n"	// S000 = p->normal[0]
		"lv.s		S001,  4 + %[normal]\n"	// S001 = p->normal[1]
		"lv.s		S002,  8 + %[normal]\n"	// S002 = p->normal[2]
		"vzero.p	C030\n"					// C030 = [0.0f, 0.0f]
		"lv.s		S032, %[dist]\n"		// S032 = p->dist
		"move		$8,   $0\n"				// $8 = 0
		"beq		%[signbits], $8, 0f\n"	// jump to 0
		"addiu		$8,   $8,   1\n"		// $8 = $8 + 1							( delay slot )
		"beq		%[signbits], $8, 1f\n"	// jump to 1
		"addiu		$8,   $8,   1\n"		// $8 = $8 + 1							( delay slot )
		"beq		%[signbits], $8, 2f\n"	// jump to 2
		"addiu		$8,   $8,   1\n"		// $8 = $8 + 1							( delay slot )
		"beq		%[signbits], $8, 3f\n"	// jump to 3
		"addiu		$8,   $8,   1\n"		// $8 = $8 + 1							( delay slot )
		"beq		%[signbits], $8, 4f\n"	// jump to 4
		"addiu		$8,   $8,   1\n"		// $8 = $8 + 1							( delay slot )
		"beq		%[signbits], $8, 5f\n"	// jump to 5
		"addiu		$8,   $8,   1\n"		// $8 = $8 + 1							( delay slot )
		"beq		%[signbits], $8, 6f\n"	// jump to 6
		"addiu		$8,   $8,   1\n"		// $8 = $8 + 1							( delay slot )
		"beq		%[signbits], $8, 7f\n"	// jump to 7
		"nop\n"								// 										( delay slot )
		"j			8f\n"					// jump to SetSides
		"nop\n"								// 										( delay slot )
	"0:\n"
/*
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
*/
		"lv.s		S010,  0 + %[emaxs]\n"	// S010 = emaxs[0]
		"lv.s		S011,  4 + %[emaxs]\n"	// S011 = emaxs[1]
		"lv.s		S012,  8 + %[emaxs]\n"	// S012 = emaxs[2]
		"lv.s		S020,  0 + %[emins]\n"	// S020 = emins[0]
		"lv.s		S021,  4 + %[emins]\n"	// S021 = emins[1]
		"lv.s		S022,  8 + %[emins]\n"	// S022 = emins[2]
		"vdot.t		S030, C000, C010\n"		// S030 = C000 * C010
		"vdot.t		S031, C000, C020\n"		// S030 = C000 * C020
		"j			8f\n"					// jump to SetSides
		"nop\n"								// 										( delay slot )
	"1:\n"
/*
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
*/
		"lv.s		S010,  0 + %[emins]\n"	// S010 = emins[0]
		"lv.s		S011,  4 + %[emaxs]\n"	// S011 = emaxs[1]
		"lv.s		S012,  8 + %[emaxs]\n"	// S012 = emaxs[2]
		"lv.s		S020,  0 + %[emaxs]\n"	// S020 = emaxs[0]
		"lv.s		S021,  4 + %[emins]\n"	// S021 = emins[1]
		"lv.s		S022,  8 + %[emins]\n"	// S022 = emins[2]
		"vdot.t		S030, C000, C010\n"		// S030 = C000 * C010
		"vdot.t		S031, C000, C020\n"		// S030 = C000 * C020
		"j			8f\n"					// jump to SetSides
		"nop\n"								// 										( delay slot )
	"2:\n"
/*
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
*/
		"lv.s		S010,  0 + %[emaxs]\n"	// S010 = emaxs[0]
		"lv.s		S011,  4 + %[emins]\n"	// S011 = emins[1]
		"lv.s		S012,  8 + %[emaxs]\n"	// S012 = emaxs[2]
		"lv.s		S020,  0 + %[emins]\n"	// S020 = emins[0]
		"lv.s		S021,  4 + %[emaxs]\n"	// S021 = emaxs[1]
		"lv.s		S022,  8 + %[emins]\n"	// S022 = emins[2]
		"vdot.t		S030, C000, C010\n"		// S030 = C000 * C010
		"vdot.t		S031, C000, C020\n"		// S030 = C000 * C020
		"j			8f\n"					// jump to SetSides
		"nop\n"								// 										( delay slot )
	"3:\n"
/*
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
*/
		"lv.s		S010,  0 + %[emins]\n"	// S010 = emins[0]
		"lv.s		S011,  4 + %[emins]\n"	// S011 = emins[1]
		"lv.s		S012,  8 + %[emaxs]\n"	// S012 = emaxs[2]
		"lv.s		S020,  0 + %[emaxs]\n"	// S020 = emaxs[0]
		"lv.s		S021,  4 + %[emaxs]\n"	// S021 = emaxs[1]
		"lv.s		S022,  8 + %[emins]\n"	// S022 = emins[2]
		"vdot.t		S030, C000, C010\n"		// S030 = C000 * C010
		"vdot.t		S031, C000, C020\n"		// S030 = C000 * C020
		"j			8f\n"					// jump to SetSides
		"nop\n"								// 										( delay slot )
	"4:\n"
/*
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
*/
		"lv.s		S010,  0 + %[emaxs]\n"	// S010 = emaxs[0]
		"lv.s		S011,  4 + %[emaxs]\n"	// S011 = emaxs[1]
		"lv.s		S012,  8 + %[emins]\n"	// S012 = emins[2]
		"lv.s		S020,  0 + %[emins]\n"	// S020 = emins[0]
		"lv.s		S021,  4 + %[emins]\n"	// S021 = emins[1]
		"lv.s		S022,  8 + %[emaxs]\n"	// S022 = emaxs[2]
		"vdot.t		S030, C000, C010\n"		// S030 = C000 * C010
		"vdot.t		S031, C000, C020\n"		// S030 = C000 * C020
		"j			8f\n"					// jump to SetSides
		"nop\n"								// 										( delay slot )
	"5:\n"
/*
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
*/
		"lv.s		S010,  0 + %[emins]\n"	// S010 = emins[0]
		"lv.s		S011,  4 + %[emaxs]\n"	// S011 = emaxs[1]
		"lv.s		S012,  8 + %[emins]\n"	// S012 = emins[2]
		"lv.s		S020,  0 + %[emaxs]\n"	// S020 = emaxs[0]
		"lv.s		S021,  4 + %[emins]\n"	// S021 = emins[1]
		"lv.s		S022,  8 + %[emaxs]\n"	// S022 = emaxs[2]
		"vdot.t		S030, C000, C010\n"		// S030 = C000 * C010
		"vdot.t		S031, C000, C020\n"		// S030 = C000 * C020
		"j			8f\n"					// jump to SetSides
		"nop\n"								// 										( delay slot )
	"6:\n"
/*
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
*/
		"lv.s		S010,  0 + %[emaxs]\n"	// S010 = emaxs[0]
		"lv.s		S011,  4 + %[emins]\n"	// S011 = emins[1]
		"lv.s		S012,  8 + %[emins]\n"	// S012 = emins[2]
		"lv.s		S020,  0 + %[emins]\n"	// S020 = emins[0]
		"lv.s		S021,  4 + %[emaxs]\n"	// S021 = emaxs[1]
		"lv.s		S022,  8 + %[emaxs]\n"	// S022 = emaxs[2]
		"vdot.t		S030, C000, C010\n"		// S030 = C000 * C010
		"vdot.t		S031, C000, C020\n"		// S030 = C000 * C020
		"j			8f\n"					// jump to SetSides
		"nop\n"								// 										( delay slot )
	"7:\n"
/*
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
*/
		"lv.s		S010,  0 + %[emins]\n"	// S010 = emins[0]
		"lv.s		S011,  4 + %[emins]\n"	// S011 = emins[1]
		"lv.s		S012,  8 + %[emins]\n"	// S012 = emins[2]
		"lv.s		S020,  0 + %[emaxs]\n"	// S020 = emaxs[0]
		"lv.s		S021,  4 + %[emaxs]\n"	// S021 = emaxs[1]
		"lv.s		S022,  8 + %[emaxs]\n"	// S022 = emaxs[2]
		"vdot.t		S030, C000, C010\n"		// S030 = C000 * C010
		"vdot.t		S031, C000, C020\n"		// S030 = C000 * C020
	"8:\n"									// SetSides
/*
		if( dist1 >= p->dist )
			sides = 1;
		if( dist2 < p->dist )
			sides |= 2;
*/
		"addiu		%[sides], $0, 0\n"		// sides = 0
		"vcmp.s		LT,   S030, S032\n"		// S030 < S032
		"bvt		0,    9f\n"				// if ( CC[0] == 1 ) jump to 9
		"nop\n"								// 										( delay slot )
		"addiu		%[sides], %[sides], 1\n"// sides = 1
	"9:\n"
		"vcmp.s		GE,   S031, S032\n"		// S031 >= S032
		"bvt		0,    10f\n"			// if ( CC[0] == 1 ) jump to 10
		"nop\n"								// 										( delay slot )
		"addiu		%[sides], %[sides], 2\n"// sides = sides + 2
	"10:\n"
		".set		pop\n"					// restore assembler option
		:	[sides]    "=r" ( sides )
		:	[normal]   "m"  (*(p->normal)),
			[emaxs]    "m"  ( *emaxs ),
			[emins]    "m"  ( *emins ),
			[signbits] "r"  ( p->signbits ),
			[dist]     "m"  ( p->dist )
		:	"$8"
	);
	return sides;
}


#if 0 // Baker: this mathlib function doesn't get used in the code anywhere
void vectoangles (vec3_t vec, vec3_t ang)
{
	float	forward, yaw, pitch;

	if (!vec[1] && !vec[0])
	{
		yaw = 0;
		pitch = (vec[2] > 0) ? 90 : 270;
	}
	else
	{
		yaw = atan2 (vec[1], vec[0]) * 180 / M_PI;
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (vec[0]*vec[0] + vec[1]*vec[1]);
		pitch = atan2 (vec[2], forward) * 180 / M_PI;
		if (pitch < 0)
			pitch += 360;
	}

	ang[0] = pitch;
	ang[1] = yaw;
	ang[2] = 0;
}
#endif

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = vfpu_sinf(angle);
	cy = vfpu_cosf(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = vfpu_sinf(angle);
	cp = vfpu_cosf(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = vfpu_sinf(angle);
	cr = vfpu_cosf(angle);

	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;
	right[0] = (-1*sr*sp*cy+-1*cr*-sy);
	right[1] = (-1*sr*sp*sy+-1*cr*cy);
	right[2] = -1*sr*cp;
	up[0] = (cr*sp*cy+-sr*-sy);
	up[1] = (cr*sp*sy+-sr*cy);
	up[2] = cr*cp;
}

void VectorMA (vec3_t veca, float scale, vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

float VectorNormalize (vec3_t v)
{
	float length = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (length)
	{
		const float ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;

}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}


int Q_log2(int val)
{
	int answer=0;
	while (val>>=1)
		answer++;
	return answer;
}


/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}


/*
================
R_ConcatTransforms
================
*/
void R_ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
				in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
				in1[2][2] * in2[2][3] + in1[2][3];
}


/*
===================
FloorDivMod

Returns mathematically correct (floor-based) quotient and remainder for
numer and denom, both of which should contain no fractional part. The
quotient must fit in 32 bits.
====================
//Baker: only software renderer uses this in d_polyse.c
*/

void FloorDivMod (double numer, double denom, int *quotient,
		int *rem)
{
	int		q, r;
	double	x;

#ifndef PARANOID
	if (denom <= 0.0)
		Sys_Error ("FloorDivMod: bad denominator %d\n", denom);

//	if ((floor(numer) != numer) || (floor(denom) != denom))
//		Sys_Error ("FloorDivMod: non-integer numer or denom %f %f\n",
//				numer, denom);
#endif

	if (numer >= 0.0)
	{

		x = floorf(numer / denom);
		q = (int)x;
		r = (int)floorf(numer - (x * denom));
	}
	else
	{
	//
	// perform operations with positive values, and fix mod to make floor-based
	//
		x = floorf(-numer / denom);
		q = -(int)x;
		r = (int)floorf(-numer - (x * denom));
		if (r != 0)
		{
			q--;
			r = (int)denom - r;
		}
	}

	*quotient = q;
	*rem = r;
}


/*
===================
GreatestCommonDivisor
====================
*/
int GreatestCommonDivisor (int i1, int i2)
{
	if (i1 > i2)
	{
		if (i2 == 0)
			return (i1);
		return GreatestCommonDivisor (i2, i1 % i2);
	}
	else
	{
		if (i1 == 0)
			return (i2);
		return GreatestCommonDivisor (i1, i2 % i1);
	}
}


#if	!id386

// TODO: move to nonintel.c

/*
===================
Invert24To16

Inverts an 8.24 value to a 16.16 value
====================
//Baker: Software renderer only?
*/

fixed16_t Invert24To16(fixed16_t val)
{
	if (val < 256)
		return (0xFFFFFFFF);

	return (fixed16_t)
			(((double)0x10000 * (double)0x1000000 / (double)val) + 0.5);
}

#endif

int ParseFloats(char *s, float *f, int *f_size) {
   int i, argc;

   if (!s || !f || !f_size)
      Sys_Error("ParseFloats() wrong params");

   if (f_size[0] <= 0)
      return (f_size[0] = 0); // array have no size, unusual but no crime

   Cmd_TokenizeString(s);
   argc = QMIN(Cmd_Argc(), f_size[0]);
   
   for(i = 0; i < argc; i++)
      f[i] = atof(Cmd_Argv(i));

   for( ; i < f_size[0]; i++)
      f[i] = 0; // zeroing unused elements

   return (f_size[0] = argc);
} 

#ifdef SUPPORTS_AUTOID_SOFTWARE
//This function is GL stylie (use as 2nd arg to ML_MultMatrix4).
float *Matrix4_NewRotation(float a, float x, float y, float z)
{
	static float ret[16];
	float c = cos(a* M_PI / 180.0);
	float s = sin(a* M_PI / 180.0);

	ret[0] = x*x*(1-c)+c;
	ret[4] = x*y*(1-c)-z*s;
	ret[8] = x*z*(1-c)+y*s;
	ret[12] = 0;

	ret[1] = y*x*(1-c)+z*s;
    ret[5] = y*y*(1-c)+c;
	ret[9] = y*z*(1-c)-x*s;
	ret[13] = 0;

	ret[2] = x*z*(1-c)-y*s;
	ret[6] = y*z*(1-c)+x*s;
	ret[10] = z*z*(1-c)+c;
	ret[14] = 0;

	ret[3] = 0;
	ret[7] = 0;
	ret[11] = 0;
	ret[15] = 1;
	return ret;
}

//This function is GL stylie (use as 2nd arg to ML_MultMatrix4).
float *Matrix4_NewTranslation(float x, float y, float z)
{
	static float ret[16];
	ret[0] = 1;
	ret[4] = 0;
	ret[8] = 0;
	ret[12] = x;

	ret[1] = 0;
    ret[5] = 1;
	ret[9] = 0;
	ret[13] = y;

	ret[2] = 0;
	ret[6] = 0;
	ret[10] = 1;
	ret[14] = z;

	ret[3] = 0;
	ret[7] = 0;
	ret[11] = 0;
	ret[15] = 1;
	return ret;
}

//be aware that this generates two sorts of matricies depending on order of a+b
void Matrix4_Multiply(float *a, float *b, float *out)
{
	out[0]  = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
	out[1]  = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
	out[2]  = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
	out[3]  = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];

	out[4]  = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
	out[5]  = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
	out[6]  = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
	out[7]  = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];

	out[8]  = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
	out[9]  = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
	out[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
	out[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];

	out[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
	out[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
	out[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
	out[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
}

//transform 4d vector by a 4d matrix.
void Matrix4_Transform4(float *matrix, float *vector, float *product)
{
	product[0] = matrix[0]*vector[0] + matrix[4]*vector[1] + matrix[8]*vector[2] + matrix[12]*vector[3];
	product[1] = matrix[1]*vector[0] + matrix[5]*vector[1] + matrix[9]*vector[2] + matrix[13]*vector[3];
	product[2] = matrix[2]*vector[0] + matrix[6]*vector[1] + matrix[10]*vector[2] + matrix[14]*vector[3];
	product[3] = matrix[3]*vector[0] + matrix[7]*vector[1] + matrix[11]*vector[2] + matrix[15]*vector[3];
}

void ML_ProjectionMatrix(float *proj, float wdivh, float fovy)
{
	float xmin, xmax, ymin, ymax;
	float nudge = 1;

	//proj
	ymax = 4 * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * wdivh;
	xmax = ymax * wdivh;

	proj[0] = (2*4) / (xmax - xmin);
	proj[4] = 0;
	proj[8] = (xmax + xmin) / (xmax - xmin);
	proj[12] = 0;

	proj[1] = 0;
	proj[5] = (2*4) / (ymax - ymin);
	proj[9] = (ymax + ymin) / (ymax - ymin);
	proj[13] = 0;

	proj[2] = 0;
	proj[6] = 0;
	proj[10] = -1  * nudge;
	proj[14] = -2*4 * nudge;

	proj[3] = 0;
	proj[7] = 0;
	proj[11] = -1;
	proj[15] = 0;
}

void ML_ModelViewMatrix(float *modelview, vec3_t viewangles, vec3_t vieworg)
{
	float tempmat[16];
	//load identity.
	memset(modelview, 0, sizeof(*modelview)*16);
#if 1
	modelview[0] = 1;
	modelview[5] = 1;
	modelview[10] = 1;
	modelview[15] = 1;

	Matrix4_Multiply(modelview, Matrix4_NewRotation(-90,  1, 0, 0), tempmat);	    // put Z going up
	Matrix4_Multiply(tempmat, Matrix4_NewRotation(90,  0, 0, 1), modelview);	    // put Z going up
#else
	//use this lame wierd and crazy identity matrix..
	modelview[2] = -1;
	modelview[4] = -1;
	modelview[9] = 1;
	modelview[15] = 1;
#endif
	//figure out the current modelview matrix

	//I would if some of these, but then I'd still need a couple of copys
	Matrix4_Multiply(modelview, Matrix4_NewRotation(-viewangles[2],  1, 0, 0), tempmat);
	Matrix4_Multiply(tempmat, Matrix4_NewRotation(-viewangles[0],  0, 1, 0), modelview);
	Matrix4_Multiply(modelview, Matrix4_NewRotation(-viewangles[1],  0, 0, 1), tempmat);

	Matrix4_Multiply(tempmat, Matrix4_NewTranslation(-vieworg[0],  -vieworg[1],  -vieworg[2]), modelview);	    // put Z going up
}



//returns fractions of screen.
//uses GL style rotations and translations and stuff.
//3d -> screen (fixme: offscreen return values needed)
void ML_Project (vec3_t in, vec3_t out, vec3_t viewangles, vec3_t vieworg, float wdivh, float fovy)
{
	float modelview[16];
	float proj[16];

	ML_ModelViewMatrix(modelview, viewangles, vieworg);
	ML_ProjectionMatrix(proj, wdivh, fovy);

	{
		float v[4], tempv[4];
		v[0] = in[0];
		v[1] = in[1];
		v[2] = in[2];
		v[3] = 1;

		Matrix4_Transform4(modelview, v, tempv);
		Matrix4_Transform4(proj, tempv, v);

		v[0] /= v[3];
		v[1] /= v[3];
		v[2] /= v[3];

		out[0] = (1+v[0])/2;
		out[1] = (1+v[1])/2;
		out[2] = (1+v[2])/2;
	}
}

#endif
