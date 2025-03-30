#include "curve.h"
#include "vertexrecorder.h"
using namespace std;

const float c_pi = 3.14159265358979323846f;

// B-spline basis
const Matrix4f M_bspline(1.0/6, -1.0/2, 1.0/2, -1.0/6,
						 2.0/3, 0, -1, 1.0/2,
						 1.0/6, 1.0/2, 1.0/2, -1.0/2,
						 0, 0, 0, 1.0/6);

// Bezier basis
const Matrix4f M_bez(1, -3, 3, -1,
					 0, 3, -6, 3,
					 0, 0, 3, -3,
					 0, 0, 0, 1);

namespace
{
// Approximately equal to.  We don't want to use == because of
// precision issues with floating point.
inline bool approx(const Vector3f& lhs, const Vector3f& rhs)
{
	const float eps = 1e-8f;
	return (lhs - rhs).absSquared() < eps;
}


}

/**
 * P: vector of control points
 * steps: number of points on each piece of the spline
*/
Curve evalBezier(const vector< Vector3f >& P, unsigned steps)
{
	// Check
	if (P.size() < 4 || P.size() % 3 != 1)
	{
		cerr << "evalBezier must be called with 3n+1 control points." << endl;
		exit(0);
	}

	// TODO:
	// You should implement this function so that it returns a Curve
	// (e.g., a vector< CurvePoint >).  The variable "steps" tells you
	// the number of points to generate on each piece of the spline.
	// At least, that's how the sample solution is implemented and how
	// the SWP files are written.  But you are free to interpret this
	// variable however you want, so long as you can control the
	// "resolution" of the discretized spline curve with it.

	// Make sure that this function computes all the appropriate
	// Vector3fs for each CurvePoint: V,T,N,B.
	// [NBT] should be unit and orthogonal.

	// Also note that you may assume that all Bezier curves that you
	// receive have G1 continuity.  Otherwise, the TNB will not be
	// be defined at points where this does not hold.

	cerr << "\t>>> evalBezier has been called with the following input:" << endl;

	cerr << "\t>>> Control points (type vector< Vector3f >): " << endl;
	for (int i = 0; i < (int)P.size(); ++i)
	{
		cerr << "\t>>> " << P[i] << endl;
	}

	cerr << "\t>>> Steps (type steps): " << steps << endl;

	int numPieces = (P.size() - 1) / 3;
	Curve Bezier;

	// for each piece of the curve, we compute the curve points
	for (int i = 0; i < numPieces; ++i){
		// the control points for this piece
		// we don't have 4*3 matrix in vecmath so we can only use seperate 
		// vectors; I'll implement it if we have time. 
		Vector3f p0 = P[i * 3];
		Vector3f p1 = P[i * 3 + 1];
		Vector3f p2 = P[i * 3 + 2];
		Vector3f p3 = P[i * 3 + 3];

		// P = GMT
		for (int q = 0; q < steps; ++q){
			if (i != 0 && q == 0){
				// if not the first piece, the first point is shared
				continue;
			}
			float t = (float) q / steps;
			// T for the point itself
			Vector4f T_normal(1, t, t * t, t * t * t);
			Vector4f MT_normal = M_bez * T_normal;	// 4*1 vector

			// T for the tangent
			Vector4f T_tangent(0, 1, 2 * t, 3 * t * t);
			Vector4f MT_tangent = M_bez * T_tangent;
			// compute the curve point
			CurvePoint cp;
			for (int j = 0; j < 3; ++j){
				Vector4f G_j(p0[j], p1[j], p2[j], p3[j]);
				// the j-th component of the curve point
				float V_j = Vector4f::dot(G_j, MT_normal);
				cp.V[j] = V_j;

				// the j-th component of the tangent
				float T_j = Vector4f::dot(G_j, MT_tangent);
				cp.T[j] = T_j;
			}

			cp.T.normalize();

			// next, we compute the N and B vectors
			Vector3f B_0 = Vector3f::cross(Vector3f(0, 0, 1), cp.T).normalized();
			// if t==0, use B_0 to compute N; else use B_t-1
			if (i == 0 && q == 0){
				cp.N = Vector3f::cross(B_0, cp.T).normalized();
			}
			else{
				cp.N = Vector3f::cross(Bezier.back().B, cp.T).normalized();
			}
			cp.B = Vector3f::cross(cp.T, cp.N).normalized();

			// finished generating the curve point, add to curve
			Bezier.push_back(cp);
		}
	}

	return Bezier;

}

Curve evalBspline(const vector< Vector3f >& P, unsigned steps)
{
	// Check
	if (P.size() < 4)
	{
		cerr << "evalBspline must be called with 4 or more control points." << endl;
		exit(0);
	}

	// TODO:
	// It is suggested that you implement this function by changing
	// basis from B-spline to Bezier.  That way, you can just call
	// your evalBezier function.

	cerr << "\t>>> evalBSpline has been called with the following input:" << endl;

	cerr << "\t>>> Control points (type vector< Vector3f >): " << endl;
	for (int i = 0; i < (int)P.size(); ++i)
	{
		cerr << "\t>>> " << P[i] << endl;
	}

	cerr << "\t>>> Steps (type steps): " << steps << endl;
	cerr << "\t>>> Returning empty curve." << endl;

	// Return an empty curve right now.
	// return Curve();

	Matrix4f M_bez_inv = M_bez.inverse();
	Matrix4f new_M = M_bspline * M_bez_inv;

	cout << "Transformation Matrix (new_M): " << endl;

	new_M.print();

	int numPieces = P.size() - 3;

	Curve Bspline;

	int cp_cnt = 0;

	for (int i = 0; i < numPieces; ++i){

		Matrix4f G_bspline (P[i][0], P[i + 1][0], P[i + 2][0], P[i + 3][0],
							P[i][1], P[i + 1][1], P[i + 2][1], P[i + 3][1],
							P[i][2], P[i + 1][2], P[i + 2][2], P[i + 3][2],
							0, 0, 0, 0);
		Matrix4f G_bez_matrix = G_bspline * new_M;
			// control points in first 3 rows

		vector<Vector3f> G_bezier;

		for (int j = 0; j < 4; ++j){
			cp_cnt++;
			printf("add a control point at i = %d, j = %d, total: %d\n", i, j, cp_cnt);
			Vector3f G_bez = G_bez_matrix.getCol(j).xyz();
			G_bez.print();
			G_bezier.push_back(G_bez);
		}

		Curve Bezier = evalBezier(G_bezier, steps);
		Bspline.insert(Bspline.end(), Bezier.begin(), Bezier.end());
	}

	return Bspline;
}

Curve evalCircle(float radius, unsigned steps)
{
	// This is a sample function on how to properly initialize a Curve
	// (which is a vector< CurvePoint >).

	// Preallocate a curve with steps+1 CurvePoints
	Curve R(steps + 1);

	// Fill it in counterclockwise
	for (unsigned i = 0; i <= steps; ++i)
	{
		// step from 0 to 2pi
		float t = 2.0f * c_pi * float(i) / steps;

		// Initialize position
		// We're pivoting counterclockwise around the y-axis
		R[i].V = radius * Vector3f(cos(t), sin(t), 0);

		// Tangent vector is first derivative
		R[i].T = Vector3f(-sin(t), cos(t), 0);

		// Normal vector is second derivative
		R[i].N = Vector3f(-cos(t), -sin(t), 0);

		// Finally, binormal is facing up.
		R[i].B = Vector3f(0, 0, 1);
	}

	return R;
}

void recordCurve(const Curve& curve, VertexRecorder* recorder)
{
	const Vector3f WHITE(1, 1, 1);
	for (int i = 0; i < (int)curve.size() - 1; ++i)
	{
		recorder->record_poscolor(curve[i].V, WHITE);
		recorder->record_poscolor(curve[i + 1].V, WHITE);
	}
}
void recordCurveFrames(const Curve& curve, VertexRecorder* recorder, float framesize)
{
	Matrix4f T;
	const Vector3f RED(1, 0, 0);
	const Vector3f GREEN(0, 1, 0);
	const Vector3f BLUE(0, 0, 1);
	
	const Vector4f ORGN(0, 0, 0, 1);
	const Vector4f AXISX(framesize, 0, 0, 1);
	const Vector4f AXISY(0, framesize, 0, 1);
	const Vector4f AXISZ(0, 0, framesize, 1);

	for (int i = 0; i < (int)curve.size(); ++i)
	{
		T.setCol(0, Vector4f(curve[i].N, 0));
		T.setCol(1, Vector4f(curve[i].B, 0));
		T.setCol(2, Vector4f(curve[i].T, 0));
		T.setCol(3, Vector4f(curve[i].V, 1));
 
		// Transform orthogonal frames into model space
		Vector4f MORGN  = T * ORGN;
		Vector4f MAXISX = T * AXISX;
		Vector4f MAXISY = T * AXISY;
		Vector4f MAXISZ = T * AXISZ;

		// Record in model space
		recorder->record_poscolor(MORGN.xyz(), RED);
		recorder->record_poscolor(MAXISX.xyz(), RED);

		recorder->record_poscolor(MORGN.xyz(), GREEN);
		recorder->record_poscolor(MAXISY.xyz(), GREEN);

		recorder->record_poscolor(MORGN.xyz(), BLUE);
		recorder->record_poscolor(MAXISZ.xyz(), BLUE);
	}
}

