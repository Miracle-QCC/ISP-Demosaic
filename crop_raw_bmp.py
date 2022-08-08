import os
import os.path as osp
import time
from glob import glob
import ctypes
import cv2
import numpy as np
import torch
import tqdm
from PIL import Image
import argparse
from multiprocessing import Pool
from scipy.io import loadmat, savemat

# numpy格式的增强方法
def aug_img_np(img, mode=0):
    # data augmentation
    if mode == 0:
        return img
    elif mode == 1:
        return np.ascontiguousarray(np.flipud(img))
    elif mode == 2:
        return np.ascontiguousarray(np.rot90(img))
    elif mode == 3:
        return np.ascontiguousarray(np.flipud(np.rot90(img)))
    elif mode == 4:
        return np.ascontiguousarray(np.rot90(img, k=2))
    elif mode == 5:
        return np.ascontiguousarray(np.flipud(np.rot90(img, k=2)))
    elif mode == 6:
        return np.ascontiguousarray(np.rot90(img, k=3))
    elif mode == 7:
        return np.ascontiguousarray(np.flipud(np.rot90(img, k=3)))
def main():
    parser = argparse.ArgumentParser(description='A multi-thread tool to crop sub images')
    parser.add_argument('--src_path', type=str, default=r'E:\Program\DL\ISP\utils\raw',
                        help='path to original mat folder')
    parser.add_argument('--save_path', type=str, default=r'E:\Program\DL\ISP\utils\new_crop_raw',
                        help='path to output folder')
    args = parser.parse_args()
    args.save_path = osp.join(os.getcwd(), args.save_path)

    os.makedirs(args.save_path, exist_ok=True)

    n_thread = 32
    crop_sz = 256
    stride = 128
    thres_sz = 128  # keep the regions in the last row/column whose thres_sz is over 256.
    ext = 'bmp'
    r_ext = 'raw'
    bmp_list = sorted(
        glob(osp.join(os.getcwd(), args.src_path, '*' + ext))
    )
    raw_list = sorted(
        glob(osp.join(os.getcwd(), args.src_path, '*' + r_ext))
    )
  
    pool = Pool(n_thread)
    # worker_cropraw_bmp(raw_list[0], bmp_list[0], args.save_path, crop_sz, stride, thres_sz)

    for i in tqdm.tqdm(range(len(raw_list))):
        print('processing {}/{}'.format(i+1, len(bmp_list)))
        pool.apply_async(worker_cropraw_bmp, args=(raw_list[i],bmp_list[i], args.save_path, crop_sz, stride, thres_sz))
    pool.close()
    pool.join()

    print('All subprocesses done.')


def worker_cropraw_bmp(raw_path, bmp_path, save_dir, crop_sz, stride, thres_sz):
    raw_name = osp.basename(raw_path)
    bmp_name = osp.basename(bmp_path)
    bmp = Image.open(bmp_path).convert('RGB')
    bmp = np.array(bmp)

    h, w,c = bmp.shape
    raw = np.fromfile(raw_path,'uint16').reshape(h,w)
    raw_mat = np.zeros((h,w,4),'uint16')
    #   R
    R_mask = np.zeros((h,w),'uint16')
    R_mask[::2,::2] = 1
    raw_mat[:,:,0] = raw * R_mask
    # Gr
    Gr_mask = np.zeros((h,w),'uint16')
    Gr_mask[::2,1::2] = 1
    raw_mat[:, :, 1] = raw * Gr_mask

    #Gb
    Gb_mask = np.zeros((h, w), 'uint16')
    Gb_mask[1::2, ::2] = 1
    raw_mat[:, :, 2] = raw * Gb_mask

    # B
    B_mask = np.zeros((h, w), 'uint16')
    B_mask[1::2, 1::2] = 1
    raw_mat[:, :, 3] = raw * B_mask

    h_space = np.arange(0, h - crop_sz + 1, stride)
    if h - (h_space[-1] + crop_sz) > thres_sz:
        h_space = np.append(h_space, h - crop_sz)
    w_space = np.arange(0, w - crop_sz + 1, stride)
    if w - (w_space[-1] + crop_sz) > thres_sz:
        w_space = np.append(w_space, w - crop_sz)

    index = 0
    for x in h_space:
        for y in w_space:
            index += 1
            patch_bmp_name = bmp_name.replace('.bmp', '_s{:05d}.bmp'.format(index))
            patch_raw_name = raw_name.replace('.raw', '_s{:05d}.mat'.format(index))
            patch_bmp = bmp[x:x + crop_sz, y:y + crop_sz, :]
            patch_raw = raw_mat[x:x + crop_sz, y:y + crop_sz,:]

            # patch_bmp.save(os.path.join(save_dir,patch_bmp_name))
            savemat(os.path.join(save_dir,patch_raw_name), {'raw': patch_raw,'gt':patch_bmp},)
            # sotools.crop_raw(raw_, x,x + crop_sz, y, y + crop_sz,os.path.join(save_dir,patch_raw_name))

if __name__ == '__main__':

    # sotools = ctypes.cdll.LoadLibrary('tools.so')
    # main()
    a = loadmat(r'E:\Program\DL\ISP\utils\new_crop_raw\0020_s00127.mat')
    x = 1
    from torchvision import transforms

    cv2.imshow('bbb', a["gt"])
    cv2.waitKey(0)
    b_ = aug_img_np(a["gt"],1)

    cv2.imshow('a',b_)
    cv2.waitKey(0)
    c = transforms.ToTensor()(b_)
    print(c.shape)

    print(a["raw"].shape)
    print(a["gt"].shape)
#
