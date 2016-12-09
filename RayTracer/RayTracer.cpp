/*---------------------------------------------------------------------
*
* Copyright © 2015  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "include\GLFW\glfw3.h"

#if defined(WIN32) || defined(_WINDOWS)
#include <Windows.h>
#include <gl/GL.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include "RayTracer.h"
#include "Ray.h"
#include "Scene.h"
#include "Camera.h"
#include "perlin.h"
#include "Vector3d.h"
//assume the monitor is sRGB
#define Gamma 2.2f
#define MAX_DEPTH 5
RayTracer::RayTracer()
{
	m_buffHeight = m_buffWidth = 0;
	m_renderCount = 0;
	SetTraceLevel(5);
	m_traceflag = (TraceFlags)(TRACE_AMBIENT | TRACE_DIFFUSE_AND_SPEC |
		TRACE_SHADOW | TRACE_REFLECTION | TRACE_REFRACTION);

}

RayTracer::RayTracer(int Width, int Height)
{
	m_buffWidth = Width;
	m_buffHeight = Height;
	m_renderCount = 0;
	SetTraceLevel(5);

	m_framebuffer = new Framebuffer(Width, Height);

	//default set default trace flag, i.e. no lighting, non-recursive
	m_traceflag = (TraceFlags)(TRACE_AMBIENT | TRACE_DIFFUSE_AND_SPEC);
}

RayTracer::~RayTracer()
{
	delete m_framebuffer;
}

void RayTracer::RayTrace(Scene* pScene)
{
	Camera* cam = pScene->GetSceneCamera();

	Vector3d camRightVector = cam->GetRightVector();
	Vector3d camUpVector = cam->GetUpVector();
	Vector3d camViewVector = cam->GetViewVector();
	Vector3d centre = cam->GetViewCentre();
	Vector3d camPosition = cam->GetPosition();

	double sceneWidth = pScene->GetSceneWidth();
	double sceneHeight = pScene->GetSceneHeight();

	double pixelDX = sceneWidth / m_buffWidth;
	double pixelDY = sceneHeight / m_buffHeight;

	int total = m_buffHeight*m_buffWidth;
	int done_count = 0;

	Vector3d start;

	start[0] = centre[0] - ((sceneWidth * camRightVector[0])
		+ (sceneHeight * camUpVector[0])) / 2.0;
	start[1] = centre[1] - ((sceneWidth * camRightVector[1])
		+ (sceneHeight * camUpVector[1])) / 2.0;
	start[2] = centre[2] - ((sceneWidth * camRightVector[2])
		+ (sceneHeight * camUpVector[2])) / 2.0;

	Colour scenebg = pScene->GetBackgroundColour();

	if (m_renderCount == 0)
	{
		fprintf(stdout, "Trace start.\n");
		clock_t tstart = clock();
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		Colour colour;
		int current = 0;
#pragma omp parallel for schedule (dynamic, 1) private(colour)		
		for (int i = 0; i < m_buffHeight; i += 1) {
			for (int j = 0; j < m_buffWidth; j += 1) {
				current = i + j;
				colour = Colour(0, 0, 0);
				Vector3d pixel;

				pixel[0] = start[0] + (i + 0.5) * camUpVector[0] * pixelDY
					+ (j + 0.5) * camRightVector[0] * pixelDX;
				pixel[1] = start[1] + (i + 0.5) * camUpVector[1] * pixelDY
					+ (j + 0.5) * camRightVector[1] * pixelDX;
				pixel[2] = start[2] + (i + 0.5) * camUpVector[2] * pixelDY
					+ (j + 0.5) * camRightVector[2] * pixelDX;
				//calculate the metric size of a pixel in the view plane (e.g. framebuffer)
				if (SuperSample == true) {
					float amt = AAFactor;
					float diffrenceamt = 0.3f;
					for (int x = 0; x < amt; x++) {
						for (int y = 0; y < amt; y++) {
							float Xadd = -diffrenceamt*x;
							float Yadd = -diffrenceamt*y;
							if (x > (amt / 2) - 1) {
								//second pass move over 1
								Xadd = diffrenceamt*(x - (amt / 2) - 1);
							}
							if (y > (amt / 2) - 1) {
								//second pass move over 1
								Yadd = diffrenceamt*(y - (amt / 2) - 1);
							}
							pixel[0] = start[0] + (i + Xadd + 0.5) * camUpVector[0] * pixelDY
								+ (j + Yadd + 0.5) * camRightVector[0] * pixelDX;
							pixel[1] = start[1] + (i + Xadd + 0.5) * camUpVector[1] * pixelDY
								+ (j + Yadd + 0.5) * camRightVector[1] * pixelDX;
							pixel[2] = start[2] + (i + Xadd + 0.5) * camUpVector[2] * pixelDY
								+ (j + Yadd + 0.5) * camRightVector[2] * pixelDX;
							/*
							* setup first generation view ray
							* In perspective projection, each view ray originates from the eye (camera) position
							* and pierces through a pixel in the view plane
							*/
							Ray viewray;
							viewray.SetRay(camPosition, (pixel - camPosition).Normalise());

							double u = (double)j / (double)m_buffWidth;
							double v = (double)i / (double)m_buffHeight;

							scenebg = pScene->GetBackgroundColour();
							//trace the scene using the view ray
							//default colour is the background colour, unless something is hit along the way
							colour = colour + this->TraceScene(pScene, viewray, scenebg, m_traceLevel);
						}
					}
					float dvisor = amt*amt;
					colour[0] = colour[0] / dvisor;
					colour[1] = colour[1] / dvisor;
					colour[2] = colour[2] / dvisor;
				}
				else
				{
					//use one ray per pixel.
					pixel[0] = start[0] + (i + 0.5) * camUpVector[0] * pixelDY
						+ (j + 0.5) * camRightVector[0] * pixelDX;
					pixel[1] = start[1] + (i + 0.5) * camUpVector[1] * pixelDY
						+ (j + 0.5) * camRightVector[1] * pixelDX;
					pixel[2] = start[2] + (i + 0.5) * camUpVector[2] * pixelDY
						+ (j + 0.5) * camRightVector[2] * pixelDX;

					/*
					* setup first generation view ray
					* In perspective projection, each view ray originates from the eye (camera) position
					* and pierces through a pixel in the view plane
					*/
					Ray viewray;
					viewray.SetRay(camPosition, (pixel - camPosition).Normalise());

					double u = (double)j / (double)m_buffWidth;
					double v = (double)i / (double)m_buffHeight;

					scenebg = pScene->GetBackgroundColour();

					//default colour is the background colour
					colour = this->TraceScene(pScene, viewray, scenebg, m_traceLevel);
				}
				m_framebuffer->WriteRGBToFramebuffer(colour, j, i);

			}
		}

		printf("Time taken: %.00fms\n", (double)(clock() - tstart) * 1000 / CLOCKS_PER_SEC);
		fprintf(stdout, "Done!!!\n");
		m_renderCount++;
	}
}

//Distance Between two vectors
float Distance(const Vector3& point1, const Vector3& point2)
{
	float distance = sqrt((point1[0] - point2[0]) * (point1[0] - point2[0]) +
		(point1[1] - point2[1]) * (point1[1] - point2[1]) +
		(point1[2] - point2[2]) * (point1[2] - point2[2]));
	return distance;
}
Colour RayTracer::TraceScene(Scene* pScene, Ray& ray, Colour incolour, int tracelevel, bool shadowray)
{
	RayHitResult result;
	tracelevel--;
	Colour outcolour = incolour; //the output colour based on the ray-primitive intersection
	Colour Refraction;
	std::vector<Light*> *light_list = pScene->GetLightList();
	Vector3d cameraPosition = pScene->GetSceneCamera()->GetPosition();
	Vector3d Reflection;
	//Intersect the ray with the scene
	result = pScene->IntersectByRay(ray);

	if (result.data) //the ray has hit something
	{
		outcolour = CalculateLighting(light_list, &cameraPosition, &result);
		Primitive* prim = (Primitive*)result.data;
		Material* mat = prim->GetMaterial();

		if (m_traceflag & TRACE_REFLECTION)
		{
		
			if (mat->GetReflectivity() > 0.0) {
				if (tracelevel >= 1)
				{
					if (tracelevel == 5) {
						//clear the back gound colour
						outcolour = Colour(0, 0, 0);
					}
					Ray reflectRay = Ray();
					Vector3d rray = ray.GetRay().Reflect(result.normal.Normalise());
					Vector3d bias = rray.Normalise() * 0.0001;
					reflectRay.SetRay(result.point + bias, rray);
					outcolour = outcolour + TraceScene(pScene, reflectRay, outcolour, tracelevel) * prim->GetDiffuseColour(result.point);
					Reflection = outcolour;
					if (tracelevel == 1) {
						//on the final level add the specular component for this material
						outcolour = outcolour + CalculateLighting(light_list, &cameraPosition, &result);
					}
				}
			}
			else
			{
				outcolour = CalculateLighting(light_list, &cameraPosition, &result);
			}
		}

		
		if (m_traceflag & TRACE_REFRACTION)
		{
		
			if (mat->GetRefractivity() > 0.0) {
				if (tracelevel >= 1) {
					if (tracelevel == 5) {
						//clear the backgound colour
						outcolour = Colour(0, 0, 0);
						result.CurrentMedium = 1;
					}
					Ray refractRay = Ray();
					Vector3 raray;
					if (result.CurrentMedium == 1.1) {
						raray = ray.GetRay().Refract(result.normal.Normalise(), (mat->GetRefractivity() / 1.0));
						result.CurrentMedium = 1.0;
					}
					else
					{
						raray = ray.GetRay().Refract(result.normal.Normalise(), (result.CurrentMedium / mat->GetRefractivity()));
						result.CurrentMedium = mat->GetRefractivity();
					}
					Vector3	bias = raray * 0.0001;
					refractRay.SetRay(result.point + bias, raray);
					outcolour = outcolour + TraceScene(pScene, refractRay, Colour(0, 0, 0), tracelevel) * mat->GetDiffuseColour();
					if (!(m_traceflag & TRACE_REFLECTION)) {
						if (tracelevel == 1) {
							//apply specular to final colour only if the relective component has not already done so.
							outcolour = outcolour + CalculateLighting(light_list, &cameraPosition, &result);;
						}
					}
				}
			}
		}
		if (m_traceflag & TRACE_SHADOW)
		{
			
			float shadowAccum = 0;
			int count = 4;
			int samplecount = count * count;
			double tlight = 0;//calculate where our light is along our ray.
			double bias = 0.001;//quite a high bias
			for (int i = 0; i < light_list->size(); i++) {
				Ray shadowray = Ray();
				RayHitResult shadowresult;
				Light* clight = (*light_list)[i];
				//we will check the pixels around our centre line
				//then avg the result

				if (Softshadows == true) {
					//this simulates an area light of size limit
					float moveamt = 0.25;
					shadowAccum = 0;
					float shadowamt = 0.75;
					float limit = 1;
					//adjust the light position so that we start offset from centre
					Vector3d adjustlightpos(clight->GetLightPosition()[0] - count * limit / 2.0, clight->GetLightPosition()[1], clight->GetLightPosition()[2] - count * limit / 2.0);
					for (int x = 0; x < count; x++) {
						for (int y = 0; y < count; y++) {
							//get the offset in the T axis
							//double rand = (double)(std::rand() % (100 - 0 + 1)) / 100.0;
							Vector3d lightpos = Vector3d(adjustlightpos[0] + x*moveamt, adjustlightpos[1], adjustlightpos[2] + y*moveamt);
							shadowray.SetRay(result.point + (lightpos - result.point).Normalise() *bias, (lightpos - result.point).Normalise());
							shadowresult = pScene->IntersectByRay(shadowray);
							tlight = Distance(result.point, clight->GetLightPosition());
							if (shadowresult.data && shadowresult.t < tlight) {
								//we hit something
								if (((Primitive*)shadowresult.data)->GetMaterial()->CastShadow() == true) {
									shadowAccum += shadowamt;
								}
							}
						}
					}
					outcolour = outcolour * (1.0 - (shadowAccum / samplecount));
				}
				else
				{
					//render normal shadows
					Vector3d dir = (clight->GetLightPosition() - result.point).Normalise();
					shadowray.SetRay(result.point + dir *bias, dir);
					shadowresult = pScene->IntersectByRay(shadowray);
					//t is intersection distance along ray
					//so:
					tlight = Distance(result.point, clight->GetLightPosition());
					if (shadowresult.data && shadowresult.t < tlight) {
						//we hit something
						//check that the object we hit is not past our light
						if (((Primitive*)shadowresult.data)->GetMaterial()->CastShadow() == true) {
							outcolour = outcolour * 0.25f;
						}
					}
				}
			}
		}
	}

	return outcolour;
}

Colour RayTracer::CalculateLighting(std::vector<Light*>* lights, Vector3d* campos, RayHitResult* hitresult)
{
	Colour outcolour;
	Colour Linearcolsum;
	std::vector<Light*>::iterator lit_iter = lights->begin();
	Primitive* prim = (Primitive*)hitresult->data;
	Material* mat = prim->GetMaterial();
	Colour CurrentDiffuse = mat->GetAmbientColour();

	if (m_traceflag & TRACE_DIFFUSE_AND_SPEC) {
		CurrentDiffuse = prim->GetDiffuseColour(hitresult->point);
		if ((m_traceflag & TRACE_REFLECTION) && mat->GetReflectivity() > 0.0) {
			CurrentDiffuse = Colour(0, 0, 0);
		}
		if ((m_traceflag & TRACE_REFRACTION) && mat->GetRefractivity() > 0.0) {
			CurrentDiffuse = Colour(0, 0, 0);
		}
	}
	bool blinn = false;
	
	if (m_traceflag & TRACE_DIFFUSE_AND_SPEC)
	{
		for (int i = 0; i < lights->size(); i++) {
		
			Light* clight = *(lit_iter + i);
			Vector3 normal;
			Vector3d eyepos = campos->Normalise();
			Vector3d lightpos = clight->GetLightPosition();
			Vector3d pos = hitresult->point;
			Vector3 lightdir = (lightpos - pos).Normalise();
			if (normalmapping && (mat->HasNormalTexture() == true)) {
				//change the normal to the simulted normal
				normal = prim->GetNormalColour(hitresult->point);
			}
			else
			{
				normal = hitresult->normal.Normalise();
			}
			double diffuseamt = max(lightdir.DotProduct(normal), 0.0);
			double specular = 0;
			//bhlim-phong
			if (blinn) {
				Vector3 H = (lightdir + eyepos).Normalise();//half vector
				double ndotH = max(min(normal.DotProduct(H), 1.0), 0.0);
				specular = pow(ndotH, mat->GetSpecPower());
			}
			else
			{
				//phong is slower but produces a much better result in the test case
				Vector3d invlightdir = lightdir*-1.0;
				Vector3d R = invlightdir.Reflect(normal).Normalise();
				specular = pow(max(min(eyepos.DotProduct(R), 1.0), 0.0), mat->GetSpecPower());
			}
			//attenuation
			Colour out = mat->GetAmbientColour()
				+ (CurrentDiffuse * clight->GetLightColour()  * diffuseamt)
				+ (mat->GetSpecularColour() * clight->GetLightColour()  *  specular);
			//add this lights colour to the rest
			Linearcolsum = Linearcolsum + out;
		}
		//apply gamma correction
		outcolour[0] = pow(Linearcolsum[0], 1.0f / Gamma);
		outcolour[1] = pow(Linearcolsum[1], 1.0f / Gamma);
		outcolour[2] = pow(Linearcolsum[2], 1.0f / Gamma);
	}
	else
	{
		outcolour = CurrentDiffuse;
	}

	return outcolour;
}

