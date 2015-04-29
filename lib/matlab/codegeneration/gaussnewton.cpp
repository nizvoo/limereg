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

/*
 * gaussnewton.cpp
 *
 * CODE GENERATED BY MATLAB CODER (THE HUMAN READABILITY IS THEREFORE LIMITED)
 *
 */

#include "../../stdafx.h"
#include <float.h>

/* Include files */
#include "rt_nonfinite.h"
#include "diffimg.h"
#include "gaussnewton.h"
#include "gen_example_data.h"
#include "generatePyramidPC.h"
#include "jacobian.h"
#include "ssd.h"
#include "transform.h"
#include "limereg_emxutil.h"
#include "mpower.h"
#include "all.h"
#include "norm.h"
#include "mldivide.h"
#include "calcMarginAddition.h"
#include "mod.h"
#include "limereg_rtwutil.h"

/* Custom Source Code */
#include "../pseudo_stdafx.h"             //precompiled header not possible because of include position of matlab

namespace Limereg {

/* Type Definitions */

/* Named Constants */

/* Variable Declarations */

/* Variable Definitions */

/* Function Declarations */
static real64_T rt_powf_snf(real64_T u0, real64_T u1);

/* Function Definitions */
static real64_T rt_powf_snf(real64_T u0, real64_T u1)
{
  real64_T y;
  real64_T f0;
  real64_T f1;
  if (rtIsNaNF(u0) || rtIsNaNF(u1)) {
    y = ((real64_T)rtNaN);
  } else {
    f0 = (real64_T)fabs(u0);
    f1 = (real64_T)fabs(u1);
    if (rtIsInfF(u1)) {
      if (f0 == 1.0F) {
        y = ((real64_T)rtNaN);
      } else if (f0 > 1.0F) {
        if (u1 > 0.0F) {
          y = ((real64_T)rtInf);
        } else {
          y = 0.0F;
        }
      } else if (u1 > 0.0F) {
        y = 0.0F;
      } else {
        y = ((real64_T)rtInf);
      }
    } else if (f1 == 0.0F) {
      y = 1.0F;
    } else if (f1 == 1.0F) {
      if (u1 > 0.0F) {
        y = u0;
      } else {
        y = 1.0F / u0;
      }
    } else if (u1 == 2.0F) {
      y = u0 * u0;
    } else if ((u1 == 0.5F) && (u0 >= 0.0F)) {
      y = (real64_T)sqrt(u0);
    } else if ((u0 < 0.0F) && (u1 > (real64_T)floor(u1))) {
      y = ((real64_T)rtNaN);
    } else {
      y = (real64_T)pow(u0, u1);
    }
  }

  return y;
}

//todo: Fix SSD output (look into git diff for afssddecay which was removed ... and pass that back in the SSD parameter)

void gaussnewton(uint32_T ImgDimX, uint32_T ImgDimY, uint32_T MaxIter,
                 real64_T StopSensitivity,
                 real64_T maxRotation, real64_T maxTranslation, uint32_T
                 LevelCount, uint32_T SkipFineLevels, const emxArray_uint8_T *Rvec,
                 emxArray_uint8_T *Tvec, uint32_T *i, real64_T *SSD,
                 real64_T wStart[3], real64_T w[3], uint32_T *iterationsPerLevel)
{
  int32_T i1;

  //real_T sTime;
  int32_T b_i;
  real64_T SSD_old;
  real64_T wNext[3];
  real64_T w_old[3];
  emxArray_uint32_T *BoundBox;
  emxArray_real64_T *DSPRange;
  emxArray_uint32_T *IterationenImLevel;
  real64_T last_w[3];
  emxArray_uint32_T *TLvlPtrs;
  uint32_T TSizeWoPyramid;
  emxArray_uint32_T *RLvlPtrs;
  emxArray_uint32_T *MarginAddition;
  real64_T relAngle;
  real64_T angleDiff;
  real64_T transDiff;
  real64_T dmax;
  real64_T maxTranslationInThisLevel;
  uint32_T b_MarginAddition[3];
  real64_T FirstSSD;
  uint32_T B;
  uint32_T d_in_this_level;
  uint32_T x;
  uint32_T Ts;
  uint32_T Te;
  uint32_T Rs;
  uint32_T Re;
  int32_T RememberSSD;
  uint32_T iLast;
  /*
  emxArray_uint8_T *b_TLvlImg;
  emxArray_uint8_T *b_RLvlImg;
  emxArray_uint8_T *c_TLvlImg;
  emxArray_uint8_T *c_RLvlImg;
  */
  int32_T exitg1;
  int32_T exitg2;
  real64_T JD2[9];
  real64_T JD[3];
  int32_T i2;
  int32_T i3;
  int32_T i4;
  uint32_T b_BoundBox[4];
  real64_T b_DSPRange[4];
  real64_T b_JD[3];
  real64_T dw[3];
  real64_T descent;
  real64_T t;
  int32_T ArmijoSuccess;
  int32_T ii;
  boolean_T exitg3;
  int8_T STOP[7];
  boolean_T b0;
  boolean_T guard1 = FALSE;
  //real_T eTime;
  emxArray_uint8_T *TvecWoMargins = NULL;

  SSD_old = 0.0F;
  for (b_i = 0; b_i < 3; b_i++) {
    wNext[b_i] = wStart[b_i];
    w_old[b_i] = wStart[b_i];
  }

  emxInit_uint32_T(&BoundBox, 2);
  i1 = BoundBox->size[0] * BoundBox->size[1];
  BoundBox->size[0] = (int32_T)LevelCount;
  BoundBox->size[1] = 4;
  emxEnsureCapacity((emxArray__common *)BoundBox, i1, (int32_T)sizeof(uint32_T));
  b_i = ((int32_T)LevelCount << 2) - 1;
  for (i1 = 0; i1 <= b_i; i1++) {
    BoundBox->data[i1] = 0U;
  }

  b_emxInit_real64_T(&DSPRange, 2);

  /* In matlab coder mode: Avoid matlab coder error about undefined type */
  i1 = DSPRange->size[0] * DSPRange->size[1];
  DSPRange->size[0] = (int32_T)LevelCount;
  DSPRange->size[1] = 4;
  emxEnsureCapacity((emxArray__common *)DSPRange, i1, (int32_T)sizeof(real64_T));
  b_i = ((int32_T)LevelCount << 2) - 1;
  for (i1 = 0; i1 <= b_i; i1++) {
    DSPRange->data[i1] = 0.0F;
  }

  b_emxInit_uint32_T(&IterationenImLevel, 1);

  /* In matlab coder mode: Avoid matlab coder error about undefined type */
  i1 = IterationenImLevel->size[0];
  IterationenImLevel->size[0] = (int32_T)LevelCount;
  emxEnsureCapacity((emxArray__common *)IterationenImLevel, i1, (int32_T)sizeof
                    (uint32_T));
  b_i = (int32_T)LevelCount - 1;
  for (i1 = 0; i1 <= b_i; i1++) {
    IterationenImLevel->data[i1] = 0U;
  }

  /* Start-Registrierungsparameter */
  for (b_i = 0; b_i < 3; b_i++) {
    w[b_i] = 0.0F;
  }

  for (b_i = 0; b_i < 3; b_i++) {
    last_w[b_i] = w[b_i];
  }

  emxInit_uint32_T(&TLvlPtrs, 2);

  /* Matlab beschwert sich sonst (aus meiner Sicht zu Unrecht, kann ggf. alles raus, wenn man die Simulationsschicht wegnimmet) */
  /* START TO DELETE */
  TSizeWoPyramid = ImgDimension * ImgDimension;
  TSizeWoPyramid = (TSizeWoPyramid + (uint32_T)rt_roundf_snf((real64_T)TSizeWoPyramid / 3.0F)) + 1U;

  /* +1 because the target will floor (matlab ceils) */
  i1 = TLvlPtrs->size[0] * TLvlPtrs->size[1];
  TLvlPtrs->size[0] = (int32_T)LevelCount;
  TLvlPtrs->size[1] = 2;
  emxEnsureCapacity((emxArray__common *)TLvlPtrs, i1, (int32_T)sizeof(uint32_T));
  b_i = ((int32_T)LevelCount << 1) - 1;
  for (i1 = 0; i1 <= b_i; i1++) {
    TLvlPtrs->data[i1] = 0U;
  }

  emxInit_uint32_T(&RLvlPtrs, 2);
  TSizeWoPyramid = ImgDimension * ImgDimension;
  TSizeWoPyramid = (TSizeWoPyramid + (uint32_T)rt_roundf_snf((real64_T) TSizeWoPyramid / 3.0F)) + 1U;

  /* +1 because the target will floor (matlab ceils) */
  i1 = RLvlPtrs->size[0] * RLvlPtrs->size[1];
  RLvlPtrs->size[0] = (int32_T)LevelCount;
  RLvlPtrs->size[1] = 2;
  emxEnsureCapacity((emxArray__common *)RLvlPtrs, i1, (int32_T)sizeof(uint32_T));
  b_i = ((int32_T)LevelCount << 1) - 1;
  for (i1 = 0; i1 <= b_i; i1++) {
    RLvlPtrs->data[i1] = 0U;
  }

  emxInit_uint32_T(&MarginAddition, 2);
  i1 = MarginAddition->size[0] * MarginAddition->size[1];
  MarginAddition->size[0] = (int32_T)LevelCount;
  MarginAddition->size[1] = 3;
  emxEnsureCapacity((emxArray__common *)MarginAddition, i1, (int32_T)sizeof
                    (uint32_T));
  b_i = (int32_T)LevelCount * 3 - 1;
  for (i1 = 0; i1 <= b_i; i1++) {
    MarginAddition->data[i1] = 0U;
  }

  /* END TO DELETE */
  /* Calculate bounding boxes (and extract its image-parts if algorithm is distributed to several DSPs) */
    /* local registration on PC */
    /* precalculate some frequent used matrices */
    relAngle = (real64_T)ImgDimension / 2.0F;
    dmax = (real64_T)ImgDimension / 2.0F - 0.5F;
    BoundBox->data[0] = 1U;
    BoundBox->data[BoundBox->size[0]] = ImgDimension;
    BoundBox->data[BoundBox->size[0] << 1] = 1U;
    BoundBox->data[BoundBox->size[0] * 3] = ImgDimension;
    DSPRange->data[0] = -dmax;
    DSPRange->data[DSPRange->size[0]] = relAngle - 0.5F;
    DSPRange->data[DSPRange->size[0] << 1] = -dmax;
    DSPRange->data[DSPRange->size[0] * 3] = relAngle - 0.5F;

    calcMarginAddition(maxRotation, maxTranslation, ImgDimension,
                       b_MarginAddition);
    for (i1 = 0; i1 < 3; i1++) {
      MarginAddition->data[MarginAddition->size[0] * i1] = b_MarginAddition[i1];
    }

	//Swap Tvec with a new buffer having enough space for margins and multilevel.
	//(Swapping the pointers avoids wasteful copy operations ;)
	TvecWoMargins = Tvec;
	Tvec = new emxArray_uint8_T();
	emxInit_uint8_T(&Tvec, 1);

	uint32_T TWidth = BoundBox->data[BoundBox->size[0]] + MarginAddition->data[0];
	uint32_T THeight = (BoundBox->data[BoundBox->size[0] * 3] + MarginAddition->data[MarginAddition->size[0]]) + MarginAddition->data[MarginAddition->size[0] << 1];
    Tvec->size[0] = TWidth * THeight;
	Tvec->size[0] = (Tvec->size[0] + (uint32_T)rt_roundf_snf((real64_T)Tvec->size[0] / 3.0F)) + 1U;
    emxEnsureCapacity((emxArray__common *)Tvec, 0, (int32_T)sizeof(uint8_T));

    //Initialize Dirichlet boundaries
    //Use the mean color of the four outermost image corners as the boundary background color
    //I have several more sophisticated ideas for the boundary conditions, mail to the author if you need some improvement.
    uint8_T backgroundColor = (uint8_T)(
    						  ( (uint32_T)(TvecWoMargins->data[0])
                              + (uint32_T)(TvecWoMargins->data[ImgDimension - 1])
                              + (uint32_T)(TvecWoMargins->data[(ImgDimension - 1) * ImgDimension])
                              + (uint32_T)(TvecWoMargins->data[ImgDimension * ImgDimension - 1])
                              )/4);
    memset(Tvec->data, backgroundColor, Tvec->size[0]);
//rbe todo: pass the backcolor to generatePyramid, then on to shrinkimage, then use it for the split pixels
//on the right, lower corner

    /* Calculate multilevel pyramid */
	//Generate multilevel pyramid
	generatePyramidPC(Tvec, BoundBox, MarginAddition, Rvec, DSPRange,
                        LevelCount, TLvlPtrs, Tvec->size[0], RLvlPtrs,
						Rvec->size[0], &TvecWoMargins->data[0]
						);

  /* Gauss-Newton Algorithmus */
  TSizeWoPyramid = LevelCount;

  /* Multilevel registration (pyramid of pixtures in diffetent scales) */
  *i = 0U;
  FirstSSD = 0.0F;
  B = mpower(LevelCount - 1U);
  d_in_this_level = ImgDimension / B;
  x = ImgDimension - d_in_this_level * B;
  if ((x > 0U) && (x >= (B >> 1U) + (B & 1U))) {
    d_in_this_level++;
  }

  Ts = 0U;
  Te = 0U;
  Rs = 0U;
  Re = 0U;
    Ts = TLvlPtrs->data[(int32_T)LevelCount - 1];
    Te = TLvlPtrs->data[((int32_T)LevelCount + TLvlPtrs->size[0]) - 1];
    Rs = RLvlPtrs->data[(int32_T)LevelCount - 1];
    Re = RLvlPtrs->data[((int32_T)LevelCount + RLvlPtrs->size[0]) - 1];

  RememberSSD = 1;
  iLast = 0U;
  /*
  emxInit_uint8_T(&b_TLvlImg, 1);
  emxInit_uint8_T(&b_RLvlImg, 1);
  emxInit_uint8_T(&c_TLvlImg, 1);
  emxInit_uint8_T(&c_RLvlImg, 1);
  */
  do {
    exitg1 = 0;
    do {
      exitg2 = 0;
      (*i)++;

      /* Call 'jacobian' directly in 1 part (dgaussnewton would call it in 4 */
      /* parts) */
        if (Ts > Te) {
          i1 = 0;
          i2 = 0;
        } else {
          i1 = (int32_T)Ts - 1;
          i2 = (int32_T)Te;
        }

        if (Rs > Re) {
          i3 = 0;
          i4 = 0;
        } else {
          i3 = (int32_T)Rs - 1;
          i4 = (int32_T)Re;
        }

        for (b_i = 0; b_i < 4; b_i++) {
          b_BoundBox[b_i] = BoundBox->data[((int32_T)TSizeWoPyramid +
            BoundBox->size[0] * b_i) - 1];
        }

        for (b_i = 0; b_i < 3; b_i++) {
          b_MarginAddition[b_i] = MarginAddition->data[((int32_T)TSizeWoPyramid
            + MarginAddition->size[0] * b_i) - 1];
        }

        for (b_i = 0; b_i < 4; b_i++) {
          b_DSPRange[b_i] = DSPRange->data[((int32_T)TSizeWoPyramid +
            DSPRange->size[0] * b_i) - 1];
        }

        jacobian(w, b_BoundBox, b_MarginAddition, b_DSPRange, &Tvec->data[i1], 0,
                 &Rvec->data[i3], 0, d_in_this_level, SSD, JD, JD2);


      /* search direction */
      /* Because of the following line we must enable the */
      /* non-finite-number support. We haven't checked yet if ths introduces a performance drawback. */
      for (i1 = 0; i1 < 3; i1++) {
        b_JD[i1] = -JD[i1];
      }

      mldivide(JD2, b_JD, dw);
      if (RememberSSD == 1) {
        FirstSSD = *SSD;
        RememberSSD = 0;
        SSD_old = *SSD;
        for (b_i = 0; b_i < 3; b_i++) {
          w_old[b_i] = w[b_i];
        }
      }

      /*  check descent direction */
      /*  note: descent is not granted if using an iterative solver  */
      descent = 0.0F;
      for (b_i = 0; b_i < 3; b_i++) {
        descent += JD[b_i] * dw[b_i];
      }

      if (descent > 0.0F) {
        for (i1 = 0; i1 < 3; i1++) {
          dw[i1] = -dw[i1];
        }
      }


      /* N�chsten Schritt gehen */
      /*  slope of line */
      t = 1.0F;

      /*  initial step */
      descent = 0.0F;
      for (b_i = 0; b_i < 3; b_i++) {
        descent += JD[b_i] * dw[b_i];
      }

      ArmijoSuccess = 0;
      ii = 1;
      exitg3 = FALSE;
      while ((exitg3 == 0U) && ((uint32_T)ii < 11U)) {
        for (b_i = 0; b_i < 3; b_i++) {
          wNext[b_i] = w[b_i] + t * dw[b_i];
        }

        /*  compute test value Yt */

		maxTranslationInThisLevel = maxTranslation / (0x1 << TSizeWoPyramid);

        /* distributed registration (on more than 1 DSPs) */
        /* Check whether the next iteration is inside the allowed parameter range */
        transDiff = (real64_T)sqrt(rt_powf_snf(wNext[1] - last_w[1], 2.0F) +
                              rt_powf_snf(wNext[2] - last_w[2], 2.0F));
        angleDiff = (real64_T)fabs(wNext[0] - last_w[0]);
        if ((angleDiff > maxRotation) || (transDiff > maxTranslationInThisLevel - 1.0F)) {
		  //printf("Ignoring SSD calculation and continue armijo line search because WORST CASE w PARAMETERS EXCEEDED.\n");
		  relAngle = FLT_MAX;
		}
		else {
			  if (Ts > Te) {
				i1 = 0;
				i2 = 0;
			  } else {
				i1 = (int32_T)Ts - 1;
				i2 = (int32_T)Te;
			  }

			  if (Rs > Re) {
				i3 = 0;
				i4 = 0;
			  } else {
				i3 = (int32_T)Rs - 1;
				i4 = (int32_T)Re;
			  }

			  for (b_i = 0; b_i < 4; b_i++) {
				b_BoundBox[b_i] = BoundBox->data[((int32_T)TSizeWoPyramid +
				  BoundBox->size[0] * b_i) - 1];
			  }

			  for (b_i = 0; b_i < 3; b_i++) {
				b_MarginAddition[b_i] = MarginAddition->data[((int32_T)
				  TSizeWoPyramid + MarginAddition->size[0] * b_i) - 1];
			  }

			  for (b_i = 0; b_i < 4; b_i++) {
				b_DSPRange[b_i] = DSPRange->data[((int32_T)TSizeWoPyramid +
				  DSPRange->size[0] * b_i) - 1];
			  }

			  relAngle = ssd(wNext, b_BoundBox, b_MarginAddition, b_DSPRange,
							 &Tvec->data[i1], 0, &Rvec->data[i3], 0, d_in_this_level);

			  /* Nur SSD berechnen, nicht JD,JD2 */
		}

		if (relAngle < *SSD + t * 0.0001F * descent) {
			  ArmijoSuccess = 1;
          /* ArmijoLeftAt = ii */
          exitg3 = TRUE;
        } else {
          t /= 2.0F;

          /*  reduce t */
          ii = (int32_T)((uint32_T)ii + 1U);
        }
      }

      for (b_i = 0; b_i < 3; b_i++) {
        w[b_i] = wNext[b_i];
      }

      /* Abbruchbedingung */
      for (b_i = 0; b_i < 7; b_i++) {
        STOP[b_i] = 0;
      }

      /* eps = machine precision   */
      STOP[3] = (int8_T)(norm(JD) <= 1.1920929E-7F);
      STOP[4] = (int8_T)(*i >= MaxIter);
      STOP[5] = (int8_T)(0 == ArmijoSuccess);

		/* Use original stop criteria of "Gill, Murray, Wright: Practical Optimization" */
		if (((int32_T)*i > 1) && ((real64_T)fabs(*SSD - SSD_old) <=
			 StopSensitivity * (1.0F + (real64_T)fabs(FirstSSD)))) {
		  b0 = TRUE;
		} else {
		  b0 = FALSE;
		}

		STOP[0] = (int8_T)b0;
		guard1 = FALSE;
		if ((int32_T)*i > 1) {
		  for (b_i = 0; b_i < 3; b_i++) {
			b_JD[b_i] = wNext[b_i] - w_old[b_i];
		  }

		  if (b_norm(b_JD) <= (real64_T)sqrt(StopSensitivity)) {
			b0 = TRUE;
		  } else {
			guard1 = TRUE;
		  }
		} else {
		  guard1 = TRUE;
		}

		if (guard1 == TRUE) {
		  b0 = FALSE;
		}

		STOP[1] = (int8_T)b0;
		STOP[2] = (int8_T)(norm(JD) <= rt_powf_snf(StopSensitivity, 0.333333343F)
						   * (1.0F + (real64_T)fabs(FirstSSD)));
		for (b_i = 0; b_i < 3; b_i++) {
		  b_MarginAddition[b_i] = (uint32_T)STOP[b_i];
		}

		if (all(b_MarginAddition) || ((1.0F == StopSensitivity) && ((uint32_T)
			  STOP[0] != 0U)) || ((uint32_T)STOP[3] != 0U) || ((uint32_T)STOP[4]
			 != 0U) || ((uint32_T)STOP[5] != 0U)) {
		  b0 = TRUE;
		} else {
		  b0 = FALSE;
		}

		STOP[6] = (int8_T)b0;

      /* STOP' */
      if ((uint32_T)STOP[6] != 0U) {
        exitg2 = 1;
      } else {
        for (b_i = 0; b_i < 3; b_i++) {
          w_old[b_i] = wNext[b_i];
        }

        SSD_old = *SSD;
        /* distributed registration (on more than 1 DSPs) */
        /* Check whether the next iteration is inside the allowed parameter range */
        transDiff = (real64_T)sqrt(rt_powf_snf(wNext[1] - last_w[1], 2.0F) +
                              rt_powf_snf(wNext[2] - last_w[2], 2.0F)) *
          (real64_T)TSizeWoPyramid;
        angleDiff = (real64_T)fabs(wNext[0] - last_w[0]);
      }
    } while (exitg2 == 0U);

    TSizeWoPyramid--;
    if (TSizeWoPyramid == SkipFineLevels) {
      exitg1 = 1;
    } else {
      /* prepare next level */
      FirstSSD = 0.0F;
      SSD_old = 0.0F;

      /* w; */
      w[1] = wNext[1] * 2.0F;
      w[2] *= 2.0F;
      RememberSSD = 1;
      IterationenImLevel->data[(int32_T)TSizeWoPyramid] = *i - iLast;
      iLast = *i;
      B = mpower(TSizeWoPyramid - 1U);
      d_in_this_level = ImgDimension / B;
      x = ImgDimension - d_in_this_level * B;
      if ((x > 0U) && (x >= (B >> 1U) + (B & 1U))) {
        d_in_this_level++;
      }

        Ts = TLvlPtrs->data[(int32_T)TSizeWoPyramid - 1];
        Te = TLvlPtrs->data[((int32_T)TSizeWoPyramid + TLvlPtrs->size[0]) - 1];
        Rs = RLvlPtrs->data[(int32_T)TSizeWoPyramid - 1];
        Re = RLvlPtrs->data[((int32_T)TSizeWoPyramid + RLvlPtrs->size[0]) - 1];
    }
  } while (exitg1 == 0U);

  IterationenImLevel->data[0] = *i - iLast;

  if(NULL != iterationsPerLevel)
  {
	  for (uint32_T iLvl = LevelCount; iLvl >= 1; iLvl--)
	  {
		  iterationsPerLevel[LevelCount - iLvl] = IterationenImLevel->data[(int32_T)iLvl - 1];
	  }
  }

  /*
  emxFree_uint8_T(&c_RLvlImg);
  emxFree_uint8_T(&c_TLvlImg);
  emxFree_uint8_T(&b_RLvlImg);
  emxFree_uint8_T(&b_TLvlImg);
  */
  emxFree_uint32_T(&MarginAddition);
  emxFree_uint32_T(&RLvlPtrs);
  emxFree_uint32_T(&TLvlPtrs);
  emxFree_real64_T(&DSPRange);
  emxFree_uint32_T(&BoundBox);
  emxFree_uint32_T(&IterationenImLevel);
  if(NULL != TvecWoMargins) {
	emxFree_uint8_T(&Tvec);	//no typo ! (swapped pointers)
  }

}

}

/* End of code generation (gaussnewton.cpp) */
