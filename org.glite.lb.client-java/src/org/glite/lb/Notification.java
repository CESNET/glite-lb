/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.glite.lb;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.rmi.RemoteException;
import java.security.KeyManagementException;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.util.Calendar;
import java.util.Date;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.rpc.holders.CalendarHolder;
import org.glite.jobid.Jobid;
import org.glite.lb.SSL;
import org.glite.wsdl.services.lb.LoggingAndBookkeepingPortType;
import org.glite.wsdl.types.lb.JobFlagsValue;
import org.glite.wsdl.types.lb.JobStatus;
import org.glite.wsdl.types.lb.QueryConditions;
import org.xml.sax.SAXException;

/**
 * This class handles all communication comming from the client toward the server.
 * it uses methods generated from .wsdl description files.
 * note that each instance of this class is dedicated to a single port. Different
 * port means a new instance has to be created
 *
 * @author Kopac
 */
public class Notification {

    private int port = 0;
    private Socket socket = null;
    private static String keyStore;
    private String notifId;
    private LoggingAndBookkeepingPortType stub;
    private LBCredentials lbCredent;

    /**
     * constructor sets the local port number
     *
     * @param port number of the local port the notification is bound to
     * @param lbCredent instance of class that handles SSL authentication
     */
    public Notification(int port, LBCredentials lbCredent) {
        this.port = port;
        this.lbCredent = lbCredent;
    }

    /**
     * a private method used to create a unique ID for a new notification
     *
     * @param host hostname
     * @return String containing the unique ID
     */
    private String makeId(String host) {
        StringBuilder returnString = new StringBuilder();
        returnString.append(host);
        Jobid jobid = new Jobid(returnString.toString(), port);
        returnString.append("/NOTIF:");
        returnString.append(jobid.getUnique());
        return returnString.toString();
    }

    /**
     * returns ID of the latest received notification
     *
     * @return notifID
     */
    public String getNotifid() {
        return notifId;
    }

    /**
     * private method used to recover LB server address from a notif ID
     *
     * @param notifId notif ID
     * @return server address
     */
    private String getServer(String notifId) {
        StringBuilder ret = new StringBuilder(notifId.split("/")[2]);
        char[] ch = new char[]{ret.charAt(ret.length()-1)};
        int i = Integer.parseInt(new String(ch)) + 3;
        ret.replace(ret.length()-1, ret.length()-1, new String()+i);
        ret.insert(0, "https://");
        return ret.toString();
    }

    /**
     * this method sends a request to the server, to create a new notification.
     * it's not necessary to provide all the options for this calling, thus
     * some of the parameters can be null. in that case, the option they correspond to
     * is not used.
     *
     * @param server a String containing the server address (e.g. https://harad.ics.muni.cz:9553).
     * can't be null
     * @param conditions an array of QueryConditions, may contain all the possible
     * conditions for the new notification. can't be null
     * @param flags an array of JobFlagsValue, may contain all the possible flags
     * and their values for the new notification.
     * @param date a Date containing the desired time, the notification will be valid for
     * note that this option can only be used to shorten the validity span, as the server
     * checks it and sets the final expiration date to a constant max, should
     * the provided Date exceed it. may be null
     * @return a Date holding info on how long the new notification will
     * be valid for
     * @throws LBException
     */
    public Date newNotif(String server, QueryConditions[] conditions, JobFlagsValue[] flags, Date date) throws LBException {
        try {
            CalendarHolder calendarHolder = new CalendarHolder(Calendar.getInstance());
            if (date != null) {
                calendarHolder.value.setTime(date);
            } else {
                calendarHolder.value.setTime(new Date(System.currentTimeMillis() + 86400000));
            }
            stub = lbCredent.getStub(server);
            String addr = "0.0.0.0:" + port;
            String id = makeId(server);
            stub.notifNew(id, addr, conditions, flags, calendarHolder);
            notifId = id;
            return calendarHolder.value.getTime();
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }
    }

    /**
     * this method drops an existing notification, removing it completely
     *
     * @param notifId id of the notification to be dropped
     * @throws LBException
     */
    public void drop(String notifId) throws LBException {
        try {
            stub = lbCredent.getStub(getServer(notifId));
            stub.notifDrop(notifId);
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }
    }

    /**
     * this method is used to extend the validity of an existing notification
     *
     * @param notifId id of the notification to be refreshed
     * @param date information about the desired validity duration of the notification
     * in Date format. may be null (in this case, the maximum possible duration is used).
     * @throws LBException
     */
    public void refresh(String notifId, Date date) throws LBException {
        try {
            stub = lbCredent.getStub(getServer(notifId));
            CalendarHolder holder = new CalendarHolder(Calendar.getInstance());
            if (date != null) {
                holder.value.setTime(date);
            } else {
                holder.value.setTime(new Date(System.currentTimeMillis() + 86400000));
            }
            stub.notifRefresh(notifId, holder);
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }
    }

    /**
     * this method is used to bind an existing notification to a different local port
     * than previously declared
     *
     * @param notifId id of th notification
     * @param date optional attribute, can be used to refresh the notification
     * @return length of the validity duration of the notification in Date format
     * @throws LBException
     */
    public Date bind(String notifId, Date date) throws LBException {
        try {
            stub = lbCredent.getStub(getServer(notifId));
            String host = InetAddress.getLocalHost().getHostName() + ":" + port;
            CalendarHolder holder = new CalendarHolder(Calendar.getInstance());
            if (date != null) {
                holder.value.setTime(date);
            } else {
                holder.value.setTime(new Date(System.currentTimeMillis() + 86400000));
            }
            stub.notifBind(notifId, host, holder);
            return holder.value.getTime();
        } catch (RemoteException ex) {
            throw new LBException(ex);
        } catch (UnknownHostException ex) {
            throw new LBException(ex);
        }
    }

    /**
     * this method is used to tell the client to start listening for incomming
     * connections on the local port, with the specified timeout
     *
     * @param timeout read timeout
     * @return comprehensible information, pulled from the received message
     * @throws LBException
     */
    public JobStatus receive(int timeout) throws LBException {
        SSL ssl = new SSL();
        ssl.setCredentials(lbCredent);
        ILProtoReceiver receiver = null;
        String received = null;
        try {
            if(socket == null) {
                socket = ssl.accept(port, timeout);
            }
            receiver = new ILProtoReceiver(socket);
            if((received = receiver.receiveMessage()) == null) {
                socket = ssl.accept(port, timeout);
                receiver = new ILProtoReceiver(socket);
                received = receiver.receiveMessage();
            }
            receiver.sendReply(0, 0, "success");
            NotifParser parser = new NotifParser(received);
            notifId = parser.getNotifId();
            return parser.getJobInfo();
        } catch (IOException ex) {
            throw new LBException(ex);
        } catch (ParserConfigurationException ex) {
            throw new LBException(ex);
        } catch (SAXException ex) {
            throw new LBException(ex);
        }
    }
}
