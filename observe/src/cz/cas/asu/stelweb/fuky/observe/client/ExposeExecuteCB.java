/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *
 *   Copyright (C) 2010-2011 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import cz.cas.asu.stelweb.fuky.observe.xml.ExposeExecuteXML;

public class ExposeExecuteCB extends CallBack {
    private String instrument;

    public ExposeExecuteCB(String instrument, String functionName, Object[] functionParams) {
        super(functionName, functionParams);

        this.instrument = instrument;
    }

    public String run(JerseyClientSSL client) throws NotOkStatusException {
        ExposeExecuteXML exposeExecuteXML = client.putExposeExecuteXML(this.instrument, this.functionName, this.functionParams);

        return exposeExecuteXML.getResult();
    }

    public String getInstrument() {
        return this.instrument;
    }
}
