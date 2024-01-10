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

package cz.cas.asu.stelweb.fuky.observe.xml;

import java.util.HashMap;
import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlElement;

@XmlRootElement(name="spectrograph_info")
public class SpectrographInfoXML {
    private String glst;
    private String spgp_4;
    private String spgp_5;
    private String spgp_13;
    private String spce_14;
    private String spfe_14;
    private String spce_24;
    private String spfe_24;
    private String spgp_22;
    private String spgs_19;
    private String spgs_20;
    
    public SpectrographInfoXML() {
    }

    public SpectrographInfoXML(HashMap info) {
        this.glst = ((String)info.get("GLST")).trim(); 
        this.spgp_4 = ((String)info.get("SPGP_4")).trim(); 
        this.spgp_5 = ((String)info.get("SPGP_5")).trim(); 
        this.spgp_13 = ((String)info.get("SPGP_13")).trim(); 
        this.spce_14 = ((String)info.get("SPCE_14")).trim(); 
        this.spfe_14 = ((String)info.get("SPFE_14")).trim(); 
        this.spce_24 = ((String)info.get("SPCE_24")).trim(); 
        this.spfe_24 = ((String)info.get("SPFE_24")).trim(); 
        this.spgp_22 = ((String)info.get("SPGP_22")).trim();
        this.spgs_19 = ((String)info.get("SPGS_19")).trim();
        this.spgs_20 = ((String)info.get("SPGS_20")).trim();
    }

    @XmlElement(name="glst")
    public String getGlst() {
        return glst;
    }

    public void setGlst(String glst) {
        this.glst = glst;
    }

    @XmlElement(name="spgp_4")
    public String getSpgp_4() {
        return this.spgp_4;
    }

    public void setSpgp_4(String spgp_4) {
        this.spgp_4 = spgp_4;
    }

    @XmlElement(name="spgp_5")
    public String getSpgp_5() {
        return this.spgp_5;
    }

    public void setSpgp_5(String spgp_5) {
        this.spgp_5 = spgp_5;
    }

    @XmlElement(name="spgp_13")
    public String getSpgp_13() {
        return this.spgp_13;
    }

    public void setSpgp_13(String spgp_13) {
        this.spgp_13 = spgp_13;
    }

    @XmlElement(name="spce_14")
    public String getSpce_14() {
        return this.spce_14;
    }

    public void setSpce_14(String spce_14) {
        this.spce_14 = spce_14;
    }

    @XmlElement(name="spfe_14")
    public String getSpfe_14() {
        return this.spfe_14;
    }

    public void setSpfe_14(String spfe_14) {
        this.spfe_14 = spfe_14;
    }

    @XmlElement(name="spce_24")
    public String getSpce_24() {
        return this.spce_24;
    }

    public void setSpce_24(String spce_24) {
        this.spce_24 = spce_24;
    }

    @XmlElement(name="spfe_24")
    public String getSpfe_24() {
        return this.spfe_24;
    }

    public void setSpfe_24(String spfe_24) {
        this.spfe_24 = spfe_24;
    }

    @XmlElement(name="spgp_22")
    public String getSpgp_22() {
        return this.spgp_22;
    }

    public void setSpgp_22(String spgp_22) {
        this.spgp_22 = spgp_22;
    }
    
    @XmlElement(name="spgs_19")
    public String getSpgs_19() {
        return this.spgs_19;
    }

    public void setSpgs_19(String spgs_19) {
        this.spgs_19 = spgs_19;
    }
    
    @XmlElement(name="spgs_20")
    public String getSpgs_20() {
        return this.spgs_20;
    }

    public void setSpgs_20(String spgs_20) {
        this.spgs_20 = spgs_20;
    }
}
