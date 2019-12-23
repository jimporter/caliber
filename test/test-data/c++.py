#!/usr/bin/env python

import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--version', action='store_true')
    args = parser.parse_args()
    if args.version:
        print('c++ 1.0')
