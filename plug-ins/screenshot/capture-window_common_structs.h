



typedef struct {
	int width;
	int height;
	int cbsize;
	byte pixels[1];
} magCapturedData;
#define CAPTURED_DATA_SIZE(x) (sizeof(magCapturedData) + (sizeof(byte) * ((x) - 1)))


