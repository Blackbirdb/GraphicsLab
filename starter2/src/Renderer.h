#ifndef RENDERER_H
#define RENDERER_H

#include <string>

#include "SceneParser.h"
#include "ArgParser.h"

class Hit;
class Vector3f;
class Ray;

class Renderer{
public:
	// Instantiates a renderer for the given scene.
	Renderer(const ArgParser& args);
	void Render();
private:
	Vector3f traceRay(const Ray& ray, float tmin, int bounces,
		Hit& hit) const;

	Image ApplyGaussianFilter(const Image& img, int w, int h);

	float clamp(float x, float min, float max);

	void vanillaRendering(int w, int h,
		Image& image, Image& nimage, Image& dimage);
	
	void jitteredRendering(int w, int h,
		Image& image, Image& nimage, Image& dimage);
	
	ArgParser _args;
	SceneParser _scene;
};

#endif // RENDERER_H
