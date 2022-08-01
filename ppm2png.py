import cv2
import numpy as np
from PIL import Image



def ImageConvertPNG(filename):
    im = Image.open(filename)  # open ppm file

    newname = filename[:-4] + '.png'  # new name for png file
    im.save(newname)
def CV2ConvertPNG(filename):
    with open(filename,'rb') as f:
        tmp_list = []
        for i in range(4):
            data = f.readline()
            # print(data)
            tmp_list.append(data)
        width,height = map(int,tmp_list[2][:-1].split())
        depth = int(np.math.log(int(tmp_list[3][:-1]), 2)) + 1
        if depth > 8:
            img = np.zeros((height,width,3),dtype='uint16')
        else:
            img = np.zeros((height, width, 3), dtype='uint8')
        for y in range(height):
            for x in range(width):
                img[y, x, 0] = int.from_bytes(f.read(2), byteorder="big") # R
                img[y, x, 1] = int.from_bytes(f.read(2), byteorder="big") # G
                img[y, x, 2] = int.from_bytes(f.read(2), byteorder="big") # B
    # imageio.imwrite('outfile.png', img,)
    # img.tofile('outfile.png')

    img_scaled = cv2.normalize(img, dst=None, alpha=0, beta=65535, norm_type=cv2.NORM_MINMAX)

    cv2.imshow('aa',img_scaled)
    cv2.waitKey(0)
    # plt.imshow(img,  vmin=0, vmax=4096)
    # plt.show()



def main(file_name, depth):
    if depth <= 8:
        ImageConvertPNG(file_name)
    else:
        CV2ConvertPNG(file_name)


if __name__ == '__main__':
    # main('out_le_000.ppm',14)
    img = cv2.imread('out_le_000.ppm',-1)
    cv2.imshow('a',img)
    cv2.waitKey(0)
