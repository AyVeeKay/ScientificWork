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

#if (defined _MSC_VER) && (!defined snprintf)
#define snprintf _snprintf
#endif

#include <string.h>
#include <pandore.h>
using namespace pandore;

/**
 * @file psetstatus.cpp
 * Set a new value  to the status value.
 */

#ifdef MAIN

/*
 * Modify only the following constants, and the function call.
 */
#define	USAGE	"usage: %s value"
#define	PARC	1
#define	FINC	0
#define	FOUTC	0
#define	MASK	0

int main( int argc, char *argv[] ) {
   Errc	 result;              // The result code of the execution.
   Pobject* mask;              // The region mask.
   Pobject* objin[FINC + 1];   // The input objects.
   Pobject* objs[FINC + 1];    // The source objects masked by the mask.
   Pobject* objout[FOUTC + 1]; // The output object.
   Pobject* objd[FOUTC + 1];   // The result object of the execution.
   char* parv[PARC + 1];       // The input parameters.
   
   ReadArgs(argc, argv, PARC, FINC, FOUTC, &mask, objin, objs, objout, objd, parv, USAGE, MASK);

   Float floatResult;
   char stringResult[512];
   if (sscanf(argv[1], "%f", &floatResult) > 0) {
      result = floatResult;
   } else if (sscanf(argv[1],"%s", stringResult) > 0) {
      stringResult[sizeof(stringResult) - 1] = 0;
      if (!strncmp("SUCCESS", stringResult, sizeof(stringResult)))
	 result = SUCCESS;
      else if (!strncmp("FAILURE", stringResult, sizeof(stringResult))) {
	 result = FAILURE;
      } else { // String
	 FILE *fp;
	 
	 char filename[256];
	 char *dir;
	 int number = 0;
	 
	 if (!(dir = getenv(PANDORE_TMP))) {
#if defined _WIN32 || defined WIN32
	    if ((dir = getenv("TEMP")) == 0)  {
	       dir = const_cast<char *>("c:\\Temp");
	    }
#else
	    dir = const_cast<char *>("/tmp");
	    number = (int)getppid();
#endif
	 }
#if defined _WIN32 || defined WIN32
	 snprintf(filename, sizeof(filename), "%s\\%s%d", dir, STATUS_FILE, number);
	 filename[sizeof(filename) - 1] = 0;
#else
	 snprintf(filename, sizeof(filename), "%s/%s%d", dir, STATUS_FILE, number);
	 filename[sizeof(filename) - 1] = 0;
#endif
	 if ((fp = fopen(filename, "wb"))) {
	    fwrite("T", sizeof(Char), 1, fp);
	    fwrite(stringResult, sizeof(Char), strlen(stringResult), fp);
	    fclose(fp);
	    exit(0);
	 }
	exit(1);
      }
   }
   
   WriteArgs(argc, argv, PARC, FINC, FOUTC, &mask, objin, objs, objout, objd, MASK);
   
   Exit(result);
  return 0;
}

#endif