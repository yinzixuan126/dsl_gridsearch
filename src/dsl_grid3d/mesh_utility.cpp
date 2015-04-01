#include <cstdlib>
#include <limits>
#include <algorithm>

#include "dsl_grid3d/mesh_utility.h"

#include "trimesh2/TriMesh.h"
#include "trimesh2/TriMesh_algo.h"
#include "trimesh2/Vec.h"

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>

namespace dsl_grid3d
{

void MeshUtility::samplePointInTriangle(double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3, double z3, double* xout, double* yout, double* zout)
{
  double ux = x2-x1;
  double uy = y2-y1;
  double uz = z2-z1;
  double vx = x3-x1;
  double vy = y3-y1;
  double vz = z3-z1;

  double nx = uy*vz-uz*vy;
  double ny = uz*vx-ux*vz;
  double nz = ux*vy-uy*vx;

  double x = double(std::rand())/RAND_MAX;
  double y = double(std::rand())/RAND_MAX;
  while(x + y > 1)
  {
    x = double(std::rand())/RAND_MAX;
    y = double(std::rand())/RAND_MAX;
  }
  
  double xs = ux*x + vx*y;
  double ys = uy*x + vy*y;
  double zs;
  if(nz > 1e-6)
  {
    zs = (-nx*xs - ny*ys)/nz;
    if(xout)
      *xout = xs+x1;
    if(yout)
      *yout = ys+y1;
    if(zout)
      *zout = zs+z1;
  }
  else
  {
    if(xout)
      *xout = x1;
    if(yout)
      *yout = y1;
    if(zout)
      *zout = z1;
  }

}

bool MeshUtility::meshToOccupancyGrid(const std::string& map_filename, double cells_per_meter, OccupancyGrid** ogrid)
{
  const double occupied_val = DSL_OCCUPIED;
  double xmin, xmax, ymin, ymax, zmin, zmax;
  int length, width, height;
  trimesh::TriMesh *map = trimesh::TriMesh::read(map_filename);
  if(!map)
  {
    std::cout << "Could not open mesh: " << map_filename << std::endl;
    return false;
  }
  map->need_faces();

  
  xmin = ymin = zmin = std::numeric_limits<double>::max();
  xmax = ymax = zmax = std::numeric_limits<double>::min();
  for(unsigned int i = 0; i < map->vertices.size(); i++)
  {
    xmin = std::min(double(map->vertices[i][0]),xmin);
    ymin = std::min(double(map->vertices[i][1]),ymin);
    zmin = std::min(double(map->vertices[i][2]),zmin);
    xmax = std::max(double(map->vertices[i][0]),xmax);
    ymax = std::max(double(map->vertices[i][1]),ymax);
    zmax = std::max(double(map->vertices[i][2]),zmax);
  }

  length = (int)ceil((xmax - xmin)*cells_per_meter);
  width = (int)ceil((ymax - ymin)*cells_per_meter);
  height = (int)ceil((zmax - zmin)*cells_per_meter);

  double *occupancy_map = new double[length*width*height];
  if(!occupancy_map)
  {
    std::cout << "Failed to malloc occupancy map" << std::endl;
    return false;
  }

  for(int i = 0; i < length*width*height; i++)
  {
    occupancy_map[i] = 0;
  }

  for(unsigned int i = 0; i < map->vertices.size(); i++)
  {
    int x = (int)floor((map->vertices[i][0] - xmin)*cells_per_meter);
    int y = (int)floor((map->vertices[i][1] - ymin)*cells_per_meter);
    int z = (int)floor((map->vertices[i][2] - zmin)*cells_per_meter);
    int idx = x + y*length + z*length*width;
    assert(!(idx >= length*width*height || idx < 0));
    if(occupancy_map[idx] != occupied_val)
    {
      occupancy_map[idx] = occupied_val;
    }
  }

  // Sample points on mesh faces
  for(unsigned int i = 0; i < map->faces.size(); i++)
  {
    trimesh::point v1 = map->vertices[map->faces[i][0]];
    trimesh::point v2 = map->vertices[map->faces[i][1]];
    trimesh::point v3 = map->vertices[map->faces[i][2]];
    Eigen::Vector3d tv1(v2[0]-v1[0], v2[1]-v1[1], v2[2]-v1[2]);
    Eigen::Vector3d tv2(v3[0]-v1[0], v3[1]-v1[1], v3[2]-v1[2]);
    double tri_area = (tv1.cross(tv2)).norm()/2.;
    int num_samples = 2*tri_area*cells_per_meter;

    for(unsigned int j = 0; j < num_samples; j++)
    {
      double xs;
      double ys;
      double zs;
      samplePointInTriangle(v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], v3[0], v3[1], v3[2], &xs, &ys, &zs);
      int x = (int)floor((xs - xmin)*cells_per_meter);
      int y = (int)floor((ys - ymin)*cells_per_meter);
      int z = (int)floor((zs - zmin)*cells_per_meter);
      
      int idx = x + y*length + z*length*width;
      assert(!(idx >= length*width*height || idx < 0));
      if(occupancy_map[idx] != occupied_val)
      {
        occupancy_map[idx] = occupied_val;
      }
    }
  }

  *ogrid = new OccupancyGrid(occupancy_map, length, width, height, Eigen::Vector3d(xmin, ymin, zmin), Eigen::Vector3d(xmax, ymax, zmax), cells_per_meter);

  return true;
}

} // namespace
