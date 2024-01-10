#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import matplotlib
import matplotlib.dates
import matplotlib.pyplot as plt
import numpy
import argparse

from datetime import datetime

def load_data(filenames):
    filenames_len = len(filenames)
    data = {}

    x = [[] for i in range(12)]
    y = [[] for i in range(12)]

    for filename in filenames:
        fo = open(filename, "r")
        lines = fo.readlines()
        fo.close()

        for line in lines:
            date, time, percent, dummy1, dummy2, exptime_sum, dummy3, dummy4, observe_time_sum = line.split()
            percent = float(percent.strip("%"))
            exptime_sum = float(exptime_sum.strip(","))
            observe_time_sum = float(observe_time_sum)
            dt = datetime.strptime(date, "%Y-%m-%d")

            if (dt not in data):
                data[dt] = [exptime_sum, observe_time_sum, percent]
            else:
                data[dt][0] += exptime_sum
                data[dt][2] += percent

    #print(data)

    value_max = 0
    x = [[] for i in range(12)]
    y = [[] for i in range(12)]
    z = [[] for i in range(12)]
    p = [[] for i in range(12)]
    for key in sorted(data.keys()):
        # TODO: konfigurovatelne
        #if (key.year != 2015):
        #if (key.year != 2017):
        #if (key.year != 2018) and (key.year != 2017):
        #    continue
        #if (key.year == 2017) and (key.month <= 4):
        #    continue
        #if (key.year == 2018) and (key.month >= 5):
        #    continue

        exptime_sum = data[key][0]
        observe_time_sum = data[key][1]
        percent_sum = data[key][2]
        x[key.month-1].append(key)
        y[key.month-1].append(exptime_sum)
        z[key.month-1].append(observe_time_sum - exptime_sum)
        p[key.month-1].append(percent_sum)

        if (observe_time_sum > value_max):
            value_max = observe_time_sum

    #print(x, y, z)

    return [x, y, z, p, value_max]

def make_year_graph(filenames):
    fig, ax = plt.subplots()

    fig.set_size_inches(19, 11)

    #y_sum = []
    #counter = 0
    #for filename in filenames:
    #    fo = open(filename, "r")
    #    lines = fo.readlines()
    #    fo.close()

    #    if ("oes" in filename.lower()):
    #        label = "OES"
    #    elif ("ccd700" in filename.lower()):
    #        label = "CCD700"
    #    elif ("calib" in filename.lower()):
    #        label = "calibration"
    #    elif ("missed" in filename.lower()):
    #        label = "missed"
    #    else:
    #        label = "OES + CCD700"

    #    x = [[] for i in range(12)]
    #    y = [[] for i in range(12)]
    #    idx = 0
    #    month = 0
    #    for line in lines:
    #        #year, percent, dummy1, dummy2, exptime_sum, dummy3, dummy4, observe_time_sum = line.split()
    #        date, time, percent, dummy1, dummy2, exptime_sum, dummy3, dummy4, observe_time_sum = line.split()
    #        percent = float(percent.strip("%"))

    #        x[month].append(datetime.strptime(date, "%Y-%m-%d"))
    #        y[month].append(percent)

    #        if (counter == 0):
    #            y_sum.append(percent)
    #        else:
    #            y_sum[idx] += percent

    #        month += 1
    #        if (month == 12):
    #            month = 0

    #        idx += 1

    x, y, z, p, value_max = load_data(filenames)

    color = [
        '#0343df', '#75bbfd', '#95d0fc',
        '#15b01a', '#89fe05', '#96f97b',
        '#e50000', '#f97306', '#ff028d',
        '#fac205', '#d5b60a', '#bf9005',
    ]

    for month in range(0, 12):
        ax.bar(x[month], y[month], color=color[month], width=20)
        ax.bar(x[month], z[month], color="#cccccc", width=20, bottom=y[month])

        ax.text(x[month][0], y[month][0], "%.0f" % y[month][0], ha="center", va="bottom",
                bbox=dict(facecolor='yellow', alpha=1.0))

        ax.text(x[month][0], y[month][0] + z[month][0], "%.0f" % z[month][0], ha="center", va="bottom",
                bbox=dict(facecolor='white', alpha=1.0))

        ax.text(x[month][0], (y[month][0] + z[month][0]) / 1.25, "%.0f%%" % p[month][0], ha="center", va="bottom",
                bbox=dict(facecolor='#eecffe', alpha=1.0))

    ax.xaxis_date()
    #ax.xaxis.set_major_locator(matplotlib.dates.YearLocator())
    ax.xaxis.set_major_locator(matplotlib.dates.MonthLocator())
    #ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%Y"))
    ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%Y-%m"))
    ax.autoscale(tight=True)

    #ax.plot(x, y, marker='o', label=label)
    #ax.plot_date(x, y, marker='o', label=label)
    #ax.set_xticks(range(1, 13))
    #counter += 1

    #ax.plot(x, y_sum, marker='o', label="OES + CCD700")
    ax.legend()

    #ax.set_xticks(range(1, 13))
    #ax.set_xticklabels(labels)
 
    plt.title("100% = Sun position is under -12°")
    #plt.xlabel("Year")
    plt.xlabel("")
    #plt.ylabel("Stars Exptime [%]")
    plt.ylabel("Observing time (hrs.)")
    plt.grid(True, linestyle="--", linewidth=1)
    #plt.xticks(numpy.arange(x[0], x[-1]+1, 1))
    #plt.ylim(0, 100)
    plt.ylim(0, value_max * 1.1)
    #plt.show()

    #plt.savefig("/tmp/graph.png")
    plt.savefig("/tmp/graph.eps", format="eps", dpi=1200)

def make_month_graph():
    fig, ax = plt.subplots()
    
    x = numpy.arange(1, 13)
    step = 0.9 / 14
    for shift in numpy.arange(0, 0.9, step):
        ax.bar(x+shift, numpy.random.randint(1, 6000, size=12), width=step)
    
    ax.set_xticks(range(1, 13))
    #ax.set_xticklabels(labels)
    
    plt.show()

def main():
    matplotlib.rcParams.update({"font.size": 18})
    matplotlib.rcParams.update({"axes.linewidth": 2})

    parser = argparse.ArgumentParser(
        description="Make graph.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog="")

    parser.add_argument("-y", "--year", action="store_true", help="make year graph")
    parser.add_argument("-m", "--month", help="make month graph")
    parser.add_argument('files', metavar='FILE', type=str, nargs='+', help='an input data files')

    args = parser.parse_args()

    if (args.year):
        make_year_graph(args.files)
        #load_data(args.files)
    elif (args.month):
        make_month_graph()

if __name__ == '__main__':
    main()
