# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# All rights reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc

import os
import cv2
import numpy as np
import argparse
from nuscenes.nuscenes import NuScenes

CWD = os.path.dirname(__file__)
if CWD == '':
    CWD = '.'

parser = argparse.ArgumentParser(
    description='convert lidar pointcloud from nuScenes dataset to inputs of the Sample DataReader')

parser.add_argument('-r', '--range', type=str, default="-10.0 10.0 -40.0 40.0 -3 3",
                    help='the range of points: [minX, maxX, minY, maxY, minZ, maxZ]',
                    required=False)

parser.add_argument('-p', '--path', type=str,
                    help='the root path of nuScenes dataset',
                    required=True)

parser.add_argument('-n', '--num_of_samples', type=int, default=100,
                    help='generate n samples from nuScenes dataset',
                    required=False)

parser.add_argument('-v', '--version', type=str, default="v1.0-trainval",
                    help='the version of nuScenes dataset, [v1.0-trainval, v1.0-test, v1.0-mini]',
                    required=False)


args = parser.parse_args()

minX, maxX, minY, maxY, minZ, maxZ = [float(x) for x in args.range.split(' ')]

if not os.path.exists('/tmp/rgb2yuv'):
    if 0 != os.system('gcc %s/rgb2yuv.c -o /tmp/rgb2yuv' %(CWD)):
        print("failed to compile the simple host color convert tool /tmp/rgb2yuv")
        exit()


def create_nuscenes_infos(root_path,
                          version='v1.0-trainval'
                          ):
    from nuscenes.nuscenes import NuScenes
    nusc = NuScenes(version=version, dataroot=root_path, verbose=True)
    return nusc

def save_file(img, path):
    img.tofile(path)


def convert_to_nv12(origin, new, width, height):
    cmd = '/tmp/rgb2yuv nv12 %s %s %s %s'%(origin, new,width, height)
    if 0 != os.system(cmd):
        print('failed: %s' %(cmd))
        exit()   

def generate_data(nusc):
    sensor_types = [
        'LIDAR_TOP',
        'CAM_FRONT',
        'CAM_FRONT_RIGHT',
        'CAM_FRONT_LEFT',
        'CAM_BACK',
        'CAM_BACK_LEFT',
        'CAM_BACK_RIGHT',
        ]
    cwd = os.getcwd()
    for sensor in sensor_types:
        dir = '%s/%s' % (cwd, sensor)
        print ("dir: ", dir)
        os.makedirs(dir, exist_ok=True)
        
    index = 0
    height = 1024
    width = 1920
    ratioI = height / width
    ratioP = (maxY - minY) / (maxX - minX)
    if ratioI < ratioP:
        offsetY = 0
        ratioH = height / (maxY - minY)
        ratioW = ratioH
        offsetX = (width - ratioW * (maxX - minX)) / 2
    else:
        offsetX = 0
        ratioW = width / (maxX - minX)
        ratioH = ratioW
        offsetY = (height - ratioH * (maxY - minY)) / 2

    print('range = [%s %s %s %s %s %s]'%(minX, maxX, minY, maxY, minZ, maxZ))
    print('ratioI:', ratioI)
    print('ratioP:', ratioP)
    print('offsetX:', offsetX)
    print('offsetY:', offsetY)
    print('ratioW:', ratioW)
    print('ratioH:', ratioH)
    with open('%s/LIDAR_TOP/info.txt'%(cwd), 'w') as f:
        f.write('offsetX: %s\n'%(offsetX))
        f.write('offsetY: %s\n'%(offsetY))
        f.write('ratioW: %s\n'%(ratioW))
        f.write('ratioH: %s\n'%(ratioH))
    for sample in nusc.sample:
        if index > args.num_of_samples:
            break
        for sensor in sensor_types:
            token = sample['data'][sensor]
            data_path, _, _ = nusc.get_sample_data(token)
            dir = '%s/%s' % (cwd, sensor)
            if sensor == 'LIDAR_TOP':
                pcd = np.fromfile(data_path, np.float32).reshape(-1, 5)
                img = np.zeros([height, width, 3],dtype=np.uint8)
                img[:, :, :] = [255, 255, 255]
                # print("pcd: ", pcd)
                pcdL = [(x,y,z) for x,y,z,r,t in pcd]
                pcdL.sort(key=lambda x:x[2]) # sort by Z
                for x,y,z in pcdL:
                    w = int(offsetX + ratioW * (x - minX))
                    h = int(offsetY + ratioH * (maxY - y))
                    color = HeatColor(z, minZ, maxZ)
                    # print('color: ', color)
                    for dx in [0, 1]:
                        for dy in [0, 1]:
                            w1 = max(0, min(w+dx, width-1))
                            h1 = max(0, min(h+dy, height-1))
                            img[h1][w1] = color
                # cv2.imwrite('new_image.jpg', img)
                
                tmp_path = os.path.join(cwd, 'img.rgb')
                nv12_file_path = os.path.join('%s/%s'%(cwd, sensor), '%s.nv12'%(index))  
                save_file(img, tmp_path)
                convert_to_nv12(tmp_path, nv12_file_path, width, height)

            else:
                image_bgr = cv2.imread(data_path, cv2.IMREAD_COLOR)
                image_bgr = cv2.resize(image_bgr,(width, height))
                # Check if the image was successfully loaded
                if image_bgr is None:
                    raise ValueError("Could not open or find the image.")
                # Convert BGR to RGB
                image_rgb = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB)
       
                tmp_path = os.path.join(cwd, 'img.rgb')
                nv12_file_path = os.path.join('%s/%s'%(cwd, sensor), '%s.nv12'%(index))  
                save_file(image_rgb, tmp_path)
                convert_to_nv12(tmp_path, nv12_file_path, width, height)
        index +=1
            
def HeatColor(z, minZ, maxZ):
    normalized_z = ( z - minZ ) / ( maxZ - minZ )
    low_color = np.array([0, 0, 255])  # Blue
    medium_color = np.array([0, 255, 0])  # Green
    high_color = np.array([255, 0, 0])  # Red
    white_color = np.array([255, 255, 255])  # White

    # Function to interpolate colors
    def interpolate_color(value, low_color, medium_color, high_color):
        if value < 0.5 and value > 0.0:
            return low_color + value * (medium_color - low_color)
        elif value < 1 and value >= 0.5:
            return medium_color + (value - 0.5) * (high_color - medium_color)
        else:
            return white_color
    return interpolate_color(normalized_z, low_color, medium_color, high_color)

root_path = args.path
version = args.version
nusc = NuScenes(version=version, dataroot=root_path, verbose=True)
generate_data(nusc)
