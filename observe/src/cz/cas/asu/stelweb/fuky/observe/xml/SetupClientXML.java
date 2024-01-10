/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date: 2011-02-21 13:13:10 +0100 (Mon, 21 Feb 2011) $
 *   $Rev: 274 $
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

package cz.cas.asu.stelweb.fuky.observe.xml;

import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlElement;

@XmlRootElement(name="client")
public class SetupClientXML {
    private Integer versionActual;
    private Integer versionMin;
    
    public SetupClientXML() {
    }
    
    @XmlElement(name="version_actual")
    public Integer getVersionActual() {
        return versionActual;
    }

    public void setVersionActual(Integer versionActual) {
        this.versionActual = versionActual;
    }
    
    @XmlElement(name="version_min")
    public Integer getVersionMin() {
        return versionMin;
    }

    public void setVersionMin(Integer versionMin) {
        this.versionMin = versionMin;
    }
}
