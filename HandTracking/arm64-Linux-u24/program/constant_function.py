import numpy as np
import math
import transforms3d


class Singleton(type):
	_instances = {}

	def __call__(cls, *args, **kwargs):
		if cls not in cls._instances:
			cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
		return cls._instances[cls]


class Debugger(metaclass=Singleton):
	def set_debug(self, debug):
		self.debug = debug

	def toggle(self):
		self.debug = not self.debug

	def get_debug(self):
		return self.debug


DEBUG = Debugger()
DEBUG.set_debug(False)


class PCF:
	def __init__(
		self,
		near=1,
		far=10000,
		frame_height=640,
		frame_width=480,
		fy=1074.520446598223,
	):

		self.near = near
		self.far = far
		self.frame_height = frame_height
		self.frame_width = frame_width
		self.fy = fy

		fov_y = 2 * np.arctan(frame_height / (2 * fy))
		# kDegreesToRadians = np.pi / 180.0 # never used
		height_at_near = 2 * near * np.tan(0.5 * fov_y)
		width_at_near = frame_width * height_at_near / frame_height

		self.fov_y = fov_y
		self.left = -0.5 * width_at_near
		self.right = 0.5 * width_at_near
		self.bottom = -0.5 * height_at_near
		self.top = 0.5 * height_at_near


canonical_metric_landmarks = np.array(
	[
		0.000000,
		-3.406404,
		5.979507,
		0.499977,
		0.652534,
		0.000000,
		-1.126865,
		7.475604,
		0.500026,
		0.547487,
		0.000000,
		-2.089024,
		6.058267,
		0.499974,
		0.602372,
		-0.463928,
		0.955357,
		6.633583,
		0.482113,
		0.471979,
		0.000000,
		-0.463170,
		7.586580,
		0.500151,
		0.527156,
		0.000000,
		0.365669,
		7.242870,
		0.499910,
		0.498253,
		0.000000,
		2.473255,
		5.788627,
		0.499523,
		0.401062,
		-4.253081,
		2.577646,
		3.279702,
		0.289712,
		0.380764,
		0.000000,
		4.019042,
		5.284764,
		0.499955,
		0.312398,
		0.000000,
		4.885979,
		5.385258,
		0.499987,
		0.269919,
		0.000000,
		8.261778,
		4.481535,
		0.500023,
		0.107050,
		0.000000,
		-3.706811,
		5.864924,
		0.500023,
		0.666234,
		0.000000,
		-3.918301,
		5.569430,
		0.500016,
		0.679224,
		0.000000,
		-3.994436,
		5.219482,
		0.500023,
		0.692348,
		0.000000,
		-4.542400,
		5.404754,
		0.499977,
		0.695278,
		0.000000,
		-4.745577,
		5.529457,
		0.499977,
		0.705934,
		0.000000,
		-5.019567,
		5.601448,
		0.499977,
		0.719385,
		0.000000,
		-5.365123,
		5.535441,
		0.499977,
		0.737019,
		0.000000,
		-6.149624,
		5.071372,
		0.499968,
		0.781371,
		0.000000,
		-1.501095,
		7.112196,
		0.499816,
		0.562981,
		-0.416106,
		-1.466449,
		6.447657,
		0.473773,
		0.573910,
	]
)
# print(len(canonical_metric_landmarks))
canonical_metric_landmarks = np.reshape(
	canonical_metric_landmarks, (canonical_metric_landmarks.shape[0] // 5, 5)
).T
canonical_metric_landmarks = canonical_metric_landmarks[:3, :]

procrustes_landmark_basis = [
	(4, 0.070909939706326),
	(8, 0.032100144773722),
	(12, 0.008446550928056),
	(16, 0.058724168688059),
	(20, 0.007667080033571),
]
landmark_weights = np.zeros((canonical_metric_landmarks.shape[1],))
for idx, weight in procrustes_landmark_basis:
	landmark_weights[idx] = weight


def calculate_rotation(list_landmarks, pcf):
	# landmarks will be list of landmarks
	#print(list_landmarks)

	list_landmarks = np.array(list_landmarks)
	landmarks = list_landmarks.T

	metric_landmarks, pose_transform_mat = get_metric_landmarks(
		landmarks.copy(), pcf
	)
	
	euler_angles = list(transforms3d.euler.mat2euler(pose_transform_mat))
	euler_angles = to_degrees_pyr(euler_angles[0], euler_angles[1], euler_angles[2])
	
	def better_roll(wrist, middle):
		dx = middle[0] - wrist[0]
		dy = middle[1] - wrist[1]
		
		roll = math.atan2(dy, dx)
		rolldeg = math.degrees(roll)
		
		rolldeg += 90
		return rolldeg
		
	euler_angles[0] = abs((euler_angles[0] - 150)) / 2
	euler_angles[1] -= 90
	euler_angles[2] = better_roll(list_landmarks[0], list_landmarks[9])
	
	print(euler_angles)

	return euler_angles


def cpp_compare(name, np_matrix):
	if DEBUG.get_debug():
		# reorder cpp matrix as memory alignment is not correct
		cpp_matrix = np.load(f"{name}_cpp.npy")
		rows, cols = cpp_matrix.shape
		cpp_matrix = np.split(np.reshape(cpp_matrix, -1), cols)
		cpp_matrix = np.stack(cpp_matrix, 1)

		# print(f"{name}:", np.sum(np.abs(cpp_matrix - np_matrix[:rows, :cols]) ** 2))
		# print()


def get_metric_landmarks(screen_landmarks, pcf):
	screen_landmarks = project_xy(screen_landmarks, pcf)
	depth_offset = np.mean(screen_landmarks[2, :])

	intermediate_landmarks = screen_landmarks.copy()
	intermediate_landmarks = change_handedness(intermediate_landmarks)
	first_iteration_scale = estimate_scale(intermediate_landmarks)

	intermediate_landmarks = screen_landmarks.copy()
	intermediate_landmarks = move_and_rescale_z(
		pcf, depth_offset, first_iteration_scale, intermediate_landmarks
	)
	intermediate_landmarks = unproject_xy(pcf, intermediate_landmarks)
	intermediate_landmarks = change_handedness(intermediate_landmarks)
	second_iteration_scale = estimate_scale(intermediate_landmarks)

	metric_landmarks = screen_landmarks.copy()
	total_scale = first_iteration_scale * second_iteration_scale
	metric_landmarks = move_and_rescale_z(
		pcf, depth_offset, total_scale, metric_landmarks
	)
	metric_landmarks = unproject_xy(pcf, metric_landmarks)
	metric_landmarks = change_handedness(metric_landmarks)

	pose_transform_mat = solve_weighted_orthogonal_problem(
		canonical_metric_landmarks, metric_landmarks, landmark_weights
	)
	cpp_compare("pose_transform_mat", pose_transform_mat)

	inv_pose_transform_mat = np.linalg.inv(pose_transform_mat)
	inv_pose_rotation = inv_pose_transform_mat[:3, :3]
	inv_pose_translation = inv_pose_transform_mat[:3, 3]

	metric_landmarks = (
		inv_pose_rotation @ metric_landmarks + inv_pose_translation[:, None]
	)

	return metric_landmarks, pose_transform_mat


def project_xy(landmarks, pcf):
	x_scale = pcf.right - pcf.left
	y_scale = pcf.top - pcf.bottom
	x_translation = pcf.left
	y_translation = pcf.bottom

	landmarks[1, :] = 1.0 - landmarks[1, :]

	landmarks = landmarks * np.array([[x_scale, y_scale, x_scale]]).T
	landmarks = landmarks + np.array([[x_translation, y_translation, 0]]).T

	return landmarks


def change_handedness(landmarks):
	landmarks[2, :] *= -1.0

	return landmarks


def move_and_rescale_z(pcf, depth_offset, scale, landmarks):
	landmarks[2, :] = (landmarks[2, :] - depth_offset + pcf.near) / scale

	return landmarks


def unproject_xy(pcf, landmarks):
	landmarks[0, :] = landmarks[0, :] * landmarks[2, :] / pcf.near
	landmarks[1, :] = landmarks[1, :] * landmarks[2, :] / pcf.near

	return landmarks


def estimate_scale(landmarks):
	transform_mat = solve_weighted_orthogonal_problem(
		canonical_metric_landmarks, landmarks, landmark_weights
	)

	return np.linalg.norm(transform_mat[:, 0])


def extract_square_root(point_weights):
	return np.sqrt(point_weights)


def solve_weighted_orthogonal_problem(source_points, target_points, point_weights):
	sqrt_weights = extract_square_root(point_weights)
	transform_mat = internal_solve_weighted_orthogonal_problem(
		source_points, target_points, sqrt_weights
	)
	return transform_mat


def internal_solve_weighted_orthogonal_problem(sources, targets, sqrt_weights):

	cpp_compare("sources", sources)
	cpp_compare("targets", targets)

	# tranposed(A_w).
	weighted_sources = sources * sqrt_weights[None, :]
	# tranposed(B_w).
	weighted_targets = targets * sqrt_weights[None, :]

	cpp_compare("weighted_sources", weighted_sources)
	cpp_compare("weighted_targets", weighted_targets)

	# w = tranposed(j_w) j_w.
	total_weight = np.sum(sqrt_weights * sqrt_weights)

	# Let C = (j_w tranposed(j_w)) / (tranposed(j_w) j_w).
	# Note that C = tranposed(C), hence (I - C) = tranposed(I - C).
	#
	# tranposed(A_w) C = tranposed(A_w) j_w tranposed(j_w) / w =
	# (tranposed(A_w) j_w) tranposed(j_w) / w = c_w tranposed(j_w),
	#
	# where c_w = tranposed(A_w) j_w / w is a k x 1 vector calculated here:
	twice_weighted_sources = weighted_sources * sqrt_weights[None, :]
	source_center_of_mass = np.sum(twice_weighted_sources, axis=1) / total_weight

	# tranposed((I - C) A_w) = tranposed(A_w) (I - C) =
	# tranposed(A_w) - tranposed(A_w) C = tranposed(A_w) - c_w tranposed(j_w).
	centered_weighted_sources = weighted_sources - np.matmul(
		source_center_of_mass[:, None], sqrt_weights[None, :]
	)
	cpp_compare("centered_weighted_sources", centered_weighted_sources)

	design_matrix = np.matmul(weighted_targets, centered_weighted_sources.T)
	cpp_compare("design_matrix", design_matrix)

	rotation = compute_optimal_rotation(design_matrix)

	scale = compute_optimal_scale(
		centered_weighted_sources, weighted_sources, weighted_targets, rotation
	)

	rotation_and_scale = scale * rotation

	pointwise_diffs = weighted_targets - np.matmul(rotation_and_scale, weighted_sources)
	cpp_compare("pointwise_diffs", pointwise_diffs)

	weighted_pointwise_diffs = pointwise_diffs * sqrt_weights[None, :]
	cpp_compare("weighted_pointwise_diffs", weighted_pointwise_diffs)

	translation = np.sum(weighted_pointwise_diffs, axis=1) / total_weight

	transform_mat = combine_transform_matrix(rotation_and_scale, translation)
	cpp_compare("transform_mat", transform_mat)

	return transform_mat


# except:
# pass


def compute_optimal_rotation(design_matrix):
	if np.linalg.norm(design_matrix) < 1e-9:
		print("Design matrix norm is too small!")

	u, _, vh = np.linalg.svd(design_matrix, full_matrices=True)

	postrotation = u
	prerotation = vh

	if np.linalg.det(postrotation) * np.linalg.det(prerotation) < 0:
		postrotation[:, 2] = -1 * postrotation[:, 2]

	cpp_compare("postrotation", postrotation)
	cpp_compare("prerotation", prerotation)

	rotation = np.matmul(postrotation, prerotation)

	cpp_compare("rotation", rotation)

	return rotation


def compute_optimal_scale(
	centered_weighted_sources, weighted_sources, weighted_targets, rotation
):
	rotated_centered_weighted_sources = np.matmul(rotation, centered_weighted_sources)

	numerator = np.sum(rotated_centered_weighted_sources * weighted_targets)
	denominator = np.sum(centered_weighted_sources * weighted_sources)

	if denominator < 1e-9:
		print("Scale expression denominator is too small!")
	if numerator / denominator < 1e-9:
		print("Scale is too small!")

	return numerator / denominator


def combine_transform_matrix(r_and_s, t):
	result = np.eye(4)
	result[:3, :3] = r_and_s
	result[:3, 3] = t
	return result


def to_degrees_pyr(pitch=0, yaw=0, roll=0):
	pitch = math.degrees(pitch)
	yaw = math.degrees(yaw)
	roll = math.degrees(roll)
	return [pitch, yaw, roll]

def round_to(num, to=5):
	return round(num / to) * to
