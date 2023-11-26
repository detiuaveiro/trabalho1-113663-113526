#include <stdio.h>
#include "image8bit.h"

int main(void){
    Image img = ImageLoad("test/original.pgm");
    int x = ImageWidth(img) / 2;
    int y = ImageHeight(img) / 2;
    int w = ImageWidth(img) / 2;
    int h = ImageHeight(img) / 2;
    int *px;
    int *py;
    Image imgBC = ImageCrop(img, 0, 0, 1, 1);
    Image imgWC = ImageCrop(img, x, y, w, h);
    Image imgWC2 = ImageCreate(2,2,255);
    ImageSetPixel(imgWC2, 0, 0, 0);
    ImageSetPixel(imgWC2, 1, 0, 255);
    ImageSetPixel(imgWC2, 1, 1, 0);
    ImageSetPixel(imgWC2, 0, 1, 255);
    ImageSetPixel(imgWC, w - 1, h - 1, 1);
    ImageSave(imgBC, "imgBC.pgm");
    ImageSave(imgWC, "imgWC.pgm");
    ImageSave(imgWC2, "imgWC2.pgm");
    int bestCase = ImageLocateSubImage(img, px, py, imgBC);
    int worstCase = ImageLocateSubImage(img, px, py, imgWC);

    /* Melhor caso ImageLocateSubImage & Image MatchSubImage: img2 é apenas
     * um pixel e corresponde ao primeiro pixel da img1 */
    /* Pior caso ImageLocateSubImage & Image MatchSubImage: img2 difere no
     * ultimo pixel, e é uma imagem com metade da altura e largura da img1 */

}