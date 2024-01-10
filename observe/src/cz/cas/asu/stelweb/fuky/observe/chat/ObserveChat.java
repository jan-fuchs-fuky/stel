/*
 *   Author: Jan Fuchs <fuky@asu.cas.cz>
 *
 *   Copyright (C) 2016 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

package cz.cas.asu.stelweb.fuky.observe.chat;

import java.io.IOException;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.layout.StackPane;
import javafx.stage.Stage;
 
// TODO: rozpracovano
public class ObserveChat extends Application {
    
    @Override
    public void start(Stage stage) {
        Parent root = null;

        try {
            root = FXMLLoader.load(getClass().getResource("ObserveChat.fxml"));
        }
        catch (IOException e) {
            e.printStackTrace();
            System.exit(1);
        }
    
        Scene scene = new Scene(root, 300, 275);
    
        stage.setTitle("FXML Welcome");
        stage.setScene(scene);
        stage.show();

        //primaryStage.setTitle("Hello World!");
        //Button btn = new Button();
        //btn.setText("Say 'Hello World'");
        //btn.setOnAction(new EventHandler<ActionEvent>() {
 
        //    @Override
        //    public void handle(ActionEvent event) {
        //        System.out.println("Hello World!");
        //    }
        //});
        //
        //StackPane root = new StackPane();
        //root.getChildren().add(btn);
        //primaryStage.setScene(new Scene(root, 300, 250));
        //primaryStage.show();
    }

    public static void main(String[] args) {
        launch(args);
    }
}

/*
public class ObserveChat {

    // TODO: rozpracovano
    public ObserveChat() {
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
            baseDir + "/properties/observe-chat.properties");

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
        javax.swing.SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                new ObserveChat();
            }
        });
    }
}
*/
