#include "surf.h"
#include "vertexrecorder.h"
using namespace std;

namespace
{
    
    // We're only implenting swept surfaces where the profile curve is
    // flat on the xy-plane.  This is a check function.
    static bool checkFlat(const Curve &profile)
    {
        for (unsigned i=0; i<profile.size(); i++)
            if (profile[i].V[2] != 0.0 ||
                profile[i].T[2] != 0.0 ||
                profile[i].N[2] != 0.0)
                return false;
    
        return true;
    }
}

// DEBUG HELPER
Surface quad() { 
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

Surface makeSurfRev(const Curve &profile, unsigned steps)
{
    Surface surface;
    
    if (!checkFlat(profile))
    {
        cerr << "surfRev profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    // Clear any existing data (remove the quad() initialization)
    surface.VV.clear();
    surface.VN.clear();
    surface.VF.clear();

    // For each step, rotate the profile curve around the y-axis
    for (unsigned i = 0; i <= steps; ++i) {
        float t = (float)i / steps;
        float theta = 2.0f * M_PI * t;
        
        // Create rotation matrix around y-axis
        Matrix3f rot;
        rot(0, 0) = cos(theta);  rot(0, 2) = sin(theta);
        rot(1, 1) = 1.0f;
        rot(2, 0) = -sin(theta); rot(2, 2) = cos(theta);
        
        // Rotate each point in the profile curve
        for (const auto& point : profile) {

            // Rotate vertex position
            Vector3f rotatedV = rot * point.V;
            surface.VV.push_back(rotatedV);
            
            // Rotate normal (using the upper 3x3 part of the rotation matrix)
            Vector3f rotatedN = rot * point.N;
            rotatedN = - rotatedN;
            surface.VN.push_back(rotatedN.normalized());
        }
    }

    // Create triangle faces
    unsigned profileSize = profile.size();
    for (unsigned i = 0; i < steps; ++i) {
        for (unsigned j = 0; j < profileSize - 1; ++j) {
            // Current and next profile indices
            unsigned curr = i * profileSize + j;
            unsigned next = ( (i + 1)% (steps + 1) ) * profileSize + j;
            
            // Create two triangles for each quad
            surface.VF.push_back(Tup3u(curr, curr + 1, next));
            surface.VF.push_back(Tup3u(next, curr + 1, next + 1));
        }
    }

    return surface;
}

Matrix4f generate_matrix(CurvePoint point){
    Vector4f tmp_n = Vector4f(point.N, 0.0f);    
    Vector4f tmp_b = Vector4f(point.B, 0.0f);
    Vector4f tmp_t = Vector4f(point.T, 0.0f);
    Vector4f tmp_v = Vector4f(point.V, 1.0f);
    Matrix4f tmp_m = Matrix4f(tmp_n, tmp_b, tmp_t, tmp_v, true);
}

Matrix3f getInverseTransposeManual(const Matrix3f& m) {
    // 提取矩阵元素
    float m00 = m(0,0), m01 = m(0,1), m02 = m(0,2);
    float m10 = m(1,0), m11 = m(1,1), m12 = m(1,2);
    float m20 = m(2,0), m21 = m(2,1), m22 = m(2,2);
    
    // 计算行列式
    float det = m00*(m11*m22 - m12*m21) 
              - m01*(m10*m22 - m12*m20) 
              + m02*(m10*m21 - m11*m20);
    
    if (fabs(det) < 1e-6) {
        // 行列式接近0，矩阵不可逆
        return Matrix3f::identity();
    }
    
    // 计算伴随矩阵(即余子式矩阵的转置)
    Matrix3f adj;
    adj(0,0) =  (m11*m22 - m12*m21);
    adj(0,1) = -(m01*m22 - m02*m21);
    adj(0,2) =  (m01*m12 - m02*m11);
    
    adj(1,0) = -(m10*m22 - m12*m20);
    adj(1,1) =  (m00*m22 - m02*m20);
    adj(1,2) = -(m00*m12 - m02*m10);
    
    adj(2,0) =  (m10*m21 - m11*m20);
    adj(2,1) = -(m00*m21 - m01*m20);
    adj(2,2) =  (m00*m11 - m01*m10);
    
    // 逆矩阵 = 伴随矩阵 / 行列式
    Matrix3f inv = adj * (1 / det);
    
    // 逆转置就是逆矩阵的转置
    return inv.transposed();
}

Matrix4f inverseTranspose(const Matrix4f& m) {
    bool isSingular;
    // 首先计算逆矩阵
    Matrix4f inverseMat = m.inverse(&isSingular);
    
    // 检查矩阵是否奇异（不可逆）
    if (isSingular) {
        // 对于奇异矩阵，可以返回单位矩阵或其他处理方式
        // 这里我们直接返回单位矩阵
        return Matrix4f::identity();
    }
    
    // 然后转置逆矩阵
    inverseMat.transpose();
    
    return inverseMat;
}


//this can run well
Surface makeGen(const Curve &profile, const Curve &sweep)
{
    Surface surface;
    
    if (!checkFlat(profile))
    {
        cerr << "genCyl profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    // Clear the quad() test data
    surface.VV.clear();
    surface.VN.clear();
    surface.VF.clear();

    // For each point in the sweep curve
    for (unsigned i = 0; i < sweep.size(); i++)
    {
        const CurvePoint &sweepPt = sweep[i];
        Vector3f T = sweepPt.T; // Tangent
        Vector3f N = sweepPt.N; // Normal
        Vector3f B = sweepPt.B; // Binormal
        
        // For each point in the profile curve
        for (unsigned j = 0; j < profile.size(); j++)
        {
            const CurvePoint &profilePt = profile[j];
            Vector3f profilePos = profilePt.V;
            
            // Transform profile point to world space using sweep point's frame
            Vector3f worldPos = sweepPt.V + 
                               (profilePos[0] * N) + 
                               (profilePos[1] * B);
            
            // Add vertex position
            surface.VV.push_back(worldPos);
            
            // Compute vertex normal (approximate by transforming profile normal)
            Vector3f profileNormal = profilePt.N;
            Vector3f worldNormal = (profileNormal[0] * N) + 
                                  (profileNormal[1] * B);
            worldNormal = - worldNormal;
            worldNormal.normalize();
            surface.VN.push_back(worldNormal);
            
            // Create faces between this and previous sweep step
            if (i > 0 && j > 0)
            {
                unsigned prevSweep = (i-1) * profile.size();
                unsigned currSweep = i * profile.size();
                
                // Create two triangles for each quad
                surface.VF.push_back(Tup3u(prevSweep + j-1, prevSweep + j, currSweep + j));
                surface.VF.push_back(Tup3u(currSweep + j-1, prevSweep + j-1, currSweep + j));
            }
        }
    }

    return surface;
}

Surface makeGenCyl(const Curve &profile, const Curve &sweep )
{
    Surface surface;
    
    if (!checkFlat(profile))
    {
        cerr << "surfRev profile curve must be flat on xy plane." << endl;
        exit(0);
    }

    vector<Matrix4f> matrices;
    for (unsigned i=0; i<sweep.size(); i++)
    {
        matrices.push_back(generate_matrix(sweep[i]));
    }
    for(unsigned j=0; j<sweep.size(); j++){
        for(unsigned i=0; i<profile.size(); i++)
        {
            // Create rotation matrix around y-axis
            Matrix4f rot = matrices[j];
            
            // Rotate vertex position
            Vector4f rotatedV = rot * Vector4f(profile[i].V, 1.0f);
            surface.VV.push_back(Vector3f(rotatedV[0], rotatedV[1], rotatedV[2]));
            
            // Rotate normal (using the upper 3x3 part of the rotation matrix)
            Matrix4f tmp = inverseTranspose(rot);
            Vector4f res = tmp * Vector4f(profile[i].N, 1.0f);
            surface.VN.push_back(Vector3f(res[0], res[1], res[2]));
        }  
    }

    // Create triangle faces
    unsigned profileSize = profile.size();
    unsigned sweep_size = sweep.size();        
    for (unsigned j = 0; j < sweep_size; ++j) {
        for (unsigned i = 0; i < profileSize - 1; ++i) {
            // Current and next profile indices
            unsigned curr = j *profileSize + i;
            unsigned next = ( (j + 1) % sweep_size) * profileSize + i;
            
            // Create two triangles for each quad
            surface.VF.push_back(Tup3u(curr, curr + 1, next));
            surface.VF.push_back(Tup3u(next, curr + 1, next + 1));
        }
    }
    return surface;
}

void recordSurface(const Surface &surface, VertexRecorder* recorder) {
	const Vector3f WIRECOLOR(0.4f, 0.4f, 0.4f);
    for (int i=0; i<(int)surface.VF.size(); i++)
    {
		recorder->record(surface.VV[surface.VF[i][0]], surface.VN[surface.VF[i][0]], WIRECOLOR);
		recorder->record(surface.VV[surface.VF[i][1]], surface.VN[surface.VF[i][1]], WIRECOLOR);
		recorder->record(surface.VV[surface.VF[i][2]], surface.VN[surface.VF[i][2]], WIRECOLOR);
    }
}

void recordNormals(const Surface &surface, VertexRecorder* recorder, float len)
{
	const Vector3f NORMALCOLOR(0, 1, 1);
    for (int i=0; i<(int)surface.VV.size(); i++)
    {
		recorder->record_poscolor(surface.VV[i], NORMALCOLOR);
		recorder->record_poscolor(surface.VV[i] + surface.VN[i] * len, NORMALCOLOR);
    }
}

void outputObjFile(ostream &out, const Surface &surface)
{
    
    for (int i=0; i<(int)surface.VV.size(); i++)
        out << "v  "
            << surface.VV[i][0] << " "
            << surface.VV[i][1] << " "
            << surface.VV[i][2] << endl;

    for (int i=0; i<(int)surface.VN.size(); i++)
        out << "vn "
            << surface.VN[i][0] << " "
            << surface.VN[i][1] << " "
            << surface.VN[i][2] << endl;

    out << "vt  0 0 0" << endl;
    
    for (int i=0; i<(int)surface.VF.size(); i++)
    {
        out << "f  ";
        for (unsigned j=0; j<3; j++)
        {
            unsigned a = surface.VF[i][j]+1;
            out << a << "/" << "1" << "/" << a << " ";
        }
        out << endl;
    }
}
