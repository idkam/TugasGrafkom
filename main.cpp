#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"
#endif



//static GLfloat spin, spin2 = 0.0;
float angle = 0;
using namespace std;

float lastx, lasty;
GLint stencilBits;
static int viewx = -110;
static int viewy = 40;
static int viewz =160;

float rot = 0;

GLuint texture[1]; //array untuk texture

struct Images {//tempat image
	unsigned long sizeX;
	unsigned long sizeY;
	char *data;
};
typedef struct Images Images;

//class untuk terain 2D
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;//memperhalus
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//end class
//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
//load terain di procedure inisialisasi
Terrain* loadTerrain(const char* filename, float height) {//ngambil file
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 0.0f;

Terrain* _terrain;



void cleanup() {//untuk mehilangin file image

	delete _terrain;

}
//mengambil gambar BMP
int ImageLoad(char *filename, Images *image) {
	FILE *file;
	unsigned long size; // ukuran image dalam bytes
	unsigned long i; // standard counter.
	unsigned short int plane; // number of planes in image

	unsigned short int bpp; // jumlah bits per pixel
	char temp; // temporary color storage for var warna sementara untuk memastikan filenya ada


	if ((file = fopen(filename, "rb")) == NULL) {
		printf("File Not Found : %s\n", filename);
		return 0;
	}
	// mencari file header bmp
	fseek(file, 18, SEEK_CUR);
	// read the width
	if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {//lebar beda
		printf("Error reading width from %s.\n", filename);
		return 0;
	}
	// membaca nilai height
	if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {//tingginya beda
		printf("Error reading height from %s.\n", filename);
		return 0;
	}
	//menghitung ukuran image(asumsi 24 bits or 3 bytes per pixel).

	size = image->sizeX * image->sizeY * 3;
	// read the planes
	if ((fread(&plane, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);//bukan file bmp
		return 0;
	}
	if (plane != 1) {
		printf("Planes from %s is not 1: %u\n", filename, plane);//
		return 0;
	}
	// read the bitsperpixel
	if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);

		return 0;
	}
	if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);//bukan 24 pixel
		return 0;
	}
	// seek past the rest of the bitmap header.
	fseek(file, 24, SEEK_CUR);
	// read the data.
	image->data = (char *) malloc(size);
	if (image->data == NULL) {
		printf("Error allocating memory for color-corrected image data");//gagal ambil memory
		return 0;
	}
	if ((i = fread(image->data, size, 1, file)) != 1) {
		printf("Error reading image data from %s.\n", filename);
		return 0;
	}
	for (i = 0; i < size; i += 3) { // membalikan semuan nilai warna (gbr - > rgb)
		temp = image->data[i];
		image->data[i] = image->data[i + 2];
		image->data[i + 2] = temp;
	}
	// we're done.
	return 1;
}





void initRendering() {//inisialisasi
	glEnable(GL_DEPTH_TEST);//kedalaman
	glEnable(GL_COLOR_MATERIAL);//warna
	glEnable(GL_LIGHTING);//cahaya
	glEnable(GL_LIGHT0);//lampu
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

void drawScene() {//buat terain
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	float scale = 500.0f / max(_terrain->width() - 1, _terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (_terrain->width() - 1) / 2, 0.0f,
			-(float) (_terrain->length() - 1) / 2);

	glColor3f(0.3f, 0.9f, 0.0f);



}


// display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {

	float scale = 500.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}

GLuint loadtextures(const char *filename, int width, int height) {//buat ngambil dari file image untuk jadi texture
	GLuint texture;

	unsigned char *data;
	FILE *file;


	file = fopen(filename, "rb");
	if (file == NULL)
		return 0;

	data = (unsigned char *) malloc(width * height * 3);  //file pendukung texture
	fread(data, width * height * 3, 1, file);

	fclose(file);

	glGenTextures(1, &texture);//generet (dibentuk)
	glBindTexture(GL_TEXTURE_2D, texture);//binding (gabung)
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			GL_LINEAR_MIPMAP_NEAREST);//untuk membaca gambar jadi text dan dapat dibaca dengan pixel
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameterf(  GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB,
			GL_UNSIGNED_BYTE, data);

	data = NULL;

	return texture;
}

const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };





void masjid(void) {

     //atap atas
    glPushMatrix();
    glScaled(0.2, 0.2, 0.2);//untuk mengatur ukuran benda
    glTranslatef(0.0, 80, -10); //untuk mengatur koordinat 3d
    glRotated(225, 0, 1, 0);
    glRotated(90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.803921568627451, 0.5215686274509804, 0.2470588235294118);
    glutSolidCone(4.2, 4, 4, 1);
    glPopMatrix();




       //atap
    glPushMatrix();
    glScaled(1, 1.0, 1);//untuk mengatur ukuran benda
    glTranslatef(0.0, 12, -1.9); //untuk mengatur koordinat 3d
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.48, 0.46, 0.46);
    glutSolidCone(4.2, 4, 4, 1);
    glPopMatrix();


//Dinding Kiri Atas

    glPushMatrix();
     glScalef(0.04, 0.4, 1); //untuk mengatur ukuran benda
    glTranslatef(-56, 29, -1.7); //untuk mengatur koordinat 3d
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);
    glutSolidCube(4.2f);
    glPopMatrix();


//Dinding Kanan Atas
    glPushMatrix();
    glScaled(0.04, 0.4, 1);//untuk mengatur ukuran benda
    glTranslatef(56.0, 29, -1.7);//untuk mengatur koordinat 3d
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);
    glutSolidCube(4.2f);
    glPopMatrix();


  //Dinding Belakang atas
    glPushMatrix();
    //glScaled(0.035, 0.5, 0.8);
    glScaled(0.95, 0.4, 0.075);//untuk mengatur ukuran benda
    glTranslatef(0, 29.5,-49);//untuk mengatur koordinat 3d
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);
    glutSolidCube(5.0);
    glPopMatrix();

  //Dinding Depan atas
    glPushMatrix();
    glScaled(0.95, 0.4, 0.075);
    glTranslatef(0,29.5,2.3);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);
    glutSolidCube(5.0);
    glPopMatrix();

   //atap 2

    glPushMatrix();
    glScaled(1.5,1.5,1.5);
    glTranslatef(0,5.5, -1.15);
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.48, 0.46, 0.46);
    glutSolidCone(4.2, 4, 4, 1);
    glPopMatrix();

  //Dinding Depan atas2
    glPushMatrix();
    glScaled(1.5, 0.3,1.5);
    glTranslatef(0,26,-1.1);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.4613, 0.4627, 0.4174);
    glutSolidCube(5.0);
    glPopMatrix();



    //atap 3

    glPushMatrix();
    glScaled(3,1.5,3);
    glTranslatef(0,5.-2.75, -0.53);
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.48, 0.46, 0.46);
    glutSolidCone(4.2, 4, 4,2);
    glPopMatrix();


    glPushMatrix();
    glScaled(3,1.5,3);
    glTranslatef(0,5.-2.75, -0.53);
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0, 0, 0);
    glutWireCone(4.2, 4, 4, 40);
    glPopMatrix();


      //Dinding Depan atas2
    glPushMatrix();
    glScaled(2.5, 0.6,2.5);
    glTranslatef(0,3,-1.2);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.95, 0.43, 0.0);
    glutSolidCube(5.0);
    glPopMatrix();


    //tiang
    float zaksis=180;
    float xaksis=75;
    float xaksis1=37.5;
    float zaksis1=89.5;
for (int i=0;i<10;i++){

//penyagga
  glPushMatrix();
    glScaled(0.2, 0.26,0.2);
    glTranslatef(xaksis1,2,zaksis1);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.3, 0.1, 0);
    glutSolidCube(5.0);
    glPopMatrix();


    glPushMatrix();
    glScaled(0.1, 0.5,0.1);
    glTranslatef(xaksis,5,zaksis);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();

    //penyagga kiri
      glPushMatrix();
    glScaled(0.2, 0.26,0.2);
    glTranslatef(-37,2,zaksis1);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.3, 0.1, 0);
    glutSolidCube(5.0);
    glPopMatrix();

    //tiang kiri
       glPushMatrix();
    glScaled(0.1, 0.5,0.1);
    glTranslatef(-75,5,zaksis);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();

    zaksis-=30;
    zaksis1-=14.95;
}
//tiang depan
for (int i=0;i<10;i++){




//penyangga


  glPushMatrix();
    glScaled(0.2, 0.26,0.2);
    glTranslatef(xaksis1,2,89.5);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.3, 0.1, 0);
    glutSolidCube(5.0);
    glPopMatrix();
xaksis1-=8.25;

//tiang depan


      glPushMatrix();
    glScaled(0.1, 0.5,0.1);
    glTranslatef(xaksis,5,180);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();
    xaksis-=16.4595;
}

  //atas depan
    glPushMatrix();
    glScaled(3.5, 0.05,2.85);
    glTranslatef(0,68,5);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();

  //Lantai
    glPushMatrix();
    glScaled(3.5, 0.05,8);
    glTranslatef(0,2,0); glPushMatrix();
    glScaled(1.5, 1.5, 1.5);//untuk mengatur ukuran benda
    glTranslatef(85.3, 25, 90); //untuk mengatur koordinat 3d
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.803921568627451, 0.5215686274509804, 0.2470588235294118);
    glutSolidCone(4.2, 4, 4, 1);
    glPopMatrix();
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();


}

void atapdepan(void){


      glRotatef(_angle, 0.0f, 0.0f, 0.0f);
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
glColor3d(0.48, 0.46, 0.46);

      //glScalef(1.0f, 1.5f, 1.0f);
      glBegin(GL_QUADS);

      //depan

      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(-1.5f, -1.0f, 1.5f);
      //glNormal3f(1.0f, 0.0f, 1.0f);
      glTexCoord2f(1.0, 0.0f);
      glVertex3f(1.5f, -1.0f, 1.5f);
            //glNormal3f(1.0f, 0.0f, 1.0f);
      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(1.1f, 1.0f, 0.0f);
      //glNormal3f(-1.0f, 0.0f, 1.0f);
      glColor3d(0.48, 0.46, 0.90);

      glTexCoord2f(0.0f, 1.0f);
      glVertex3f(-1.1f, 1.0f, 0.0f);

      //kanan
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
glColor3d(0.48, 0.46, 0.46);


      glVertex3f(1.5f, -1.0f, -1.5f);
      //glNormal3f(1.0f, 0.0f, -1.0f);
      glVertex3f(1.1f, 1.0f, 0.0f);
      //glNormal3f(1.0f, 0.0f, 1.0f);
      glColor3d(0.48, 0.46, 0.90);
      glVertex3f(1.5f, -1.0f, 1.5f);
      //glNormal3f(1.0f, 0.0f, 1.0f);

      glVertex3f(1.5f, -1.0f, -1.5f);

      //belakang
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
glColor3d(0.48, 0.46, 0.46);


      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(-1.5f, -1.0f, -1.5f);

      glTexCoord2f(1.0, 0.0f);
      glVertex3f(-1.1f, 1.0f, 0.0f);

      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(1.1f, 1.0f, 0.0f);

      glTexCoord2f(0.0f, 1.0f);
      glVertex3f(1.5f, -1.0f, -1.5f);

      //kiri
glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
glColor3d(0.48, 0.46, 0.46);


      glVertex3f(-1.5f, -1.0f, -1.5f);
      //glNormal3f(-1.0f, 0.0f, 1.0f);
      glColor3d(0.48, 0.46, 0.90);
      glVertex3f(-1.5f, -1.0f, 1.5f);
      glVertex3f(-1.1f, 1.0f, 0.0f);
      glVertex3f(-1.5f, -1.0f, -1.5f);


      glEnd();

}


//menara
void menara(void){

 glPushMatrix();
    glScaled(3, 0.3,3);
    glTranslatef(-20,80,30);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0, 0, 0);
    glutSolidCube(5.0);
    glPopMatrix();



}

void pagar(void){

float xaksis=-90.0;
float xaksis2=-21;
for (int i=0;i<11;i++){



    glPushMatrix();
    glScaled(1, 3,0.5);
    glTranslatef(xaksis,10,260);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();
    //pagar pinggirnya
    glPushMatrix();
    glScaled(4, 2,0.5);
    glTranslatef(xaksis2,10,260);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();

    //corak
    glPushMatrix();
    glScaled(1, 2,0.5);
    glTranslatef(xaksis2,10,260);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();


xaksis2+=5;
   xaksis+=20;

}
//patok
    glPushMatrix();
    glScaled(2, 4,2);
    glTranslatef(64,6.5,67);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glScaled(1.6, 0.5,1.6);
    glTranslatef(80,72,84);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0, 0, 0);
    glutSolidCube(5.0);
    glPopMatrix();

       glPushMatrix();
    glScaled(1.5, 1.5, 1.5);//untuk mengatur ukuran benda
    glTranslatef(85.3, 25, 90); //untuk mengatur koordinat 3d
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.803921568627451, 0.5215686274509804, 0.2470588235294118);
    glutSolidCone(4.2, 4, 4, 1);
    glPopMatrix();

//patok kanan
      glPushMatrix();
    glScaled(2, 4,2);
    glTranslatef(85,6.5,67);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();

     glPushMatrix();
    glScaled(1.6, 0.5,1.6);
    glTranslatef(106.35,72,84);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0, 0, 0);
    glutSolidCube(5.0);
    glPopMatrix();

     glPushMatrix();
    glScaled(1.5, 1.5, 1.5);//untuk mengatur ukuran benda
    glTranslatef(113.3, 25, 90); //untuk mengatur koordinat 3d
    glRotated(45, 0, 1, 0);
    glRotated(-90, 1, 0, 0);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3d(0.803921568627451, 0.5215686274509804, 0.2470588235294118);
    glutSolidCone(4.2, 4, 4, 1);
    glPopMatrix();


//pagar kanan
float xas=45;
float xas1=187;
for(int i=0;i<10;i++){

glPushMatrix();
    glScaled(4, 2,0.5);
    glTranslatef(xas,10,260);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();

     glPushMatrix();
    glScaled(1, 3,0.5);
    glTranslatef(xas1,10,260);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();
    xas+=5;
    xas1+=20;

}


}


void display(void){
//    glutSwapBuffers();
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);

	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();//reset posisi
	gluLookAt(viewx, viewy, viewz, 0.0, 0.0, -100.0, 0.0, 1.0, 0.0);


    glPushMatrix();
    drawScene();
    glPopMatrix();

    glPushMatrix();

	glBindTexture(GL_TEXTURE_2D, texture[1]); //untuk mmanggil texture
	drawSceneTanah(_terrain, 0.3f, 0.53999999f, 0.0654f);
	glPopMatrix();


//masjid 1

glPushMatrix();
glTranslatef(0,5,-10);
glScalef(5, 5, 5);
//glBindTexture(GL_TEXTURE_2D, texture[0]);
masjid();
glPopMatrix();

//menara
glPushMatrix();
glTranslatef(-60,0,60);
glScalef(0.5, 0.5, 0.5);
menara();
glPopMatrix();

//pagar

glPushMatrix();
glTranslatef(-70,0,60);
glScalef(0.5, 0.5, 0.5);
pagar();
glPopMatrix();

//atapatas

glPushMatrix();
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(0.3, 0.1, 0);
glTranslatef(0,30,62);
glScalef(28, 8, 23);
atapdepan();
glPopMatrix();


    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); //disable the color mask
	glDepthMask(GL_FALSE); //disable the depth mask

	glEnable(GL_STENCIL_TEST); //enable the stencil testing

	glStencilFunc(GL_ALWAYS, 1, 0xFFFFFFFF);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE); //set the stencil buffer to replace our next lot of data

	//ground
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); //enable the color mask
	glDepthMask(GL_TRUE); //enable the depth mask

	glStencilFunc(GL_EQUAL, 1, 0xFFFFFFFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); //set the stencil buffer to keep our next lot of data

	glDisable(GL_DEPTH_TEST); //disable depth testing of the reflection

	// glPopMatrix();
	glEnable(GL_DEPTH_TEST); //enable the depth testing
	glDisable(GL_STENCIL_TEST); //disable the stencil testing
	//end of ground
	glEnable(GL_BLEND); //enable alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //set the blending function
	glRotated(1, 0, 0, 0);

	glDisable(GL_BLEND);



    glutSwapBuffers();//buffeer ke memory
	glFlush();//memaksa untuk menampilkan
	rot++;
	angle++;




//glDisable(GL_COLOR_MATERIAL);
}

void init(void){

glEnable(GL_DEPTH_TEST);
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);
glDepthFunc(GL_LESS);
glEnable(GL_NORMALIZE);
glEnable(GL_COLOR_MATERIAL);
glDepthFunc(GL_LEQUAL);
glShadeModel(GL_SMOOTH);
glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
glEnable(GL_CULL_FACE);
glEnable(GL_TEXTURE_2D);
glEnable(GL_TEXTURE_GEN_S);
glEnable(GL_TEXTURE_GEN_T);


    initRendering();
	_terrain = loadTerrain("heightmap02.bmp", 20);

glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


}

void reshape(int w, int h){
glViewport(0, 0 , (GLsizei) w,(GLsizei)h);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();

gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
glMatrixMode(GL_MODELVIEW);

}

static void kibot(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_PAGE_UP:
		viewy++;
		break;
	case GLUT_KEY_PAGE_DOWN:
		viewy--;
		break;
	case GLUT_KEY_UP:
		viewz--;
		break;
	case GLUT_KEY_DOWN:
		viewz++;
		break;

	case GLUT_KEY_RIGHT:
		viewx++;
		break;
	case GLUT_KEY_LEFT:
		viewx--;
		break;

	case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2); //untuk lighting
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	default:
		break;
	}
}



int main(int argc, char** argv){
glutInit(&argc, argv);
glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
glutInitWindowSize(800,600);
glutInitWindowPosition(100,100);
glutCreateWindow("Bisa dibilang Masjid Demak");
init();
glutDisplayFunc(display);
glutIdleFunc(display);
glutReshapeFunc(reshape);

glutSpecialFunc(kibot);

glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
glLightfv(GL_LIGHT0, GL_POSITION, light_position);

glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
glColorMaterial(GL_FRONT, GL_DIFFUSE);

glutMainLoop();
return 0;
}
