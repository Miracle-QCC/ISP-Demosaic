
import cv2
import numpy as np
from PIL import Image
from torchvision import transforms


def convertPPM(img_file,shape, depth):
    img = cv2.imread(img_file, cv2.IMREAD_UNCHANGED).convert('RGB')
    # cv2.imshow('a',img)
    # cv2.waitKey(0)

    # img = np.transpose(img,(0,2,1))
    data = img.flatten()
    G = data[::3]
    R = data[1::3]
    B = data[2::3]
    data_ = np.zeros((1,3*shape[0]*shape[1]),dtype='uint8')
    for i in range(shape[0]*shape[1]):
        data_[0][i*3] = R[i]
        data_[0][i*3+1] = G[i]
        data_[0][i*3+2] = B[i]

    # data = data.flatten()
    # data = data.numpy()
    width ,height = shape[1],shape[0]
    with open('customize.ppm', 'w') as f:
        f.write("P6\n")
        f.write("# Created by CVITEK DPU Model\n")
        f.write("{} {}\n".format(width, height))
        f.write("{}\n".format((1<<depth) - 1))
    with open('customize.ppm','ab') as f:
        data_.tofile(f)


# 只能转8bit的ppm
def ImageConvertPNG(filename):
    im = Image.open(filename)  # open ppm file

    newname = filename[:-4] + '.png'  # new name for png file
    im.save(newname)

if __name__ == '__main__':
    # Png2PPM('')
    convertPPM('0002.png', (1848,2040),8)
    # img = np.random.randn(1,2,3) * 2
    # img = img.astype('uint16')
    # cv2.imwrite('customize.png',img)
    # data =  cv2.imread('customize.png')
    # data = transforms.ToTensor()(data)
