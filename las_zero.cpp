
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.

*********************************************************************************************/


#include "las_zero.hpp"
#include "version.hpp"


/***************************************************************************\
*                                                                           *
*   Module Name:        las_zero                                            *
*                                                                           *
*   Programmer(s):      Jan C. Depner                                       *
*                                                                           *
*   Date Written:       August 10, 2016                                     *
*                                                                           *
*   Purpose:            Sets withheld bit for values above 0.0 in a LAS.    *
*                       Written for Mick Hawkins at USM                     *
*                                                                           *
\***************************************************************************/

int32_t main (int32_t argc, char **argv)
{
  new las_zero (argc, argv);
}


void las_zero::usage ()
{
  fprintf (stderr, "\nUsage: las_zero <LAS_FILE | LAZ_FILE>\n");
  fflush (stderr);
}


las_zero::las_zero (int32_t argc, char **argv)
{
  FILE                    *las_fp;
  LASheader               lasheader;
  uint8_t                 endian = 0, laz = NVFalse;
  char                    las_file[1024], laz_file[1024];
  SLAS_POINT_DATA         slas;
  QString                 fileLAS, fileLAZ, lzName;
  int32_t                 recnum = 0, percent = 0, old_percent = -1;


  printf ("\n\n %s \n\n", VERSION);


  //  Make sure we got the mandatory file name argument.

  if (argc < 2)
    {
      usage ();
      exit (-1);
    }


  printf ("\nLAS file : %s\n\n", argv[1]);


  //  If we've got a LAZ file, check for the laszip program.

  if (QString (argv[1]).endsWith (".laz") || QString (argv[1]).endsWith (".LAZ"))
    {
      strcpy (laz_file, argv[1]);
      strcpy (las_file, argv[1]);


      laz = NVTrue;


      char lz_name[1024];

#ifdef NVWIN3X
      strcpy (lz_name, "laszip.exe");
#else
      strcpy (lz_name, "laszip");
#endif
      if (find_startup_name (lz_name) == NULL)
        {
          fprintf (stderr, "\n\n*** ERROR ***\nLAZ file %s\nwill not be unloaded because %s is not in the PATH\n", laz_file, lz_name);
          fflush (stderr);
          exit (-1);
        }


      lzName = QString (lz_name);
    }
  else
    {
      strcpy (las_file, argv[1]);
    }


  //  Open the LAS file with LASlib and read the header.

  LASreadOpener lasreadopener;
  LASreader *lasreader;

  lasreadopener.set_file_name (las_file);
  lasreader = lasreadopener.open ();
  if (!lasreader)
    {
      fprintf (stderr, "\n\n*** ERROR ***\nUnable to open LAS file %s\n", las_file);
      fflush (stderr);
      exit (-1);
    }


  lasheader = lasreader->header;


  if (lasheader.version_major != 1)
    {
      lasreader->close ();
      fprintf (stderr, "\nLAS major version %d incorrect, file %s : %s %s %d\n\n", lasheader.version_major, las_file, __FILE__, __FUNCTION__, __LINE__);
      fflush (stderr);
      exit (-1);
    }


  if (lasheader.version_minor > 4)
    {
      lasreader->close ();
      fprintf (stderr, "\nLAS minor version %d incorrect, file %s : %s %s %d\n\n", lasheader.version_minor, las_file, __FILE__, __FUNCTION__, __LINE__);
      fflush (stderr);
      exit (-1);
    }


  //  Now close it since all we really wanted was the header.

  lasreader->close ();


  //  Check for endian-ness.

  endian = big_endian ();


  //  If it's a LAZ file we have to uncompress it, update the LAS file, then recompress it.  What A PITA!

  if (laz)
    {
      QProcess unzipper;
      QStringList uparams;

      fileLAS = fileLAZ = QString (laz_file);

      uparams << fileLAZ;

      unzipper.start (lzName, uparams);

      unzipper.waitForFinished (-1);

      fileLAS.replace (".laz", ".las");

      strcpy (las_file, fileLAS.toLatin1 ());
    }


  //  Open the file for update.

  if ((las_fp = fopen64 (las_file, "rb+")) == NULL)
    {
      fprintf (stderr, "\nError opening LAS file %s : %s\n\n", las_file, strerror (errno));
      fflush (stderr);
      exit (-1);
    }


  for (uint64_t i = 0 ; i < lasheader.number_of_point_records ; i++)
    {
      if (slas_read_point_data (las_fp, i, &lasheader, endian, &slas))
        {
          fprintf (stderr, "\nError reading record %" PRIu64 " from %s : %s\n\n", i, las_file, strerror (errno));
          fflush (stderr);
          exit (-1);
        }

      if (slas.z > 0.0)
        {
          slas.withheld = 1;

          if (slas_update_point_data (las_fp, i, &lasheader, endian, &slas))
            {
              fprintf (stderr, "\nError %s updating record %d in file %s : %s %s %d\n\n", strerror (errno), recnum, las_file, __FILE__, __FUNCTION__,
                       __LINE__);
              fflush (stderr);
              fclose (las_fp);
              exit (-1);
            }
        }

      percent = NINT (((float) i / (float) lasheader.number_of_point_records) * 100.0);
      if (old_percent != percent)
        {
          printf ("%3d%% processed    \r", percent);
          fflush (stdout);
          old_percent = percent;
        }
    }

  fclose (las_fp);


  printf ("100%% processed    \n\n");
  fflush (stdout);


  //  Recompress if it was LAZ

  if (laz)
    {
      //  We have to rename the old LAZ file to WHATEVER.bck, compress the new LAS file to the old LAZ file name, delete the LAS file,
      //  then delete the BCK file.  At that point we are back to the original LAZ file name.

      QString backFile (fileLAZ);
      backFile.replace (".laz", ".bck");

      QFile bckFile (fileLAZ);

      if (!bckFile.rename (backFile))
        {
          fprintf (stderr, "\n\n*** ERROR ***\nUnable to rename LAZ file %s\n", laz_file);
          fflush (stderr);
          exit (-1);
        }


      QProcess zipper;
      QStringList zparams;

      zparams << fileLAS;

      zipper.start (lzName, zparams);

      zipper.waitForFinished (-1);


      QFile lasFile (fileLAS);

      if (!lasFile.remove ())
        {
          char name[1024];
          strcpy (name, fileLAS.toLatin1 ());

          fprintf (stderr, "\n\n*** ERROR ***\nUnable to remove LAS file %s\n", name);
          fflush (stderr);
          exit (-1);
        }

      if (!bckFile.remove ())
        {
          char name[1024];
          strcpy (name, backFile.toLatin1 ());

          fprintf (stderr, "\n\n*** ERROR ***\nUnable to remove BCK file %s\n", name);
          fflush (stderr);
          exit (-1);
        }
    }
}


las_zero::~las_zero ()
{
}
