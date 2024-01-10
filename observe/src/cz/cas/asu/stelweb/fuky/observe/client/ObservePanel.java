/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2010-2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import cz.cas.asu.stelweb.fuky.observe.jaxb.ManualSetupExposureCoefType;
import cz.cas.asu.stelweb.fuky.observe.jaxb.ManualSetupExposureSetupType;
import cz.cas.asu.stelweb.fuky.observe.jaxb.ManualSetupExposureXML;
import cz.cas.asu.stelweb.fuky.observe.jaxb.ManualSetupExposureXML.CoefAngle;
import cz.cas.asu.stelweb.fuky.observe.xml.ExposeInfoXML;
import cz.cas.asu.stelweb.fuky.observe.xml.setup_exposure.SetupExposure;

import java.awt.Color;
import java.util.*;
import java.util.logging.Level;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.awt.event.*;
import java.io.IOException;

import javax.swing.*;
import javax.swing.event.ChangeListener;

public class ObservePanel extends javax.swing.JPanel {
    static final long serialVersionUID = 43L;

    private Color red_color = new Color(255, 204, 204);
    private Color green_color = new Color(204, 255, 204);
    private Color yellow_color = new Color(255, 255, 51);

    // [H]H:[M]M:[S]S
    public static final String REGEX_H_M_S = "^(\\d{1,2}):(\\d{1,2}):(\\d{1,2})$";
    public static final Pattern PATTERN_H_M_S = Pattern.compile(REGEX_H_M_S);
    
    // [M]M:[S]S
    public static final String REGEX_M_S = "^(\\d{1,2}):(\\d{1,2})$";
    public static final Pattern PATTERN_M_S = Pattern.compile(REGEX_M_S);
    
    // [SSSS]S
    public static final String REGEX_S = "^(\\d{1,5})$";
    public static final Pattern PATTERN_S = Pattern.compile(REGEX_S);
    
    // DEPRECATED: [CCCCCCCC]C[kM]
    //public static final String REGEX_COUNT = "^(\\d{1,9})([kKmM]?)$";
    // [NNN]N[.][NNNNNN]
    public static final String REGEX_COUNT = "^(\\d{1,4}\\.{0,1}\\d{0,6})$";
    public static final Pattern PATTERN_COUNT = Pattern.compile(REGEX_COUNT);
    
    private ObserveWindow observeWindow;
    private int exposure_time = -1;
    private int exposure_meter = -1;
    private int exposeNumber;
    private boolean showFitsFile = false;
    private String lastFitsFile;
    private String exposure_target = "";

    private Map<String, SetupExposure> setupExposureMap;
    private SetupExposure setupExposure = new SetupExposure();
    private SetupExposure manualSetupExposure = new SetupExposure();
    
    private ManualSetupExposureXML manualSetupExposureXML;

    /** Creates new form ObservePanel */
    public ObservePanel() {
        String[] objects = { "flat", "comp", "zero", "dark", "target" };

        initComponents();
        
        exposure_count_of_pulses_PB.setStringPainted(true);
        exposure_count_of_pulses_PB.setMinimum(0);
        exposure_count_of_pulses_PB.setMaximum(100);
        exposure_count_of_pulses_PB.setValue(100);
        
        exposure_meter_count_off_CB.setSelected(true);
        exposure_meter_count_off_CBActionPerformed(null);

        // value, min, max, step
        exposure_count_repeat_SN.setModel(new SpinnerNumberModel(1, 1, 1000, 1));
        //exposure_length_SN.setModel(new SpinnerNumberModel(0, 0, 86400, 1));
        ccd_require_temp_SN.setModel(new SpinnerNumberModel(-110, -120, -80, 1));

        for (String object : objects) {
            exposure_object_CBI.addItem(object);
        }

        exposure_target_TF.setDocument(new JTextFieldLimit(ObserveWindow.FITS_HDR_VALUE_MAX));
        
        exposure_PB.setStringPainted(true);
        exposure_PB.setMinimum(0);
        exposure_PB.setMaximum(100);
        exposure_PB.setValue(100);

        exposure_file_fits_TF.setEditable(false);
        exposure_autogenerate_file_fits_CB.setEnabled(false);
        exposure_autogenerate_file_fits_CB.setSelected(true);
        ccd_archive_CB.setEnabled(false);

        ccd_archive_path_LB.setVisible(false);
        ccd_archive_path_CBI.setVisible(false);
        ccd_archive_CB.setVisible(false);
        
        String text = "<html>Exposure time must be [H]H:[M]M:[S]S or [M]M:[S]S or [SSSS]S.<br><br>"
                + "Example:<br><br>"
                + "    01:30:00 - 1 hour, 30 minutes, 0 seconds<br>"
                + "    45:00 - 45 minutes, 0 seconds<br>"
                + "    90 - 90 seconds</html>";
        
        exposure_time_TF.setToolTipText(text);
        
        text = "<html>Exposure meter count must be [NNN]N[.][NNNNNN] Mcounts.<br><br>"
                + "Example:<br><br>"
                + "    2 - 2 000 000 counts<br>"
                + "    0.5 - 500 000 counts</html>";
        
        exposure_meter_count_TF.setToolTipText(text);
    }

    public ObservePanel(ObserveWindow observeWindow, String name) {
        this();
        this.observeWindow = observeWindow;
        this.setName(name);
                
        // OES
        if (name.equals("oes")) {
            exposure_central_wavelength_LB.setVisible(false);
            exposure_central_wavelength_SN.setVisible(false);
            exposure_wavelength_range_LB.setVisible(false);
            exposure_manual_setup_BT.setVisible(false);
        }
        // CCD700, CCD400
        else {
            exposure_manual_setup_setEnabled(false);
            observe_others_P.add(observe_coude_P);
            
            exposure_central_wavelength_SN.setModel(new SpinnerNumberModel(3000, 3000, 10000, 1));
            
            // TODO: nacitat povoleny rozsah z XML
            String text = "<html>Central wavelength must be an integer within the allowed range "
                        + "&lt;3 000; 10 000&gt;</html>";
            
            exposure_central_wavelength_SN.setToolTipText(text);
        }
        
        ccd_require_temp_title_LB.setVisible(false);
        ccd_require_temp_SN.setVisible(false);
    }
    
    public void exposure_manual_setup_setEnabled(boolean enabled) {
        exposure_central_wavelength_LB.setEnabled(enabled);
        exposure_central_wavelength_SN.setEnabled(enabled);
        exposure_wavelength_range_LB.setEnabled(enabled);
        exposure_manual_setup_BT.setEnabled(enabled);
    }
    
    public String getExposureTarget()
    {
        return exposure_target_TF.getText();
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
    // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
    private void initComponents() {
        java.awt.GridBagConstraints gridBagConstraints;

        exposure_control_BG = new javax.swing.ButtonGroup();
        observe_coude_P = new javax.swing.JPanel();
        coude_dm_label_LB = new javax.swing.JLabel();
        coude_dm_LB = new javax.swing.JLabel();
        coude_sf_label_LB = new javax.swing.JLabel();
        coude_sf_LB = new javax.swing.JLabel();
        coude_ga_label_LB = new javax.swing.JLabel();
        coude_ga_LB = new javax.swing.JLabel();
        coude_ga_state_LB = new javax.swing.JLabel();
        coude_em_label_LB = new javax.swing.JLabel();
        coude_em_LB = new javax.swing.JLabel();
        observe_output_P = new javax.swing.JPanel();
        ccd_data_path_LB = new javax.swing.JLabel();
        ccd_archive_path_LB = new javax.swing.JLabel();
        ccd_archive_path_CBI = new javax.swing.JComboBox();
        ccd_data_path_CBI = new javax.swing.JComboBox();
        ccd_archive_CB = new javax.swing.JCheckBox();
        observe_ccd_P = new javax.swing.JPanel();
        ccd_actual_temp_title_LB = new javax.swing.JLabel();
        ccd_require_temp_title_LB = new javax.swing.JLabel();
        ccd_readout_speed_LB = new javax.swing.JLabel();
        ccd_actual_temp_LB = new javax.swing.JLabel();
        ccd_require_temp_SN = new javax.swing.JSpinner();
        ccd_readout_speed_CBI = new javax.swing.JComboBox();
        ccd_gain_LB = new javax.swing.JLabel();
        ccd_gain_CBI = new javax.swing.JComboBox();
        observe_exposure_P = new javax.swing.JPanel();
        exposure_count_repeat_LB = new javax.swing.JLabel();
        exposure_time_LB = new javax.swing.JLabel();
        exposure_file_fits_LB = new javax.swing.JLabel();
        exposure_target_LB = new javax.swing.JLabel();
        exposure_object_LB = new javax.swing.JLabel();
        exposure_count_repeat_SN = new javax.swing.JSpinner();
        exposure_file_fits_TF = new javax.swing.JTextField();
        exposure_target_TF = new javax.swing.JTextField();
        exposure_object_CBI = new javax.swing.JComboBox();
        exposure_autogenerate_file_fits_CB = new javax.swing.JCheckBox();
        exposure_control_P = new javax.swing.JPanel();
        exposure_start_BT = new javax.swing.JButton();
        exposure_readout_BT = new javax.swing.JButton();
        exposure_abort_BT = new javax.swing.JButton();
        exposure_PB = new javax.swing.JProgressBar();
        exposure_state_LB = new javax.swing.JLabel();
        exposure_count_of_pulses_PB = new javax.swing.JProgressBar();
        exposure_meter_count_LB = new javax.swing.JLabel();
        exposure_meter_count_TF = new javax.swing.JTextField();
        exposure_time_TF = new javax.swing.JTextField();
        jSeparator1 = new javax.swing.JSeparator();
        jSeparator2 = new javax.swing.JSeparator();
        exposure_control_update_BT = new javax.swing.JButton();
        exposure_time_off_CB = new javax.swing.JCheckBox();
        exposure_meter_count_off_CB = new javax.swing.JCheckBox();
        exposure_setup_LB = new javax.swing.JLabel();
        exposure_setup_CBI = new javax.swing.JComboBox();
        exposure_setup_BT = new javax.swing.JButton();
        exposure_central_wavelength_LB = new javax.swing.JLabel();
        exposure_central_wavelength_SN = new javax.swing.JSpinner();
        exposure_wavelength_range_LB = new javax.swing.JLabel();
        exposure_manual_setup_BT = new javax.swing.JButton();
        observe_others_P = new javax.swing.JPanel();

        observe_coude_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Coude"));
        observe_coude_P.setName("observe_coude_P"); // NOI18N
        observe_coude_P.setLayout(new java.awt.GridBagLayout());

        coude_dm_label_LB.setText("DM");
        coude_dm_label_LB.setName("coude_dm_label_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_coude_P.add(coude_dm_label_LB, gridBagConstraints);

        coude_dm_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        coude_dm_LB.setText("unknown");
        coude_dm_LB.setName("coude_dm_LB"); // NOI18N
        coude_dm_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(0, 5, 5, 5);
        observe_coude_P.add(coude_dm_LB, gridBagConstraints);

        coude_sf_label_LB.setText("SF");
        coude_sf_label_LB.setName("coude_sf_label_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_coude_P.add(coude_sf_label_LB, gridBagConstraints);

        coude_sf_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        coude_sf_LB.setText("unknown");
        coude_sf_LB.setName("coude_sf_LB"); // NOI18N
        coude_sf_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(0, 5, 5, 5);
        observe_coude_P.add(coude_sf_LB, gridBagConstraints);

        coude_ga_label_LB.setText("GA");
        coude_ga_label_LB.setName("coude_ga_label_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_coude_P.add(coude_ga_label_LB, gridBagConstraints);

        coude_ga_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        coude_ga_LB.setText("unknown");
        coude_ga_LB.setName("coude_ga_LB"); // NOI18N
        coude_ga_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(0, 5, 5, 5);
        observe_coude_P.add(coude_ga_LB, gridBagConstraints);

        coude_ga_state_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        coude_ga_state_LB.setText("unknown");
        coude_ga_state_LB.setName("coude_ga_state_LB"); // NOI18N
        coude_ga_state_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(0, 5, 5, 5);
        observe_coude_P.add(coude_ga_state_LB, gridBagConstraints);

        coude_em_label_LB.setText("EM");
        coude_em_label_LB.setName("coude_em_label_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_coude_P.add(coude_em_label_LB, gridBagConstraints);

        coude_em_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        coude_em_LB.setText("unknown");
        coude_em_LB.setName("coude_em_LB"); // NOI18N
        coude_em_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 4;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.FIRST_LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(0, 5, 5, 5);
        observe_coude_P.add(coude_em_LB, gridBagConstraints);

        setName("Form"); // NOI18N
        setLayout(new java.awt.GridBagLayout());

        observe_output_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Output"));
        observe_output_P.setName("observe_output_P"); // NOI18N
        observe_output_P.setLayout(new java.awt.GridBagLayout());

        ccd_data_path_LB.setText("Data path:");
        ccd_data_path_LB.setName("ccd_data_path_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_output_P.add(ccd_data_path_LB, gridBagConstraints);

        ccd_archive_path_LB.setText("Archive path:");
        ccd_archive_path_LB.setName("ccd_archive_path_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_output_P.add(ccd_archive_path_LB, gridBagConstraints);

        ccd_archive_path_CBI.setName("ccd_archive_path_CBI"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_output_P.add(ccd_archive_path_CBI, gridBagConstraints);

        ccd_data_path_CBI.setName("ccd_data_path_CBI"); // NOI18N
        ccd_data_path_CBI.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                ccd_data_path_CBIActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_output_P.add(ccd_data_path_CBI, gridBagConstraints);

        ccd_archive_CB.setText("Archive");
        ccd_archive_CB.setName("ccd_archive_CB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_output_P.add(ccd_archive_CB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        add(observe_output_P, gridBagConstraints);

        observe_ccd_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "CCD"));
        observe_ccd_P.setName("observe_ccd_P"); // NOI18N
        observe_ccd_P.setLayout(new java.awt.GridBagLayout());

        ccd_actual_temp_title_LB.setText("Actual CCD temp:");
        ccd_actual_temp_title_LB.setName("ccd_actual_temp_title_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_ccd_P.add(ccd_actual_temp_title_LB, gridBagConstraints);

        ccd_require_temp_title_LB.setText("Require CCD temp:");
        ccd_require_temp_title_LB.setEnabled(false);
        ccd_require_temp_title_LB.setName("ccd_require_temp_title_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_ccd_P.add(ccd_require_temp_title_LB, gridBagConstraints);

        ccd_readout_speed_LB.setText("Readout speed:");
        ccd_readout_speed_LB.setName("ccd_readout_speed_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_ccd_P.add(ccd_readout_speed_LB, gridBagConstraints);

        ccd_actual_temp_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        ccd_actual_temp_LB.setText("unknown");
        ccd_actual_temp_LB.setName("ccd_actual_temp_LB"); // NOI18N
        ccd_actual_temp_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_ccd_P.add(ccd_actual_temp_LB, gridBagConstraints);

        ccd_require_temp_SN.setEnabled(false);
        ccd_require_temp_SN.setName("ccd_require_temp_SN"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_ccd_P.add(ccd_require_temp_SN, gridBagConstraints);

        ccd_readout_speed_CBI.setName("ccd_readout_speed_CBI"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_ccd_P.add(ccd_readout_speed_CBI, gridBagConstraints);

        ccd_gain_LB.setText("Gain:");
        ccd_gain_LB.setName("ccd_gain_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_ccd_P.add(ccd_gain_LB, gridBagConstraints);

        ccd_gain_CBI.setName("ccd_gain_CBI"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_ccd_P.add(ccd_gain_CBI, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        add(observe_ccd_P, gridBagConstraints);

        observe_exposure_P.setBorder(javax.swing.BorderFactory.createTitledBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(0, 0, 0)), "Exposure"));
        observe_exposure_P.setName("observe_exposure_P"); // NOI18N
        observe_exposure_P.setLayout(new java.awt.GridBagLayout());

        exposure_count_repeat_LB.setText("Count of repeating:");
        exposure_count_repeat_LB.setName("exposure_count_repeat_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_count_repeat_LB, gridBagConstraints);

        exposure_time_LB.setText("Exposure Time:");
        exposure_time_LB.setName("exposure_time_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_time_LB, gridBagConstraints);

        exposure_file_fits_LB.setText("File FITs:");
        exposure_file_fits_LB.setName("exposure_file_fits_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 11;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_file_fits_LB, gridBagConstraints);

        exposure_target_LB.setText("Target:");
        exposure_target_LB.setName("exposure_target_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 10;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_target_LB, gridBagConstraints);

        exposure_object_LB.setText("Object:");
        exposure_object_LB.setName("exposure_object_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 9;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_object_LB, gridBagConstraints);

        exposure_count_repeat_SN.setName("exposure_count_repeat_SN"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_count_repeat_SN, gridBagConstraints);

        exposure_file_fits_TF.setName("exposure_file_fits_TF"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 11;
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_file_fits_TF, gridBagConstraints);

        exposure_target_TF.setName("exposure_target_TF"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 10;
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_target_TF, gridBagConstraints);

        exposure_object_CBI.setName("exposure_object_CBI"); // NOI18N
        exposure_object_CBI.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                exposure_object_CBIActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 9;
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_object_CBI, gridBagConstraints);

        exposure_autogenerate_file_fits_CB.setText("Autogenerate file FITs");
        exposure_autogenerate_file_fits_CB.setName("exposure_autogenerate_file_fits_CB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 12;
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        observe_exposure_P.add(exposure_autogenerate_file_fits_CB, gridBagConstraints);

        exposure_control_P.setName("exposure_control_P"); // NOI18N
        exposure_control_P.setLayout(new java.awt.GridBagLayout());

        exposure_start_BT.setText("Start");
        exposure_start_BT.setName("exposure_start_BT"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        exposure_control_P.add(exposure_start_BT, gridBagConstraints);

        exposure_readout_BT.setText("Readout");
        exposure_readout_BT.setName("exposure_readout_BT"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        exposure_control_P.add(exposure_readout_BT, gridBagConstraints);

        exposure_abort_BT.setText("Abort");
        exposure_abort_BT.setName("exposure_abort_BT"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        exposure_control_P.add(exposure_abort_BT, gridBagConstraints);

        exposure_PB.setName("exposure_PB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        exposure_control_P.add(exposure_PB, gridBagConstraints);

        exposure_state_LB.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
        exposure_state_LB.setText("unknown");
        exposure_state_LB.setName("exposure_state_LB"); // NOI18N
        exposure_state_LB.setOpaque(true);
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.ipadx = 10;
        gridBagConstraints.ipady = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.LINE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        exposure_control_P.add(exposure_state_LB, gridBagConstraints);

        exposure_count_of_pulses_PB.setName("exposure_count_of_pulses_PB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.gridwidth = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        exposure_control_P.add(exposure_count_of_pulses_PB, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 13;
        gridBagConstraints.gridwidth = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        observe_exposure_P.add(exposure_control_P, gridBagConstraints);

        exposure_meter_count_LB.setText("Count of pulses:");
        exposure_meter_count_LB.setName("exposure_meter_count_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_meter_count_LB, gridBagConstraints);

        exposure_meter_count_TF.setName("exposure_meter_count_TF"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_meter_count_TF, gridBagConstraints);

        exposure_time_TF.setName("exposure_time_TF"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_time_TF, gridBagConstraints);

        jSeparator1.setName("jSeparator1"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.gridwidth = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(10, 5, 10, 5);
        observe_exposure_P.add(jSeparator1, gridBagConstraints);

        jSeparator2.setName("jSeparator2"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.gridwidth = 4;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.insets = new java.awt.Insets(10, 5, 10, 5);
        observe_exposure_P.add(jSeparator2, gridBagConstraints);

        exposure_control_update_BT.setText("Update");
        exposure_control_update_BT.setName("exposure_control_update_BT"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_control_update_BT, gridBagConstraints);

        exposure_time_off_CB.setText("OFF");
        exposure_time_off_CB.setName("exposure_time_off_CB"); // NOI18N
        exposure_time_off_CB.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                exposure_time_off_CBActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_time_off_CB, gridBagConstraints);

        exposure_meter_count_off_CB.setText("OFF");
        exposure_meter_count_off_CB.setName("exposure_meter_count_off_CB"); // NOI18N
        exposure_meter_count_off_CB.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                exposure_meter_count_off_CBActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_meter_count_off_CB, gridBagConstraints);

        exposure_setup_LB.setText("Setup:");
        exposure_setup_LB.setName("exposure_setup_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_setup_LB, gridBagConstraints);

        exposure_setup_CBI.setName("exposure_setup_CBI"); // NOI18N
        exposure_setup_CBI.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                exposure_setup_CBIActionPerformed(evt);
            }
        });
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = 2;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_setup_CBI, gridBagConstraints);

        exposure_setup_BT.setText("Set");
        exposure_setup_BT.setName("exposure_setup_BT"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_setup_BT, gridBagConstraints);

        exposure_central_wavelength_LB.setText("Central wavelength:");
        exposure_central_wavelength_LB.setName("exposure_central_wavelength_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_central_wavelength_LB, gridBagConstraints);

        exposure_central_wavelength_SN.setName("exposure_central_wavelength_SN"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_central_wavelength_SN, gridBagConstraints);

        exposure_wavelength_range_LB.setText("unknown");
        exposure_wavelength_range_LB.setName("exposure_wavelength_range_LB"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 2;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_wavelength_range_LB, gridBagConstraints);

        exposure_manual_setup_BT.setText("Set");
        exposure_manual_setup_BT.setName("exposure_manual_setup_BT"); // NOI18N
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 3;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 0, 5);
        observe_exposure_P.add(exposure_manual_setup_BT, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        add(observe_exposure_P, gridBagConstraints);

        observe_others_P.setName("observe_others_P"); // NOI18N
        observe_others_P.setLayout(new java.awt.GridLayout(1, 0));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.PAGE_START;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(5, 5, 5, 5);
        add(observe_others_P, gridBagConstraints);
    }// </editor-fold>//GEN-END:initComponents

    private void exposure_object_CBIActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_exposure_object_CBIActionPerformed
        String object = exposure_object_CBI.getSelectedItem().toString();

        exposure_count_repeat_SN.setValue(1);
        
        if (object.equals("target")) {
            exposure_time_TF.setText("");
            exposure_target_TF.setEditable(true);
            exposure_target_TF.setText(exposure_target);
            exposure_meter_count_off_CB.setEnabled(true);
        }
        else {
            exposure_target_TF.setEditable(false);
            exposure_target_TF.setText(object);
            exposure_meter_count_off_CB.setEnabled(false);
            exposure_meter_count_off_CB.setSelected(true);
            exposure_meter_count_off_CBActionPerformed(null);
            exposure_time_off_CB.setSelected(false);
            exposure_time_off_CBActionPerformed(null);
        }
        
        if (object.equals("zero")) {
            exposure_time_TF.setText("0");
            exposure_time_TF.setEditable(false);
            exposure_time_off_CB.setEnabled(false);
            exposure_time_off_CB.setSelected(false);
            exposure_time_off_CBActionPerformed(null);
        }
        else {
            exposure_time_TF.setEditable(true);
            exposure_time_off_CB.setEnabled(true);
        }
        
        if (object.equals("flat")) {
            exposure_time_TF.setText(setupExposure.getFlat().toString());
        }
        else if (object.equals("comp")) {
            exposure_time_TF.setText(setupExposure.getComp().toString());
        }
    }//GEN-LAST:event_exposure_object_CBIActionPerformed

    private void ccd_data_path_CBIActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_ccd_data_path_CBIActionPerformed
        if (ccd_data_path_CBI.getSelectedItem().toString().startsWith("/tmp")) {
            ccd_archive_CB.setSelected(false);
            observeWindow.showWarningMessage("Temporary data path. Data will be deleted.");
        }
        else {
            ccd_archive_CB.setSelected(true);
        }
    }//GEN-LAST:event_ccd_data_path_CBIActionPerformed

    private void exposure_time_off_CBActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_exposure_time_off_CBActionPerformed
        exposure_control_update();
    }//GEN-LAST:event_exposure_time_off_CBActionPerformed

    private void exposure_meter_count_off_CBActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_exposure_meter_count_off_CBActionPerformed
        exposure_control_update();
    }//GEN-LAST:event_exposure_meter_count_off_CBActionPerformed

    private void exposure_setup_CBIActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_exposure_setup_CBIActionPerformed
        String selectedItem = (String)exposure_setup_CBI.getSelectedItem();
        if (selectedItem == null) {
            return;
        }
        
        SetupExposure setupExposure = setupExposureMap.get(selectedItem.toString());
        if (setupExposure == null) {
            exposure_setup_BT.setToolTipText("");
            return;
        }
        
        String text = String.format("<html>%s - range %s<br>" +
            "  Grating angle: %s<br>" +
            "  Dichroic mirror: %d<br>" +
            "  Spectral filter: %d<br>" +
            "  Flat: %ds<br>" +
            "  Comp: %ds<br></html>",
            setupExposure.getName(),
            setupExposure.getRange(),            
            setupExposure.getGa(),
            setupExposure.getDm(),
            setupExposure.getSf(),
            setupExposure.getFlat(),
            setupExposure.getComp()
        );
        
        exposure_setup_BT.setToolTipText(text);
    }//GEN-LAST:event_exposure_setup_CBIActionPerformed

    public void set_exposure_target(String exposure_target) {
        String object = exposure_object_CBI.getSelectedItem().toString();
        
        this.exposure_target = exposure_target;
        
        if (object.equals("target")) {
            exposure_target_TF.setText(exposure_target);
        }
    }
    
    private void exposure_control_update() {        
        exposure_meter_count_LB.setEnabled(!exposure_meter_count_off_CB.isSelected());
        exposure_meter_count_TF.setEnabled(!exposure_meter_count_off_CB.isSelected());
        exposure_time_LB.setEnabled(!exposure_time_off_CB.isSelected());
        exposure_time_TF.setEnabled(!exposure_time_off_CB.isSelected());
    }
    
    public void exposure_start_BT_addActionListener(ActionListener newActionListener) {
        exposure_start_BT.addActionListener(newActionListener);
    }

    public void exposure_readout_BT_addActionListener(ActionListener newActionListener) {
        exposure_readout_BT.addActionListener(newActionListener);
    }

    public void exposure_abort_BT_addActionListener(ActionListener newActionListener) {
        exposure_abort_BT.addActionListener(newActionListener);
    }
    
    public void exposure_control_update_BT_addActionListener(ActionListener newActionListener) {
        exposure_control_update_BT.addActionListener(newActionListener);
    }
    
    public void exposure_setup_BT_addActionListener(ActionListener newActionListener) {
        exposure_setup_BT.addActionListener(newActionListener);
    }
    
    public void exposure_manual_setup_BT_addActionListener(ActionListener newActionListener) {
        exposure_manual_setup_BT.addActionListener(newActionListener);
    }
    
    public void exposure_central_wavelength_SN_addChangeListener(ChangeListener newChangeListener) {
        exposure_central_wavelength_SN.addChangeListener(newChangeListener);
    }

    public CallBack createCallBack(String functionName, ArrayList functionParams) {
        return new ExposeExecuteCB(this.getName(), functionName, functionParams.toArray());
    }

    public CallBack createCallBack(String functionName) {
        return this.createCallBack(functionName, new ArrayList());
    }

    public static int parseExposureTime(String exposure_time_str) throws Exception {
        Matcher matcher_h_m_s = PATTERN_H_M_S.matcher(exposure_time_str);
        Matcher matcher_m_s = PATTERN_M_S.matcher(exposure_time_str);
        Matcher matcher_s = PATTERN_S.matcher(exposure_time_str);
        int exposure_time = -1;
        
        if (matcher_h_m_s.find()) {
            int hours = Integer.parseInt(matcher_h_m_s.group(1));
            int minutes = Integer.parseInt(matcher_h_m_s.group(2));
            int seconds = Integer.parseInt(matcher_h_m_s.group(3));
            
            exposure_time = (3600 * hours) + (60 * minutes) + seconds;
        }
        else if (matcher_m_s.find()) {
            int minutes = Integer.parseInt(matcher_m_s.group(1));
            int seconds = Integer.parseInt(matcher_m_s.group(2));
            
            exposure_time = (60 * minutes) + seconds;
        }
        else if (matcher_s.find()) {
            exposure_time = Integer.parseInt(matcher_s.group(1));
        }
        else {
            String msg = "Exposure time must be [H]H:[M]M:[S]S or [M]M:[S]S or [SSSS]S.\n\n" + 
                         "Example:\n\n" +
                         "    01:30:00 - 1 hour, 30 minutes, 0 seconds\n" +
                         "    45:00 - 45 minutes, 0 seconds\n" +
                         "    90 - 90 seconds";
            
            throw new ObserveClientException(msg);
        }
        
        return exposure_time;
    }

    public static int parseExposureMeter(String count_str) throws Exception {
        Matcher matcher_count = PATTERN_COUNT.matcher(count_str);
        Double count = new Double(-1);
        
        if (matcher_count.find()) {
            count = Double.parseDouble(matcher_count.group(1));
            count *= 1000000;
            
// DEPRECATED
//            String prefix_str = matcher_count.group(2).toLowerCase();
//            int prefix = 1;
//            
//            if (prefix_str.length() > 0) {
//                switch (prefix_str.charAt(0)) {
//                case 'k':
//                    prefix = 1000;
//                    break;
//
//                case 'm':
//                    prefix = 1000000;
//                    break;
//                }
//            }
//            
//            count *= prefix;
        }
        else {
            String msg = "Exposure meter count must be [NNN]N[.][NNNNNN] Mcounts.\n\n" + 
                    "Example:\n\n" +
                    "    2 - 2 000 000 counts\n" +
                    "    0.5 - 500 000 counts";
            
// DEPRECATED
//            String msg = "Exposure meter count must be [NNNNNNNN]N[kKmM].\n\n" + 
//                         "Example:\n\n" +
//                         "    2000 - 2000 counts\n" +
//                         "    15k - 15 000 counts\n" +
//                         "    20M - 20 000 000 counts";
            
            throw new ObserveClientException(msg);
        }
        
        return count.intValue();        
    }    
    
    private boolean load_exposure_control(){
        exposure_time = -1;
        exposure_meter = -1;
        
        try {
            if (!exposure_time_off_CB.isSelected()) {
                exposure_time = parseExposureTime(exposure_time_TF.getText());
            }
        }
        catch (ObserveClientException e) {
            observeWindow.showWarningMessage(e.getMessage());
            return false;
        }
        catch (Exception e) {
            observeWindow.showWarningMessage(String.format(
                    "Bad exposure time\n\n%s", e.getMessage()));
            return false;
        }

        try {
            if (!exposure_meter_count_off_CB.isSelected()) {
                exposure_meter = parseExposureMeter(exposure_meter_count_TF.getText());
                exposure_count_of_pulses_PB.setVisible(true);
            }
            else {
                exposure_count_of_pulses_PB.setVisible(false);
            }
        }
        catch (ObserveClientException e) {
            observeWindow.showWarningMessage(e.getMessage());
            return false;
        }
        catch (Exception e) {
            observeWindow.showWarningMessage(String.format(
                    "Bad exposure meter count\n\n%s", e.getMessage()));
            return false;
        }
        
        return true;
    }
    
    public void exposure_control_update(Queue<CallBack> callBacksFIFO) {
        if (!load_exposure_control()) {
            return;
        }

        synchronized (callBacksFIFO) {
            ArrayList<Object> params = new ArrayList<Object>();

            params.clear();
            params.add(exposure_time);
            callBacksFIFO.offer(createCallBack("expose_time_update", params));
        }

        synchronized (callBacksFIFO) {
            ArrayList<Object> params = new ArrayList<Object>();

            params.clear();
            params.add(exposure_meter);
            callBacksFIFO.offer(createCallBack("expose_meter_update", params));
        }
    }
    
    public void exposure_manual_setup(Queue<CallBack> callBacksFIFO) {
        setupExposure = manualSetupExposure;
        
        run_exposure_setup(callBacksFIFO);
    }

    private ManualSetupExposureCoefType getManualSetupExposureCoefType(
            List<ManualSetupExposureCoefType> coefList, double waveLength) {
        
        for (ManualSetupExposureCoefType coef : coefList) {
            if (waveLength < coef.getCentralWaveLengthMax()) {
                return coef;
            }
        }
        
        // TODO: vyvolat vyjimku
        return null;
    }
    
    // TODO: udelat self-test
    public void exposure_calc_wavelength() {
        Integer value = (Integer) exposure_central_wavelength_SN.getValue();
        
        ManualSetupExposureCoefType coefAngle = getManualSetupExposureCoefType(
                manualSetupExposureXML.getCoefAngle().getItem(), value);
        ManualSetupExposureCoefType coefLong = getManualSetupExposureCoefType(
                manualSetupExposureXML.getCoefLong().getItem(), value);
        ManualSetupExposureCoefType coefShort = getManualSetupExposureCoefType(
                manualSetupExposureXML.getCoefShort().getItem(), value);
        
        double angle = coefAngle.getA() * Math.pow(value, 2) + coefAngle.getB()
                * value + coefAngle.getC();

        double degrees = (int) angle;
        double minutes = Math.round((angle - degrees) * 60);
        
        if (minutes == 60) {
            ++degrees;
            minutes = 0;
        }
        
        angle = degrees + (minutes / 60);
        
        double wave_short = coefShort.getA() * Math.pow(angle, 2)
                + coefShort.getB() * angle + coefShort.getC();
        double wave_long = coefLong.getA() * Math.pow(angle, 2)
                + coefLong.getB() * angle + coefLong.getC();
        
        String range = String.format("%.0f-%.0f", wave_short, wave_long);
        
        for (ManualSetupExposureSetupType setup : manualSetupExposureXML.getSetup().getItem()) {
            if (value < setup.getCentralWaveLengthMax()) {
                manualSetupExposure.setName("Manual");
                manualSetupExposure.setRange(range);
                manualSetupExposure.setGa(String.format("%.0f:%.0f", degrees, minutes));
                manualSetupExposure.setDm(setup.getDichroicMirror());
                manualSetupExposure.setSf(setup.getSpectralFilter());
                manualSetupExposure.setFlat(setup.getFlatExposeTime());
                manualSetupExposure.setComp(setup.getCompExposeTime());
                break;
            }
        }
                
        String text = String.format("<html>%s - range %s<br>" +
                "  Grating angle: %s<br>" +
                "  Dichroic mirror: %d<br>" +
                "  Spectral filter: %d<br>" +
                "  Flat: %ds<br>" +
                "  Comp: %ds<br></html>",
                manualSetupExposure.getName(),
                manualSetupExposure.getRange(),            
                manualSetupExposure.getGa(),
                manualSetupExposure.getDm(),
                manualSetupExposure.getSf(),
                manualSetupExposure.getFlat(),
                manualSetupExposure.getComp()
            );
        
        exposure_manual_setup_BT.setToolTipText(text);
        exposure_wavelength_range_LB.setText(range);
    }
    
    public void exposure_setup(Queue<CallBack> callBacksFIFO) {
        String selectedItem = (String)exposure_setup_CBI.getSelectedItem();
        if (selectedItem == null) {
            return;
        }
        
        SetupExposure setupExposure = setupExposureMap.get(selectedItem.toString());
        if (setupExposure == null) {
            this.setupExposure = new SetupExposure();
            return;
        }
        
        this.setupExposure = setupExposure;
        
        run_exposure_setup(callBacksFIFO);
    }
    
    public void run_exposure_setup(Queue<CallBack> callBacksFIFO) {
        String object = exposure_object_CBI.getSelectedItem().toString();
        if (object.equals("flat")) {
            exposure_time_TF.setText(setupExposure.getFlat().toString());
        }
        else if (object.equals("comp")) {
            exposure_time_TF.setText(setupExposure.getComp().toString());
        }
        
        if (getName().equals("oes")) {
            return;
        }

        // Grating angle
        String command = "SPAP 13";
        Integer value;
        
        try {
            value = observeWindow.prepareCoudeGratingAngle(setupExposure.getGa());
        } catch (Exception e) {
            observeWindow.logger.log(Level.WARNING, "prepareCoudeGratingAngle() failed", e);
            return;
        }

        command = String.format("%s %d", command, value.intValue());

        synchronized (callBacksFIFO) {
            callBacksFIFO.offer(new SpectrographExecuteCB(command));
        }
        
        // Dichroic mirror
        synchronized (callBacksFIFO) {
            callBacksFIFO.offer(new SpectrographExecuteCB(
                String.format("SPCH 1 %d", setupExposure.getDm())));
        }
        
        // Spectral filter
        synchronized (callBacksFIFO) {
            callBacksFIFO.offer(new SpectrographExecuteCB(
                String.format("SPCH 2 %d", setupExposure.getSf())));
        }        
    }
    
    private boolean check_exposure_setup() {
        int result;
        String object = exposure_object_CBI.getSelectedItem().toString();
        String collimator = observeWindow.getCollimator();
        String gain_default = "default";
        String collimator_default = "1 - open";
        String readout_speed_default = "";
        String correction_plates_700_default = "";
        String correction_plates_400_default = "";
        String coude_oes_default = "Coude";
        
        // 0 - value
        // 1 - expected_value
        // 2 - description
        List<String[]> items = new LinkedList<String[]>();
        
        if (getName().equals("ccd700")) {
            readout_speed_default = "50kHz";
            gain_default = "high";
            correction_plates_700_default = "in";
            correction_plates_400_default = "out";
        } else if (getName().equals("ccd400")) {
            readout_speed_default = "10kHz";
            correction_plates_700_default = "out";
            correction_plates_400_default = "out";
        }
        
        if (getName().equals("oes")) {
            readout_speed_default = "100kHz";
            collimator = observeWindow.getOesCollimator();
            coude_oes_default = "OES";
        } else if ((!object.equals("zero")) && (!object.equals("dark"))) {
            // Correction plates 700
            items.add(new String[] { observeWindow.getCorrectionPlates700(),
                    correction_plates_700_default,
                    "Spectrograph correction plates 700" });

            // Correction plates 400
            items.add(new String[] { observeWindow.getCorrectionPlates400(),
                    correction_plates_400_default,
                    "Spectrograph correction plates 400" });

            if (!setupExposure.getName().equals("")) {
                // Dichroic mirror
                items.add(new String[] { observeWindow.getDichroicMirror(),
                        setupExposure.getDm().toString(),
                        "Spectrograph dichroic mirror" });

                // Spectral filter
                items.add(new String[] { observeWindow.getSpectralFilter(),
                        setupExposure.getSf().toString(),
                        "Spectrograph spectral filter" });

                // Absolute grating angle
                items.add(new String[] { observeWindow.getGratingAngle(),
                        setupExposure.getGa(), "Spectrograph grating angle" });
            }
        }

        // Mirror Coude/OES
        items.add(new String[]{
                observeWindow.getCoudeOes(),
                coude_oes_default,
                "Mirror Coude/OES"
        });
        
        // CCD gain
        items.add(new String[]{
                ccd_gain_CBI.getSelectedItem().toString(),
                gain_default,
                "CCD gain"
        });
        
        // CCD readout speed
        items.add(new String[]{
                ccd_readout_speed_CBI.getSelectedItem().toString(),
                readout_speed_default,
                "CCD readout speed"
        });
        
        if ((!object.equals("zero")) && (!object.equals("dark"))) {
            // Spectrograph collimator
            items.add(new String[] {
                collimator,
                collimator_default,
                "Spectrograph collimator" });
        }
        
        for (String[] item : items) {
            if (!item[0].equals(item[1])) {
                result = observeWindow.showYesNoMessage(String.format(
                        "WARNING: %s\n\n'%s' (actual) != '%s' (expected).\n\nRun expose?", item[2], item[0], item[1]));
                
                if (result == JOptionPane.NO_OPTION) {
                    return false;
                }
            }            
        }
        
        return true;
    }
    
    public void expose(Queue<CallBack> callBacksFIFO, String observers) {
        String archive;
        ArrayList<Object> params = new ArrayList<Object>();
        
        if (!check_exposure_setup()) {
            return;
        }
        
        if (!load_exposure_control()) {
            return;
        }
        
        synchronized (callBacksFIFO) {
            params.clear();
            params.add("READOUT_SPEED");
            params.add(ccd_readout_speed_CBI.getSelectedItem().toString());
            callBacksFIFO.offer(createCallBack("expose_set", params));
            
            params.clear();
            params.add("GAIN");
            params.add(ccd_gain_CBI.getSelectedItem().toString());
            callBacksFIFO.offer(createCallBack("expose_set", params));

            params.clear();
            params.add("PATH");
            params.add(ccd_data_path_CBI.getSelectedItem().toString());
            callBacksFIFO.offer(createCallBack("expose_set", params));

            archive = "0";
            if (ccd_archive_CB.isSelected()) {
                //params.clear();
                //params.add("ARCHIVEPATH");
                //params.add(ccd_archive_path_CBI.getSelectedItem().toString());
                //callBacksFIFO.offer(createCallBack("expose_set", params));
                archive = "1";
            }

            params.clear();
            params.add("ARCHIVE");
            params.add(archive);
            callBacksFIFO.offer(createCallBack("expose_set", params));

            params.clear();
            params.add("IMAGETYP");
            
            String imagetyp = exposure_object_CBI.getSelectedItem().toString();
            if (imagetyp.equals("target")) {
                imagetyp = "object";
            }
            
            params.add(imagetyp);
            params.add("");
            callBacksFIFO.offer(createCallBack("expose_set_key", params));

            params.clear();
            params.add("OBJECT");
            params.add(exposure_target_TF.getText());
            params.add("");
            callBacksFIFO.offer(createCallBack("expose_set_key", params));

            params.clear();
            params.add("OBSERVER");
            params.add(observers);
            params.add("");
            callBacksFIFO.offer(createCallBack("expose_set_key", params));

            if (this.getName().equals("oes")) {
                params.clear();
                params.add("SPECFILT");
                params.add(this.observeWindow.getOesSPECFILT());
                params.add("");
                callBacksFIFO.offer(createCallBack("expose_set_key", params));

                params.clear();
                params.add("SLITHEIG");
                params.add(this.observeWindow.getOesSLITHEIG());
                params.add("");
                callBacksFIFO.offer(createCallBack("expose_set_key", params));
            }

            params.clear();
            params.add(exposure_time);
            params.add(exposure_count_repeat_SN.getValue());
            params.add(exposure_meter);
            callBacksFIFO.offer(createCallBack("expose_start", params));
        }
    }

    public String get_ccd_data_path() {
        return ccd_data_path_CBI.getSelectedItem().toString();
    }

    public String get_ccd_archive_path() {
        return ccd_archive_path_CBI.getSelectedItem().toString();
    }

    private void append_new_values2CBI(JComboBox cb, String values) {
        int count = cb.getItemCount();
        List items = Arrays.asList(values.split(";"));
        List<Object> list = new ArrayList<Object>();

        for (int i = 0; i < count; ++i) {
            list.add(cb.getItemAt(i));
        }

        for (Object item : items) {
            if (!list.contains(item)) {
                cb.addItem(item);
            }
        }
    }

    public String seconds2human(int seconds) {
        int minutes;
        int hours;

        if (seconds < 60) {
            return String.format("00:00:%02d", seconds);
        }
        else if (seconds < 3600) {
            minutes = seconds / 60;
            seconds = seconds % 60;

            return String.format("00:%02d:%02d", minutes, seconds);
        }
        else {
            hours = seconds / 3600;
            seconds = seconds % 3600;

            minutes = seconds / 60;
            seconds = seconds % 60;

            return String.format("%02d:%02d:%02d", hours, minutes, seconds);
        }
    }

    public void exposeInit(ExposeState exposeState) {
        ExposeInfoXML exposeInfoXML = exposeState.getExposeInfoXML();

        append_new_values2CBI(ccd_readout_speed_CBI, exposeState.getReadoutSpeeds());
        append_new_values2CBI(ccd_gain_CBI, exposeState.getGains());

        exposure_count_repeat_SN.setValue(exposeInfoXML.getExposeCount());
        //exposure_length_SN.setValue(exposeInfoXML.getFullTime());
        exposure_object_CBI.setSelectedItem(exposeState.getObject());
        exposure_target_TF.setText(exposeState.getTarget());
        //ccd_data_path_CBI.setSelectedItem(exposeInfoXML.getPath());
        ccd_gain_CBI.setSelectedIndex(ccd_gain_CBI.getItemCount()-1);
    }
    
    public void loadSetupExposure(Map<String, SetupExposure> setupExposureMap) {
        this.setupExposureMap = setupExposureMap;
        
        exposure_setup_CBI.removeAllItems();
        
        List<String> sortedKeys = new ArrayList<String>(setupExposureMap.keySet());
        
        Collections.sort(sortedKeys, new Comparator<String>() {
            @Override
            public int compare(String s1, String s2) {
                return s1.compareToIgnoreCase(s2);
            }
        });
        
        exposure_setup_CBI.addItem("unset");
        
        for (String key : sortedKeys) {
            exposure_setup_CBI.addItem(key);
            
            if ((getName().equals("oes")) && (key.equals("default"))) {
                setupExposure = setupExposureMap.get(key);
            }
        }
    }
    
    public void setManualSetupExposure(ManualSetupExposureXML manualSetupExposureXML) {
        if (manualSetupExposureXML != null) {
            this.manualSetupExposureXML = manualSetupExposureXML;
            exposure_manual_setup_setEnabled(true);
        }
    }

    // TODO: implementovat OES
    public void spectrographRefresh(Spectrograph spectrograph) {
        spectrograph.setJLabel(coude_dm_LB, Spectrograph.Element.DICHROIC_MIRROR);
        spectrograph.setJLabel(coude_sf_LB, Spectrograph.Element.SPECTRAL_FILTER);
        spectrograph.setJLabel(coude_ga_LB, Spectrograph.Element.GRATING_POS);
        spectrograph.setJLabel(coude_ga_state_LB, Spectrograph.Element.GRATING);
        spectrograph.setJLabel(coude_em_LB, Spectrograph.Element.EXP_COUNT);
        
        String object = exposure_object_CBI.getSelectedItem().toString();
        
        if (object.equals("target")) {
            exposure_count_of_pulses_PB.setString(String.format(
                    "%s from %.1f Mcounts", coude_em_LB.getText(),
                    exposure_meter / 1000000.0));

            double count = Double.parseDouble(spectrograph
                    .getValue(Spectrograph.Element.EXP_COUNT));
            double percent = exposure_meter / 100.0;
            int value = (int) (count / percent);

            exposure_count_of_pulses_PB.setValue(value);
        }
    }
    
    public void ccdRefresh(ExposeInfoXML exposeInfoXML) {
        String state;
        String elapsedTime = null;
        String remainedTime = null;
        String fullTime = null;
        boolean ready = false;

        if (exposeInfoXML.getElapsedTime() != -1) {
            elapsedTime = seconds2human(exposeInfoXML.getElapsedTime());
            remainedTime = seconds2human(exposeInfoXML.getFullTime() - exposeInfoXML.getElapsedTime());
            fullTime = seconds2human(exposeInfoXML.getFullTime());
        }

        exposure_file_fits_TF.setText(exposeInfoXML.getFilename());

        if (exposeInfoXML.isState("ready")) {
            exposure_PB.setIndeterminate(false);
            exposure_PB.setStringPainted(true);
            exposure_state_LB.setText("Ready");
            exposure_PB.setValue(exposure_PB.getMaximum());
            ready = true;
        }
        else if (exposeInfoXML.isState("prepare expose")) {
            exposure_PB.setIndeterminate(true);
            exposure_PB.setStringPainted(false);
            exposure_state_LB.setText("Preparing expose");
        }
        else if (exposeInfoXML.isState("expose")) {
            exposure_PB.setIndeterminate(false);
            exposure_PB.setStringPainted(true);
            state = String.format("%d. exposing, elapsed %s, remains %s from %s",
                                  exposeInfoXML.getExposeNumber(), elapsedTime, remainedTime, fullTime);
            exposure_state_LB.setText(state);
            exposure_PB.setMaximum(exposeInfoXML.getFullTime());
            exposure_PB.setValue(exposeInfoXML.getElapsedTime());
        }
        else if (exposeInfoXML.isState("finish expose")) {
            exposure_PB.setIndeterminate(true);
            exposure_PB.setStringPainted(false);
            exposure_state_LB.setText("Finishing expose");
        }
        else if (exposeInfoXML.isState("readout")) {
            if (!showFitsFile) {
                exposeNumber = exposeInfoXML.getExposeNumber();
                lastFitsFile = exposure_file_fits_TF.getText();
            }
            showFitsFile = true;
            exposure_PB.setIndeterminate(false);
            exposure_PB.setStringPainted(true);
            state = String.format("%d. reading out CCD, elapsed %s, remains %s from %s",
                                  exposeInfoXML.getExposeNumber(), elapsedTime, remainedTime, fullTime);
            exposure_state_LB.setText(state);
            exposure_PB.setMaximum(exposeInfoXML.getFullTime());
            exposure_PB.setValue(exposeInfoXML.getElapsedTime());
        }
        
        if ((!exposeInfoXML.isState("readout") || (exposeNumber != exposeInfoXML
                .getExposeNumber())) && (showFitsFile)) {
            showFitsFile = false;

            try {
                String[] cmdArray = new String[2];

                cmdArray[0] = "/opt/exposed/bin/show_fits.sh";
                cmdArray[1] = String.format("%s/%s", ccd_data_path_CBI
                        .getSelectedItem().toString(), lastFitsFile);

                Runtime.getRuntime().exec(cmdArray);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        if (ready) {
            exposure_count_repeat_SN.setEnabled(true);
            //exposure_length_SN.setEnabled(true);
            exposure_object_CBI.setEnabled(true);
            ccd_gain_CBI.setEnabled(true);
            exposure_target_TF.setEnabled(true);
            exposure_file_fits_TF.setEnabled(true);
            ccd_readout_speed_CBI.setEnabled(true);
            ccd_data_path_CBI.setEnabled(true);
            ccd_archive_path_CBI.setEnabled(true);
            // TODO: deprecated
            //ccd_archive_CB.setEnabled(true);
            exposure_start_BT.setEnabled(true);

            exposure_readout_BT.setEnabled(false);
            exposure_abort_BT.setEnabled(false);
            exposure_state_LB.setBackground(green_color);
            
            if (getName().equals("ccd700")) {
                exposure_abort_BT.setEnabled(false);    
            }
        }
        else {
            exposure_count_repeat_SN.setEnabled(false);
            //exposure_length_SN.setEnabled(false);
            exposure_object_CBI.setEnabled(false);
            ccd_gain_CBI.setEnabled(false);
            exposure_target_TF.setEnabled(false);
            exposure_file_fits_TF.setEnabled(false);
            ccd_readout_speed_CBI.setEnabled(false);
            ccd_data_path_CBI.setEnabled(false);
            ccd_archive_path_CBI.setEnabled(false);
            // TODO: deprecated
            //ccd_archive_CB.setEnabled(false);
            exposure_start_BT.setEnabled(false);

            exposure_readout_BT.setEnabled(true);
            exposure_abort_BT.setEnabled(true);

            exposure_state_LB.setBackground(yellow_color);
            
            if (getName().equals("ccd700")) {
                exposure_abort_BT.setEnabled(false);    
            }
        }

        append_new_values2CBI(ccd_data_path_CBI, exposeInfoXML.getPaths());
        append_new_values2CBI(ccd_archive_path_CBI, exposeInfoXML.getArchivePaths());

        ccd_actual_temp_LB.setText(String.format("%.1f", exposeInfoXML.getCCDTemp()));

        if (exposeInfoXML.isCCDTempAlarm()) {
            ccd_actual_temp_LB.setBackground(red_color);
        }
        else {
            ccd_actual_temp_LB.setBackground(green_color);
        }
    }

    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JLabel ccd_actual_temp_LB;
    private javax.swing.JLabel ccd_actual_temp_title_LB;
    private javax.swing.JCheckBox ccd_archive_CB;
    private javax.swing.JComboBox ccd_archive_path_CBI;
    private javax.swing.JLabel ccd_archive_path_LB;
    private javax.swing.JComboBox ccd_data_path_CBI;
    private javax.swing.JLabel ccd_data_path_LB;
    private javax.swing.JComboBox ccd_gain_CBI;
    private javax.swing.JLabel ccd_gain_LB;
    private javax.swing.JComboBox ccd_readout_speed_CBI;
    private javax.swing.JLabel ccd_readout_speed_LB;
    private javax.swing.JSpinner ccd_require_temp_SN;
    private javax.swing.JLabel ccd_require_temp_title_LB;
    private javax.swing.JLabel coude_dm_LB;
    private javax.swing.JLabel coude_dm_label_LB;
    private javax.swing.JLabel coude_em_LB;
    private javax.swing.JLabel coude_em_label_LB;
    private javax.swing.JLabel coude_ga_LB;
    private javax.swing.JLabel coude_ga_label_LB;
    private javax.swing.JLabel coude_ga_state_LB;
    private javax.swing.JLabel coude_sf_LB;
    private javax.swing.JLabel coude_sf_label_LB;
    private javax.swing.JProgressBar exposure_PB;
    private javax.swing.JButton exposure_abort_BT;
    private javax.swing.JCheckBox exposure_autogenerate_file_fits_CB;
    private javax.swing.JLabel exposure_central_wavelength_LB;
    private javax.swing.JSpinner exposure_central_wavelength_SN;
    private javax.swing.ButtonGroup exposure_control_BG;
    private javax.swing.JPanel exposure_control_P;
    private javax.swing.JButton exposure_control_update_BT;
    private javax.swing.JProgressBar exposure_count_of_pulses_PB;
    private javax.swing.JLabel exposure_count_repeat_LB;
    private javax.swing.JSpinner exposure_count_repeat_SN;
    private javax.swing.JLabel exposure_file_fits_LB;
    private javax.swing.JTextField exposure_file_fits_TF;
    private javax.swing.JButton exposure_manual_setup_BT;
    private javax.swing.JLabel exposure_meter_count_LB;
    private javax.swing.JTextField exposure_meter_count_TF;
    private javax.swing.JCheckBox exposure_meter_count_off_CB;
    private javax.swing.JComboBox exposure_object_CBI;
    private javax.swing.JLabel exposure_object_LB;
    private javax.swing.JButton exposure_readout_BT;
    private javax.swing.JButton exposure_setup_BT;
    private javax.swing.JComboBox exposure_setup_CBI;
    private javax.swing.JLabel exposure_setup_LB;
    private javax.swing.JButton exposure_start_BT;
    private javax.swing.JLabel exposure_state_LB;
    private javax.swing.JLabel exposure_target_LB;
    private javax.swing.JTextField exposure_target_TF;
    private javax.swing.JLabel exposure_time_LB;
    private javax.swing.JTextField exposure_time_TF;
    private javax.swing.JCheckBox exposure_time_off_CB;
    private javax.swing.JLabel exposure_wavelength_range_LB;
    private javax.swing.JSeparator jSeparator1;
    private javax.swing.JSeparator jSeparator2;
    private javax.swing.JPanel observe_ccd_P;
    private javax.swing.JPanel observe_coude_P;
    private javax.swing.JPanel observe_exposure_P;
    private javax.swing.JPanel observe_others_P;
    private javax.swing.JPanel observe_output_P;
    // End of variables declaration//GEN-END:variables
}
