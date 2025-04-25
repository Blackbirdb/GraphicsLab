#include "Renderer.h"

#include "ArgParser.h"
#include "Camera.h"
#include "Image.h"
#include "Ray.h"
#include "VecUtils.h"

#include <limits>

#define eps 1e-4f

Renderer::Renderer(const ArgParser &args) : _args(args),
                                            _scene(args.input_file)
{
}

void Renderer::Render()
{
    int w = _args.width;
    int h = _args.height;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);

    // loop through all the pixels in the image
    // generate all the samples

    // This look generates camera rays and callse traceRay.
    // It also write to the color, normal, and depth images.
    // You should understand what this code does.
    Camera *cam = _scene.getCamera();
    for (int y = 0; y < h; ++y)
    {
        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;
        for (int x = 0; x < w; ++x)
        {
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            // Use PerspectiveCamera to generate a ray.
            // You should understand what generateRay() does.
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

            Hit h;
            Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, h);

            image.setPixel(x, y, color);
            nimage.setPixel(x, y, (h.getNormal() + 1.0f) / 2.0f);
            float range = (_args.depth_max - _args.depth_min);
            if (range)
            {
                dimage.setPixel(x, y, Vector3f((h.t - _args.depth_min) / range));
            }
        }
    }
    // END SOLN

    // save the files
    if (_args.output_file.size())
    {
        image.savePNG(_args.output_file);
    }
    if (_args.depth_file.size())
    {
        dimage.savePNG(_args.depth_file);
    }
    if (_args.normals_file.size())
    {
        nimage.savePNG(_args.normals_file);
    }
}

Vector3f
Renderer::traceRay(const Ray &r,    
                   float tmin,
                   int bounces,
                   Hit &h) const
{
    if (_scene.getGroup()->intersect(r, tmin, h))
    {
        Material *material = h.getMaterial();
        Vector3f hitPoint = r.pointAtParameter(h.getT());
        Vector3f normal = h.getNormal().normalized();
        Vector3f finalColor = Vector3f::ZERO;

        finalColor += _scene.getAmbientLight() * material->getDiffuseColor();

        for (int i = 0; i < _scene.getNumLights(); ++i)
        {
            Light *light = _scene.getLight(i);
            Vector3f dirToLight;
            Vector3f lightIntensity;
            float distToLight;

            light->getIllumination(hitPoint, dirToLight, lightIntensity, distToLight);

            // shadow
            Ray shadowRay(hitPoint + eps * dirToLight, dirToLight);
            Hit shadowHit;

            if (_scene.getGroup()->intersect(shadowRay, tmin, shadowHit))
            {
                if (shadowHit.getT() < distToLight)
                {
                    continue;
                }
            }

            finalColor += material->shade(r, h, dirToLight, lightIntensity);
        }

        // Reflection
        if (bounces > 0)
        {
            Vector3f incident = r.getDirection().normalized();
            Vector3f reflectDir = incident - 2 * Vector3f::dot(incident, normal) * normal;
            reflectDir.normalize();

            Ray reflectRay(hitPoint + eps * reflectDir, reflectDir);

            Hit reflectedHit;
            Vector3f reflectedColor = traceRay(reflectRay, tmin, bounces - 1, reflectedHit);

            finalColor += material->getSpecularColor() * reflectedColor;
        }

        return finalColor;
    }
    else
    {
        return _scene.getBackgroundColor(r.getDirection());
    }
}
