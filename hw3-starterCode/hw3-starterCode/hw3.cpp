/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Jimmy Lin
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <vector>
#include <glm/glm.hpp>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char * filename = NULL;

// The different display modes.
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

// While solving the homework, it is useful to make the below values smaller for debugging purposes.
// The still images that you need to submit with the homework should be at the below resolution (640x480).
// However, for your own purposes, after you have solved the homework, you can increase those values to obtain higher-resolution images.
#define WIDTH 640
#define HEIGHT 480
double aspectRatio = WIDTH /HEIGHT;

// The field of view of the camera, in degrees.
#define fov 60.0

// Buffer to store the image when saving it to a JPEG.
unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

struct Ray
{
  glm::vec3 origin;
  glm::vec3 direction;
};

glm::vec3 crossProduct(glm::vec3 vec0, glm::vec3 vec1) {
    glm::vec3 vec2;
    vec2.x = vec0.y * vec1.z - vec0.z * vec1.y;
    vec2.y = vec0.z * vec1.x - vec0.x * vec1.z;
    vec2.z = vec0.x * vec1.y - vec0.y * vec1.x;
    return vec2;
}

glm::vec3 crossProduct(glm::vec3 vec0, glm::vec3 vec1) {
    glm::vec3 vec2;
    vec2.x = vec0.y * vec1.z - vec0.z * vec1.y;
    vec2.y = vec0.z * vec1.x - vec0.x * vec1.z;
    vec2.z = vec0.x * vec1.y - vec0.y * vec1.x;
    return vec2;
}

float dotProduct(glm::vec3 vec0, glm::vec3 vec1) {
    return vec0.x * vec1.x + vec0.y * vec1.y + vec0.z * vec1.z;
}

bool IntersectSpheres(const Ray &ray, const Sphere &sphere, glm::vec3 &intersect) {
    glm::vec3 oc;
    oc.x = ray.origin.x - sphere.position[0];
    oc.y = ray.origin.y - sphere.position[1];
    oc.z = ray.origin.z - sphere.position[2];

    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0 * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false;
    }

    float t = (-b - sqrt(discriminant)) / (2 * a);
    if (t < 0) {
      t = (-b + sqrt(discriminant)) / (2 * a);
    }
    if (t < 0) {
      return false;
    }

    intersect = ray.origin + t * ray.direction;
    return true;
}

bool IntersectTriangles(const Ray &ray, const Triangle &triangle, glm::vec3 &intersect) {
    glm::vec3 v0;
    v0.x = triangle.v[0].position[0];
    v0.y = triangle.v[0].position[1];
    v0.z = triangle.v[0].position[2];

    glm::vec3 v1;
    v1.x = triangle.v[1].position[0];
    v1.y = triangle.v[1].position[1];
    v1.z = triangle.v[1].position[2];

    glm::vec3 v2;
    v2.x = triangle.v[2].position[0];
    v2.y = triangle.v[2].position[1];
    v2.z = triangle.v[2].position[2];

    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(edge1, edge2);
    h = glm::normalize(h);
    float d = glm::dot(h, v0);

    if (glm::dot(h, ray.direction) > -0.000001 && glm::dot(h, ray.direction) < 0.000001){
      return false;
    }
    float t = -(glm::dot(h, ray.origin) + d)/(glm::dot(h, ray.direction));

    if (t > 0.00001) { // ray intersection
        intersect = ray.origin + ray.direction * t;
        return true;
    }

    return false;
}

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

std::vector<Ray> cameraRays;

Ray createCameraRay(double x, double y){
  double mapX = 2*(x/WIDTH)-1;
  double mapY = 2*(y/HEIGHT)-1;
  double xRay = mapX * aspectRatio*tan(fov/2.0);
  double yRay = mapY * tan(fov/2.0);
  double zRay = -1.0;

  Ray newRay;
  newRay.direction.x = xRay;
  newRay.direction.y = yRay;
  newRay.direction.z = zRay;

  newRay.direction = glm::normalize(newRay.direction);

  newRay.origin.x = 0;
  newRay.origin.y = 0;
  newRay.origin.z = 0;

  return newRay;
}

void draw_scene()
{
  for(unsigned int x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);  
    // Do not worry about this usage of OpenGL. This is here just so that we can draw the pixels to the screen,
    // after their R,G,B colors were determined by the ray tracer.
    glBegin(GL_POINTS);
    for(unsigned int y=0; y<HEIGHT; y++)
    {
      Ray newRay = createCameraRay(x, y);
      cameraRays.push_back(newRay);
      // A simple R,G,B output for testing purposes.
      // Modify these R,G,B colors to the values computed by your ray tracer.
      unsigned char r = x % 256; // modify
      unsigned char g = y % 256; // modify
      unsigned char b = (x+y) % 256; // modify
      plot_pixel(x, y, r, g, b);
    }
    glEnd();
    glFlush();
  }
  printf("Ray tracing completed.\n"); 
  fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in saving\n");
  else 
    printf("File saved successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parsing error; abnormal program abortion.\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  if (!file)
  {
    printf("Unable to open input file %s. Program exiting.\n", argv);
    exit(0);
  }

  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  // Hack to make it only draw once.
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 3))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
  #endif
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}
