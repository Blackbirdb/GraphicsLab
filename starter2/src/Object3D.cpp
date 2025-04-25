#include "Object3D.h"
#include "iostream"

bool Sphere::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER

    // We provide sphere intersection code for you.
    // You should model other intersection implementations after this one.

    // Locate intersection point ( 2 pts )
    const Vector3f &rayOrigin = r.getOrigin(); // Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection();

    Vector3f origin = rayOrigin - _center; // Ray origin in the sphere coordinate

    float a = dir.absSquared();
    float b = 2 * Vector3f::dot(dir, origin);
    float c = origin.absSquared() - _radius * _radius;

    // no intersection
    if (b * b - 4 * a * c < 0)
    {
        return false;
    }

    float d = sqrt(b * b - 4 * a * c);

    float tplus = (-b + d) / (2.0f * a);
    float tminus = (-b - d) / (2.0f * a);

    // the two intersections are at the camera back
    if ((tplus < tmin) && (tminus < tmin))
    {
        return false;
    }

    float t = 10000;
    // the two intersections are at the camera front
    if (tminus > tmin)
    {
        t = tminus;
    }

    // one intersection at the front. one at the back
    if ((tplus > tmin) && (tminus < tmin))
    {
        t = tplus;
    }

    if (t < h.getT())
    {
        Vector3f normal = r.pointAtParameter(t) - _center;
        normal = normal.normalized();
        h.set(t, this->material, normal);
        return true;
    }
    // END STARTER
    return false;
}

// Add object to group
void Group::addObject(Object3D *obj)
{
    m_members.push_back(obj);
}

// Return number of objects in group
int Group::getGroupSize() const
{
    return (int)m_members.size();
}

bool Group::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER
    // we implemented this for you
    bool hit = false;
    for (Object3D *o : m_members)
    {
        if (o->intersect(r, tmin, h))
        {
            hit = true;
        }
    }
    return hit;
    // END STARTER
}

Vector3f Plane::getPointOnPlane() const
{
    if (std::abs(_normal.x()) > 1e-6)
    {
        return Vector3f(_d / _normal.x(), 0.0f, 0.0f);
    }
    else if (std::abs(-_normal.y()) > 1e-6)
    {
        return Vector3f(0.0f, _d / _normal.y(), 0.0f);
    }
    else
    {
        return Vector3f(0.0f, 0.0f, _d / _normal.z());
    }
}

Plane::Plane(const Vector3f &normal, float d, Material *m) : Object3D(m)
{
    _d = d;
    _normal = normal.normalized();
    _p = getPointOnPlane();
    material = m;
}

bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    const Vector3f &rayOrigin = r.getOrigin();
    const Vector3f &rayDir = r.getDirection();

    float denominator = Vector3f::dot(rayDir, _normal);

    if (fabs(denominator) < 1e-6)
    {
        return false;
    }

    float t = Vector3f::dot((_p - rayOrigin), _normal) / denominator;
    if (t < tmin)
    {
        return false;
    }

    if (t < h.getT())
    {
        h.set(t, this->material, _normal);
        return true;
    }

    return false;
}

bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const
{

    Vector3f u = _v[1] - _v[0];
    Vector3f v = _v[2] - _v[0];
    Vector3f n = Vector3f::cross(u, v).normalized();
    float dis = Vector3f::dot(n, _v[0]);

    Hit my_hit;
    my_hit.set(std::numeric_limits<float>::max(), this->material, Vector3f::ZERO);
    Plane plane(n, dis, this->material);

    bool my_judge = plane.intersect(r, tmin, my_hit);
    if (my_judge == false)
    {
        return false;
    }

    Vector3f hit_point = r.pointAtParameter(my_hit.getT());
    Matrix3f triangle = Matrix3f(_v[0], _v[1], _v[2], true);
    bool is_inverse = false;
    Matrix3f triangle_inv = triangle.inverse(&is_inverse);
    if (is_inverse)
    {
        return false;
    }

    Vector3f param = triangle_inv * hit_point;
    if (param.x() < 0.0 || param.y() < 0.0 || param.z() < 0.0)
    {
        return false;
    }

    if (param.x() > 1.0 || param.y() > 1.0 || param.z() > 1.0)
    {
        return false;
    }

    Vector3f final_normal = param.x() * _normals[0] + param.y() * _normals[1] + param.z() * _normals[2];

    if (my_hit.getT() < h.getT())
    {
        h.set(my_hit.getT(), this->material, final_normal);
        return true;
    }

    return false;
}

Transform::Transform(const Matrix4f &m,
                     Object3D *obj) : _object(obj)
{
    _m = m;
}

Matrix4f inverseTranspose(const Matrix4f &m)
{
    Matrix4f inverseMat = m.inverse();
    inverseMat.transpose();

    return inverseMat;
}

bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    Matrix4f inv_m = _m.inverse();
    Vector4f ray_origin_local = inv_m * Vector4f(r.getOrigin(), 1.0f);
    Vector4f ray_dir_local = inv_m * Vector4f(r.getDirection(), 0.0f);
    Ray ray_local = Ray(Vector3f(ray_origin_local.x(), ray_origin_local.y(), ray_origin_local.z()),
                        Vector3f(ray_dir_local.x(), ray_dir_local.y(), ray_dir_local.z()));
    Hit my_hit;
    my_hit.set(10000, this->material, Vector3f::ZERO);
    if (_object->intersect(ray_local, tmin, my_hit) == false)
    {
        return false;
    }

    Vector3f normal_local = my_hit.getNormal();
    Matrix4f inv_transpose_m = inverseTranspose(_m);
    Vector4f normal_world_4f = inv_transpose_m * Vector4f(normal_local, 0.0f);
    Vector3f normal_world = Vector3f(normal_world_4f.x(), normal_world_4f.y(), normal_world_4f.z());
    normal_world = normal_world.normalized();

    if (my_hit.getT() < h.getT())
    {
        h.set(my_hit.getT(), _object->material, normal_world);
        return true;
    }
    return false;
}