#ifndef ARCBALLH
#define ARCBALLH

typedef struct {double x, y, z, w;} Quat;
enum QuatPart {X, Y, Z, W, QuatLen};
typedef Quat HVect;
typedef double HMatrix[QuatLen][QuatLen];

typedef enum AxisSet{NoAxes, CameraAxes, BodyAxes, OtherAxes, NSets} AxisSet;
typedef double *ConstraintSet;

extern Quat qOne;

extern void ArcBall_Init();
extern void ArcBall_Place(HVect Center, double Radius);
extern void ArcBall_UseSet(AxisSet axis_Set);
extern void ArcBall_Update(void);
extern void ArcBall_Value(HMatrix m_Now);
extern void ArcBall_Values(double *alpha,double *beta,double *gamma);
extern void ArcBall_BeginDrag(void);
extern void ArcBall_EndDrag(void);
extern void ArcBall_Mouse(HVect v_Now);
extern void ArcBall_CopyMat(HMatrix inm,HMatrix outm);

#endif
