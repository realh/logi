#!/usr/bin/python

#   logi - A DVB DVR designed for web-based clients.
#   Copyright (C) 2012, 2017 Tony Houghton <h@realh.co.uk>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


""" Loads one of freesat.t1/.t2 from <http://www.rst38.org.uk/vdr/>
    and converts it into a C++ array for logi's Huffman decoder.
    Usage:
    
    huff2c.py INFILE OUTFILE
    
    where INFILE is freesat.t1/.t2. C++ variable name is derived from this.
"""

import errno
import os
import sys

START_TOK = 0
STOP_TOK = 0
ESC_TOK = 1

class TreeNode(object):
    def __init__(self, index):
        self.index = index
        self.left = 0
        self.right = 0
    
    def get_index(self):
        return self.index

def add_val_and_char(tree, index, val, char):
    """ tree is a dict of nodes keyed by (partial) strings of binary.
        index is index of next node to be inserted in tree.
        val is a string of binary (field 2 from the table file).
        char is the ascii code of the character
            (derived from field 3 of table file).
        Returns new index.
    """
    return add_node(tree, index, val, char | 128)

def add_node(tree, index, val, ooc):
    # ooc = offset_or_char; offset = index
    digit = int(val[-1])
    val = val[:-1]
    if not val:
        index_ = 0
        bin = ' '
    else:
        index_ = index
        bin = val
    if digit:
        right = ooc
        left = 0
    else:
        left = ooc
        right = 0
    node = tree.get(bin)
    if not node:
        node = TreeNode(index_)
        tree[bin] = node
        if index_:
            index += 1
        if val:
            index = add_node(tree, index, val, index_)
    if left:
        node.left = left
    if right:
        node.right = right
    return index
    
def parse_line(trees, line):
    " Trees is a dict of [tree, index] pairs, keyed by start char "
    if line[0] == '#' or not line or not line.strip():
        return
    line = line.split(':')
    char = line[0]
    if char == 'START':
        tree_index = START_TOK
    elif char == 'STOP':
        tree_index = STOP_TOK
    elif char == 'ESCAPE':
        tree_index = ESC_TOK
    elif char.startswith('0x'):
        tree_index = int(char[2:], 16)
    else:
        try:
            tree_index = ord(char)
        except:
            print "Problem char '%s'" % char
            raise
    tree_and_index = trees.get(tree_index)
    if tree_and_index == None:
        tree_and_index = [{}, 1]
        trees[tree_index] = tree_and_index
    val = line[1]
    char = line[2]
    if char == 'ESCAPE':
        char = ESC_TOK
    elif char == 'STOP':
        char = STOP_TOK
    elif char.startswith('0x'):
        char = int(char[2:], 16)
    else:
        try:
            char = ord(char)
        except:
            print "Problem char '%s'" % char
            raise
    tree_and_index[1] = add_val_and_char(tree_and_index[0],
            tree_and_index[1], val, char)
    
def load_file(filename):
    trees = {}
    fp = open(filename, 'r')
    for line in fp.readlines():
        parse_line(trees, line)
    fp.close()
    return trees

def generate_file(trees, oot):
    " oot = 1 or 2 "
    s = """/* This file was auto-generated for logi by huff2c.py */

#include "si/huffman.h"

namespace logi
{

"""
    keys = trees.keys()
    keys.sort()
    for k in keys:
        s += 'static HuffmanNode table%02x[] = {' % k
        col = 0
        nodes = trees[k][0].values()
        nodes.sort(key = TreeNode.get_index)
        if nodes[0].index != 0:
            s += '\n    {0x00, 0x00},'
            col = 1
        l = len(nodes)
        for n in range(l):
            node = nodes[n]
            if not col:
                s += '\n   '
            s += ' {0x%02x, 0x%02x}' % (node.left, node.right)
            if n < l - 1:
                s += ','
            col += 1
            if col == 4:
                col = 0
        s += '\n};\n\n'
    
    s += 'HuffmanNode *huffman_table%c[] = {' % oot
    col = 0
    for n in range(128):
        if not col:
            s += '\n   '
        if n in keys:
            s += 'table%02x' % n
        else:
            s += 'NULL   '
        if n < 127:
            s += ', '
        col += 1
        if col == 4:
            col = 0
    s += '\n};\n\n}\n'
    
    return s
        
def save_file(filename, body):
    fp = open(filename, 'w')
    fp.write(body)
    fp.close()


filename = sys.argv[1]
trees = load_file(filename)
body = generate_file(trees, filename[-1])
path = os.path.dirname(sys.argv[2])
try:
    os.makedirs(path)
except OSError as exc:
    if exc.errno == errno.EEXIST and os.path.isdir(path):
        pass
    else:
        raise
save_file(sys.argv[2], body)

