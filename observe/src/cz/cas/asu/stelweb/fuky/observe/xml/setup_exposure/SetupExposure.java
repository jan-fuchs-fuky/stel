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

import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement(name = "setup_exposure")
public class SetupExposure {

    private String name;
    private String ga;
    private String range;
    private Integer dm;
    private Integer sf;
    private Integer flat;
    private Integer comp;

    public SetupExposure() {
        name = "";
        ga = "";
        range = "";
        dm = 0;
        sf = 0;
        flat = 0;
        comp = 0;
    }
    
    @XmlElement(name = "name")
    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    @XmlElement(name = "ga")
    public String getGa() {
        return ga;
    }

    public void setGa(String ga) {
        this.ga = ga;
    }

    @XmlElement(name = "range")
    public String getRange() {
        return range;
    }

    public void setRange(String range) {
        this.range = range;
    }

    @XmlElement(name = "dm")
    public Integer getDm() {
        return dm;
    }

    public void setDm(Integer dm) {
        this.dm = dm;
    }

    @XmlElement(name = "sf")
    public Integer getSf() {
        return sf;
    }

    public void setSf(Integer sf) {
        this.sf = sf;
    }

    @XmlElement(name = "flat")
    public Integer getFlat() {
        return flat;
    }

    public void setFlat(Integer flat) {
        this.flat = flat;
    }

    @XmlElement(name = "comp")
    public Integer getComp() {
        return comp;
    }

    public void setComp(Integer comp) {
        this.comp = comp;
    }
}