/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
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

package cz.cas.asu.stelweb.fuky.observe.server.resources;

import java.util.Map;
import java.util.HashMap;

import javax.ws.rs.core.MultivaluedMap;

public class UserForm {

    public static String[] keys = {
        "login",
        "password",
        "first_name",
        "last_name",
        "email",
        "permission",
    };

    private Map<String, Object> rootMap = new HashMap<String, Object>();
    private boolean error = false;

    public UserForm(MultivaluedMap<String, String> formParams, String login, boolean update, String role) {
        Map<String, String> append = new HashMap<String, String>();
        String value;

        if (formParams.containsKey("password") && formParams.containsKey("confirm_password")) {
            if (!formParams.getFirst("password").equals(formParams.getFirst("confirm_password"))) {
                rootMap.put("message", "Please confirm your password. Passwords are not identical.");
                error = true;
            }
        }

        for (String key : keys) {
            value = formParams.getFirst(key);
            append.put(key, value);

            if (role.equals("user") && key.equals("permission")) {
                continue;
            }

            if ((!role.equals("permission")) && (!error) && ((value == null) || (value.length() == 0))) {
                if ((!update) || (update && (!key.matches("password.*")))) {
                    rootMap.put("message",
                        String.format("Please fill out the form below. Key '%s' is empty.", key));
                    error = true;
                }
            }
        }

        rootMap.put("role", role);
        rootMap.put("login", login);
        rootMap.put("append", append);
        rootMap.put("update", update);
    }

    public Map<String, Object> getRootMap() {
        return rootMap;
    }

    public boolean isBad() {
        return error;
    }

    public void setMessage(String message) {
        rootMap.put("message", message);
    }
}
