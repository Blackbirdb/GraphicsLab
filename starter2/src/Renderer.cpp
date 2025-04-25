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

    if (!_args.jitter && !_args.filter){
        // no super-sampling
        vanillaSampling(w, h, image, nimage, dimage);
    }

    else if (_args.filter){
        // filter
        int super_w = w * 3;
        int super_h = h * 3;

        Image superImage(super_w, super_h);
        Image superNImage(super_w, super_h);
        Image superDImage(super_w, super_h);

        // filter + jitter
        if (_args.jitter){
            jitteredSampling(super_w, super_h,
                superImage, superNImage, superDImage);
        }
        else{
            vanillaSampling(super_w, super_h,
                superImage, superNImage, superDImage);
        }
        
        image = ApplyGaussianFilter(superImage, w, h);
        nimage = ApplyGaussianFilter(superNImage, w, h);
        dimage = ApplyGaussianFilter(superDImage, w, h);
    }
    // jitter
    else if (_args.jitter){
        jitteredSampling(w, h, image, nimage, dimage);
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
            Vector3f reflectDir = (incident + 2 * Vector3f::dot(incident, normal) * normal).normalized();

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

/**
 * Applies Gaussian filter to a super sampled image.
 * Assumes img is a super-sampled image.
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


void Renderer::vanillaSampling(int w, int h,
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
 * Jittered sampling. Samples 16 rays per pixel, each with a random offset.
 */
void Renderer::jitteredSampling(int w, int h,
                                 Image& image, Image& nimage, Image& dimage){
    
    Camera* cam = _scene.getCamera();
    int samples = 16;

    for (int y = 0; y < w; ++y){
        for (int x = 0; x < h; ++x){

            Vector3f color_sum = Vector3f::ZERO;
            Vector3f normal_sum = Vector3f::ZERO;
            Vector3f depth_sum = Vector3f::ZERO;

            for (int s = 0; s < samples; ++s){
                
                float jitter_x = (rand() / (float) RAND_MAX) - 0.5f; 
                float jitter_y = (rand() / (float) RAND_MAX) - 0.5f; 

                float ndcx = 2 * ((x + 0.5f + jitter_x) / w) - 1.0f;
                float ndcy = 2 * ((y + 0.5f + jitter_y) / h) - 1.0f;

                Ray r = cam->generateRay(Vector2f(ndcx, ndcy));
                Hit hit;
                Vector3f color = traceRay(r, cam->getTMin(), _args.bounces, hit);

                color_sum += color;
                normal_sum += (hit.getNormal() + 1.0f) / 2.0f;

                float range = (_args.depth_max - _args.depth_min);
                if (range){
                    depth_sum += Vector3f((hit.t - _args.depth_min) / range);
                }
            }

            image.setPixel(x, y, color_sum / samples);
            nimage.setPixel(x, y, normal_sum / samples);

            float range = _args.depth_max - _args.depth_min;
            if (range > 0){
                dimage.setPixel(x, y, depth_sum);
            }
        }
    }
}
