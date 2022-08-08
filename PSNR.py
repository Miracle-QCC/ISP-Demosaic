import torch
import PIL.Image as Image
import torch.nn as nn
import numpy as np
import torchvision


def PSNR(mse, peak=1.):
    return 10 * torch.log10((peak ** 2) / mse)


if __name__ == '__main__':
    critien = nn.MSELoss()
    

    img1 = np.array(Image.open('data/0001.bmp').convert('RGB'))
    # img1 = img1 / 255.0
    img1_tensor = torchvision.transforms.ToTensor()(img1)

    img2 = np.array(Image.open('data/0001-modelout.png').convert('RGB'))
    # img2 = img2 / 255.0
    img2_tensor = torchvision.transforms.ToTensor()(img2)


    # loss2 = critien(torch.sqrt(img_tensor),torch.sqrt(img_100_tensor))
    # print("loss1:",loss1)
    # print("loss2:",loss2)


    mse = (img1_tensor - img2_tensor).pow(2).mean()
    print("PSNR:",PSNR(mse, peak=1.))



