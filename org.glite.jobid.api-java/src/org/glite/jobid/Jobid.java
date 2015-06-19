/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package org.glite.jobid;

import java.net.UnknownHostException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Calendar;
import java.util.Random;
//import org.apache.commons.codec.binary.Base64;
import org.apache.commons.codec.binary.Base64;

/**
 * Class representing jobId
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 * @version 15. 3. 2008
 */
public class Jobid {
    
    String unique;
    String bkserver;
    int port;
    
    /**
     * Creates new instance of Jobid class.
     */
    public Jobid() {
    }

    /**
     * Creates new instace of JobId with BK server address and port number, unique part 
     * is generated. If some exception is catched during generating the unique part, then
     * System.exit(-1); is called.
     * 
     * @param bkserver BK server address
     * @param port BK server port
     * @throws java.lang.IllegalArgumentException if bkserver is null
     * @throws java.lang.IllegalArgumentException if port is lower than 1 or 
     * bigger than 65535
     */
    
    public Jobid(String bkserver, int port) {
        if (bkserver == null) {
            throw new IllegalArgumentException("Jobid bkserver");
        }
        
        if (port < 1 || port > 65535) {
            throw new IllegalArgumentException("Jobid port");
        }
        
        if (bkserver.indexOf("https://") == -1)
		this.bkserver = "https://" + bkserver;
        else this.bkserver = bkserver;

        this.port = port;
        
        MessageDigest digest = null;
        
        try {
            String hostname = "";
            try {
                hostname = java.net.InetAddress.getLocalHost().getHostName();
            } catch (UnknownHostException ex) {
               System.err.println(ex);
            }
            
            digest = java.security.MessageDigest.getInstance("MD5");
            unique = hostname + bkserver + port + Calendar.getInstance().getTimeInMillis() +
                     new Random().nextInt(999999);
            
            digest.update(unique.getBytes(),0,unique.length());
            Base64 base64 = new Base64();
            byte[] tmp = base64.encode(digest.digest());
            unique = new String(tmp, 0, tmp.length-2);
	    unique = unique.replaceAll("/", "_");
	    unique = unique.replaceAll("\\+", "-");
        } catch (NoSuchAlgorithmException ex) {
            System.err.println(ex);
            System.exit(-1);
        }
    }
    
    /**
     * Creates new instace of Jobid with BK server address, port number and 
     * unique part as parameters. 
     * 
     * @param bkserver BK server address
     * @param port BK server port
     * @param unique unique part of jobid
     * @throws java.lang.IllegalArgumentException if bkserver is null
     * @throws java.lang.IllegalArgumentException if port is lower than 1 or 
     * bigger than 65535
     * @throws java.lang.IllegalArgumentException if unique is null
     */
    public Jobid(String bkserver, int port, String unique) {
        if (bkserver == null) {
            throw new IllegalArgumentException("Jobid bkserver");
        }
        
        if (port < 1 || port > 65535) {
            throw new IllegalArgumentException("Jobid port");
        }
        
        if (unique == null) {
            throw new IllegalArgumentException("Jobid unique");
        }
        
        if (bkserver.indexOf("https://") == -1)
		this.bkserver = "https://" + bkserver;
        else this.bkserver = bkserver;

        this.port = port;
        this.unique = unique;
    }
    
    /**
     * Creates new instace of Jobid from string which represents jobid
     * 
     * @param jobidString jobid string representation
     * @throws java.lang.IllegalArgumentException if jobidString is null
     */
    public Jobid(String jobidString) {
        if (jobidString == null) {
            throw new IllegalArgumentException("Jobid jobidString");
        }
        
        int doubleSlashPosition = jobidString.indexOf("https://");
        if (doubleSlashPosition == -1) {
            throw new IllegalArgumentException("wrong jobid https");
        }
        
        int colonPosition = jobidString.indexOf(":", doubleSlashPosition + 8);
        if (colonPosition == -1) {
            throw new IllegalArgumentException("wrong jobid colon");
        }
        
        int dashAfterPort = jobidString.indexOf("/", colonPosition);
        String bkserverS = jobidString.substring(0, colonPosition);
        Integer portS = new Integer(jobidString.substring(colonPosition+1, 
                dashAfterPort));
        String uniqueS = jobidString.substring(dashAfterPort+1, jobidString.length());
        
        this.bkserver = bkserverS;
        this.port = portS.intValue();
        this.unique = uniqueS;
    }

    /**
     * Returns BK server address
     * 
     * @return bkserver BK server address
     */
    public String getBkserver() {
        return bkserver;
    }

    /**
     * Sets BK server address
     * 
     * @param bkserver BK server address
     * @throws java.lang.IllegalArgumentException if bkserver is null
     */
    public void setBkserver(String bkserver) {
        
        if (bkserver == null) {
            throw new IllegalArgumentException("Jobid bkserver");
        }
        
        this.bkserver = bkserver;
    }

    /**
     * Returns unique part of jobId
     * 
     * @return unique part of jobId
     */
    public String getUnique() {
        return unique;
    }

    /**
     * Sets unique part of jobId
     * 
     * @param unique unique part of jobId
     * @throws java.lang.IllegalArgumentException if unique is null
     */
    public void setUnique(String unique) {
        
        if (unique == null) {
            throw new IllegalArgumentException("Jobid unique");
        }
        
        this.unique = unique;
    }

    /**
     * Returns port number
     * 
     * @return port number
     */
    public int getPort() {
        return port;
    }

    /**
     * Sets port number
     * 
     * @param port number
     * @throws java.lang.IllegalArgumentException if port is lower than 0 or
     * bigger than 65535
     */
    public void setPort(int port) {
        
        if (port <= 0 || port >= 65536) {
            throw new IllegalArgumentException("Jobid port");
        }
        
        this.port = port;
    }
    
    /**
     * Returns Jobid string representation in format bkserver:port/unique
     * 
     * @return Jobid string representation in format bkserver:port/unique
     */
    public String toString() {
        return bkserver + ":" + port + "/" + unique;
    }
}
