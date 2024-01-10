/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *
 *   Copyright (C) 2008-2014 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

import cz.cas.asu.stelweb.fuky.observe.jaxb.ManualSetupExposureXML;
import cz.cas.asu.stelweb.fuky.observe.xml.SetupClientXML;
import cz.cas.asu.stelweb.fuky.observe.xml.TelescopeInfoXML;
import cz.cas.asu.stelweb.fuky.observe.xml.TelescopeExecuteXML;
import cz.cas.asu.stelweb.fuky.observe.xml.SpectrographInfoXML;
import cz.cas.asu.stelweb.fuky.observe.xml.SpectrographExecuteXML;
import cz.cas.asu.stelweb.fuky.observe.xml.ExposeInfoXML;
import cz.cas.asu.stelweb.fuky.observe.xml.ExposeExecuteXML;
import cz.cas.asu.stelweb.fuky.observe.xml.UserXML;
import cz.cas.asu.stelweb.fuky.observe.xml.setup_exposure.SetupExposureXML;

import java.io.StringReader;

import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response.Status;
import javax.xml.bind.JAXBContext;
import javax.xml.bind.JAXBElement;
import javax.xml.bind.Unmarshaller;
import javax.xml.bind.JAXBException;
import javax.xml.transform.stream.StreamSource;

import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.config.ClientConfig;
import com.sun.jersey.api.client.config.DefaultClientConfig;
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter;
import com.sun.jersey.client.urlconnection.HTTPSProperties;
import com.sun.jersey.api.client.WebResource;

import javax.net.ssl.SSLContext;
import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.X509TrustManager;
import javax.net.ssl.TrustManager;

import java.security.cert.X509Certificate;
import java.security.cert.CertificateException;

import javax.net.ssl.SSLSession;

public class JerseyClientSSL extends Client {

    private X509TrustManager x509TrustManager = new X509TrustManager() {

        @Override
        public void checkClientTrusted(X509Certificate[] arg0, String arg1) throws CertificateException {
            return;
        }

        @Override
        public void checkServerTrusted(X509Certificate[] arg0, String arg1) throws CertificateException {
            return;
        }

        @Override
        public X509Certificate[] getAcceptedIssuers() {
            return null;
        }
    };

    private HostnameVerifier hostnameVerifier = new HostnameVerifier() {

        @Override
        public boolean verify(String hostname, SSLSession sslSession) {
            return true;
        }
    };

    private static final int OK = Status.OK.getStatusCode();

    private TrustManager trustManager[] = { x509TrustManager };
    private ObserveConnect connect;
    private ClientConfig config = new DefaultClientConfig();
    private WebResource webResource;
    private HTTPBasicAuthFilter authFilter = null;
    private Client client;

    public JerseyClientSSL(ObserveConnect connect) {
        super();

        this.connect = connect;

        try {
            SSLContext ctx = SSLContext.getInstance("SSL");
            ctx.init(null, trustManager, null);
            this.config.getProperties().put(HTTPSProperties.PROPERTY_HTTPS_PROPERTIES, new HTTPSProperties(hostnameVerifier, ctx));
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        this.reload();

        //System.out.printf("%s:%s\n", connect.username, connect.password);
    }

    public void reload() {
        this.client = Client.create(this.config);

        //if (this.authFilter != null) {
        //    this.client.removeFilter(this.authFilter);
        //}

        this.authFilter = new HTTPBasicAuthFilter(connect.username, connect.password);
        this.client.addFilter(this.authFilter);

        this.webResource = this.client.resource(
            String.format("https://%s:%d/observe", connect.host, connect.port));
        this.webResource.addFilter(new ObserveClientFilter(5000, 5000));
    }

    public TelescopeInfoXML getTelescopeInfoXML() throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path("telescope").
            accept(MediaType.APPLICATION_XML).
            get(ClientResponse.class);

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }

        return (TelescopeInfoXML)this.response2object(response, TelescopeInfoXML.class);
    }

    public TelescopeExecuteXML putTelescopeExecuteXML(String functionName, Object[] functionParams) throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path("telescope").
            accept(MediaType.APPLICATION_XML).
            put(ClientResponse.class, new TelescopeExecuteXML(functionName, functionParams));

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }

        return (TelescopeExecuteXML)this.response2object(response, TelescopeExecuteXML.class);
    }

    public UserXML getUser(String username) throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path(String.format("users/%s", username)).
            accept(MediaType.APPLICATION_XML).
            get(ClientResponse.class);

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }

        return (UserXML)this.response2object(response, UserXML.class);
    }

    private Object response2object(ClientResponse response, Class... classesToBeBound) {
        try {
            JAXBContext context = JAXBContext.newInstance(classesToBeBound);
            Unmarshaller unmarshaller = context.createUnmarshaller();
            return unmarshaller.unmarshal(new StringReader(response.getEntity(String.class)));
        }
        catch (JAXBException e) {
            System.out.println(e.getMessage());
        }

        // TODO
        return null;
    }

    public ExposeInfoXML getExposeInfoXML(String instrument) throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path(instrument).
            accept(MediaType.APPLICATION_XML).
            get(ClientResponse.class);

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }

        return (ExposeInfoXML)this.response2object(response, ExposeInfoXML.class);
    }

    // TODO: dodelat
    public SetupExposureXML getSetupExposureXML(String instrument) throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path(String.format("setup/exposure_%s.xml", instrument)).
            accept(MediaType.APPLICATION_XML).
            get(ClientResponse.class);

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }
        
        SetupExposureXML setupExposureXML = (SetupExposureXML) response2object(
                response, SetupExposureXML.class);
        
        setupExposureXML.setInstrument(instrument);
        
        return setupExposureXML;
    }
    
    // TODO: lepe osetrit vyjimky
    public ManualSetupExposureXML getManualSetupExposureXML() throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path("setup/exposure_manual.xml").
            accept(MediaType.APPLICATION_XML).
            get(ClientResponse.class);

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }
        
        try {
            JAXBContext context = JAXBContext.newInstance("cz.cas.asu.stelweb.fuky.observe.jaxb");
            Unmarshaller unmarshaller = context.createUnmarshaller();
            StringReader stringReader = new StringReader(response.getEntity(String.class));
            StreamSource streamSource = new StreamSource(stringReader);
            
            JAXBElement<ManualSetupExposureXML> jaxbElement = unmarshaller
                    .unmarshal(streamSource, ManualSetupExposureXML.class); 
            
            return jaxbElement.getValue();
        } catch (JAXBException e) {
            System.out.println(e.getMessage());
            return null;
        }
    }
    
    public SetupClientXML getSetupClientXML() throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path("setup/client.xml").
            accept(MediaType.APPLICATION_XML).
            get(ClientResponse.class);

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }
        
        SetupClientXML setupClientXML = (SetupClientXML) response2object(
                response, SetupClientXML.class);
        
        return setupClientXML;
    }
    
    public ExposeExecuteXML putExposeExecuteXML(String instrument, String functionName, Object[] functionParams) throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path(instrument).
            accept(MediaType.APPLICATION_XML).
            put(ClientResponse.class, new ExposeExecuteXML(functionName, functionParams));

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }

        return (ExposeExecuteXML)this.response2object(response, ExposeExecuteXML.class);
    }

    // TODO: odstraneni duplicity
    public SpectrographInfoXML getSpectrographInfoXML() throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path("spectrograph").
            accept(MediaType.APPLICATION_XML).
            get(ClientResponse.class);

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }

        return (SpectrographInfoXML)this.response2object(response, SpectrographInfoXML.class);
    }

    // TODO: odstraneni duplicity
    public SpectrographExecuteXML putSpectrographExecuteXML(String functionName, Object[] functionParams) throws NotOkStatusException {
        ClientResponse response = this.webResource.
            path("spectrograph").
            accept(MediaType.APPLICATION_XML).
            put(ClientResponse.class, new SpectrographExecuteXML(functionName, functionParams));

        if (response.getStatus() != OK) {
            throw new NotOkStatusException(response.getStatus());
        }

        return (SpectrographExecuteXML)this.response2object(response, SpectrographExecuteXML.class);
    }
}
