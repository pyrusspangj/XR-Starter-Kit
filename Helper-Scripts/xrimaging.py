import cv2
import numpy as np
import threading
import yaml
import os


#


K_KEY = 'K'
DIST_COEFFS_KEY = 'dist'


yml_file = None
K = None
dist_coeffs = None

new_K, roi = None, None


cores = os.cpu_count()


undistorted_images_global = []


def read_yml(yml_file_path):
    global yml_file, K, dist_coeffs

    try:
        with open(yml_file_path, 'r') as file:
            yml_file = yaml.safe_load(file)
            K = yml_file[K_KEY]
            dist_coeffs = yml_file[DIST_COEFFS_KEY]
    except FileNotFoundError:
        print("ERROR within 'xrimaging.py': file {} not found.".format(yml_file_path))


def scan_for_file(yml_file_path):
    global yml_file

    if yml_file is None:
        read_yml(yml_file_path)


def undistort(image, yml_file_path):
    global K, dist_coeffs, new_K, roi

    scan_for_file(yml_file_path)
    
    h, w, _ = image.shape

    if new_K is None:
        new_K, roi = cv2.getOptimalNewCameraMatrix(K, dist_coeffs, (w, h), 0, (w, h))

    undistorted = cv2.undistort(image, K, dist_coeffs, None, new_K)
    
    return undistorted


def undistort_n(image_arr, yml_file_path):
    images = []

    for image in image_arr:
        images.append(undistort(image, yml_file_path))

    return images


def undistort_n_opt(image_arr, yml_file_path):
    images = []

    def undistort_n_opt_worker(image_arr, yml_file_path):
        images += undistort_n(image_arr, yml_file_path)

    thread_count = min(len(image_arr), cores)
    images_per_thread = int(len(image_arr) / thread_count)
    threads = []

    for t in range(thread_count):
        start_pos = images_per_thread * t
        threads.append(threading.Thread(target=undistort_n_opt_worker, args=(image_arr[start_pos:min(start_pos + images_per_thread, len(image_arr))], yml_file_path)))
        threads[t].start()

    for thread in threads:
        thread.join()

    return images

