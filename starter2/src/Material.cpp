#include "Material.h"
#include <math.h>

float clamp(const Vector3f &L, const Vector3f &N)
{
    if (Vector3f::dot(L, N) <= 0)
    {
        return 0.0f;
    }
    else
    {
        return Vector3f::dot(L, N);
    }
}

Vector3f Material::shade(const Ray &ray,
                         const Hit &hit,
                         const Vector3f &dirToLight,
                         const Vector3f &lightIntensity)
{
    Vector3f L = dirToLight.normalized();
    Vector3f N = hit.getNormal().normalized();

    Vector3f diffuse = clamp(L, N) * lightIntensity * _diffuseColor;

    Vector3f V = -ray.getDirection().normalized();
    Vector3f R = (2.0f * clamp(L, N) * N - L).normalized();
    float specularFactor = powf(clamp(R, V), _shininess);
    Vector3f specular = specularFactor * lightIntensity * _specularColor;

    return diffuse + specular;
}
