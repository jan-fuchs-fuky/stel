/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *
 *   Copyright (C) 2010 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import cz.cas.asu.stelweb.fuky.observe.xml.SpectrographExecuteXML;

public class SpectrographExecuteCB extends CallBack {
    private static final String INSTRUMENT = "Spectrograph";

    public SpectrographExecuteCB(String command) {
        super("spectrograph_execute", new Object[]{ command });
    }

    public SpectrographExecuteCB(String functionName, Object[] functionParams) {
        super(functionName, functionParams);
    }

    public String run(JerseyClientSSL client) throws NotOkStatusException {
        SpectrographExecuteXML spectrographExecuteXML = client.putSpectrographExecuteXML(
            this.functionName, this.functionParams);

        return spectrographExecuteXML.getResult();
    }

    public String getInstrument() {
        return this.INSTRUMENT;
    }
}
