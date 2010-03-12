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

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

import org.glite.lb.Notification;
import org.glite.lb.LBCredentials;
import org.glite.lb.LBException;

import java.util.Date;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.glite.wsdl.types.lb.JobStatus;
import org.glite.wsdl.types.lb.QueryAttr;
import org.glite.wsdl.types.lb.QueryConditions;
import org.glite.wsdl.types.lb.QueryOp;
import org.glite.wsdl.types.lb.QueryRecValue;
import org.glite.wsdl.types.lb.QueryRecord;
import org.glite.wsdl.types.lb.StatName;

/**
 * This is an example on how to use the package org.glite.lb.notif_java
 *
 *
 * @author Kopac
 */
public class NotificationExample {

    /**
     * example method on how to use this package
     *
     * @param args parameters for the methods, order as follows:
     * <p>
     * 1. path to user's certificate
     * <p>
     * 2. path to a list of trusted certificates
     * <p>
     * 3. owner ID
     * <p>
     * 4. port number
     * <p>
     * 5. LB server address
     */
    public static void main(String[] args){
        try {
            
            //creation of an instance of class that prepares SSL connection for 
            //webservice, first of the two possibilities is used
            LBCredentials credentials = new LBCredentials(args[0], args[1]);

            //creation of QueryConditions instance. this one tells the notification to
            //be associated with jobs of an owner and that don't have a tag. meh
            QueryRecValue value1 = new QueryRecValue(null, args[2], null);
            QueryRecord[] record = new QueryRecord[]{new QueryRecord(QueryOp.EQUAL, value1, null)};
            QueryConditions[] conditions = new QueryConditions[]{new
                    QueryConditions(QueryAttr.OWNER, null, StatName.SUBMITTED, record)};

            //creating an instance of NotificationImpl, the argument is port number
            Notification notif = new Notification(Integer.parseInt(args[3]), credentials);

            Date date = new Date(System.currentTimeMillis()+86400000);

            //registering a new notification, first parameter is LB server address in String,
            //second previously created QueryConditions, there are no JobFlags provided
            //and the last one tells the server to keep it active for two days
            notif.newNotif(args[4], conditions, null, date);

            //id of the newly created notification
            String notifId = notif.getNotifid();

            //refreshing the previously created notification by two days
            notif.refresh(notifId, date);

            //tells the client to read incomming notifications with a timeout of 1 minute
            JobStatus state = notif.receive(60000);
            //from here on, state can be used to pick the desired info using its
            //get methods

            //like this
            System.out.println(state.getAcl());
            System.out.println(state.getCancelReason());
            System.out.println(state.getJobId());

            //creates a new instance of NotificationImpl with the port 14342 and
            //binds the notification to this new instance, meaning to a new local
            //address
            Notification notif2 = new Notification(14342, credentials);
            notif2.bind(notifId, date);

            //lastly, the notification is dropped
            notif2.drop(notifId);

        } catch (LBException ex) {
            Logger.getLogger(NotificationExample.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
