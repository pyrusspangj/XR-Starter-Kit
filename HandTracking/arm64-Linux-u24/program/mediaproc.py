import cv2
import numpy as np
import mediapipe as mp
import json
import constant_function

# Hand landmarks
LANDMARKS = {
	"WRIST": 0, "THUMB_CMC": 1, "THUMB_MCP": 2, "THUMB_IP": 3, "THUMB_TIP": 4,
	"INDEX_FINGER_MCP": 5, "INDEX_FINGER_PIP": 6, "INDEX_FINGER_DIP": 7, "INDEX_FINGER_TIP": 8,
	"MIDDLE_FINGER_MCP": 9, "MIDDLE_FINGER_PIP": 10, "MIDDLE_FINGER_DIP": 11, "MIDDLE_FINGER_TIP": 12,
	"RING_FINGER_MCP": 13, "RING_FINGER_PIP": 14, "RING_FINGER_DIP": 15, "RING_FINGER_TIP": 16,
	"PINKY_MCP": 17, "PINKY_PIP": 18, "PINKY_DIP": 19, "PINKY_TIP": 20
}

# Mediapipe setup
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(static_image_mode=False, max_num_hands=2, min_detection_confidence=0.2, min_tracking_confidence=0.2, model_complexity=0)
pcf = constant_function.PCF()
W, H = 0, 0
left_label, right_label = 'Left', 'Right'

def calculate_landmarks(no: int, landmarks):
	global W, H
	lmlist = []  # landmark list
	lmraw = []
	sumx, sumy = 0, 0  # for calculating "center-of-mass"
	for lm in landmarks.landmark:
		cx, cy, z = int(lm.x * W), int(lm.y * H), lm.z
		lmlist.append([no, cx, cy, z])
		lmraw.append([lm.x, lm.y, z])
		sumx += cx
		sumy += cy
	return lmlist, [sumx / 21, sumy / 21], lmraw

def process_image(img_arr):
	global left_label, right_label, W, H, pcf

	def get_opposite_label(label: str):
		return right_label if label == left_label else left_label

	H, W, _ = img_arr.shape
	image_rgb = cv2.cvtColor(img_arr, cv2.COLOR_BGR2RGB)
	results = hands.process(image_rgb)

	if not results.multi_hand_landmarks:
		return json.dumps([[0, 0], [0, 0], [], [], 0, " ", " "])

	hand_landmarks = results.multi_hand_landmarks
	hand_count = len(hand_landmarks)

	if hand_count == 2:
		landmarks1, com1, lmraw1 = calculate_landmarks(0, hand_landmarks[0])
		landmarks2, com2, lmraw2 = calculate_landmarks(1, hand_landmarks[1])
		
		if landmarks1[0][1] < landmarks2[0][1]:
			left_list, left_com, lmrawL = landmarks1, com1, lmraw1
			right_list, right_com, lmrawR = landmarks2, com2, lmraw2
		else:
			left_list, left_com, lmrawL = landmarks2, com2, lmraw2
			right_list, right_com, lmrawR = landmarks1, com1, lmraw1
			
		return json.dumps([left_com, right_com, left_list, right_list, 1, left_label, right_label])
	else:
		hand_label = get_opposite_label(results.multi_handedness[0].classification[0].label)
		landmarks, com, lmraw = calculate_landmarks(0, hand_landmarks[0])
		
		return json.dumps([[0, 0], com, [], landmarks, 0, " ", hand_label])
						# l_com, r_com, l_list, r_list, both, l_label, r_label


def draw_landmarks(img, landmarks_list):
	for landmark in landmarks_list:
		_, cx, cy, _ = landmark
		cv2.circle(img, (cx, cy), 5, (0, 255, 0), -1)


if __name__ == "__main__":
	cap = cv2.VideoCapture(0)
	if not cap.isOpened():
		print("Error: Could not open one or both cameras.")
		exit()

	while True:
		ret, frame = cap.read()
		if not ret:
			print("Error: Failed to capture image.")
			break

		arr = np.array(frame)
		result = process_image(arr)
		result_json = json.loads(result)

		left_landmarks = result_json[2]
		right_landmarks = result_json[3]

		draw_landmarks(frame, left_landmarks)
		draw_landmarks(frame, right_landmarks)

		cv2.imshow('Frame', frame)
		if cv2.waitKey(1) == ord('q'):
			break

	cap.release()
	cv2.destroyAllWindows()
