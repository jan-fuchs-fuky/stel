#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Author: Jan Fuchs <fuky@asu.cas.cz>
#
# $ ./import_fits.py -i -d ccd700.db -f /i/data2m/CCD700.RAW/2017/20170922/a201709220046.fit
# $ ./import_fits.py -d ccd700.db -p '/i/data2m/CCD700.RAW/20*/**/*.fit'
#
# $ ./import_fits.py -i -d oes.db -f /i/data2m/OES/2017/20170921/c201709210001.fit
# $ ./import_fits.py -d oes.db -p '/i/data2m/OES/20*/**/*.fit'
#

import sys
import argparse
import glob
import math
import sqlite3
import ephem
import traceback

from datetime import datetime, timedelta
from dateutil.relativedelta import relativedelta

class ImportFitsHeaders:

    def __init__(self, args, sqlite_cursor):
        self.args = args
        self.sqlite_cursor = sqlite_cursor

        self.observer = ephem.Observer()
        self.observer.lon = "14:46:48"
        self.observer.lat = "49:54:36"
        self.observer.elevation = 525
        self.observer.horizon = math.radians(-12)

        self.sun = ephem.Sun()

    def run_import(self):
        self.header_allowed_keys = []

        sql = "PRAGMA table_info(headers)"
        for row in self.sqlite_cursor.execute(sql):
            self.header_allowed_keys.append(row[1])

        for filename in glob.iglob(self.args.pathname, recursive=True):
            try:
                self.process_fits(filename)
                print("%s processed successfully" % filename)
            except:
                print("ERROR: %s" % filename, file=sys.stderr)
                traceback.print_exc()

    def process_fits(self, filename):
        fits_header = self.get_fits_header(filename, self.header_allowed_keys)

        cols = ", ".join(fits_header.keys())
        colon_cols = ":%s" % cols.replace(", ", ", :")
        sql = "INSERT INTO headers(%s) VALUES(%s)" % (cols, colon_cols)

        self.sqlite_cursor.execute(sql, fits_header)

    @classmethod
    def get_expose_start(cls, fits_header):
        minute, second = divmod(fits_header["TM_START"], 60)
        hour, minute = divmod(minute, 60)

        expose_start_str = "%sT%02i:%02i:%02i" % (fits_header["DATE_OBS"], hour, minute, second)

        expose_start_dt = datetime.strptime(expose_start_str, "%Y-%m-%dT%H:%M:%S")

        return expose_start_dt

    @classmethod
    def get_expose_end(cls, fits_header):
        expose_end_dt = cls.get_expose_start(fits_header) + timedelta(seconds=fits_header["EXPTIME"])

        return expose_end_dt

    @classmethod
    def get_fits_header(cls, filename, header_allowed_keys=None):
        data = []
        counter = 0
        with open(filename, "r", encoding="iso-8859-2") as fo:
            for item in iter(lambda: fo.read(80), ''):
                if (counter > 200) or (item.startswith("END")):
                    break
                data.append(item)
                counter += 1

        fits_header = {}
        for item in data:
            try:
                key, other = item.split('=', 1)
            except:
                continue

            try:
                value, comment = other.split('/', 1)
            except ValueError:
                value = other

            key = key.strip()
            value = value.strip("' ")

            # sqlite nepovoluje znak '-' v nazvu sloupecku
            key = key.replace('-', '_')

            # ignorujeme "zavadne" polozky v hlavicce
            if (key.startswith('!')):
                continue
            # ignoruje v hlavicce polozky, ktere nejsou soucasti tabulky
            elif (header_allowed_keys) and (key not in header_allowed_keys):
                continue

            for type_ in (int, float):
                try:
                    value = type_(value)
                    break
                except ValueError:
                    continue

            fits_header[key] = value

        fits_header["TIMESTAMP_START"] = cls.get_expose_start(fits_header).timestamp()
        fits_header["TIMESTAMP_END"] = cls.get_expose_end(fits_header).timestamp()

        return fits_header

    def init_sqlite_db(self, filename):
        fits_header = self.get_fits_header(filename)

        cols = []
        for key in fits_header.keys():
            if (isinstance(fits_header[key], str)):
                value_type = "TEXT"
            elif (isinstance(fits_header[key], int)):
                value_type = "INTEGER"
            elif (isinstance(fits_header[key], float)):
                value_type = "REAL"
            else:
                continue

            cols.append("%s %s" % (key, value_type))

        sql = "CREATE TABLE headers (%s)" % ", ".join(cols)

        self.sqlite_cursor.execute(sql)

    def get_exptime_sum(self, start_dt, stop_dt, conditions):
        observe_time_sum = 0
        exptime_sum = 0
        while (start_dt < stop_dt):
            start_dt += timedelta(days=1)
            dt_sun_set, dt_sun_rise = self.get_sun_set_rise(start_dt)
            observe_time = dt_sun_rise - dt_sun_set
            observe_time_sum += observe_time.total_seconds()

            #star_condition = " AND (IMAGETYP = 'object') AND ((OBJECT NOT LIKE '%dome%') OR (OBJECT NOT LIKE '%flat%'))"

            for condition in conditions:
                sql = "SELECT sum(EXPTIME) FROM headers WHERE (TIMESTAMP_START > ?) AND (TIMESTAMP_END < ?)%s" % condition

                #print(sql)
                #print(dt_sun_set, dt_sun_rise, start_dt)
                for row in self.sqlite_cursor.execute(sql, (dt_sun_set.timestamp(), dt_sun_rise.timestamp())):
                    exptime = row[0]
                    if (exptime):
                        exptime_sum += exptime
                        #print(exptime, "%.1f" % (exptime / 3600.0), start_dt)

        return [exptime_sum, observe_time_sum]

    def make_stats(self):
        #start_dt = datetime.strptime("2017-03-05 12:00:00", "%Y-%m-%d %H:%M:%S")
        #start_dt = datetime.strptime("2004-01-01 12:00:00", "%Y-%m-%d %H:%M:%S")
        #start_dt = datetime.strptime("2000-05-09 12:00:00", "%Y-%m-%d %H:%M:%S")

        #stop_dt = datetime.utcnow()

        star_condition = " AND (IMAGETYP = 'object') AND ((OBJECT NOT LIKE '%dome%') OR (OBJECT NOT LIKE '%flat%'))"

        calib_condition = (
            " AND ("
            "  ((IMAGETYP = 'object') AND ((OBJECT LIKE '%dome%') OR (OBJECT LIKE '%flat%'))) OR"
            "  (IMAGETYP != 'object')"
            ")"
        )

        missed_condition = " AND (IMAGETYP = 'object') AND ((OBJECT NOT LIKE '%dome%') OR (OBJECT NOT LIKE '%flat%'))"

        dt_format = "%Y-%m-%d %H:%M:%S"

        # WARNING: Je treba nastavit cas na 12:00, aby se generoval nasledujici
        # zapad Slunce, nasledovany vychodem Slunce a ne naopak
        #dt_pattern = "%i-01-01 12:00:00"
        start_dt = datetime.strptime("2003-12-01 12:00:00", dt_format)

        #for year in range(2004, 2018):
        while (True):
            start_dt += relativedelta(months=+1)
            stop_dt = start_dt + relativedelta(months=+1)

            #start_dt = datetime.strptime(dt_pattern % year, dt_format)
            #stop_dt = datetime.strptime(dt_pattern % (year+1), dt_format)

            if (stop_dt > datetime.now()):
                break

            #exptime_sum, observe_time_sum = self.get_exptime_sum(start_dt, stop_dt, [calib_condition])
            exptime_sum, observe_time_sum = self.get_exptime_sum(start_dt, stop_dt, [star_condition])

            # prevod vterin na hodiny
            exptime_sum = exptime_sum / 3600.0
            observe_time_sum = observe_time_sum / 3600.0
            percent = exptime_sum / (observe_time_sum / 100.0)
            print("%s %.1f%% exptime_sum = %.1f, observe_time_sum = %.1f" % (start_dt, percent, exptime_sum, observe_time_sum))

    def get_sun_set_rise(self, start_dt):
        self.observer.date = start_dt

        next_setting = self.observer.next_setting(self.sun, use_center=True)
        next_rising = self.observer.next_rising(self.sun, use_center=True)

        return [next_setting.datetime(), next_rising.datetime()]

def main():
    parser = argparse.ArgumentParser(
        description="Importing FITS headers.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        epilog="")

    parser.add_argument("-p", "--pathname", default="/i/data2m/OES/20*/**/*.fit", help="path to RAW data")
    #parser.add_argument("-p", "--pathname", default="/i/data2m/OES/2017/**/*.fit", help="path to RAW data")
    #parser.add_argument("-p", "--pathname", default="/i/data2m/OES/2017/**/20170914/*.fit", help="path to RAW data")
    parser.add_argument("-f", "--filename", default="/i/data2m/OES/2017/20170914/c201709140028.fit", help="path to FITS file")
    parser.add_argument("-d", "--db", default="fits_headers.db", help="path to database file")
    parser.add_argument("-i", "--init-db", action="store_true", help="initialize database")
    parser.add_argument("-m", "--make-stats", action="store_true", help="make statistics")

    args = parser.parse_args()

    try:
        sqlite_connection = sqlite3.connect(args.db)
        sqlite_cursor = sqlite_connection.cursor()

        import_fits_headers = ImportFitsHeaders(args, sqlite_cursor)

        if (args.init_db):
            import_fits_headers.init_sqlite_db(args.filename)
        elif (args.make_stats):
            import_fits_headers.make_stats()
        else:
            import_fits_headers.run_import()
    finally:
        sqlite_connection.commit()
        sqlite_connection.close()

if __name__ == '__main__':
    main()
