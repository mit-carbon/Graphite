/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

/********** storing/loading of large arrays to/from files **********/

#include <fcntl.h>
#include "incl.h"

#define	PMODE	0644		/* RW for owner, R for group, R for others */
#define	RWMODE	0		/* Read-only                               */

EXTERN_ENV

int Create_File(char filename[])
{
  int fd;
  if ((fd = creat(filename,PMODE)) == -1) {
    Error("    Can't create %s\n",filename);
  }
  return(fd);
}


int Open_File(char filename[])
{
  int fd;
  if ((fd = open(filename,RWMODE)) == -1) {
    Error("    Can't open %s\n",filename);
  }
  return(fd);
}


void Write_Bytes(int fd, unsigned char array[], long length)
{
  long n_written;
  long more_written;
  n_written = write(fd,array,MIN(length,32766));
  if (n_written != -1) {
    while (n_written < length) {
      more_written = write(fd,&array[n_written],
			   MIN(length-n_written,32766));
      if (more_written == -1) break;
      n_written += more_written;
    }
  }
  if (n_written != length) {
    Close_File(fd);
    Error("    Write failed on file %d\n",fd);
  }
}


void Write_Shorts(int fd, unsigned char array[], long length)
{
  long n_written;
  long more_written;
#ifdef FLIP
  for (i=0; i<length; i+=2) {
    byte = array[i];
    array[i] = array[i+1];
    array[i+1] = byte;
  }
#endif
  n_written = write(fd,array,MIN(length,32766));
  if (n_written != -1) {
    while (n_written < length) {
      more_written = write(fd,&array[n_written],
			   MIN(length-n_written,32766));
      if (more_written == -1) break;
      n_written += more_written;
    }
  }
  if (n_written != length) {
    Close_File(fd);
    Error("    Write failed on file %d\n",fd);
  }
#ifdef FLIP
  for (i=0; i<length; i+=2) {
    byte = array[i];
    array[i] = array[i+1];
    array[i+1] = byte;
  }
#endif
}


void Write_Longs(int fd, unsigned char array[], long length)
{
  long n_written;
  long more_written;
#ifdef FLIP
  for (i=0; i<length; i+=4) {
    byte = array[i];
    array[i] = array[i+3];
    array[i+3] = byte;
    byte = array[i+1];
    array[i+1] = array[i+2];
    array[i+2] = byte;
  }
#endif
  n_written = write(fd,array,MIN(length,32766));
  if (n_written != -1) {
    while (n_written < length) {
      more_written = write(fd,&array[n_written],
			   MIN(length-n_written,32766));
      if (more_written == -1) break;
      n_written += more_written;
    }
  }
  if (n_written != length) {
    Close_File(fd);
    Error("    Write failed on file %d\n",fd);
  }
#ifdef FLIP
  for (i=0; i<length; i+=4) {
    byte = array[i];
    array[i] = array[i+3];
    array[i+3] = byte;
    byte = array[i+1];
    array[i+1] = array[i+2];
    array[i+2] = byte;
  }
#endif
}


void Read_Bytes(int fd, unsigned char array[], long length)
{
  long n_read;
  long more_read;
  n_read = read(fd,array,MIN(length,32766));
  if (n_read != -1 && n_read != 0) {
    while (n_read < length) {
      more_read = read(fd,&array[n_read],
		       MIN(length-n_read,32766));
      if (more_read == -1 || more_read == 0) break;
      n_read += more_read;
    }
  }
  if (n_read != length) {
    Close_File(fd);
    Error("    Read failed on file %d\n",fd);
  }
}


void Read_Shorts(int fd, unsigned char array[], long length)
{
  long n_read;
  long more_read;
  n_read = read(fd,array,MIN(length,32766));
  if (n_read != -1 && n_read != 0) {
    while (n_read < length) {
      more_read = read(fd,&array[n_read],
		       MIN(length-n_read,32766));
      if (more_read == -1 || more_read == 0) break;
      n_read += more_read;
    }
  }
  if (n_read != length) {
    Close_File(fd);
    Error("    Read failed on file %d\n",fd);
  }
#ifdef FLIP
  for (i=0; i<length; i+=2) {
    byte = array[i];
    array[i] = array[i+1];
    array[i+1] = byte;
  }
#endif
}


void Read_Longs(int fd, unsigned char array[], long length)
{
  long n_read;
  long more_read;
  n_read = read(fd,array,MIN(length,32766));
  if (n_read != -1 && n_read != 0) {
    while (n_read < length) {
      more_read = read(fd,&array[n_read],
		       MIN(length-n_read,32766));
      if (more_read == -1 || more_read == 0) break;
      n_read += more_read;
    }
  }
  if (n_read != length) {
    Close_File(fd);
    Error("    Read failed on file %d\n",fd);
  }
#ifdef FLIP
  for (i=0; i<length; i+=4) {
    byte = array[i];
    array[i] = array[i+3];
    array[i+3] = byte;
    byte = array[i+1];
    array[i+1] = array[i+2];
    array[i+2] = byte;
  }
#endif
}


void Close_File(int fd)
{
  if (close(fd) == -1) {
    Error("    Can't close file %d\n",fd);
  }
}
