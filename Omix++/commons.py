import csv
import errno
import getopt
import os
import re
import subprocess
import sys
from timeit import default_timer as timer
from datetime import timedelta
import matplotlib.pyplot as plt
from collections import namedtuple

import pandas as pd
import yaml

PROG = ' ./app '

def start_timer():
    return timer()


def stop_timer(start):
    print("Execution time: " + str(timedelta(seconds=(timer()-start))))


def escape_ansi(line):
    ansi_escape = re.compile(r'(\x9B|\x1B\[)[0-?]*[ -\/]*[@-~]')
    return ansi_escape.sub('', line)


def compile_app(flags=None, config=None, debug=False, additional_vars=[]):
    if flags is None:
        flags = []
    print("Make clean")
    subprocess.check_output(["make", "clean"], cwd="./")

    cflags = 'CFLAGS='

    for flag in flags:
        cflags += ' -D' + flag + ' '

    if config is None:
        enclave_string = ' CONFIG=Enclave/Enclave.config.xml '
    else:
        enclave_string = ' CONFIG=' + str(config) + ' '

    print("Make enclave " + enclave_string + " with flags " + cflags)
    if debug:
        args = ["make", 'SGX_DEBUG=1', 'SGX_PRERELEASE=0',
                enclave_string, cflags] + additional_vars
        subprocess.check_output(args, cwd="./", stderr=subprocess.DEVNULL)
    else:
        args = ["make", 'SGX_DEBUG=0', 'SGX_PRERELEASE=1', enclave_string, cflags] + additional_vars
        subprocess.check_output(args, cwd="./", stderr=subprocess.DEVNULL)


def remove_file(fname):
    try:
        os.remove(fname)
    except FileNotFoundError:
        pass
    except OSError as e:
        raise

def init_file(fname, first_line):
    print(first_line)
    with open(fname, "w") as file:
        file.write(first_line)

##############
# colors:
#
# 30s
# mint: bcd9be
# yellow brock road: e5cf3c
# poppy field: b33122
# powder blue: 9fb0c7
# egyptian blue: 455d95
# jade: 51725b
#
# 20s
# tuscan sun: e3b22b
# antique gold: a39244
# champagne: decd9d
# cadmium red: ad2726
# ultramarine: 393c99
# deco silver: a5a99f
#
# 40s
# war bond blue: 30356b
# keep calm & canary on: e9ce33
# rosie's nail polish: b90d08
# air force blue: 6d9eba
# nomandy sand: d1bc90
# cadet khaki: a57f6a
#
# 80s
# acid wash: 3b6bc6
# purple rain: c77dd4
# miami: fd69a5
# pacman: f9e840
# tron turqouise: 28e7e1
# powersuit: fd2455
#
# 2010s
# millennial pink: eeb5be
# polished copper: c66d51
# quartz: dcdcdc
# one million likes: fc3e70
# succulent: 39d36e
# social bubble: 0084f9
#############


def color_categorical(i):
    colors = {
        0:'#0fb5ae',
        1:'#4046ca',
        2:'#f68511',
        3:'#de3d82',
        4:'#7e84fa',
        5:'#72e06a',
        6:'#147af3',
        7:'#7326d3',
        8:'#e8c600',
        9:'#cb5d00',
        10:'#008f5d',
        11:'#bce931',
    }
    return colors[i]


def color_alg(alg):
    # colors = {"CHT":"#e068a4", "PHT":"#829e58", "RHO":"#5ba0d0"}
    colors = {"CHT":"#b44b20",  # burnt sienna
              "PHT":"#7b8b3d",  # avocado
              "PSM":"#c7b186",  # natural
              "BHJ":"#c7b186",
              "RHT":"#885a20",  # teak
              "RHO":"#e6ab48",  # harvest gold
              "RSM":"#4c748a",  # blue mustang
              'TinyDB': '#0084f9',  # avocado
              "OJ":"#fd2455",
              "OPAQ":"#c77dd4",
              "OBLI":"#3b6bc6",
              "INL":'#7620b4',
              'NL':'#20b438',
              'MWAY':'#fd2455',
              # 'PaMeCo':'#28e7e1',
              'PaMeCo':'#bcd9be',
              'CRKJ':"#b3346c", # moody mauve
              "CRKJS":"#deeppink",
              'CrkJ':"#b3346c", # moody mauve
              'CrkJoin':"#b3346c", # moody mauve
              'CrkJoin+Skew':"#7620b4", # moody mauve
              'GHJ':'black',
              'RHO_atomic':'deeppink',
              # 'MCJoin':'#0084f9', # social bubble
              'MCJoin':'#51725b', # social bubble
              'CRKJ_CHT':'deeppink',
              'oldCRKJ':"#7b8b3d", # avocado
              'CRKJ_static':"#4c748a", # blue mustang

              'RHO-sgx': '#e6ab48',
              'RHO-sgx-affinity':'g',
              'RHO-lockless':'deeppink',
              'RHO_atomic-sgx':'deeppink'}
    # colors = {"CHT":"g", "PHT":"deeppink", "RHO":"dodgerblue"}
    return colors[alg]


def marker_alg(alg):
    markers = {
        "CHT": 'o',
        "PHT": 'v',
        "PSM": 'P',
        "RHT": 's',
        "RHO": 'X',
        "RSM": 'D',
        "INL": '^',
        'MWAY': '*',
        'RHO_atomic': 'P',
        'CRKJ':'>',
        'CrkJoin':'>',
        'CrkJoin+Skew':'D',
        'CRKJoin':'>',
        'CRKJS':'*',
        'CRKJ_CHT':'^',
        'oldCRKJ':'X',
        'CRKJ_static': 'D',
        "MCJoin": 'o',
        'PaMeCo': 'P',
    }
    return markers[alg]


def savefig(filename, font_size=15, tight_layout=True):
    plt.rcParams.update({'font.size': font_size})
    if tight_layout:
        plt.tight_layout()
    plt.savefig(filename, transparent = False, bbox_inches = 'tight', pad_inches = 0.1, dpi=200)
    print("Saved image file: " + filename)


def draw_vertical_lines(plt, x_values, linestyle='--', color='#209bb4', linewidth=1):
    for x in x_values:
        plt.axvline(x=x, linestyle=linestyle, color=color, linewidth=linewidth)


def draw_horizontal_lines(plt, y_values, linestyle='--', color='#209bb4', linewidth=1):
    for y in y_values:
        plt.axhline(y=y, linestyle=linestyle, color=color, linewidth=linewidth)


def read_config(input_argv, yaml_file='config.yaml'):
    with open(yaml_file, 'r') as file:
        config = yaml.safe_load(file)
    argv = input_argv[1:]
    try:
        opts, args = getopt.getopt(argv, 'r:',['reps='])
    except getopt.GetoptError:
        print('Unknown argument')
        sys.exit(1)
    for opt, arg in opts:
        if opt in ('-r', '--reps'):
            config['reps'] = int(arg)
    return config