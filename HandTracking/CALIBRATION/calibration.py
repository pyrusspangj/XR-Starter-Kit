import cv2
import numpy as np
import tkinter as tk
import threading

CHECKERBOARD = (9, 6)
SQUARE_SIZE = 0.025		# meters

captured_frames = []
objpoints = []
imgpoints = []

def generate_sparse_corner_targets():
	rows = CHECKERBOARD[1]
	cols = CHECKERBOARD[0]
	
	row_indices = [0, rows // 2, rows - 1]
	col_indices = [0, cols // 2, cols - 1]
	
	targets = []
	for r in row_indices:
		for c in col_indices:
			targets.append((r, c))
	
	return targets


def draw_checkerboard():
	root = tk.Tk()
	root.title("Checkerboard")
	size = 50
	canvas = tk.Canvas(root, width=CHECKERBOARD[0]*size, height=CHECKERBOARD[1]*size)
	canvas.pack()
	
	for i in range(CHECKERBOARD[1]):
		for j in range(CHECKERBOARD[0]):
			if (i + j) % 2 == 0:
				canvas.create_rectangle(j*size, i*size, (j+1)*size, (i+1)*size, fill="black")
	root.mainloop()
	

def capture_mode():
	cap = cv2.VideoCapture(0)
	if not cap.isOpened():
		pass
