import cv2
import numpy as np
import mediapipe as mp

LANDMARKS = {
	"WRIST": 0, "THUMB_CMC": 1, "THUMB_MCP": 2, "THUMB_IP": 3, "THUMB_TIP": 4,
	"INDEX_FINGER_MCP": 5, "INDEX_FINGER_PIP": 6, "INDEX_FINGER_DIP": 7, "INDEX_FINGER_TIP": 8,
	"MIDDLE_FINGER_MCP": 9, "MIDDLE_FINGER_PIP": 10, "MIDDLE_FINGER_DIP": 11, "MIDDLE_FINGER_TIP": 12,
	"RING_FINGER_MCP": 13, "RING_FINGER_PIP": 14, "RING_FINGER_DIP": 15, "RING_FINGER_TIP": 16,
	"PINKY_MCP": 17, "PINKY_PIP": 18, "PINKY_DIP": 19, "PINKY_TIP": 20
}

mp_hands = mp.solutions.hands
hands = mp_hands.Hands(
	static_image_mode=False,
	max_num_hands=2,
	min_detection_confidence=0.2,
	min_tracking_confidence=0.2,
	model_complexity=0
)

W, H = 0, 0
left_label, right_label = 'Left', 'Right'

def calculate_landmarks(no: int, landmarks):
	global W, H
	lmlist = []
	sumx, sumy = 0, 0
	for lm in landmarks.landmark:
		cx, cy, z = int(lm.x * W), int(lm.y * H), lm.z
		lmlist.append([no, cx, cy, z])
		sumx += cx
		sumy += cy
	return lmlist, [sumx / 21, sumy / 21]

def process_image(img_arr):
	global left_label, right_label, W, H
	
	H, W, _ = img_arr.shape
	image_rgb = cv2.cvtColor(img_arr, cv2.COLOR_BGR2RGB)
	results = hands.process(image_rgb)
	
	if not results.multi_hand_landmarks:
		return (
			np.zeros((2,), dtype=np.float32),
			np.zeros((2,), dtype=np.float32),
			np.zeros((21, 4), dtype=np.int32),
			np.zeros((21, 4), dtype=np.int32),
			0, " ", " "
		)
	
	hand_landmarks = results.multi_hand_landmarks
	
	if len(hand_landmarks) == 2:
		landmarks1, com1 = calculate_landmarks(0, hand_landmarks[0])
		landmarks2, com2 = calculate_landmarks(1, hand_landmarks[1])
		
		if landmarks1[0][1] < landmarks2[0][1]:
			l_list, l_com = landmarks1, com1
			r_list, r_com = landmarks2, com2
		else:
			l_list, l_com = landmarks2, com2
			r_list, r_com = landmarks1, com1
		
		return (
			np.array(l_com, dtype=np.float32),
			np.array(r_com, dtype=np.float32),
			np.array(l_list, dtype=np.int32),
			np.array(r_list, dtype=np.int32),
			1, left_label, right_label
		)
	
	landmarks, com = calculate_landmarks(0, hand_landmarks[0])
	label = results.multi_handedness[0].classification[0].label
	hand_label = right_label if label == left_label else left_label
	
	return (
		np.zeros((2,), dtype=np.float32),
		np.array(com, dtype=np.float32),
		np.zeros((21, 4), dtype=np.int32),
		np.array(landmarks, dtype=np.int32),
		0, " ", hand_label
	)
	
