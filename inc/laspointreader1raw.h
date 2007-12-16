/***************************************************************************
 * $Id$
 * $Date$
 *
 * Project: libLAS -- C/C++ read/write library for LAS LIDAR data
 * Purpose: Reads a point of type 1 (with gps_time) in standard LAS format 1.1
 * Author:  Martin Isenburg isenburg@cs.unc.edu 
 ***************************************************************************
 * Copyright (c) 2007, Martin Isenburg isenburg@cs.unc.edu 
 *
 * See LICENSE.txt in this source distribution for more information.
 **************************************************************************/

#ifndef LAS_POINT_READER_1RAW_H
#define LAS_POINT_READER_1RAW_H

#include "laspointreader.h"

#include <cstdio>

class LASpointReader1raw : public LASpointReader
{
public:
  inline bool read_point(LASpoint* point, double* gps_time = 0)
  {
    fread(point, sizeof(LASpoint), 1, file);
    return (fread(gps_time, sizeof(double), 1, file) == 1);
  };
  LASpointReader1raw(FILE* file){this->file = file;};
  ~LASpointReader1raw(){};
private:
  FILE* file;
};

#endif
