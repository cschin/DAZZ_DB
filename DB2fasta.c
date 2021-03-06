/************************************************************************************\
*                                                                                    *
* Copyright (c) 2014, Dr. Eugene W. Myers (EWM). All rights reserved.                *
*                                                                                    *
* Redistribution and use in source and binary forms, with or without modification,   *
* are permitted provided that the following conditions are met:                      *
*                                                                                    *
*  · Redistributions of source code must retain the above copyright notice, this     *
*    list of conditions and the following disclaimer.                                *
*                                                                                    *
*  · Redistributions in binary form must reproduce the above copyright notice, this  *
*    list of conditions and the following disclaimer in the documentation and/or     *
*    other materials provided with the distribution.                                 *
*                                                                                    *
*  · The name of EWM may not be used to endorse or promote products derived from     *
*    this software without specific prior written permission.                        *
*                                                                                    *
* THIS SOFTWARE IS PROVIDED BY EWM ”AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,    *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND       *
* FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL EWM BE LIABLE   *
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS  *
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY      *
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING     *
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN  *
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                      *
*                                                                                    *
* For any issues regarding this software and its use, contact EWM at:                *
*                                                                                    *
*   Eugene W. Myers Jr.                                                              *
*   Bautzner Str. 122e                                                               *
*   01099 Dresden                                                                    *
*   GERMANY                                                                          *
*   Email: gene.myers@gmail.com                                                      *
*                                                                                    *
\************************************************************************************/

/********************************************************************************************
 *
 *  Recreate all the .fasta files that have been loaded into a specified database.
 *
 *  Author:  Gene Myers
 *  Date  :  May 2014
 *
 ********************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "DB.h"

static char *Usage = "[-vU] [-w<int(80)>] <path:db>";

int main(int argc, char *argv[])
{ HITS_DB    _db, *db = &_db;
  FILE       *dbfile;
  int         nfiles;
  int         VERBOSE, UPPER, WIDTH;

  //  Process arguments

  { int   i, j, k;
    int   flags[128];
    char *eptr;

    ARG_INIT("DB2fasta")

    WIDTH = 80;

    j = 1;
    for (i = 1; i < argc; i++)
      if (argv[i][0] == '-')
        switch (argv[i][1])
        { default:
            ARG_FLAGS("vU")
            break;
          case 'w':
            ARG_NON_NEGATIVE(WIDTH,"Line width")
            break;
        }
      else
        argv[j++] = argv[i];
    argc = j;

    UPPER   = 1 + flags['U'];
    VERBOSE = flags['v'];

    if (argc != 2)
      { fprintf(stderr,"Usage: %s %s\n",Prog_Name,Usage);
        exit (1);
      }
  }

  //  Open db and also db image file (dbfile)

  if (Open_DB(argv[1],db))
    { fprintf(stderr,"%s: Database %s.db could not be opened\n",Prog_Name,argv[1]);
      exit (1);
    }
  if (db->part > 0)
    { fprintf(stderr,"%s: Cannot be called on a block: %s.db\n",Prog_Name,argv[1]);
      exit (1);
    }

  { char *pwd, *root;

    pwd    = PathTo(argv[1]);
    root   = Root(argv[1],".db");
    dbfile = Fopen(Catenate(pwd,"/",root,".db"),"r");
    free(pwd);
    free(root);
    if (dbfile == NULL)
      exit (1);
  }

  //  nfiles = # of files in data base

  if (fscanf(dbfile,DB_NFILE,&nfiles) != 1)
    SYSTEM_ERROR

  //  For each file do:

  { HITS_READ  *reads;
    char       *read;
    int         f, first;

    reads = db->reads;
    read  = New_Read_Buffer(db);
    first = 0;
    for (f = 0; f < nfiles; f++)
      { int   i, last;
        FILE *ofile;
        char  prolog[MAX_NAME], fname[MAX_NAME];

        //  Scan db image file line, create .fasta file for writing

        if (fscanf(dbfile,DB_FDATA,&last,fname,prolog) != 3)
          SYSTEM_ERROR

        if ((ofile = Fopen(Catenate(".","/",fname,".fasta"),"w")) == NULL)
          exit (1);

        if (VERBOSE)
          { fprintf(stderr,"Creating %s.fasta ...\n",fname);
            fflush(stdout);
          }

        //   For the relevant range of reads, write each to the file
        //     recreating the original headers with the index meta-data about each read

        for (i = first; i < last; i++)
          { int        j, len;
            int        flags, qv;
            HITS_READ *r;

            r     = reads + i;
            len   = r->end - r->beg;
            flags = r->flags;
            qv    = (flags & DB_QV);
            fprintf(ofile,">%s/%d/%d_%d",prolog,r->origin,r->beg,r->end);
            if (qv > 0)
              fprintf(ofile," RQ=0.%3d",qv);
            fprintf(ofile,"\n");

            Load_Read(db,i,read,UPPER);

            for (j = 0; j+WIDTH < len; j += WIDTH)
              fprintf(ofile,"%.*s\n",WIDTH,read+j);
            if (j < len)
              fprintf(ofile,"%s\n",read+j);
          }

        first = last;
      }
  }

  fclose(dbfile);
  Close_DB(db);

  exit (0);
}
