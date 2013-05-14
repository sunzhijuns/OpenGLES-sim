/**
 *  @file GPU Top module
 *
 *  @author Liou Jhe-Yu(lioujheyu@gmail.com)
 *
 */

#ifndef GPU_CORE_H_INCLUDED
#define GPU_CORE_H_INCLUDED

#include "geometry.h"
#include "rasterizer.h"

#endif // GPU_CORE_H_INCLUDED

class GPU_Core
{
public:
    Geometry    gm;
    Rasterizer rm;

    int         vtxCount;
    int         vtxFirst;
    const void  *vtxPointer[MAX_VARYING_NUMBER];
    int         attribSize[MAX_VARYING_NUMBER];
    bool        varyingEnable[MAX_VARYING_NUMBER];
    int         vtxPosIndx;

    void        Run();
};

extern GPU_Core gpu;
