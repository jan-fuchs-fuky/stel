/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2008-2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.LinkedList;
import java.util.Queue;
import java.util.logging.FileHandler;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JSlider;
import javax.swing.ToolTipManager;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.text.BadLocationException;
import javax.swing.text.Style;
import javax.swing.text.StyleConstants;
import javax.swing.text.StyledDocument;

import cz.cas.asu.stelweb.fuky.observe.ObserveBuild;
import cz.cas.asu.stelweb.fuky.observe.xml.SetupClientXML;

public class ObserveWindow extends javax.swing.JFrame {
    static final long serialVersionUID = 42L;

    private Toolkit tk = Toolkit.getDefaultToolkit();
    private Dimension screenSize = tk.getScreenSize();
    private Handler handler;
    private ObserveConnect connect;
    private StyledDocument log_doc;
    private ObserveThread observe_thread = null;
    private String text = new String();;
    private Color color = new Color(255, 255, 255);
    private Color red_color = new Color(255, 204, 204);
    private Color green_color = new Color(204, 255, 204);
    private Color blue_color = new Color(204, 204, 255);
    private Color gray59_color = new Color(150, 150, 150);
    private Color gold2_color = new Color(238, 201, 0);
    private Color aquamarine2_color = new Color(118, 238, 198);
    private boolean logEnabled = true;
    private String tsra_ra;
    private String tsra_dec;
    private String tsra_position;
    private String tle_object;

    // [+/-]DDMMSS[.SSS]
    public static final String REGEX_DDMMSS = "^[+-]?\\d{6}(\\.\\d{0,3})?$";
    public static final Pattern PATTERN_DDMMSS = Pattern.compile(REGEX_DDMMSS);
    
    // [+/-]D[DD][.DDDD]
    public static final String REGEX_DEGREES = "^[+-]?\\d{1,3}(\\.\\d{0,4})?$";
    public static final Pattern PATTERN_DEGREES = Pattern.compile(REGEX_DEGREES);
    
    // D:M[.MMM]
    public static final String REGEX_DM = "^(\\d{1,3}):(\\d{1,2}(\\.\\d{0,3})?)$";
    public static final Pattern PATTERN_DM = Pattern.compile(REGEX_DM);
    
    /*
     * FITs header: Note that the header unit may only contain ASCII text
     * characters ranging from hexadecimal 20 to 7E); non-printing ASCII
     * characters such as tabs, carriage-returns, or line-feeds are not allowed
     * anywhere within the header unit.
     * 
     * http://fits.gsfc.nasa.gov/fits_primer.html
     */
    public static final int FITS_HDR_VALUE_MAX = 18;
    public static final String REGEX_FITS_HDR = "^[\\x20-\\x7E]{1,18}$";
    public static final Pattern PATTERN_FITS_HDR = Pattern.compile(REGEX_FITS_HDR);
    
    public boolean verbose;
    public Logger logger;
    public Queue<CallBack> callBacksFIFO = new LinkedList<CallBack>();
    public String error_message;
    public Style normal_style;
    public Style warning_style;
    public Style higlight_style;
    public ObservePanel oes_P = new ObservePanel(this, "oes");
    public ObservePanel ccd400_P = new ObservePanel(this, "ccd400");
    public ObservePanel ccd700_P = new ObservePanel(this, "ccd700");

    public boolean exit = false;
    
    /** Creates new form ObserveWindow */
    public ObserveWindow(ObserveConnect connect, FileHandler handler, boolean verbose) {
        String appName = String.format("ObserveClient %s (%s)",
            ObserveBuild.getSvnVersion(), ObserveBuild.getDate());

        this.connect = connect;
        this.handler = handler;
        this.verbose = verbose;

        initComponents();
        setTitle(appName);
        
        deprecated_version_LB.setVisible(false);
        observers_TF.setDocument(new JTextFieldLimit(FITS_HDR_VALUE_MAX));
        
        ToolTipManager.sharedInstance().setDismissDelay(120000);
        ToolTipManager.sharedInstance().setReshowDelay(250);
        ToolTipManager.sharedInstance().setInitialDelay(250); 

        observe_TP.add("OES", oes_P);
        observe_TP.add("CCD400", ccd400_P);
        observe_TP.add("CCD700", ccd700_P);

        opso_slider_addChangeListener();
        spectrograph_button_addActionListener();
        spectrograph_combobox_addActionListener();
        ascol_button_addActionListener();
        ascol_combobox_addActionListener();
        observe_addActionListener();

        //setSize(screenSize.width, screenSize.height);
        // TODO: konfigurovatelne
        setSize(625, screenSize.height - 50);
        centerJFrame(this);

        log_doc = (StyledDocument)log_TP.getDocument();
            
        normal_style = log_doc.addStyle("normal", null);
        warning_style = log_doc.addStyle("warning", null);
        higlight_style = log_doc.addStyle("higlight", null);
 
        StyleConstants.setForeground(normal_style, Color.BLACK);
        StyleConstants.setForeground(warning_style, Color.RED);
        StyleConstants.setForeground(higlight_style, Color.BLUE);

        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                quit();
            }
        });

        String text = "<html>Execute ASCOL command - INSTRUMENT ASCOL_CMD [PARAM1 [PARAM2]]<br><br>"
                + "Example:<br><br>"
                + "    T GLST - telescope get global state<br>"
                + "    S GLST - spectrograph get global state<br>"
                + "    S SPCH 27 1 - spectrograph slit camera power on</html>";
        
        execute_ascol_TF.setToolTipText(text);
        
        logger = Logger.getLogger("observe_client");
        handler.setFormatter(new SimpleFormatter());
        logger.setLevel(Level.FINEST);
        logger.addHandler(handler);
        logger.log(Level.INFO, String.format("Starting %s", appName));

        state_LB.setText("Client started");
        setEnabled(false);
        setVisible(true);

        //observe_thread = new ObserveThread(this, connect);
        //observe_thread.start();

        observe_thread = new ObserveThread(this, connect);
        observe_thread.execute();
    }
    
    public void setDeprecatedVersionMsg(SetupClientXML setupClientXML) {
        int svn_version = -1;
        int version_actual = setupClientXML.getVersionActual().intValue();
        String msg = null;
        
        try {
            // TODO: predelat
            //svn_version = Integer.parseInt(ObserveBuild.getSvnVersion());
            svn_version = 20150103;

            if (svn_version >= version_actual) {
                return;
            }

            msg = String.format(
                    "%d is deprecated version, please download new version %d",
                    svn_version, version_actual);
        }
        catch (NumberFormatException e) {
            msg = String.format(
                    "%s is development version, please download stable version %d",
                    ObserveBuild.getSvnVersion(), version_actual);
        }
                
        if (msg != null) {
            // TODO: udelat globani barvy
            Color red_color = new Color(255, 204, 204);
            deprecated_version_LB.setBackground(red_color);
            deprecated_version_LB.setVisible(true);
            deprecated_version_LB.setText(msg);
        }
    }
    
    public String getCollimator() {
        return collimator_LB.getText();
    }
    
    public String getOesCollimator() {
        return oes_collimator_LB.getText();
    }
    
    public String getDichroicMirror() {
        return dichroic_mirror_LB.getText();
    }
    
    public String getSpectralFilter() {
        return spectral_filter_LB.getText();
    }
    
    public String getGratingAngle() {
        return grating_angle_LB.getText();
    }
    
    public String getCorrectionPlates400() {
        return correction_plates_400_LB.getText();
    }
    
    public String getCorrectionPlates700() {
        return correction_plates_700_LB.getText();
    }
    
    public String getCoudeOes() {
        return coude_oes_LB.getText();
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {
        java.awt.GridBagConstraints gridBagConstraints;

        connect_P = new javax.swing.JPanel();
        host_LB = new javax.swing.JLabel();
        port_LB = new javax.swing.JLabel();
        username_LB = new javax.swing.JLabel();
        password_LB = new javax.swing.JLabel();
        host_TF = new javax.swing.JTextField();
        port_TF = new javax.swing.JTextField();
        username_TF = new javax.swing.JTextField();
        password_PF = new javax.swing.JPasswordField();
        jPanel1 = new javax.swing.JPanel();
        main_P = new javax.swing.JPanel();
        jTabbedPane1 = new javax.swing.JTabbedPane();
        services_status_P = new javax.swing.JPanel();
        status_opso_LB = new javax.swing.JLabel();
        jLabel10 = new javax.swing.JLabel();
        jLabel13 = new javax.swing.JLabel();
        jLabel14 = new javax.swing.JLabel();
        jLabel15 = new javax.swing.JLabel();
        status_telescope_LB = new javax.swing.JLabel();
        status_ccd700_LB = new javax.swing.JLabel();
        status_oes_LB = new javax.swing.JLabel();
        log_P = new javax.swing.JPanel();
        log_SP = new javax.swing.JScrollPane();
        log_TP = new javax.swing.JTextPane();
        clearLog_BT = new javax.swing.JButton();
        disableLog_BT = new javax.swing.JButton();
        enableLog_BT = new javax.swing.JButton();
        execute_ascol_BT = new javax.swing.JButton();
        execute_ascol_TF = new javax.swing.JTextField();
        status_ccd400_LB = new javax.swing.JLabel();
        jLabel20 = new javax.swing.JLabel();
        jLabel16 = new javax.swing.JLabel();
        status_spectrograph_LB = new javax.swing.JLabel();
        deprecated_version_LB = new javax.swing.JLabel();
        observe_P = new javax.swing.JPanel();
        observe_bottom_P = new javax.swing.JPanel();
        observers_LB = new javax.swing.JLabel();
        observers_TF = new javax.swing.JTextField();
        observe_TP = new javax.swing.JTabbedPane();
        observe_top_P = new javax.swing.JPanel();
        selected_ccd_LB = new javax.swing.JLabel();
        spectrograph_TB = new javax.swing.JTabbedPane();
        spectrograph_control_P = new javax.swing.JPanel();
        spectrograph_main_P = new javax.swing.JPanel();
        star_calibration_label_LB = new javax.swing.JLabel();
        star_calibration_LB = new javax.swing.JLabel();
        star_BT = new javax.swing.JButton();
        calibration_BT = new javax.swing.JButton();
        star_calibration_stop_BT = new javax.swing.JButton();
        coude_oes_label_LB = new javax.swing.JLabel();
        coude_oes_LB = new javax.swing.JLabel();
        coude_BT = new javax.swing.JButton();
        oes_BT = new javax.swing.JButton();
        coude_oes_stop_BT = new javax.swing.JButton();
        flat_label_LB = new javax.swing.JLabel();
        flat_LB = new javax.swing.JLabel();
        comp_label_LB = new javax.swing.JLabel();
        comp_LB = new javax.swing.JLabel();
        flat_on_BT = new javax.swing.JButton();
        flat_off_BT = new javax.swing.JButton();
        comp_on_BT = new javax.swing.JButton();
        comp_off_BT = new javax.swing.JButton();
        jTabbedPane4 = new javax.swing.JTabbedPane();
        spectrograph_coude_P = new javax.swing.JPanel();
        dichroic_mirror_label_LB = new javax.swing.JLabel();
        spectral_filter_label_LB = new javax.swing.JLabel();
        collimator_label_LB = new javax.swing.JLabel();
        dichroic_mirror_LB = new javax.swing.JLabel();
        spectral_filter_LB = new javax.swing.JLabel();
        collimator_LB = new javax.swing.JLabel();
        exposure_meter_shutter_label_LB = new javax.swing.JLabel();
        exposure_meter_shutter_LB = new javax.swing.JLabel();
        shutter_400_LB = new javax.swing.JLabel();
        shutter_400_label_LB = new javax.swing.JLabel();
        shutter_400_open_BT = new javax.swing.JButton();
        shutter_400_close_BT = new javax.swing.JButton();
        shutter_400_stop_BT = new javax.swing.JButton();
        grating_angle_LB = new javax.swing.JLabel();
        absolute_grating_angle_label_LB = new javax.swing.JLabel();
        absolute_grating_angle_go_BT = new javax.swing.JButton();
        exposure_meter_frequency_label_LB = new javax.swing.JLabel();
        exposure_meter_frequency_LB = new javax.swing.JLabel();
        exposure_meter_count_label_LB = new javax.swing.JLabel();
        exposure_meter_count_LB = new javax.swing.JLabel();
        exp_stop_BT = new javax.swing.JButton();
        coude_slit_camera_filter_label_LB = new javax.swing.JLabel();
        coude_slit_camera_filter_LB = new javax.swing.JLabel();
        dichroic_mirror_CBI = new javax.swing.JComboBox();
        spectral_filter_CBI = new javax.swing.JComboBox();
        collimator_CBI = new javax.swing.JComboBox();
        dichroic_mirror_stop_BT = new javax.swing.JButton();
        spectral_filter_stop_BT = new javax.swing.JButton();
        collimator_stop_BT = new javax.swing.JButton();
        exp_shutter_open_BT = new javax.swing.JButton();
        exp_shutter_close_BT = new javax.swing.JButton();
        exp_shutter_stop_BT = new javax.swing.JButton();
        coude_slit_camera_filter_CBI = new javax.swing.JComboBox();
        coude_slit_camera_filter_stop_BT = new javax.swing.JButton();
        correction_plates_700_lable_LB = new javax.swing.JLabel();
        correction_plates_700_LB = new javax.swing.JLabel();
        correction_plates_400_label_LB = new javax.swing.JLabel();
        correction_plates_400_LB = new javax.swing.JLabel();
        grating_LB = new javax.swing.JLabel();
        exp_start_BT = new javax.swing.JButton();
        grating_angle_stop_BT = new javax.swing.JButton();
        absolute_graiting_angle_TF = new javax.swing.JTextField();
        coude_slit_camera_power_label_LB = new javax.swing.JLabel();
        coude_slit_camera_power_LB = new javax.swing.JLabel();
        coude_slit_camera_power_on_BT = new javax.swing.JButton();
        coude_slit_camera_power_off_BT = new javax.swing.JButton();
        spectrograph_coude_invisible_IF = new javax.swing.JInternalFrame();
        shutter_700_label_LB = new javax.swing.JLabel();
        shutter_700_LB = new javax.swing.JLabel();
        shutter_700_open_BT = new javax.swing.JButton();
        shutter_700_close_BT = new javax.swing.JButton();
        shutter_700_stop_BT = new javax.swing.JButton();
        spectrograph_oes_P = new javax.swing.JPanel();
        exp_oes_shutter_label_LB = new javax.swing.JLabel();
        exp_oes_count_label_LB = new javax.swing.JLabel();
        exp_oes_frequency_label_LB = new javax.swing.JLabel();
        exp_oes_shutter_LB = new javax.swing.JLabel();
        exp_oes_count_LB = new javax.swing.JLabel();
        exp_oes_frequency_LB = new javax.swing.JLabel();
        exp_oes_open_BT = new javax.swing.JButton();
        exp_oes_stop_BT = new javax.swing.JButton();
        exp_oes_shutter_close_BT = new javax.swing.JButton();
        exp_oes_shutter_stop_BT = new javax.swing.JButton();
        oes_collimator_label_LB = new javax.swing.JLabel();
        oes_collimator_LB = new javax.swing.JLabel();
        oes_collimator_CBI = new javax.swing.JComboBox();
        oes_collimator_stop_BT = new javax.swing.JButton();
        exp_oes_start_BT = new javax.swing.JButton();
        oes_slit_height_label_LB = new javax.swing.JLabel();
        oes_spectral_filter_label_LB = new javax.swing.JLabel();
        oes_spectral_filter_CBI = new javax.swing.JComboBox();
        oes_slit_height_CBI = new javax.swing.JComboBox();
        oes_slit_camera_power_label_LB = new javax.swing.JLabel();
        oes_slit_camera_power_LB = new javax.swing.JLabel();
        oes_slit_camera_power_on_BT = new javax.swing.JButton();
        oes_slit_camera_power_off_BT = new javax.swing.JButton();
        spectrograph_focus_P = new javax.swing.JPanel();
        coude_focus_700_P = new javax.swing.JPanel();
        relative_focus_position_700_label_LB = new javax.swing.JLabel();
        absolute_focus_position_700_label_LB = new javax.swing.JLabel();
        focus_position_700_LB = new javax.swing.JLabel();
        focus_calibration_700_BT = new javax.swing.JButton();
        focus_stop_700_BT = new javax.swing.JButton();
        relative_focus_position_700_SN = new javax.swing.JSpinner();
        absolute_focus_position_700_SN = new javax.swing.JSpinner();
        relative_focus_position_700_BT = new javax.swing.JButton();
        absolute_focus_position_700_BT = new javax.swing.JButton();
        focus_700_LB = new javax.swing.JLabel();
        coude_focus_400_P = new javax.swing.JPanel();
        relative_focus_position_400_label_LB = new javax.swing.JLabel();
        absolute_focus_position_400_label_LB = new javax.swing.JLabel();
        focus_position_400_LB = new javax.swing.JLabel();
        focus_calibration_400_BT = new javax.swing.JButton();
        focus_stop_400_BT = new javax.swing.JButton();
        relative_focus_position_400_SN = new javax.swing.JSpinner();
        absolute_focus_position_400_SN = new javax.swing.JSpinner();
        relative_focus_position_400_BT = new javax.swing.JButton();
        absolute_focus_position_400_BT = new javax.swing.JButton();
        focus_400_LB = new javax.swing.JLabel();
        oes_focus_400_P = new javax.swing.JPanel();
        relative_focus_position_oes_label_LB = new javax.swing.JLabel();
        absolute_focus_position_oes_label_LB = new javax.swing.JLabel();
        focus_position_oes_LB = new javax.swing.JLabel();
        focus_calibration_oes_BT = new javax.swing.JButton();
        focus_stop_oes_BT = new javax.swing.JButton();
        relative_focus_position_oes_SN = new javax.swing.JSpinner();
        absolute_focus_position_oes_SN = new javax.swing.JSpinner();
        relative_focus_position_oes_BT = new javax.swing.JButton();
        absolute_focus_position_oes_BT = new javax.swing.JButton();
        focus_oes_LB = new javax.swing.JLabel();
        telescope_P = new javax.swing.JPanel();
        jTabbedPane3 = new javax.swing.JTabbedPane();
        telescope_control_P = new javax.swing.JPanel();
        telescope_star_P = new javax.swing.JPanel();
        telescope_ra_title_LB = new javax.swing.JLabel();
        telescope_dec_title_LB = new javax.swing.JLabel();
        telescope_position_title_LB = new javax.swing.JLabel();
        telescope_go_star_BT = new javax.swing.JButton();
        telescope_ra_LB = new javax.swing.JLabel();
        telescope_dec_LB = new javax.swing.JLabel();
        telescope_position_LB = new javax.swing.JLabel();
        telescope_ra_TF = new javax.swing.JTextField();
        telescope_dec_TF = new javax.swing.JTextField();
        telescope_position_CBI = new javax.swing.JComboBox();
        telescope_get_tsra_BT = new javax.swing.JButton();
        telescope_object_title_LB = new javax.swing.JLabel();
        telescope_object_LB = new javax.swing.JLabel();
        telescope_source_P = new javax.swing.JPanel();
        telescope_ha_title_LB = new javax.swing.JLabel();
        telescope_da_title_LB = new javax.swing.JLabel();
        telescope_go_source_BT = new javax.swing.JButton();
        telescope_ha_LB = new javax.swing.JLabel();
        telescope_da_LB = new javax.swing.JLabel();
        telescope_ha_TF = new javax.swing.JTextField();
        telescope_da_TF = new javax.swing.JTextField();
        telescope_time_P = new javax.swing.JPanel();
        telescope_utc_title_LB = new javax.swing.JLabel();
        telescope_utc_LB = new javax.swing.JLabel();
        telescope_lst_title_LB = new javax.swing.JLabel();
        telescope_lst_LB = new javax.swing.JLabel();
        telescope_hour_angle_title_LB = new javax.swing.JLabel();
        telescope_hour_angle_LB = new javax.swing.JLabel();
        telescope_azimuth_P = new javax.swing.JPanel();
        telescope_azimuth_title_LB = new javax.swing.JLabel();
        telescope_azimuth_LB = new javax.swing.JLabel();
        telescope_altitude_title_LB = new javax.swing.JLabel();
        telescope_altitude_LB = new javax.swing.JLabel();
        telescope_airmass_title_LB = new javax.swing.JLabel();
        telescope_airmass_LB = new javax.swing.JLabel();
        telescope_corrections_P = new javax.swing.JPanel();
        telescope_corrections_ra_title_LB = new javax.swing.JLabel();
        telescope_corrections_dec_title_LB = new javax.swing.JLabel();
        telescope_corrections_ra_SN = new javax.swing.JSpinner();
        telescope_corrections_dec_SN = new javax.swing.JSpinner();
        telescope_corrections_ra_LB = new javax.swing.JLabel();
        telescope_corrections_dec_LB = new javax.swing.JLabel();
        telescope_corrections_set_BT = new javax.swing.JButton();
        telescope_speed_P = new javax.swing.JPanel();
        telescope_speed_ra_title_LB = new javax.swing.JLabel();
        telescope_speed_dec_title_LB = new javax.swing.JLabel();
        telescope_speed_ra_SN = new javax.swing.JSpinner();
        telescope_speed_dec_SN = new javax.swing.JSpinner();
        telescope_speed_ra_LB = new javax.swing.JLabel();
        telescope_speed_dec_LB = new javax.swing.JLabel();
        telescope_speed_set_BT = new javax.swing.JButton();
        telescope_dome_P = new javax.swing.JPanel();
        telescope_dome_state_title_LB = new javax.swing.JLabel();
        telescope_dome_state_LB = new javax.swing.JLabel();
        telescope_dome_automatic_BT = new javax.swing.JButton();
        telescope_dome_stop_BT = new javax.swing.JButton();
        telescope_dome_calibration_BT = new javax.swing.JButton();
        telescope_dome_azimuth_title_LB = new javax.swing.JLabel();
        telescope_dome_azimuth_LB = new javax.swing.JLabel();
        telescope_dome_height_title_LB = new javax.swing.JLabel();
        telescope_dome_height_LB = new javax.swing.JLabel();
        telescope_dome_position_title_LB = new javax.swing.JLabel();
        telescope_dome_position_LB = new javax.swing.JLabel();
        telescope_dome_position_SN = new javax.swing.JSpinner();
        telescope_dome_relative_position_LB = new javax.swing.JLabel();
        telescope_dome_relative_position_SN = new javax.swing.JSpinner();
        telescope_dome_relative_position_BT = new javax.swing.JButton();
        telescope_dome_position_BT = new javax.swing.JButton();
        telescope_settings_P = new javax.swing.JPanel();
        jPanel13 = new javax.swing.JPanel();
        jLabel7 = new javax.swing.JLabel();
        ascol_focussing_state_LB = new javax.swing.JLabel();
        jLabel9 = new javax.swing.JLabel();
        ascol_focussing_calibration_LB = new javax.swing.JLabel();
        ascol_focussing_calibration_start_BT = new javax.swing.JButton();
        ascol_focussing_calibration_stop_BT = new javax.swing.JButton();
        jLabel11 = new javax.swing.JLabel();
        jLabel12 = new javax.swing.JLabel();
        ascol_focussing_abs_goto_BT = new javax.swing.JButton();
        ascol_focussing_rel_goto_BT = new javax.swing.JButton();
        ascol_focussing_rel_stop_BT = new javax.swing.JButton();
        ascol_focussing_abs_stop_BT = new javax.swing.JButton();
        ascol_focussing_position_value_LB = new javax.swing.JLabel();
        telescope_focussing_relative_SN = new javax.swing.JSpinner();
        telescope_focussing_absolute_SN = new javax.swing.JSpinner();
        jPanel6 = new javax.swing.JPanel();
        ascol_abberation_LB = new javax.swing.JLabel();
        ascol_abberation_on_BT = new javax.swing.JButton();
        ascol_abberation_off_BT = new javax.swing.JButton();
        ascol_abberatrion_value_LB = new javax.swing.JLabel();
        ascol_precesion_LB = new javax.swing.JLabel();
        ascol_precesion_on_BT = new javax.swing.JButton();
        ascol_precesion_off_BT = new javax.swing.JButton();
        ascol_precesion_value_LB = new javax.swing.JLabel();
        ascol_refraction_LB = new javax.swing.JLabel();
        ascol_refraction_on_BT = new javax.swing.JButton();
        ascol_refraction_off_BT = new javax.swing.JButton();
        ascol_refraction_value_LB = new javax.swing.JLabel();
        ascol_model_LB = new javax.swing.JLabel();
        ascol_model_on_BT = new javax.swing.JButton();
        ascol_model_off_BT = new javax.swing.JButton();
        ascol_model_value_LB = new javax.swing.JLabel();
        ascol_guide_mode_LB = new javax.swing.JLabel();
        ascol_guide_mode_on_BT = new javax.swing.JButton();
        ascol_guide_mode_off_BT = new javax.swing.JButton();
        ascol_guide_mode_value_LB = new javax.swing.JLabel();
        ascol_model_number_LB = new javax.swing.JLabel();
        ascol_model_CBI = new javax.swing.JComboBox();
        jPanel7 = new javax.swing.JPanel();
        ascol_mirror_LB = new javax.swing.JLabel();
        ascol_mirror_open_BT = new javax.swing.JButton();
        ascol_mirror_close_BT = new javax.swing.JButton();
        ascol_mirror_stop_BT = new javax.swing.JButton();
        ascol_mirror_value_LB = new javax.swing.JLabel();
        ascol_tube_LB = new javax.swing.JLabel();
        ascol_tube_open_BT = new javax.swing.JButton();
        ascol_tube_close_BT = new javax.swing.JButton();
        ascol_tube_stop_BT = new javax.swing.JButton();
        ascol_tube_value_LB = new javax.swing.JLabel();
        ascol_slith_LB = new javax.swing.JLabel();
        ascol_slith_open_BT = new javax.swing.JButton();
        ascol_slith_close_BT = new javax.swing.JButton();
        ascol_slith_stop_BT = new javax.swing.JButton();
        ascol_slith_value_LB = new javax.swing.JLabel();
        jPanel11 = new javax.swing.JPanel();
        ascol_tel_state_LB = new javax.swing.JLabel();
        ascol_stop_slew_BT = new javax.swing.JButton();
        ascol_telescope_on_BT = new javax.swing.JButton();
        ascol_telescope_off_BT = new javax.swing.JButton();
        ascol_ha_state_LB = new javax.swing.JLabel();
        ascol_ha_state_value_LB = new javax.swing.JLabel();
        ascol_dec_state_LB = new javax.swing.JLabel();
        ascol_dec_state_value_LB = new javax.swing.JLabel();
        ascol_tracking_LB = new javax.swing.JLabel();
        ascol_tracking_value_LB = new javax.swing.JLabel();
        ascol_ha_calibration_LB = new javax.swing.JLabel();
        ascol_dec_calibration_LB = new javax.swing.JLabel();
        ascol_ha_calibration_value_LB = new javax.swing.JLabel();
        ascol_tracking_on_BT = new javax.swing.JButton();
        ascol_tracking_off_BT = new javax.swing.JButton();
        ascol_ha_calibration_start_BT = new javax.swing.JButton();
        ascol_dec_calibration_start_BT = new javax.swing.JButton();
        ascol_ha_calibration_stop_BT = new javax.swing.JButton();
        ascol_dec_calibration_stop_BT = new javax.swing.JButton();
        ascol_dec_calibration_value_LB = new javax.swing.JLabel();
        ascol_state_value_LB = new javax.swing.JLabel();
        ascol_oil_LB = new javax.swing.JLabel();
        ascol_oil_on_BT = new javax.swing.JButton();
        ascol_oil_off_BT = new javax.swing.JButton();
        ascol_oil_value_LB = new javax.swing.JLabel();
        opso_P = new javax.swing.JPanel();
        jTabbedPane2 = new javax.swing.JTabbedPane();
        jPanel5 = new javax.swing.JPanel();
        opso_settings_P = new javax.swing.JPanel();
        opso_average_LB = new javax.swing.JLabel();
        opso_average_SL = new javax.swing.JSlider();
        opso_settings_LB = new javax.swing.JLabel();
        opso_settings_CB = new javax.swing.JComboBox();
        opso_settings_BT = new javax.swing.JButton();
        opso_cut_P = new javax.swing.JPanel();
        opso_cut_x_LB = new javax.swing.JLabel();
        opso_cut_y_LB = new javax.swing.JLabel();
        opso_cut_x_size_LB = new javax.swing.JLabel();
        opso_cut_y_size_LB = new javax.swing.JLabel();
        opso_cut_y_slith_size_LB = new javax.swing.JLabel();
        opso_cut_x_SL = new javax.swing.JSlider();
        opso_cut_y_SL = new javax.swing.JSlider();
        opso_cut_x_size_SL = new javax.swing.JSlider();
        opso_cut_y_size_SL = new javax.swing.JSlider();
        opso_cut_y_slith_size_SL = new javax.swing.JSlider();
        opso_histogram_CB = new javax.swing.JCheckBox();
        opso_show_cursor_cut_xy_CB = new javax.swing.JCheckBox();
        opso_show_fit_gaus_xy_CB = new javax.swing.JCheckBox();
        jSeparator1 = new javax.swing.JSeparator();
        opso_target_P = new javax.swing.JPanel();
        opso_target_x_LB = new javax.swing.JLabel();
        opso_target_y_LB = new javax.swing.JLabel();
        opso_target_x_SL = new javax.swing.JSlider();
        opso_target_y_SL = new javax.swing.JSlider();
        jSeparator2 = new javax.swing.JSeparator();
        opso_show_target_CB = new javax.swing.JCheckBox();
        jPanel8 = new javax.swing.JPanel();
        jPanel2 = new javax.swing.JPanel();
        opso_timeout_tsgc_LB = new javax.swing.JLabel();
        opso_permissible_deviation_LB = new javax.swing.JLabel();
        opso_sec_in_pix_LB = new javax.swing.JLabel();
        opso_min_intenzity_star_LB = new javax.swing.JLabel();
        opso_max_fwhm_star_LB = new javax.swing.JLabel();
        opso_corr_at_error_LB = new javax.swing.JLabel();
        opso_allowed_offset_LB = new javax.swing.JLabel();
        opso_timeout_allowed_offset_LB = new javax.swing.JLabel();
        jSpinner1 = new javax.swing.JSpinner();
        jSpinner2 = new javax.swing.JSpinner();
        jSpinner3 = new javax.swing.JSpinner();
        jSpinner4 = new javax.swing.JSpinner();
        jSpinner5 = new javax.swing.JSpinner();
        jSpinner6 = new javax.swing.JSpinner();
        jSpinner7 = new javax.swing.JSpinner();
        jSpinner8 = new javax.swing.JSpinner();
        jLabel1 = new javax.swing.JLabel();
        jLabel2 = new javax.swing.JLabel();
        jLabel3 = new javax.swing.JLabel();
        jLabel4 = new javax.swing.JLabel();
        jLabel5 = new javax.swing.JLabel();
        jLabel6 = new javax.swing.JLabel();
        jPanel4 = new javax.swing.JPanel();
        opso_slith_begin_LB = new javax.swing.JLabel();
        opso_slith_end_LB = new javax.swing.JLabel();
        opso_slith_begin_SL = new javax.swing.JSlider();
        opso_slith_end_SL = new javax.swing.JSlider();
        opso_show_cursor_slith_CB = new javax.swing.JCheckBox();
        jSeparator3 = new javax.swing.JSeparator();
        state_P = new javax.swing.JPanel();
        state_LB = new javax.swing.JLabel();
        state_PB = new javax.swing.JProgressBar();

        connect_P.setMinimumSize(new java.awt.Dimension(350, 25));
        connect_P.setLayout(new java.awt.GridBagLayout());

        host_LB.setText("Host:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        connect_P.add(host_LB, gridBagConstraints);

        port_LB.setText("Port:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        connect_P.add(port_LB, gridBagConstraints);

        username_LB.setText("Username:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        connect_P.add(username_LB, gridBagConstraints);

        password_LB.setText("Password:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        connect_P.add(password_LB, gridBagConstraints);

        host_TF.setMinimumSize(new java.awt.Dimension(4, 60));
        host_TF.setPreferredSize(new java.awt.Dimension(120, 19));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        connect_P.add(host_TF, gridBagConstraints);

        port_TF.setPreferredSize(new java.awt.Dimension(120, 19));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        connect_P.add(port_TF, gridBagConstraints);

        username_TF.setPreferredSize(new java.awt.Dimension(120, 19));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        connect_P.add(username_TF, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        connect_P.add(password_PF, gridBagConstraints);

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);
        setTitle("Observe");

        jPanel1.setLayout(new java.awt.GridBagLayout());

        main_P.setLayout(new java.awt.BorderLayout());

        services_status_P.setLayout(new java.awt.GridBagLayout());

        status_opso_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        status_opso_LB.setText("unknown");
        status_opso_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(status_opso_LB, gridBagConstraints);

        jLabel10.setText("OES:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(jLabel10, gridBagConstraints);

        jLabel13.setText("CCD400:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(jLabel13, gridBagConstraints);

        jLabel14.setText("Pointing system:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(jLabel14, gridBagConstraints);

        jLabel15.setText("Telescope:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(jLabel15, gridBagConstraints);

        status_telescope_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        status_telescope_LB.setText("unknown");
        status_telescope_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(status_telescope_LB, gridBagConstraints);

        status_ccd700_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        status_ccd700_LB.setText("unknown");
        status_ccd700_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(status_ccd700_LB, gridBagConstraints);

        status_oes_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        status_oes_LB.setText("unknown");
        status_oes_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(status_oes_LB, gridBagConstraints);

        log_P.setBorder(javax.swing.BorderFactory.createTitledBorder("Log"));
        log_P.setLayout(new java.awt.GridBagLayout());

        log_TP.setEditable(false);
        log_TP.setPreferredSize(new java.awt.Dimension(6, 200));
        log_SP.setViewportView(log_TP);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        log_P.add(log_SP, gridBagConstraints);

        clearLog_BT.setText("Clear");
        clearLog_BT.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                clearLog_BTActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        log_P.add(clearLog_BT, gridBagConstraints);

        disableLog_BT.setText("Disable");
        disableLog_BT.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                disableLog_BTActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        log_P.add(disableLog_BT, gridBagConstraints);

        enableLog_BT.setText("Enable");
        enableLog_BT.setEnabled(false);
        enableLog_BT.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                enableLog_BTActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        log_P.add(enableLog_BT, gridBagConstraints);

        execute_ascol_BT.setText("Execute ASCOL");
        execute_ascol_BT.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                execute_ascol_BTActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        log_P.add(execute_ascol_BT, gridBagConstraints);

        execute_ascol_TF.addKeyListener(new java.awt.event.KeyAdapter() {
            public void keyPressed(java.awt.event.KeyEvent evt) {
                execute_ascol_TFKeyPressed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        log_P.add(execute_ascol_TF, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        services_status_P.add(log_P, gridBagConstraints);

        status_ccd400_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        status_ccd400_LB.setText("unknown");
        status_ccd400_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(status_ccd400_LB, gridBagConstraints);

        jLabel20.setText("CCD700:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(jLabel20, gridBagConstraints);

        jLabel16.setText("Spectrograph:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(jLabel16, gridBagConstraints);

        status_spectrograph_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        status_spectrograph_LB.setText("unknown");
        status_spectrograph_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(status_spectrograph_LB, gridBagConstraints);

        deprecated_version_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        deprecated_version_LB.setText("unknown");
        deprecated_version_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        services_status_P.add(deprecated_version_LB, gridBagConstraints);

        jTabbedPane1.addTab("Status of services", services_status_P);

        observe_P.setLayout(new java.awt.GridBagLayout());

        observe_bottom_P.setLayout(new java.awt.GridBagLayout());

        observers_LB.setText("Observers:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_bottom_P.add(observers_LB, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_bottom_P.add(observers_TF, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 0, 0, 0);
        observe_P.add(observe_bottom_P, gridBagConstraints);

        observe_TP.setTabPlacement(javax.swing.JTabbedPane.BOTTOM);
        observe_TP.addChangeListener(new javax.swing.event.ChangeListener() {
            public void stateChanged(javax.swing.event.ChangeEvent evt) {
                observe_TPStateChanged(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        observe_P.add(observe_TP, gridBagConstraints);

        observe_top_P.setLayout(new java.awt.GridBagLayout());

        selected_ccd_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        selected_ccd_LB.setText("Selected CCD is unknown");
        selected_ccd_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_top_P.add(selected_ccd_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        observe_P.add(observe_top_P, gridBagConstraints);

        jTabbedPane1.addTab("Observe", observe_P);

        spectrograph_TB.setTabPlacement(javax.swing.JTabbedPane.BOTTOM);

        spectrograph_control_P.setLayout(new java.awt.GridBagLayout());

        spectrograph_main_P.setBorder(javax.swing.BorderFactory.createTitledBorder(""));
        spectrograph_main_P.setLayout(new java.awt.GridBagLayout());

        star_calibration_label_LB.setText("Star/Calibration:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_main_P.add(star_calibration_label_LB, gridBagConstraints);

        star_calibration_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        star_calibration_LB.setText("unknown");
        star_calibration_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_main_P.add(star_calibration_LB, gridBagConstraints);

        star_BT.setText("Star");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(star_BT, gridBagConstraints);

        calibration_BT.setText("Calibration");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(calibration_BT, gridBagConstraints);

        star_calibration_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(star_calibration_stop_BT, gridBagConstraints);

        coude_oes_label_LB.setText("Coude/OES:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_main_P.add(coude_oes_label_LB, gridBagConstraints);

        coude_oes_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        coude_oes_LB.setText("unknown");
        coude_oes_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_main_P.add(coude_oes_LB, gridBagConstraints);

        coude_BT.setText("Coude");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(coude_BT, gridBagConstraints);

        oes_BT.setText("OES");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(oes_BT, gridBagConstraints);

        coude_oes_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(coude_oes_stop_BT, gridBagConstraints);

        flat_label_LB.setText("Flat field:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_main_P.add(flat_label_LB, gridBagConstraints);

        flat_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        flat_LB.setText("unkwnon");
        flat_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_main_P.add(flat_LB, gridBagConstraints);

        comp_label_LB.setText("Comp:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_main_P.add(comp_label_LB, gridBagConstraints);

        comp_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        comp_LB.setText("unkwnon");
        comp_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_main_P.add(comp_LB, gridBagConstraints);

        flat_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(flat_on_BT, gridBagConstraints);

        flat_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(flat_off_BT, gridBagConstraints);

        comp_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(comp_on_BT, gridBagConstraints);

        comp_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_main_P.add(comp_off_BT, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        spectrograph_control_P.add(spectrograph_main_P, gridBagConstraints);

        spectrograph_coude_P.setLayout(new java.awt.GridBagLayout());

        dichroic_mirror_label_LB.setText("Dichroic mirror:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(dichroic_mirror_label_LB, gridBagConstraints);

        spectral_filter_label_LB.setText("Spectral filter:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(spectral_filter_label_LB, gridBagConstraints);

        collimator_label_LB.setText("Collimator:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(collimator_label_LB, gridBagConstraints);

        dichroic_mirror_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        dichroic_mirror_LB.setText("unknown");
        dichroic_mirror_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(dichroic_mirror_LB, gridBagConstraints);

        spectral_filter_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        spectral_filter_LB.setText("unknown");
        spectral_filter_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(spectral_filter_LB, gridBagConstraints);

        collimator_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        collimator_LB.setText("unknown");
        collimator_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(collimator_LB, gridBagConstraints);

        exposure_meter_shutter_label_LB.setText("EM shutter:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 11;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exposure_meter_shutter_label_LB, gridBagConstraints);

        exposure_meter_shutter_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        exposure_meter_shutter_LB.setText("unknown");
        exposure_meter_shutter_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 11;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exposure_meter_shutter_LB, gridBagConstraints);

        shutter_400_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        shutter_400_LB.setText("unknown");
        shutter_400_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(shutter_400_LB, gridBagConstraints);

        shutter_400_label_LB.setText("Shutter CCD 400/1400");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(shutter_400_label_LB, gridBagConstraints);

        shutter_400_open_BT.setText("Open");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_P.add(shutter_400_open_BT, gridBagConstraints);

        shutter_400_close_BT.setText("Close");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_P.add(shutter_400_close_BT, gridBagConstraints);

        shutter_400_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_P.add(shutter_400_stop_BT, gridBagConstraints);

        grating_angle_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        grating_angle_LB.setText("unknown");
        grating_angle_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(grating_angle_LB, gridBagConstraints);

        absolute_grating_angle_label_LB.setText("Angle of grating:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(absolute_grating_angle_label_LB, gridBagConstraints);

        absolute_grating_angle_go_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(absolute_grating_angle_go_BT, gridBagConstraints);

        exposure_meter_frequency_label_LB.setText("EM frequency of pulses:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 13;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exposure_meter_frequency_label_LB, gridBagConstraints);

        exposure_meter_frequency_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        exposure_meter_frequency_LB.setText("unknown");
        exposure_meter_frequency_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 13;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exposure_meter_frequency_LB, gridBagConstraints);

        exposure_meter_count_label_LB.setText("EM count of pulses:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 12;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exposure_meter_count_label_LB, gridBagConstraints);

        exposure_meter_count_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        exposure_meter_count_LB.setText("unknown");
        exposure_meter_count_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 12;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exposure_meter_count_LB, gridBagConstraints);

        exp_stop_BT.setText("Stop and Reset");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 12;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_P.add(exp_stop_BT, gridBagConstraints);

        coude_slit_camera_filter_label_LB.setText("Slit camera filter:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 9;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(coude_slit_camera_filter_label_LB, gridBagConstraints);

        coude_slit_camera_filter_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        coude_slit_camera_filter_LB.setText("unknown");
        coude_slit_camera_filter_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 9;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(coude_slit_camera_filter_LB, gridBagConstraints);

        dichroic_mirror_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "1", "2", "3", "4" }));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(dichroic_mirror_CBI, gridBagConstraints);

        spectral_filter_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "1", "2", "3", "4", "5" }));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(spectral_filter_CBI, gridBagConstraints);

        collimator_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "open", "open left", "open right", "closed" }));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(collimator_CBI, gridBagConstraints);

        dichroic_mirror_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(dichroic_mirror_stop_BT, gridBagConstraints);

        spectral_filter_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(spectral_filter_stop_BT, gridBagConstraints);

        collimator_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(collimator_stop_BT, gridBagConstraints);

        exp_shutter_open_BT.setText("Open");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 11;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exp_shutter_open_BT, gridBagConstraints);

        exp_shutter_close_BT.setText("Close");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 11;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exp_shutter_close_BT, gridBagConstraints);

        exp_shutter_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 11;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(exp_shutter_stop_BT, gridBagConstraints);

        coude_slit_camera_filter_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "1", "2", "3", "4", "5" }));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 9;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(coude_slit_camera_filter_CBI, gridBagConstraints);

        coude_slit_camera_filter_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 9;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(coude_slit_camera_filter_stop_BT, gridBagConstraints);

        correction_plates_700_lable_LB.setText("Correction plate 700:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(correction_plates_700_lable_LB, gridBagConstraints);

        correction_plates_700_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        correction_plates_700_LB.setText("unknown");
        correction_plates_700_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(correction_plates_700_LB, gridBagConstraints);

        correction_plates_400_label_LB.setText("Correction plate 400/1400:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(correction_plates_400_label_LB, gridBagConstraints);

        correction_plates_400_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        correction_plates_400_LB.setText("unknown");
        correction_plates_400_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(correction_plates_400_LB, gridBagConstraints);

        grating_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        grating_LB.setText("unknown");
        grating_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(grating_LB, gridBagConstraints);

        exp_start_BT.setText("Start");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 12;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_P.add(exp_start_BT, gridBagConstraints);

        grating_angle_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(grating_angle_stop_BT, gridBagConstraints);

        absolute_graiting_angle_TF.setHorizontalAlignment(javax.swing.JTextField.RIGHT);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(absolute_graiting_angle_TF, gridBagConstraints);

        coude_slit_camera_power_label_LB.setText("Slit camera power:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 10;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(coude_slit_camera_power_label_LB, gridBagConstraints);

        coude_slit_camera_power_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        coude_slit_camera_power_LB.setText("unknown");
        coude_slit_camera_power_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 10;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_P.add(coude_slit_camera_power_LB, gridBagConstraints);

        coude_slit_camera_power_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 10;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_P.add(coude_slit_camera_power_on_BT, gridBagConstraints);

        coude_slit_camera_power_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 10;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_P.add(coude_slit_camera_power_off_BT, gridBagConstraints);

        spectrograph_coude_invisible_IF.getContentPane().setLayout(new java.awt.GridBagLayout());

        shutter_700_label_LB.setText("Shutter CCD 700");
        shutter_700_label_LB.setEnabled(false);
        shutter_700_label_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_invisible_IF.getContentPane().add(shutter_700_label_LB, gridBagConstraints);

        shutter_700_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        shutter_700_LB.setText("unknown");
        shutter_700_LB.setEnabled(false);
        shutter_700_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_coude_invisible_IF.getContentPane().add(shutter_700_LB, gridBagConstraints);

        shutter_700_open_BT.setText("Open");
        shutter_700_open_BT.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_invisible_IF.getContentPane().add(shutter_700_open_BT, gridBagConstraints);

        shutter_700_close_BT.setText("Close");
        shutter_700_close_BT.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_invisible_IF.getContentPane().add(shutter_700_close_BT, gridBagConstraints);

        shutter_700_stop_BT.setText("Stop");
        shutter_700_stop_BT.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_coude_invisible_IF.getContentPane().add(shutter_700_stop_BT, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 14;
        gridBagConstraints.gridwidth = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        spectrograph_coude_P.add(spectrograph_coude_invisible_IF, gridBagConstraints);

        jTabbedPane4.addTab("Coude", spectrograph_coude_P);

        spectrograph_oes_P.setLayout(new java.awt.GridBagLayout());

        exp_oes_shutter_label_LB.setText("EM shutter:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_shutter_label_LB, gridBagConstraints);

        exp_oes_count_label_LB.setText("EM count of pulses:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_count_label_LB, gridBagConstraints);

        exp_oes_frequency_label_LB.setText("EM frequency of pulses:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_frequency_label_LB, gridBagConstraints);

        exp_oes_shutter_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        exp_oes_shutter_LB.setText("unkwnon");
        exp_oes_shutter_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_shutter_LB, gridBagConstraints);

        exp_oes_count_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        exp_oes_count_LB.setText("unkwnon");
        exp_oes_count_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_count_LB, gridBagConstraints);

        exp_oes_frequency_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        exp_oes_frequency_LB.setText("unkwnon");
        exp_oes_frequency_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_frequency_LB, gridBagConstraints);

        exp_oes_open_BT.setText("Open");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_open_BT, gridBagConstraints);

        exp_oes_stop_BT.setText("Stop and Reset");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_oes_P.add(exp_oes_stop_BT, gridBagConstraints);

        exp_oes_shutter_close_BT.setText("Close");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_shutter_close_BT, gridBagConstraints);

        exp_oes_shutter_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(exp_oes_shutter_stop_BT, gridBagConstraints);

        oes_collimator_label_LB.setText("Collimator:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_collimator_label_LB, gridBagConstraints);

        oes_collimator_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        oes_collimator_LB.setText("unkwnon");
        oes_collimator_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_collimator_LB, gridBagConstraints);

        oes_collimator_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "open", "open left", "open right", "closed" }));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_collimator_CBI, gridBagConstraints);

        oes_collimator_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_collimator_stop_BT, gridBagConstraints);

        exp_oes_start_BT.setText("Start");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_oes_P.add(exp_oes_start_BT, gridBagConstraints);

        oes_slit_height_label_LB.setText("Slit height:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_slit_height_label_LB, gridBagConstraints);

        oes_spectral_filter_label_LB.setText("Spectral filter:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_spectral_filter_label_LB, gridBagConstraints);

        oes_spectral_filter_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "0 - none", "2 - blue" }));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_spectral_filter_CBI, gridBagConstraints);

        oes_slit_height_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "0.22 mm", "0.5 mm", "1.07 mm", "2.02 mm", "3.1 mm" }));
        oes_slit_height_CBI.setSelectedIndex(2);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_slit_height_CBI, gridBagConstraints);

        oes_slit_camera_power_label_LB.setText("Slit camera power:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_slit_camera_power_label_LB, gridBagConstraints);

        oes_slit_camera_power_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        oes_slit_camera_power_LB.setText("unknown");
        oes_slit_camera_power_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        spectrograph_oes_P.add(oes_slit_camera_power_LB, gridBagConstraints);

        oes_slit_camera_power_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_oes_P.add(oes_slit_camera_power_on_BT, gridBagConstraints);

        oes_slit_camera_power_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        spectrograph_oes_P.add(oes_slit_camera_power_off_BT, gridBagConstraints);

        jTabbedPane4.addTab("OES", spectrograph_oes_P);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        spectrograph_control_P.add(jTabbedPane4, gridBagConstraints);

        spectrograph_TB.addTab("Control", spectrograph_control_P);

        spectrograph_focus_P.setLayout(new java.awt.GridBagLayout());

        coude_focus_700_P.setBorder(javax.swing.BorderFactory.createTitledBorder("Focus 700"));
        coude_focus_700_P.setLayout(new java.awt.GridBagLayout());

        relative_focus_position_700_label_LB.setText("Relative position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        coude_focus_700_P.add(relative_focus_position_700_label_LB, gridBagConstraints);

        absolute_focus_position_700_label_LB.setText("Absolute position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        coude_focus_700_P.add(absolute_focus_position_700_label_LB, gridBagConstraints);

        focus_position_700_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        focus_position_700_LB.setText("unkwnon");
        focus_position_700_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        coude_focus_700_P.add(focus_position_700_LB, gridBagConstraints);

        focus_calibration_700_BT.setText("Calibration");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_700_P.add(focus_calibration_700_BT, gridBagConstraints);

        focus_stop_700_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_700_P.add(focus_stop_700_BT, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_700_P.add(relative_focus_position_700_SN, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_700_P.add(absolute_focus_position_700_SN, gridBagConstraints);

        relative_focus_position_700_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_700_P.add(relative_focus_position_700_BT, gridBagConstraints);

        absolute_focus_position_700_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_700_P.add(absolute_focus_position_700_BT, gridBagConstraints);

        focus_700_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        focus_700_LB.setText("unkwnon");
        focus_700_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        coude_focus_700_P.add(focus_700_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        spectrograph_focus_P.add(coude_focus_700_P, gridBagConstraints);

        coude_focus_400_P.setBorder(javax.swing.BorderFactory.createTitledBorder("Focus 400/1400"));
        coude_focus_400_P.setLayout(new java.awt.GridBagLayout());

        relative_focus_position_400_label_LB.setText("Relative position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        coude_focus_400_P.add(relative_focus_position_400_label_LB, gridBagConstraints);

        absolute_focus_position_400_label_LB.setText("Absolute position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        coude_focus_400_P.add(absolute_focus_position_400_label_LB, gridBagConstraints);

        focus_position_400_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        focus_position_400_LB.setText("unkwnon");
        focus_position_400_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        coude_focus_400_P.add(focus_position_400_LB, gridBagConstraints);

        focus_calibration_400_BT.setText("Calibration");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_400_P.add(focus_calibration_400_BT, gridBagConstraints);

        focus_stop_400_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_400_P.add(focus_stop_400_BT, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_400_P.add(relative_focus_position_400_SN, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_400_P.add(absolute_focus_position_400_SN, gridBagConstraints);

        relative_focus_position_400_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_400_P.add(relative_focus_position_400_BT, gridBagConstraints);

        absolute_focus_position_400_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        coude_focus_400_P.add(absolute_focus_position_400_BT, gridBagConstraints);

        focus_400_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        focus_400_LB.setText("unkwnon");
        focus_400_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        coude_focus_400_P.add(focus_400_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        spectrograph_focus_P.add(coude_focus_400_P, gridBagConstraints);

        oes_focus_400_P.setBorder(javax.swing.BorderFactory.createTitledBorder("Focus OES"));
        oes_focus_400_P.setLayout(new java.awt.GridBagLayout());

        relative_focus_position_oes_label_LB.setText("Relative position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        oes_focus_400_P.add(relative_focus_position_oes_label_LB, gridBagConstraints);

        absolute_focus_position_oes_label_LB.setText("Absolute position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        oes_focus_400_P.add(absolute_focus_position_oes_label_LB, gridBagConstraints);

        focus_position_oes_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        focus_position_oes_LB.setText("unkwnon");
        focus_position_oes_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        oes_focus_400_P.add(focus_position_oes_LB, gridBagConstraints);

        focus_calibration_oes_BT.setText("Calibration");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        oes_focus_400_P.add(focus_calibration_oes_BT, gridBagConstraints);

        focus_stop_oes_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        oes_focus_400_P.add(focus_stop_oes_BT, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        oes_focus_400_P.add(relative_focus_position_oes_SN, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        oes_focus_400_P.add(absolute_focus_position_oes_SN, gridBagConstraints);

        relative_focus_position_oes_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        oes_focus_400_P.add(relative_focus_position_oes_BT, gridBagConstraints);

        absolute_focus_position_oes_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        oes_focus_400_P.add(absolute_focus_position_oes_BT, gridBagConstraints);

        focus_oes_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        focus_oes_LB.setText("unkwnon");
        focus_oes_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        oes_focus_400_P.add(focus_oes_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        spectrograph_focus_P.add(oes_focus_400_P, gridBagConstraints);

        spectrograph_TB.addTab("Focus", spectrograph_focus_P);

        jTabbedPane1.addTab("Spectrograph", spectrograph_TB);

        telescope_P.setLayout(new java.awt.GridBagLayout());

        jTabbedPane3.setTabPlacement(javax.swing.JTabbedPane.BOTTOM);

        telescope_control_P.setLayout(new java.awt.GridBagLayout());

        telescope_star_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Star coordinates:"));
        telescope_star_P.setLayout(new java.awt.GridBagLayout());

        telescope_ra_title_LB.setText("RA:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_ra_title_LB, gridBagConstraints);

        telescope_dec_title_LB.setText("DEC:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_dec_title_LB, gridBagConstraints);

        telescope_position_title_LB.setText("Position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_position_title_LB, gridBagConstraints);

        telescope_go_star_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_go_star_BT, gridBagConstraints);

        telescope_ra_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_ra_LB.setText("unknown");
        telescope_ra_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_star_P.add(telescope_ra_LB, gridBagConstraints);

        telescope_dec_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_dec_LB.setText("unknown");
        telescope_dec_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_star_P.add(telescope_dec_LB, gridBagConstraints);

        telescope_position_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_position_LB.setText("unknown");
        telescope_position_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_star_P.add(telescope_position_LB, gridBagConstraints);

        telescope_ra_TF.setHorizontalAlignment(javax.swing.JTextField.RIGHT);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_ra_TF, gridBagConstraints);

        telescope_dec_TF.setHorizontalAlignment(javax.swing.JTextField.RIGHT);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_dec_TF, gridBagConstraints);

        telescope_position_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "east", "western" }));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_position_CBI, gridBagConstraints);

        telescope_get_tsra_BT.setText("Get prepared star coordinates");
        telescope_get_tsra_BT.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                telescope_get_tsra_BTActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_get_tsra_BT, gridBagConstraints);

        telescope_object_title_LB.setText("Object:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_star_P.add(telescope_object_title_LB, gridBagConstraints);

        telescope_object_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_object_LB.setText("unknown");
        telescope_object_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_star_P.add(telescope_object_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_control_P.add(telescope_star_P, gridBagConstraints);

        telescope_source_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Source coordinates:"));
        telescope_source_P.setLayout(new java.awt.GridBagLayout());

        telescope_ha_title_LB.setText("H.A.:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_source_P.add(telescope_ha_title_LB, gridBagConstraints);

        telescope_da_title_LB.setText("D.A.:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_source_P.add(telescope_da_title_LB, gridBagConstraints);

        telescope_go_source_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_source_P.add(telescope_go_source_BT, gridBagConstraints);

        telescope_ha_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_ha_LB.setText("unknown");
        telescope_ha_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_source_P.add(telescope_ha_LB, gridBagConstraints);

        telescope_da_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_da_LB.setText("unknown");
        telescope_da_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_source_P.add(telescope_da_LB, gridBagConstraints);

        telescope_ha_TF.setHorizontalAlignment(javax.swing.JTextField.RIGHT);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_source_P.add(telescope_ha_TF, gridBagConstraints);

        telescope_da_TF.setHorizontalAlignment(javax.swing.JTextField.RIGHT);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_source_P.add(telescope_da_TF, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_control_P.add(telescope_source_P, gridBagConstraints);

        telescope_time_P.setBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)));
        telescope_time_P.setLayout(new java.awt.GridBagLayout());

        telescope_utc_title_LB.setText("UTC:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_time_P.add(telescope_utc_title_LB, gridBagConstraints);

        telescope_utc_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_utc_LB.setText("unknown");
        telescope_utc_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_time_P.add(telescope_utc_LB, gridBagConstraints);

        telescope_lst_title_LB.setText("LST:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_time_P.add(telescope_lst_title_LB, gridBagConstraints);

        telescope_lst_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_lst_LB.setText("unknown");
        telescope_lst_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_time_P.add(telescope_lst_LB, gridBagConstraints);

        telescope_hour_angle_title_LB.setText("HA:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_time_P.add(telescope_hour_angle_title_LB, gridBagConstraints);

        telescope_hour_angle_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_hour_angle_LB.setText("unknown");
        telescope_hour_angle_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_time_P.add(telescope_hour_angle_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_control_P.add(telescope_time_P, gridBagConstraints);

        telescope_azimuth_P.setBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)));
        telescope_azimuth_P.setLayout(new java.awt.GridBagLayout());

        telescope_azimuth_title_LB.setText("Azimuth:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_azimuth_P.add(telescope_azimuth_title_LB, gridBagConstraints);

        telescope_azimuth_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_azimuth_LB.setText("unknown");
        telescope_azimuth_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_azimuth_P.add(telescope_azimuth_LB, gridBagConstraints);

        telescope_altitude_title_LB.setText("Altitude:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_azimuth_P.add(telescope_altitude_title_LB, gridBagConstraints);

        telescope_altitude_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_altitude_LB.setText("unknown");
        telescope_altitude_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_azimuth_P.add(telescope_altitude_LB, gridBagConstraints);

        telescope_airmass_title_LB.setText("Airmass:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_azimuth_P.add(telescope_airmass_title_LB, gridBagConstraints);

        telescope_airmass_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_airmass_LB.setText("unknown");
        telescope_airmass_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_azimuth_P.add(telescope_airmass_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_control_P.add(telescope_azimuth_P, gridBagConstraints);

        telescope_corrections_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Corrections"));
        telescope_corrections_P.setLayout(new java.awt.GridBagLayout());

        telescope_corrections_ra_title_LB.setText("RA:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_corrections_P.add(telescope_corrections_ra_title_LB, gridBagConstraints);

        telescope_corrections_dec_title_LB.setText("DEC:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_corrections_P.add(telescope_corrections_dec_title_LB, gridBagConstraints);

        telescope_corrections_ra_SN.setModel(new javax.swing.SpinnerNumberModel(0, -3600, 3600, 1));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 20;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_corrections_P.add(telescope_corrections_ra_SN, gridBagConstraints);

        telescope_corrections_dec_SN.setModel(new javax.swing.SpinnerNumberModel(0, -3600, 3600, 1));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 20;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_corrections_P.add(telescope_corrections_dec_SN, gridBagConstraints);

        telescope_corrections_ra_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_corrections_ra_LB.setText("unknown");
        telescope_corrections_ra_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_corrections_P.add(telescope_corrections_ra_LB, gridBagConstraints);

        telescope_corrections_dec_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_corrections_dec_LB.setText("unknown");
        telescope_corrections_dec_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_corrections_P.add(telescope_corrections_dec_LB, gridBagConstraints);

        telescope_corrections_set_BT.setText("Set");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_corrections_P.add(telescope_corrections_set_BT, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_control_P.add(telescope_corrections_P, gridBagConstraints);

        telescope_speed_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Common speed"));
        telescope_speed_P.setLayout(new java.awt.GridBagLayout());

        telescope_speed_ra_title_LB.setText("RA:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_speed_P.add(telescope_speed_ra_title_LB, gridBagConstraints);

        telescope_speed_dec_title_LB.setText("DEC:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_speed_P.add(telescope_speed_dec_title_LB, gridBagConstraints);

        telescope_speed_ra_SN.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 20;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_speed_P.add(telescope_speed_ra_SN, gridBagConstraints);

        telescope_speed_dec_SN.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 20;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_speed_P.add(telescope_speed_dec_SN, gridBagConstraints);

        telescope_speed_ra_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_speed_ra_LB.setText("unknown");
        telescope_speed_ra_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_speed_P.add(telescope_speed_ra_LB, gridBagConstraints);

        telescope_speed_dec_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_speed_dec_LB.setText("unknown");
        telescope_speed_dec_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_speed_P.add(telescope_speed_dec_LB, gridBagConstraints);

        telescope_speed_set_BT.setText("Set");
        telescope_speed_set_BT.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_speed_P.add(telescope_speed_set_BT, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_control_P.add(telescope_speed_P, gridBagConstraints);

        telescope_dome_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Dome"));
        telescope_dome_P.setLayout(new java.awt.GridBagLayout());

        telescope_dome_state_title_LB.setText("State:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_state_title_LB, gridBagConstraints);

        telescope_dome_state_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_dome_state_LB.setText("unknown");
        telescope_dome_state_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_state_LB, gridBagConstraints);

        telescope_dome_automatic_BT.setText("Automatic");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_automatic_BT, gridBagConstraints);

        telescope_dome_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_stop_BT, gridBagConstraints);

        telescope_dome_calibration_BT.setText("Calibration");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_calibration_BT, gridBagConstraints);

        telescope_dome_azimuth_title_LB.setText("Azimuth:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_azimuth_title_LB, gridBagConstraints);

        telescope_dome_azimuth_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_dome_azimuth_LB.setText("unknown");
        telescope_dome_azimuth_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_azimuth_LB, gridBagConstraints);

        telescope_dome_height_title_LB.setText("Height on dome:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_height_title_LB, gridBagConstraints);

        telescope_dome_height_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_dome_height_LB.setText("unknown");
        telescope_dome_height_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_height_LB, gridBagConstraints);

        telescope_dome_position_title_LB.setText("Position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_position_title_LB, gridBagConstraints);

        telescope_dome_position_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        telescope_dome_position_LB.setText("unknown");
        telescope_dome_position_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_position_LB, gridBagConstraints);

        telescope_dome_position_SN.setModel(new javax.swing.SpinnerNumberModel(0.0d, 0.0d, 359.99d, 1.0d));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 20;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_position_SN, gridBagConstraints);

        telescope_dome_relative_position_LB.setText("Relative position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_relative_position_LB, gridBagConstraints);

        telescope_dome_relative_position_SN.setModel(new javax.swing.SpinnerNumberModel(0.0d, -179.99d, 180.0d, 1.0d));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 20;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_relative_position_SN, gridBagConstraints);

        telescope_dome_relative_position_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_relative_position_BT, gridBagConstraints);

        telescope_dome_position_BT.setText("GO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_dome_P.add(telescope_dome_position_BT, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_control_P.add(telescope_dome_P, gridBagConstraints);

        jTabbedPane3.addTab("Control", telescope_control_P);

        telescope_settings_P.setLayout(new java.awt.GridBagLayout());

        jPanel13.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Focus"));
        jPanel13.setLayout(new java.awt.GridBagLayout());

        jLabel7.setText("Position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(jLabel7, gridBagConstraints);

        ascol_focussing_state_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_focussing_state_LB.setText("unknown");
        ascol_focussing_state_LB.setAlignmentX(0.5F);
        ascol_focussing_state_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_focussing_state_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_focussing_state_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(ascol_focussing_state_LB, gridBagConstraints);

        jLabel9.setText("Calibration:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(jLabel9, gridBagConstraints);

        ascol_focussing_calibration_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_focussing_calibration_LB.setText("unknown");
        ascol_focussing_calibration_LB.setAlignmentX(0.5F);
        ascol_focussing_calibration_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_focussing_calibration_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_focussing_calibration_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(ascol_focussing_calibration_LB, gridBagConstraints);

        ascol_focussing_calibration_start_BT.setText("Start");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(ascol_focussing_calibration_start_BT, gridBagConstraints);

        ascol_focussing_calibration_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(ascol_focussing_calibration_stop_BT, gridBagConstraints);

        jLabel11.setText("Relative position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(jLabel11, gridBagConstraints);

        jLabel12.setText("Absolute position:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel13.add(jLabel12, gridBagConstraints);

        ascol_focussing_abs_goto_BT.setText("GOTO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel13.add(ascol_focussing_abs_goto_BT, gridBagConstraints);

        ascol_focussing_rel_goto_BT.setText("GOTO");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(ascol_focussing_rel_goto_BT, gridBagConstraints);

        ascol_focussing_rel_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(ascol_focussing_rel_stop_BT, gridBagConstraints);

        ascol_focussing_abs_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel13.add(ascol_focussing_abs_stop_BT, gridBagConstraints);

        ascol_focussing_position_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_focussing_position_value_LB.setText("unknown");
        ascol_focussing_position_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(ascol_focussing_position_value_LB, gridBagConstraints);

        telescope_focussing_relative_SN.setModel(new javax.swing.SpinnerNumberModel(0.0d, -49.0d, 49.0d, 1.0d));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(telescope_focussing_relative_SN, gridBagConstraints);

        telescope_focussing_absolute_SN.setModel(new javax.swing.SpinnerNumberModel(0.0d, 0.0d, 49.0d, 1.0d));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel13.add(telescope_focussing_absolute_SN, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        telescope_settings_P.add(jPanel13, gridBagConstraints);

        jPanel6.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Correction"));
        jPanel6.setLayout(new java.awt.GridBagLayout());

        ascol_abberation_LB.setText("Abberation:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_abberation_LB, gridBagConstraints);

        ascol_abberation_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_abberation_on_BT, gridBagConstraints);

        ascol_abberation_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_abberation_off_BT, gridBagConstraints);

        ascol_abberatrion_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_abberatrion_value_LB.setText("unknown");
        ascol_abberatrion_value_LB.setAlignmentX(0.5F);
        ascol_abberatrion_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_abberatrion_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_abberatrion_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 5;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_abberatrion_value_LB, gridBagConstraints);

        ascol_precesion_LB.setText("Precesion and nutation:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_precesion_LB, gridBagConstraints);

        ascol_precesion_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_precesion_on_BT, gridBagConstraints);

        ascol_precesion_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_precesion_off_BT, gridBagConstraints);

        ascol_precesion_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_precesion_value_LB.setText("unknown");
        ascol_precesion_value_LB.setAlignmentX(0.5F);
        ascol_precesion_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_precesion_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_precesion_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 5;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_precesion_value_LB, gridBagConstraints);

        ascol_refraction_LB.setText("Refraction:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_refraction_LB, gridBagConstraints);

        ascol_refraction_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_refraction_on_BT, gridBagConstraints);

        ascol_refraction_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_refraction_off_BT, gridBagConstraints);

        ascol_refraction_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_refraction_value_LB.setText("unknown");
        ascol_refraction_value_LB.setAlignmentX(0.5F);
        ascol_refraction_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_refraction_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_refraction_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 5;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_refraction_value_LB, gridBagConstraints);

        ascol_model_LB.setText("Deviation model:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_model_LB, gridBagConstraints);

        ascol_model_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_model_on_BT, gridBagConstraints);

        ascol_model_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_model_off_BT, gridBagConstraints);

        ascol_model_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_model_value_LB.setText("unknown");
        ascol_model_value_LB.setAlignmentX(0.5F);
        ascol_model_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_model_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_model_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 5;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_model_value_LB, gridBagConstraints);

        ascol_guide_mode_LB.setText("Guide mode:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel6.add(ascol_guide_mode_LB, gridBagConstraints);

        ascol_guide_mode_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel6.add(ascol_guide_mode_on_BT, gridBagConstraints);

        ascol_guide_mode_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel6.add(ascol_guide_mode_off_BT, gridBagConstraints);

        ascol_guide_mode_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_guide_mode_value_LB.setText("unknown");
        ascol_guide_mode_value_LB.setAlignmentX(0.5F);
        ascol_guide_mode_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_guide_mode_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_guide_mode_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 5;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel6.add(ascol_guide_mode_value_LB, gridBagConstraints);

        ascol_model_number_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_model_number_LB.setText("unknown");
        ascol_model_number_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_model_number_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_model_number_LB, gridBagConstraints);

        ascol_model_CBI.setModel(new javax.swing.DefaultComboBoxModel(new String[] { "0", "1", "2", "3", "4" }));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel6.add(ascol_model_CBI, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_settings_P.add(jPanel6, gridBagConstraints);

        jPanel7.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Apertures"));
        jPanel7.setLayout(new java.awt.GridBagLayout());

        ascol_mirror_LB.setText("Mirror:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_mirror_LB, gridBagConstraints);

        ascol_mirror_open_BT.setText("Open");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_mirror_open_BT, gridBagConstraints);

        ascol_mirror_close_BT.setText("Close");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_mirror_close_BT, gridBagConstraints);

        ascol_mirror_stop_BT.setText("Stop");
        ascol_mirror_stop_BT.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_mirror_stop_BT, gridBagConstraints);

        ascol_mirror_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_mirror_value_LB.setText("unknown");
        ascol_mirror_value_LB.setAlignmentX(0.5F);
        ascol_mirror_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_mirror_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_mirror_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_mirror_value_LB, gridBagConstraints);

        ascol_tube_LB.setText("Tube:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_tube_LB, gridBagConstraints);

        ascol_tube_open_BT.setText("Open");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_tube_open_BT, gridBagConstraints);

        ascol_tube_close_BT.setText("Close");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_tube_close_BT, gridBagConstraints);

        ascol_tube_stop_BT.setText("Stop");
        ascol_tube_stop_BT.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_tube_stop_BT, gridBagConstraints);

        ascol_tube_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_tube_value_LB.setText("unknown");
        ascol_tube_value_LB.setAlignmentX(0.5F);
        ascol_tube_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_tube_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_tube_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel7.add(ascol_tube_value_LB, gridBagConstraints);

        ascol_slith_LB.setText("Slit:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel7.add(ascol_slith_LB, gridBagConstraints);

        ascol_slith_open_BT.setText("Open");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel7.add(ascol_slith_open_BT, gridBagConstraints);

        ascol_slith_close_BT.setText("Close");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel7.add(ascol_slith_close_BT, gridBagConstraints);

        ascol_slith_stop_BT.setText("Stop");
        ascol_slith_stop_BT.setEnabled(false);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_END;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel7.add(ascol_slith_stop_BT, gridBagConstraints);

        ascol_slith_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_slith_value_LB.setText("unknown");
        ascol_slith_value_LB.setAlignmentX(0.5F);
        ascol_slith_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_slith_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_slith_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel7.add(ascol_slith_value_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_settings_P.add(jPanel7, gridBagConstraints);

        jPanel11.setBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)));
        jPanel11.setLayout(new java.awt.GridBagLayout());

        ascol_tel_state_LB.setText("State:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_tel_state_LB, gridBagConstraints);

        ascol_stop_slew_BT.setText("Stop slew");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_stop_slew_BT, gridBagConstraints);

        ascol_telescope_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_telescope_on_BT, gridBagConstraints);

        ascol_telescope_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 5;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_telescope_off_BT, gridBagConstraints);

        ascol_ha_state_LB.setText("HA:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_ha_state_LB, gridBagConstraints);

        ascol_ha_state_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_ha_state_value_LB.setText("unknown");
        ascol_ha_state_value_LB.setAlignmentX(0.5F);
        ascol_ha_state_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_ha_state_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_ha_state_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_ha_state_value_LB, gridBagConstraints);

        ascol_dec_state_LB.setText("DEC:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_dec_state_LB, gridBagConstraints);

        ascol_dec_state_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_dec_state_value_LB.setText("unknown");
        ascol_dec_state_value_LB.setAlignmentX(0.5F);
        ascol_dec_state_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_dec_state_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_dec_state_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_dec_state_value_LB, gridBagConstraints);

        ascol_tracking_LB.setText("Tracking:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_tracking_LB, gridBagConstraints);

        ascol_tracking_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_tracking_value_LB.setText("unknown");
        ascol_tracking_value_LB.setAlignmentX(0.5F);
        ascol_tracking_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_tracking_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_tracking_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_tracking_value_LB, gridBagConstraints);

        ascol_ha_calibration_LB.setText("Hour axis calibration:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_ha_calibration_LB, gridBagConstraints);

        ascol_dec_calibration_LB.setText("Declination axis calibration:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel11.add(ascol_dec_calibration_LB, gridBagConstraints);

        ascol_ha_calibration_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_ha_calibration_value_LB.setText("unknown");
        ascol_ha_calibration_value_LB.setAlignmentX(0.5F);
        ascol_ha_calibration_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_ha_calibration_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_ha_calibration_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_ha_calibration_value_LB, gridBagConstraints);

        ascol_tracking_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_tracking_on_BT, gridBagConstraints);

        ascol_tracking_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_tracking_off_BT, gridBagConstraints);

        ascol_ha_calibration_start_BT.setText("Start");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_ha_calibration_start_BT, gridBagConstraints);

        ascol_dec_calibration_start_BT.setText("Start");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel11.add(ascol_dec_calibration_start_BT, gridBagConstraints);

        ascol_ha_calibration_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_ha_calibration_stop_BT, gridBagConstraints);

        ascol_dec_calibration_stop_BT.setText("Stop");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel11.add(ascol_dec_calibration_stop_BT, gridBagConstraints);

        ascol_dec_calibration_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_dec_calibration_value_LB.setText("unknown");
        ascol_dec_calibration_value_LB.setAlignmentX(0.5F);
        ascol_dec_calibration_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_dec_calibration_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_dec_calibration_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel11.add(ascol_dec_calibration_value_LB, gridBagConstraints);

        ascol_state_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_state_value_LB.setText("unknown");
        ascol_state_value_LB.setAlignmentX(0.5F);
        ascol_state_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_state_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_state_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_state_value_LB, gridBagConstraints);

        ascol_oil_LB.setText("Oil:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel11.add(ascol_oil_LB, gridBagConstraints);

        ascol_oil_on_BT.setText("ON");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel11.add(ascol_oil_on_BT, gridBagConstraints);

        ascol_oil_off_BT.setText("OFF");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel11.add(ascol_oil_off_BT, gridBagConstraints);

        ascol_oil_value_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ascol_oil_value_LB.setText("unknown");
        ascol_oil_value_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(0, 0, 0, 0));
        ascol_oil_value_LB.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
        ascol_oil_value_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        jPanel11.add(ascol_oil_value_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        telescope_settings_P.add(jPanel11, gridBagConstraints);

        jTabbedPane3.addTab("Settings", telescope_settings_P);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        telescope_P.add(jTabbedPane3, gridBagConstraints);

        jTabbedPane1.addTab("Telescope", telescope_P);

        opso_P.setLayout(new java.awt.BorderLayout());

        jTabbedPane2.setTabPlacement(javax.swing.JTabbedPane.BOTTOM);

        jPanel5.setLayout(new java.awt.GridBagLayout());

        opso_settings_P.setBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)));
        opso_settings_P.setLayout(new java.awt.GridBagLayout());

        opso_average_LB.setText("Average:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_settings_P.add(opso_average_LB, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_settings_P.add(opso_average_SL, gridBagConstraints);

        opso_settings_LB.setText("Settings:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_settings_P.add(opso_settings_LB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        opso_settings_P.add(opso_settings_CB, gridBagConstraints);

        opso_settings_BT.setText("Apply");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_settings_P.add(opso_settings_BT, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel5.add(opso_settings_P, gridBagConstraints);

        opso_cut_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Cut"));
        opso_cut_P.setLayout(new java.awt.GridBagLayout());

        opso_cut_x_LB.setText("Cut X:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_cut_P.add(opso_cut_x_LB, gridBagConstraints);

        opso_cut_y_LB.setText("Cut Y:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_cut_P.add(opso_cut_y_LB, gridBagConstraints);

        opso_cut_x_size_LB.setText("Cut X size:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_cut_P.add(opso_cut_x_size_LB, gridBagConstraints);

        opso_cut_y_size_LB.setText("Cut Y size:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_cut_P.add(opso_cut_y_size_LB, gridBagConstraints);

        opso_cut_y_slith_size_LB.setText("Cut Y slit size:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_cut_P.add(opso_cut_y_slith_size_LB, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_cut_P.add(opso_cut_x_SL, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_cut_P.add(opso_cut_y_SL, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_cut_P.add(opso_cut_x_size_SL, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_cut_P.add(opso_cut_y_size_SL, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_cut_P.add(opso_cut_y_slith_size_SL, gridBagConstraints);

        opso_histogram_CB.setText("All lines");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_cut_P.add(opso_histogram_CB, gridBagConstraints);

        opso_show_cursor_cut_xy_CB.setText("Show cursor cut X/Y");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_cut_P.add(opso_show_cursor_cut_xy_CB, gridBagConstraints);

        opso_show_fit_gaus_xy_CB.setText("Show gaus fitting");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_cut_P.add(opso_show_fit_gaus_xy_CB, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        opso_cut_P.add(jSeparator1, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel5.add(opso_cut_P, gridBagConstraints);

        opso_target_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Target"));
        opso_target_P.setLayout(new java.awt.GridBagLayout());

        opso_target_x_LB.setText("Target X:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_target_P.add(opso_target_x_LB, gridBagConstraints);

        opso_target_y_LB.setText("Target Y:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_target_P.add(opso_target_y_LB, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_target_P.add(opso_target_x_SL, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        opso_target_P.add(opso_target_y_SL, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        opso_target_P.add(jSeparator2, gridBagConstraints);

        opso_show_target_CB.setText("Show Target");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        opso_target_P.add(opso_show_target_CB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel5.add(opso_target_P, gridBagConstraints);

        jTabbedPane2.addTab("Settings", jPanel5);

        jPanel8.setLayout(new java.awt.GridBagLayout());

        jPanel2.setBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)));
        jPanel2.setLayout(new java.awt.GridBagLayout());

        opso_timeout_tsgc_LB.setText("Timeout TSGC:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel2.add(opso_timeout_tsgc_LB, gridBagConstraints);

        opso_permissible_deviation_LB.setText("Permissible deviation:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel2.add(opso_permissible_deviation_LB, gridBagConstraints);

        opso_sec_in_pix_LB.setText("1 pixel =");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel2.add(opso_sec_in_pix_LB, gridBagConstraints);

        opso_min_intenzity_star_LB.setText("Min. intenzity star:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel2.add(opso_min_intenzity_star_LB, gridBagConstraints);

        opso_max_fwhm_star_LB.setText("Max. FWHM star:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel2.add(opso_max_fwhm_star_LB, gridBagConstraints);

        opso_corr_at_error_LB.setText("Immediate corr. at error:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel2.add(opso_corr_at_error_LB, gridBagConstraints);

        opso_allowed_offset_LB.setText("Max. allowed offset:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel2.add(opso_allowed_offset_LB, gridBagConstraints);

        opso_timeout_allowed_offset_LB.setText("Timeout for max. allowed offset:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel2.add(opso_timeout_allowed_offset_LB, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel2.add(jSpinner1, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel2.add(jSpinner2, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel2.add(jSpinner3, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel2.add(jSpinner4, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel2.add(jSpinner5, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel2.add(jSpinner6, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel2.add(jSpinner7, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel2.add(jSpinner8, gridBagConstraints);

        jLabel1.setText("s");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        jPanel2.add(jLabel1, gridBagConstraints);

        jLabel2.setText("x 0.01\"");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        jPanel2.add(jLabel2, gridBagConstraints);

        jLabel3.setText("x 0.001\"");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        jPanel2.add(jLabel3, gridBagConstraints);

        jLabel4.setText("\"");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        jPanel2.add(jLabel4, gridBagConstraints);

        jLabel5.setText("\"");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        jPanel2.add(jLabel5, gridBagConstraints);

        jLabel6.setText("s");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        jPanel2.add(jLabel6, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel8.add(jPanel2, gridBagConstraints);

        jPanel4.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Slit"));
        jPanel4.setLayout(new java.awt.GridBagLayout());

        opso_slith_begin_LB.setText("Slit begin:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel4.add(opso_slith_begin_LB, gridBagConstraints);

        opso_slith_end_LB.setText("Slit end:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel4.add(opso_slith_end_LB, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        jPanel4.add(opso_slith_begin_SL, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 10);
        jPanel4.add(opso_slith_end_SL, gridBagConstraints);

        opso_show_cursor_slith_CB.setText("Show slit cursor");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 10, 5, 5);
        jPanel4.add(opso_show_cursor_slith_CB, gridBagConstraints);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel4.add(jSeparator3, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        jPanel8.add(jPanel4, gridBagConstraints);

        jTabbedPane2.addTab("Automation", jPanel8);

        opso_P.add(jTabbedPane2, java.awt.BorderLayout.CENTER);

        jTabbedPane1.addTab("Pointing system", opso_P);

        main_P.add(jTabbedPane1, java.awt.BorderLayout.CENTER);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        jPanel1.add(main_P, gridBagConstraints);

        getContentPane().add(jPanel1, java.awt.BorderLayout.PAGE_START);

        state_P.setLayout(new java.awt.GridBagLayout());

        state_LB.setHorizontalAlignment(javax.swing.SwingConstants.LEFT);
        state_LB.setText("unknown");
        state_LB.setBorder(javax.swing.BorderFactory.createEmptyBorder(5, 5, 5, 5));
        state_LB.setHorizontalTextPosition(javax.swing.SwingConstants.LEFT);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        state_P.add(state_LB, gridBagConstraints);

        state_PB.setBorder(javax.swing.BorderFactory.createEmptyBorder(5, 5, 5, 5));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        state_P.add(state_PB, gridBagConstraints);

        getContentPane().add(state_P, java.awt.BorderLayout.PAGE_END);

        pack();
    }// </editor-fold>//GEN-END:initComponents

    private void telescope_get_tsra_BTActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_telescope_get_tsra_BTActionPerformed
        int position = 0;
        
        telescope_object_LB.setText(this.tle_object);
        telescope_ra_TF.setText(this.tsra_ra);
        telescope_dec_TF.setText(this.tsra_dec);
        
        if ((this.tsra_position.length() > 0)
                && (this.tsra_position.charAt(0) == '1')) {
            position = 1;
        }
        
        telescope_position_CBI.setSelectedIndex(position);
}//GEN-LAST:event_telescope_get_tsra_BTActionPerformed

    private void observe_TPStateChanged(javax.swing.event.ChangeEvent evt) {//GEN-FIRST:event_observe_TPStateChanged
        String ccd_name = observe_TP.getTitleAt(observe_TP.getSelectedIndex());
        Color ccd_color = red_color;
        
        selected_ccd_LB.setText(String.format("%s is selected", ccd_name));
        
        if (ccd_name.equals("OES")) {
            ccd_color = gray59_color;
        } else if (ccd_name.equals("CCD400")) {
            ccd_color = gold2_color;
        } else if (ccd_name.equals("CCD700")) {
            ccd_color = aquamarine2_color;
        }
        
        selected_ccd_LB.setBackground(ccd_color);
}//GEN-LAST:event_observe_TPStateChanged

    private void enableLog_BTActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_enableLog_BTActionPerformed
        logEnabled = true;
        enableLog_BT.setEnabled(false);
        disableLog_BT.setEnabled(true);
}//GEN-LAST:event_enableLog_BTActionPerformed

    private void disableLog_BTActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_disableLog_BTActionPerformed
        logEnabled = false;
        disableLog_BT.setEnabled(false);
        enableLog_BT.setEnabled(true);
}//GEN-LAST:event_disableLog_BTActionPerformed

    private void clearLog_BTActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_clearLog_BTActionPerformed
        try {
            log_doc.remove(0, log_doc.getLength());
        } catch (BadLocationException e) {
            logger.log(Level.SEVERE, "clearLog_BTActionPerformed() failed", e);
        }
}//GEN-LAST:event_clearLog_BTActionPerformed

    private void execute_ascol_BTActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_execute_ascol_BTActionPerformed
        String command = execute_ascol_TF.getText().trim();

        execute_ascol_TF.setText("");

        if (command.length() < 3) {
            showWarningMessage(String.format("Unknown command '%s'.", command));
            return;
        }

        char instrument = command.charAt(0);
        command = command.substring(2);

        switch (instrument) {
            case 'S':
                callBacksFIFO.offer(new SpectrographExecuteCB(command));
                break;

            case 'T':
                callBacksFIFO.offer(new TelescopeExecuteCB(command));
                break;

            default:
                showWarningMessage(String.format("Unknown command '%s'.", command));
                break;
        }
    }//GEN-LAST:event_execute_ascol_BTActionPerformed

    private void execute_ascol_TFKeyPressed(java.awt.event.KeyEvent evt) {//GEN-FIRST:event_execute_ascol_TFKeyPressed
        int key = evt.getKeyCode();

        if (key == KeyEvent.VK_ENTER) {
            execute_ascol_BTActionPerformed(null);
        }
    }//GEN-LAST:event_execute_ascol_TFKeyPressed
    
    private void opso_slider_addChangeListener() {
        Object[][] sliders_names = {
            { opso_average_SL, "I_AVERAGE_SL" },
            { opso_cut_x_SL, "I_CUT_X_SL" },
            { opso_cut_x_size_SL, "I_CUT_X_SIZE_SL" },
            { opso_cut_y_SL, "I_CUT_Y_SL" },
            { opso_cut_y_size_SL, "I_CUT_Y_SIZE_SL" },
            { opso_cut_y_slith_size_SL, "I_CUT_Y_SLITH_SIZE_SL" },
            { opso_slith_begin_SL, "I_SLITH_BEGIN_SL" },
            { opso_slith_end_SL, "I_SLITH_END_SL" },
            { opso_target_x_SL, "I_TARGET_X_SL" },
            { opso_target_y_SL, "I_TARGET_Y_SL" },
        };

        for (int i = 0; i < sliders_names.length; ++i) {
            JSlider slider = (JSlider)sliders_names[i][0];

            slider.setName((String)sliders_names[i][1]);
            slider.addChangeListener(new ChangeListener() {
                public void stateChanged(ChangeEvent evt) {
                    JSlider slider = (JSlider)evt.getSource();

                    if (!slider.getValueIsAdjusting()) {
                        System.out.println("value = " + slider.getValue());

                        // TODO
                        //synchronized (callBacksFIFO) {
                        //    callBacksFIFO.offer(new TelescopeExecuteCB(
                        //        String.format("OPSO WI %s %d", slider.getName(), slider.getValue())));
                        //}
                    }
                }
            });
        }
    }

    private void spectrograph_button_addActionListener() {
        Object[][] buttons_commands = {
            { star_BT, "SPCH 6 1" },
            { calibration_BT, "SPCH 6 2" },
            { star_calibration_stop_BT, "SPCH 6 0" },
            { coude_BT, "SPCH 7 1" },
            { oes_BT, "SPCH 7 2" },
            { coude_oes_stop_BT, "SPCH 7 0" },
            { flat_on_BT, "SPCH 8 1" },
            { flat_off_BT, "SPCH 8 0" },
            { comp_on_BT, "SPCH 9 1" },
            { comp_off_BT, "SPCH 9 0" },
            { dichroic_mirror_stop_BT, "SPCH 1 0" },
            { spectral_filter_stop_BT, "SPCH 2 0" },
            { collimator_stop_BT, "SPCH 3 0" },
            { absolute_grating_angle_go_BT, "SPAP 13" },
            { grating_angle_stop_BT, "SPST 13" },
            { shutter_700_open_BT, "SPCH 11 1" },
            { shutter_700_close_BT, "SPCH 11 2" },
            { shutter_700_stop_BT, "SPCH 11 0" },
            { shutter_400_open_BT, "SPCH 12 1" },
            { shutter_400_close_BT, "SPCH 12 2" },
            { shutter_400_stop_BT, "SPCH 12 0" },
            { coude_slit_camera_filter_stop_BT, "SPCH 15 0" },
            { exp_shutter_open_BT, "SPCH 10 1" },
            { exp_shutter_close_BT, "SPCH 10 2" },
            { exp_shutter_stop_BT, "SPCH 10 0" },
            { exp_start_BT, "SSTE 14" },
            { exp_stop_BT, "SSPE 14" },
            { oes_collimator_stop_BT, "SPCH 21 0" },
            { exp_oes_open_BT, "SPCH 23 1" },
            { exp_oes_shutter_close_BT, "SPCH 23 2" },
            { exp_oes_shutter_stop_BT, "SPCH 23 0" },
            { exp_oes_start_BT, "SSTE 24" },
            { exp_oes_stop_BT, "SSPE 24" },
            { focus_calibration_700_BT, "SPCA 4" },
            { absolute_focus_position_700_BT, "SPAP 4" },
            { relative_focus_position_700_BT, "SPRP 4" },
            { focus_stop_700_BT, "SPST 4" },
            { focus_calibration_400_BT, "SPCA 5" },
            { absolute_focus_position_400_BT, "SPAP 5" },
            { relative_focus_position_400_BT, "SPRP 5" },
            { focus_stop_400_BT, "SPST 5" },
            { focus_calibration_oes_BT, "SPCA 22" },
            { absolute_focus_position_oes_BT, "SPAP 22" },
            { relative_focus_position_oes_BT, "SPRP 22" },
            { focus_stop_oes_BT, "SPST 22" },
            { coude_slit_camera_power_on_BT, "SPCH 27 1" },
            { coude_slit_camera_power_off_BT, "SPCH 27 0" },
            { oes_slit_camera_power_on_BT, "SPCH 28 1" },
            { oes_slit_camera_power_off_BT, "SPCH 28 0" },
        };

        for (int i = 0; i < buttons_commands.length; ++i) {
            JButton button = (JButton)buttons_commands[i][0];

            button.setActionCommand((String)buttons_commands[i][1]);
            button.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    String command = evt.getActionCommand();
                    Integer value = null;

                    if (evt.getActionCommand().equals("SPCH 1")) {
                        //dichroic_mirror_CBI
                        //Integer ra = (Integer)telescope_corrections_ra_SN.getValue();
                        //command = String.format("SPCH 1 %d", dec.intValue());
                    }
                    else if (evt.getActionCommand().equals("SPAP 4")) {
                        value = (Integer)absolute_focus_position_700_SN.getValue();
                    }
                    else if (evt.getActionCommand().equals("SPRP 4")) {
                        value = (Integer)relative_focus_position_700_SN.getValue();
                    }
                    else if (evt.getActionCommand().equals("SPAP 5")) {
                        value = (Integer)absolute_focus_position_400_SN.getValue();
                    }
                    else if (evt.getActionCommand().equals("SPRP 5")) {
                        value = (Integer)relative_focus_position_400_SN.getValue();
                    }
                    else if (evt.getActionCommand().equals("SPAP 22")) {
                        value = (Integer)absolute_focus_position_oes_SN.getValue();
                    }
                    else if (evt.getActionCommand().equals("SPRP 22")) {
                        value = (Integer)relative_focus_position_oes_SN.getValue();
                    }
                    else if (evt.getActionCommand().equals("SPAP 13")) {
                        try {
                            value = prepareCoudeGratingAngle(absolute_graiting_angle_TF.getText());
                        }
                        catch (Exception e) {
                            logger.log(Level.WARNING, "prepareCoudeGratingAngle() failed", e);
                            return;
                        }
                    }

                    if (value != null) {
                        command = String.format("%s %d", command, value.intValue());
                    }

                    synchronized (callBacksFIFO) {
                        callBacksFIFO.offer(new SpectrographExecuteCB(command));
                    }
                }
            });
        }
    }

    private void ascol_button_addActionListener() {
        Object[][] buttons_commands = {
            { ascol_abberation_off_BT, "TSCA 0" },
            { ascol_abberation_on_BT, "TSCA 1" },
            { ascol_dec_calibration_start_BT, "TEDC 1" },
            { ascol_dec_calibration_stop_BT, "TEDC 0" },
            { ascol_focussing_abs_goto_BT, "FOGA" },
            { ascol_focussing_abs_stop_BT, "FOST" },
            { ascol_focussing_calibration_start_BT, "FOCA" },
            { ascol_focussing_calibration_stop_BT, "FOST" },
            { ascol_focussing_rel_goto_BT, "FOGR" },
            { ascol_focussing_rel_stop_BT, "FOST" },
            { ascol_guide_mode_off_BT, "TSGM 0" },
            { ascol_guide_mode_on_BT, "TSGM 1" },
            { ascol_ha_calibration_start_BT, "TEHC 1" },
            { ascol_ha_calibration_stop_BT, "TEHC 0" },
            { ascol_mirror_close_BT, "FMOC 0" },
            { ascol_mirror_open_BT, "FMOC 1" },
            { ascol_mirror_stop_BT, "" },
            { ascol_model_off_BT, "TSCM 0" },
            { ascol_model_on_BT, "TSCM 1" },
            { ascol_oil_off_BT, "OION 0" },
            { ascol_oil_on_BT, "OION 1" },
            { ascol_precesion_off_BT, "TSCP 0" },
            { ascol_precesion_on_BT, "TSCP 1" },
            { ascol_refraction_off_BT, "TSCR 0" },
            { ascol_refraction_on_BT, "TSCR 1" },
            { ascol_slith_close_BT, "DOSO 0" },
            { ascol_slith_open_BT, "DOSO 1" },
            { ascol_slith_stop_BT, "" },
            { ascol_stop_slew_BT, "TGRA 0" },
            { ascol_telescope_off_BT, "TEON 0" },
            { ascol_telescope_on_BT, "TEON 1" },
            { ascol_tracking_off_BT, "TETR 0" },
            { ascol_tracking_on_BT, "TETR 1" },
            { ascol_tube_close_BT, "FTOC 0" },
            { ascol_tube_open_BT, "FTOC 1" },
            { ascol_tube_stop_BT, "" },
            { telescope_dome_automatic_BT, "DOAM" },
            { telescope_dome_calibration_BT, "DOCA" },
            { telescope_dome_stop_BT, "DOST" },
            { telescope_go_star_BT, "TGRA 1" },
            { telescope_go_source_BT, "TGHA 1" },
            { telescope_corrections_set_BT, "TSGV" },
            { telescope_dome_position_BT, "DOGA" },
            { telescope_dome_relative_position_BT, "DOGR" },
        };

        for (int i = 0; i < buttons_commands.length; ++i) {
            JButton button = (JButton)buttons_commands[i][0];

            button.setActionCommand((String)buttons_commands[i][1]);
            button.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    String command = evt.getActionCommand();

                    if (evt.getActionCommand().equals("TGRA 1") && (!prepareTGRA())) {
                        return;
                    }
                    else if (evt.getActionCommand().equals("TGHA 1") && (!prepareTGHA())) {
                        return;
                    }
                    else if (evt.getActionCommand().equals("TSGV")) {
                        Integer ra = (Integer)telescope_corrections_ra_SN.getValue();
                        Integer dec = (Integer)telescope_corrections_dec_SN.getValue();
                        command = String.format("TSGV %d %d", ra.intValue(), dec.intValue());
                    }
                    else if (evt.getActionCommand().equals("DOGA")) {
                        Double value = (Double)telescope_dome_position_SN.getValue();
                        synchronized (callBacksFIFO) {
                            callBacksFIFO.offer(new TelescopeExecuteCB(
                                String.format("DOSA %.2f", value.doubleValue())));
                        }
                    }
                    else if (evt.getActionCommand().equals("DOGR")) {
                        Double value = (Double)telescope_dome_relative_position_SN.getValue();
                        synchronized (callBacksFIFO) {
                            callBacksFIFO.offer(new TelescopeExecuteCB(
                                String.format("DOSR %.2f", value.doubleValue())));
                        }
                    }
                    else if (evt.getActionCommand().equals("FOGA")) {
                        Double value = (Double)telescope_focussing_absolute_SN.getValue();
                        synchronized (callBacksFIFO) {
                            callBacksFIFO.offer(new TelescopeExecuteCB(
                                String.format("FOSA %.2f", value.doubleValue())));
                        }
                    }
                    else if (evt.getActionCommand().equals("FOGR")) {
                        Double value = (Double)telescope_focussing_relative_SN.getValue();
                        synchronized (callBacksFIFO) {
                            callBacksFIFO.offer(new TelescopeExecuteCB(
                                String.format("FOSR %.2f", value.doubleValue())));
                        }
                    }

                    synchronized (callBacksFIFO) {
                        callBacksFIFO.offer(new TelescopeExecuteCB(command));
                    }
                }
            });
        }
    }

    private void spectrograph_combobox_addActionListener() {
        Object[][] combobox_commands = {
            { dichroic_mirror_CBI, "SPCH 1" },
            { spectral_filter_CBI, "SPCH 2" },
            { coude_slit_camera_filter_CBI, "SPCH 15" },
            { collimator_CBI, "SPCH 3" },
            { oes_collimator_CBI, "SPCH 21" },
        };

        for (int i = 0; i < combobox_commands.length; ++i) {
            JComboBox combobox = (JComboBox)combobox_commands[i][0];

            combobox.setActionCommand((String)combobox_commands[i][1]);
            combobox.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    JComboBox cb = (JComboBox)evt.getSource();
                    String value = (String)cb.getSelectedItem();

                    if (evt.getActionCommand().equals("SPCH 3") ||
                        evt.getActionCommand().equals("SPCH 21")) {
                        if (value.equals("open")) {
                            value = "1";
                        }
                        else if (value.equals("closed")) {
                            value = "2";
                        }
                        else if (value.equals("open left")) {
                            value = "3";
                        }
                        else if (value.equals("open right")) {
                            value = "4";
                        }
                    }

                    synchronized (callBacksFIFO) {
                        callBacksFIFO.offer(new SpectrographExecuteCB(
                            String.format("%s %s", evt.getActionCommand(), value)));
                    }
                }
            });
        }
    }

    private void ascol_combobox_addActionListener() {
        Object[][] combobox_commands = {
            { ascol_model_CBI, "TSCS" },
        };

        for (int i = 0; i < combobox_commands.length; ++i) {
            JComboBox combobox = (JComboBox)combobox_commands[i][0];

            combobox.setActionCommand((String)combobox_commands[i][1]);
            combobox.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    JComboBox cb = (JComboBox)evt.getSource();
                    String value = (String)cb.getSelectedItem();

                    synchronized (callBacksFIFO) {
                        callBacksFIFO.offer(new TelescopeExecuteCB(
                            String.format("%s %s", evt.getActionCommand(), value)));
                    }
                }
            });
        }
    }

    public Integer prepareCoudeGratingAngle(String value) throws Exception {
        Matcher matcherDM;

        matcherDM = PATTERN_DM.matcher(value);
        if (!matcherDM.find()) {
            String msg = "Graiting angle must be [DD]D:[M]M[.MMM]";
            showWarningMessage(msg);
            throw new Exception(msg);
        }

        float degrees = Integer.parseInt(matcherDM.group(1));
        float minutes = Float.parseFloat(matcherDM.group(2));
        int increments;

        degrees += (minutes / 60);

        increments = Math.round(-205.294f * degrees + 12667.1f);
        System.out.println(increments);

        return new Integer(increments);
    }

    private boolean prepareTGRA() {
        Matcher matcherDDMMSS;
        String command;
        int position = 0;
        
        matcherDDMMSS = PATTERN_DDMMSS.matcher(telescope_ra_TF.getText());
        if (!matcherDDMMSS.find()) {
            showWarningMessage("RA must be [+/-]DDMMSS[.SSS]");
            return false;
        }

        matcherDDMMSS = PATTERN_DDMMSS.matcher(telescope_dec_TF.getText());
        if (!matcherDDMMSS.find()) {
            showWarningMessage("DEC must be [+/-]DDMMSS[.SSS]");
            return false;
        }

        if (telescope_position_CBI.getSelectedItem().toString().equals("western")) {
            position = 1;
        }

        command = String.format(
            "TSRA %s %s %d",
            telescope_ra_TF.getText(),
            telescope_dec_TF.getText(),
            position
        );

        synchronized (callBacksFIFO) {
            callBacksFIFO.offer(new TelescopeExecuteCB(command));
        }

        oes_P.set_exposure_target(telescope_object_LB.getText());
        ccd700_P.set_exposure_target(telescope_object_LB.getText());
        ccd400_P.set_exposure_target(telescope_object_LB.getText());
        
        return true;
    }

    private boolean prepareTGHA() {
        Matcher matcherDegrees;
        String command;
        int position = 0;
        double ha;
        double da;
        
        matcherDegrees = PATTERN_DEGREES.matcher(telescope_ha_TF.getText());
        if (!matcherDegrees.find()) {
            showWarningMessage("H.A. must be [+/-]D[DD][.DDDD]");
            return false;
        }

        matcherDegrees = PATTERN_DEGREES.matcher(telescope_da_TF.getText());
        if (!matcherDegrees.find()) {
            showWarningMessage("D.A. must be [+/-]D[DD][.DDDD]");
            return false;
        }

        ha = Double.parseDouble(telescope_ha_TF.getText());
        if ((ha < -180) || (ha > 330)) {
            showWarningMessage("H.A must be >= -180° and <= 330°");
            return false;
        }

        da = Double.parseDouble(telescope_da_TF.getText());
        if ((da < -90) || (da > 270)) {
            showWarningMessage("D.A must be >= -90° and <= 270°");
            return false;
        }

        command = String.format(
            "TSHA %s %s",
            telescope_ha_TF.getText(),
            telescope_da_TF.getText()
        );

        synchronized (callBacksFIFO) {
            callBacksFIFO.offer(new TelescopeExecuteCB(command));
        }

        return true;
    }

    private void observe_addActionListener() {
        // OES
        oes_P.exposure_start_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_start_BT_ActionListener(evt, oes_P);
            }
        });

        oes_P.exposure_readout_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_readout_BT_ActionListener(evt, oes_P);
            }
        });

        oes_P.exposure_abort_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_abort_BT_ActionListener(evt, oes_P);
            }
        });
        
        oes_P.exposure_control_update_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_control_update_BT_ActionListener(evt, oes_P);
            }
        });
        
        oes_P.exposure_setup_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_setup_BT_ActionListener(evt, oes_P);
            }
        });

        // CCD400
        ccd400_P.exposure_start_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_start_BT_ActionListener(evt, ccd400_P);
            }
        });

        ccd400_P.exposure_readout_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_readout_BT_ActionListener(evt, ccd400_P);
            }
        });

        ccd400_P.exposure_abort_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_abort_BT_ActionListener(evt, ccd400_P);
            }
        });
        
        ccd400_P.exposure_control_update_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_control_update_BT_ActionListener(evt, ccd400_P);
            }
        });
        
        ccd400_P.exposure_setup_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_setup_BT_ActionListener(evt, ccd400_P);
            }
        });
        
        ccd400_P.exposure_central_wavelength_SN_addChangeListener(new ChangeListener() {
            @Override
            public void stateChanged(ChangeEvent evt) {
                exposure_central_wavelength_SN_ChangeListener(evt, ccd400_P);
            }
        });
        
        ccd400_P.exposure_manual_setup_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_manual_setup_BT_ActionListener(evt, ccd400_P);
            }
        });

        // CCD700
        ccd700_P.exposure_start_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_start_BT_ActionListener(evt, ccd700_P);
            }
        });

        ccd700_P.exposure_readout_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_readout_BT_ActionListener(evt, ccd700_P);
            }
        });

        ccd700_P.exposure_abort_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_abort_BT_ActionListener(evt, ccd700_P);
            }
        });
        
        ccd700_P.exposure_control_update_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_control_update_BT_ActionListener(evt, ccd700_P);
            }
        });
        
        ccd700_P.exposure_central_wavelength_SN_addChangeListener(new ChangeListener() {
            @Override
            public void stateChanged(ChangeEvent evt) {
                exposure_central_wavelength_SN_ChangeListener(evt, ccd700_P);
            }
        });
        
        ccd700_P.exposure_manual_setup_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_manual_setup_BT_ActionListener(evt, ccd700_P);
            }
        });
        
        ccd700_P.exposure_setup_BT_addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                exposure_setup_BT_ActionListener(evt, ccd700_P);
            }
        });
    }

    public void showWarningMessage(String msg) {
        JOptionPane.showMessageDialog(
            this,
            msg,
            "Warning",
            JOptionPane.WARNING_MESSAGE);
    }
    
    public int showYesNoMessage(String msg) {
        int result = JOptionPane.showConfirmDialog(
            this,
            msg,
            "Question",
            JOptionPane.YES_NO_OPTION);
        
        return result;
    }

    private void exposure_start_BT_ActionListener(ActionEvent evt, ObservePanel observePanel) {
        Matcher matcher_observers = PATTERN_FITS_HDR.matcher(observers_TF.getText());
        if (!matcher_observers.find()) {
            showWarningMessage("Observers format must be:\n\n" +
                "- ASCII text characters ranging from hexadecimal 20 to 7E\n" +
                "- max length is 18 characters\n" +
                "- min length is 1 character");
            return;            
        }
        
        Matcher matcher_target = PATTERN_FITS_HDR.matcher(observePanel.getExposureTarget());
        if (!matcher_target.find()) {
            showWarningMessage("Target format must be:\n\n" +
                    "- ASCII text characters ranging from hexadecimal 20 to 7E\n" +
                    "- max length is 18 characters\n" +
                    "- min length is 1 character");
            return;            
        }
                
        observePanel.expose(callBacksFIFO, observers_TF.getText());
    }

    private void exposure_readout_BT_ActionListener(ActionEvent evt, ObservePanel observePanel) {
        synchronized (callBacksFIFO) {
            callBacksFIFO.offer(observePanel.createCallBack("expose_readout"));
        }
    }

    private void exposure_abort_BT_ActionListener(ActionEvent evt, ObservePanel observePanel) {
        synchronized (callBacksFIFO) {
            callBacksFIFO.offer(observePanel.createCallBack("expose_abort"));
        }
    }
    
    private void exposure_control_update_BT_ActionListener(ActionEvent evt, ObservePanel observePanel) {
        observePanel.exposure_control_update(callBacksFIFO);
    }
    
    private void exposure_setup_BT_ActionListener(ActionEvent evt, ObservePanel observePanel) {
        observePanel.exposure_setup(callBacksFIFO);
    }
    
    private void exposure_manual_setup_BT_ActionListener(ActionEvent evt, ObservePanel observePanel) {
        observePanel.exposure_manual_setup(callBacksFIFO);
    }
    
    private void exposure_central_wavelength_SN_ChangeListener(ChangeEvent evt, ObservePanel observePanel) {
        observePanel.exposure_calc_wavelength();
    }

    public void quit() {
        exit = true;
        logger.log(Level.INFO, "Exiting Observe client");
        handler.close();

        System.exit(0);
    }

    private void centerJFrame(JFrame frame) {
        Dimension frameSize = frame.getSize();

        int x = (screenSize.width / 4) - (frameSize.width / 2);
        int y = (screenSize.height / 2) - (frameSize.height / 2);

        frame.setLocation(x, y);
    }
    
    private void state_bit2text_color(int state_bit) {
        state_bit2text_color(state_bit, "off", "on");
    }

    private void state_bit2text_color(int state_bit, String state_0, String state_1) {
        if (state_bit == 0) {
            text = state_0;
            color = red_color;
        }
        else {
            text = state_1;
            color = green_color;
        }
    }

    public void setConnectState(ConnectState state) {
        state_LB.setText(state.getMsg());

        if (state.getState() == ConnectState.State.SUCCESS) {
            state_P.setBackground(green_color);
            state_PB.setIndeterminate(false);
            setEnabled(true);
        }
        else if (state.getState() == ConnectState.State.FAILURE) {
            state_P.setBackground(red_color);
            state_PB.setIndeterminate(false);
            setEnabled(false);
        }
        else if (state.getState() == ConnectState.State.CLOSE) {
            state_P.setBackground(red_color);
            state_PB.setIndeterminate(false);
            setEnabled(false);
        }
        else {
            state_P.setBackground(blue_color);
            state_PB.setIndeterminate(true);
            setEnabled(false);
        }
    }

    public JLabel getStatus_telescope_LB() {
        return this.status_telescope_LB;
    }

    public JLabel getStatus_oes_LB() {
        return this.status_oes_LB;
    }

    public JLabel getStatus_ccd400_LB() {
        return this.status_ccd400_LB;
    }

    public JLabel getStatus_ccd700_LB() {
        return this.status_ccd700_LB;
    }

    public JLabel getStatus_spectrograph_LB() {
        return this.status_spectrograph_LB;
    }

    public void setText(ObserveGuiSetText guiSetText) {
        JLabel jLabel = guiSetText.getJLabel();

        jLabel.setText(guiSetText.getText());

        if (guiSetText.getText().equals("available")) {
            jLabel.setBackground(green_color);
        }
        else {
            jLabel.setBackground(red_color);
        }
    }

    public void showConnectDialog() {
        Object[] options = {"Connect", "Quit"};

        host_TF.setText(connect.host);
        port_TF.setText(String.valueOf(connect.port));
        username_TF.setText(connect.username);
        password_PF.setText(connect.password);

        int result = JOptionPane.showOptionDialog(
            this,
            connect_P,
            "Connect",
            JOptionPane.OK_CANCEL_OPTION,
            JOptionPane.PLAIN_MESSAGE,
            null,
            options,
            options[0]
        );
         
        if (result == JOptionPane.OK_OPTION) {
            connect.host = host_TF.getText();
            connect.port = Integer.parseInt(port_TF.getText());
            connect.username = username_TF.getText();
            connect.password = new String(password_PF.getPassword());
            setEnabled(true);
        }
        else {
            quit();
        }
    }

    private void setLabelTextBg(JLabel label, String value) {
        label.setText(value);

        if (value.equals("unknown")) {
            label.setBackground(red_color);
        }
        else {
            label.setBackground(green_color);
        }
    }

    private void setLabelTextBg(JLabel label, double value) {
        setLabelTextBg(label, value, "");
    }

    private void setLabelTextBg(JLabel label, double value, String suffix) {
        if (Double.isNaN(value)) {
            label.setText("unknown");
            label.setBackground(red_color);
        }
        else {
            label.setText(String.format("%.2f%s", value, suffix));
            label.setBackground(green_color);
        }
    }

    private void setLabelTextBg(JLabel label, int value) {
        setLabelTextBg(label, value, "");
    }

    private void setLabelTextBg(JLabel label, int value, String suffix) {
        if (value == Integer.MAX_VALUE) {
            label.setText("unknown");
            label.setBackground(red_color);
        }
        else {
            label.setText(Integer.toString(value) + suffix);
            label.setBackground(green_color);
        }
    }

    public void telescopeRefresh(Telescope telescope) {
        tsra_ra = telescope.tsra_ra;
        tsra_dec = telescope.tsra_dec;
        tsra_position = telescope.tsra_position;
        tle_object = telescope.object;

        setLabelTextBg(telescope_utc_LB, telescope.utc);
        setLabelTextBg(telescope_lst_LB, telescope.lst);
        setLabelTextBg(telescope_airmass_LB, telescope.airmass);
        setLabelTextBg(telescope_azimuth_LB, telescope.azimuth);
        setLabelTextBg(telescope_altitude_LB, telescope.altitude);
        setLabelTextBg(telescope_ra_LB, telescope.ra);
        setLabelTextBg(telescope_dec_LB, telescope.dec);
        setLabelTextBg(telescope_position_LB, telescope.position);
        setLabelTextBg(telescope_ha_LB, telescope.ha);
        setLabelTextBg(telescope_da_LB, telescope.da);
        setLabelTextBg(telescope_hour_angle_LB, telescope.hourAngle);
        setLabelTextBg(telescope_corrections_ra_LB, telescope.correctionsRa);
        setLabelTextBg(telescope_corrections_dec_LB, telescope.correctionsDec);
        setLabelTextBg(telescope_speed_ra_LB, telescope.speedRa);
        setLabelTextBg(telescope_speed_dec_LB, telescope.speedDec);
        //setLabelTextBg(telescope_dome_azimuth_LB, telescope.domeAzimuth);
        setLabelTextBg(telescope_dome_position_LB, telescope.domePosition);
        setLabelTextBg(ascol_model_number_LB, telescope.modelNumber);
        setLabelTextBg(ascol_focussing_position_value_LB, telescope.sharping, "mm");

        telescope_dome_state_LB.setText(telescope.state_dome[1]);
        if (telescope.state_dome[0].startsWith("AUTO_")) {
            telescope_dome_state_LB.setBackground(green_color);
        }
        else {
            telescope_dome_state_LB.setBackground(red_color);
        }

        ascol_oil_value_LB.setText(telescope.state_oil[1]);
        if (telescope.state_oil[0].equals("ON")) {
            ascol_oil_value_LB.setBackground(green_color);
        }
        else {
            ascol_oil_value_LB.setBackground(red_color);
        }

        ascol_dec_state_value_LB.setText(telescope.state_dec[1]);
        if (telescope.state_dec[0].equals("POSITION")) {
            ascol_dec_state_value_LB.setBackground(green_color);
        }
        else if (telescope.state_dec[0].equals("MO_SLOWEST")) {
            ascol_dec_state_value_LB.setBackground(blue_color);
        }
        else {
            ascol_dec_state_value_LB.setBackground(red_color);
        }

        ascol_ha_state_value_LB.setText(telescope.state_ha[1]);
        if (telescope.state_ha[0].equals("POSITION")) {
            ascol_ha_state_value_LB.setBackground(green_color);
        }
        else if (telescope.state_ha[0].equals("MO_SLOWEST")) {
            ascol_ha_state_value_LB.setBackground(blue_color);
        }
        else {
            ascol_ha_state_value_LB.setBackground(red_color);
        }

        ascol_state_value_LB.setText(telescope.state_telescope[1]);
        if (telescope.state_telescope[0].equals("TRACK")) {
            ascol_state_value_LB.setBackground(green_color);
            ascol_tracking_value_LB.setBackground(green_color);
            ascol_tracking_value_LB.setText("on");
        }
        else {
            ascol_state_value_LB.setBackground(red_color);
            ascol_tracking_value_LB.setBackground(red_color);
            ascol_tracking_value_LB.setText("off");
        }

        ascol_tube_value_LB.setText(telescope.state_tube[1]);
        if (telescope.state_tube[0].equals("OPEN")) {
            ascol_tube_value_LB.setBackground(green_color);
        }
        else {
            ascol_tube_value_LB.setBackground(red_color);
        }

        ascol_mirror_value_LB.setText(telescope.state_speculum[1]);
        if (telescope.state_speculum[0].equals("OPEN")) {
            ascol_mirror_value_LB.setBackground(green_color);
        }
        else {
            ascol_mirror_value_LB.setBackground(red_color);
        }

        ascol_slith_value_LB.setText(telescope.state_slith[1]);
        if (telescope.state_slith[0].equals("OPEN")) {
            ascol_slith_value_LB.setBackground(green_color);
        }
        else {
            ascol_slith_value_LB.setBackground(red_color);
        }

        ascol_focussing_state_LB.setText(telescope.state_sharping[1]);
        if (telescope.state_sharping[0].equals("STOP")) {
            ascol_focussing_state_LB.setBackground(green_color);
        }
        else {
            ascol_focussing_state_LB.setBackground(red_color);
        }

        state_bit2text_color(telescope.state_bit_dec, "uncalibrated", "calibrated");
        ascol_dec_calibration_value_LB.setText(text);
        ascol_dec_calibration_value_LB.setBackground(color);

        state_bit2text_color(telescope.state_bit_ha, "uncalibrated", "calibrated");
        ascol_ha_calibration_value_LB.setText(text);
        ascol_ha_calibration_value_LB.setBackground(color);

        state_bit2text_color(telescope.state_bit_sharping, "uncalibrated", "calibrated");
        ascol_focussing_calibration_LB.setText(text);
        ascol_focussing_calibration_LB.setBackground(color);

        state_bit2text_color(telescope.state_bit_abberation);
        ascol_abberatrion_value_LB.setText(text);
        ascol_abberatrion_value_LB.setBackground(color);

        state_bit2text_color(telescope.state_bit_precesion);
        ascol_precesion_value_LB.setText(text);
        ascol_precesion_value_LB.setBackground(color);

        state_bit2text_color(telescope.state_bit_refraction);
        ascol_refraction_value_LB.setText(text);
        ascol_refraction_value_LB.setBackground(color);

        state_bit2text_color(telescope.state_bit_model);
        ascol_model_value_LB.setText(text);
        ascol_model_value_LB.setBackground(color);

        state_bit2text_color(telescope.state_bit_guide_mode);
        ascol_guide_mode_value_LB.setText(text);
        ascol_guide_mode_value_LB.setBackground(color);

        //speed_t3_value_LB.setText(telescope.speed_t3);
    }

    public void spectrographRefresh(Spectrograph spectrograph) {
        spectrograph.setJLabel(star_calibration_LB, Spectrograph.Element.STAR_CALIBRATION);
        spectrograph.setJLabel(coude_oes_LB, Spectrograph.Element.COUDE_OES);
        spectrograph.setJLabel(flat_LB, Spectrograph.Element.FLAT);
        spectrograph.setJLabel(comp_LB, Spectrograph.Element.COMP);
        spectrograph.setJLabel(dichroic_mirror_LB, Spectrograph.Element.DICHROIC_MIRROR);
        spectrograph.setJLabel(spectral_filter_LB, Spectrograph.Element.SPECTRAL_FILTER);
        spectrograph.setJLabel(collimator_LB, Spectrograph.Element.COLLIMATOR);
        spectrograph.setJLabel(exposure_meter_shutter_LB, Spectrograph.Element.EXP_SHUTTER);
        spectrograph.setJLabel(exposure_meter_count_LB, Spectrograph.Element.EXP_COUNT);
        spectrograph.setJLabel(exposure_meter_frequency_LB, Spectrograph.Element.EXP_FREQ);
        spectrograph.setJLabel(shutter_700_LB, Spectrograph.Element.CAM_700_SHUTTER);
        spectrograph.setJLabel(shutter_400_LB, Spectrograph.Element.CAM_400_SHUTTER);
        spectrograph.setJLabel(grating_angle_LB, Spectrograph.Element.GRATING_POS);
        spectrograph.setJLabel(grating_LB, Spectrograph.Element.GRATING);
        spectrograph.setJLabel(coude_slit_camera_filter_LB, Spectrograph.Element.SLITH_CAM);
        spectrograph.setJLabel(focus_700_LB, Spectrograph.Element.FOCUS_700);
        spectrograph.setJLabel(focus_400_LB, Spectrograph.Element.FOCUS_400);
        spectrograph.setJLabel(focus_oes_LB, Spectrograph.Element.FOCUS_OES);
        spectrograph.setJLabel(focus_position_700_LB, Spectrograph.Element.FOCUS_700_POS);
        spectrograph.setJLabel(focus_position_400_LB, Spectrograph.Element.FOCUS_400_POS);
        spectrograph.setJLabel(focus_position_oes_LB, Spectrograph.Element.FOCUS_OES_POS);
        spectrograph.setJLabel(correction_plates_700_LB, Spectrograph.Element.CORR_700);
        spectrograph.setJLabel(correction_plates_400_LB, Spectrograph.Element.CORR_400);
        spectrograph.setJLabel(oes_collimator_LB, Spectrograph.Element.COLLIMATOR_OES);
        spectrograph.setJLabel(exp_oes_shutter_LB, Spectrograph.Element.EXP_SHUTTER_OES);
        spectrograph.setJLabel(exp_oes_count_LB, Spectrograph.Element.EXP_OES_COUNT);
        spectrograph.setJLabel(exp_oes_frequency_LB, Spectrograph.Element.EXP_OES_FREQ);
        spectrograph.setJLabel(coude_slit_camera_power_LB, Spectrograph.Element.COUDE_SLIT_CAMERA_POWER);
        spectrograph.setJLabel(oes_slit_camera_power_LB, Spectrograph.Element.OES_SLIT_CAMERA_POWER);
    }

    public void addLog(ObserveGuiLog log) {
        SimpleDateFormat actual_time = new SimpleDateFormat("hh:mm:ss");

        if (logEnabled) {
            try {
               log_doc.insertString(log_doc.getLength(),
                    actual_time.format(new Date()) + " - " + log.getMsg() + "\n", log.getStyle());
            }
            catch (BadLocationException e) {
                logger.log(Level.SEVERE, "addLog() failed", e);
            }
        }
    }

    public String getOesSPECFILT() {
        String value = oes_spectral_filter_CBI.getSelectedItem().toString();

        return value.split(" ")[0];
    }

    public String getOesSLITHEIG() {
        String value = oes_slit_height_CBI.getSelectedItem().toString();

        return value.split(" ")[0];
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JButton absolute_focus_position_400_BT;
    private javax.swing.JSpinner absolute_focus_position_400_SN;
    private javax.swing.JLabel absolute_focus_position_400_label_LB;
    private javax.swing.JButton absolute_focus_position_700_BT;
    private javax.swing.JSpinner absolute_focus_position_700_SN;
    private javax.swing.JLabel absolute_focus_position_700_label_LB;
    private javax.swing.JButton absolute_focus_position_oes_BT;
    private javax.swing.JSpinner absolute_focus_position_oes_SN;
    private javax.swing.JLabel absolute_focus_position_oes_label_LB;
    private javax.swing.JTextField absolute_graiting_angle_TF;
    private javax.swing.JButton absolute_grating_angle_go_BT;
    private javax.swing.JLabel absolute_grating_angle_label_LB;
    private javax.swing.JLabel ascol_abberation_LB;
    private javax.swing.JButton ascol_abberation_off_BT;
    private javax.swing.JButton ascol_abberation_on_BT;
    private javax.swing.JLabel ascol_abberatrion_value_LB;
    private javax.swing.JLabel ascol_dec_calibration_LB;
    private javax.swing.JButton ascol_dec_calibration_start_BT;
    private javax.swing.JButton ascol_dec_calibration_stop_BT;
    private javax.swing.JLabel ascol_dec_calibration_value_LB;
    private javax.swing.JLabel ascol_dec_state_LB;
    private javax.swing.JLabel ascol_dec_state_value_LB;
    private javax.swing.JButton ascol_focussing_abs_goto_BT;
    private javax.swing.JButton ascol_focussing_abs_stop_BT;
    private javax.swing.JLabel ascol_focussing_calibration_LB;
    private javax.swing.JButton ascol_focussing_calibration_start_BT;
    private javax.swing.JButton ascol_focussing_calibration_stop_BT;
    private javax.swing.JLabel ascol_focussing_position_value_LB;
    private javax.swing.JButton ascol_focussing_rel_goto_BT;
    private javax.swing.JButton ascol_focussing_rel_stop_BT;
    private javax.swing.JLabel ascol_focussing_state_LB;
    private javax.swing.JLabel ascol_guide_mode_LB;
    private javax.swing.JButton ascol_guide_mode_off_BT;
    private javax.swing.JButton ascol_guide_mode_on_BT;
    private javax.swing.JLabel ascol_guide_mode_value_LB;
    private javax.swing.JLabel ascol_ha_calibration_LB;
    private javax.swing.JButton ascol_ha_calibration_start_BT;
    private javax.swing.JButton ascol_ha_calibration_stop_BT;
    private javax.swing.JLabel ascol_ha_calibration_value_LB;
    private javax.swing.JLabel ascol_ha_state_LB;
    private javax.swing.JLabel ascol_ha_state_value_LB;
    private javax.swing.JLabel ascol_mirror_LB;
    private javax.swing.JButton ascol_mirror_close_BT;
    private javax.swing.JButton ascol_mirror_open_BT;
    private javax.swing.JButton ascol_mirror_stop_BT;
    private javax.swing.JLabel ascol_mirror_value_LB;
    private javax.swing.JComboBox ascol_model_CBI;
    private javax.swing.JLabel ascol_model_LB;
    private javax.swing.JLabel ascol_model_number_LB;
    private javax.swing.JButton ascol_model_off_BT;
    private javax.swing.JButton ascol_model_on_BT;
    private javax.swing.JLabel ascol_model_value_LB;
    private javax.swing.JLabel ascol_oil_LB;
    private javax.swing.JButton ascol_oil_off_BT;
    private javax.swing.JButton ascol_oil_on_BT;
    private javax.swing.JLabel ascol_oil_value_LB;
    private javax.swing.JLabel ascol_precesion_LB;
    private javax.swing.JButton ascol_precesion_off_BT;
    private javax.swing.JButton ascol_precesion_on_BT;
    private javax.swing.JLabel ascol_precesion_value_LB;
    private javax.swing.JLabel ascol_refraction_LB;
    private javax.swing.JButton ascol_refraction_off_BT;
    private javax.swing.JButton ascol_refraction_on_BT;
    private javax.swing.JLabel ascol_refraction_value_LB;
    private javax.swing.JLabel ascol_slith_LB;
    private javax.swing.JButton ascol_slith_close_BT;
    private javax.swing.JButton ascol_slith_open_BT;
    private javax.swing.JButton ascol_slith_stop_BT;
    private javax.swing.JLabel ascol_slith_value_LB;
    private javax.swing.JLabel ascol_state_value_LB;
    private javax.swing.JButton ascol_stop_slew_BT;
    private javax.swing.JLabel ascol_tel_state_LB;
    private javax.swing.JButton ascol_telescope_off_BT;
    private javax.swing.JButton ascol_telescope_on_BT;
    private javax.swing.JLabel ascol_tracking_LB;
    private javax.swing.JButton ascol_tracking_off_BT;
    private javax.swing.JButton ascol_tracking_on_BT;
    private javax.swing.JLabel ascol_tracking_value_LB;
    private javax.swing.JLabel ascol_tube_LB;
    private javax.swing.JButton ascol_tube_close_BT;
    private javax.swing.JButton ascol_tube_open_BT;
    private javax.swing.JButton ascol_tube_stop_BT;
    private javax.swing.JLabel ascol_tube_value_LB;
    private javax.swing.JButton calibration_BT;
    private javax.swing.JButton clearLog_BT;
    private javax.swing.JComboBox collimator_CBI;
    private javax.swing.JLabel collimator_LB;
    private javax.swing.JLabel collimator_label_LB;
    private javax.swing.JButton collimator_stop_BT;
    private javax.swing.JLabel comp_LB;
    private javax.swing.JLabel comp_label_LB;
    private javax.swing.JButton comp_off_BT;
    private javax.swing.JButton comp_on_BT;
    private javax.swing.JPanel connect_P;
    private javax.swing.JLabel correction_plates_400_LB;
    private javax.swing.JLabel correction_plates_400_label_LB;
    private javax.swing.JLabel correction_plates_700_LB;
    private javax.swing.JLabel correction_plates_700_lable_LB;
    private javax.swing.JButton coude_BT;
    private javax.swing.JPanel coude_focus_400_P;
    private javax.swing.JPanel coude_focus_700_P;
    private javax.swing.JLabel coude_oes_LB;
    private javax.swing.JLabel coude_oes_label_LB;
    private javax.swing.JButton coude_oes_stop_BT;
    private javax.swing.JComboBox coude_slit_camera_filter_CBI;
    private javax.swing.JLabel coude_slit_camera_filter_LB;
    private javax.swing.JLabel coude_slit_camera_filter_label_LB;
    private javax.swing.JButton coude_slit_camera_filter_stop_BT;
    private javax.swing.JLabel coude_slit_camera_power_LB;
    private javax.swing.JLabel coude_slit_camera_power_label_LB;
    private javax.swing.JButton coude_slit_camera_power_off_BT;
    private javax.swing.JButton coude_slit_camera_power_on_BT;
    private javax.swing.JLabel deprecated_version_LB;
    private javax.swing.JComboBox dichroic_mirror_CBI;
    private javax.swing.JLabel dichroic_mirror_LB;
    private javax.swing.JLabel dichroic_mirror_label_LB;
    private javax.swing.JButton dichroic_mirror_stop_BT;
    private javax.swing.JButton disableLog_BT;
    private javax.swing.JButton enableLog_BT;
    private javax.swing.JButton execute_ascol_BT;
    private javax.swing.JTextField execute_ascol_TF;
    private javax.swing.JLabel exp_oes_count_LB;
    private javax.swing.JLabel exp_oes_count_label_LB;
    private javax.swing.JLabel exp_oes_frequency_LB;
    private javax.swing.JLabel exp_oes_frequency_label_LB;
    private javax.swing.JButton exp_oes_open_BT;
    private javax.swing.JLabel exp_oes_shutter_LB;
    private javax.swing.JButton exp_oes_shutter_close_BT;
    private javax.swing.JLabel exp_oes_shutter_label_LB;
    private javax.swing.JButton exp_oes_shutter_stop_BT;
    private javax.swing.JButton exp_oes_start_BT;
    private javax.swing.JButton exp_oes_stop_BT;
    private javax.swing.JButton exp_shutter_close_BT;
    private javax.swing.JButton exp_shutter_open_BT;
    private javax.swing.JButton exp_shutter_stop_BT;
    private javax.swing.JButton exp_start_BT;
    private javax.swing.JButton exp_stop_BT;
    private javax.swing.JLabel exposure_meter_count_LB;
    private javax.swing.JLabel exposure_meter_count_label_LB;
    private javax.swing.JLabel exposure_meter_frequency_LB;
    private javax.swing.JLabel exposure_meter_frequency_label_LB;
    private javax.swing.JLabel exposure_meter_shutter_LB;
    private javax.swing.JLabel exposure_meter_shutter_label_LB;
    private javax.swing.JLabel flat_LB;
    private javax.swing.JLabel flat_label_LB;
    private javax.swing.JButton flat_off_BT;
    private javax.swing.JButton flat_on_BT;
    private javax.swing.JLabel focus_400_LB;
    private javax.swing.JLabel focus_700_LB;
    private javax.swing.JButton focus_calibration_400_BT;
    private javax.swing.JButton focus_calibration_700_BT;
    private javax.swing.JButton focus_calibration_oes_BT;
    private javax.swing.JLabel focus_oes_LB;
    private javax.swing.JLabel focus_position_400_LB;
    private javax.swing.JLabel focus_position_700_LB;
    private javax.swing.JLabel focus_position_oes_LB;
    private javax.swing.JButton focus_stop_400_BT;
    private javax.swing.JButton focus_stop_700_BT;
    private javax.swing.JButton focus_stop_oes_BT;
    private javax.swing.JLabel grating_LB;
    private javax.swing.JLabel grating_angle_LB;
    private javax.swing.JButton grating_angle_stop_BT;
    private javax.swing.JLabel host_LB;
    private javax.swing.JTextField host_TF;
    private javax.swing.JLabel jLabel1;
    private javax.swing.JLabel jLabel10;
    private javax.swing.JLabel jLabel11;
    private javax.swing.JLabel jLabel12;
    private javax.swing.JLabel jLabel13;
    private javax.swing.JLabel jLabel14;
    private javax.swing.JLabel jLabel15;
    private javax.swing.JLabel jLabel16;
    private javax.swing.JLabel jLabel2;
    private javax.swing.JLabel jLabel20;
    private javax.swing.JLabel jLabel3;
    private javax.swing.JLabel jLabel4;
    private javax.swing.JLabel jLabel5;
    private javax.swing.JLabel jLabel6;
    private javax.swing.JLabel jLabel7;
    private javax.swing.JLabel jLabel9;
    private javax.swing.JPanel jPanel1;
    private javax.swing.JPanel jPanel11;
    private javax.swing.JPanel jPanel13;
    private javax.swing.JPanel jPanel2;
    private javax.swing.JPanel jPanel4;
    private javax.swing.JPanel jPanel5;
    private javax.swing.JPanel jPanel6;
    private javax.swing.JPanel jPanel7;
    private javax.swing.JPanel jPanel8;
    private javax.swing.JSeparator jSeparator1;
    private javax.swing.JSeparator jSeparator2;
    private javax.swing.JSeparator jSeparator3;
    private javax.swing.JSpinner jSpinner1;
    private javax.swing.JSpinner jSpinner2;
    private javax.swing.JSpinner jSpinner3;
    private javax.swing.JSpinner jSpinner4;
    private javax.swing.JSpinner jSpinner5;
    private javax.swing.JSpinner jSpinner6;
    private javax.swing.JSpinner jSpinner7;
    private javax.swing.JSpinner jSpinner8;
    private javax.swing.JTabbedPane jTabbedPane1;
    private javax.swing.JTabbedPane jTabbedPane2;
    private javax.swing.JTabbedPane jTabbedPane3;
    private javax.swing.JTabbedPane jTabbedPane4;
    private javax.swing.JPanel log_P;
    private javax.swing.JScrollPane log_SP;
    private javax.swing.JTextPane log_TP;
    private javax.swing.JPanel main_P;
    private javax.swing.JPanel observe_P;
    private javax.swing.JTabbedPane observe_TP;
    private javax.swing.JPanel observe_bottom_P;
    private javax.swing.JPanel observe_top_P;
    private javax.swing.JLabel observers_LB;
    private javax.swing.JTextField observers_TF;
    private javax.swing.JButton oes_BT;
    private javax.swing.JComboBox oes_collimator_CBI;
    private javax.swing.JLabel oes_collimator_LB;
    private javax.swing.JLabel oes_collimator_label_LB;
    private javax.swing.JButton oes_collimator_stop_BT;
    private javax.swing.JPanel oes_focus_400_P;
    private javax.swing.JLabel oes_slit_camera_power_LB;
    private javax.swing.JLabel oes_slit_camera_power_label_LB;
    private javax.swing.JButton oes_slit_camera_power_off_BT;
    private javax.swing.JButton oes_slit_camera_power_on_BT;
    private javax.swing.JComboBox oes_slit_height_CBI;
    private javax.swing.JLabel oes_slit_height_label_LB;
    private javax.swing.JComboBox oes_spectral_filter_CBI;
    private javax.swing.JLabel oes_spectral_filter_label_LB;
    private javax.swing.JPanel opso_P;
    private javax.swing.JLabel opso_allowed_offset_LB;
    private javax.swing.JLabel opso_average_LB;
    private javax.swing.JSlider opso_average_SL;
    private javax.swing.JLabel opso_corr_at_error_LB;
    private javax.swing.JPanel opso_cut_P;
    private javax.swing.JLabel opso_cut_x_LB;
    private javax.swing.JSlider opso_cut_x_SL;
    private javax.swing.JLabel opso_cut_x_size_LB;
    private javax.swing.JSlider opso_cut_x_size_SL;
    private javax.swing.JLabel opso_cut_y_LB;
    private javax.swing.JSlider opso_cut_y_SL;
    private javax.swing.JLabel opso_cut_y_size_LB;
    private javax.swing.JSlider opso_cut_y_size_SL;
    private javax.swing.JLabel opso_cut_y_slith_size_LB;
    private javax.swing.JSlider opso_cut_y_slith_size_SL;
    private javax.swing.JCheckBox opso_histogram_CB;
    private javax.swing.JLabel opso_max_fwhm_star_LB;
    private javax.swing.JLabel opso_min_intenzity_star_LB;
    private javax.swing.JLabel opso_permissible_deviation_LB;
    private javax.swing.JLabel opso_sec_in_pix_LB;
    private javax.swing.JButton opso_settings_BT;
    private javax.swing.JComboBox opso_settings_CB;
    private javax.swing.JLabel opso_settings_LB;
    private javax.swing.JPanel opso_settings_P;
    private javax.swing.JCheckBox opso_show_cursor_cut_xy_CB;
    private javax.swing.JCheckBox opso_show_cursor_slith_CB;
    private javax.swing.JCheckBox opso_show_fit_gaus_xy_CB;
    private javax.swing.JCheckBox opso_show_target_CB;
    private javax.swing.JLabel opso_slith_begin_LB;
    private javax.swing.JSlider opso_slith_begin_SL;
    private javax.swing.JLabel opso_slith_end_LB;
    private javax.swing.JSlider opso_slith_end_SL;
    private javax.swing.JPanel opso_target_P;
    private javax.swing.JLabel opso_target_x_LB;
    private javax.swing.JSlider opso_target_x_SL;
    private javax.swing.JLabel opso_target_y_LB;
    private javax.swing.JSlider opso_target_y_SL;
    private javax.swing.JLabel opso_timeout_allowed_offset_LB;
    private javax.swing.JLabel opso_timeout_tsgc_LB;
    private javax.swing.JLabel password_LB;
    private javax.swing.JPasswordField password_PF;
    private javax.swing.JLabel port_LB;
    private javax.swing.JTextField port_TF;
    private javax.swing.JButton relative_focus_position_400_BT;
    private javax.swing.JSpinner relative_focus_position_400_SN;
    private javax.swing.JLabel relative_focus_position_400_label_LB;
    private javax.swing.JButton relative_focus_position_700_BT;
    private javax.swing.JSpinner relative_focus_position_700_SN;
    private javax.swing.JLabel relative_focus_position_700_label_LB;
    private javax.swing.JButton relative_focus_position_oes_BT;
    private javax.swing.JSpinner relative_focus_position_oes_SN;
    private javax.swing.JLabel relative_focus_position_oes_label_LB;
    private javax.swing.JLabel selected_ccd_LB;
    private javax.swing.JPanel services_status_P;
    private javax.swing.JLabel shutter_400_LB;
    private javax.swing.JButton shutter_400_close_BT;
    private javax.swing.JLabel shutter_400_label_LB;
    private javax.swing.JButton shutter_400_open_BT;
    private javax.swing.JButton shutter_400_stop_BT;
    private javax.swing.JLabel shutter_700_LB;
    private javax.swing.JButton shutter_700_close_BT;
    private javax.swing.JLabel shutter_700_label_LB;
    private javax.swing.JButton shutter_700_open_BT;
    private javax.swing.JButton shutter_700_stop_BT;
    private javax.swing.JComboBox spectral_filter_CBI;
    private javax.swing.JLabel spectral_filter_LB;
    private javax.swing.JLabel spectral_filter_label_LB;
    private javax.swing.JButton spectral_filter_stop_BT;
    private javax.swing.JTabbedPane spectrograph_TB;
    private javax.swing.JPanel spectrograph_control_P;
    private javax.swing.JPanel spectrograph_coude_P;
    private javax.swing.JInternalFrame spectrograph_coude_invisible_IF;
    private javax.swing.JPanel spectrograph_focus_P;
    private javax.swing.JPanel spectrograph_main_P;
    private javax.swing.JPanel spectrograph_oes_P;
    private javax.swing.JButton star_BT;
    private javax.swing.JLabel star_calibration_LB;
    private javax.swing.JLabel star_calibration_label_LB;
    private javax.swing.JButton star_calibration_stop_BT;
    private javax.swing.JLabel state_LB;
    private javax.swing.JPanel state_P;
    private javax.swing.JProgressBar state_PB;
    private javax.swing.JLabel status_ccd400_LB;
    private javax.swing.JLabel status_ccd700_LB;
    private javax.swing.JLabel status_oes_LB;
    private javax.swing.JLabel status_opso_LB;
    private javax.swing.JLabel status_spectrograph_LB;
    private javax.swing.JLabel status_telescope_LB;
    private javax.swing.JPanel telescope_P;
    private javax.swing.JLabel telescope_airmass_LB;
    private javax.swing.JLabel telescope_airmass_title_LB;
    private javax.swing.JLabel telescope_altitude_LB;
    private javax.swing.JLabel telescope_altitude_title_LB;
    private javax.swing.JLabel telescope_azimuth_LB;
    private javax.swing.JPanel telescope_azimuth_P;
    private javax.swing.JLabel telescope_azimuth_title_LB;
    private javax.swing.JPanel telescope_control_P;
    private javax.swing.JPanel telescope_corrections_P;
    private javax.swing.JLabel telescope_corrections_dec_LB;
    private javax.swing.JSpinner telescope_corrections_dec_SN;
    private javax.swing.JLabel telescope_corrections_dec_title_LB;
    private javax.swing.JLabel telescope_corrections_ra_LB;
    private javax.swing.JSpinner telescope_corrections_ra_SN;
    private javax.swing.JLabel telescope_corrections_ra_title_LB;
    private javax.swing.JButton telescope_corrections_set_BT;
    private javax.swing.JLabel telescope_da_LB;
    private javax.swing.JTextField telescope_da_TF;
    private javax.swing.JLabel telescope_da_title_LB;
    private javax.swing.JLabel telescope_dec_LB;
    private javax.swing.JTextField telescope_dec_TF;
    private javax.swing.JLabel telescope_dec_title_LB;
    private javax.swing.JPanel telescope_dome_P;
    private javax.swing.JButton telescope_dome_automatic_BT;
    private javax.swing.JLabel telescope_dome_azimuth_LB;
    private javax.swing.JLabel telescope_dome_azimuth_title_LB;
    private javax.swing.JButton telescope_dome_calibration_BT;
    private javax.swing.JLabel telescope_dome_height_LB;
    private javax.swing.JLabel telescope_dome_height_title_LB;
    private javax.swing.JButton telescope_dome_position_BT;
    private javax.swing.JLabel telescope_dome_position_LB;
    private javax.swing.JSpinner telescope_dome_position_SN;
    private javax.swing.JLabel telescope_dome_position_title_LB;
    private javax.swing.JButton telescope_dome_relative_position_BT;
    private javax.swing.JLabel telescope_dome_relative_position_LB;
    private javax.swing.JSpinner telescope_dome_relative_position_SN;
    private javax.swing.JLabel telescope_dome_state_LB;
    private javax.swing.JLabel telescope_dome_state_title_LB;
    private javax.swing.JButton telescope_dome_stop_BT;
    private javax.swing.JSpinner telescope_focussing_absolute_SN;
    private javax.swing.JSpinner telescope_focussing_relative_SN;
    private javax.swing.JButton telescope_get_tsra_BT;
    private javax.swing.JButton telescope_go_source_BT;
    private javax.swing.JButton telescope_go_star_BT;
    private javax.swing.JLabel telescope_ha_LB;
    private javax.swing.JTextField telescope_ha_TF;
    private javax.swing.JLabel telescope_ha_title_LB;
    private javax.swing.JLabel telescope_hour_angle_LB;
    private javax.swing.JLabel telescope_hour_angle_title_LB;
    private javax.swing.JLabel telescope_lst_LB;
    private javax.swing.JLabel telescope_lst_title_LB;
    private javax.swing.JLabel telescope_object_LB;
    private javax.swing.JLabel telescope_object_title_LB;
    private javax.swing.JComboBox telescope_position_CBI;
    private javax.swing.JLabel telescope_position_LB;
    private javax.swing.JLabel telescope_position_title_LB;
    private javax.swing.JLabel telescope_ra_LB;
    private javax.swing.JTextField telescope_ra_TF;
    private javax.swing.JLabel telescope_ra_title_LB;
    private javax.swing.JPanel telescope_settings_P;
    private javax.swing.JPanel telescope_source_P;
    private javax.swing.JPanel telescope_speed_P;
    private javax.swing.JLabel telescope_speed_dec_LB;
    private javax.swing.JSpinner telescope_speed_dec_SN;
    private javax.swing.JLabel telescope_speed_dec_title_LB;
    private javax.swing.JLabel telescope_speed_ra_LB;
    private javax.swing.JSpinner telescope_speed_ra_SN;
    private javax.swing.JLabel telescope_speed_ra_title_LB;
    private javax.swing.JButton telescope_speed_set_BT;
    private javax.swing.JPanel telescope_star_P;
    private javax.swing.JPanel telescope_time_P;
    private javax.swing.JLabel telescope_utc_LB;
    private javax.swing.JLabel telescope_utc_title_LB;
    private javax.swing.JLabel username_LB;
    private javax.swing.JTextField username_TF;
    // End of variables declaration//GEN-END:variables
 
    public static void main(String[] args) {
        Matcher matcherDDMMSS;
        String[] inputsDDMMSS = {
            "112233",
            "+112233",
            "-112233",
            "112233.",
            "+112233.444",
        };
        
        System.out.println("REGEX_DDMMSS: " + ObserveWindow.REGEX_DDMMSS);
        for (String input : inputsDDMMSS) {
            System.out.format("    checking %11s", input);
            matcherDDMMSS = ObserveWindow.PATTERN_DDMMSS.matcher(input);
            if (!matcherDDMMSS.find()) {
                System.out.println(" FAILED");
                System.exit(-1);
            }
            else {
                System.out.println(" OK");
            }
        }

        Matcher matcherDegrees;
        String[] inputsDegrees = {
            "111",
            "+111",
            "-111",
            "111.",
            "+111.2222",
        };
        
        System.out.println("REGEX_DEGREES: " + ObserveWindow.REGEX_DEGREES);
        for (String input : inputsDegrees) {
            System.out.format("    checking %11s", input);
            matcherDegrees = ObserveWindow.PATTERN_DEGREES.matcher(input);
            if (!matcherDegrees.find()) {
                System.out.println(" FAILED");
                System.exit(-1);
            }
            else {
                System.out.println(" OK");
            }
        }
    }
}
