#include "surf.h"
#include "vertexrecorder.h"
using namespace std;

namespace{
    // We're only implenting swept surfaces where the profile curve is
    // flat on the xy-plane.  This is a check function.
    static bool checkFlat(const Curve& profile){
        for (unsigned i = 0; i < profile.size(); i++)
            if (profile[i].V[2] != 0.0 ||
                profile[i].T[2] != 0.0 ||
                profile[i].N[2] != 0.0)
                return false;

        return true;
    }
}

// DEBUG HELPER
Surface quad(){
    Surface ret;
    ret.VV.push_back(Vector3f(-1, -1, 0));
    ret.VV.push_back(Vector3f(+1, -1, 0));
    ret.VV.push_back(Vector3f(+1, +1, 0));
    ret.VV.push_back(Vector3f(-1, +1, 0));

    ret.VN.push_back(Vector3f(0, 0, 1));
    ret.VN.push_back(Vector3f(0, 0, 1));
    ret.VN.push_back(Vector3f(0, 0, 1));
    ret.VN.push_back(Vector3f(0, 0, 1));

    ret.VF.push_back(Tup3u(0, 1, 2));
    ret.VF.push_back(Tup3u(0, 2, 3));
    return ret;
}

void generate_triangles(unsigned profileSize, unsigned steps, Surface& surface){
    std::vector< Tup3u > triangles;

    for (unsigned j = 0; j < steps; ++j){
        for (unsigned i = 0; i < profileSize - 1; ++i){
            // Current and next profile indices
            unsigned curr = j * profileSize + i;
            unsigned next = ((j + 1) % (steps)) * profileSize + i;

            // Create two triangles for each quad
            surface.VF.push_back(Tup3u(curr, curr + 1, next));
            surface.VF.push_back(Tup3u(next, curr + 1, next + 1));
        }
    }
}

Surface makeSurfRev(const Curve& profile, unsigned steps){
    Surface surface;

    if (!checkFlat(profile)){
        cerr << "surfRev profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    // Clear any existing data (remove the quad() initialization)
    surface.VV.clear();
    surface.VN.clear();
    surface.VF.clear();

    // For each step, rotate the profile curve around the y-axis
    for (unsigned i = 0; i <= steps; ++i){
        float t = (float) i / steps;
        float theta = 2.0f * M_PI * t;

        // Create rotation matrix around y-axis
        Matrix3f rot;
        rot(0, 0) = cos(theta);  rot(0, 2) = sin(theta);
        rot(1, 1) = 1.0f;
        rot(2, 0) = -sin(theta); rot(2, 2) = cos(theta);

        // Rotate each point in the profile curve
        for (const auto& point : profile){
            // Rotate vertex position
            Vector3f rotatedV = rot * point.V;
            surface.VV.push_back(rotatedV);

            // Rotate normal (using the upper 3x3 part of the rotation matrix)
            Vector3f rotatedN = rot * point.N;
            rotatedN = -rotatedN;
            surface.VN.push_back(rotatedN.normalized());
        }
    }

    // Create triangle faces
    unsigned profileSize = profile.size();
    generate_triangles(profileSize, steps, surface);

    return surface;
}

Matrix4f generate_M(CurvePoint point){
    Vector4f tmp_n = Vector4f(point.N, 0.0f);
    Vector4f tmp_b = Vector4f(point.B, 0.0f);
    Vector4f tmp_t = Vector4f(point.T, 0.0f);
    Vector4f tmp_v = Vector4f(point.V, 1.0f);
    Matrix4f tmp_m = Matrix4f(tmp_n, tmp_b, tmp_t, tmp_v, true);

    return tmp_m;
}


Matrix4f inverseTranspose(const Matrix4f& m){
    bool isSingular;
    
    Matrix4f inverseMat = m.inverse(&isSingular);
    inverseMat.transpose();

    return inverseMat;
}


Surface makeGenCyl(const Curve& profile, const Curve& sweep){
    Surface surface;

    if (!checkFlat(profile)){
        cerr << "surfRev profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    for (unsigned j = 0; j < sweep.size(); j++){
        for (unsigned i = 0; i < profile.size(); i++){
            // Create rotation matrix around y-axis
            Matrix4f rot = generate_M(sweep[j]);

            // Rotate vertex position
            Vector4f rotatedV = rot * Vector4f(profile[i].V, 1.0f);
            surface.VV.push_back(rotatedV.xyz());

            // Rotate normal (using the upper 3x3 part of the rotation matrix)
            Matrix4f tmp = inverseTranspose(rot);
            Vector4f res = tmp * Vector4f(profile[i].N, 1.0f);
            surface.VN.push_back(-res.xyz());
        }
    }

    // Create triangle faces
    unsigned profileSize = profile.size();
    unsigned sweep_size = sweep.size();
    generate_triangles(profileSize, sweep_size, surface);

    return surface;
}

void recordSurface(const Surface& surface, VertexRecorder* recorder){
    const Vector3f WIRECOLOR(0.4f, 0.4f, 0.4f);
    for (int i = 0; i < (int) surface.VF.size(); i++){
        recorder->record(surface.VV[surface.VF[i][0]], surface.VN[surface.VF[i][0]], WIRECOLOR);
        recorder->record(surface.VV[surface.VF[i][1]], surface.VN[surface.VF[i][1]], WIRECOLOR);
        recorder->record(surface.VV[surface.VF[i][2]], surface.VN[surface.VF[i][2]], WIRECOLOR);
    }
}

void recordNormals(const Surface& surface, VertexRecorder* recorder, float len){
    const Vector3f NORMALCOLOR(0, 1, 1);
    for (int i = 0; i < (int) surface.VV.size(); i++){
        recorder->record_poscolor(surface.VV[i], NORMALCOLOR);
        recorder->record_poscolor(surface.VV[i] + surface.VN[i] * len, NORMALCOLOR);
    }
}

void outputObjFile(ostream& out, const Surface& surface){

    for (int i = 0; i < (int) surface.VV.size(); i++)
        out << "v  "
        << surface.VV[i][0] << " "
        << surface.VV[i][1] << " "
        << surface.VV[i][2] << endl;

    for (int i = 0; i < (int) surface.VN.size(); i++)
        out << "vn "
        << surface.VN[i][0] << " "
        << surface.VN[i][1] << " "
        << surface.VN[i][2] << endl;

    out << "vt  0 0 0" << endl;

    for (int i = 0; i < (int) surface.VF.size(); i++){
        out << "f  ";
        for (unsigned j = 0; j < 3; j++){
            unsigned a = surface.VF[i][j] + 1;
            out << a << "/" << "1" << "/" << a << " ";
        }
        out << endl;
    }
}
