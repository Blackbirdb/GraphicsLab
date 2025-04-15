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
	////////////////////// start of debug information ////////////////////////
	// Check
	if (P.size() < 4 || P.size() % 3 != 1)
	{
		cerr << "evalBezier must be called with 3n+1 control points." << endl;
		exit(0);
	}

	cerr << "\t>>> evalBezier has been called with the following input:" << endl;

	cerr << "\t>>> Control points (type vector< Vector3f >): " << endl;
	for (int i = 0; i < (int)P.size(); ++i)
	{
		cerr << "\t>>> " << P[i] << endl;
	}

	cerr << "\t>>> Steps (type steps): " << steps << endl;

	////////////////////// end of debug information ////////////////////////

	int numPieces = (P.size() - 1) / 3;
	Curve Bezier;

	// for each piece of the curve, we compute the curve points
	for (int i = 0; i < numPieces; ++i){

		Matrix4f G_bezier(
			P[i * 3][0], P[i * 3 + 1][0], P[i * 3 + 2][0], P[i * 3 + 3][0],
			P[i * 3][1], P[i * 3 + 1][1], P[i * 3 + 2][1], P[i * 3 + 3][1],
			P[i * 3][2], P[i * 3 + 1][2], P[i * 3 + 2][2], P[i * 3 + 3][2],
			0, 0, 0, 0);

		// P = GMT
		for (unsigned q = 0; q <= steps; ++q){

			if (i != 0 && q == 0){
				// if not the first piece, the first point is shared
				continue;
			}

			float t = (float) q / steps;

			// T for the point position
			Vector4f T_normal(1, t, t * t, t * t * t);
			// T for the tangent
			Vector4f T_tangent(0, 1, 2 * t, 3 * t * t);

			// compute the curve point and tangent
			CurvePoint cp;
			cp.V = (G_bezier * M_bez * T_normal).xyz();
			cp.T = (G_bezier * M_bez * T_tangent).xyz().normalized();

			// next, we compute the N and B vectors
			Vector3f B_0 = Vector3f(0, 0, 1);
			
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
	////////////////////// start of debug information ////////////////////////
	// Check
	if (P.size() < 4)
	{
		cerr << "evalBspline must be called with 4 or more control points." << endl;
		exit(0);
	}

	cerr << "\t>>> evalBSpline has been called with the following input:" << endl;

	cerr << "\t>>> Control points (type vector< Vector3f >): " << endl;
	for (int i = 0; i < (int)P.size(); ++i)
	{
		cerr << "\t>>> " << P[i] << endl;
	}

	cerr << "\t>>> Steps (type steps): " << steps << endl;
	cerr << "\t>>> Returning empty curve." << endl;

	////////////////////// end of debug information ////////////////////////

	Matrix4f M_bez_inv = M_bez.inverse();
	Matrix4f new_M = M_bspline * M_bez_inv;

	int numPieces = P.size() - 3;

	Curve Bspline;

	for (int i = 0; i < numPieces; ++i){

		Matrix4f G_bspline (P[i][0], P[i + 1][0], P[i + 2][0], P[i + 3][0],
							P[i][1], P[i + 1][1], P[i + 2][1], P[i + 3][1],
							P[i][2], P[i + 1][2], P[i + 2][2], P[i + 3][2],
							0, 0, 0, 0);
		Matrix4f G_bez_matrix = G_bspline * new_M;
			// control points in first 3 rows

		vector<Vector3f> G_bezier;

		for (int j = 0; j < 4; ++j){
			Vector3f G_bez = G_bez_matrix.getCol(j).xyz();
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

