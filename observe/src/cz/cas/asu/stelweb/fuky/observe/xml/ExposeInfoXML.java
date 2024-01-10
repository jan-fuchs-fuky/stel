/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
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

package cz.cas.asu.stelweb.fuky.observe.xml;

import java.util.HashMap;
import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlElement;

@XmlRootElement(name="expose_info")
public class ExposeInfoXML {
    private String filename;
    private String state;
    private Integer elapsedTime;
    private Integer fullTime;
    private Integer archive;
    private Integer exposeNumber;
    private Integer exposeCount;
    private String path;
    private String archivePath;
    private String paths;
    private String archivePaths;
    private String instrument;
    private Double ccdTemp;

    public ExposeInfoXML() {
    }

    public ExposeInfoXML(HashMap info) {
        this.filename = ((String)info.get("filename")); 
        this.state = ((String)info.get("state"));
        this.elapsedTime = ((Integer)info.get("elapsed_time"));
        this.fullTime = ((Integer)info.get("full_time"));
        this.archive = ((Integer)info.get("archive"));
        this.exposeNumber = ((Integer)info.get("expose_number"));
        this.exposeCount = ((Integer)info.get("expose_count"));
        this.path = ((String)info.get("path"));
        this.archivePath = ((String)info.get("archive_path"));
        this.paths = ((String)info.get("paths"));
        this.archivePaths = ((String)info.get("archive_paths"));
        this.instrument = ((String)info.get("instrument"));
        this.ccdTemp = ((Double)info.get("ccd_temp"));
    }

    @XmlElement(name="filename")
    public String getFilename() {
        return filename;
    }

    public void setFilename(String filename) {
        this.filename = filename;
    }

    @XmlElement(name="state")
    public String getState() {
        return state;
    }

    public void setState(String state) {
        this.state = state;
    }

    public boolean isState(String state) {
        return this.state.equals(state);
    }

    @XmlElement(name="elapsed_time")
    public Integer getElapsedTime() {
        return elapsedTime;
    }

    public void setElapsedTime(Integer elapsedTime) {
        this.elapsedTime = elapsedTime;
    }

    @XmlElement(name="full_time")
    public Integer getFullTime() {
        return fullTime;
    }

    public void setFullTime(Integer fullTime) {
        this.fullTime = fullTime;
    }

    @XmlElement(name="archive")
    public Integer getArchive() {
        return archive;
    }

    public void setArchive(Integer archive) {
        this.archive = archive;
    }

    @XmlElement(name="expose_number")
    public Integer getExposeNumber() {
        return exposeNumber;
    }

    public void setExposeNumber(Integer exposeNumber) {
        this.exposeNumber = exposeNumber;
    }

    @XmlElement(name="expose_count")
    public Integer getExposeCount() {
        return exposeCount;
    }

    public void setExposeCount(Integer exposeCount) {
        this.exposeCount = exposeCount;
    }

    @XmlElement(name="path")
    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    @XmlElement(name="archive_path")
    public String getArchivePath() {
        return archivePath;
    }

    public void setArchivePath(String archivePath) {
        this.archivePath = archivePath;
    }

    @XmlElement(name="paths")
    public String getPaths() {
        return paths;
    }

    public void setPaths(String paths) {
        this.paths = paths;
    }

    @XmlElement(name="archive_paths")
    public String getArchivePaths() {
        return archivePaths;
    }

    public void setArchivePaths(String archivePaths) {
        this.archivePaths = archivePaths;
    }

    @XmlElement(name="instrument")
    public String getInstrument() {
        return instrument;
    }

    public void setInstrument(String instrument) {
        this.instrument = instrument;
    }

    public boolean isInstrument(String instrument) {
        return this.instrument.equalsIgnoreCase(instrument);
    }

    @XmlElement(name="ccd_temp")
    public Double getCCDTemp() {
        return ccdTemp;
    }

    public void setCCDTemp(Double ccdTemp) {
        this.ccdTemp = ccdTemp;
    }

    public boolean isCCDTempAlarm() {
        if ((ccdTemp <= -98) && (ccdTemp >= -150)) {
            return false;
        }

        return true;
    }
}
