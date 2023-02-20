# Script to compute 3D point clouds from depth images donwloaded with recorder_console.py.
#

import argparse
import cv2
from glob import glob
import numpy as np
import os

from recorder_console import read_sensor_poses

# This script computes normals using 'open3d'. See instruction how to install:
#
#    > pip install open3d-python
#    or
#    > conda install -c open3d-admin open3d
#
import open3d

# Depth range for short throw and long throw, in meters (approximate)
SHORT_THROW_RANGE = [0.02, 3.]
LONG_THROW_RANGE = [1., 4.]


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
            x = us[i, j]
            y = vs[i, j]

            # Compute Z values as described in issue #63
            # https://github.com/Microsoft/HoloLensForCV/issues/63#issuecomment-429469425
            D = distance_img[i, j]
            z = - float(D) / np.sqrt(x*x + y*y + 1)

            if np.isinf(x) or np.isinf(y) or \
              D < depth_range[0] or D > depth_range[1]:
                continue

            # 3D point in camera coordinate system
            point = np.array([x, y, 1.]) * z

            # Camera to World
            point = np.dot(R, point) + t

            # Append point
            points.append(point)

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
    assert (os.path.exists(cam_folder))

    # Output folder
    output_folder = os.path.join(args.output_path, cam)
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    # Get camera projection info
    bin_path = os.path.join(args.workspace_path,
                            "%s_camera_space_projection.bin" % cam)

    # From frame to world coordinate system
    sensor_poses = None
    if not args.ignore_sensor_poses:
        sensor_poses = read_sensor_poses(
            os.path.join(folder, cam + ".csv"), identity_camera_to_image=True)

    # Get appropriate depth thresholds
    depth_range = LONG_THROW_RANGE if 'long' in cam else SHORT_THROW_RANGE

    # Get depth paths
    depth_paths = sorted(glob(os.path.join(cam_folder, "*pgm")))
    if args.max_num_frames == -1:
        args.max_num_frames = len(depth_paths)
    depth_paths = depth_paths[args.start_frame:(args.start_frame + args.
                                                max_num_frames)]

    # Process paths
    merge_points = args.merge_points
    overwrite    = args.overwrite
    use_cache    = args.use_cache
    points_merged = []
    normals_merged = []
    us = vs = None
    for i_path, path in enumerate(depth_paths):
        suffix = "_%s" % args.output_suffix if len(args.output_suffix) else ""
        pcloud_output_path = \
            os.path.join(output_folder,
                        os.path.basename(path).replace(
                                ".pgm","%s.ply" % suffix))
        print("Progress file (%d/%d): %s" %
              (i_path+1, len(depth_paths), pcloud_output_path))

        # if file exist
        output_file_exist = os.path.exists(pcloud_output_path)
        if output_file_exist and use_cache:
            pcd = open3d.read_point_cloud(pcloud_output_path)
        else:
            img = cv2.imread(path, -1)
            if us is None or vs is None:
                us, vs = parse_projection_bin(bin_path, img.shape[1], img.shape[0])
            cam2world = get_cam2world(path, sensor_poses) if sensor_poses is not None else None
            points = get_points(img, us, vs, cam2world, depth_range)

            # get camera center. The point '(0,0,0)' is the camera center
            t = cam2world[:3, 3]
            cam_center = t

            pcd = open3d.PointCloud()
            pcd.points = open3d.Vector3dVector(points)

            # compute normal of current point cloud (orientes towards the cam)
            open3d.estimate_normals(
                    pcd,
                    search_param =
                    # open3d.KDTreeSearchParamKNN(knn = 30))
                    open3d.KDTreeSearchParamHybrid(radius = 0.2, max_nn = 30))

            # orient normals
            open3d.orient_normals_towards_camera_location(pcd, cam_center)

        if merge_points:
            points_merged.extend(pcd.points)
            normals_merged.extend(pcd.normals)

        if not output_file_exist or overwrite:
            open3d.write_point_cloud(pcloud_output_path, pcd)

    return points_merged, normals_merged


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
    parser.add_argument("--merge_points",  action='store_true', default=False, help="Save file with all the points (in world coordinate system)")
    parser.add_argument("--use_cache", action='store_true', default=False, help="Load already existing files")
    parser.add_argument("--overwrite", action='store_true', default=False, help="Write output files (overwrite if exist).")
    parser.add_argument("--display", action='store_true', default=False, help="Display merged.")

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
    # read options
    args = parse_args()
    if args.short_throw:
        camera = 'short_throw_depth'
    if args.long_throw:
        camera = 'long_throw_depth'

    # force collecting all points when '--display' is set
    if args.display:
        args.merge_points = True

    # output file
    output_folder = os.path.join(args.output_path, camera)
    output_filename = output_folder + ".ply"
    output_file_exist = os.path.exists(output_filename)

    # process
    print("Processing '%s' depth folder..." % camera)
    if output_file_exist and args.use_cache:
        pcd = open3d.read_point_cloud(output_filename)
    else:
        points, normals = process_folder(args, camera)
        print('Done processing.')

        # save output
        if args.merge_points:
            # transform 'points' and 'normals' to Open3D format
            pcd = open3d.PointCloud()
            pcd.points  = open3d.Vector3dVector(points)
            pcd.normals = open3d.Vector3dVector(normals)

            print("Saving file with all points: %s" % output_filename)
            open3d.write_point_cloud(output_filename, pcd)

    # Display
    if args.display:
        open3d.draw_geometries([pcd],
            window_name=u"Point Cloud with normals (press '9' to see colors)")
    #
    # Display Options
    #
    #  -- Mouse view control --
    #    Left button + drag         : Rotate.
    #    Ctrl + left button + drag  : Translate.
    #    Wheel button + drag        : Translate.
    #    Shift + left button + drag : Roll.
    #    Wheel                      : Zoom in/out.
    #
    #  -- Keyboard view control --
    #    [/]          : Increase/decrease field of view.
    #    R            : Reset view point.
    #    Ctrl/Cmd + C : Copy current view status into the clipboard.
    #    Ctrl/Cmd + V : Paste view status from clipboard.
    #
    #  -- General control --
    #    Q, Esc       : Exit window.
    #    H            : Print help message.
    #    P, PrtScn    : Take a screen capture.
    #    D            : Take a depth capture.
    #    O            : Take a capture of current rendering settings.
    #
    #  -- Render mode control --
    #    L            : Turn on/off lighting.
    #    +/-          : Increase/decrease point size.
    #    Ctrl + +/-   : Increase/decrease width of geometry::LineSet.
    #    N            : Turn on/off point cloud normal rendering.
    #    S            : Toggle between mesh flat shading and smooth shading.
    #    W            : Turn on/off mesh wireframe.
    #    B            : Turn on/off back face rendering.
    #    I            : Turn on/off image zoom in interpolation.
    #    T            : Toggle among image render:
    #                   no stretch / keep ratio / freely stretch.
    #
    #  -- Color control --
    #    0..4,9       : Set point cloud color option.
    #                   0 - Default behavior, render point color.
    #                   1 - Render point color.
    #                   2 - x coordinate as color.
    #                   3 - y coordinate as color.
    #                   4 - z coordinate as color.
    #                   9 - normal as color.
    #    Ctrl + 0..4,9: Set mesh color option.
    #                   0 - Default behavior, render uniform gray color.
    #                   1 - Render point color.
    #                   2 - x coordinate as color.
    #                   3 - y coordinate as color.
    #                   4 - z coordinate as color.
    #                   9 - normal as color.
    #    Shift + 0..4 : Color map options.
    #                   0 - Gray scale color.
    #                   1 - JET color map.
    #                   2 - SUMMER color map.
    #                   3 - WINTER color map.
    #                   4 - HOT color map.

    print("Done.")


if __name__ == "__main__":
    main()
