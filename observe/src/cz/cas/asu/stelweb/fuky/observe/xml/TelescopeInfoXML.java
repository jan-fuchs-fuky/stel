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

@XmlRootElement(name="telescope_info")
public class TelescopeInfoXML {
    private String ut;
    private String glst;
    private String trrd;
    private String trhd;
    private String trgv;
    private String trus;
    private String dopo;
    private String trcs;
    private String fopo;
    private String tsra;
    private String object;
    
    public TelescopeInfoXML() {
    }

    public TelescopeInfoXML(HashMap info) {
        this.ut = ((String)info.get("ut")).trim(); 
        this.glst = ((String)info.get("glst")).trim(); 
        this.trrd = ((String)info.get("trrd")).trim();
        this.trhd = ((String)info.get("trhd")).trim();
        this.trgv = ((String)info.get("trgv")).trim();
        this.trus = ((String)info.get("trus")).trim();
        this.dopo = ((String)info.get("dopo")).trim();
        this.trcs = ((String)info.get("trcs")).trim();
        this.fopo = ((String)info.get("fopo")).trim();
        this.tsra = ((String)info.get("tsra")).trim();
        this.object = ((String)info.get("object")).trim();
    }

    @XmlElement(name="ut")
    public String getUT() {
        return ut;
    }

    public void setUT(String ut) {
        this.ut = ut;
    }

    @XmlElement(name="glst")
    public String getGlst() {
        return glst;
    }

    public void setGlst(String glst) {
        this.glst = glst;
    }

    @XmlElement(name="trrd")
    public String getTrrd() {
        return trrd;
    }

    public void setTrrd(String trrd) {
        this.trrd = trrd;
    }
    @XmlElement(name="trhd")
    public String getTrhd() {
        return trhd;
    }

    public void setTrhd(String trhd) {
        this.trhd = trhd;
    }
    @XmlElement(name="trgv")
    public String getTrgv() {
        return trgv;
    }

    public void setTrgv(String trgv) {
        this.trgv = trgv;
    }
    @XmlElement(name="trus")
    public String getTrus() {
        return trus;
    }

    public void setTrus(String trus) {
        this.trus = trus;
    }

    @XmlElement(name="dopo")
    public String getDopo() {
        return dopo;
    }

    public void setDopo(String dopo) {
        this.dopo = dopo;
    }

    @XmlElement(name="trcs")
    public String getTrcs() {
        return trcs;
    }

    public void setTrcs(String trcs) {
        this.trcs = trcs;
    }

    @XmlElement(name="fopo")
    public String getFopo() {
        return fopo;
    }

    public void setFopo(String fopo) {
        this.fopo = fopo;
    }

    @XmlElement(name="tsra")
    public String getTsra() {
        return tsra;
    }

    public void setTsra(String tsra) {
        this.tsra = tsra;
    }

    @XmlElement(name="object")
    public String getObject() {
        return object;
    }

    public void setObject(String object) {
        this.object = object;
    }
}
