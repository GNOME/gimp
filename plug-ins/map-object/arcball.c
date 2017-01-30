/************************************/
/* ArcBall.c (c) Ken Shoemake, 1993 */
/* Modified by Tom Bech, 1996       */
/************************************/

#include "config.h"

#include <libgimp/gimp.h>

#include "arcball.h"

/* Global variables */
/* ================ */

Quat qOne = { 0, 0, 0, 1 };

static HVect         center;
static double        radius;
static Quat          qNow, qDown, qDrag;
static HVect         vNow, vDown, vFrom, vTo, vrFrom, vrTo;
static HMatrix       mNow, mDown;
static unsigned int  showResult, dragging;
static ConstraintSet sets[NSets];
static int           setSizes[NSets];
static AxisSet       axisSet;
static int           axisIndex;

static HMatrix mId =
{
  { 1, 0, 0, 0 },
  { 0, 1, 0, 0 },
  { 0, 0, 1, 0 },
  { 0, 0, 0, 1 }
};

static double otherAxis[][4] =
{
  {-0.48, 0.80, 0.36, 1}
};

/* Internal methods */
/* ================ */

static void     Qt_ToMatrix(Quat q,HMatrix out);
static Quat     Qt_Conj(Quat q);
static Quat     Qt_Mul(Quat qL, Quat qR);
static Quat     Qt_FromBallPoints(HVect from, HVect to);
static void     Qt_ToBallPoints(Quat q, HVect *arcFrom, HVect *arcTo);

static HVect    V3_(double x, double y, double z);
static double   V3_Norm(HVect v);
static HVect    V3_Unit(HVect v);
static HVect    V3_Scale(HVect v, double s);
static HVect    V3_Negate(HVect v);
/*
static HVect    V3_Add(HVect v1, HVect v2);
*/
static HVect    V3_Sub(HVect v1, HVect v2);
static double   V3_Dot(HVect v1, HVect v2);
/*
static HVect    V3_Cross(HVect v1, HVect v2);
static HVect    V3_Bisect(HVect v0, HVect v1);
*/

static HVect    MouseOnSphere(HVect mouse, HVect ballCenter, double ballRadius);
static HVect    ConstrainToAxis(HVect loose, HVect axis);
static int      NearestConstraintAxis(HVect loose, HVect *axes, int nAxes);

/* Establish reasonable initial values for controller. */
/* =================================================== */

void
ArcBall_Init (void)
{
  int i;

  center = qOne;
  radius = 1.0;
  vDown = vNow = qOne;
  qDown = qNow = qOne;
  for (i=15; i>=0; i--)
    ((double *)mNow)[i] = ((double *)mDown)[i] = ((double *)mId)[i];

  showResult = dragging = FALSE;
  axisSet = NoAxes;
  sets[CameraAxes] = mId[X];
  setSizes[CameraAxes] = 3;
  sets[BodyAxes] = mDown[X];
  setSizes[BodyAxes] = 3;
  sets[OtherAxes] = otherAxis[X];
  setSizes[OtherAxes] = 1;
}

/* Set the center and size of the controller. */
/* ========================================== */

void
ArcBall_Place (HVect  Center,
               double Radius)
{
  center = Center;
  radius = Radius;
}

/* Incorporate new mouse position. */
/* =============================== */

void
ArcBall_Mouse (HVect v_Now)
{
  vNow = v_Now;
}

/* Choose a constraint set, or none. */
/* ================================= */

void
ArcBall_UseSet (AxisSet axis_Set)
{
  if (!dragging) axisSet = axis_Set;
}

/* Using vDown, vNow, dragging, and axisSet, compute rotation etc. */
/* =============================================================== */

void
ArcBall_Update (void)
{
  int setSize = setSizes[axisSet];
  HVect *set = (HVect *)(sets[axisSet]);

  vFrom = MouseOnSphere(vDown, center, radius);
  vTo = MouseOnSphere(vNow, center, radius);
  if (dragging)
    {
      if (axisSet!=NoAxes)
        {
          vFrom = ConstrainToAxis(vFrom, set[axisIndex]);
          vTo = ConstrainToAxis(vTo, set[axisIndex]);
        }
      qDrag = Qt_FromBallPoints(vFrom, vTo);
      qNow = Qt_Mul(qDrag, qDown);
    }
  else
    {
      if (axisSet!=NoAxes) axisIndex = NearestConstraintAxis(vTo, set, setSize);
    }
  Qt_ToBallPoints(qDown, &vrFrom, &vrTo);
  Qt_ToMatrix(Qt_Conj(qNow), mNow); /* Gives transpose for GL. */
}

/* Return rotation matrix defined by controller use. */
/* ================================================= */

void
ArcBall_Value (HMatrix m_Now)
{
  ArcBall_CopyMat (mNow, m_Now);
}

/* Extract rotation angles from matrix */
/* =================================== */

void
ArcBall_Values (double *alpha,
                double *beta,
                double *gamma)
{
  if ((*beta=asin(-mNow[0][2]))!=0.0)
    {
      *gamma=atan2(mNow[1][2],mNow[2][2]);
      *alpha=atan2(mNow[0][1],mNow[0][0]);
    }
  else
    {
      *gamma=atan2(mNow[1][0],mNow[1][1]);
      *alpha=0.0;
    }
}

/* Begin drag sequence. */
/* ==================== */

void
ArcBall_BeginDrag (void)
{
  dragging = TRUE;
  vDown = vNow;
}

/* Stop drag sequence. */
/* =================== */

void
ArcBall_EndDrag (void)
{
  dragging = FALSE;
  qDown = qNow;

  ArcBall_CopyMat (mNow, mDown);
}

/*===================*/
/***** BallAux.c *****/
/*===================*/

/* Return quaternion product qL * qR.  Note: order is important! */
/* To combine rotations, use the product Mul(qSecond, qFirst),   */
/* which gives the effect of rotating by qFirst then qSecond.    */
/* ============================================================= */

static Quat
Qt_Mul (Quat qL,
        Quat qR)
{
  Quat qq;
  qq.w = qL.w*qR.w - qL.x*qR.x - qL.y*qR.y - qL.z*qR.z;
  qq.x = qL.w*qR.x + qL.x*qR.w + qL.y*qR.z - qL.z*qR.y;
  qq.y = qL.w*qR.y + qL.y*qR.w + qL.z*qR.x - qL.x*qR.z;
  qq.z = qL.w*qR.z + qL.z*qR.w + qL.x*qR.y - qL.y*qR.x;
  return (qq);
}

/* Construct rotation matrix from (possibly non-unit) quaternion. */
/* Assumes matrix is used to multiply column vector on the left:  */
/* vnew = mat vold.  Works correctly for right-handed coordinate  */
/* system and right-handed rotations.                             */
/* ============================================================== */

static void
Qt_ToMatrix (Quat    q,
             HMatrix out)
{
  double Nq = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
  double s = (Nq > 0.0) ? (2.0 / Nq) : 0.0;
  double xs = q.x*s,              ys = q.y*s,          zs = q.z*s;
  double wx = q.w*xs,              wy = q.w*ys,          wz = q.w*zs;
  double xx = q.x*xs,              xy = q.x*ys,          xz = q.x*zs;
  double yy = q.y*ys,              yz = q.y*zs,          zz = q.z*zs;
  out[X][X] = 1.0 - (yy + zz); out[Y][X] = xy + wz; out[Z][X] = xz - wy;
  out[X][Y] = xy - wz; out[Y][Y] = 1.0 - (xx + zz); out[Z][Y] = yz + wx;
  out[X][Z] = xz + wy; out[Y][Z] = yz - wx; out[Z][Z] = 1.0 - (xx + yy);
  out[X][W] = out[Y][W] = out[Z][W] = out[W][X] = out[W][Y] = out[W][Z] = 0.0;
  out[W][W] = 1.0;
}

/* Return conjugate of quaternion. */
/* =============================== */

static Quat
Qt_Conj (Quat q)
{
  Quat qq;
  qq.x = -q.x; qq.y = -q.y; qq.z = -q.z; qq.w = q.w;
  return (qq);
}

/* Return vector formed from components */
/* ==================================== */

static HVect
V3_ (double x,
     double y,
     double z)
{
  HVect v;
  v.x = x; v.y = y; v.z = z; v.w = 0;
  return (v);
}

/* Return norm of v, defined as sum of squares of components */
/* ========================================================= */

static double
V3_Norm (HVect v)
{
  return ( v.x*v.x + v.y*v.y + v.z*v.z );
}

/* Return unit magnitude vector in direction of v */
/* ============================================== */

static HVect
V3_Unit (HVect v)
{
  static HVect u = {0, 0, 0, 0};
  double vlen = sqrt(V3_Norm(v));

  if (vlen != 0.0)
    {
      u.x = v.x/vlen;
      u.y = v.y/vlen;
      u.z = v.z/vlen;
    }
  return (u);
}

/* Return version of v scaled by s */
/* =============================== */

static HVect
V3_Scale (HVect v,
          double s)
{
  HVect u;
  u.x = s*v.x; u.y = s*v.y; u.z = s*v.z; u.w = v.w;
  return (u);
}

/* Return negative of v */
/* ==================== */

static HVect
V3_Negate (HVect v)
{
  static HVect u = {0, 0, 0, 0};
  u.x = -v.x; u.y = -v.y; u.z = -v.z;
  return (u);
}

/* Return sum of v1 and v2 */
/* ======================= */
/*
static HVect
V3_Add (HVect v1,
        HVect v2)
{
  static HVect v = {0, 0, 0, 0};
  v.x = v1.x+v2.x; v.y = v1.y+v2.y; v.z = v1.z+v2.z;
  return (v);
}
*/
/* Return difference of v1 minus v2 */
/* ================================ */

static HVect
V3_Sub (HVect v1,
        HVect v2)
{
  static HVect v = {0, 0, 0, 0};
  v.x = v1.x-v2.x; v.y = v1.y-v2.y; v.z = v1.z-v2.z;
  return (v);
}

/* Halve arc between unit vectors v0 and v1. */
/* ========================================= */
/*
static HVect
V3_Bisect (HVect v0,
           HVect v1)
{
  HVect v = {0, 0, 0, 0};
  double Nv;

  v = V3_Add(v0, v1);
  Nv = V3_Norm(v);
  if (Nv < 1.0e-5) v = V3_(0, 0, 1);
  else v = V3_Scale(v, 1/sqrt(Nv));
  return (v);
}
*/

/* Return dot product of v1 and v2 */
/* =============================== */

static double
V3_Dot (HVect v1,
        HVect v2)
{
  return (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z);
}


/* Return cross product, v1 x v2 */
/* ============================= */
/*
static HVect
V3_Cross (HVect v1,
          HVect v2)
{
  static HVect v = {0, 0, 0, 0};
  v.x = v1.y*v2.z-v1.z*v2.y;
  v.y = v1.z*v2.x-v1.x*v2.z;
  v.z = v1.x*v2.y-v1.y*v2.x;
  return (v);
}
*/

void
ArcBall_CopyMat (HMatrix inm,
                 HMatrix outm)
{
  int x=0,y=0;

  for (x=0;x<4;x++)
    {
      for (y=0;y<4;y++)
        {
          outm[y][x]=inm[y][x];
        }
    }
}

/*=====================================================*/
/**** BallMath.c - Essential routines for ArcBall.  ****/
/*=====================================================*/

/* Convert window coordinates to sphere coordinates. */
/* ================================================= */

static HVect
MouseOnSphere (HVect  mouse,
               HVect  ballCenter,
               double ballRadius)
{
  HVect ballMouse;
  register double mag;

  ballMouse.x = (mouse.x - ballCenter.x) / ballRadius;
  ballMouse.y = (mouse.y - ballCenter.y) / ballRadius;
  mag = ballMouse.x*ballMouse.x + ballMouse.y*ballMouse.y;
  if (mag > 1.0)
    {
      register double scale = 1.0/sqrt(mag);
      ballMouse.x *= scale; ballMouse.y *= scale;
      ballMouse.z = 0.0;
    }
  else ballMouse.z = sqrt(1 - mag);
  ballMouse.w = 0.0;
  return (ballMouse);
}

/* Construct a unit quaternion from two points on unit sphere */
/* ========================================================== */

static Quat
Qt_FromBallPoints (HVect from,
                   HVect to)
{
  Quat qu;
  qu.x = from.y*to.z - from.z*to.y;
  qu.y = from.z*to.x - from.x*to.z;
  qu.z = from.x*to.y - from.y*to.x;
  qu.w = from.x*to.x + from.y*to.y + from.z*to.z;
  return (qu);
}

/* Convert a unit quaternion to two points on unit sphere */
/* ====================================================== */

static void
Qt_ToBallPoints (Quat   q,
                 HVect *arcFrom,
                 HVect *arcTo)
{
  double s;

  s = sqrt(q.x*q.x + q.y*q.y);
  if (s == 0.0) *arcFrom = V3_(0.0, 1.0, 0.0);
  else          *arcFrom = V3_(-q.y/s, q.x/s, 0.0);
  arcTo->x = q.w*arcFrom->x - q.z*arcFrom->y;
  arcTo->y = q.w*arcFrom->y + q.z*arcFrom->x;
  arcTo->z = q.x*arcFrom->y - q.y*arcFrom->x;
  if (q.w < 0.0) *arcFrom = V3_(-arcFrom->x, -arcFrom->y, 0.0);
}

/* Force sphere point to be perpendicular to axis. */
/* =============================================== */

static HVect
ConstrainToAxis (HVect loose,
                 HVect axis)
{
  HVect onPlane;
  register double norm;

  onPlane = V3_Sub(loose, V3_Scale(axis, V3_Dot(axis, loose)));
  norm = V3_Norm(onPlane);
  if (norm > 0.0)
    {
      if (onPlane.z < 0.0) onPlane = V3_Negate(onPlane);
      return ( V3_Scale(onPlane, 1/sqrt(norm)) );
    }
  /* else drop through */
  /* ================= */

  if (axis.z == 1) onPlane = V3_(1.0, 0.0, 0.0);
  else             onPlane = V3_Unit(V3_(-axis.y, axis.x, 0.0));
  return (onPlane);
}

/* Find the index of nearest arc of axis set. */
/* ========================================== */

static int
NearestConstraintAxis (HVect  loose,
                       HVect *axes,
                       int    nAxes)
{
  HVect onPlane;
  register double max, dot;
  register int i, nearest;
  max = -1; nearest = 0;

  for (i=0; i<nAxes; i++)
    {
      onPlane = ConstrainToAxis(loose, axes[i]);
      dot = V3_Dot(onPlane, loose);
      if (dot>max)
        {
          max = dot; nearest = i;
        }
    }
  return (nearest);
}
