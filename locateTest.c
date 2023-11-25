#include <stdio.h>
#include "image8bit.h"

int main(void){
    Image img = ImageLoad("test/original.pgm");
    int x = 59;
    int y = 38;
    int *px;
    int *py;
    Image img2 = ImageCrop(img, x, y, 5, 5);
    int maxval = ImageMaxval(img2);
    ImageSave(img2, "teste.pgm");
    int b = ImageMatchSubImage(img, x, y, img2);
    printf("ImageMatchSubImage > %d\n", b);
    int b2 = ImageLocateSubImage(img, px, py, img2);
    printf("ImageLocateSubImage > %d\n%d\n", b2, maxval);
}