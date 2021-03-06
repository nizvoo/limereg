/* =====================================
=== LIMEREG - Lightweight Image Registration ===
========================================

Forked from the project FIMREG, which was written for a distributed calculation on the PCIe card DSPC-8681 of Advantech. LIMEREG does not use DSPs and can
be run on an ordinary PC without special hardware. FIMREG was originally developed by by Roelof Berg, Berg Solutions (rberg@berg-solutions.de) with support
from Lars Koenig, Fraunhofer MEVIS (lars.koenig@mevis.fraunhofer.de) and Jan Ruehaak, Fraunhofer MEVIS (jan.ruehaak@mevis.fraunhofer.de).

THIS IS A LIMITED RESEARCH PROTOTYPE. Documentation: www.berg-solutions.de/limereg.html

------------------------------------------------------------------------------

Copyright (c) 2014, Roelof Berg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the owner nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------*/

//precompiled header (MUST BE THE FIRST ENTRY IN EVERY CPP FILE)
#include "stdafx.h"

#include "limereg_common.h"
#include "CRegistrator.h"

#include "matlab/codegeneration/gaussnewton.h"
#include "matlab/codegeneration/transform.h"
#include "matlab/codegeneration/diffimg.h"
#include "matlab/codegeneration/limereg_initialize.h"
#include "matlab/codegeneration/limereg_terminate.h"
#include "matlab/codegeneration/limereg_emxutil.h"

#include "CMatlabArray.h"

#include "matlab/pseudo_stdafx.h"                    //stdafx not possible because of include position of matlab

//(Global ugliness came originally from the matlab c callbacks, search for g_CHPRPCConnection to understand this)

namespace Limereg {

CRegistrator::CRegistrator()
{
	//Matlab initialisieren
	limereg_initialize();
}

CRegistrator::~CRegistrator()
{
	//Matlab deinitialisieren
    limereg_terminate();
}

/**
* regParams will contain the final registration parameters
* \ret SSD distance
*/
uint32_t CRegistrator::RegisterImages(uint32_t iPicDimX, uint32_t iPicDimY, uint32_t iMaxIter, t_reg_real fMaxRotation, t_reg_real fMaxTranslation,
		uint32_t iLevelCount, uint32_t iSkipFineLevels, t_reg_real fStopSensitivity, t_pixel* imgRef, t_pixel* imgTmp,
		t_reg_real (&aRegStartParams)[3], t_reg_real (&aRegResult)[3], t_reg_real& fSSD, uint32_t *iterationsPerLevel
		)
{  
	//Output parameters
	uint32_t iNoIterations=0;
	fSSD=0;

	//Allocate image memory as an array suitable for matlab coder generated code
	uint32_t iPixelAmount = iPicDimX*iPicDimY;
	TMatlabArray_Pixel Tvec(iPixelAmount);
	uint32_t iRPixelAmount = iPixelAmount;

	//Memory for multilevel pyramid
	iRPixelAmount = (iRPixelAmount + (uint32_T)ceil((real64_T)iRPixelAmount / 3.0F)) + 1U;
	TMatlabArray_Pixel Rvec(iRPixelAmount);

	//Convert opencv image data to a matlab array
	//ToDo: Try to avoid this wasting memcpy operation (convert to matlab array without copying ... maybe by modifying the matlab classes.)
	memcpy(Rvec.GetCMemoryArrayPtr(), imgRef, iPixelAmount * sizeof(t_pixel));
	memcpy(Tvec.GetCMemoryArrayPtr(), imgTmp, iPixelAmount * sizeof(t_pixel));

	//Start image registration
	uint32_t iMaxPicDim = (iPicDimX < iPicDimY) ? iPicDimY : iPicDimX;
	gaussnewton(iPicDimX, iPicDimY, iMaxIter, fStopSensitivity, fMaxRotation, (t_reg_real)iMaxPicDim*(fMaxTranslation / 100),
		iLevelCount, iSkipFineLevels, Rvec.GetMatlabArrayPtr(), Tvec.GetMatlabArrayPtr(), &iNoIterations, &fSSD,
		aRegStartParams, aRegResult, iterationsPerLevel);

    return iNoIterations;
}

void CRegistrator::CalculateDiffImage(uint32_t iPicDimX, uint32_t iPicDimY, t_pixel* imgRef, t_pixel* imgTmp, t_pixel* imgDst)
{
	//Allocate image memory as an array suitable for matlab coder generated code
	uint32_t iPixelAmount = iPicDimX*iPicDimY;
	TMatlabArray_Pixel Rvec(iPixelAmount);
	TMatlabArray_Pixel Tvec(iPixelAmount);
	TMatlabArray_Pixel Dvec(iPixelAmount);

	//Convert opencv image data to a matlab array
	//ToDo: Try to avoid this wasting memcpy operation (convert to matlab array without copying ... maybe by modifying the matlab classes.)
	memcpy(Rvec.GetCMemoryArrayPtr(), imgRef, iPixelAmount * sizeof(t_pixel));
	memcpy(Tvec.GetCMemoryArrayPtr(), imgTmp, iPixelAmount * sizeof(t_pixel));

	//Execute computation
	diffimg(Rvec.GetMatlabArrayPtr(), Tvec.GetMatlabArrayPtr(), iPicDimX, iPicDimY, Dvec.GetMatlabArrayPtr());

	//Copy buffer back to application
	memcpy(imgDst, Dvec.GetCMemoryArrayPtr(), iPixelAmount);
}

void CRegistrator::TransformImage(uint32_t iPicDimX, uint32_t iPicDimY, t_reg_real w[3], t_pixel* imgSrc, t_pixel* imgDst)
{
	//Allocate image memory as an array suitable for matlab coder generated code
	uint32_t iPixelAmount = iPicDimX*iPicDimY;
	TMatlabArray_Pixel Svec(iPixelAmount);
	TMatlabArray_Pixel Dvec(iPixelAmount);

	//Convert opencv image data to a matlab array
	//ToDo: Try to avoid this wasting memcpy operation (convert to matlab array without copying ... maybe by modifying the matlab classes.)
	memcpy(Svec.GetCMemoryArrayPtr(), imgSrc, iPixelAmount * sizeof(t_pixel));

	//Execute computation
	transform(w, Svec.GetMatlabArrayPtr(), iPicDimX, iPicDimY, Dvec.GetMatlabArrayPtr());

	//Copy buffer back to application
	memcpy(imgDst, Dvec.GetCMemoryArrayPtr(), iPixelAmount);
}

}
