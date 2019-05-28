# Author: Johannes L. Schoenberger (jsch at inf.ethz.ch)

# Script to list, download, delete, and reconstruct sensor recordings from the
# HoloLens using COLMAP. COLMAP is an open-source image-based 3D reconstruction
# pipeline and, in this example, we demonstrate sparse and dense reconstruction
# using COLMAP from the visible light cameras only.
#
# This script can be used independent of COLMAP and can be used to manage
# recordings on the HoloLens. If you want to perform reconstruction using
# COLMAP, you must download or compile COLMAP separately from:
# https://colmap.github.io/.

import os
import sys
import glob
import tarfile
import argparse
import sqlite3
import shutil
import json
import subprocess
import urllib.request
import numpy as np


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dev_portal_address", default="127.0.0.1:10080",
                        help="The IP address for the HoloLens Device Portal")
    parser.add_argument("--dev_portal_username", required=True,
                        help="The username for the HoloLens Device Portal")
    parser.add_argument("--dev_portal_password", required=True,
                        help="The password for the HoloLens Device Portal")

    parser.add_argument("--workspace_path", required=True,
                        help="Path to workspace folder used for downloading "
                             "recordings and reconstruction using COLMAP")
    parser.add_argument("--colmap_path", help="Path to COLMAP.bat executable")

    parser.add_argument("--ref_camera_name", default="vlc_ll")
    parser.add_argument("--frame_rate", type=int, default=5)
    parser.add_argument("--start_frame", type=int, default=-1)
    parser.add_argument("--max_num_frames", type=int, default=-1)
    parser.add_argument("--num_refinements", type=int, default=3)

    args = parser.parse_args()

    return args


def mkdir_if_not_exists(path):
    if not os.path.exists(path):
        os.makedirs(path)


def rotmat2qvec(rotmat):
    Rxx, Ryx, Rzx, Rxy, Ryy, Rzy, Rxz, Ryz, Rzz = rotmat.flat
    K = np.array([
        [Rxx - Ryy - Rzz, 0, 0, 0],
        [Ryx + Rxy, Ryy - Rxx - Rzz, 0, 0],
        [Rzx + Rxz, Rzy + Ryz, Rzz - Rxx - Ryy, 0],
        [Ryz - Rzy, Rzx - Rxz, Rxy - Ryx, Rxx + Ryy + Rzz]]) / 3.0
    eigvals, eigvecs = np.linalg.eigh(K)
    qvec = eigvecs[[3, 0, 1, 2], np.argmax(eigvals)]
    if qvec[0] < 0:
        qvec *= -1
    return qvec


class DevicePortalBrowser(object):

    def connect(self, address, username, password):
        print("Connecting to HoloLens Device Portal...")
        self.url = "http://{}".format(address)
        password_manager = urllib.request.HTTPPasswordMgrWithDefaultRealm()
        password_manager.add_password(None, self.url, username, password)
        handler = urllib.request.HTTPBasicAuthHandler(password_manager)
        opener = urllib.request.build_opener(handler)
        opener.open(self.url)
        urllib.request.install_opener(opener)

        print("=> Connected to HoloLens at address:", self.url)

        print("Searching for CV: Recorder application...")

        response = urllib.request.urlopen(
            "{}/api/app/packagemanager/packages".format(self.url))
        packages = json.loads(response.read().decode())

        self.package_full_name = None
        for package in packages["InstalledPackages"]:
            if package["Name"] == "CV: Recorder":
                self.package_full_name = package["PackageFullName"]
                break
        assert self.package_full_name is not None, \
            "CV: Recorder package must be installed on HoloLens"

        print("=> Found CV: Recorder application with name:",
              self.package_full_name)

        print("Searching for recordings...")

        response = urllib.request.urlopen(
            "{}/api/filesystem/apps/files?knownfolderid="
            "LocalAppData&packagefullname={}&path=\\\\TempState".format(
                self.url, self.package_full_name))
        recordings = json.loads(response.read().decode())

        self.recording_names = []
        for recording in recordings["Items"]:
            # Check if the recording contains any file data.
            response = urllib.request.urlopen(
                "{}/api/filesystem/apps/files?knownfolderid="
                "LocalAppData&packagefullname={}&path=\\\\TempState\\{}".format(
                    self.url, self.package_full_name, recording["Id"]))
            files = json.loads(response.read().decode())
            if len(files["Items"]) > 0:
                self.recording_names.append(recording["Id"])
        self.recording_names.sort()

        print("=> Found a total of {} recordings".format(
              len(self.recording_names)))

    def list_recordings(self, verbose=True):
        for i, recording_name in enumerate(self.recording_names):
            print("[{: 6d}]  {}".format(i, recording_name))

        if len(self.recording_names) == 0:
            print("=> No recordings found on device")

    def get_recording_name(self, recording_idx):
        try:
            return self.recording_names[recording_idx]
        except IndexError:
            print("=> Recording does not exist")

    def download_recording(self, recording_idx, workspace_path):
        recording_name = self.get_recording_name(recording_idx)
        if recording_name is None:
            return

        recording_path = os.path.join(workspace_path, recording_name)
        mkdir_if_not_exists(recording_path)

        print("Downloading recording {}...".format(recording_name))

        response = urllib.request.urlopen(
            "{}/api/filesystem/apps/files?knownfolderid="
            "LocalAppData&packagefullname={}&path=\\\\TempState\\{}".format(
                self.url, self.package_full_name, recording_name))
        files = json.loads(response.read().decode())

        for file in files["Items"]:
            if file["Type"] != 32:
                continue

            destination_path = os.path.join(recording_path, file["Id"])
            if os.path.exists(destination_path):
                print("=> Skipping, already downloaded:", file["Id"])
                continue

            print("=> Downloading:", file["Id"])
            urllib.request.urlretrieve(
                "{}/api/filesystem/apps/file?knownfolderid=LocalAppData&" \
                "packagefullname={}&filename=\\\\TempState\\{}\\{}".format(
                    self.url, self.package_full_name,
                    recording_name, file["Id"]), destination_path)

    def delete_recording(self, recording_idx):
        recording_name = self.get_recording_name(recording_idx)
        if recording_name is None:
            return

        print("Deleting recording {}...".format(recording_name))

        response = urllib.request.urlopen(
            "{}/api/filesystem/apps/files?knownfolderid="
            "LocalAppData&packagefullname={}&path=\\\\TempState\\{}".format(
                self.url, self.package_full_name, recording_name))
        files = json.loads(response.read().decode())

        for file in files["Items"]:
            if file["Type"] != 32:
                continue

            print("=> Deleting:", file["Id"])
            urllib.request.urlopen(urllib.request.Request(
                "{}/api/filesystem/apps/file?knownfolderid=LocalAppData&" \
                "packagefullname={}&filename=\\\\TempState\\{}\\{}".format(
                    self.url, self.package_full_name,
                    recording_name, file["Id"]), method="DELETE"))

        self.recording_names.remove(recording_name)


def read_sensor_poses(path, identity_camera_to_image=False):
    poses = {}
    with open(path, "r") as fid:
        header = fid.readline()
        for line in fid:
            line = line.strip()
            if not line:
                continue
            elems = line.split(",")
            assert len(elems) == 50
            time_stamp = int(elems[0])
            # Compose the absolute camera pose from the two relative
            # camera poses provided by the recorder application.
            # The absolute camera pose defines the transformation from
            # the world to the camera coordinate system.
            frame_to_origin = np.array(list(map(float, elems[2:18])))
            frame_to_origin = frame_to_origin.reshape(4, 4).T
            camera_to_frame = np.array(list(map(float, elems[18:34])))
            camera_to_frame = camera_to_frame.reshape(4, 4).T
            if abs(np.linalg.det(frame_to_origin[:3, :3]) - 1) < 0.01:
                if identity_camera_to_image:
                    camera_to_image = np.eye(4)
                else:
                    camera_to_image = np.array(
                        [[1, 0, 0, 0], [0, -1, 0, 0], [0, 0, -1, 0], [0, 0, 0, 1]])
                poses[time_stamp] = np.dot(
                    camera_to_image,
                    np.dot(camera_to_frame, np.linalg.inv(frame_to_origin)))
    return poses


def read_sensor_images(recording_path, camera_name):
    image_poses = read_sensor_poses(os.path.join(
        recording_path, camera_name + ".csv"))

    image_paths = sorted(glob.glob(
        os.path.join(recording_path, camera_name, "*.pgm")))

    paths = []
    names = []
    time_stamps = []
    poses = []

    for image_path in image_paths:
        basename = os.path.basename(image_path)
        name = os.path.join(camera_name, basename).replace("\\", "/")
        time_stamp = int(os.path.splitext(basename)[0])
        if time_stamp in image_poses:
            paths.append(image_path)
            names.append(name)
            time_stamps.append(time_stamp)
            poses.append(image_poses[time_stamp])

    return paths, names, np.array(time_stamps), poses


def synchronize_sensor_frames(args, recording_path, output_path, camera_names):
    # Collect all sensor frames.

    images = {}
    for camera_name in camera_names:
        images[camera_name] = read_sensor_images(recording_path, camera_name)

    # Synchronize the frames based on their time stamps.

    ref_image_paths, ref_image_names, ref_time_stamps, ref_image_poses = \
        images[args.ref_camera_name]
    ref_time_diffs = np.diff(ref_time_stamps)
    assert np.all(ref_time_stamps >= 0)

    time_per_frame = 10**7 / 30.0
    time_per_frame_sampled = 30.0 / args.frame_rate * time_per_frame

    ref_image_paths_sampled = []
    ref_image_names_sampled = []
    ref_time_stamps_sampled = []
    ref_image_poses_sampled = []
    ref_prev_time_stamp = ref_time_stamps[0]
    for i in range(1, len(ref_time_stamps)):
        if ref_time_stamps[i] - ref_prev_time_stamp >= time_per_frame_sampled:
            ref_image_paths_sampled.append(ref_image_paths[i])
            ref_image_names_sampled.append(ref_image_names[i])
            ref_time_stamps_sampled.append(ref_time_stamps[i])
            ref_image_poses_sampled.append(ref_image_poses[i])
            ref_prev_time_stamp = ref_time_stamps[i]
    ref_image_paths, ref_image_names, ref_time_stamps, ref_image_poses = \
        (ref_image_paths_sampled, ref_image_names_sampled,
         ref_time_stamps_sampled, ref_image_poses_sampled)

    if args.max_num_frames > 0:
        assert args.start_frame < len(ref_image_paths)
        end_frame = min(len(ref_image_paths),
                        args.start_frame + args.max_num_frames)
        ref_image_paths = ref_image_paths[args.start_frame:end_frame]
        ref_image_names = ref_image_names[args.start_frame:end_frame]
        ref_time_stamps = ref_time_stamps[args.start_frame:end_frame]

    sync_image_paths = {}
    sync_image_names = {}
    sync_image_poses = {}
    for image_path, image_name, image_pose in zip(ref_image_paths,
                                                  ref_image_names,
                                                  ref_image_poses):
        sync_image_paths[image_name] = [image_path]
        sync_image_names[image_name] = [image_name]
        sync_image_poses[image_name] = [image_pose]

    max_sync_time_diff = time_per_frame / 5
    for camera_name, (image_paths, image_names, time_stamps, image_poses) \
            in images.items():
        if camera_name == args.ref_camera_name:
            continue
        for image_path, image_name, time_stamp, image_pose in \
                zip(image_paths, image_names, time_stamps, image_poses):
            time_diffs = np.abs(time_stamp - ref_time_stamps)
            min_time_diff_idx = np.argmin(time_diffs)
            min_time_diff = time_diffs[min_time_diff_idx]
            if min_time_diff < max_sync_time_diff:
                sync_ref_image_name = ref_image_names[min_time_diff_idx]
                sync_image_paths[sync_ref_image_name].append(image_path)
                sync_image_names[sync_ref_image_name].append(image_name)
                sync_image_poses[sync_ref_image_name].append(image_pose)

    # Copy the frames to the output directory.

    for camera_name in camera_names:
        mkdir_if_not_exists(os.path.join(output_path, camera_name))

    sync_frames = []
    sync_poses = []
    for ref_image_name, ref_time_stamp in zip(ref_image_names, ref_time_stamps):
        image_basename = "{}.pgm".format(ref_time_stamp)
        frame_images = []
        frame_poses = []
        for image_path, image_name, image_pose in \
                zip(sync_image_paths[ref_image_name],
                    sync_image_names[ref_image_name],
                    sync_image_poses[ref_image_name]):
            if len(sync_image_paths[ref_image_name]) == 4:
                camera_name = os.path.dirname(image_name)
                new_image_path = os.path.join(
                    output_path, camera_name, image_basename)
                if not os.path.exists(new_image_path):
                    shutil.copyfile(image_path, new_image_path)
                new_image_name = os.path.join(camera_name, image_basename)
                frame_images.append(new_image_name.replace("\\", "/"))
                frame_poses.append(image_pose)
        sync_frames.append(frame_images)
        sync_poses.append(frame_poses)

    return sync_frames, sync_poses


def extract_recording(recording_path):
    print("Extracting recording data...")
    for file_name in glob.glob(os.path.join(recording_path, "*.tar")):
        print("=> Extracting tarfile:", file_name)
        tar = tarfile.open(file_name)
        tar.extractall(path=recording_path)
        tar.close()


def reconstruct_recording(args, recording_path, dense=True):
    reconstruction_path = os.path.join(recording_path, "reconstruction")
    database_path = os.path.join(reconstruction_path, "database.db")
    image_path = os.path.join(reconstruction_path, "images")
    image_list_path = os.path.join(reconstruction_path, "image_list.txt")
    sparse_colmap_path = os.path.join(reconstruction_path, "sparse_colmap")
    sparse_hololens_path = \
        os.path.join(reconstruction_path, "sparse_hololens")
    dense_path = os.path.join(reconstruction_path, "dense")
    rig_config_path = os.path.join(reconstruction_path, "rig_config.json")

    mkdir_if_not_exists(reconstruction_path)

    extract_recording(recording_path)

    camera_names = ("vlc_ll", "vlc_lf", "vlc_rf", "vlc_rr")

    print("Syncrhonizing sensor frames...")
    frames, poses = synchronize_sensor_frames(
        args, recording_path, image_path, camera_names)

    with open(image_list_path, "w") as fid:
        for frame in frames:
            for image_name in frame:
                fid.write("{}\n".format(image_name))

    subprocess.call([
        args.colmap_path, "feature_extractor",
        "--image_path", image_path,
        "--database_path", database_path,
        "--image_list_path", image_list_path,
    ])

    # These OpenCV camera model parameters were determined for a specific
    # HoloLens using the self-calibration capabilities of COLMAP.
    # The parameters should be sufficiently accurate as an initialization and
    # the parameters will be refined during the COLMAP reconstruction process.
    camera_model_id = 4
    camera_model_name = "OPENCV"
    camera_width = 640
    camera_height = 480
    camera_params = {
        "vlc_ll": "450.072070 450.274345 320 240 "
                  "-0.013211 0.012778 -0.002714 -0.003603",
        "vlc_lf": "448.189452 452.478090 320 240 "
                  "-0.009463 0.003013 -0.006169 -0.008975",
        "vlc_rf": "449.435779 453.332057 320 240 "
                  "-0.000305 -0.013207 0.003258 0.001051",
        "vlc_rr": "450.301002 450.244147 320 240 "
                  "-0.010926 0.008377 -0.003105 -0.004976",
    }

    mkdir_if_not_exists(sparse_hololens_path)

    cameras_file = open(os.path.join(sparse_hololens_path, "cameras.txt"), "w")
    images_file = open(os.path.join(sparse_hololens_path, "images.txt"), "w")
    points_file = open(os.path.join(sparse_hololens_path, "points3D.txt"), "w")

    connection = sqlite3.connect(database_path)
    cursor = connection.cursor()

    camera_ids = {}
    for camera_name in camera_names:
        camera_params_list = \
            list(map(float, camera_params[camera_name].split()))
        camera_params_float = np.array(camera_params_list, dtype=np.double)

        cursor.execute("INSERT INTO cameras"
            "(model, width, height, params, prior_focal_length) "
            "VALUES(?, ?, ?, ?, ?);",
            (camera_model_id, camera_width,
             camera_height, camera_params_float, 1))

        camera_id = cursor.lastrowid
        camera_ids[camera_name] = camera_id

        cursor.execute("UPDATE images SET camera_id=? "
                       "WHERE name LIKE '{}%';".format(camera_name),
                       (camera_id,))
        connection.commit()

        cameras_file.write("{} {} {} {} {}\n".format(
            camera_id, camera_model_name,
            camera_width, camera_height,
            camera_params[camera_name]))

    for image_names, image_poses in zip(frames, poses):
        for image_name, image_pose in zip(image_names, image_poses):
            camera_name = os.path.dirname(image_name)
            camera_id = camera_ids[camera_name]
            cursor.execute(
                "SELECT image_id FROM images WHERE name=?;", (image_name,))
            image_id = cursor.fetchone()[0]
            qvec = rotmat2qvec(image_pose[:3, :3])
            tvec = image_pose[:, 3]
            images_file.write("{} {} {} {} {} {} {} {} {} {}\n\n".format(
                image_id, qvec[0], qvec[1], qvec[2], qvec[3],
                tvec[0], tvec[1], tvec[2], camera_id, image_name
            ))

    connection.close()

    cameras_file.close()
    images_file.close()
    points_file.close()

    subprocess.call([
        args.colmap_path, "exhaustive_matcher",
        "--database_path", database_path,
        "--SiftMatching.guided_matching", "true",
    ])

    with open(rig_config_path, "w") as fid:
        fid.write("""[
  {{
    "ref_camera_id": {},
    "cameras":
    [
      {{
          "camera_id": {},
          "image_prefix": "vlc_ll"
      }},
      {{
          "camera_id": {},
          "image_prefix": "vlc_lf"
      }},
      {{
          "camera_id": {},
          "image_prefix": "vlc_rf"
      }},
      {{
          "camera_id": {},
          "image_prefix": "vlc_rr"
      }}
    ]
  }}
]""".format(camera_ids[args.ref_camera_name],
            camera_ids["vlc_ll"],
            camera_ids["vlc_lf"],
            camera_ids["vlc_rf"],
            camera_ids["vlc_rr"]))

    for i in range(args.num_refinements):
        if i == 0:
            sparse_input_path = sparse_hololens_path
        else:
            sparse_input_path = sparse_colmap_path + str(i - 1)

        sparse_output_path = sparse_colmap_path + str(i)

        mkdir_if_not_exists(sparse_output_path)

        subprocess.call([
            args.colmap_path, "point_triangulator",
            "--database_path", database_path,
            "--image_path", image_path,
            "--input_path", sparse_input_path,
            "--output_path", sparse_output_path,
        ])

        subprocess.call([
            args.colmap_path, "rig_bundle_adjuster",
            "--input_path", sparse_output_path,
            "--output_path", sparse_output_path,
            "--rig_config_path", rig_config_path,
            "--BundleAdjustment.max_num_iterations", str(25),
            "--BundleAdjustment.max_linear_solver_iterations", str(100),
        ])

    if not dense:
        return

    subprocess.call([
        args.colmap_path, "image_undistorter",
        "--image_path", image_path,
        "--input_path", sparse_output_path,
        "--output_path", dense_path,
    ])

    subprocess.call([
        args.colmap_path, "patch_match_stereo",
        "--workspace_path",  dense_path,
        "--PatchMatchStereo.geom_consistency", "0",
        "--PatchMatchStereo.min_triangulation_angle", "2",
    ])

    subprocess.call([
        args.colmap_path, "stereo_fusion",
        "--workspace_path",  dense_path,
        "--StereoFusion.min_num_pixels", "15",
        "--input_type", "photometric",
        "--output_path", os.path.join(dense_path, "fused.ply"),
    ])


def print_help():
    print("Available commands:")
    print("  help:                     Print this help message")
    print("  exit:                     Exit the console loop")
    print("  list:                     List all recordings")
    print("  list device:              List all recordings on the HoloLens")
    print("  list workspace:           List all recordings in the workspace")
    print("  download X:               Download recording X from the HoloLens")
    print("  delete X:                 Delete recording X from the HoloLens")
    print("  delete all:               Delete all recordings from the HoloLens")
    print("  extract X:                Extract recording X in the workspace")
    print("  reconstruct X:            Perform sparse and dense reconstruction "
                                      "of recording X in the workspace")
    print("  reconstruct sparse X:     Perform sparse reconstruction "
                                      "of recording X in the workspace")


def list_workspace_recordings(workspace_path):
    recording_names = sorted(os.listdir(workspace_path))
    for i, recording_name in enumerate(recording_names):
        print("[{: 6d}]  {}".format(i, recording_name))
    if len(recording_names) == 0:
        print("=> No recordings found in workspace")


def parse_command_and_index(command, num_commands=2):
    error_in_command = False
    sub_commands = command.split()
    if len(sub_commands) != num_commands:
        error_in_command = True
    else:
        try:
            index = int(sub_commands[-1])
        except:
            error_in_command = True
    if error_in_command:
        print("=> Invalid command, expected a recording index")
        return
    return index


def main():
    args = parse_args()

    mkdir_if_not_exists(args.workspace_path)

    dev_portal_browser = DevicePortalBrowser()
    dev_portal_browser.connect(args.dev_portal_address,
                               args.dev_portal_username,
                               args.dev_portal_password)

    print()
    print_help()
    print()

    while True:
        try:
            command = input(">>> ").strip().lower()
        except EOFError:
            break

        if command == "help":
            print_help()
        if command == "exit":
            break
        elif command == "list":
            print("Device recordings:")
            dev_portal_browser.list_recordings()
            print("Workspace recordings:")
            list_workspace_recordings(args.workspace_path)
        elif command == "list device":
            dev_portal_browser.list_recordings()
        elif command == "list workspace":
            list_workspace_recordings(args.workspace_path)
        elif command.startswith("download"):
            recording_idx = parse_command_and_index(command)
            if recording_idx is not None:
                dev_portal_browser.download_recording(
                    recording_idx, args.workspace_path)
        elif command.startswith("delete"):
            if command == "delete all":
                for _ in range(len(dev_portal_browser.recording_names)):
                    dev_portal_browser.delete_recording(0)
            else:
                recording_idx = parse_command_and_index(command)
                if recording_idx is not None:
                    dev_portal_browser.delete_recording(recording_idx)
        elif command.startswith("extract"):
            recording_idx = parse_command_and_index(command)
            if recording_idx is not None:
                try:
                    recording_names = sorted(os.listdir(args.workspace_path))
                    recording_name = recording_names[recording_idx]
                except IndexError:
                    print("=> Recording does not exist")
                else:
                    extract_recording(
                        os.path.join(args.workspace_path, recording_name))
        elif command.startswith("reconstruct"):
            if not args.colmap_path:
                print("=> Cannot reconstruct, "
                      "because path to COLMAP is not specified")
                continue
            if "sparse" in command:
                recording_idx = parse_command_and_index(command, num_commands=3)
            else:
                recording_idx = parse_command_and_index(command)
            if recording_idx is not None:
                try:
                    recording_names = sorted(os.listdir(args.workspace_path))
                    recording_name = recording_names[recording_idx]
                except IndexError:
                    print("=> Recording does not exist")
                else:
                    dense = "sparse" not in command
                    reconstruct_recording(
                        args, os.path.join(args.workspace_path, recording_name),
                        dense=dense)
        else:
            print("=> Command not found")
            print()
            print_help()


if __name__ == "__main__":
    main()
