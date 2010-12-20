//compile with g++ -O3 itm.cpp lrprog.cpp -o lrprog

//02-24-2009 - v1.2 Fixed terrain elevation bug for elevations below sea level.

#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <string>
#include <vector>
#include <functional>

using namespace std;

#define pi 3.14159265358979323846
//#define MAPSPATH "/export/home/aneplokh/maps"
#define MAPSPATH "/content/fcc/dtv-coverage-maps/maps"

ifstream* file_array = new ifstream[18100];
int file_array_flag[18100];

double get_elevation( int lat, int lon) {
	//elevation files are stored as 1 degree lat by 1 degree lon files, 1200 points by 1200 points, 2 bytes per point.  This subroutine finds the offset in the proper elevation file for a given lat/lon and reads the elevation from the file.


	int bottom_edge = lat/12000000;
	int right_edge = lon/12000000;

	int pixel_lat = (lat - bottom_edge * 12000000)/10000;
	int pixel_lon = (lon - right_edge * 12000000)/10000;

	int file_id = 100*right_edge + bottom_edge;
	if (file_id >= 18100) {
		//makes sure that I'm not reading or writing out of bounds
		file_id = 0;
	} 

	char buffer[2];
	int elev;

	if (!(file_array_flag[file_id])) {
		//file not yet tried so attempt to open it.

		ostringstream filename;
		filename << MAPSPATH << "/" << bottom_edge << ":" << bottom_edge+1 << ":" << right_edge << ":" << right_edge+1 << ".jgc";
		string file_string = filename.str();
		file_array[file_id].open(file_string.c_str(), ios::binary);
		
		if(!(file_array[file_id])) {
			//open still failed, so flag as nonexistant.
			file_array_flag[file_id]=1;
		}else{
			//open succeeded, so flag as populated.
			file_array_flag[file_id]=2;
		}
	}
	
	if (file_array_flag[file_id]==1){
		//file has been marked as non-existant, so assume sea level
		elev = 0;
	}else{
		//file must be there, so let's read from it.
		int offset = 2*(1200*pixel_lat + pixel_lon + 4);
		file_array[file_id].seekg(offset);
		file_array[file_id].read(buffer,2);
		elev = 256*static_cast<unsigned char>(buffer[1]) + static_cast<unsigned char>(buffer[0]);
	}

	//02-24-2009 - v1.2
	if(elev>40000) {
		elev-=65536;
	}

	return static_cast<double>(elev);
}

int read_path(int tx_lat, int tx_lon, int rx_lat, int rx_lon, int num_steps, double *array) {
	int lat_increment = (rx_lat - tx_lat)/num_steps;
	int lon_increment = (rx_lon - tx_lon)/num_steps;

	int lat_pos = tx_lat;
	int lon_pos = tx_lon;

	array[2] = get_elevation(tx_lat,tx_lon);
	for (int i=1; i<num_steps; i++) {
		lat_pos+=lat_increment;
		lon_pos+=lon_increment;
		array[i+2] = get_elevation(lat_pos,lon_pos);
	}
	array[num_steps+2] = get_elevation(rx_lat,rx_lon);
}

double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  //calculates the great-circle distance between two lat/lon points.
  double theta, dist;
  theta = lon1 - lon2;
  dist = sin(pi/180*(lat1)) * sin(pi/180*(lat2)) + cos(pi/180*(lat1)) * cos(pi/180*(lat2)) * cos(pi/180*(theta));
  dist = acos(dist);
  dist = 180/pi*(dist);
  dist = dist * 60 * 1.1515;
  switch(unit) {
    case 'M':
      break;
    case 'K':
      dist = dist * 1.609344;
      break;
    case 'N':
      dist = dist * 0.8684;
      break;
  }
  return (dist);
}

void point_to_point(double elev[], double tht_m, double rht_m,
	  double eps_dielect, double sgm_conductivity, double eno_ns_surfref,
	  double frq_mhz, int radio_climate, int pol, double conf,
	  double rel, double &dbloss, char *strmode, int &errnum);

void split(const string& s, char c, vector<string>& v) {
  string::size_type i = 0;
  string::size_type j = s.find(c);
  
  while (j != string::npos) {
    v.push_back(s.substr(i, j-i));
    i = ++j;
    j = s.find(c,j);
    
    if (j == string::npos)
      v.push_back(s.substr(i, s.length()));
    
  }
}


int main (int argc,char *argv[]) {
	double tx_lat;
	double tx_lon;
	double rx_lat;
	double rx_lon;
	double increment;
	double frq_mhz;
	double tht_m;
	double rht_m;
	double conf;			//% of locations
	double rel;			//% of time
	double tx_elev;
	string tx_id;
	string rx_id;

	//constants:
	const double eps_dielect=15.000;
	const double sgm_conductivity=0.005;
	const double eno_ns_surfref=301.0;
	const int radio_climate=5;		//radio climate 5-Continental Temperate
	const int pol=0; 			//polarization 0-Horizontal
	double dbloss;
	char strmode[50];
	int errnum;

	//number of test paths should be less than, say 1200 for this little limited purpose program
	int num_of_test_paths;
	string test_path_strings[1200];

	//the program should be called with argv[1]=num of test paths (=tx_sites * rx_sites);
	sscanf(argv[1],"%d",&num_of_test_paths);

	//loop through to collect all of the test paths
	for (int i=0; i < num_of_test_paths; i++) {
	  //do the stdin thing.
	  string temp;
	  cin >> temp;
	  test_path_strings[i]=temp;
	}

	//now loop through the test strings and compute them
	for (int i=0; i < num_of_test_paths; i++) {
	  vector<string> v;
	  split(test_path_strings[i], '|' , v);
	  
	  sscanf(v[0].c_str(),"%lf",&tx_lat);
	  sscanf(v[1].c_str(),"%lf",&tx_lon);
	  sscanf(v[2].c_str(),"%lf",&rx_lat);
	  sscanf(v[3].c_str(),"%lf",&rx_lon);
	  sscanf(v[4].c_str(),"%lf",&increment);
	  sscanf(v[5].c_str(),"%lf",&frq_mhz);
	  sscanf(v[6].c_str(),"%lf",&tht_m);
	  sscanf(v[7].c_str(),"%lf",&rht_m);
	  sscanf(v[8].c_str(),"%lf",&conf);
	  sscanf(v[9].c_str(),"%lf",&rel);
	  sscanf(v[10].c_str(),"%lf",&tx_elev);
	  
	  
	  //calculate distance between points
	  double dist = distance(tx_lat,tx_lon,rx_lat,rx_lon,'K');
	  if (dist < 1.) {
	    dist = 1.;
	  }
	  
	  if (increment < .05) {
		increment = .05;
	  }

	  int num_of_points = static_cast<int>(dist/increment);

	  if (num_of_points < 1) {
	    num_of_points = 1;
	  }else if (num_of_points > 20000) {
	    num_of_points = 20000;
	  }
	  
	  double dist_between_points = 1000*dist/(static_cast<double>(num_of_points));

	  //validate frq_mhz
	  if (frq_mhz < 54.) {
		frq_mhz = 54.;
	  }else if (frq_mhz > 806.) {
		frq_mhz = 806.;
	  }
	  	
	  double *elev_array = NULL;
	  elev_array = new double[num_of_points+3];
	  
	  //now let's move into all integer math!
	  
	  tx_lat*=12000000;
	  tx_lon*=12000000;
	  rx_lat*=12000000;
	  rx_lon*=12000000;
	  
	  int tx_lat_int = static_cast<int>(tx_lat);
	  int tx_lon_int = static_cast<int>(tx_lon);
	  int rx_lat_int = static_cast<int>(rx_lat);
	  int rx_lon_int = static_cast<int>(rx_lon);
	  
	  read_path(tx_lat_int, tx_lon_int, rx_lat_int, rx_lon_int, num_of_points, elev_array);
	  
	  elev_array[0]=static_cast<double>(num_of_points);
	  elev_array[1]=dist_between_points;

	  if(tx_elev)
		elev_array[2]=tx_elev;
	  
	  point_to_point(elev_array,tht_m,rht_m,eps_dielect,sgm_conductivity,eno_ns_surfref,frq_mhz,radio_climate,pol,conf,rel,dbloss,strmode,errnum);
	  
	  cout << v[11] << "," << v[12] << "," << dbloss << "|";
	}
	cout << endl;
	return 1;
}


