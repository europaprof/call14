#!/usr/bin/env python3
"""Build a v6 alignment reference from one already-cropped still image."""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('image', type=Path, help='sharp display crop (same size as live crop)')
    parser.add_argument('output', type=Path, help='output alignment JSON')
    args = parser.parse_args()
    gray = np.asarray(Image.open(args.image).convert('L'))
    if gray.shape[0] < 77 or gray.shape[1] < 111:
        parser.error('crop must be at least 111x77 pixels (passenger reference is 120x95)')
    args.output.write_text(json.dumps({'version': 1, 'gray': gray.tolist()}))
    print(f'wrote {args.output} from {args.image} ({gray.shape[1]}x{gray.shape[0]})')


if __name__ == '__main__':
    main()
