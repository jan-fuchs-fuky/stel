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

package cz.cas.asu.stelweb.fuky.observe.client;

import java.io.*;
import java.util.logging.*;
import java.net.URL;
import java.util.Properties;
import java.lang.ClassLoader;
import javax.swing.JOptionPane;

public class ObserveClient {

    public ObserveClient() {
        ObserveConnect connect = new ObserveConnect();
        ObserveProperties props = null;
        FileHandler handler = null;
        ObserveWindow frame;
        String logFilename;
        String baseDir;
        boolean verbose = false;
        boolean value = false;

        baseDir = System.getProperty("observe.dir");

        System.setProperty("observe.properties.filename",
            baseDir + "/properties/observe-client.properties");

        if (baseDir == null) {
            JOptionPane.showMessageDialog(
                null,
                "Please set observe.dir property.",
                "Property observe.dir is not set",
                JOptionPane.ERROR_MESSAGE
            );

            System.exit(1);
        }

        props = ObserveProperties.getInstance();
        if (props == null) {
            JOptionPane.showMessageDialog(
                null,
                String.format("Properties '%s' load failure",
                    System.getProperty("observe.properties.filename")),
                "Error",
                JOptionPane.ERROR_MESSAGE
            );

            System.exit(1);
        }

        connect.host = props.getProperty("observe.server.host", "observe.asu.cas.cz");
        connect.port = props.getInteger("observe.server.port", 443);
        connect.username = props.getProperty("observe.server.username", "tcsuser");
        connect.password = props.getProperty("observe.server.password", "");

        verbose = props.getBoolean("observe.client.verbose", false);
        logFilename = props.getProperty("observe.client.log.filename", "observe-client.log");

        try {
            handler = new FileHandler(logFilename, true);
        }
        catch (IOException e) {
            JOptionPane.showMessageDialog(
                null,
                e.getMessage(),
                "Logging failure",
                JOptionPane.ERROR_MESSAGE
            );

            System.exit(1);
        }

        frame = new ObserveWindow(connect, handler, verbose);
    }

    public static void main(String[] args) {
        // TODO: doladit
        try {
            File trustStore = File.createTempFile("observe",".truststore");
            InputStream in = ClassLoader.getSystemResourceAsStream("keystore.p12");
            byte buffer[] = new byte[4096];
            int count;

            trustStore.deleteOnExit();

            FileOutputStream out = new FileOutputStream(trustStore);
            while ((count = in.read(buffer)) != -1) {
                out.write(buffer, 0, count);
            }

            in.close();
            out.close();

            System.setProperty("javax.net.ssl.trustStore", trustStore.getAbsolutePath());
        }
        catch (IOException e) { 
            System.out.println(e.getMessage());
        }

        System.setProperty("javax.net.ssl.trustStorePassword", "qwerty");
        System.setProperty("javax.net.ssl.trustStoreType", "pkcs12");

        javax.swing.SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                new ObserveClient();
            }
        });
    }
}
