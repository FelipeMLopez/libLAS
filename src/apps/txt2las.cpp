/***************************************************************************
 * $Id$
 * $Date$
 *
 * Project: libLAS -- C/C++ read/write library for LAS LIDAR data
 * Purpose: ASCII text to LAS translation
 * Author:  Martin Isenburg isenburg@cs.unc.edu 
 ***************************************************************************
 * Copyright (c) 2007, Martin Isenburg isenburg@cs.unc.edu 
 *
 * See LICENSE.txt in this source distribution for more information.
 **************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "laswriter.h"

void usage()
{
  fprintf(stderr,"usage:\n");
  fprintf(stderr,"txt2las -parse tsxyz lidar.txt\n");
  fprintf(stderr,"txt2las -parse txyzar lidar.txt.gz lidar.laz\n");
  fprintf(stderr,"txt2las -parse xyz -scale 0.02 -i lidar.txt -o lidar.laz\n");
  fprintf(stderr,"txt2las -parse xyzsst -verbose -xyz_scale 0.05 lidar.txt\n");
  fprintf(stderr,"txt2las -parse xsysz -xyz_scale 0.02 0.02 0.01 lidar.txt\n");
  fprintf(stderr,"txt2las -h\n");
  fprintf(stderr,"---------------------------------------------\n");
  fprintf(stderr,"The '-parse tsxyz' flag specifies how to interpret\n");
  fprintf(stderr,"each line of the ASCII file. For example, 'tsxyzssa'\n");
  fprintf(stderr,"means that the first number is the gpstime, the next\n");
  fprintf(stderr,"number should be skipped, the next three numbers are\n");
	fprintf(stderr,"the x, y, and z coordinate, the next two should be\n");
	fprintf(stderr,"skipped, and the next number is the scan angle.\n");
	fprintf(stderr,"The other supported entries are i - intensity,\n");
	fprintf(stderr,"n - number of returns of given pulse, r - number\n");
	fprintf(stderr,"of return, c - classification, u - user data, and\n");
	fprintf(stderr,"p - point source ID, e - edge of flight line flag, and\n");
	fprintf(stderr,"d - direction of scan flag.\n");
  fprintf(stderr,"---------------------------------------------\n");
  fprintf(stderr,"The '-scale 0.02' flag specifies the quantization. The\n");
  fprintf(stderr,"default value of 0.01 means that the smallest increment\n");
  fprintf(stderr,"two between coordinates is 0.01. If measurements are in\n");
  fprintf(stderr,"meters this corresponds to centimeter accuracy, which is\n");
  fprintf(stderr,"commonly considered sufficient for LIDAR data.\n");
  fprintf(stderr,"---------------------------------------------\n");
  fprintf(stderr,"Other parameters such as '-xyz_offset 500000 2000000 0'\n");
  fprintf(stderr,"or '-xyz_scale 0.02 0.02 0.01' or '-file_creation 67 2003'\n");
  fprintf(stderr,"or '-system_identifier \"Airborne One Leica 50,000 Hz\"'\n");
  fprintf(stderr,"or '-generating_software \"TerraScan\"' are available too.\n");
  fprintf(stderr,"Read the source code for documentation.\n");
  exit(1);
}

static inline void VecUpdateMinMax3dv(double min[3], double max[3], const double v[3])
{
  if (v[0]<min[0]) min[0]=v[0]; else if (v[0]>max[0]) max[0]=v[0];
  if (v[1]<min[1]) min[1]=v[1]; else if (v[1]>max[1]) max[1]=v[1];
  if (v[2]<min[2]) min[2]=v[2]; else if (v[2]>max[2]) max[2]=v[2];
}

static inline void VecCopy3dv(double v[3], const double a[3])
{
  v[0] = a[0];
  v[1] = a[1];
  v[2] = a[2];
}

static bool parse(const char* parse_string, const char* line, double* xyz, LASpoint* point, double* gps_time)
{
	int temp_i;
	float temp_f;
	const char* p = parse_string;
	const char* l = line;

	while (p[0])
	{
		if (p[0] == 'x') // we expect the x coordinate
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%lf", &(xyz[0])) != 1) return false;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'y') // we expect the y coordinate
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%lf", &(xyz[1])) != 1) return false;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'z') // we expect the x coordinate
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%lf", &(xyz[2])) != 1) return false;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 's') // we expect a string or a number that we don't care about
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'i') // we expect the intensity
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%f", &temp_f) != 1) return false;
			if (temp_f < 0.0f || temp_f > 65535.0f) fprintf(stderr, "WARNING: intensity %g is out of range of unsigned short\n", temp_f);
			point->intensity = (unsigned short)temp_f;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'a') // we expect the scan angle
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%f", &temp_f) != 1) return false;
			if (temp_f < -128.0f || temp_f > 127.0f) fprintf(stderr, "WARNING: scan angle %g is out of range of char\n", temp_f);
			point->scan_angle_rank = (char)temp_f;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'n') // we expect the number of returns of given pulse
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%d", &temp_i) != 1) return false;
			if (temp_i < 0 || temp_i > 7) fprintf(stderr, "WARNING: return number %d is out of range of three bits\n", temp_i);
			point->number_of_returns_of_given_pulse = temp_i & 7;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'r') // we expect the number of the return
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%d", &temp_i) != 1) return false;
			if (temp_i < 0 || temp_i > 7) fprintf(stderr, "WARNING: return number %d is out of range of three bits\n", temp_i);
			point->return_number = temp_i & 7;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'c') // we expect the classification
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%d", &temp_i) != 1) return false;
			if (temp_i < 0 || temp_i > 255) fprintf(stderr, "WARNING: classification %d is out of range of unsigned char\n", temp_i);
			point->classification = (unsigned char)temp_i;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'u') // we expect the user data
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%d", &temp_i) != 1) return false;
			if (temp_i < 0 || temp_i > 255) fprintf(stderr, "WARNING: user data %d is out of range of unsigned char\n", temp_i);
			point->user_data = temp_i & 255;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'p') // we expect the point source ID
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%d", &temp_i) != 1) return false;
			if (temp_i < 0 || temp_i > 65535) fprintf(stderr, "WARNING: point source ID %d is out of range of unsigned short\n", temp_i);
			point->point_source_ID = temp_i & 65535;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'e') // we expect the edge of flight line flag
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%d", &temp_i) != 1) return false;
			if (temp_i < 0 || temp_i > 1) fprintf(stderr, "WARNING: edge of flight line flag %d is out of range of boolean flag\n", temp_i);
			point->edge_of_flight_line = (temp_i ? 1 : 0);
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 'd') // we expect the direction of scan flag
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%d", &temp_i) != 1) return false;
			if (temp_i < 0 || temp_i > 1) fprintf(stderr, "WARNING: direction of scan flag %d is out of range of boolean flag\n", temp_i);
			point->scan_direction_flag = (temp_i ? 1 : 0);
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else if (p[0] == 't') // we expect the gps time
		{
			while (l[0] && (l[0] == ' ' || l[0] == ',' || l[0] == '\t')) l++; // first skip white spaces
			if (l[0] == 0) return false;
			if (sscanf(l, "%lf", gps_time) != 1) return false;
			while (l[0] && l[0] != ' ' && l[0] != ',' && l[0] != '\t') l++; // then advance to next white space
		}
		else
		{
	    fprintf(stderr, "ERROR: next symbol '%s' unknown in parse control string\n", p);
		}
		p++;
	}
	return true;
}

#ifdef _WIN32
extern "C" FILE* fopenGzipped(const char* filename, const char* mode);
#endif

int main(int argc, char *argv[])
{
  int i;
  bool dry = false;
  bool verbose = false;
	char* file_name_in = 0;
	char* file_name_out = 0;
	bool itxt = false;
	bool olas = false;
	bool olaz = false;
	double xyz_min[3];
	double xyz_max[3];
	double xyz_scale[3] = {0.01,0.01,0.01};
	double xyz_offset[3] = {0.0,0.0,0.0};
  unsigned int number_of_point_records = 0;
  unsigned int number_of_points_by_return[8] = {0,0,0,0,0,0,0,0};
	char* parse_string = "xyz";
	int file_creation_day = 0;
	int file_creation_year = 0;
	char* system_identifier = 0;
	char* generating_software = 0;
#define MAX_CHARACTERS_PER_LINE 512
  char line[MAX_CHARACTERS_PER_LINE];
	double xyz[3];
	LASpoint point;
	double gps_time;

  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i],"-dry") == 0)
    {
      dry = true;
    }
    else if (strcmp(argv[i],"-verbose") == 0)
    {
      verbose = true;
    }
    else if (strcmp(argv[i],"-h") == 0)
    {
      usage();
    }
    else if (strcmp(argv[i],"-parse") == 0)
    {
			i++;
      parse_string = argv[i];
    }
    else if (strcmp(argv[i],"-scale") == 0)
    {
			i++;
			sscanf(argv[i], "%lf", &(xyz_scale[2]));
 			xyz_scale[0] = xyz_scale[1] = xyz_scale[2];
    }
    else if (strcmp(argv[i],"-xyz_scale") == 0)
    {
			i++;
			sscanf(argv[i], "%lf", &(xyz_scale[0]));
			i++;
			sscanf(argv[i], "%lf", &(xyz_scale[1]));
			i++;
			sscanf(argv[i], "%lf", &(xyz_scale[2]));
    }
    else if (strcmp(argv[i],"-xyz_offset") == 0)
    {
			i++;
			sscanf(argv[i], "%lf", &(xyz_offset[0]));
			i++;
			sscanf(argv[i], "%lf", &(xyz_offset[1]));
			i++;
			sscanf(argv[i], "%lf", &(xyz_offset[2]));
    }
    else if (strcmp(argv[i],"-i") == 0)
    {
      i++;
      file_name_in = argv[i];
    }
    else if (strcmp(argv[i],"-itxt") == 0)
    {
      itxt = true;
    }
    else if (strcmp(argv[i],"-o") == 0)
    {
      i++;
      file_name_out = argv[i];
    }
    else if (strcmp(argv[i],"-olas") == 0)
    {
      olas = true;
    }
    else if (strcmp(argv[i],"-olaz") == 0)
    {
      olaz = true;
    }
    else if (strcmp(argv[i],"-file_creation") == 0)
    {
			i++;
			sscanf(argv[i], "%d", &file_creation_day);
			i++;
			sscanf(argv[i], "%d", &file_creation_year);
		}
    else if (strcmp(argv[i],"-system_identifier") == 0 || strcmp(argv[i],"-sys_id") == 0)
    {
			i++;
			system_identifier = argv[i];
		}
    else if (strcmp(argv[i],"-generating_software") == 0 || strcmp(argv[i],"-gen_soft") == 0)
    {
			i++;
			generating_software = argv[i];
		}
		else if (i == argc - 2 && file_name_in == 0 && file_name_out == 0)
		{
			file_name_in = argv[i];
		}
		else if (i == argc - 1 && file_name_in == 0 && file_name_out == 0)
		{
			file_name_in = argv[i];
		}
		else if (i == argc - 1 && file_name_in && file_name_out == 0)
		{
			file_name_out = argv[i];
		}
	}

	// create output file name if none specified and no piped output requested (i.e. neither -olas nor -olaz)

	if (file_name_out == 0 && !olas && !olaz)
	{
		int len = strlen(file_name_in);
		file_name_out = strdup(file_name_in);
		if (file_name_out[len-3] == '.' || file_name_out[len-2] == 'g' || file_name_out[len-1] == 'z')
		{
			len = len - 4;
		}
		while (len > 0 && file_name_out[len] != '.')
		{
			len--;
		}
		file_name_out[len] = '.';
		file_name_out[len+1] = 'l';
		file_name_out[len+2] = 'a';
		file_name_out[len+3] = 's';
		file_name_out[len+4] = '\0';
	}

  // make sure that input and output are not *both* piped

  if (file_name_in == 0 && file_name_out == 0)
  {
		fprintf(stderr, "ERROR: input and output cannot both be pipes\n");
		exit(1);
  }

  // here we make *ONE* big switch. that replicates some code but makes it simple.

  if (file_name_out == 0)
  {
    // because the output goes to a pipe we have to precompute the header
    // information with an additional pass. the input must be a file.

    // open input file for first pass

	  FILE* file_in;

		if (strstr(file_name_in, ".gz"))
		{
#ifdef _WIN32
			file_in = fopenGzipped(file_name_in, "r");
#else
			fprintf(stderr, "ERROR: no support for gzipped input\n");
			exit(1);
#endif
		}
		else
		{
			file_in = fopen(file_name_in, "r");
		}

    if (file_in == 0)
		{
			fprintf(stderr, "ERROR: could not open '%s' for first pass\n",file_name_in);
			exit(1);
		}

	  // create a cheaper parse string that only looks for 'x' 'y' 'z' and 'r'

	  char* parse_less = strdup(parse_string);
	  for (i = 0; i < (int)strlen(parse_string); i++)
	  {
		  if (parse_less[i] != 'x' && parse_less[i] != 'y' && parse_less[i] != 'z' && parse_less[i] != 'r') 
		  {
			  parse_less[i] = 's';
		  }
	  }
	  do
	  {
		  parse_less[i] = '\0';
		  i--;
	  } while (parse_less[i] == 's');

	  // first pass to figure out the bounding box and number of returns

    fprintf(stderr, "first pass over file '%s' with parse '%s'\n", file_name_in, parse_less);

	  // read the first line

	  while (fgets(line, sizeof(char) * MAX_CHARACTERS_PER_LINE, file_in))
	  {
		  if (parse(parse_less, line, xyz, &point, &gps_time))
		  {
			  // init the bounding box
			  VecCopy3dv(xyz_min, xyz);
			  VecCopy3dv(xyz_max, xyz);
			  // mark that we found the first point
			  number_of_point_records = 1;
			  // create return histogram
			  number_of_points_by_return[point.return_number]++;
			  // we can stop this loop
			  break;
		  }
		  else
		  {
			  fprintf(stderr, "WARNING: cannot parse '%s' with '%s'. skipping ...\n", line, parse_less);
		  }
	  }

	  // did we manage to parse a line

	  if (number_of_point_records != 1)
	  {
		  fprintf(stderr, "ERROR: could not parse any lines with '%s'\n", parse_less);
		  exit(1);
	  }

	  // loop over the remaining lines

	  while (fgets(line, sizeof(char) * MAX_CHARACTERS_PER_LINE, file_in))
	  {
		  if (parse(parse_less, line, xyz, &point, &gps_time))
		  {
			  // update bounding box
			  VecUpdateMinMax3dv(xyz_min, xyz_max, xyz);
			  // count points
			  number_of_point_records++;
			  // create return histogram
			  number_of_points_by_return[point.return_number]++;
		  }
		  else
		  {
			  fprintf(stderr, "WARNING: cannot parse '%s' with '%s'. skipping ...\n", line, parse_less);
		  }
	  }

	  // output some stats
	  
	  if (verbose)
	  {
		  fprintf(stderr, "npoints %d min %g %g %g max %g %g %g\n", number_of_point_records, xyz_min[0], xyz_min[1], xyz_min[2], xyz_max[0], xyz_max[1], xyz_max[2]);
		  fprintf(stderr, "return histogram %d %d %d %d %d %d %d\n", number_of_points_by_return[0], number_of_points_by_return[1], number_of_points_by_return[2], number_of_points_by_return[3], number_of_points_by_return[4], number_of_points_by_return[5], number_of_points_by_return[6], number_of_points_by_return[7]);
	  }

	  // close the input file
	  
	  fclose(file_in);

	  // compute bounding box after quantization

	  int xyz_min_quant[3];
	  int xyz_max_quant[3];

	  for (i = 0; i < 3; i++)
	  {
		  xyz_min_quant[i] = (int)(0.5 + (xyz_min[i] - xyz_offset[i]) / xyz_scale[i]);
		  xyz_max_quant[i] = (int)(0.5 + (xyz_max[i] - xyz_offset[i]) / xyz_scale[i]);
	  }

	  double xyz_min_dequant[3];
	  double xyz_max_dequant[3];

	  for (i = 0; i < 3; i++)
	  {
		  xyz_min_dequant[i] = xyz_offset[i] + (xyz_min_quant[i] * xyz_scale[i]);
		  xyz_max_dequant[i] = xyz_offset[i] + (xyz_max_quant[i] * xyz_scale[i]);
	  }

	  // make sure there is not sign flip

#define log_xor !=0==!

	  for (i = 0; i < 3; i++)
	  {
		  if ((xyz_min[i] > 0) log_xor (xyz_min_dequant[i] > 0))
		  {
			  fprintf(stderr, "WARNING: quantization sign flip for %s min coord %g -> %g. use offset or scale up\n", (i ? (i == 1 ? "y" : "z") : "x"), xyz_min[i], xyz_min_dequant[i]);
		  }
		  if ((xyz_max[i] > 0) log_xor (xyz_max_dequant[i] > 0))
		  {
			  fprintf(stderr, "WARNING: quantization sign flip for %s max coord %g -> %g. use offset or scale up\n", (i ? (i == 1 ? "y" : "z") : "x"), xyz_max[i], xyz_max_dequant[i]);
		  }
	  }

#undef log_xor

	  // populate the header

	  LASheader header;

	  if (system_identifier) strncpy(header.system_identifier, system_identifier, 32);
	  if (generating_software) strncpy(header.generating_software, generating_software, 32);
	  header.file_creation_day = file_creation_day;
	  header.file_creation_year = file_creation_year;
	  if (strstr(parse_string,"t"))
	  {
		  header.point_data_format = 1;
		  header.point_data_record_length = 28;
	  }
	  else
	  {
		  header.point_data_format = 0;
		  header.point_data_record_length = 20;
	  }
	  header.number_of_point_records = number_of_point_records;
	  header.x_scale_factor = xyz_scale[0];
	  header.y_scale_factor = xyz_scale[1];
	  header.z_scale_factor = xyz_scale[2];
	  header.x_offset = xyz_offset[0];
	  header.y_offset = xyz_offset[1];
	  header.z_offset = xyz_offset[2];
	  header.min_x = xyz_min_dequant[0];
	  header.min_y = xyz_min_dequant[1];
	  header.min_z = xyz_min_dequant[2];
	  header.max_x = xyz_max_dequant[0];
	  header.max_y = xyz_max_dequant[1];
	  header.max_z = xyz_max_dequant[2];
	  header.number_of_points_by_return[0] = number_of_points_by_return[1];
	  header.number_of_points_by_return[1] = number_of_points_by_return[2];
	  header.number_of_points_by_return[2] = number_of_points_by_return[3];
	  header.number_of_points_by_return[3] = number_of_points_by_return[4];
	  header.number_of_points_by_return[4] = number_of_points_by_return[5];

	  // reopen input file for the second pass

	  if (strstr(file_name_in, ".gz"))
	  {
#ifdef _WIN32
		  file_in = fopenGzipped(file_name_in, "r");
#else
		  fprintf(stderr, "ERROR: no support for gzipped input\n");
		  exit(1);
#endif
	  }
	  else
	  {
		  file_in = fopen(file_name_in, "r");
	  }

	  if (file_in == 0)
	  {
		  fprintf(stderr, "ERROR: could not open '%s' for second pass\n",file_name_in);
		  exit(1);
	  }

	  // open the output pipe

    LASwriter* laswriter = new LASwriter();

    if (laswriter->open(stdout, &header, olaz ? 1 : 0) == false)
	  {
		  fprintf(stderr, "ERROR: could not open laswriter\n");
		  exit(1);
	  }

    fprintf(stderr, "second pass over file '%s' with parse '%s' writing to '%s'\n", file_name_in, parse_string, file_name_out ? file_name_out : "stdout");

	  // loop over points

	  while (fgets(line, sizeof(char) * MAX_CHARACTERS_PER_LINE, file_in))
	  {
		  if (parse(parse_string, line, xyz, &point, &gps_time))
		  {
			  point.x = (int)(0.5 + (xyz[0] - xyz_offset[0]) / xyz_scale[0]);
			  point.y = (int)(0.5 + (xyz[1] - xyz_offset[1]) / xyz_scale[1]);
			  point.z = (int)(0.5 + (xyz[2] - xyz_offset[2]) / xyz_scale[2]);
			  laswriter->write_point(&point, gps_time);
			  number_of_point_records--;
		  }
		  else
		  {
			  fprintf(stderr, "WARNING: cannot parse '%s' with '%s'. skipping ...\n", line, parse_string);
		  }
	  }

	  if (number_of_point_records)
	  {
		  fprintf(stderr, "WARNING: second pass has different number of points (%d instead of %d)\n", header.number_of_point_records - number_of_point_records, header.number_of_point_records);
	  }

	  laswriter->close();

	  if (verbose)
	  {
		  fprintf(stderr, "done.\n");
	  }

	  fclose(file_in);
  }
  else
  {
    // because the output goes to a file we can do everything in a single pass
    // and compute the header information along the way and then set it at the
    // end by fopen() the file with "rb+"

    // open input file

	  FILE* file_in;

    if (file_name_in)
	  {
		  if (strstr(file_name_in, ".gz"))
		  {
#ifdef _WIN32
			  file_in = fopenGzipped(file_name_in, "r");
#else
			  fprintf(stderr, "ERROR: no support for gzipped input\n");
			  exit(1);
#endif
		  }
		  else
		  {
			  file_in = fopen(file_name_in, "r");
		  }
		  if (file_in == 0)
		  {
			  fprintf(stderr, "ERROR: could not open input file '%s'\n",file_name_in);
			  exit(1);
		  }
	  }
	  else
	  {
      if (!itxt)
      {
  		  fprintf(stderr, "WARNING: no input specified. reading from stdin.\n");
      }
		  file_in = stdin;
	  }

	  // open output file

	  FILE* file_out = fopen(file_name_out, "wb");
    
    if (file_out == 0)
    {
			fprintf(stderr, "ERROR: could not open output file '%s'\n",file_name_out);
			exit(1);
	  }

    // populate header as much as possible (missing: bounding box, number of points, number of returns)

    LASheader header;

	  if (system_identifier) strncpy(header.system_identifier, system_identifier, 32);
	  if (generating_software) strncpy(header.generating_software, generating_software, 32);
	  header.file_creation_day = file_creation_day;
	  header.file_creation_year = file_creation_year;
	  if (strstr(parse_string,"t"))
	  {
		  header.point_data_format = 1;
		  header.point_data_record_length = 28;
	  }
	  else
	  {
		  header.point_data_format = 0;
		  header.point_data_record_length = 20;
	  }
	  header.x_scale_factor = xyz_scale[0];
	  header.y_scale_factor = xyz_scale[1];
	  header.z_scale_factor = xyz_scale[2];
	  header.x_offset = xyz_offset[0];
	  header.y_offset = xyz_offset[1];
	  header.z_offset = xyz_offset[2];

    // did the user request compressed output

	  int compression = 0;
		if (strstr(file_name_out, ".laz") || strstr(file_name_out, ".las.lz") || olaz)
		{
			compression = 1;
		}

    // create the las writer

    LASwriter* laswriter = new LASwriter();

	  if (laswriter->open(file_out, &header, compression) == false)
	  {
		  fprintf(stderr, "ERROR: could not open laswriter\n");
		  exit(1);
	  }

    fprintf(stderr, "scanning %s with parse '%s' writing to %s\n", file_name_in ? file_name_in : "stdin" , parse_string, file_name_out);

	  // read the first line

	  while (fgets(line, sizeof(char) * MAX_CHARACTERS_PER_LINE, file_in))
	  {
		  if (parse(parse_string, line, xyz, &point, &gps_time))
		  {
			  // init the bounding box
			  VecCopy3dv(xyz_min, xyz);
			  VecCopy3dv(xyz_max, xyz);
			  // we found the first point
			  number_of_point_records = 1;
			  // create return histogram
			  number_of_points_by_return[point.return_number]++;
        // compute the quantized x, y, and z values
        point.x = (int)(0.5 + (xyz[0] - xyz_offset[0]) / xyz_scale[0]);
			  point.y = (int)(0.5 + (xyz[1] - xyz_offset[1]) / xyz_scale[1]);
			  point.z = (int)(0.5 + (xyz[2] - xyz_offset[2]) / xyz_scale[2]);
        // write the first point
			  laswriter->write_point(&point, gps_time);
			  // we can stop this loop
			  break;
		  }
		  else
		  {
			  fprintf(stderr, "WARNING: cannot parse '%s' with '%s'. skipping ...\n", line, parse_string);
		  }
	  }

	  // did we manage to parse a line

	  if (number_of_point_records != 1)
	  {
		  fprintf(stderr, "ERROR: could not parse any lines with '%s'\n", parse_string);
		  exit(1);
	  }

	  // loop over the remaining lines

	  while (fgets(line, sizeof(char) * MAX_CHARACTERS_PER_LINE, file_in))
	  {
		  if (parse(parse_string, line, xyz, &point, &gps_time))
		  {
			  // update bounding box
			  VecUpdateMinMax3dv(xyz_min, xyz_max, xyz);
			  // count points
			  number_of_point_records++;
			  // create return histogram
			  number_of_points_by_return[point.return_number]++;
        // compute the quantized x, y, and z values
        point.x = (int)(0.5 + (xyz[0] - xyz_offset[0]) / xyz_scale[0]);
			  point.y = (int)(0.5 + (xyz[1] - xyz_offset[1]) / xyz_scale[1]);
			  point.z = (int)(0.5 + (xyz[2] - xyz_offset[2]) / xyz_scale[2]);
        // write the first point
			  laswriter->write_point(&point, gps_time);
		  }
		  else
		  {
			  fprintf(stderr, "WARNING: cannot parse '%s' with '%s'. skipping ...\n", line, parse_string);
		  }
	  }

    // done writing the points

	  if (file_in != stdin) fclose(file_in);
    laswriter->close();
    fclose(file_out);

    // output some stats
	  
	  if (verbose)
	  {
		  fprintf(stderr, "npoints %d min %g %g %g max %g %g %g\n", number_of_point_records, xyz_min[0], xyz_min[1], xyz_min[2], xyz_max[0], xyz_max[1], xyz_max[2]);
		  fprintf(stderr, "return histogram %d %d %d %d %d %d %d\n", number_of_points_by_return[0], number_of_points_by_return[1], number_of_points_by_return[2], number_of_points_by_return[3], number_of_points_by_return[4], number_of_points_by_return[5], number_of_points_by_return[6], number_of_points_by_return[7]);
	  }

	  // compute bounding box after quantization

	  int xyz_min_quant[3];
	  int xyz_max_quant[3];

	  for (i = 0; i < 3; i++)
	  {
		  xyz_min_quant[i] = (int)(0.5 + (xyz_min[i] - xyz_offset[i]) / xyz_scale[i]);
		  xyz_max_quant[i] = (int)(0.5 + (xyz_max[i] - xyz_offset[i]) / xyz_scale[i]);
	  }

	  double xyz_min_dequant[3];
	  double xyz_max_dequant[3];

	  for (i = 0; i < 3; i++)
	  {
		  xyz_min_dequant[i] = xyz_offset[i] + (xyz_min_quant[i] * xyz_scale[i]);
		  xyz_max_dequant[i] = xyz_offset[i] + (xyz_max_quant[i] * xyz_scale[i]);
	  }

	  // make sure there is not sign flip

#define log_xor !=0==!

	  for (i = 0; i < 3; i++)
	  {
		  if ((xyz_min[i] > 0) log_xor (xyz_min_dequant[i] > 0))
		  {
			  fprintf(stderr, "WARNING: quantization sign flip for %s min coord %g -> %g. use offset or scale up\n", (i ? (i == 1 ? "y" : "z") : "x"), xyz_min[i], xyz_min_dequant[i]);
		  }
		  if ((xyz_max[i] > 0) log_xor (xyz_max_dequant[i] > 0))
		  {
			  fprintf(stderr, "WARNING: quantization sign flip for %s max coord %g -> %g. use offset or scale up\n", (i ? (i == 1 ? "y" : "z") : "x"), xyz_max[i], xyz_max_dequant[i]);
		  }
	  }

#undef log_xor

	  // re-open output file to rewrite the missing header information

	  file_out = fopen(file_name_out, "rb+");
    
    if (file_out == 0)
    {
			fprintf(stderr, "ERROR: could not open re-output file '%s'\n",file_name_out);
			exit(1);
	  }

    // rewrite the information

    fseek(file_out, 107, SEEK_SET);
    fwrite(&number_of_point_records, sizeof(unsigned int), 5, file_out);

    fseek(file_out, 111, SEEK_SET);
    fwrite(&(number_of_points_by_return[1]), sizeof(unsigned int), 5, file_out);

    fseek(file_out, 179, SEEK_SET);
    fwrite(&(xyz_max_dequant[0]), sizeof(double), 1, file_out);
    fwrite(&(xyz_min_dequant[0]), sizeof(double), 1, file_out);
    fwrite(&(xyz_max_dequant[1]), sizeof(double), 1, file_out);
    fwrite(&(xyz_min_dequant[1]), sizeof(double), 1, file_out);
    fwrite(&(xyz_max_dequant[2]), sizeof(double), 1, file_out);
    fwrite(&(xyz_min_dequant[2]), sizeof(double), 1, file_out);

    // close the rewritten file

    fclose(file_out);

    if (verbose)
	  {
		  fprintf(stderr, "done.\n");
	  }
  }

	return 0;
}
