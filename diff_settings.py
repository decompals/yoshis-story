#!/usr/bin/env python3

def apply(config, args):
    config['baseimg'] = 'baserom.z64'
    config['myimg'] = 'build/yoshisstory.z64'
    config['mapfile'] = 'build/yoshisstory.map'
    config['source_directories'] = ['./src','./include']
    config['objdump_flags'] = ['-M','reg-names=32']
    config['makeflags'] = []
