/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *
 *   Copyright (C) 2008-2011 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import cz.cas.asu.stelweb.fuky.observe.xml.SpectrographInfoXML;

import java.lang.NumberFormatException;
import java.util.HashMap;
import java.util.Map;
import java.awt.Color;
import javax.swing.JLabel;

public class Spectrograph {

    public enum Element {
        // BEGIN glst
        DICHROIC_MIRROR(0),
        SPECTRAL_FILTER(1),
        COLLIMATOR(2),
        FOCUS_700(3),
        FOCUS_400(4),
        STAR_CALIBRATION(5),
        COUDE_OES(6),
        FLAT(7),
        COMP(8),
        EXP_SHUTTER(9),
        CAM_700_SHUTTER(10),
        CAM_400_SHUTTER(11),
        GRATING(12),
        EXP(13),
        SLITH_CAM(14),
        CORR_700(15),
        CORR_400(16),
        NONE_01(17),
        NONE_02(18),
        NONE_03(19),
        COLLIMATOR_OES(20),
        FOCUS_OES(21),
        EXP_SHUTTER_OES(22),
        EXP_OES(23),
        NONE_04(24),
        IOD_FLASK(25),
        COUDE_SLIT_CAMERA_POWER(26),
        OES_SLIT_CAMERA_POWER(27),
        // END glst
        FOCUS_700_POS(28),
        FOCUS_400_POS(29),
        GRATING_POS(30),
        EXP_COUNT(31),
        EXP_FREQ(32),
        EXP_OES_COUNT(33),
        EXP_OES_FREQ(34),
        FOCUS_OES_POS(35),
        ELEMENT_COUNT(36);

        private int index;

        private Element(int index) {
            this.index = index;
        }

        public int getIndex() {
            return this.index;
        }

        public static int getCount() {
            return Element.ELEMENT_COUNT.getIndex();
        }
    }

    private SpectrographInfoXML spectrographInfoXML;
    private String[] values;
    // TODO: udelat globani barvy
    private Color red_color = new Color(255, 204, 204);
    private Color green_color = new Color(204, 255, 204);
    private Color yellow_color = new Color(255, 255, 51);

    public Spectrograph(SpectrographInfoXML spectrographInfoXML) {
        this.spectrographInfoXML = spectrographInfoXML;
        this.values = new String[Element.getCount()];

        String[] glst = spectrographInfoXML.getGlst().split(" ");

        for (int i = 0; i < glst.length; ++i) {
            if (i == Element.getCount()) {
                break;
            }

            this.values[i] = glst[i];
        }

        this.values[Element.FOCUS_700_POS.getIndex()] = spectrographInfoXML.getSpgp_4();
        this.values[Element.FOCUS_400_POS.getIndex()] = spectrographInfoXML.getSpgp_5();
        this.values[Element.GRATING_POS.getIndex()]   = spectrographInfoXML.getSpgp_13();
        this.values[Element.EXP_COUNT.getIndex()]     = spectrographInfoXML.getSpce_14();
        this.values[Element.EXP_FREQ.getIndex()]      = spectrographInfoXML.getSpfe_14();
        this.values[Element.EXP_OES_COUNT.getIndex()] = spectrographInfoXML.getSpce_24();
        this.values[Element.EXP_OES_FREQ.getIndex()]  = spectrographInfoXML.getSpfe_24();
        this.values[Element.FOCUS_OES_POS.getIndex()] = spectrographInfoXML.getSpgp_22();
    }

    private void setValueColor(Map<String, Object> map, int stop, int moving, int timeout) {
        int value = Integer.valueOf((String)map.get("value"));

        if (value == stop) {
            map.put("value", "stop");
            map.put("color", red_color);
        }
        else if (value == moving) {
            map.put("value", "moving");
            map.put("color", red_color);
        }
        else if (value == timeout) {
            map.put("value", "timeout");
            map.put("color", red_color);
        }
    }

    private String gratingPos2Angle(String value) {
        int pos = Integer.valueOf(value);
        double angle = -0.00487106 * pos + 61.7024;
        double degree;
        double minute;

        degree = Math.floor(angle);
        minute = Math.round((angle - degree) * 60);
        
        if (minute == 60) {
            ++degree;
            minute = 0;
        }

        return String.format("%02.0f:%02.0f", degree, minute);
    }

    public String getValue(Element element) {
        return values[element.getIndex()];   
    }
    
    public void setJLabel(JLabel jLabel, Element element) {
        String value = this.values[element.getIndex()];
        Map<String, Object> map = new HashMap<String, Object>();
        map.put("value", value);
        map.put("color", green_color);

        switch (element) {
            case COLLIMATOR:
            case COLLIMATOR_OES:
                String collimator_states[] = {
                    "stop",
                    "open",
                    "closed",
                    "open left",
                    "open right",
                    "moving",
                    "timeout",
                };

                try {
                    int number = Integer.parseInt(value);

                    if (number != 1) {
                        map.put("color", red_color);
                    }

                    value = String.format("%s - %s", value, collimator_states[number]);
                    map.put("value", value);
                }
                catch (Exception e) {
                    map.put("color", red_color);
                }

                break;

            case DICHROIC_MIRROR:
                setValueColor(map, 0, 5, 6);
                break;

            case SLITH_CAM:
            case SPECTRAL_FILTER:
                setValueColor(map, 0, 6, 7);
                break;

            case FOCUS_700:
            case FOCUS_400:
            case FOCUS_OES:
                if (value.equals("0")) {
                    map.put("value", "stop");
                }
                else {
                    setValueColor(map, -1, 1, -1);
                }
                break;

            case COUDE_OES:
                if (value.equals("1")) {
                    map.put("value", "Coude");
                }
                else if (value.equals("2")) {
                    map.put("value", "OES");
                }
                else {
                    setValueColor(map, 0, 3, 4);
                }
                break;

            case STAR_CALIBRATION:
                if (value.equals("1")) {
                    map.put("value", "Star");
                }
                else if (value.equals("2")) {
                    map.put("value", "Calibration");
                }
                else {
                    setValueColor(map, 0, 3, 4);
                }
                break;

            case EXP_SHUTTER:
            case EXP_SHUTTER_OES:
            case CAM_700_SHUTTER:
            case CAM_400_SHUTTER:
                if (value.equals("1")) {
                    map.put("value", "open");
                }
                else if (value.equals("2")) {
                    map.put("value", "closed");
                }
                else {
                    setValueColor(map, 0, 3, 4);
                }
                break;

            case COMP:
            case FLAT:
                if (value.equals("0")) {
                    map.put("value", "off");
                }
                else {
                    map.put("value", "on");
                    map.put("color", yellow_color);
                }
                break;
                
            case COUDE_SLIT_CAMERA_POWER:
            case OES_SLIT_CAMERA_POWER:
                if (value.equals("0")) {
                    map.put("value", "off");
                    map.put("color", red_color);
                }
                else {
                    map.put("value", "on");
                }
                break;

            case GRATING:
                if (value.equals("0")) {
                    map.put("value", "stop");
                }
                else {
                    setValueColor(map, -1, 1, 2);
                }
                break;

            case EXP:
            case EXP_OES:
                if (value.equals("0")) {
                    map.put("value", "stop");
                    map.put("color", red_color);
                }
                else {
                    map.put("value", "recording");
                    map.put("color", green_color);
                }
                break;


            case CORR_700:
            case CORR_400:
                if (value.equals("0")) {
                    map.put("value", "unknown");
                    map.put("color", red_color);
                }
                else if (value.equals("1")) {
                    map.put("value", "in");
                    map.put("color", green_color);
                }
                else if (value.equals("2")) {
                    map.put("value", "out");
                    map.put("color", green_color);
                }
                break;

            case IOD_FLASK:
                if (value.equals("1")) {
                    map.put("value", "on");
                    map.put("color", yellow_color);
                }
                else if (value.equals("2")) {
                    map.put("value", "off");
                }
                else {
                    setValueColor(map, 0, 3, 4);
                }
                break;

            case GRATING_POS:
                jLabel.setText(gratingPos2Angle((String)map.get("value")));
                jLabel.setToolTipText((String)map.get("value"));
                jLabel.setBackground((Color)map.get("color"));
                return;

            case EXP_OES_COUNT:
            case EXP_COUNT:
                double count = Double.parseDouble(value);
                
                if (count >= 1000000.0) {
                    value = String.format("%.3f Mcounts", count / 1000000.0);
                } else if (count >= 1000.0) {
                    value = String.format("%.3f Kcounts", count / 1000.0);
                } else {
                    value = String.format("%.0f counts", count);
                }

                map.put("value", value);
                break;
                
            case FOCUS_700_POS:
            case FOCUS_400_POS:
            case EXP_FREQ:
            case EXP_OES_FREQ:
            case FOCUS_OES_POS:
            default:
                break;
        }

        jLabel.setText((String)map.get("value"));
        jLabel.setBackground((Color)map.get("color"));
    }
}
