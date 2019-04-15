# Script to compute 3D point clouds from depth images donwloaded with recorder_console.py.
#

import argparse
import cv2
from glob import glob
import numpy as np
import os

from recorder_console import read_sensor_poses


# Depth range for short throw and long throw, in meters (approximate)
SHORT_THROW_RANGE = [0.02, 3.]
LONG_THROW_RANGE = [1., 4.]


def save_obj(output_path, points):
    with open(output_path, 'w') as f:
        f.write("# OBJ file\n")
        for v in points:
            f.write("v %.4f %.4f %.4f\n" % (v[0], v[1], v[2]))


def parse_projection_bin(path, w, h):
    # See repo issue #63
    # Read binary file
    projection = np.fromfile(path, dtype=np.float32)
    x_list = [ projection[i] for i in range(0, len(projection), 2) ]
    y_list = [ projection[i] for i in range(1, len(projection), 2) ]
    
    u = np.asarray(x_list).reshape(w, h).T
    v = np.asarray(y_list).reshape(w, h).T

    return [u, v]


def pgm2distance(img, encoded=False):
    # See repo issue #19
    distance = np.zeros((img.shape))
    for y in range(img.shape[0]):
        for x in range(img.shape[1]):
            val = hex(img[y, x])
            val = str(val[2:]).zfill(4)        
            val = val[-2:] + val[:2]
            distance[y, x] = float(int(val, 16))/1000.0
    return distance


def get_points(img, us, vs, cam2world, depth_range):
    distance_img = pgm2distance(img, encoded=False)

    if cam2world is not None:
        R = cam2world[:3, :3]
        t = cam2world[:3, 3]
    else:
        R, t = np.eye(3), np.zeros(3)

    points = []
    for i in np.arange(distance_img.shape[0]):
        for j in np.arange(distance_img.shape[1]):
            z = distance_img[i, j]
            x = us[i, j]
            y = vs[i, j]
            if np.isinf(x) or np.isinf(y) or z < depth_range[0] or z > depth_range[1]:
                continue
            point = np.array([-x, -y, -1.])
            point /= np.linalg.norm(point)
            point = t + z * np.dot(R, point)
            points.append(point.reshape((1, -1)))    
    return np.vstack(points)


def get_cam2world(path, sensor_poses):
    time_stamp = int(os.path.splitext(os.path.basename(path))[0])
    world2cam = sensor_poses[time_stamp]    
    cam2world = np.linalg.inv(world2cam)
    return cam2world


def process_folder(args, cam):
    # Input folder
    folder = args.workspace_path
    cam_folder = os.path.join(folder, cam)
    assert(os.path.exists(cam_folder))
    # Output folder
    output_folder = os.path.join(args.output_path, cam)
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    # Get camera projection info
    bin_path = os.path.join(args.workspace_path, "%s_camera_space_projection.bin" % cam)
    
    # From frame to world coordinate system
    sensor_poses = None
    if not args.ignore_sensor_poses:
        sensor_poses = read_sensor_poses(os.path.join(folder, cam + ".csv"), identity_camera_to_image=True)        

    # Get appropriate depth thresholds
    depth_range = LONG_THROW_RANGE if 'long' in cam else SHORT_THROW_RANGE

    # Get depth paths
    depth_paths = sorted(glob(os.path.join(cam_folder, "*pgm")))
    if args.max_num_frames == -1:
        args.max_num_frames = len(depth_paths)
    depth_paths = depth_paths[args.start_frame:(args.start_frame + args.max_num_frames)]    

    us = vs = None
    # Process paths
    for i_path, path in enumerate(depth_paths):
        if (i_path % 10) == 0:
            print("Progress: %d/%d" % (i_path+1, len(depth_paths)))
        output_suffix = "_%s" % args.output_suffix if len(args.output_suffix) else ""
        pcloud_output_path = os.path.join(output_folder, os.path.basename(path).replace(".pgm", "%s.obj" % output_suffix))
        if os.path.exists(pcloud_output_path):
            continue
        img = cv2.imread(path, -1)
        if us is None or vs is None:
            us, vs = parse_projection_bin(bin_path, img.shape[1], img.shape[0])
        cam2world = get_cam2world(path, sensor_poses) if sensor_poses is not None else None
        points = get_points(img, us, vs, cam2world, depth_range)        
        save_obj(pcloud_output_path, points)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--workspace_path", required=True, help="Path to workspace folder used for downloading")
    parser.add_argument("--output_path", required=False, help="Path to output folder where to save the point clouds. By default, equal to output_path")
    parser.add_argument("--output_suffix", required=False, default="", help="If a suffix is specified, point clouds will be saved as [tstamp]_[suffix].obj")
    parser.add_argument("--short_throw", action='store_true', help="Extract point clouds from short throw frames")
    parser.add_argument("--long_throw", action='store_true', help="Extract point clouds from long throw frames")
    parser.add_argument("--ignore_sensor_poses", action='store_true', help="Drop HL pose information (point clouds will not be aligned to a common ref space)")
    parser.add_argument("--start_frame", type=int, default=0)
    parser.add_argument("--max_num_frames", type=int, default=-1)

    args = parser.parse_args()

    if (not args.short_throw) and (not args.long_throw):
        print("At least one between short_throw and long_throw must be set to true.\
                Please pass \"--short_throw\" and/or \"--long_throw\" as parameter.")
        exit()
    
    assert(os.path.exists(args.workspace_path))    
    if args.output_path is None:
        args.output_path = args.workspace_path

    return args


def main():
    args = parse_args()

    if args.short_throw:
        print('Processing short throw depth folder...')
        process_folder(args, 'short_throw_depth')
        print('done.')

    if args.long_throw:
        print('Processing long throw depth folder...')
        process_folder(args, 'long_throw_depth')
        print('done.')


if __name__ == "__main__":    
    main()    
