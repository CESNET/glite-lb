package org.glite.lb;

/**
 * This class represents sequence code.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 * @version 9. 4. 2008
 */
public class SeqCode {

    public static final int NORMAL = 1;
    public static final int DUPLICATE = 11;
    public static final int PBS = 2;
    public static final int CONDOR = 4;
    public static final int CREAM = 4;
    
    private int[] seqCode = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    private int type = 0;
    
    /**
     * Empty constructor which creates new instance of SeqCode with all values
     * equal 0
     */
    public SeqCode() {
    }
    
    /**
     * Constructor which creates new instance of SeqCode with values set by user
     * in input attribute
     * 
     * @param seqCodeString
     */
    public SeqCode(int type,String seqCodeString) {
	getSeqCodeFromString(type,seqCodeString);
    }

    /**
     * This method increments one specific part of sequence code given by part attribute
     * 
     * @param part part of sequence number which will be increased
     */
    public void incrementSeqCode(int part) {
	switch (type) {
		case NORMAL:
		case DUPLICATE:
		        if (part <= -1 || part >= Sources.EDG_WLL_SOURCE_LB_SERVER)
				throw new IllegalArgumentException("SeqCode part");
       	 		seqCode[part-1]++;
			break;
		default: break;
       	 }
    }
    
    /**
     * Converts string representation of sequence code to format which is used
     * in this class.
     * Insert sequence codes in format:
     * UI=000000:NS=0000000000:WM=000000:BH=0000000000:JSS=000000:LM=000000:LRMS=000000:APP=000000:LBS=000000
     * @param seqCodeString
     */
    public void getSeqCodeFromString(int type,String seqCodeString) {
	switch (type) {
		case NORMAL:
		case DUPLICATE:
			if (!seqCodeString.matches("UI=\\d{1,}:NS=\\d{1,}:WM=\\d{1,}:BH=\\d{1,}:" +
			"JSS=\\d{1,}:LM=\\d{1,}:LRMS=\\d{1,}:APP=\\d{1,}:LBS=\\d{1,}")) {
			throw new IllegalArgumentException("this is not correct sequence code");
			}
			
			int currentPosition = 0;
			int equalsPosition = 0;
			int colonPosition = 0;
			for (int i = 0; i <= 8; i++) {
				equalsPosition = seqCodeString.indexOf('=', currentPosition);
				if (i == 8) {
				colonPosition = seqCodeString.length();
				} else {
				colonPosition = seqCodeString.indexOf(':', currentPosition);
				}
				seqCode[i] = (new Integer(seqCodeString.substring(equalsPosition+1, colonPosition))).intValue();
				currentPosition = colonPosition + 1;
			}
			break;
		case CREAM: break;
		default: throw new IllegalArgumentException("unsupported seqcode type " + type);
	}
	this.type = type;
    }
    
    public String toString() {        
	switch (type) {
		case NORMAL:
		case DUPLICATE:
			String tmp = Integer.toString(seqCode[0]);    
			String output = "UI=";
			output += "000000".substring(0, 6 - tmp.length ()) + tmp;
			output += ":";
			output += "NS=";
			tmp = Integer.toString(seqCode[1]);
			output += "0000000000".substring(0, 10 - tmp.length ()) + tmp;
			output += ":";
			output += "WM=";
			tmp = Integer.toString(seqCode[2]);
			output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
			output += ":";
			output += "BH=";
			tmp = Integer.toString(seqCode[3]);
			output += "0000000000".substring(0, 10 - tmp.length ()) + tmp;
			output += ":";
			output += "JSS=";
			tmp = Integer.toString(seqCode[4]);
			output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
			output += ":";
			output += "LM=";
			tmp = Integer.toString(seqCode[5]);
			output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
			output += ":";
			output += "LMRS=";
			tmp = Integer.toString(seqCode[6]);
			output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
			output += ":";
			output += "APP=";
			tmp = Integer.toString(seqCode[7]);
			output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
			output += ":";
			output += "LBS=";
			tmp = Integer.toString(seqCode[8]);
			output += "0000000000".substring(0, 6 - tmp.length ()) + tmp;
			return output;
		case CREAM: 
			return "no_seqcodes_with_cream";
		default:
			throw new IllegalArgumentException("unitialized seqcode");
	}
    }
}
