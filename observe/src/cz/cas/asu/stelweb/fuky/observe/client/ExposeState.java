/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2011 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import cz.cas.asu.stelweb.fuky.observe.xml.ExposeInfoXML;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

class ExposeState {
    final String READOUT_SPEEDS_OK = "+OK READOUT_SPEEDS = ";
    final String GAINS_OK = "+OK GAINS = ";
    final String REGEX_GET_KEY = "^\\+OK \\w+ = ([^/]+) /.*$";
    final Pattern PATTERN_GET_KEY = Pattern.compile(REGEX_GET_KEY);


    String readoutSpeeds;
    String gains;
    String object;
    String target;
    ExposeInfoXML exposeInfoXML;

    public ExposeState(ExposeInfoXML exposeInfoXML) {
        this.exposeInfoXML = exposeInfoXML;
    }

    private String getKey(String answer) {
        Matcher matcher = PATTERN_GET_KEY.matcher(answer);

        if (matcher.find()) {
            return matcher.group(1);
        }

        return "";
    }

    public String getReadoutSpeeds() {
        return this.readoutSpeeds;
    }

    public void setReadoutSpeeds(String readoutSpeeds) {
        if (readoutSpeeds.startsWith(READOUT_SPEEDS_OK)) {
            this.readoutSpeeds = readoutSpeeds.substring(READOUT_SPEEDS_OK.length());
        }
    }
    
    public String getGains() {
        return gains;
    }

    public void setGains(String gains) {
        if (gains.startsWith(GAINS_OK)) {
            this.gains = gains.substring(GAINS_OK.length());
        }
    }

    public String getObject() {
        return this.object;
    }

    public void setObject(String object) {
        this.object = getKey(object);
    }

    public String getTarget() {
        return this.target;
    }

    public void setTarget(String target) {
        this.target = getKey(target);
    }

    public ExposeInfoXML getExposeInfoXML() {
        return this.exposeInfoXML;
    }

    public boolean isInstrument(String instrument) {
        return this.exposeInfoXML.isInstrument(instrument);
    }
}
