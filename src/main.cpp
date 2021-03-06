/* 
 * File:   main.cpp
 * Author: Jose J. Camata
 * Email: camata@gmail.com
 * Created on 24 de Março de 2014, 13:44
 */

#include "gmsh.h"

#include <cstdlib>
#include <cstdio>
#include <vector>
using namespace std;

#include <glib.h>
#include <gts.h>


void writeVTK(int nnodes,                   /* number of nodes */
                   int nelem ,                   /* number of elements */
                   vector<double>& coords,       /* nodes coordinates */
                   vector<int>   & elements,     /* element connectivity */
                   vector<int>   & element_tag,  /* element physical id */
                   const char *filename);         /* vtk file name */

// Read the gts file format and create a gts surface.
GtsSurface* SurfaceRead(const char* fname)
{
    FILE      *gts_file;
    GtsSurface *s;
    GtsPoint   *p;
    GtsFile    *fp;
    
    gts_file = fopen(fname, "r");
    if(!gts_file) return NULL;
    fp = gts_file_new(gts_file);
    s  = gts_surface_new(gts_surface_class (),
		         gts_face_class (),
		         gts_edge_class (),
		         gts_vertex_class ());
    if (gts_surface_read (s, fp)) {
        fputs ("file on standard input is not a valid GTS file\n", stderr);
        fprintf (stderr, "stdin:%d:%d: %s\n", fp->line, fp->pos, fp->error);
        return NULL; /* failure */
    }
    
    gts_file_destroy(fp);
    return s;
    
}
// Compute the distance between a point and a triangle.
gdouble  distance(GtsPoint *p, gpointer bounded)
{
    GtsTriangle *t = (GtsTriangle*) bounded;
    return gts_point_triangle_distance(p, t);
}


// Compute node id based on cartesian coordinates.
int get_id(int nx, int ny, int i, int j, int k)
{
	return (k*((nx+1)*(ny+1))+j*(nx+1)+i+1);
}


// Given a bottom left corner from cartesian cell, we get the hexahedra connectivety
void BuildHex(int nx, int ny, int i,int j, int k,int *nodes)
{
  nodes[0] = get_id(nx,ny,i  ,j  ,k);
  nodes[1] = get_id(nx,ny,i+1,j  ,k);
  nodes[2] = get_id(nx,ny,i+1,j+1,k);
  nodes[3] = get_id(nx,ny,i  ,j+1,k);
  nodes[4] = get_id(nx,ny,i  ,j  ,k+1);
  nodes[5] = get_id(nx,ny,i+1,j  ,k+1);
  nodes[6] = get_id(nx,ny,i+1,j+1,k+1);
  nodes[7] = get_id(nx,ny,i  ,j+1,k+1);
}


//Write the mesh data in gmsh ASCII format
void writeGmshMesh(int nnodes,                   /* number of nodes */
                   int nelem ,                   /* number of elements */
                   vector<double>& coords,       /* nodes coordinates */
                   vector<int>   & elements,     /* element connectivity */
                   vector<int>   & element_tag,  /* element physical id */
                   const char *filename)         /* gmsh file name */
{

	FILE *fout;

	int *id;
	char buffer[50];

	fout = fopen(filename, "w");
	
	double xyz[3];
	double version = 2.0, z, dz;
	int tmp;
	int nodes[8], header[5];
	int count;
	int npoints = nnodes;
	int nel     = nelem;
	fprintf(fout,"$MeshFormat\n");
        fprintf(fout,"2.2 0 8\n");
	fprintf(fout,"$EndMeshFormat\n");
	fprintf(fout,"$Nodes\n");
	fprintf(fout,"%10d\n", npoints);
	//npoints = 0;
	for(int i = 0; i < npoints; i++)
		fprintf(fout,"%d %8.3f %8.3f %8.3f\n",i+1,coords[i*3], coords[i*3+1], coords[i*3+2]);
			
	fprintf(fout,"$EndNodes\n");
	fprintf(fout,"$Elements\n");
	fprintf(fout,"%10d\n", nel);

	nel = 0;
	count = 0;
        for(int i = 0; i < nelem; i++)
	{
            //nel++;
            header[0] = i+1;
            header[1] = 5;
	    header[2] = 2;
	    header[3] = 99 ;//element_tag[i];
	    header[4] = 2;

	   fprintf(fout,"%d %d %d %d %d "     , header[0]      , header[1]      , header[2], header[3], header[4]); 
	   fprintf(fout, "%8d %8d %8d %8d "   , elements[i*8]  , elements[i*8+1], elements[i*8+2], elements[i*8+3]);
           fprintf(fout, "%8d %8d %8d %8d\n"  , elements[i*8+4], elements[i*8+5], elements[i*8+6], elements[i*8+7]);
					
	}

	fprintf(fout,"$EndElements\n");
	fclose(fout);
}


/*
 *    Main routine:
 *      Read Surface
 *      
 */
int run1(int argc, char** argv) {

    GtsSurface *s;
    GtsBBox    *box;
    GNode      *t;
    GtsPoint   *p;
    int    nx,ny,nz;
    double dx, dy, dz;
    
    // input data 
    // You can change this part
    nx = 30;
    ny = 30;
    nz  =30;
    double zmax = 30000.0;
    
    // Note that here we use a gts file.
    // There is a tool called stl2gts that convert STL files to GTS.
    // It is installed together with the gts library.
    s   = SurfaceRead("bedrock.gts");
    
    
    // Get the surface bounding box
    box = gts_bbox_surface (gts_bbox_class (), s);
    printf("Bounding box: \n");
    printf(" x ranges from %f to %f\n", box->x1, box->x2);
    printf(" y ranges from %f to %f\n", box->y1, box->y2);
    printf(" z ranges from %f to %f\n", box->z1, box->z2);
    
    // Get grid-spacing at x and y direction
    dx = (box->x2 - box->x1)/(double)nx;
    dy = (box->y2 - box->y1)/(double)ny;
    
    
    // Build the bounding box tree
    t = gts_bb_tree_surface(s);
    p = gts_point_new(gts_point_class(), 0.0 , 0.0 , box->z2);
    
    int nnodes = (nx+1)*(ny+1)*(nz+1);
    vector<double> coords(nnodes*3);
    
    int c = 0;
    for(int k=0; k <= nz; k++) {
        for(int j=0; j <= ny; j++) {
            for(int i=0; i <= nx; i++) {
                p->x = box->x1 + i*dx;
                p->y = box->y1 + j*dy;
                double d    = gts_bb_tree_point_distance(t,p,distance,NULL);
                double zmin = box->z2 - d;
                dz = (zmax-zmin)/(double)nz;
                coords[c*3 + 0] = p->x;
                coords[c*3 + 1] = p->y;
                coords[c*3 + 2] = zmin + k*dz;
                c++;
            }
        }
    }
    
    int nelem = nx*ny*nz;
    vector<int> elements(nelem*8);
    
    int nel=0;
    for(int k=0;k<nz;k++)
	for(int j=0; j < ny;  j++)
            for(int i=0; i < nx; i++)
            {
				BuildHex(nx,ny,i,j,k,&elements[nel*8]);
                nel++;
            }
    
//  writeGmshMesh(nnodes,nelem,coords,elements,"bedrock.msh");
    
    gts_bb_tree_destroy(t,false);
    gts_object_destroy (GTS_OBJECT (s));    
    gts_object_destroy (GTS_OBJECT (box));   
    return 0;
}




int BuildHexMeshFrom2DMesh(int argc, char** argv){

    GtsSurface *s;
    GtsBBox    *box;
    GNode      *t;
    GtsPoint   *p;
    
    Gmsh gmsh;

    if( argc != 4)
    {
		printf("%s [2d-Mesh (in Gmsh bin format)] [batimetria (in gts format)] [output file]\n", argv[0]);
		return 0;
    } 

    
    // Read 2D mesh generated from GMSH.
    gmsh.read(argv[1]);
    
    // Read the bathymetry
    s   = SurfaceRead(argv[2]);
    
    
    // Get the surface bounding box
    box = gts_bbox_surface (gts_bbox_class (), s);
    printf("Bounding box: \n");
    printf(" x ranges from %f to %f\n", box->x1, box->x2);
    printf(" y ranges from %f to %f\n", box->y1, box->y2);
    printf(" z ranges from %f to %f\n", box->z1, box->z2);
    printf("-----------------------------------------------------------\n");
	// Compute the depth
    // Here, 6000 meters above the deepest bathymetry's point.
    double zmin = box->z1-6000.0;
    double zmax = box->z2;
    
    // Build the bounding box tree
    t = gts_bb_tree_surface(s);
    p = gts_point_new(gts_point_class(), 0.0 , 0.0 , box->z1);
    
    int n_quad_elements = gmsh.getNumberofElements();
    int n_base_nodes    = gmsh.getNumberofNodes();
    
    // Number of cells at z-direction
    int nz     =  30;
    double dz;
    
    // Compute number od nodes and cells
    int n_total_nodes     = (nz+1)*n_base_nodes;
    int n_total_elements = n_quad_elements*nz; 
    

    vector<double> coords(n_total_nodes*3);
    
	// Some stats.
    double dzmin, dzmax;
    dzmin  = 1.0E10;
    dzmax  = -1.0E10;


    int c = 0;
    for(int k=0; k <= nz; k++) {
        for(int nb = 0; nb < n_base_nodes; nb++){
            double x = gmsh.getX(nb);
            double y = gmsh.getY(nb);
            p->x     = x;
            p->y     = y;
            double d    = gts_bb_tree_point_distance(t,p,distance,NULL);
            double zmax = box->z1+d;
            dz = (zmax-zmin)/(double)nz;
            dz<dzmin?dzmin=dz:dzmin=dzmin;
	    dz>dzmax?dzmax=dz:dzmax=dzmax;
            double z = k*dz;
            int id_node = (k*n_base_nodes + nb);
            coords[id_node*3+0] = x;
            coords[id_node*3+1] = y;
            coords[id_node*3+2] = zmin + z;
            c++;
        }
    }
    
   
    printf("DZ min: %10.10f\n", dzmin);
    printf("DZ max: %10.10f\n", dzmax);
    printf("-----------------------------------------------------------\n");

    vector<int> elements(n_total_elements*8);
    vector<int> element_tag(n_total_elements);
    
    // build connectivities
    int nel=0;
    for(int k=0;k<nz;k++) {
        for(int iel = 0; iel < n_quad_elements; iel++ ) {
            GmshElement* elem = gmsh.getElement(iel);
            std::vector<int> conn      = elem->getConnectivity(); 
            elements[nel*8+0] = k*n_base_nodes + conn[0];
            elements[nel*8+1] = k*n_base_nodes + conn[1];
            elements[nel*8+2] = k*n_base_nodes + conn[2];
            elements[nel*8+3] = k*n_base_nodes + conn[3];
            elements[nel*8+4] = (k+1)*n_base_nodes + conn[0];
            elements[nel*8+5] = (k+1)*n_base_nodes + conn[1];
            elements[nel*8+6] = (k+1)*n_base_nodes + conn[2];
            elements[nel*8+7] = (k+1)*n_base_nodes + conn[3];
            element_tag[nel] = elem->getPhysicalID(); //foo
            nel++;
        }
    }
    
    printf("Mesh Generation Info: \n");
    printf("Number of nodes: %d\n" , n_total_nodes);
    printf("Number of elements: %d\n", n_total_elements);
    printf("-----------------------------------------------------------\n");

    // Save in Gmsh ASCII format.
    writeGmshMesh(n_total_nodes,n_total_elements,coords,elements,element_tag,argv[3]);
   
    gts_bb_tree_destroy(t,false);
    gts_object_destroy (GTS_OBJECT (s));    
    gts_object_destroy (GTS_OBJECT (box)); 
    gts_object_destroy (GTS_OBJECT (p)); 
 
    
    return 0;
}


int main(int argc, char*argv[])
{
    return BuildHexMeshFrom2DMesh(argc, argv);
}


