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

import java.io.FileInputStream;
import java.net.URL;
import java.util.Properties;
import java.lang.ClassLoader;

public class ObserveProperties extends Properties {

    static final long serialVersionUID = 1L;

    private static boolean loadSuccess;

    private URL propsURL;

    private ObserveProperties() {
        super();

        //propsURL = this.getClass().getResource(path);
        //if (propsURL == null) {
        //    throw new Exception(String.format("Properties '%s' not found.", path));
        //}

        //this.load(new FileInputStream(propsURL.getPath()));

        try {
            this.load(new FileInputStream(System.getProperty("observe.properties.filename")));
            this.loadSuccess = true;
        }
        catch (Exception e) {
            this.loadSuccess = false;
        }
    }

    private static class LazyHolder {
        private static final ObserveProperties INSTANCE = new ObserveProperties();
    }
 
    public static ObserveProperties getInstance() {
        ObserveProperties instance = LazyHolder.INSTANCE;

        if (loadSuccess) {
            return instance;
        }

        return null;
    }

    public int getInteger(String key) {
        return Integer.parseInt(this.getProperty(key));
    }

    public int getInteger(String key, int value) {
        return Integer.parseInt(this.getProperty(key, Integer.toString(value)));
    }

    public boolean getBoolean(String key) {
        return Boolean.parseBoolean(this.getProperty(key));
    }

    public boolean getBoolean(String key, boolean value) {
        return Boolean.parseBoolean(this.getProperty(key, Boolean.toString(value)));
    }
}
