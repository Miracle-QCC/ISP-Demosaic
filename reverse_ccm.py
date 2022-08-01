import numpy as np
import cv2
import torch


ccm_table = np.array([[2008,-1012,28],
             [-543,2026,-459],
             [-47,-613,1684]],np.float)
ccm_table = torch.from_numpy(ccm_table / 1024.0)

def reverse_ccm(img):

    inv_ccm = np.linalg.inv(ccm_table)
    inv_ccm = torch.from_numpy(inv_ccm)
    shape = img.shape
    # img = img.reshape(3,-1)
    x = inv_ccm[0,0] * img[:,:,0] + inv_ccm[0,1] * img[:,:,1] + inv_ccm[0,2] * img[:,:,2]
    y = inv_ccm[1, 0] * img[:, :, 0] + inv_ccm[1, 1] * img[:, :, 1] + inv_ccm[1, 2] * img[:, :, 2]
    z = inv_ccm[2, 0] * img[:, :, 0] + inv_ccm[2, 1] * img[:, :, 1] + inv_ccm[2, 2] * img[:, :, 2]
    out = torch.stack((x,y,z),dim=-1)
    out = out.numpy().astype('uint8')
    # img = np.transpose(img,(1,2,0))
    return out


if __name__ == '__main__':
    # # reverse_ccm()
    img = cv2.imread('ILSVRC2017_test_00000001.JPEG').astype(float)
    img = torch.from_numpy(img)
    out = reverse_ccm(img)

    cv2.imshow('aa',out)
    cv2.waitKey(0)
    # cv2.imwrite('reverse_ccm.png',out)
    # random_ccm()

