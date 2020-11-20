#!/usr/bin/env python

import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-?', action='store_true', dest='help')
    args = parser.parse_args()
    if args.help:
        print('clang LLVM compiler')
