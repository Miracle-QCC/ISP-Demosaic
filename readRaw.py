import numpy as np
import cv2
import imageio


if __name__ == '__main__':

    img = imageio.imread('out_le_000.ppm').astype(np.float32)
    scale = 2**12-1 # 归一化
    scale = 255/scale  # 拉伸至0-255
    img = img*scale # 拉伸至0-255
    img = img.astype(np.uint8) # 修改type
    mean = np.mean(img)
    print('mean:',mean)
    cv2.imshow('xxx',img)
    cv2.waitKey(0)
