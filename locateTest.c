#include <stdio.h>
#include "image8bit.h"

int main(void){
    Image img = ImageLoad("test/original.pgm");
    int x = 57;
    int y = 38;
    int *px;
    int *py;
    Image img2 = ImageCrop(img, x, y, ImageWidth(img) - x, ImageHeight(img) - y);
    int b = ImageMatchSubImage(img, x, y, img2);
    printf("ImageMatchSubImage > %d\n", b);
    int b2 = ImageLocateSubImage(img, px, py, img2);
    printf("ImageLocateSubImage > %d\n", b2);
}