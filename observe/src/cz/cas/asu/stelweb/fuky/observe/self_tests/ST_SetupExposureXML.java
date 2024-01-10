package cz.cas.asu.stelweb.fuky.observe.self_tests;

import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;

import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBException;
import javax.xml.bind.Unmarshaller;

import cz.cas.asu.stelweb.fuky.observe.xml.setup_exposure.SetupExposureXML;
import cz.cas.asu.stelweb.fuky.observe.xml.setup_exposure.SetupExposure;

public class ST_SetupExposureXML {
    
    private static final String SETUP_EXPORUSE_XML = "/tmp/setup_exposure.xml";
    
    public static void main(String[] args) throws JAXBException, IOException {
        
        JAXBContext context = JAXBContext.newInstance(SetupExposureXML.class);
        Unmarshaller um = context.createUnmarshaller();
        SetupExposureXML setupExposureXML = (SetupExposureXML) um.unmarshal(new FileReader(
                SETUP_EXPORUSE_XML));
        
        ArrayList<SetupExposure> setupExposureList = setupExposureXML.getSetupExposureList();
        for (SetupExposure setupExposure : setupExposureList) {
            System.out.println(setupExposure.getName());
        }
    }
}
