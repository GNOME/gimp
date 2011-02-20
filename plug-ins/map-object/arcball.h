#ifndef __ARCBALL_H__
#define __ARCBALL_H__

typedef struct
{
  double x, y, z, w;
} Quat;

enum QuatPart
{
  X,
  Y,
  Z,
  W,
  QuatLen
};

typedef Quat HVect;

typedef double HMatrix[QuatLen][QuatLen];

typedef enum AxisSet
{
  NoAxes,
  CameraAxes,
  BodyAxes,
  OtherAxes,
  NSets
} AxisSet;

typedef double *ConstraintSet;

extern Quat qOne;

void ArcBall_Init      (void);
void ArcBall_Place     (HVect    Center,
                        double   Radius);
void ArcBall_UseSet    (AxisSet  axis_Set);
void ArcBall_Update    (void);
void ArcBall_Value     (HMatrix  m_Now);
void ArcBall_Values    (double  *alpha,
                        double  *beta,
                        double  *gamma);
void ArcBall_BeginDrag (void);
void ArcBall_EndDrag   (void);
void ArcBall_Mouse     (HVect    v_Now);
void ArcBall_CopyMat   (HMatrix  inm,
                        HMatrix  outm);

#endif /* __ARCBALL_H__ */
