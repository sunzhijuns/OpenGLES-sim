/*
 * Copyright (c) 2013, Liou Jhe-Yu <lioujheyu@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 *	@file texture_unit.cpp
 *  @brief TextureUnit class implementation
 *  @author Liou Jhe-Yu(lioujheyu@gmail.com)
 */

#include "texture_unit.h"

void TextureUnit::ClearTexCache()
{
	for(int i=0; i<TEX_CACHE_ENTRY_SIZE; i++) {
		TexCache.valid[i][0] =
		TexCache.valid[i][1] =
		TexCache.valid[i][2] =
		TexCache.valid[i][3] = false;
		TexCache.RRFlag[i] = 0;
	}
	hit = 0;
	miss = 0;
	coldMiss = 0;
}

int TextureUnit::CalcTexAdd(short int us, short int ub, short int uo,
							short int vs, short int vb, short int vo,
							int width)
{
	return (vs*TEX_CACHE_BLOCK_SIZE_ROOT*TEX_CACHE_ENTRY_SIZE_ROOT
			+ vb*TEX_CACHE_BLOCK_SIZE_ROOT
			+ vo ) * width
			+ us*TEX_CACHE_BLOCK_SIZE_ROOT*TEX_CACHE_ENTRY_SIZE_ROOT
			+ ub*TEX_CACHE_BLOCK_SIZE_ROOT
			+ uo;
}

floatVec4 TextureUnit::GetTexColor(const floatVec4 &coordIn, int level, int tid)
{
	uint8_t *texTmpPtr = NULL;
	uint32_t tmpData;
	unsigned short u,v;

	if (targetImage->maxLevel == -1) {
		fprintf(stderr,"TexUnit: Using Null tex2D\n");
		return floatVec4(0.0, 0.0, 0.0, 0.0);
	}
	else if (level > targetImage->maxLevel) {
		fprintf(stderr,
				"TexUnit: %d exceed maximum level %d\n",
				level,
				targetImage->maxLevel );
		return floatVec4(0.0, 0.0, 0.0, 0.0);
	}

	u = (unsigned short)coordIn.s;
	v = (unsigned short)coordIn.t;

	if (u > targetImage->widthLevel[level] ||
		v > targetImage->heightLevel[level]) {
		fprintf(stderr,
				"TexUnit: (%d,%d)Out of the texture image's bound(%d,%d)\n",
				u, v,
				targetImage->widthLevel[level],
				targetImage->heightLevel[level] );
		return floatVec4(0.0, 0.0, 0.0, 0.0);
	}

#ifdef NO_TEX_CACHE
	floatVec4 color;

	texTmpPtr = targetImage->data[level] +
				(v*targetImage->widthLevel[level] + u)*4;

	dram->LocalAccess(false, (size_t)texTmpPtr, tmpData, 4, 1);
	color.r = (float)(tmpData&0xff)/255;
	color.g = (float)((tmpData>>8)&0xff)/255;
	color.b = (float)((tmpData>>16)&0xff)/255;
	color.a = (float)((tmpData>>24)&0xff)/255;

	texTmpPtr+=4;

	return color;
#else
	int i,j; //loop counter
	uint32_t tag;
	uint16_t entry, offset, U_Block, V_Block, U_Offset, V_Offset, U_Super, V_Super;
	uint8_t tWay = 0;
    bool isColdMiss = false;

/**
 *	While level image size is smaller than cache block size, the 6D texture
 *	addressing mode will be no longer required because all texel on this level
 *	are guaranteed fall into a single cache block so that sequential access will
 *	be enough. However, there will be more than one level image living on this
 *	cache block in such situation. For example, if a texture has size in 32x32
 *	and the cache block size is 64. So we have mip-map in
 *	level 0: 32x32
 *	level 1: 16x16
 *	level 2: 8x8
 *	...
 *	All level beyond 1 will actually have image size smaller than single cache
 *	block. For the reason to save memory usage. We can put level 2~4 in a single
 *	cache block ( (4x4 + 2x2 + 1x1 = 21) < 64).
 *
 *	Therefore, as we access these level images, we have to generate cache offset
 *	differ than normal since there are multiple level inside the cache block.
 *	So does tags and entry. (But these two are much more easy to calculate)
 */
	if (targetImage->heightLevel[level] >= TEX_CACHE_BLOCK_SIZE_ROOT) {
		U_Super = u >> (TEX_CACHE_BLOCK_SIZE_ROOT_LOG + TEX_CACHE_ENTRY_SIZE_ROOT_LOG);
		V_Super = v >> (TEX_CACHE_BLOCK_SIZE_ROOT_LOG + TEX_CACHE_ENTRY_SIZE_ROOT_LOG);
		U_Block = u >> (TEX_CACHE_BLOCK_SIZE_ROOT_LOG) & (TEX_CACHE_ENTRY_SIZE_ROOT - 1);
		V_Block = v >> (TEX_CACHE_BLOCK_SIZE_ROOT_LOG) & (TEX_CACHE_ENTRY_SIZE_ROOT - 1);
		U_Offset = u & (TEX_CACHE_BLOCK_SIZE_ROOT - 1);
		V_Offset = v & (TEX_CACHE_BLOCK_SIZE_ROOT - 1);
		tag = (int)( (V_Super << 12) | (U_Super&0x0fff) );

		///@note Simply append all texture related selection bit after tag bit
		tag = (tag << 3) | (imageSelection&0x7);
		tag = (tag << 4) | (level&0xf);
		tag = (tag << 1) | (tid&0x1);

		entry = V_Block * TEX_CACHE_ENTRY_SIZE_ROOT + U_Block;
		offset = V_Offset * TEX_CACHE_BLOCK_SIZE_ROOT + U_Offset;
	}
	else { //targetImage->heightLevel[levelCount] < TEX_CACHE_BLOCK_SIZE_ROOT
        tag = imageSelection&0x7;
        tag = (tag << 4) | ((targetImage->maxLevel - TEX_CACHE_BLOCK_SIZE_ROOT_LOG)&0xf);
        tag = (tag << 1) | (tid&0x1);

        entry = 0;
        offset = 0;
        for (i = TEX_CACHE_BLOCK_SIZE_ROOT_LOG-1;
			 i > (int)log2(targetImage->heightLevel[level]);
			 i--) {
			//printf("%d", (int)log2(targetImage->heightLevel[level]));
			offset += 1<<(i*2);
        }
        offset += (v*targetImage->widthLevel[level] + u);
	}

	for (i=0; i<TEX_WAY_ASSOCIATION; i++) {
		if (TexCache.valid[entry][i] == true) {
			if (TexCache.tag[entry][i] == tag) {
			//*************** Texture cache hit *************
				hit++;
				return TexCache.color[entry][offset][i];
			}
		}
		else
			isColdMiss = true;
	}

	//*********** Texture cache miss ****************
	miss++;

	tWay = TexCache.RRFlag[entry] % TEX_WAY_ASSOCIATION;
	TexCache.RRFlag[entry]++;
	TexCache.tag[entry][tWay] = tag;

	if (TexCache.valid[entry][tWay] == false)
		TexCache.valid[entry][tWay] = true;
#	ifdef IMAGE_MEMORY_OPTIMIZE
	if (targetImage->heightLevel[level] >= TEX_CACHE_BLOCK_SIZE_ROOT)
		texTmpPtr = targetImage->data[level] +
		( (v>>TEX_CACHE_BLOCK_SIZE_ROOT_LOG)*(targetImage->widthLevel[level]>>TEX_CACHE_BLOCK_SIZE_ROOT_LOG)
		+ (u>>TEX_CACHE_BLOCK_SIZE_ROOT_LOG) )*TEX_CACHE_BLOCK_SIZE*4;
	else //targetImage->heightLevel[levelCount] < TEX_CACHE_BLOCK_SIZE_ROOT
		texTmpPtr = targetImage->data[(targetImage->maxLevel - TEX_CACHE_BLOCK_SIZE_ROOT_LOG)&0xf];

	for (i=0; i<TEX_CACHE_BLOCK_SIZE; i++) {
		if (TEX_CACHE_BLOCK_SIZE > 16) // Limit maximum burst length to 16
			dram->LocalAccess(false, (size_t)texTmpPtr+i*4, tmpData, 4, 16-(i&0xf));
		else
			dram->LocalAccess(false, (size_t)texTmpPtr+i*4, tmpData, 4, TEX_CACHE_BLOCK_SIZE-i);
		TexCache.color[entry][i][tWay].r = (float)(tmpData&0xff)/255;
		TexCache.color[entry][i][tWay].g = (float)((tmpData>>8)&0xff)/255;
		TexCache.color[entry][i][tWay].b = (float)((tmpData>>16)&0xff)/255;
		TexCache.color[entry][i][tWay].a = (float)((tmpData>>24)&0xff)/255;
	}
#	else
    if (targetImage->heightLevel[level] >= TEX_CACHE_BLOCK_SIZE_ROOT) {
		for (j=0; j<TEX_CACHE_BLOCK_SIZE_ROOT; j++) {
			for (i=0; i<TEX_CACHE_BLOCK_SIZE_ROOT; i++) {
				texTmpPtr = targetImage->data[level] +
							CalcTexAdd(U_Super,U_Block,i,
									V_Super,V_Block,j,
									targetImage->widthLevel[level]) * 4;
				if (TEX_CACHE_BLOCK_SIZE_ROOT > 16) // Limit maximum burst length to 16
					dram->LocalAccess(false, (size_t)texTmpPtr, tmpData, 4, 16-(i&0xf));
				else
					dram->LocalAccess(false, (size_t)texTmpPtr, tmpData, 4, TEX_CACHE_BLOCK_SIZE_ROOT-i);
//				dram->LocalAccess(false, (size_t)texTmpPtr, tmpData, 4, 1);
				TexCache.color[entry][j*TEX_CACHE_BLOCK_SIZE_ROOT+i][tWay].r =
					(float)(tmpData&0xff)/255;
				TexCache.color[entry][j*TEX_CACHE_BLOCK_SIZE_ROOT+i][tWay].g =
					(float)((tmpData>>8)&0xff)/255;
				TexCache.color[entry][j*TEX_CACHE_BLOCK_SIZE_ROOT+i][tWay].b =
					(float)((tmpData>>16)&0xff)/255;
				TexCache.color[entry][j*TEX_CACHE_BLOCK_SIZE_ROOT+i][tWay].a =
					(float)((tmpData>>24)&0xff)/255;

				texTmpPtr+=4;
			}
		}
    }
    else { //targetImage->heightLevel[levelCount] < TEX_CACHE_BLOCK_SIZE_ROOT
		texTmpPtr = targetImage->data[(targetImage->maxLevel - TEX_CACHE_BLOCK_SIZE_ROOT_LOG)&0xf];

		for (i=0; i<TEX_CACHE_BLOCK_SIZE; i++) {
			if (TEX_CACHE_BLOCK_SIZE > 16) // Limit maximum burst length to 16
				dram->LocalAccess(false, (size_t)texTmpPtr+i*4, tmpData, 4, 16-(i&0xf));
			else
				dram->LocalAccess(false, (size_t)texTmpPtr+i*4, tmpData, 4, TEX_CACHE_BLOCK_SIZE-i);
			TexCache.color[entry][i][tWay].r = (float)(tmpData&0xff)/255;
			TexCache.color[entry][i][tWay].g = (float)((tmpData>>8)&0xff)/255;
			TexCache.color[entry][i][tWay].b = (float)((tmpData>>16)&0xff)/255;
			TexCache.color[entry][i][tWay].a = (float)((tmpData>>24)&0xff)/255;
		}
    }
#	endif // IMAGE_MEMORY_OPTIMIZE

#	ifdef SHOW_TEXCACHE_COLD_MISS
	if (isColdMiss)
		return floatVec4(1.0, 0.0, 0.0, 1.0);
#	endif //SHOW_TEXCACHE_COLD_MISS

#	if defined(SHOW_TEXCACHE_MISS)
	return floatVec4(0.0, 1.0, 0.0, 1.0);
#	endif //SHOW_TEXCACHE_MISS

	return TexCache.color[entry][offset][tWay];

#endif // NO_TEX_CACHE
}

floatVec4 TextureUnit::TexCoordWrap(const floatVec4 &coordIn, int level, int tid)
{
	floatVec4 temp;

	switch (wrapS[tid]){
	case GL_REPEAT:
		/*	The function's behavior of "fmod" in C++ is not matched
		 *	with OpenGL.
		 *	fmod(x,y) in C++: x - y * trunc(x/y);
		 *	fmod(x,y) in OpenGL: x - y * floor(x/y);
		 */
//		temp.s = fmod(coordIn.s,targetImage->widthLevel[level]);
		temp.s = coordIn.s - targetImage->widthLevel[level] *
				 floor(coordIn.s/targetImage->widthLevel[level]);
		break;
	case GL_CLAMP_TO_EDGE:
		temp.s = CLAMP(coordIn.s, 0.0f, (float)targetImage->widthLevel[level]-1);
		break;
	default:
		fprintf(stderr, "TexUnit: Unknown Texture Wrap mode in x-axis!!\n");
		break;
	}

	switch (wrapT[tid]){
	case GL_REPEAT:
//		temp.t = fmod(coordIn.t,targetImage->heightLevel[level]);
		temp.t = coordIn.t - targetImage->heightLevel[level] *
				 floor(coordIn.t/targetImage->heightLevel[level]);
		break;
	case GL_CLAMP_TO_EDGE:
		temp.t = CLAMP(coordIn.t, 0.0f, (float)targetImage->widthLevel[level]-1);
		break;
	default:
		fprintf(stderr, "TexUnit: Unknown Texture Wrap mode in y-axis!!\n");
		break;
	}

	return temp;
}

floatVec4 TextureUnit::BilinearFilter(const floatVec4 &coordIn,int level, int tid)
{
	// coord[4]: 2 3
	//			 0 1
	floatVec4 coordLOD[4];

	float u_ratio, v_ratio;
	floatVec4 color;
	floatVec4 TexColor[4];

	double intPart; //No use

	coordLOD[0].s = coordIn.s / (1<<level) - 0.5;
	coordLOD[0].t = coordIn.t / (1<<level) - 0.5;

	coordLOD[1] =  coordLOD[2] = coordLOD[3] = coordLOD[0];
	coordLOD[1].s = coordLOD[1].s + 1;
	coordLOD[2].t = coordLOD[2].t + 1;
	coordLOD[3].s = coordLOD[3].s + 1;
	coordLOD[3].t = coordLOD[3].t + 1;
	coordLOD[0] = TexCoordWrap(coordLOD[0], level, tid);
	coordLOD[1] = TexCoordWrap(coordLOD[1], level, tid);
	coordLOD[2] = TexCoordWrap(coordLOD[2], level, tid);
	coordLOD[3] = TexCoordWrap(coordLOD[3], level, tid);


	TexColor[0] = GetTexColor(coordLOD[0], level, tid);
	TexColor[1] = GetTexColor(coordLOD[1], level, tid);
	TexColor[2] = GetTexColor(coordLOD[2], level, tid);
	TexColor[3] = GetTexColor(coordLOD[3], level, tid);

	u_ratio = modf(coordLOD[0].s, &intPart);
	v_ratio = modf(coordLOD[0].t, &intPart);

	color = (TexColor[0]*(1-u_ratio) + TexColor[1]*u_ratio)*(1-v_ratio) +
			(TexColor[2]*(1-u_ratio) + TexColor[3]*u_ratio)*v_ratio;

	return color;
}

floatVec4 TextureUnit::TrilinearFilter(const floatVec4 &coordIn,
									   const int level,
									   const float w_ratio,
									   const int tid )
{
	floatVec4 color[2];
	int maxLevel = targetImage->maxLevel;

	if (w_ratio <= 0.01)
		color[0] = BilinearFilter(coordIn, level, tid);
	else if (w_ratio >= 0.99)
		color[0] = BilinearFilter(coordIn, std::min(level+1, maxLevel), tid);
	else {
		color[0] = BilinearFilter(coordIn, level, tid);
		color[1] = BilinearFilter(coordIn, std::min(level+1, maxLevel), tid);
		color[0] = color[0]*(1-w_ratio) + color[1]*w_ratio;
	}

	return color[0];
}

floatVec4 TextureUnit::TextureSample(const floatVec4 &coordIn,
									 float level,
									 const floatVec4 &scaleFacDX,
									 const floatVec4 &scaleFacDY,
									 int targetType,
									 int tid )
{
	floatVec4 coord;
	float w_ratio;
	float maxScaleFac, maxScaleFacX, maxScaleFacY, scaleFacLoser;
	floatVec4 TexColor[2];
	floatVec4 color, colorAni;
	floatVec4 mainAxis;
	int LoD, maxLevel;
	floatVec4 deltaDX, deltaDY;

	//find absolutely coordinate in texture image
	switch (targetType) {
	case TT_2D:
		imageSelection = TEX_2D;
		targetImage = &tex2D[tid];
		coord.s = coordIn.s;
		coord.t = coordIn.t;
		break;
	case TT_CUBE:
		if (fabs(coordIn.s) > fabs(coordIn.t)) {
			if (fabs(coordIn.s) > fabs(coordIn.p)) {
				imageSelection = (coordIn.s<0.0f)? CUBE_NEG_X : CUBE_POS_X;
				targetImage = (coordIn.s<0.0f)? &texCubeNX[tid] : &texCubePX[tid];
				coord.s = ((coordIn.s<0.0f)? coordIn.p : -coordIn.p)/fabs(coordIn.s)/2 + 0.5f;
				coord.t = -coordIn.t/fabs(coordIn.s)/2 + 0.5f;
			}
			else { // |coordIn.p| > |coordIn.s|
				imageSelection = (coordIn.p<0.0f)? CUBE_NEG_Z : CUBE_POS_Z;
				targetImage = (coordIn.p<0.0f)? &texCubeNZ[tid] : &texCubePZ[tid];
				coord.s = ((coordIn.p<0.0f)? -coordIn.s : coordIn.s)/fabs(coordIn.p)/2 + 0.5f;
				coord.t = -coordIn.t/fabs(coordIn.p)/2 + 0.5f;
			}
		}
		else { // |coordIn.t| > |coordIn.s|
			if (fabs(coordIn.t) > fabs(coordIn.p)) {
				imageSelection = (coordIn.t<0.0f)? CUBE_NEG_Y : CUBE_POS_Y;
				targetImage = (coordIn.t<0.0f)? &texCubeNY[tid] : &texCubePY[tid];
				coord.s = coordIn.s/fabs(coordIn.t)/2 + 0.5f;
				coord.t = ((coordIn.t<0.0f)? -coordIn.p : coordIn.p)/fabs(coordIn.t)/2 + 0.5f;
			}
			else { // |coordIn.p| > |coordIn.t|
				imageSelection = (coordIn.p<0.0f)? CUBE_NEG_Z : CUBE_POS_Z;
				targetImage = (coordIn.p<0.0f)? &texCubeNZ[tid] : &texCubePZ[tid];
				coord.s = ((coordIn.p<0.0f)? -coordIn.s : coordIn.s)/fabs(coordIn.p)/2 + 0.5f;
				coord.t = -coordIn.t/fabs(coordIn.p)/2 + 0.5f;
			}
		}
		break;
	default:
		printf("Tex Unit: undefined or unimplemented texture target\n");
		return floatVec4(0.0, 0.0, 0.0, 0.0);
		break;
	}
	coord.s = coord.s*targetImage->widthLevel[0];
	coord.t = coord.t*targetImage->heightLevel[0];
	coord.p = coordIn.p;
	coord.q = coordIn.q;

	maxLevel = targetImage->maxLevel;

	if (level < 0) { // Normal scale factor and LoD calculation
		deltaDX.s = scaleFacDX.s*targetImage->widthLevel[0];
		deltaDX.t = scaleFacDX.t*targetImage->heightLevel[0];
		deltaDY.s = scaleFacDY.s*targetImage->widthLevel[0];
		deltaDY.t = scaleFacDY.t*targetImage->heightLevel[0];

		maxScaleFacX = std::max(std::abs(deltaDX.s), std::abs(deltaDX.t));
		maxScaleFacY = std::max(std::abs(deltaDY.s), std::abs(deltaDY.t));
//		maxScaleFacX = sqrt(deltaDX.s*deltaDX.s + deltaDX.t*deltaDX.t);
//		maxScaleFacY = sqrt(deltaDY.s*deltaDY.s + deltaDY.t*deltaDY.t);
		if (maxScaleFacX > maxScaleFacY) {
			mainAxis = deltaDX;
			maxScaleFac = maxScaleFacX;
			scaleFacLoser = maxScaleFacY;
		}
		else {
			mainAxis = deltaDY;
			maxScaleFac = maxScaleFacY;
			scaleFacLoser = maxScaleFacX;
		}
		w_ratio = frexp(maxScaleFac, &LoD);
		w_ratio = w_ratio*2-1;
		LoD--;
	}
	else { // LoD specified
		/* Always using minification filter, or the the effect of level of
		   detail will be lost. (because magnification filter doesn't use
		   level of detail)
		*/
		maxScaleFac = 2.0f; //force using minification filter
		LoD = (int)floor(level);
		w_ratio = level - LoD;
	}

	LoD = CLAMP(LoD, 0, maxLevel);

	if(maxScaleFac>1) {
		switch (minFilter[tid]) {
		case GL_NEAREST:    //u,v nearest filter
			coord = TexCoordWrap(coord, 0, tid);
			color = GetTexColor(coord, 0, tid);
			break;

		case GL_LINEAR:     //u,v bilinear filter
			color = BilinearFilter(coord, 0, tid);
			break;

		case GL_NEAREST_MIPMAP_NEAREST: //u,v,w nearest filter
			coord = TexCoordWrap(coord, 0, tid);
			coord.s = coord.s / (1<<LoD);
			coord.t = coord.t / (1<<LoD);
			color = GetTexColor(coord, LoD, tid);
			break;

		case GL_LINEAR_MIPMAP_NEAREST:  //u,v bilinear, w nearest filter
			color = BilinearFilter(coord, LoD, tid);
			break;

		case GL_NEAREST_MIPMAP_LINEAR:  //u,v nearest, w linear filter
			coord = TexCoordWrap(coord, 0, tid);
			coord.s = coord.s / (1<<LoD);
			coord.t = coord.t / (1<<LoD);
			TexColor[0] = GetTexColor(coord, LoD, tid);

			if (LoD+1 < maxLevel) {
				coord.s = coord.s / 2;
				coord.t = coord.t / 2;
				TexColor[1] = GetTexColor(coord, LoD, tid);
				color = TexColor[0]*(1-w_ratio) + TexColor[1]*w_ratio;
			}
			else
				color = TexColor[0];
			break;

		case GL_LINEAR_MIPMAP_LINEAR:	//u,v,w trilinear filter
			uint8_t sampleN;

			/* Extension defines the sample number have to apply ceiling on the
			 * ratio but I use floor to get a smaller ratio for better
			 * performance. The result image seems OK so far, but have to aware
			 * any unusual if it is just happened. Maybe this is why most GFX
			 * has the configuration between quality and performance about
			 * anisotropic filter.
			 */
			if (level < 0)
				sampleN = std::min((uint8_t)floor(maxScaleFac/scaleFacLoser),
									maxAnisoFilterRatio);
			else // level of detail has been specified, perform isometric filter
				sampleN = 1;

			// Round down to nearest power of 2, the same reason mentioned above
			if (sampleN == 3)
				sampleN = 2;
			else if (sampleN > 4 && sampleN < 8)
				sampleN = 4;

			if (sampleN == 1) { //perform isometric filter only
				color = TrilinearFilter(coord, LoD, w_ratio, tid);
				break;
			}

			maxScaleFac = maxScaleFac/sampleN;

			w_ratio = frexp(maxScaleFac, &LoD);
			w_ratio = w_ratio*2-1;
			LoD--;
			LoD = CLAMP(LoD, 0, maxLevel);

			color = floatVec4(0.0, 0.0, 0.0, 0.0);
			for (int i=0; i<sampleN; i++) {
				if (sampleN == 8)
					colorAni = TrilinearFilter(coord + mainAxis*(2*i - 7)/16,
												  LoD, w_ratio, tid);
				else if (sampleN == 4)
					colorAni = TrilinearFilter(coord + mainAxis*(2*i - 3)/8,
												  LoD, w_ratio, tid);
				else if (sampleN == 2)
					colorAni = TrilinearFilter(coord + mainAxis*(2*i - 1)/4,
												  LoD, w_ratio, tid);

				color = color + colorAni;
			}
			color = color/sampleN;

			break;
		}
	} else {
		switch (magFilter[tid]) {
		case GL_NEAREST:    //u,v nearest filter
			coord = TexCoordWrap(coord, 0, tid);
			color = GetTexColor(coord ,0, tid);
			break;

		case GL_LINEAR:     //u,v bilinear filter
			color = BilinearFilter(coord,0, tid);
			break;
		}
	}

	TEXPRINTF("%7.2f %7.2f\t", coord.s,  coord.t);
	//TEXPRINTF("  %2x %2x %2x %2x",color.r,color.g,color.b,color.a);
	TEXPRINTF("%2d %3.2f\t\t",LoD, w_ratio);
	TEXPRINTF("%3.2f %3.2f %3.2f %3.2f\n",
			  scaleFacDX.s*targetImage->widthLevel[LoD],
			  scaleFacDX.t*targetImage->heightLevel[LoD],
			  scaleFacDY.s*targetImage->widthLevel[LoD],
			  scaleFacDY.t*targetImage->heightLevel[LoD]);

	return color;
}
