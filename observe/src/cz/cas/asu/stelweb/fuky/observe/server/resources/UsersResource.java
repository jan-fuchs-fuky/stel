/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2010-2013 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

package cz.cas.asu.stelweb.fuky.observe.server.resources;

import java.io.StringWriter;
import java.net.URI;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import javax.servlet.ServletContext;
import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.MultivaluedMap;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.SecurityContext;
import javax.xml.bind.JAXBContext;
import javax.xml.bind.Marshaller;

import org.hibernate.Session;
import org.hibernate.SessionFactory;

import com.sun.jersey.api.view.Viewable;

import cz.cas.asu.stelweb.fuky.observe.server.Mail;
import cz.cas.asu.stelweb.fuky.observe.xml.UserXML;

@Path("/users")
public class UsersResource {
    SecurityContext sc;
    HttpServletRequest httpServletRequest;
    ServletContext servletContext;
    String role;
    SessionFactory sessionFactory;
    String login;

    public UsersResource(@Context SecurityContext sc,
                         @Context HttpServletRequest httpServletRequest,
                         @Context SessionFactory sessionFactory,
                         @Context ServletContext servletContext) {
        this.sessionFactory = sessionFactory;
        this.httpServletRequest = httpServletRequest;
        this.servletContext = servletContext;
        this.login = sc.getUserPrincipal().getName();

        if (sc.isUserInRole("admin")) {
            this.role = "admin";
        }
        else if (ObserveCommon.allowAddr(httpServletRequest, servletContext, sessionFactory) &&
                 this.login.equals("tcsuser")) {
            this.role = "permission";
        }
        else {
            this.role = "none";
        }
    }

    public Map<String, Object> getUsers() {
        Map<String, Object> rootMap = new HashMap<String, Object>();

        try {
            rootMap.put("users", getUsersMap());
            rootMap.put("role", this.role);
            rootMap.put("login", this.login);
        }
        catch (Exception e) {
            servletContext.log("UsersResource.getUsers()", e);
            throw new WebApplicationException(e);
        }

        return rootMap;
    }

    @GET 
    @Produces(MediaType.TEXT_HTML)
    public Viewable getUsersHTML() {
        return new Viewable("/users.ftl", getUsers());
    }

    @GET 
    @Produces({MediaType.APPLICATION_XML, MediaType.TEXT_XML})
    public Viewable getUsersXML() {
        return new Viewable("/users_xml.ftl", getUsers());
    }

    @GET 
    @Path("{login}")
    @Produces({MediaType.APPLICATION_XML, MediaType.TEXT_XML})
    public String getUser(@PathParam("login") String login) {
        Map<String, String> user = null;
        UserXML userXML = new UserXML();
        StringWriter response = new StringWriter();

        user = getUsersMap(login).getFirst();

        try {
            userXML.setLogin(user.get("login"));
            userXML.setFirstName(user.get("first_name"));
            userXML.setLastName(user.get("last_name"));
            userXML.setEmail(user.get("email"));
            userXML.setPermission(user.get("permission"));

            JAXBContext context = JAXBContext.newInstance(UserXML.class);
            Marshaller marshaller = context.createMarshaller();

            marshaller.setProperty(Marshaller.JAXB_FORMATTED_OUTPUT, true);
            marshaller.marshal(userXML, response);
            response.flush();

            return response.toString();
        }
        catch (Exception e) {
            servletContext.log("UsersResource.getUser()", e);
            throw new WebApplicationException(e);
        }
    }

    private void sendChangePwdLink(String login, Map<String, Object> rootMap)
    {
        Session session = null;
        UUID uuid = UUID.randomUUID();
        String from = "observe@sunstel.asu.cas.cz";
        String subject = String.format("Observe: change '%s' password", login);
        String body = String.format(
                "Link for change password: https://localhost:8443/observe/users/%s/%s",
                login, uuid.toString());
        
        rootMap.put("message", String.format(
                "Link for change password was sended on '%s' e-mail address.", login));
        
        try {
            session = sessionFactory.openSession();
            String hqlSelect = String.format("from User u where u.login like :login");
            
            session.beginTransaction();
            User user = (User)session.createQuery(hqlSelect)
                .setString("login", login).list().get(0);
            session.getTransaction().commit();
            
            Mail mail = new Mail(from, user.getEmail(), subject, body);
            
            mail.send();
            
            String hqlUpdate = "update User u set "
                             + "u.changePwdUUID = :uuid, "
                             + "u.changePwdDate = :date "
                             + "where login like :login";
            
            session.beginTransaction();
            session.createQuery(hqlUpdate)
                .setInteger("uuid", uuid.hashCode())
                .setString("login", login)
                .setDate("date", new Date())
                .executeUpdate();
            session.getTransaction().commit();
        }
        catch (Exception e) {
            throw new WebApplicationException(new Exception(String.format(
                    "Send e-mail to '%s' failed: %s", login, e.getMessage())), Status.NOT_FOUND);
        }
        finally {
            if (session != null) {
                session.close();
            }
        }        
    }
    
    @GET
    @Path("{login}/{change_pwd_uuid}")
    @Produces(MediaType.TEXT_HTML)
    public Viewable changePasswordLink(@PathParam("login") String login,
            @PathParam("change_pwd_uuid") String change_pwd_uuid) {
        Map<String, Object> rootMap = new HashMap<String, Object>();
        
        if (login.equals("admin")) {
            throw new WebApplicationException(Status.BAD_REQUEST);
        }
        
        rootMap.put("role", this.role);
        rootMap.put("login", login);
        
        if (change_pwd_uuid.equals("change_password")) {
            sendChangePwdLink(login, rootMap);
        }
        else {
            rootMap.put("change_pwd_uuid", change_pwd_uuid);
            rootMap.put("message", String.format(
                    "Please enter new password for user '%s'", login));            
        }
        
        return new Viewable("/users.ftl", rootMap);
    }
    
    @POST
    @Path("{login}/{change_pwd_uuid}")
    @Produces(MediaType.TEXT_HTML)
    public Viewable changePassword(
            @PathParam("login") String login,
            @PathParam("change_pwd_uuid") String change_pwd_uuid,
            MultivaluedMap<String, String> formParams) {
        Map<String, Object> rootMap = new HashMap<String, Object>();
        String password;
        
        rootMap.put("role", this.role);
        rootMap.put("login", login);

        // TODO: odstranit duplicitu
        if (formParams.containsKey("password") && formParams.containsKey("confirm_password")) {
            password = formParams.getFirst("password");
            
            if (!password.equals(formParams.getFirst("confirm_password"))) {
                rootMap.put("change_pwd_uuid", change_pwd_uuid);
                rootMap.put("message", "Please confirm your password. Passwords are not identical.");
                return new Viewable("/users.ftl", rootMap);
            }
        }
        else {
            throw new WebApplicationException(Status.BAD_REQUEST);
        }
        
        Session session = null;
        
        try {
            session = this.sessionFactory.openSession();
            
            String hqlSelect = String.format("from User u where u.login like :login");
            session.beginTransaction();
            User user = (User)session.createQuery(hqlSelect)
                .setString("login", login).list().get(0);
            session.getTransaction().commit();

            if (user.getChangePwdUUID() != UUID.fromString(change_pwd_uuid).hashCode()) {
                throw new WebApplicationException(Status.BAD_REQUEST);    
            }
            
            String passwordDigest = new ObservePassword(password).getPasswordDigest();
            String hqlUpdate = "update User u set u.password = :passwordDigest where login like :login";
            session.beginTransaction();
            session.createQuery(hqlUpdate)
                .setString("passwordDigest", passwordDigest)
                .setString("login", login)
                .executeUpdate();
            session.getTransaction().commit();

            rootMap.put("message",
                    String.format("Password has been successfully changed"));
        }
        catch (WebApplicationException e) {
            throw e;
        }
        catch (Exception e) {
            servletContext.log("UsersResource.changePassword()", e);
            throw new WebApplicationException(e);
        }
        finally {
            if (session != null) {
                session.close();
            }
        }
        
        return new Viewable("/users.ftl", rootMap);
    }
    
    @POST
    //@DELETE
    @Path("{login}")
    @Produces(MediaType.TEXT_HTML)
    public Viewable deleteUser(@PathParam("login") String login) {
        Map<String, Object> rootMap = new HashMap<String, Object>();
        Session session = null;

        if (!this.role.equals("admin")) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }

        rootMap.put("role", this.role);
        rootMap.put("login", this.login);

        try {
            String hqlDelete = String.format("delete from User user where login='%s'", login);

            session = this.sessionFactory.openSession();
            session.beginTransaction();
            session.createQuery(hqlDelete).executeUpdate();
            session.getTransaction().commit();

            rootMap.put("users", getUsersMap());
            rootMap.put("message", String.format("User '%s' was deleted successfully.", login));
        }
        catch (Exception e) {
            servletContext.log("UsersResource.deleteUser()", e);
            throw new WebApplicationException(e);
        }
        finally {
            if (session != null) {
                session.close();
            }
        }

        return new Viewable("/users.ftl", rootMap);
    }

    @POST
    @Produces(MediaType.TEXT_HTML)
    @Consumes(MediaType.APPLICATION_FORM_URLENCODED)
    public Viewable postUser(MultivaluedMap<String, String> formParams) {
        Map<String, Object> rootMap = new HashMap<String, Object>();
        boolean update = false;
        UserForm userForm = null;
        Session session = null;
        String login = formParams.getFirst("login");
        String password = formParams.getFirst("password");

        if (login.equals(this.login) && (!login.equals("tcsuser")) && (!login.equals("admin"))) {
            this.role = "user";
        }
        else if (this.role.equals("none")) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }
        else if (!this.role.equals("admin") && login.equals("admin")) {
            throw new WebApplicationException(Status.FORBIDDEN);
        }

        rootMap.put("role", this.role);
        rootMap.put("login", this.login);

        try {
            User user = null;

            session = sessionFactory.openSession();

            if (formParams.getFirst("login").equals("all")) {
                update = true;
            }
            else {
                session.beginTransaction();
                List result = session.createQuery(String.format(
                    "from User u where u.login like '%s'", login)).list();
                session.getTransaction().commit();
                session.clear();

                if (!result.isEmpty()) {
                    update = true;
                    user = (User)result.get(0);
                }
            }

            userForm = new UserForm(formParams, this.login, update, this.role);

            if ((!update) && (!this.role.equals("admin"))) {
                throw new WebApplicationException(Status.FORBIDDEN);
            }

            if (userForm.isBad() || (!formParams.containsKey("password"))) {
                return new Viewable("/users.ftl", userForm.getRootMap());
            }

            String passwordDigest = new ObservePassword(password).getPasswordDigest();

            if (user == null) {
                user = new User(login, passwordDigest);
            }

            if (!password.isEmpty()) {
                user.setPassword(passwordDigest);
            }

            user.setFirstName(formParams.getFirst("first_name"));
            user.setLastName(formParams.getFirst("last_name"));
            user.setEmail(formParams.getFirst("email"));

            if (this.role.equals("admin")) {
                user.setPermission(formParams.getFirst("permission"));
            }

            session.beginTransaction();

            if (update) {
                rootMap.put("message", "User was updated successfully.");
                if (this.role.equals("permission")) {
                    if (formParams.getFirst("permission").equals("admin") ||
                        formParams.getFirst("login").equals("admin") ||
                        formParams.getFirst("login").equals("tcsuser")) {
                        throw new WebApplicationException(Status.FORBIDDEN);
                    }

                    if (password.isEmpty()) {
                        String where = "login not like 'admin' and login not like 'tcsuser'";

                        if (!login.equals("all")) {
                            where = String.format("u.login = '%s'", login);
                        }

                        String hqlUpdate = "update User u set u.permission = :permission where " + where;

                        session.createQuery(hqlUpdate)
                            .setString("permission", formParams.getFirst("permission"))
                            .executeUpdate();
                    }
                    else {
                        if (!this.login.equals(login)) {
                            throw new WebApplicationException(Status.FORBIDDEN);
                        }
                        else {
                            session.saveOrUpdate(user);
                        }
                    }
                }
                else {
                    session.saveOrUpdate(user);
                }
            }
            else {
                rootMap.put("message", "User was added successfully.");
                session.saveOrUpdate(user);
            }

            session.getTransaction().commit();

            rootMap.put("users", getUsersMap());
        }
        catch (Exception e) {
            // TODO: if userForm == null
            servletContext.log("UsersResource.postUser()", e);
            userForm.setMessage(e.getMessage());

            return new Viewable("/users.ftl", userForm.getRootMap());
        }
        finally {
            if (session != null) {
                session.close();
            }
        }

        return new Viewable("/users.ftl", rootMap);
    }

    private LinkedList<Map<String, String>> getUsersMap() {
        return getUsersMap(null);
    }

    private LinkedList<Map<String, String>> getUsersMap(String login) {
        LinkedList<Map<String, String>> users = new LinkedList<Map<String, String>>();
        String where = "";
        String hqlSelect;
        Session session = null;

        if (login != null) {
            where = String.format(" where login='%s'", login);
        }

        hqlSelect = String.format("from User%s order by login", where);

        try {
            session = this.sessionFactory.openSession();
            session.beginTransaction();

            @SuppressWarnings("unchecked")
            List<User> result = session.createQuery(hqlSelect).list();

            for (User user : result) {
                Map<String, String> userMap = new HashMap<String, String>();

                userMap.put("login", user.getLogin());
                userMap.put("first_name", user.getFirstName());
                userMap.put("last_name", user.getLastName());
                userMap.put("email", user.getEmail());
                userMap.put("permission", user.getPermission());

                users.add(userMap);
            }

            session.getTransaction().commit();

            if ((login != null) && (users.size() == 0)) {
                throw new WebApplicationException(
                    new Exception(String.format("User '%s' does not exists.", login)),
                    Status.NOT_FOUND);
            }
        }
        catch (WebApplicationException e) {
            throw new WebApplicationException(e);
        }
        finally {
            if (session != null) {
                session.close();
            }
        }

        return users;
    }
}
