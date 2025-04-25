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

/**
 * Expected behavior for super-sampling:
 * 1. -jitter=true, -filter=false: Jittered super-sampling, no filtering, return image with resolution (3 * w, 3 * h)
 * 2. -jitter=false, -filter=true: Regular super-sampling with Gaussian filter
 * 3. -jitter=true, -filter=true: Jittered super-sampling with Gaussian filter
 */

void Renderer::Render()
{
    int w = _args.width;
    int h = _args.height;

    Image image(w, h);
    Image nimage(w, h);
    Image dimage(w, h);

    if (!_args.jitter && !_args.filter){
        // no super-sampling
        image = Image(w, h);
        nimage = Image(w, h);
        dimage = Image(w, h);

        vanillaRendering(w, h, image, nimage, dimage);
    }

    else{
        // super-sampling
        int super_w = w * 3;
        int super_h = h * 3;

        Image superImage(super_w, super_h);
        Image superNImage(super_w, super_h);
        Image superDImage(super_w, super_h);

        // jittering
        if (_args.jitter){
            jitteredRendering(super_w, super_h,
                superImage, superNImage, superDImage);
        }
        else{
            vanillaRendering(super_w, super_h,
                superImage, superNImage, superDImage);
        }
        // filtering
        if (_args.filter){
            image = ApplyGaussianFilter(superImage, w, h);
            nimage = ApplyGaussianFilter(superNImage, w, h);
            dimage = ApplyGaussianFilter(superDImage, w, h);
        }
        else{
            image = superImage;
            nimage = superNImage;
            dimage = superDImage;
        }
    }

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
            if (_args.shadows){
                Ray shadowRay(hitPoint + eps * dirToLight, dirToLight);
                Hit shadowHit;

                if (_scene.getGroup()->intersect(shadowRay, tmin, shadowHit)){
                    if (shadowHit.getT() < distToLight){
                        continue;
                    }
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
        h.t = 0;
        return _scene.getBackgroundColor(r.getDirection());
    }
}

/**
 * Applies Gaussian filter to a super sampled image.
 * Returns downsampled image with resolution (w, h).
 */
Image Renderer::ApplyGaussianFilter(const Image& img, int w, int h){
    Image result(w, h);
    float kernel[3][3] = { {1,2,1}, {2,4,2}, {1,2,1} };
    float kernelSum = 16.0f;

    for (int y = 0; y < h; ++y){
        for (int x = 0; x < w; ++x){
            Vector3f blurred = Vector3f::ZERO;

            for (int dy = -1; dy <= 1; ++dy){
                for (int dx = -1; dx <= 1; ++dx){
                    int sx = clamp(x * 3 + dx, 0, img.getWidth() - 1);
                    int sy = clamp(y * 3 + dy, 0, img.getHeight() - 1);

                    blurred += img.getPixel(sx, sy) * kernel[dy + 1][dx + 1];
                }
            }

            result.setPixel(x, y, blurred / kernelSum);
        }
    }

    return result;
}

float Renderer::clamp (float x, float min, float max){
    if (x < min) return min;
    if (x > max) return max;
    return x;
}


/**
 * Vanilla rendering. Renders image, nimage and dimage by modifying the
 * passed references to images.
 */
void Renderer::vanillaRendering(int w, int h,
                                Image& image, Image& nimage, Image& dimage){

    Camera* cam = _scene.getCamera();

    for (int y = 0; y < h; ++y){

        float ndcy = 2 * (y / (h - 1.0f)) - 1.0f;

        for (int x = 0; x < w; ++x){
            
            float ndcx = 2 * (x / (w - 1.0f)) - 1.0f;
            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

            Hit h;
            Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, h);

            image.setPixel(x, y, color);
            nimage.setPixel(x, y, (h.getNormal() + 1.0f) / 2.0f);
            float range = (_args.depth_max - _args.depth_min);
            if (range){
                dimage.setPixel(x, y, Vector3f((h.t - _args.depth_min) / range));
            }
        }
    }
}

/**
 * Jittered rendering. Assumes w, h are the super-sampled size of the images.
 */
void Renderer::jitteredRendering(int w, int h,
                                 Image& image, Image& nimage, Image& dimage){
    
    Camera* cam = _scene.getCamera();

    for (int y = 0; y < w; ++y){
        for (int x = 0; x < h; ++x){
            
            float jitter_x = (x + drand48()) / (w - 1.0f); 
            float jitter_y = (y + drand48()) / (h - 1.0f);

            float ndcx = 2.0f * jitter_x - 1.0f;
            float ndcy = 2.0f * jitter_y - 1.0f;

            Ray r = cam->generateRay(Vector2f(ndcx, ndcy));

            Hit hit;
            Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, hit);

            image.setPixel(x, y, color);
            nimage.setPixel(x, y, (hit.getNormal() + 1.0f) / 2.0f);

            float range = _args.depth_max - _args.depth_min;
            if (range > 0){
                dimage.setPixel(x, y, Vector3f((hit.t - _args.depth_min) / range));
            }
        }
    }
}
