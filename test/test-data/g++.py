#!/usr/bin/env python

import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog='gcc')
    parser.add_argument('--version', action='store_true')
    args = parser.parse_args()
    if args.version:
        print('g++ 1.0\nCopyright (C) 2019 Free Software Foundation, Inc')
