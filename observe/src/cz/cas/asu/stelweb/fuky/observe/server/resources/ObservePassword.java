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

package cz.cas.asu.stelweb.fuky.observe.server.resources;

import java.math.BigInteger;
import java.util.Random;
import java.util.logging.Level;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import org.apache.commons.codec.binary.Hex;
import javax.ws.rs.WebApplicationException;

public class ObservePassword {
    private String passwordDigest;
    private String salt;

    public ObservePassword(String password) {
        make(password);
    }

    public ObservePassword(String password, String salt) {
        if (salt == null) {
            Random rnd = new Random(System.currentTimeMillis());
            BigInteger bi = new BigInteger(64, rnd);

            salt = bi.toString(16);
        }

        make(password, salt);
    }

    private void make(String password) {
        make(password, null);
    }

    private void make(String password, String salt) {
        try {
            String password_salt;
            //MessageDigest md = MessageDigest.getInstance("SHA-512");
            MessageDigest md = MessageDigest.getInstance("MD5");

            if (salt == null) {
                password_salt = password;
            }
            else {
                password_salt = String.format("%s:%s", password, salt);
            }

            byte[] digest = md.digest(password_salt.getBytes("UTF-8"));

            this.passwordDigest = Hex.encodeHexString(digest);
            this.salt = salt;
        }
        catch (Exception e) {
            throw new WebApplicationException(e);
        }
    }

    public String getPasswordDigest() {
        return this.passwordDigest;
    }

    public String getSalt() {
        return this.salt;
    }
}
