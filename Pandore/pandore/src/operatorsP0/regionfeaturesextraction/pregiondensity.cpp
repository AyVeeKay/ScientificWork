/* -*- c-basic-offset: 3 -*-
 *
 * Copyright (c), GREYC.
 * All rights reserved
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the GREYC, nor the name of its
 *     contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 *
 * 
 * For more information, refer to:
 * https://clouard.users.greyc.fr/Pandore
 */

/**
 * @author Regis Clouard - Jan 24, 2012
 */

#include <pandore.h>
using namespace pandore;

/**
 * @file pregiondensity.cpp
 * Calculate density factor of each regions.
 * then save in a collection.
 */

#ifdef MAIN
#undef MAIN
#include "region/pdensityselection.cpp"
#define MAIN
#else
#include "region/pdensityselection.cpp"
#endif

Errc PRegionDensity( const Reg2d &rgs, const Img2duc &ims, Collection &cold, const std::string &s ) {
   struct attrs_density *regions;
   regions = (struct attrs_density*)calloc(rgs.Labels()+1,sizeof(struct attrs_density));
   Float *t = (Float*)calloc(rgs.Labels()+1,sizeof(float));
   
   if (PDensitySelection(rgs, ims, regions)==FAILURE) return FAILURE;
   
   for (Ulong r=1;r<=rgs.Labels();r++)
      t[r]=regions[r].density;
   
   cold.SETARRAY(s, Float, t+1, rgs.Labels()); // Do not delete t
   
   free(regions);
   return SUCCESS;
}

#ifdef MAIN

/*
 * Modify only the following constants, and the function call.
 */
#define	USAGE	"usage: %s name [-m mask] [rg_in|-] [im_in] [col_out|-]"
#define	PARC	1
#define	FINC	2
#define	FOUTC	1
#define	MASK	2

int main( int argc, char *argv[] ) {
   Errc result;              // The result code of the execution.
   Pobject* mask;            // The region mask.
   Pobject* objin[FINC+1];   // The input objects.
   Pobject* objs[FINC+1];    // The source objects masked by the mask.
   Pobject* objout[FOUTC+1]; // The output object.
   Pobject* objd[FOUTC+1];   // The result object of the execution.
   char* parv[PARC+1];       // The input parameters.
   
   ReadArgs(argc,argv,PARC,FINC,FOUTC,&mask,objin,objs,objout,objd,parv,USAGE,MASK);
   
   if (objs[0]->Type()==Po_Reg2d &&  objs[1]->Type()==Po_Img2duc) {
      Reg2d* const rgs=(Reg2d*)objs[0];
      Img2duc* const ims=(Img2duc*)objs[1];
      objd[0]=new Collection();
      Collection* const cold=(Collection*)objd[0];
      
      result=PRegionDensity(*rgs,*ims,*cold,argv[1]);
   } else {
      PrintErrorFormat(objin,FINC);
      result=FAILURE;
   }
   
   if (result)
      WriteArgs(argc,argv,PARC,FINC,FOUTC,&mask,objin,objs,objout,objd,MASK);
   
   Exit(result);
   return 0;
}

#endif