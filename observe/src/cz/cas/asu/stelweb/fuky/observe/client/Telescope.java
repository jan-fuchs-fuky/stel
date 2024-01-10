/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *
 *   Copyright (C) 2008-2010 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
 *
 *   This file is part of Observe (Observing System for Ondrejov).
 *
 *   Observe is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Observe is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Observe.  If not, see <http://www.gnu.org/licenses/>.
 */

package cz.cas.asu.stelweb.fuky.observe.client;

public class Telescope {
    public static final int OIL_INDEX = 0;
    public static final int TELESCOPE_INDEX = 1;
    public static final int HA_INDEX = 2;
    public static final int DEC_INDEX = 3;
    public static final int SHARPING_INDEX = 4;
    public static final int DOME_INDEX = 5;
    public static final int SLITH_INDEX = 6;
    public static final int TUBE_INDEX = 7;
    public static final int SPECULUM_INDEX = 8;
    public static final int STATE_INDEX = 9;

    public static final String[][] OIL = {
        { "UNKNOWN",   "unknown" },
        { "OFF",       "off" },
        { "START1",    "checking presure nitrogen" },
        { "START2",    "starting pumps" },
        { "START3",    "stabilization presure" },
        { "ON",        "on" },
        { "OFF_DELAY", "delay before oil off" },
    };

    public static final String[][] TELESCOPE = {
        { "UNKNOWN",  "unknown" },
        { "INIT",     "initialization sensors" },
        { "OFF",      "off" },
        { "OFF_WAIT", "waiting on start converters" },
        { "STOP",     "stop telescope" },
        { "TRACK",    "tracking" },
        { "OFF_REQ",  "delay before off" },
        { "SS_CLU1",  "connector" },
        { "SS_SLEW",  "aproach on source coordinate" },
        { "SS_DECC2", "connector" },
        { "SS_CLU2",  "connector" },
        { "SS_DECC3", "braking after halt aproach" },
        { "SS_CLU3",  "connector after halt aproach" },
        { "ST_DECC1", "braking before aproach on star coordinate" },
        { "ST_CLU1",  "connector" },
        { "ST_SLEW",  "aproach on star coordinate" },
        { "ST_DECC2", "braking" },
        { "ST_CLU2",  "connector" },
        { "ST_DECC3", "braking after halt aproach" },
        { "ST_CLU3",  "connector after halt aproach" },
    };

    public static final String[][] HA = {
        { "UNKNOWN",   "unknown" },
        { "STOP",      "stop" },
        { "POSITION",  "position" },
        { "CA_CLU1",   "connector before calibration" },
        { "CA_FAST",   "calibration roughly" },
        { "CA_FASTBR", "calibration roughly braking" },
        { "CA_CLU2",   "connector finely calibration" },
        { "CA_SLOW",   "calibration finely" },
        { "MO_BR",     "shift T1 braking" },
        { "MO_CLU1",   "shift T1 connector" },
        { "MO_FAST",   "shift T1" },
        { "MO_FASTBR", "shift T1 braking" },
        { "MO_CLU2",   "shift T1 connector" },
        { "MO_SLOW",   "shift T2" },
        { "MO_SLOWEST","shift T3" },
    };

    public static final String[][] DEC = {
        { "UNKNOWN",     "unknown" },
        { "STOP",        "stop" },
        { "POSITION",    "position" },
        { "CA_CLU1",     "connector before calibration" },
        { "CA_FAST",     "calibration roughly" },
        { "CA_FASTBR",   "calibration roughly braking" },
        { "CA_CLU2",     "connector finely calibration" },
        { "CA_SLOW",     "calibration finely" },
        { "MO_BR",       "shift T1 braking" },
        { "MO_CLU1",     "shift T1 connector" },
        { "MO_FAST",     "shift T1" },
        { "MO_FASTBR",   "shift T1 braking" },
        { "MO_CLU2",     "shift T1 connector" },
        { "MO_SLOW",     "shift T2" },
        { "MO_SLOWEST",  "shift T3" },
        { "CENM_SLOWBR", "manual centering, braking" },
        { "CENM_CLU3",   "manual centering, connector" },
        { "CENM_CEN",    "manual centering, centering" },
        { "CENM_BR",     "manual centering, braking" },
        { "CENM_CLU4",   "manual centering, connector" },
        { "CENA_SLOWBR", "automatic centering, braking" },
        { "CENA_CLU3",   "automatic centering, connector" },
        { "CENA_CEN",    "automatic centering, centering" },
        { "CENA_BR",     "automatic centering, braking" },
        { "CENA_CLU4",   "automatic centering, connector" },
    };

    public static final String[][] SHARPING = {
        { "UNKNOWN", "unknown" },
        { "OFF",     "off" },
        { "STOP",    "stop" },
        { "PLUS",    "manual shift +" },
        { "MINUS",   "manual shift -" },
        { "SLEW",    "aproach on position" },
        { "CAL1",    "roughly calibration" },
        { "CAL2",    "finely calibration" },
    };

    public static final String[][] DOME = {
        { "UNKNOWN",     "unknown" },
        { "OFF",         "off" },
        { "STOP",        "stop" },
        { "PLUS",        "manual shift +" },
        { "MINUS",       "manual shift -" },
        { "SLEW_PLUS",   "aproach on position +" },
        { "SLEW_MINUS",  "aproach on position -" },
        { "AUTO_STOP",   "automatic positioning, stop" },
        { "AUTO_PLUS",   "automatic positioning, aproach on position +" },
        { "AUTO_MINUS",  "automatic positioning, aproach on position -" },
        { "CALIBRATION", "calibration" },
    };

    public static final String[][] SLITH = {
        { "UNKNOWN", "unknown" },
        { "UNDEF",   "undefine" },
        { "OPENING", "opening" },
        { "CLOSING", "closing" },
        { "OPEN",    "open" },
        { "CLOSE",   "close" },
    };

    public static final String[][] TUBE = {
        { "UNKNOWN", "unknown" },
        { "UNDEF",   "undefine" },
        { "OPENING", "opening" },
        { "CLOSING", "closing" },
        { "OPEN",    "open" },
        { "CLOSE",   "close" },
    };

    public static final String[][] SPECULUM = {
        { "UNKNOWN", "unknown" },
        { "UNDEF",   "undefine" },
        { "OPENING", "opening" },
        { "CLOSING", "closing" },
        { "OPEN",    "open" },
        { "CLOSE",   "close" },
    };

    public String[] state_oil = new String[2];
    public String[] state_telescope = new String[2];
    public String[] state_ha = new String[2];
    public String[] state_dec = new String[2];
    public String[] state_sharping = new String[2];
    public String[] state_dome = new String[2];
    public String[] state_slith = new String[2];
    public String[] state_tube = new String[2];
    public String[] state_speculum = new String[2];

    public String ra = "unknown";
    public String dec = "unknown";
    public String position = "unknown";
    public String ha = "unknown";
    public String da = "unknown";
    public String utc = "unknown";
    public String lst = "unknown";
    public String hourAngle = "unknown";
    public String tsra_ra = "";
    public String tsra_dec = "";
    public String tsra_position = "";
    public String object;
    public double correctionsRa = Double.NaN;
    public double correctionsDec = Double.NaN;
    public double speedRa = Double.NaN;
    public double speedDec = Double.NaN;
    public double airmass = Double.NaN;
    public double altitude = Double.NaN;
    public double azimuth = Double.NaN;
    public double domeAzimuth = Double.NaN;
    public double domePosition = Double.NaN;
    public double sharping = Double.NaN;
    public int modelNumber = Integer.MAX_VALUE;

    public int state_bit;
    public int state_bit_ha;
    public int state_bit_dec;
    public int state_bit_dome;
    public int state_bit_sharping;
    public int state_bit_abberation;
    public int state_bit_precesion;
    public int state_bit_refraction;
    public int state_bit_model;
    public int state_bit_guide_mode;

    public void parse(String glst) {
        parseGLST(glst);
    }

    private void updateState(String[][] what, int index, String[] values, String[] state) {
        int value;

        value = Integer.valueOf(values[index]).intValue() + 1;
        if (value >= what.length) value = 0;

        state[0] = what[value][0];
        state[1] = what[value][1];
    }

    private void parseGLST(String glst) {
        String[] values;

        values = glst.split(" ");

        updateState(OIL, OIL_INDEX, values, state_oil);
        updateState(TELESCOPE, TELESCOPE_INDEX, values, state_telescope);
        updateState(HA, HA_INDEX, values, state_ha);
        updateState(DEC, DEC_INDEX, values, state_dec);
        updateState(SHARPING, SHARPING_INDEX, values, state_sharping);
        updateState(DOME, DOME_INDEX, values, state_dome);
        updateState(SLITH, SLITH_INDEX, values, state_slith);
        updateState(TUBE, TUBE_INDEX, values, state_tube);
        updateState(SPECULUM, SPECULUM_INDEX, values, state_speculum);

        state_bit = Integer.valueOf(values[STATE_INDEX]).intValue();

        state_bit_ha = state_bit & 1;
        state_bit_dec = (state_bit >> 1) & 1;
        state_bit_dome = (state_bit >> 2) & 1;
        state_bit_sharping = (state_bit >> 3) & 1;
        state_bit_abberation = (state_bit >> 4) & 1;
        state_bit_precesion = (state_bit >> 5) & 1;
        state_bit_refraction = (state_bit >> 6) & 1;
        state_bit_model = (state_bit >> 7) & 1;
        state_bit_guide_mode = (state_bit >> 8) & 1;
    }
}
