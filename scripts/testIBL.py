import numpy as np
import imageio.v2 as imageio
import os
import random

SIZE = 1024
NUM_SPOTS_MIN = 5
NUM_SPOTS_MAX = 15

BACKGROUND_LUMINANCE = 0.01
SPOT_INTENSITY_MIN = 10.0
SPOT_INTENSITY_MAX = 50.0
SPOT_RADIUS_MIN = 10
SPOT_RADIUS_MAX = 40

OUTPUT_DIR = "../textures/IBL/cubemap_test/origin"

FACES = ["px", "nx", "py", "ny", "pz", "nz"]

def add_gaussian_spot(img, cx, cy, radius, intensity, color):
    h, w, _ = img.shape
    y, x = np.ogrid[:h, :w]
    dx = x - cx
    dy = y - cy
    dist2 = dx * dx + dy * dy

    sigma2 = radius * radius
    gaussian = np.exp(-dist2 / (2 * sigma2))

    for c in range(3):
        img[:, :, c] += gaussian * intensity * color[c]


def tonemap_reinhard(hdr):
    """Reinhard tone mapping + gamma"""
    mapped = hdr / (1.0 + hdr)
    mapped = np.clip(mapped, 0.0, 1.0)
    mapped = np.power(mapped, 1.0 / 2.2)
    return mapped


def generate_face():
    img = np.ones((SIZE, SIZE, 3), dtype=np.float32) * BACKGROUND_LUMINANCE

    num_spots = random.randint(NUM_SPOTS_MIN, NUM_SPOTS_MAX)

    for _ in range(num_spots):
        cx = random.randint(0, SIZE - 1)
        cy = random.randint(0, SIZE - 1)
        radius = random.randint(SPOT_RADIUS_MIN, SPOT_RADIUS_MAX)
        intensity = random.uniform(SPOT_INTENSITY_MIN, SPOT_INTENSITY_MAX)

        color = np.array([
            random.uniform(0.8, 1.0),
            random.uniform(0.8, 1.0),
            random.uniform(0.8, 1.0)
        ], dtype=np.float32)

        add_gaussian_spot(img, cx, cy, radius, intensity, color)

    return img


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    for face in FACES:
        hdr_img = generate_face()

        hdr_path = os.path.join(OUTPUT_DIR, f"{face}.hdr")
        imageio.imwrite(hdr_path, hdr_img)

        ldr_img = tonemap_reinhard(hdr_img)
        ldr_path = os.path.join(OUTPUT_DIR, f"{face}_tonemap.png")
        imageio.imwrite(ldr_path, (ldr_img * 255).astype(np.uint8))

        print(f"Saved: {hdr_path}")
        print(f"Saved: {ldr_path}")

    print("\nDone. HDR + tonemapped previews generated.")


if __name__ == "__main__":
    main()
