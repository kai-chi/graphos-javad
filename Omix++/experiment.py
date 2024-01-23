#!/usr/bin/python3
import os
import statistics
import subprocess

import numpy as np
import pandas as pd
from matplotlib import pyplot as plt, ticker

import commons

filename = os.path.basename(__file__)[:-3]
res_file = filename + '.csv'
img_file = filename + '.png'


def run(size, repetitions):
    print('Run with size = ' + str(size) + ' ' + str(repetitions) + ' times')
    initTimes = []
    readTimes = []
    writeTimes = []
    deleteTimes = []

    for i in range(repetitions):
        command = './app ' + str(size)
        # stdout = ''
        try:
            stdout = subprocess.check_output(command, cwd='./', shell=True, stderr=subprocess.DEVNULL) \
                .decode('utf-8')
        except subprocess.CalledProcessError as e:
            print("App error:\n", e.output)
            print(e.stdout)
            exit()
        print(stdout)
        for line in stdout.splitlines():
            if 'ORAM Initialization Time' in line:
                time = float(commons.escape_ansi(line.split(": ", 1)[1]))
                initTimes.append(time)
            elif 'Average OMAP Read Time' in line:
                time = float(commons.escape_ansi(line.split(": ", 1)[1]))
                readTimes.append(time)
            elif 'Average OMAP Write Time' in line:
                time = float(commons.escape_ansi(line.split(": ", 1)[1]))
                writeTimes.append(time)
            elif 'Average OMAP Delete Time' in line:
                time = float(commons.escape_ansi(line.split(": ", 1)[1]))
                deleteTimes.append(time)

    initTime = statistics.median(initTimes)
    readTime = statistics.median(readTimes)
    writeTime = statistics.median(writeTimes)
    deleteTime = statistics.median(deleteTimes)
    result = str(size) + ',' + str(initTime) + ',' + str(readTime) + ',' + str(writeTime) + ',' + str(deleteTime)
    print('Results: ' + result)
    f = open(res_file, 'a')
    f.write(result + '\n')
    f.close()

def plot(sizes):
    data = pd.read_csv(res_file)
    fig = plt.figure(figsize=(10,6))
    platforms = data['sgx'].unique()

    plots = ['initTime', 'readTime', 'writeTime', 'deleteTime']
    ylabels = ['Initialization Time(milliseconds)','Search Time(milliseconds)',
               'Insert Time(milliseconds)', 'Delete Time(milliseconds)']
    dataV1 = data[data['sgx'] == 'sgxv1']
    for i in range(len(plots)):
        plt.subplot(2, 4, i+1)
        for algorithm in platforms:
            df = data[data['sgx'] == algorithm]
            omap_sizes = df['size']
            times = df[plots[i]] / 1000
            plt.plot(omap_sizes, times, label=algorithm, marker='o')

        # plt.title('Self-join stream comparison with batch=' + str(batch_size))
        plt.xlabel('OMAP Size')
        plt.ylabel(ylabels[i])
        # plt.xscale('log')
        plt.gca().set_xscale('log',basex=2)
        plt.yscale('log')
        plt.xticks(sizes[0::2])
        plt.legend()

    for i in range(len(plots)):
        plt.subplot(2, 4, i+1+4)
        for algorithm in ['sgxv2', 'sgxv2-epc']:
            df = data[data['sgx'] == algorithm]
            omap_sizes = df['size'].values
            df = df[df['size'].isin(omap_sizes)]
            times = df[plots[i]].values
            dataV1 = data[data['sgx'] == 'sgxv1']
            dataV1 = dataV1[dataV1['size'].isin(omap_sizes)]
            timesV1 = dataV1[plots[i]].values
            rel = times / timesV1
            plt.gca().set_xscale('log',basex=2)
            plt.xticks(sizes[0::2])
            plt.plot(omap_sizes, rel, label=algorithm, marker='o')
            plt.legend()
            plt.ylim([0, 1.1])
            plt.xlabel('OMAP Size')
            plt.ylabel('Time relative to SGXv1 ')


    commons.savefig(img_file)


if __name__ == '__main__':

    sizes = 256 * 2 ** np.arange(20)
    experimentFlag = 0
    compileFlag = 1
    plotFlag = 1
    repetitions = 1

    if experimentFlag:
        if compileFlag:
            commons.remove_file(res_file)
            commons.init_file(res_file, "size,initTime,readTime,writeTime,deleteTime\n")
            commons.compile_app()
        for size in sizes:
            run(size, repetitions)

    if plotFlag:
        plot(sizes)
