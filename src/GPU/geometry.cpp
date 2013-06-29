/**
 *	@file geometry.cpp
 *  @brief The Geometry related GPU function module
 *  @author Liou Jhe-Yu(lioujheyu@gmail.com)
 */

#include "gpu_core.h"

void GPU_Core::InitPrimitiveAssembly()
{
    primitiveRdy = false;
    stripIndicator = false;

    switch (drawMode) {
    case GL_TRIANGLES:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP:
        vtxCntDwn = 3;
        break;
    case GL_LINES:
    case GL_LINE_LOOP:
    case GL_LINE_STRIP:
        vtxCntDwn = 2;
        break;
    default:
        vtxCntDwn = 1;
    }
}

/****** Grahphic Related Function ******/

void GPU_Core::VertexShaderEXE(int sid, void *input)
{
	totalVtx++;

	sCore[sid].instPool = VSinstPool;
	sCore[sid].instCnt = VSinstCnt;
	sCore[sid].uniformPool = uniformPool;
	sCore[sid].shaderType = VERTEX_SHADER;

	sCore[sid].Init();
	sCore[sid].input = input;
	sCore[sid].Exec();
}

void GPU_Core::PerspectiveDivision()
{
	float w = 1.0/curVtx.attr[0].w;
	curVtx.attr[0].w = 1.0;

	for (int i=0; i<MAX_ATTRIBUTE_NUMBER; i++) {
		if (attrEnable[i] == false)
			continue;
		else
			curVtx.attr[i] = curVtx.attr[i] * w;
	}
}

void GPU_Core::ViewPort()
{
    float x,y,z;

    x = curVtx.attr[0].x;
    y = curVtx.attr[0].y;
    z = curVtx.attr[0].z;

    x = x*viewPortW/2 + viewPortLX + viewPortW/2;
    y = y*viewPortH/2 + viewPortLY + viewPortH/2;
    z = z*(depthRangeF-depthRangeN)/2 + (depthRangeF+depthRangeN)/2;

    curVtx.attr[0].x = x;
    curVtx.attr[0].y = y;
    curVtx.attr[0].z = z;
}

void GPU_Core::PrimitiveAssembly()
{
    switch (drawMode) {
    case GL_TRIANGLES:
            prim.v[vtxCntDwn-1] = curVtx;
        break;

	/*	Vertex order in Strip mode
	 *	-1, 0, 1
	 *	 0, 2, 1
	 *	 1, 2, 3
	 *	 2, 4, 3
	 *	 3, 4, 5
	 *	 4, 6, 5
	 */
    case GL_TRIANGLE_STRIP:
    	if (vtxCntDwn == 1) {
			if (stripIndicator == false) {
				prim.v[2] = prim.v[0];
				prim.v[0] = curVtx;
				stripIndicator = true;
			}
			else {
				prim.v[2] = prim.v[1];
				prim.v[1] = curVtx;
				stripIndicator = false;
			}
		}
    	else if (vtxCntDwn == 3)
			prim.v[0] = curVtx;
		else if (vtxCntDwn == 2)
			prim.v[1] = curVtx;
        break;

	/*	Vertex order in Fan mode
	 *	1, 0, 0
	 *	1, 0, 2
	 *	1, 2, 3
	 *	1, 3, 4
	 *	1, 4, 5
	 *	1, 5, 6
	 */
    case GL_TRIANGLE_FAN:
        if (vtxCntDwn == 1) {
			prim.v[1] = prim.v[0];
			prim.v[0] = curVtx;
        }
        else if (vtxCntDwn == 3)
			prim.v[2] = curVtx;
		else if (vtxCntDwn == 2)
            prim.v[0] = curVtx;
        break;
    default:
    	printf("GPU_Core: Unsurpport draw mode: %x\n",drawMode);
    	exit(1);
        break;
    }

    vtxCntDwn--;

    if (vtxCntDwn == 0)
    {
		totalPrimitive++;
        primitiveRdy = true;

        switch (drawMode) {
        case GL_TRIANGLES:
            vtxCntDwn = 3;
            break;
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
            vtxCntDwn = 1;
            break;
        default:
        	printf("GPU_Core: Unsurpport draw mode: %x",drawMode);
			exit(1);
            break;
        }
    }
}


