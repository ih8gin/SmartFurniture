import os.path

import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

n_features = 0
features = np.zeros((10, 12, 59), dtype=int)

def load_img(path):
    img = Image.open(path)
    img = img.resize((240, 320))
    x = np.array(img)
    plt.imshow(x)
    plt.show()
    return x


def rgb2grey(img):
    h, w, _ = img.shape
    grey = np.zeros((h, w))
    for i in range(h):
        for j in range(w):
            grey[i, j] = img[i, j, 0] * 0.33 + img[i, j, 1] * 0.59 + img[i, j, 2] * 0.11
    plt.imshow(grey, cmap=plt.get_cmap('Greys'))
    plt.show()
    return grey


def get_lbph_map(grey):
    h, w = grey.shape
    lbph_map = np.zeros((h, w), dtype=int)
    for i in range(1, h - 1):
        for j in range(1, w - 1):
            # 顺时针方向定序，左上像素点为高位，左中像素点为低位
            lbph_map[i, j] += (grey[i - 1, j - 1] > grey[i, j]) << 7
            lbph_map[i, j] += (grey[i - 1, j] > grey[i, j]) << 6
            lbph_map[i, j] += (grey[i - 1, j + 1] > grey[i, j]) << 5
            lbph_map[i, j] += (grey[i, j + 1] > grey[i, j]) << 4
            lbph_map[i, j] += (grey[i + 1, j + 1] > grey[i, j]) << 3
            lbph_map[i, j] += (grey[i + 1, j] > grey[i, j]) << 2
            lbph_map[i, j] += (grey[i + 1, j - 1] > grey[i, j]) << 1
            lbph_map[i, j] += (grey[i, j - 1] > grey[i, j])
    plt.imshow(lbph_map, cmap=plt.get_cmap('Greys'))
    plt.show()
    return lbph_map


patterns = {
    '0b0': 0,
    '0b11111111': 1,
    '0b1': 2,  '0b11': 3,  '0b111': 4,  '0b1111': 5,  '0b11111': 6,  '0b111111': 7,  '0b1111111': 8,
    '0b10': 9,  '0b110': 10, '0b1110': 11, '0b11110': 12, '0b111110': 13, '0b1111110': 14, '0b11111110': 15,
    '0b100': 16, '0b1100': 17, '0b11100': 18, '0b111100': 19, '01111100': 20, '0b11111100': 21, '0b11111101': 22,
    '0b1000': 23, '0b11000': 24, '0b111000': 25, '0b1111000': 26, '0b11111000': 27, '0b11111001': 28, '0b11111011': 29,
    '0b10000': 30, '0b110000': 31, '0b1110000': 32, '0b11110000': 33, '0b11110001': 34, '0b11110011': 35, '0b11110111': 36,
    '0b100000': 37, '0b1100000': 38, '0b11100000': 39, '0b11100001': 40, '0b11100011': 41, '0b11100111': 42, '0b11101111': 43,
    '0b1000000': 44, '0b11000000': 45, '0b11000001': 46, '0b11000011': 47, '0b11000111': 48, '0b11001111': 49, '0b11011111': 50,
    '0b10000000': 51, '0b10000001': 52, '0b10000011': 53, '0b10000111': 54, '0b10001111': 55, '0b10011111': 56, '0b10111111': 57,
}


def get_feature(img):
    grey = rgb2grey(img)
    lbph_map = get_lbph_map(grey)
    h, w = lbph_map.shape
    feature = np.zeros((12, 59))
    for i in range(h):
        for j in range(w):
            idx = int(i / 80) * 4 + int(j / 80)
            pattern = patterns.get(bin(lbph_map[i, j]), 58)
            feature[idx, pattern] += 1
    for i in range(feature.shape[0]):
        plt.subplot(h//80,w//80,i+1)
        plt.bar(range(feature.shape[1]), feature[i,:], width=1)
    plt.show()
    return feature


def compute_distance(f1, f2):
    assert(f1.shape == f2.shape)
    distance = 0
    for i in range(f1.shape[0]):
        for j in range(f2.shape[1]):
            distance += (f1[i, j] - f2[i, j]) ** 2 / (f1[i, j] + 1)
    return distance

def load_img_from_vector(H, W, C, path):
    pic = np.zeros((H, W, C), dtype=int)
    pic_cache = np.load(path)
    for h in range(H):
        for w in range(W):
            rgb565 = pic_cache[h * W + w, 0]
            r = (rgb565 & 0xf800) >> 8
            g = (rgb565 & 0x07e0) >> 3
            b = (rgb565 & 0x001f) << 3
            pic[h, w, 0] = r
            pic[h, w, 1] = g
            pic[h, w, 2] = b
    return pic

def init():
    global n_features
    for i in range(10):
        path = './picture_resp/'+str(i)+'.png'
        if not os.path.exists(path):
            break
        img = load_img(path)
        features[i, :, :] = get_feature(img)
        n_features = i
    return n_features

def check(img):
    f = get_feature(img)
    ret = -1
    best_score = 10000
    for i in range(n_features):
        if compute_distance(f, features[i, :, :]) < best_score:
            ret = i
    return ret, best_score

def inspect():
    for i in range(n_features):
        plt.imshow(features[i,:,:])
        plt.show()


H = 240
W = 320
C = 3
img = load_img_from_vector(W, H, C, "./pic.npy")
plt.imshow(img)
plt.show()
img = load_img_from_vector(H, W, C, "./pic.npy")
plt.imshow(img)
plt.show()
get_feature(img)
# f1 = get_feature(img)
# f2 = get_feature(img_c)
# print(compute_distance(f1,f2))

# path = 'F:/一寸登记照 - 副本.jpg'
# img = load_img(path)
# get_feature(img)
