#!/usr/bin/env python3
import os
import sys

from PIL import Image


def main() -> None:
    directory = sys.argv[1]
    frames = sorted(f for f in os.listdir(directory) if f.endswith(".ppm"))
    first = last = -1
    for index, name in enumerate(frames):
        path = os.path.join(directory, name)
        with Image.open(path) as image:
            if image.width <= image.height:
                continue
            image.transpose(Image.ROTATE_270).save(path)
        if first < 0:
            first = index
        last = index
    print(f"{first} {last} {len(frames)}")


if __name__ == "__main__":
    main()
