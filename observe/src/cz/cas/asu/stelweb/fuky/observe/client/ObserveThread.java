/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *
 *   Copyright (C) 2008-2011 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TimeZone;
import java.util.logging.Level;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.swing.JLabel;
import javax.swing.SwingUtilities;
import javax.swing.SwingWorker;
import javax.swing.text.Style;
import javax.ws.rs.core.Response.Status;

import jsky.coords.DMS;
import jsky.coords.HMS;
import jsky.coords.SiteDesc;
import jsky.coords.WorldCoords;
import jsky.util.SkyCalc;
import cz.cas.asu.stelweb.fuky.observe.jaxb.ManualSetupExposureXML;
import cz.cas.asu.stelweb.fuky.observe.xml.ExposeInfoXML;
import cz.cas.asu.stelweb.fuky.observe.xml.SetupClientXML;
import cz.cas.asu.stelweb.fuky.observe.xml.SpectrographInfoXML;
import cz.cas.asu.stelweb.fuky.observe.xml.TelescopeInfoXML;
import cz.cas.asu.stelweb.fuky.observe.xml.setup_exposure.SetupExposureXML;
import cz.cas.asu.stelweb.fuky.observe.xml.setup_exposure.SetupExposure;

public class ObserveThread extends SwingWorker<Void, Object> {
    private ObserveWindow gui;
    private ObserveConnect observeConnect;
    private JerseyClientSSL client;

    private final String REGEX_STAR_COORDINATES = "(\\d+\\.?\\d*) ([+-]?\\d+\\.?\\d*) ([01])";
    private final String REGEX_SOURCE_COORDINATES = "([+-]?\\d+\\.?\\d*) ([+-]*\\d+\\.?\\d*)";
    private final String REGEX_COORDINATE = "([+-]?\\d{2})(\\d{2})(\\d{2}\\.?\\d*)";
    private final String REGEX_TWO_DOUBLE = "([+-]?\\d+\\.?\\d*) ([+-]?\\d+\\.?\\d*)";
    private final String REGEX_DOUBLE = "([+-]?\\d+\\.?\\d*)";
    private final String REGEX_INTEGER = "([+-]?\\d+)";

    private final Pattern PATTERN_STAR_COORDINATES = Pattern.compile(REGEX_STAR_COORDINATES);
    private final Pattern PATTERN_SOURCE_COORDINATES = Pattern.compile(REGEX_SOURCE_COORDINATES);
    private final Pattern PATTERN_COORDINATE = Pattern.compile(REGEX_COORDINATE);
    private final Pattern PATTERN_TWO_DOUBLE = Pattern.compile(REGEX_TWO_DOUBLE);
    private final Pattern PATTERN_DOUBLE = Pattern.compile(REGEX_DOUBLE);
    private final Pattern PATTERN_INTEGER = Pattern.compile(REGEX_INTEGER);

    public ObserveThread(ObserveWindow gui, ObserveConnect observeConnect) {
        this.gui = gui;
        this.observeConnect = observeConnect;
        this.client = new JerseyClientSSL(observeConnect);
    }

    @Override
    protected Void doInBackground() {
        connectDialog();

        while (!gui.exit) {
            if (!connect()) {
                connectDialog();
                continue;
            }

            try {
                mainLoop();
            }
            catch (NotOkStatusException e) {
                gui.logger.log(Level.SEVERE, "mainLoop() failed", e);

                final String msg = String.format("%d %s", e.getStatusCode(), e.getReasonPhrase());
                log(Level.INFO, msg);
                publish(new ConnectState(msg, ConnectState.State.CLOSE));

                connectDialog();
                continue;
            }
            catch (Exception e) {
                String msg = "";
                String[] message = e.getMessage().split(":", 2);

                if (message.length >= 2) {
                    msg = message[1].trim();
                }
                else {
                    msg = message[0];
                }

                log(Level.SEVERE, msg, e);
                publish(new ConnectState(msg, ConnectState.State.FAILURE));
            }

            final String msg = String.format("Connection closed");
            log(Level.INFO, msg);
            publish(new ConnectState(msg, ConnectState.State.CLOSE));
        }

        return null;
    }

    @Override
    protected void process(List<Object> objectList) {
        for (Object object : objectList) {
            if (object instanceof Telescope) {
                gui.telescopeRefresh((Telescope)object);
            }
            else if (object instanceof Spectrograph) {
                gui.spectrographRefresh((Spectrograph)object);
                gui.ccd400_P.spectrographRefresh((Spectrograph)object);
                gui.ccd700_P.spectrographRefresh((Spectrograph)object);
            }
            else if (object instanceof ExposeInfoXML) {
                ExposeInfoXML exposeInfoXML = (ExposeInfoXML)object;

                if (exposeInfoXML.isInstrument("oes")) {
                    gui.oes_P.ccdRefresh(exposeInfoXML);
                }
                else if (exposeInfoXML.isInstrument("ccd400")) {
                    gui.ccd400_P.ccdRefresh(exposeInfoXML);
                }
                else if (exposeInfoXML.isInstrument("ccd700")) {
                    gui.ccd700_P.ccdRefresh(exposeInfoXML);
                }
            }
            else if (object instanceof ExposeState) {
                ExposeState exposeState = (ExposeState)object;

                if (exposeState.isInstrument("oes")) {
                    gui.oes_P.exposeInit((ExposeState)object);
                }
                else if (exposeState.isInstrument("ccd400")) {
                    gui.ccd400_P.exposeInit((ExposeState)object);
                }
                else if (exposeState.isInstrument("ccd700")) {
                    gui.ccd700_P.exposeInit((ExposeState)object);
                }
            }
            else if (object instanceof SetupExposureXML) {
                SetupExposureXML setupExposureXML = (SetupExposureXML)object;
                
                ArrayList<SetupExposure> setupExposureList = setupExposureXML
                        .getSetupExposureList();

                Map<String, SetupExposure> map = new HashMap<String, SetupExposure>();
                for (SetupExposure setupExposure : setupExposureList) {
                    map.put(setupExposure.getName(), setupExposure);
                }

                if (setupExposureXML.isInstrument("oes")) {
                    gui.oes_P.loadSetupExposure(map);
                }
                else if (setupExposureXML.isInstrument("ccd400")) {
                    gui.ccd400_P.loadSetupExposure(map);
                }
                else if (setupExposureXML.isInstrument("ccd700")) {
                    gui.ccd700_P.loadSetupExposure(map);
                }
            }
            else if (object instanceof ManualSetupExposureXML) {
                ManualSetupExposureXML manualSetupExposureXML = (ManualSetupExposureXML)object;
                
                gui.ccd400_P.setManualSetupExposure(manualSetupExposureXML);
                gui.ccd700_P.setManualSetupExposure(manualSetupExposureXML);
            }
            else if (object instanceof SetupClientXML) {
                gui.setDeprecatedVersionMsg((SetupClientXML)object);
            }
            else if (object instanceof ObserveGuiLog) {
                gui.addLog((ObserveGuiLog)object);
            }
            else if (object instanceof ConnectState) {
                gui.setConnectState((ConnectState)object);
            }
            else if (object instanceof ObserveGuiSetText) {
                gui.setText((ObserveGuiSetText)object);
            }
        }
    }

    private void connectDialog() {
        Runnable runnable = new Runnable() {
            public void run() {
                gui.showConnectDialog();
            }
        };
        swingInvokeAndWait(runnable);
        this.client.reload();
    }

    // TODO: odstraneni duplicitniho kodu
    private void mainLoop() throws NotOkStatusException {
        JLabel status_LB = null;
        boolean oesInit = true;
        boolean ccd400Init = true;
        boolean ccd700Init = true;
        
        publish(client.getSetupClientXML());

        while (!gui.exit) {
            execCmdsFromFIFO();

            for (int i = 1; i <= 5; i++) {
                try {
                    switch (i) {
                        case 1:
                            status_LB = gui.getStatus_telescope_LB();
                            getTelescopeState();
                            break;

                        case 2:
                            status_LB = gui.getStatus_spectrograph_LB();
                            getSpectrographState();
                            break;

                        case 3:
                            status_LB = gui.getStatus_oes_LB();
                            getCCDState("oes", oesInit);
                            oesInit = false;
                            break;

                        case 4:
                            status_LB = gui.getStatus_ccd400_LB();
                            try {
                                getCCDState("ccd400", ccd400Init);
                            }
                            // TODO
                            catch (Exception e) {
                                log(Level.INFO, "CCD400 exception");
                                throw new NotOkStatusException(Status.SERVICE_UNAVAILABLE.getStatusCode());
                            }
                            ccd400Init = false;
                            break;

                        case 5:
                            status_LB = gui.getStatus_ccd700_LB();
                            getCCDState("ccd700", ccd700Init);
                            ccd700Init = false;
                            break;

                        default:
                            break;
                    }

                    publish(new ObserveGuiSetText("available", status_LB));
                }
                catch (NotOkStatusException e) {
                    if (e.getStatusCode() != Status.SERVICE_UNAVAILABLE.getStatusCode()) {
                        throw e;
                    }

                    final String msg = String.format("Connected %s:%d", observeConnect.host, observeConnect.port);
                    publish(new ConnectState(msg, ConnectState.State.SUCCESS));
                    publish(new ObserveGuiSetText("unavailable", status_LB));
                }
            }

            try {
                Thread.currentThread().sleep(500);
            }
            catch (InterruptedException e) {
                gui.logger.log(Level.SEVERE, "Thread.currentThread().sleep() failed", e);
            }
        }
    }

    private void getTelescopeState() throws NotOkStatusException {
        Telescope telescope = new Telescope();
        double[] ra_dec;
        String[] tsra;

        TelescopeInfoXML telescopeXML = this.client.getTelescopeInfoXML();

        try {
            telescope.parse(telescopeXML.getGlst());
        }
        catch (Exception e) {
            gui.logger.log(Level.SEVERE, "telescope.parse()", e);
            return;
        }

        // Height on dome:
        // Dome position:

        parseStarCoordinates(telescopeXML.getTrrd(), telescopeXML.getUT(), telescope);
        parseSourceCoordinates(telescopeXML.getTrhd(), telescope);

        // Telescope Read Guiding Value
        ra_dec = parseTwoDouble(telescopeXML.getTrgv());
        telescope.correctionsRa = ra_dec[0];
        telescope.correctionsDec = ra_dec[1];

        // Telescope Read User Speed
        ra_dec = parseTwoDouble(telescopeXML.getTrus());
        telescope.speedRa = ra_dec[0];
        telescope.speedDec = ra_dec[1];

        // DOme POsition
        telescope.domePosition = parseDouble(telescopeXML.getDopo());

        // Telescope Read Correction Set
        telescope.modelNumber = parseInteger(telescopeXML.getTrcs());

        // FOcus POsition
        telescope.sharping = parseDouble(telescopeXML.getFopo());

        tsra = telescopeXML.getTsra().split(" ");
        if (tsra.length == 3) {
            telescope.tsra_ra = tsra[0]; 
            telescope.tsra_dec = tsra[1]; 
            telescope.tsra_position = tsra[2];
            telescope.object = telescopeXML.getObject();
        }

        publish(telescope);
    }

    private void getSpectrographState() throws NotOkStatusException {
        SpectrographInfoXML spectrographXML = this.client.getSpectrographInfoXML();
        Spectrograph spectrograph = new Spectrograph(spectrographXML);

        publish(spectrograph);
    }

    private String degrees2dms(String value) {
        DMS dms = new DMS(value);

        return String.format(
            "%02d %02d %02.2f",
            dms.getDegrees() * dms.getSign(),
            dms.getMin(),
            dms.getSec()
        );
    }

    private int parseInteger(String input) {
        int number = Integer.MAX_VALUE;
        Matcher matcherInteger = PATTERN_INTEGER.matcher(input);

        if (matcherInteger.find() && (matcherInteger.groupCount() == 1)) {
            number = Integer.parseInt(matcherInteger.group(1));
        }

        return number;
    }

    private double parseDouble(String input) {
        double number = Double.NaN;
        Matcher matcherDouble = PATTERN_DOUBLE.matcher(input);

        if (matcherDouble.find() && (matcherDouble.groupCount() == 1)) {
            number = Double.parseDouble(matcherDouble.group(1));
        }

        return number;
    }

    private double[] parseTwoDouble(String input) {
        double[] num1_num2 = { Double.NaN, Double.NaN };
        Matcher matcherTwoDouble = PATTERN_TWO_DOUBLE.matcher(input);

        if (matcherTwoDouble.find() && (matcherTwoDouble.groupCount() == 2)) {
            num1_num2[0] = Double.parseDouble(matcherTwoDouble.group(1));
            num1_num2[1] = Double.parseDouble(matcherTwoDouble.group(2));
        }

        return num1_num2;
    }

    /*   
     *   TRHD (Telescope Read Hour and Declination Axis)
     *
     *   TRHD\r
     *   -180.9000 55.7890\r
     *
     *   unit = degree
     */
    private void parseSourceCoordinates(String input, Telescope telescope) {
        Matcher matcherSourceCoordinates = PATTERN_SOURCE_COORDINATES.matcher(input);

        if (matcherSourceCoordinates.find() && (matcherSourceCoordinates.groupCount() == 2)) {
            telescope.ha = degrees2dms(matcherSourceCoordinates.group(1));
            telescope.da = degrees2dms(matcherSourceCoordinates.group(2));
        }
    }

    /* 
     *   TRRD (Telescope Read Right ascension and Declination)
     *
     *   TRRD\r
     *   235912.10 -102312.43 0\r
     *
     *   ra = hour:minute:second
     *   dec = degree:minute:second
     *   position = 0 - east or 1 - western
     */
    private void parseStarCoordinates(String trrd, String ut, Telescope telescope) {
        final SimpleDateFormat simpleDateFormat = new SimpleDateFormat("HH:mm:ss");
        Matcher matcherStarCoordinates;
        Matcher matcherCoordinate;

        matcherStarCoordinates = PATTERN_STAR_COORDINATES.matcher(trrd);
        if (matcherStarCoordinates.find()) {
            if (matcherStarCoordinates.groupCount() == 3) {
                matcherCoordinate = PATTERN_COORDINATE.matcher(matcherStarCoordinates.group(1));
                if (matcherCoordinate.find()) {
                    telescope.ra = String.format(
                        "%s %s %s",
                        matcherCoordinate.group(1),
                        matcherCoordinate.group(2),
                        matcherCoordinate.group(3)
                    );
                }

                matcherCoordinate = PATTERN_COORDINATE.matcher(matcherStarCoordinates.group(2));
                if (matcherCoordinate.find()) {
                    telescope.dec = String.format(
                        "%s %s %s",
                        matcherCoordinate.group(1),
                        matcherCoordinate.group(2),
                        matcherCoordinate.group(3)
                    );
                }

                if (matcherStarCoordinates.group(3).equals("0")) {
                    telescope.position = "east";
                }
                else {
                    telescope.position = "western";
                }

                /*
                 *   TODO:
                 *   - load settings from file
                 */
                Date date;

                try {
                    SimpleDateFormat ut_format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

                    ut_format.setTimeZone(TimeZone.getTimeZone("GMT"));
                    date = ut_format.parse(ut);
                }
                catch (java.text.ParseException e) {
                    date = new Date();
                    String msg = String.format("Parse UT = '%s' - %s", ut, e.getMessage());
                    log(Level.SEVERE, msg, e);
                }

                Calendar calendar = Calendar.getInstance(TimeZone.getTimeZone("GMT"));
                TimeZone tz = TimeZone.getTimeZone("Europe/Prague");
                SiteDesc site = new SiteDesc("Ondrejov", 14.7836111, 49.910555, tz);
                SkyCalc skyCalc = new SkyCalc(site);

                // J = 2000.0 + (Julian date 2451545.0) / 365.25
                double equinox = calendar.get(Calendar.YEAR) + calendar.get(Calendar.DAY_OF_YEAR) / 365.25;
                WorldCoords obj = new WorldCoords(telescope.ra, telescope.dec, equinox);
                HMS raHMS = new HMS(telescope.ra);
                HMS lstHMS;
                HMS hourAngleHMS;
                double hourAngle;

                skyCalc.calculate(obj, calendar.getTime());

                simpleDateFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
                telescope.utc = simpleDateFormat.format(date);
                telescope.lst = simpleDateFormat.format(skyCalc.getLst(date));
                telescope.airmass = skyCalc.getAirmass();
                telescope.azimuth = skyCalc.getAzimuth();
                telescope.altitude = skyCalc.getAltitude();

                lstHMS = new HMS(telescope.lst);
                /*
                 *   TODO: check range
                 *
                 *   HA = LST - RA
                 */
                hourAngle = lstHMS.getVal() - raHMS.getVal();
                hourAngleHMS = new HMS(hourAngle);

                telescope.hourAngle = String.format(
                    "%02d:%02d:%02.0f",
                    hourAngleHMS.getHours(),
                    hourAngleHMS.getMin(),
                    hourAngleHMS.getSec()
                );
            }
        }
    }

    /*
     *   +OK 1 ccd is ready ''
     *   +OK 1 ccd is ready 'xtc24001.fit'
     *   +OK 3 exposing 1 3 'xtc24001.fit'
     *   +OK 5 reading out CCD 0 5 'xtc24001.fit'
     */
    private void getCCDState(String instrument, boolean exposeInit) throws NotOkStatusException {
        ExposeInfoXML exposeInfoXML = this.client.getExposeInfoXML(instrument);

        publish(exposeInfoXML);

        if (exposeInit) {
            SetupExposureXML setupExposureXML = this.client.getSetupExposureXML(instrument);
            publish(setupExposureXML);
            
            if (instrument.equals("ccd700")) {
                ManualSetupExposureXML manualSetupExposureXML = this.client
                        .getManualSetupExposureXML();
                publish(manualSetupExposureXML);
            }
                        
            CallBack callBack;
            ArrayList<Object> params = new ArrayList<Object>();
            ExposeState exposeState = new ExposeState(exposeInfoXML);

            params.add("READOUT_SPEEDS");
            callBack = new ExposeExecuteCB(instrument, "expose_get", params.toArray());
            exposeState.setReadoutSpeeds(callBack.run(this.client));
            
            params.clear();
            params.add("GAINS");
            callBack = new ExposeExecuteCB(instrument, "expose_get", params.toArray());
            exposeState.setGains(callBack.run(this.client));
            
            params.clear();
            params.add("OBJECT");
            callBack = new ExposeExecuteCB(instrument, "expose_get_key", params.toArray());
            exposeState.setObject(callBack.run(this.client));

            params.clear();
            params.add("IMAGETYP");
            callBack = new ExposeExecuteCB(instrument, "expose_get_key", params.toArray());
            exposeState.setTarget(callBack.run(this.client));

            publish(exposeState);
        }
    }

    private void execCmdsFromFIFO() {
        CallBack callBack;
        String answer;

        synchronized (gui.callBacksFIFO) {
            callBack = gui.callBacksFIFO.poll();
        }

        while (callBack != null) {
            StringBuffer strBuffer = new StringBuffer();
            Object[] params = callBack.getFunctionParams();

            strBuffer.append(callBack.getInstrument());
            strBuffer.append(" - ");
            strBuffer.append(callBack.getFunctionName());
            strBuffer.append("(");

            for (int i = 0; i < params.length; ++i) {
                strBuffer.append("\"");
                strBuffer.append(params[i].toString());
                strBuffer.append("\"");

                if (i != (params.length - 1)) {
                    strBuffer.append(", ");
                }
            }

            strBuffer.append(")");

            log(Level.INFO, strBuffer.toString());

            try {
                answer = callBack.run(this.client);

                if (answer.startsWith("ERR")) {
                    log(Level.WARNING, String.format("Answer: %s", answer));
                }
                else {
                    log(Level.FINE, String.format("Answer: %s", answer));
                }
            }
            catch (NotOkStatusException e) {
                String msg = String.format("Execute failure - %s", e.getMessage());
                log(Level.SEVERE, msg, e);
            }

            synchronized (gui.callBacksFIFO) {
                callBack = gui.callBacksFIFO.poll();
            }
        }
    }

    private boolean connect() {
        String msg = String.format("Connecting %s:%d", observeConnect.host, observeConnect.port);
        publish(new ConnectState(msg, ConnectState.State.PROGRESS));

        try {
            this.client.getUser(observeConnect.username);
        }
        catch (NotOkStatusException e) {
            msg = String.format("%d %s", e.getStatusCode(), e.getReasonPhrase());
            log(Level.SEVERE, msg, e);
            publish(new ConnectState(msg, ConnectState.State.FAILURE));

            return false;
        }
        catch (Exception e) {
            String[] message = e.getMessage().split(":", 2);

            if (message.length >= 2) {
                msg = message[1].trim();
            }
            else {
                msg = message[0];
            }

            log(Level.SEVERE, msg, e);
            publish(new ConnectState(msg, ConnectState.State.FAILURE));

            return false;
        }

        msg = String.format("Connected %s:%d", observeConnect.host, observeConnect.port);
        publish(new ConnectState(msg, ConnectState.State.SUCCESS));

        return true;
    }

    private void log(Level level, final String msg) {
        log(level, msg, null);
    }

    private void log(Level level, final String msg, Throwable thrown) {
        final Style style;

        if ((level == Level.WARNING) || (level == Level.SEVERE)) {
            style = gui.warning_style;
        }
        else if (level == Level.FINE) {
            style = gui.higlight_style;
        }
        else {
            style = gui.normal_style;
        }

        if (thrown == null) {
            gui.logger.log(level, msg);
        }
        else {
            gui.logger.log(level, msg, thrown);
        }

        publish(new ObserveGuiLog(msg, style));
    }

    private void swingInvokeAndWait(Runnable runnable) {
        try {
            SwingUtilities.invokeAndWait(runnable);
        }
        catch (Exception e) {
            gui.logger.log(Level.SEVERE, "SwingUtilities.invokeAndWait failed", e);
        }
    }
}
