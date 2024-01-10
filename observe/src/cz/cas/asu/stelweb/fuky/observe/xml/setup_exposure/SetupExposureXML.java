/*
 *
 *   Author: Jan Fuchs <fuky@asu.cas.cz>
 *
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2013 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

package cz.cas.asu.stelweb.fuky.observe.xml.setup_exposure;

import java.util.ArrayList;

import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement(name = "setup_exposure_list")
public class SetupExposureXML {
    private String instrument = "";
    private ArrayList<SetupExposure> setupExposureList;

    @XmlElement(name = "setup_exposure")
    public ArrayList<SetupExposure> getSetupExposureList() {
        return setupExposureList;
    }
    
    public void setSetupExposureList(ArrayList<SetupExposure> setupExposureList) {
        this.setupExposureList = setupExposureList;
    }
    
    public String getInstrument() {
        return instrument;
    }
    
    public void setInstrument(String instrument) {
        this.instrument = instrument;
    }
    
    public boolean isInstrument(String instrument) {
        return this.instrument.equalsIgnoreCase(instrument);
    }
}