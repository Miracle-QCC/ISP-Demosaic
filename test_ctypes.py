import numpy as np
import ctypes

arr = np.zeros((2,3))
x = 1
sotools = ctypes.cdll.LoadLibrary('tools.so')
dataptr = arr.ctypes.data_as(ctypes.c_char_p)  # 转换为指针传入c
sotools.crop_raw(dataptr, x, 1, 0, 1 ,arr)
